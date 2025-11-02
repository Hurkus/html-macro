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

constexpr uint64_t MASK_QUOTE = MASK_QUOTE_1 | MASK_QUOTE_2;	// ['"]


// ----------------------------------- [ Structures ] --------------------------------------- //


struct Parser {
	Node* current;				// Current parsing parent node.
	vector<Node*> lastChild;	// Stack of last child of `current`.
	vector<Node*>* macros;
	ParseResult result;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


const char* html::errstr(ParseStatus status) noexcept {
	switch (status){
		case ParseStatus::OK:
			return "";
		case ParseStatus::UNCLOSED_TAG:
			return "Missing closing bracket '>'.";
		case ParseStatus::UNCLOSED_STRING:
			return "Unterminated string. Multiline strings are not permitted.";
		case ParseStatus::UNCLOSED_QUESTION:
			return "Missing end bracket '?>' for processing instruction.";
		case ParseStatus::UNCLOSED_COMMENT:
			return "Missing end bracket '-->' for comment.";
		case ParseStatus::INVALID_TAG_NAME:
			return "Invalid character in tag name.";
		case ParseStatus::INVALID_TAG_CHAR:
			return "Invalid character in tag.";
		case ParseStatus::MISSING_END_TAG:
			return "Missing end tag.";
		case ParseStatus::INVALID_END_TAG:
			return "Invalid end tag doesn't close any opened tag.";
		case ParseStatus::MISSING_ATTR_VALUE:
			return "Missing value of attribute.";
		case ParseStatus::MEMORY:
			return "Out of memory.";
		case ParseStatus::IO:
			return "Failed to read file.";
		case ParseStatus::ERROR:
			return "Internal error.";
	}
	assert(false);
	return nullptr;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline Attr* allocAttr(){
	html::Attr* a = html::newAttr();
	a->options |= NodeOptions::LIST_FORWARDS;
	return a;
}

inline Node* allocNode(NodeType type){
	html::Node* node = html::newNode();
	node->type = type;
	node->options |= NodeOptions::LIST_FORWARDS;
	return node;
}


inline Node* addChild(Parser& state, Node* node){
	node->parent = state.current;
	
	// Append sibling
	html::Node*& prev_sibling = state.lastChild.back();
	if (prev_sibling != nullptr)
		prev_sibling->next = node;
	else
		node->parent->child = node;

	prev_sibling = node;
	return node;
}


inline Node* push(Parser& state, Node* node){
	state.lastChild.emplace_back(node->child);
	state.current = node;
	return node;
}


inline void pop(Parser& state){
	state.current = state.current->parent;
	state.lastChild.pop_back();
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


// (c & 0b11000000) == 0
constexpr bool isMasked(char c, uint64_t mask){
	return ((uint64_t(1) << uint8_t(c)) & mask) != 0;
}

constexpr bool isMasked(char c, uint64_t mask, uint8_t maskBase){
	return ((c & 0b11000000) == (maskBase & 0b11000000)) & isMasked(c & 0b00111111, mask);
}

consteval uint64_t mask(const char* chars){
	assert(chars != nullptr);
	
	auto pfx = uint8_t(chars[0]) & 0b11000000;
	uint64_t m = 0;
	
	for (const char* c = chars ; *c != 0 ; c++){
		assert((*c & 0b11000000) == pfx);
		m |= (uint64_t(1) << (*c & 0b00111111));
	}
	
	return m;
}


constexpr bool isWhitespace(char c){
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

constexpr bool isQuote(char c){
	return c == '\'' || c == '"';
}

// [a-zA-Z:_-]
bool isTagChar(char c){
	constexpr uint64_t MASK_1 = mask("0123456789-:");
	constexpr uint64_t MASK_2 = []() consteval {
		uint64_t m = mask("_");
		for (uint8_t c = 'A' ; c <= 'Z' ; c++)
			m |= (uint64_t(1) << (c & 0b00111111));
		for (uint8_t c = 'a' ; c <= 'z' ; c++)
			m |= (uint64_t(1) << (c & 0b00111111));
		return m;
	}();
	
	if (uint8_t(c) < 64){
		return isMasked(c, MASK_1);
	} else {
		return isMasked(c, MASK_2, 'a');
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
	NodeOptions opts = {};
	
	// Parse all plaintext.
	while (true){
		if (*s == '<' || *s == 0)
			break;
		else if (*s == '{')
			opts = NodeOptions::INTERPOLATE;
		s++;
	}
	
	// Append text nodes
	while (beg != s){
		Node* txt = addChild(state, allocNode(NodeType::TEXT));
		txt->options |= opts;
		
		// Text chunk
		size_t len = min(size_t(s - beg), size_t(UINT32_MAX));
		txt->value_len = uint32_t(len);
		txt->value_p = beg;
		
		beg += len;
	}
	
	return s;
}


static const char* parse_rawpcData(Parser& state, Node* parent, const char* s){
	const char* const name = parent->value_p;
	const size_t name_len = parent->value_len;
	assert(name != nullptr && name_len > 0);
	
	const char* beg = s;
	const char* end = s;
	
	// Parse all plaintext untill </tag>
	while (true){ loop:
		if (s[0] == '<'){
			if (s[1] == '/')
				goto check_end_tag;
		} else if (*s == 0){
			goto err_no_end;
		}
		
		s++;
		continue;
		
		// Check for </tag>
		check_end_tag:
		end = s;
		s += 2;
		
		// Compare names
		for (size_t i = 0 ; i < name_len ; i++, s++){
			if (*s != name[i])
				goto loop;
		}
		
		s = parse_whiteSpace(state, s);
		if (*s == '>'){
			break;
		}
		
		err_no_end:
		state.result.pos = string_view(name, name_len);
		throw ParseStatus::MISSING_END_TAG;
	}
	
	// Check suffix space
	s++;
	if (isWhitespace(*s)){
		parent->options |= NodeOptions::SPACE_AFTER;
	}
	
	assert(parent->child == nullptr);
	size_t totalLen = size_t(end - beg);
	
	// Append text nodes back to front
	while (totalLen > 0){
		Node* txt = allocNode(NodeType::TEXT);
		txt->next = parent->child;
		parent->child = txt;
		txt->parent = parent;
		
		// Chunk of text
		size_t len = min(totalLen, size_t(UINT32_MAX));
		txt->value_len = uint32_t(len);
		txt->value_p = end - len;
		
		totalLen -= len;
		end -= len;
	}
	
	return s;
}


inline const char* parse_string(Parser& state, const char* s, NodeOptions& out_opts){
	assert(isQuote(*s));
	const char* beg = s;
	const char quote = *beg;
	
	s++;
	while (*s != quote){
		if (*s == '{')
			out_opts |= NodeOptions::INTERPOLATE;
		else if (*s == 0){
			state.result.pos = string_view(beg, s);
			throw ParseStatus::UNCLOSED_STRING;
		}
		s++;
	}
	
	return s + 1;
}


static const char* parse_question(Parser& state, const char* s){
	assert(*s == '?');
	const char* beg = ++s;
	NodeOptions __trash;
	
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
		state.result.pos = string_view(beg - 2, 2);
		throw ParseStatus::UNCLOSED_QUESTION;
	}
	
	Node* node = addChild(state, allocNode(NodeType::PI));
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
			state.result.pos = string_view(beg - 1, s);
			throw ParseStatus::UNCLOSED_COMMENT;
		}
		s++;
	}
	
	// -->
	s += 2;
	
	Node* node = addChild(state, allocNode(NodeType::COMMENT));
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
	NodeOptions __trash;
	
	while (true){
		if (*s < 64 && isMasked(*s, MASK_QUOTE | MASK_GT | MASK_EOF)){
			if (*s == '>')
				break;
			else if (isMasked(*s, MASK_QUOTE))
				s = parse_string(state, s, __trash);
			else if (*s == 0){
				state.result.pos = string_view(beg - 1, 2);
				throw ParseStatus::UNCLOSED_TAG;
			}
		}
		s++;
	}
	
	Node* node = addChild(state, allocNode(NodeType::DIRECTIVE));
	node->value_len = uint32_t(min(s - beg, long(UINT32_MAX)));
	node->value_p = beg;
	return s + 1;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static const char* parse_attribute(Parser& state, const char* s, Attr& attr){
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
	switch (*s){
		case '\'':
			attr.options |= NodeOptions::SINGLE_QUOTE;
		case '"':
			attr.value_p = s + 1;
			s = parse_string(state, s, attr.options);
			attr.value_len = uint32_t(min(s - attr.value_p - 1, long(UINT32_MAX)));
			break;
		default:
			state.result.pos = string_view(attr.name_p, s);
			throw ParseStatus::MISSING_ATTR_VALUE;
	}
	
	return s;
}


static const char* parse_openTag(Parser& state, const char* s, NodeOptions options){
	if (!isTagChar(*s)){
		throw ParseStatus::INVALID_TAG_NAME;
	}
	
	// Parse name
	const char* name_p = s;
	while (isTagChar(*(++s))) continue;
	const uint32_t name_len = uint32_t(min(s - name_p, long(UINT32_MAX)));
	
	Attr* firstAttr = nullptr;
	Attr* lastAttr = nullptr;
	
	// Self close
	if (s[0] == '/'){ tag_self_close:
		if (s[1] != '>'){
			state.result.pos = string_view(s, 1);
			throw ParseStatus::UNCLOSED_TAG;
		} else {
			s += 2;
		}
		
		html::Node* node = addChild(state, allocNode(NodeType::TAG));
		node->value_p = name_p;
		node->value_len = name_len;
		node->attribute = firstAttr;
		
		if (isWhitespace(*s))
			options |= NodeOptions::SPACE_AFTER;
		node->options |= options | NodeOptions::SELF_CLOSE;
		
		if (string_view(name_p, name_len) == "MACRO"){
			state.macros->emplace_back(node);
		}
		
		return s; 
	}
	
	// End
	else if (s[0] == '>'){ tag_end:
		html::Node* node = allocNode(NodeType::TAG);
		node->options |= options;
		node->value_p = name_p;
		node->value_len = name_len;
		node->attribute = firstAttr;
		s++;
		
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
					goto raw_pcdata;
				else
					goto regular_tag;
			case 4:
				if (name == "link" || name == "meta" || name == "area" || name == "base")
					goto void_tag;
				else
					goto regular_tag;
			case 5:
				if (name == "style" || name == "SHELL")
					goto raw_pcdata;
				else if (name == "input" || name == "embed" || name == "param" || name == "track")
					goto void_tag;
				else if (name == "MACRO")
					goto macro_tag;
				else
					goto regular_tag;
			case 6:
				if (name == "script")
					goto raw_pcdata;
				else if (name == "source")
					goto void_tag;
				else
					goto regular_tag;
			default:
				goto regular_tag;
		}
		
		raw_pcdata:
		addChild(state, node);
		return parse_rawpcData(state, node, s);
		
		macro_tag:
		state.macros->emplace_back(node);
		regular_tag:
		push(state, addChild(state, node));
		return s;
		
		void_tag:
		node->options |= NodeOptions::SELF_CLOSE;
		addChild(state, node);
		return s;
	}
	
	// Check for attributes
	else if (!isWhitespace(s[0])){
		state.result.pos = string_view(name_p, s+1);
		throw (s[0] == 0) ? ParseStatus::UNCLOSED_TAG : ParseStatus::INVALID_TAG_NAME;
	}
	
	while (true){
		s = parse_whiteSpace(state, s);
		
		// Self-close
		if (*s == '/'){
			goto tag_self_close;
		} else if (*s == '>'){
			goto tag_end;
		}
		
		else if (!isTagChar(*s)){
			if (*s == 0){
				state.result.pos = string_view(name_p, s);
				throw ParseStatus::UNCLOSED_TAG;
			} else {
				state.result.pos = string_view(s, 1);
				throw ParseStatus::INVALID_TAG_CHAR;
			}
		}
		
		// Attribute
		Attr* attr = allocAttr();
		if (firstAttr == nullptr){
			firstAttr = lastAttr = attr;
		} else {
			lastAttr->next = attr;
			lastAttr = attr;
		}
		
		s = parse_attribute(state, s, *attr);
		continue;
	}
	
	// Error
	state.result.pos = string_view(s, 1);
	throw ParseStatus::UNCLOSED_TAG;
}


static const char* parse_closeTag(Parser& state, const char* s){
	assert(s[0] == '/');
	assert(state.current != nullptr);
	const char* beg = s;
	
	Node* node = state.current;
	const uint32_t name_len = node->value_len;
	const char* const name = node->value_p;
	
	if (name_len <= 0){
		while (isTagChar(*(++s))) continue;
		state.result.pos = string_view(beg - 1, s);
		throw ParseStatus::INVALID_END_TAG;
	}
	
	s++;
	for (size_t i = 0 ; i < name_len ; i++){
		if (s[i] != name[i])
			goto check_void_tag;
	}
	
	s = parse_whiteSpace(state, s + name_len);
	if (*s == '>'){
		pop(state);
		s++;
		goto suffix;
	}
	
	check_void_tag:
	// Parse tag name
	while (isTagChar(*(++s))) continue;
	
	// Discard void tag
	if (isVoidTag(string_view(beg + 1, s))){
		s = parse_whiteSpace(state, s);
		if (*s == '>'){
			s++;
			goto suffix;
		}
	}
	
	// Error
	state.result.pos = string_view(beg, s);
	throw ParseStatus::INVALID_END_TAG;
	
	suffix:
	if (isWhitespace(*s)){
		node->options |= NodeOptions::SPACE_AFTER;
	}
	
	return s;
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
				beg = s;
				goto check_tag;
			}
		}
		
		// Close tag
		else if (s[1] == '/'){
			s = parse_closeTag(state, s+1);
			continue;
		}
		
		// Tag
		else if (isTagChar(s[1])) [[likely]] {
			NodeOptions opts = (beg != s) ? NodeOptions::SPACE_BEFORE : NodeOptions::NONE;
			s = parse_openTag(state, s+1, opts);
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
		
		else {
			state.result.pos = string_view(s+1, 1);
			throw ParseStatus::INVALID_TAG_NAME;
		}
		
	}
	
	// Check if parsed completely
	if (state.current->parent != nullptr){
		assert(state.current != nullptr && state.current->value_p != nullptr);
		state.result.pos = state.current->value();
		throw ParseStatus::MISSING_END_TAG;
	}
	
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static ParseResult parse(Document& doc, const char* buff){
	Parser parser;
	
	try {
		parser = {
			.current = &doc,
			.lastChild = { nullptr },			// Initial root child.
			.macros = &parser.result.macros,
			.result = {
				.status = ParseStatus::OK,
				.pos = string_view(buff, 1)
			}
		};
		parse_all(parser, buff);
	} catch (const bad_alloc&){
		parser.result.status = ParseStatus::MEMORY;
	} catch (const ParseStatus& err){
		parser.result.status = err;
	} catch (...){
		parser.result.status = ParseStatus::ERROR;
	}
	
	return parser.result;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool readFile(const filesystem::path& path, string& buff){
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
	
	assert(pos >= 0);
	const size_t size = size_t(pos);
	buff.resize(size);
	size_t count = 0;
	
	while (count < size && in.read(&buff[count], size - count)){
		const streamsize n = in.gcount();
		
		if (n < 0){
			return false;
		} else if (n == 0){
			break;
		}
		
		assert(n >= 0);
		count += size_t(n);
	}
	
	buff.resize(count);
	return true;
}


ParseResult Document::parseBuff(shared_ptr<const string> buff){
	reset();
	this->buffer = move(buff);
	return parse(*this, this->buffer->c_str());
}


ParseResult Document::parseFile(shared_ptr<const filesystem::path> path){
	assert(path != nullptr);
	unique_ptr<string> buff = make_unique<string>();
	
	if (!readFile(*path, *buff)){
		return ParseResult {
			.status = ParseStatus::IO
		};
	}
	
	reset();
	this->buffer = move(buff);
	this->srcFile = move(path);
	return parse(*this, this->buffer->c_str());
}


// ------------------------------------------------------------------------------------------ //