#include "html-rd.hpp"
#include <cassert>
#include <vector>
#include <fstream>

using namespace std;
using namespace html;


// ----------------------------------- [ Constants ] ---------------------------------------- //


constexpr int PAGE_SIZE = 64;

constexpr uint64_t MASK_EOF      = (uint64_t(1) << '\0');	// \0
constexpr uint64_t MASK_SPACE    = (uint64_t(1) << ' ');	// [ ]
constexpr uint64_t MASK_TAB      = (uint64_t(1) << '\t');	// \t
constexpr uint64_t MASK_CR       = (uint64_t(1) << '\r');	// \r
constexpr uint64_t MASK_LF       = (uint64_t(1) << '\n');	// \n
constexpr uint64_t MASK_QUOTE_1  = (uint64_t(1) << '\'');	// '
constexpr uint64_t MASK_QUOTE_2  = (uint64_t(1) << '"');	// "
constexpr uint64_t MASK_QUESTION = (uint64_t(1) << '?');	// ?
constexpr uint64_t MASK_MINUS    = (uint64_t(1) << '-');	// -
constexpr uint64_t MASK_SLASH    = (uint64_t(1) << '/');	// /
constexpr uint64_t MASK_LT       = (uint64_t(1) << '<');	// <
constexpr uint64_t MASK_GT       = (uint64_t(1) << '>');	// >

constexpr uint64_t MASK_WHITESPACE = MASK_SPACE | MASK_TAB | MASK_CR | MASK_LF;	// [ ] \t \r \n
constexpr uint64_t MASK_QUOTE = MASK_QUOTE_1 | MASK_QUOTE_2;	// ' "


// ----------------------------------- [ Structures ] --------------------------------------- //


namespace html {
struct Parser {
	const_allocator<rd_node>* nodeAlloc;
	const_allocator<rd_attr>* attrAlloc;
	
	html::rd_node* parent;				// Current parsing parent node.
	vector<html::rd_node*> lastChild;	// Stack of last child of `current`.
	
	parse_result result;
};
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline rd_attr& allocAttr(Parser& state){
	rd_attr* attr = state.attrAlloc->alloc();
	if (attr != nullptr)
		return *attr;
	throw parse_status::MEMORY;
}


inline rd_node& addChild(Parser& state, node_type type){
	rd_node* node = state.nodeAlloc->alloc();
	if (node == nullptr){
		throw parse_status::MEMORY;
	}
	
	node->type = type;
	node->parent = state.parent;
	
	// Append sibling
	{
		rd_node*& prev_sibling = state.lastChild.back();
		
		if (prev_sibling != nullptr)
			prev_sibling->next = node;
		else
			node->parent->child = node;
			
		prev_sibling = node;
	}
	
	return *node;
}


inline rd_node& pushNew(Parser& state){
	rd_node& node = addChild(state, node_type::TAG);
	state.lastChild.emplace_back(node.child);
	state.parent = &node;
	return node;
}


inline rd_node& pop(Parser& state){
	state.parent = state.parent->parent;
	state.lastChild.pop_back();
	return *state.parent;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr bool isMasked(auto i, uint64_t mask){
	return ((uint64_t(1) << i) & mask) != 0;
}

constexpr bool isWhitespace(char c){
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

constexpr bool isQuote(char c){
	return c == '\'' || c == '"';
}

// [a-zA-Z:_-]
constexpr bool isTagChar(char c){
	constexpr uint64_t MASK = []() consteval {
		uint64_t m = 0;
		m |= (uint64_t(1) << (':' & 0b00111111));
		m |= (uint64_t(1) << ('-' & 0b00111111));
		m |= (uint64_t(1) << ('_' & 0b00111111));
		
		for (char c = 'A' ; c <= 'Z' ; c++)
			m |= (uint64_t(1) << (c & 0b00111111));
		for (char c = 'a' ; c <= 'z' ; c++)
			m |= (uint64_t(1) << (c & 0b00111111));
		
		return m;
	}();
	
	uint8_t b2 = c >> 6;
	uint8_t b6 = c & 0b00111111;
	return ((b2 == 0b01) & isMasked(b6, MASK));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline const char* parse_whiteSpace(Parser& state, const char* s){
	while (isWhitespace(*s)){
		if (*s == '\n')
			state.result.row++;
		s++;
	}
	return s;
}


inline const char* parse_text(Parser& state, const char* s){
	while (true){
		if (*s < 64 && isMasked(*s, MASK_LF | MASK_LT | MASK_EOF)){
			if (*s == '\n')
				state.result.row++;
			else if (*s == '<' || *s == 0)
				break;
		}
		s++;
	}
	return s;
}


inline const char* parse_string(Parser& state, const char* s){
	assert(isQuote(*s));
	const char quote = *s;
	s++;
	
	while (true){
		if (*s < 64 && isMasked(*s, MASK_LF | MASK_QUOTE | MASK_EOF)){
			if (*s == '\n')
				state.result.row++;
			else if (*s == quote)
				break;
			else if (*s == 0){
				state.result.s = s;
				throw parse_status::UNCLOSED_STRING;
			}
		}
		s++;
	}
	
	return s + 1;
}


static const char* parse_question(Parser& state, const char* s){
	assert(*s == '?');
	const char* beg = ++s;
	
	while (true){
		if (*s < 64 && isMasked(*s, MASK_QUOTE | MASK_LF | MASK_QUESTION | MASK_EOF)){
			if (isMasked(*s, MASK_QUOTE))
				s = parse_string(state, s) - 1;
			else if (isMasked(*s, MASK_QUESTION | MASK_EOF))
				break;
			else if (*s == '\n')
				state.result.row++;
		}
		s++;
	}
	
	if (s[0] != '?' || s[1] != '>'){
		state.result.s = s;
		throw parse_status::UNCLOSED_QUESTION;
	}
	
	rd_node& node = addChild(state, node_type::PI);
	node.value_len = uint32_t(s - beg);
	node.value_p = beg;
	return s + 2;
}


static const char* parse_comment(Parser& state, const char* s){
	assert(s[0] == '!' && s[1] == '-' && s[2] == '-');
	const char* beg = s + 1;
	s += 3;
	
	while (s[0] != 0 && !(s[0] == '-' && s[1] == '-' && s[2] == '>')){
		s++;
	}
	
	if (*s == 0){
		state.result.s = s;
		throw parse_status::UNCLOSED_COMMENT;
	}
	
	rd_node& node = addChild(state, node_type::COMMENT);
	node.value_len = uint32_t(s - beg + 2);
	node.value_p = beg;
	return s + 3;
}


static const char* parse_exclamation(Parser& state, const char* s){
	assert(s[0] == '!');
	
	// Parse comment
	if (s[1] == '-' && s[2] == '-'){
		return parse_comment(state, s);
	}
	
	// Parse directive
	const char* beg = s;
	
	while (true){
		if (*s < 64 && isMasked(*s, MASK_QUOTE | MASK_LF | MASK_GT | MASK_EOF)){
			if (*s == '>')
				break;
			else if (isMasked(*s, MASK_QUOTE))
				s = parse_string(state, s);
			else if (*s == '\n')
				state.result.row++;
			else if (*s == 0){
				state.result.s = s;
				throw parse_status::UNCLOSED_TAG;
			}
		}
		s++;
	}
	
	rd_node& node = addChild(state, node_type::DIRECTIVE);
	node.value_len = uint32_t(s - beg);
	node.value_p = beg;
	return s + 1;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static const char* parse_attribute(Parser& state, const char* s, rd_attr& attr){
	assert(isTagChar(s[0]));
	
	// Parse name
	attr.name_p = s;
	while (isTagChar(*(++s))) continue;
	attr.name_len = uint32_t(s - attr.name_p);
	
	// Check for value
	s = parse_whiteSpace(state, s);
	if (s[0] != '=')
		return s;
	s = parse_whiteSpace(state, s + 1);
	
	if (!isQuote(*s)){
		throw parse_status::MISSING_ATTR_VALUE;
	}
	
	// Parse value
	attr.value_p = s + 1;
	s = parse_string(state, s);
	attr.name_len = uint32_t(s - attr.value_p - 1);
	
	return s;
}


static const char* parse_openTag(Parser& state, const char* s){
	if (!isTagChar(*s)){
		throw parse_status::INVALID_TAG_NAME;
	}
	
	// Parse name
	const char* name_p = s;
	while (isTagChar(*(++s))) continue;
	const uint32_t name_len = uint32_t(s - name_p);
	
	rd_attr* firstAttr = nullptr;
	rd_attr* lastAttr = nullptr;
	
	// Self close
	if (s[0] == '/'){ tag_self_close:
		if (s[1] == '>'){
			rd_node& node = addChild(state, node_type::TAG);
			node.value_p = name_p;
			node.value_len = name_len;
			node.attribute = firstAttr;
			return s + 2; 
		} else {
			state.result.s = s;
			throw parse_status::MISSING_TAG_GT;
		}
	}
	
	// End
	else if (s[0] == '>'){ tag_end:
		rd_node& node = pushNew(state);
		node.value_p = name_p;
		node.value_len = name_len;
		node.attribute = firstAttr;
		return s + 1;
	}
	
	// Check for attributes
	else if (!isWhitespace(s[0])){
		state.result.s = s;
		throw (s[0] == 0) ? parse_status::MISSING_TAG_GT : parse_status::INVALID_TAG_NAME;
	}
	
	while (true){
		s = parse_whiteSpace(state, s);
		
		// Self-close
		if (s[0] == '/'){
			goto tag_self_close;
		} else if (s[0] == '>'){
			goto tag_end;
		}
		
		else if (!isTagChar(s[0])){
			state.result.s = s;
			throw parse_status::INVALID_TAG_CHAR;
		}
		
		// Attribute
		rd_attr& attr = allocAttr(state);
		s = parse_attribute(state, s, attr);
		
		if (firstAttr == nullptr){
			firstAttr = lastAttr = &attr;
		} else {
			lastAttr->next = &attr;
			lastAttr = &attr;
		}
		
		continue;
	}
	
	state.result.s = s;
	throw parse_status::UNCLOSED_TAG;
}


static const char* parse_closeTag(Parser& state, const char* s){
	assert(s[0] == '/');
	s++;
	
	uint32_t len = state.parent->value_len;
	const char* name = state.parent->value_p;
	
	if (len <= 0){
		state.result.s = s;
		throw parse_status::INVALID_TAG_CLOSE;
	} else {
		assert(name != nullptr);
	}
	
	while (len > 0){
		if (*s != *name){
			state.result.s = s;
			throw parse_status::INVALID_TAG_CLOSE;
		}
		len--;
		s++;
		name++;
	}
	
	s = parse_whiteSpace(state, s);
	
	if (*s != '>'){
		state.result.s = s;
		throw parse_status::INVALID_TAG_CLOSE;
	}
	
	pop(state);
	return s + 1;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void parse_all(Parser& state, const char* s){
	while (true){
		const char* beg = s;
		s = parse_whiteSpace(state, s);
		
		check_tag:
		if (*s != '<'){
			if (*s == 0){
				break;
			}
			
			// Text
			s = parse_text(state, s + 1);
			
			rd_node& node = addChild(state, node_type::TEXT);
			node.value_p = beg;
			node.value_len = uint32_t(s - beg);
			goto check_tag;
		}
		
		// Close tag
		else if (s[1] == '/'){
			s = parse_closeTag(state, s + 1);
			continue;
		}
		
		// Tag
		else if (isTagChar(s[1])) [[likely]] {
			s = parse_openTag(state, s + 1);
			continue;
		}
		
		else if (s[1] == '?'){
			s = parse_question(state, s+1);
			continue;
		}
		
		else if (s[1] == '!'){
			s = parse_exclamation(state, s+1);
			continue;
		}
		
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool readFile(const char* path, rd_document& doc){
	ifstream in = ifstream(path);
	if (!in){
		return false;
	}
	
	if (!in.seekg(0, ios_base::end)){
		return false;
	}
	
	const streampos pos = in.tellg();
	if (pos < 0 || !in.seekg(0, ios_base::beg)){
		return false;
	}
	
	const size_t size = size_t(pos);
	size_t total = 0;
	doc.buffer = make_unique<char[]>(size + 1);
	
	while (total < size && in.read(doc.buffer.get(), size - total)){
		
		const streamsize n = in.gcount();
		if (n <= 0){
			break;
		}
		
		total += size_t(n);
	}
	
	if (total <= size){
		doc.buffer_len = total;
	}
	
	doc.buffer[total] = 0;
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


parse_result html::parse(const char* path){
	rd_document doc;
	Parser parser;
	
	try {
		if (!readFile(path, doc)){
			throw parse_status::IO;
		}
		
		parser = {
			.nodeAlloc = &doc.nodeAlloc,
			.attrAlloc = &doc.attrAlloc,
			.parent = &doc,
			.lastChild = { nullptr },			// Initial root child.
			.result = {
				.s = doc.buffer.get(),
				.row = 1
			}
		};
		
		parse_all(parser, doc.buffer.get());
		parser.result.status = parse_status::OK;
		
	} catch (const bad_alloc&){
		parser.result.status = parse_status::MEMORY;
	} catch (const parse_status& err){
		parser.result.status = err;
	} catch (...){
		parser.result.status = parse_status::ERROR;
	}
	
	return parser.result;
}


// ------------------------------------------------------------------------------------------ //