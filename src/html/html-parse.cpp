#include "html.hpp"
#include <vector>
#include <fstream>

using namespace std;
using namespace html;


// ----------------------------------- [ Constants ] ---------------------------------------- //


constexpr int PAGE_SIZE = 64;

constexpr uint64_t MASK_EOF      = (uint64_t(1) << '\0');	// \0
constexpr uint64_t MASK_QUOTE_1  = (uint64_t(1) << '\'');	// '
constexpr uint64_t MASK_QUOTE_2  = (uint64_t(1) << '"');	// "
constexpr uint64_t MASK_QUESTION = (uint64_t(1) << '?');	// ?
constexpr uint64_t MASK_LT       = (uint64_t(1) << '<');	// <
constexpr uint64_t MASK_GT       = (uint64_t(1) << '>');	// >

constexpr uint64_t MASK_QUOTE = MASK_QUOTE_1 | MASK_QUOTE_2;	// ' "


// ----------------------------------- [ Structures ] --------------------------------------- //


struct Parser {
	html::node* current;			// Current parsing parent node.
	vector<html::node*> lastChild;	// Stack of last child of `current`.
	html::node* macros = nullptr;	// Linked list of macro nodes.
	parse_result result;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline attr* allocAttr(){
	html::attr* a = html::newAttr();
	a->options |= node_options::LIST_FORWARDS;
	return a;
}

inline node* allocNode(node_type type){
	html::node* node = html::newNode();
	node->type = type;
	node->options |= node_options::LIST_FORWARDS;
	return node;
}


inline node* addChild(Parser& state, node* node){
	node->parent = state.current;
	
	// Append sibling
	html::node*& prev_sibling = state.lastChild.back();
	if (prev_sibling != nullptr)
		prev_sibling->next = node;
	else
		node->parent->child = node;

	prev_sibling = node;
	return node;
}


inline node* addMacro(Parser& state, node* macro){
	macro->parent = state.current;
	macro->next = state.macros;
	state.macros = macro;
	return macro;
}


inline node* push(Parser& state, node* node){
	state.lastChild.emplace_back(node->child);
	state.current = node;
	return node;
}


inline void pop(Parser& state){
	state.current = state.current->parent;
	state.lastChild.pop_back();
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


long document::row(const char* const p) const noexcept {
	const char* b = buffer;
	if (b == nullptr || p == nullptr || p < b){
		return -1;
	}
	
	long row = 1;
	while (p > b){
		if (*b == '\n')
			row++;
		else if (*b == 0)
			return -1;
		b++;
	}
	
	return row;
}


long document::col(const char* const p) const noexcept {
	const char* b = buffer;
	if (b == nullptr || p == nullptr || p < b){
		return -1;
	}
	
	bool tabs = false;
	
	// Find begining of line
	const char* beg = p;
	while (beg > b){
		tabs |= (*beg == '\t');
		if (beg[-1] == '\n')
			break;
		beg--;
	}
	
	// No tabs
	if (!tabs){
		return p - beg + 1;
	}
	
	// Find colum number accounting tabs
	long col = 1;
	while (beg != p){
		col++;
		if (*beg == '\t')
			col = ((col + 2) & ~0b11L) + 1;
		beg++;
	}
	
	return col;
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
	constexpr uint64_t MASK_1 = []() consteval {
		uint64_t m = 0;
		m |= (uint64_t(1) << (':' & 0b00111111));
		m |= (uint64_t(1) << ('-' & 0b00111111));
		
		for (char c = '0' ; c <= '9' ; c++)
			m |= (uint64_t(1) << (c & 0b00111111));
		
		return m;
	}();
	constexpr uint64_t MASK_2 = []() consteval {
		uint64_t m = 0;
		m |= (uint64_t(1) << ('_' & 0b00111111));
		
		for (char c = 'A' ; c <= 'Z' ; c++)
			m |= (uint64_t(1) << (c & 0b00111111));
		for (char c = 'a' ; c <= 'z' ; c++)
			m |= (uint64_t(1) << (c & 0b00111111));
		
		return m;
	}();
	
	if (c < 64){
		return isMasked(c, MASK_1);
	} else {
		uint8_t b2 = c >> 6;
		uint8_t b6 = c & 0b00111111;
		return ((b2 == 0b01) & isMasked(b6, MASK_2));
	}
	
}


constexpr bool isVoidTag(string_view name){
	switch (name.length()){
		case 2:
			return name == "br" || name == "hr";
		case 3:
			return name == "img" || name == "col" || name == "wbr";
		case 4:
			return name == "link" || name == "meta" || name == "area" || name == "base";
		case 5:
			return name == "input" || name == "embed" || name == "param" || name == "track";
		case 6:
			return name == "source";
	}
	return false;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline const char* parse_whiteSpace(Parser& state, const char* s){
	while (isWhitespace(*s))
		s++;
	return s;
}


static const char* parse_pcData(Parser& state, const char* beg, const char* s){
	node_options opts = {};
	
	// Parse all plaintext.
	while (true){
		if (*s == '<' || *s == 0)
			break;
		else if (*s == '{')
			opts = node_options::INTERPOLATE;
		s++;
	}
	
	// Append text nodes
	while (beg != s){
		node* txt = addChild(state, allocNode(node_type::TEXT));
		txt->options |= opts;
		
		// Text chunk
		size_t len = min(size_t(s - beg), size_t(UINT32_MAX));
		txt->value_len = uint32_t(len);
		txt->value_p = beg;
		
		beg += len;
	}
	
	return s;
}


static const char* parse_rawpcData(Parser& state, node* parent, const char* s){
	const char* name = parent->value_p;
	size_t len = parent->value_len;
	assert(len > 0 && name != nullptr);
	
	const char* beg = s;
	const char* end = s;
	
	// Parse all plaintext untill
	while (true){ loop:
		if (s[0] == '<'){
			if (s[1] == '/')
				goto check_end_tag;
		} else if (*s == 0){
			state.result.pos = name;
			throw parse_status::MISSING_END_TAG;
		}
		
		s++;
		continue;
		
		// Check for </tag>
		check_end_tag:
		end = s;
		s += 2;
		
		// Compare names
		for (size_t i = 0 ; i < len ; i++, s++){
			if (*s != name[i])
				goto loop;
		}
		
		s = parse_whiteSpace(state, s);
		if (*s == '>'){
			break;
		}
		
		state.result.pos = end;
		throw parse_status::MISSING_END_TAG;
	}
	
	if (end == beg){
		return s + 1;
	}
	
	assert(parent->child == nullptr);
	size_t totalLen = size_t(end - beg);
	
	// Append text nodes back to front
	while (totalLen > 0){
		node* txt = allocNode(node_type::TEXT);
		txt->prev = parent->child;
		parent->child = txt;
		txt->parent = parent;
		
		// Chunk of text
		size_t len = min(totalLen, size_t(UINT32_MAX));
		txt->value_len = uint32_t(len);
		txt->value_p = end - len;
		
		totalLen -= len;
		end -= len;
	}
	
	return s + 1;
}


inline const char* parse_string(Parser& state, const char* s, node_options& out_opts){
	assert(isQuote(*s));
	const char* beg = s;
	const char quote = *beg;
	
	s++;
	while (*s != quote){
		if (*s == '{')
			out_opts |= node_options::INTERPOLATE;
		else if (*s == 0){
			state.result.pos = beg;
			throw parse_status::UNCLOSED_STRING;
		}
		s++;
	}
	
	return s + 1;
}


static const char* parse_question(Parser& state, const char* s){
	assert(*s == '?');
	const char* beg = ++s;
	node_options __trash;
	
	while (true){
		if (*s < 64 && isMasked(*s, MASK_QUOTE | MASK_QUESTION | MASK_EOF)){
			if (isMasked(*s, MASK_QUOTE))
				s = parse_string(state, s, __trash) - 1;
			else if (isMasked(*s, MASK_QUESTION | MASK_EOF))
				break;
		}
		s++;
	}
	
	if (s[0] != '?' || s[1] != '>'){
		state.result.pos = beg - 1;
		throw parse_status::UNCLOSED_QUESTION;
	}
	
	node* node = addChild(state, allocNode(node_type::PI));
	node->value_len = uint32_t(min(s - beg, long(UINT32_MAX)));
	node->value_p = beg;
	return s + 2;
}


static const char* parse_comment(Parser& state, const char* s){
	assert(s[0] == '!' && s[1] == '-' && s[2] == '-');
	const char* beg = s;
	s += 3;
	
	while (s[0] != '-' || s[1] != '-' || s[2] != '>'){
		if (s[0] == 0){
			state.result.pos = beg;
			throw parse_status::UNCLOSED_COMMENT;
		}
		s++;
	}
	
	// -->
	s += 2;
	
	node* node = addChild(state, allocNode(node_type::COMMENT));
	node->value_len = uint32_t(min(s - beg, long(UINT32_MAX)));
	node->value_p = beg;
	return s + 1;
}


static const char* parse_exclamation(Parser& state, const char* s){
	assert(s[0] == '!');
	
	// Parse comment
	if (s[1] == '-' && s[2] == '-'){
		return parse_comment(state, s);
	}
	
	// Parse directive
	const char* beg = s;
	node_options __trash;
	
	while (true){
		if (*s < 64 && isMasked(*s, MASK_QUOTE | MASK_GT | MASK_EOF)){
			if (*s == '>')
				break;
			else if (isMasked(*s, MASK_QUOTE))
				s = parse_string(state, s, __trash);
			else if (*s == 0){
				state.result.pos = beg;
				throw parse_status::UNCLOSED_TAG;
			}
		}
		s++;
	}
	
	node* node = addChild(state, allocNode(node_type::DIRECTIVE));
	node->value_len = uint32_t(min(s - beg, long(UINT32_MAX)));
	node->value_p = beg;
	return s + 1;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static const char* parse_attribute(Parser& state, const char* s, attr& attr){
	assert(isTagChar(s[0]));
	
	// Parse name
	attr.name_p = s;
	while (isTagChar(*(++s))) continue;
	attr.name_len = uint16_t(min(s - attr.name_p, long(UINT16_MAX)));
	
	// Check for value
	s = parse_whiteSpace(state, s);
	if (s[0] != '=')
		return s;
	s = parse_whiteSpace(state, s + 1);
	
	// Parse value
	if (isQuote(*s)){
		attr.value_p = s + 1;
		s = parse_string(state, s, attr.options);
		attr.value_len = uint32_t(min(s - attr.value_p - 1, long(UINT32_MAX)));
	} else {
		state.result.pos = attr.name_p + attr.name_len;
		throw parse_status::MISSING_ATTR_VALUE;
	}
	
	return s;
}


static const char* parse_openTag(Parser& state, const char* s){
	if (!isTagChar(*s)){
		throw parse_status::INVALID_TAG_NAME;
	}
	
	// Parse name
	const char* name_p = s;
	while (isTagChar(*(++s))) continue;
	const uint32_t name_len = uint32_t(min(s - name_p, long(UINT32_MAX)));
	
	attr* firstAttr = nullptr;
	attr* lastAttr = nullptr;
	
	// Self close
	if (s[0] == '/'){ tag_self_close:
		if (s[1] != '>'){
			state.result.pos = s;
			throw parse_status::UNCLOSED_TAG_GT;
		}
		
		html::node* node = allocNode(node_type::TAG);
		node->value_p = name_p;
		node->value_len = name_len;
		node->attribute = firstAttr;
		
		if (string_view(name_p, name_len) == "MACRO"){
			addMacro(state, node);
		} else {
			addChild(state, node);
		}
		
		return s + 2; 
	}
	
	// End
	else if (s[0] == '>'){ tag_end:
		html::node* node = allocNode(node_type::TAG);
		node->value_p = name_p;
		node->value_len = name_len;
		node->attribute = firstAttr;
		
		const string_view name = string_view(name_p, name_len);
		switch (name.length()){
			case 2:
				if (name == "br" || name == "hr")
					goto void_tag;
				else
					goto regular_tag;
			case 3:
				if (name == "img" || name == "col" || name == "wbr")
					goto void_tag;
				else if (name == "pre")
					return parse_rawpcData(state, node, s+1);
				else
					goto regular_tag;
			case 4:
				if (name == "link" || name == "meta" || name == "area" || name == "base")
					goto void_tag;
				else
					goto regular_tag;
			case 5:
				if (name == "style")
					return parse_rawpcData(state, node, s+1);
				else if (name == "input" || name == "embed" || name == "param" || name == "track")
					goto void_tag;
				else if (name == "MACRO")
					goto macro_tag;
				else
					goto regular_tag;
			case 6:
				if (name == "script")
					return parse_rawpcData(state, node, s+1);
				else if (name == "source")
					goto void_tag;
				else
					goto regular_tag;
			default:
				goto regular_tag;
		}
		
		macro_tag:
		push(state, addMacro(state, node));
		return s + 1;
		
		regular_tag:
		push(state, addChild(state, node));
		return s + 1;
		
		void_tag:
		addChild(state, node);
		return s + 1;
	}
	
	// Check for attributes
	else if (!isWhitespace(s[0])){
		state.result.pos = s;
		throw (s[0] == 0) ? parse_status::UNCLOSED_TAG_GT : parse_status::INVALID_TAG_NAME;
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
			state.result.pos = s;
			throw parse_status::INVALID_TAG_CHAR;
		}
		
		// Attribute
		attr* attr = allocAttr();
		s = parse_attribute(state, s, *attr);
		
		if (firstAttr == nullptr){
			firstAttr = lastAttr = attr;
		} else {
			lastAttr->next = attr;
			lastAttr = attr;
		}
		
		continue;
	}
	
	// Error
	state.result.pos = s;
	throw parse_status::UNCLOSED_TAG;
}


static const char* parse_closeTag(Parser& state, const char* s){
	assert(s[0] == '/');
	uint32_t len = state.current->value_len;
	const char* name = state.current->value_p;
	const char* beg = s;
	
	s++;
	while (len > 0){
		if (*s != *name)
			goto check_void_tag;
		len--;
		s++;
		name++;
	}
	
	s = parse_whiteSpace(state, s);
	if (*s == '>'){
		pop(state);
		return s + 1;
	}
	
	check_void_tag:
	// Parse tag name
	s = beg;
	while (isTagChar(*(++s))) continue;
	
	// Discard void tag
	if (isVoidTag(string_view(beg + 1, s))){
		s = parse_whiteSpace(state, s);
		if (*s == '>')
			return s + 1;
	}
	
	// Error
	state.result.pos = s;
	throw parse_status::INVALID_TAG_CLOSE;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


const char* parse_all(Parser& state, const char* s){
	while (true){
		const char* beg = s;
		s = parse_whiteSpace(state, s);
		
		check_tag:
		if (*s != '<'){
			if (*s == 0){
				break;
			} else {
				s = parse_pcData(state, beg, s);
				goto check_tag;
			}
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
	
	// Check if parsed completely
	if (state.current->parent != nullptr){
		if (state.current->value_p != nullptr)
			state.result.pos = state.current->value_p;
		else
			state.result.pos = s;
		throw parse_status::MISSING_END_TAG;
	}
	
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static parse_result parse(node& root, const char* buff){
	Parser parser;
	
	try {
		parser = {
			.current = &root,
			.lastChild = { nullptr },			// Initial root child.
			.result = {
				.status = parse_status::OK,
				.pos = buff
			}
		};
		parse_all(parser, buff);
	} catch (const bad_alloc&){
		parser.result.status = parse_status::MEMORY;
	} catch (const parse_status& err){
		parser.result.status = err;
	} catch (...){
		parser.result.status = parse_status::ERROR;
	}
	
	// TEMP
	node* m = parser.macros;
	while (m != nullptr){
		node* next = m->next;
		html::del(m);
		m = next;
	}
	
	return parser.result;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


parse_result document::parseBuff(const char* buff){
	reset();
	this->buffer = buff;
	this->buffer_owned = false;
	return parse(*this, buff);
}


static char* readFile(const char* path){
	ifstream in = ifstream(path);
	if (!in){
		return nullptr;
	}
	
	if (!in.seekg(0, ios_base::end)){
		return nullptr;
	}
	
	const streampos pos = in.tellg();
	if (pos < 0 || !in.seekg(0, ios_base::beg)){
		return nullptr;
	}
	
	const size_t size = size_t(pos);
	size_t total = 0;
	char* buff = (char*)::operator new (size + 1);
	
	while (total < size && in.read(buff, size - total)){
		
		const streamsize n = in.gcount();
		if (n <= 0){
			break;
		}
		
		total += size_t(n);
	}
	
	buff[total] = 0;
	return buff;
}


parse_result document::parseFile(const char* path){
	char* buff = readFile(path);
	
	if (buff == nullptr){
		return parse_result {
			.status = parse_status::IO
		};
	}
	
	reset();
	this->buffer = buff;
	this->buffer_owned = true;
	return parse(*this, buff);
}


// ------------------------------------------------------------------------------------------ //