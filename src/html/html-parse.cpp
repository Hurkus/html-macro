#include "html.hpp"
#include <vector>

#include "fs.hpp"

using namespace std;
using namespace html;


// ----------------------------------- [ Structures ] --------------------------------------- //


struct Parser {
	Node* current;				// Current parsing parent node.
	vector<Node*> lastChild;	// Stack of last child of `current`.
	vector<Node*> macros;		// <MACRO> nodes
};


struct ParseError {
	ParseStatus status = ParseStatus::ERROR;
	string_view mark;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


const char* html::errstr(ParseStatus status) noexcept {
	switch (status){
		case ParseStatus::OK:
			return "";
		case ParseStatus::ERROR:
			return "Internal error.";
		case ParseStatus::MEMORY:
			return "Out of memory.";
		case ParseStatus::IO:
			return "Failed to read file.";
		case ParseStatus::UNEXPECTED_CHAR:
			return "Unexpected character.";
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
	}
	assert(false);
	return nullptr;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline Attr* allocAttr(){
	Attr* a = new Attr();
	a->options |= NodeOptions::LIST_FORWARDS;
	return a;
}

inline Node* allocNode(NodeType type){
	Node* node = new Node();
	node->type = type;
	node->options |= NodeOptions::LIST_FORWARDS;
	return node;
}


inline Node* addChild(Parser& state, Node* node) noexcept {
	node->parent = state.current;
	
	// Append sibling
	Node*& prev_sibling = state.lastChild.back();
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


inline void pop(Parser& state) noexcept {
	state.current = state.current->parent;
	state.lastChild.pop_back();
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template<typename T>
constexpr T clampLen(const char* beg, const char* end);

template<>
constexpr uint32_t clampLen<uint32_t>(const char* beg, const char* end){
	return uint32_t(min(size_t(end - beg), size_t(UINT32_MAX)));
}

template<>
constexpr uint16_t clampLen<uint16_t>(const char* beg, const char* end){
	return uint16_t(min(size_t(end - beg), size_t(UINT16_MAX)));
}


// (c & 0b11000000) == 0
constexpr bool isMasked(char c, uint64_t mask) noexcept {
	return ((uint64_t(1) << uint8_t(c)) & mask) != 0;
}

constexpr bool isMasked(char c, uint64_t mask, uint8_t maskBase) noexcept {
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


constexpr bool isWhitespace(char c) noexcept {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

constexpr bool isQuote(char c) noexcept {
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

constexpr bool isVoidTag(string_view name) noexcept {
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


constexpr const char* parseWhitespace(const char* s, const char* end) noexcept {
	while (s != end && isWhitespace(*s)) s++;
	return s;
}


// Text untill next HTML element
static const char* parse_pcData(Parser& state, const char* beg, const char* s, const char* end){
	NodeOptions opts = {};
	
	// Parse all plaintext.
	while (true){
		if (s == end || *s == '<')
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
		uint32_t len = clampLen<uint32_t>(beg, s);
		txt->value_len = len;
		txt->value_p = beg;
		
		beg += len;
	}
	
	return s;
}


// Raw text untill closing tag from `parent`
static const char* parse_rawpcData(Parser& state, Node* parent, const char* const beg, const char* const end){
	assert(parent != nullptr);
	assert(parent->value_p != nullptr && parent->value_len > 0);
	
	const char* const name = parent->value_p;
	const size_t name_len = parent->value_len;
	const char* s = beg;	// After </tag>
	const char* _s = beg;	// Before </tag>
	
	// Parse all plaintext untill </tag>
	while (true){
		loop:
		if (s == end || s+1 == end){
			goto err_no_end;
		}
		
		else if (s[0] == '<'){
			if (s[1] == '/')
				goto check_end_tag;
		}
		
		s++;
		continue;
		
		// Check for </tag>
		check_end_tag:
		assert(s+1 != end);
		_s = s;
		s += 2;
		
		// Compare names
		for (size_t i = 0 ; i < name_len && s != end ; i++, s++){
			if (*s != name[i])
				goto loop;
		}
		
		s = parseWhitespace(s, end);
		if (s != end && *s == '>'){
			break;
		}
		
		err_no_end:
		throw ParseError(ParseStatus::MISSING_END_TAG, string_view(name, name_len));
	}
	
	assert(s != end && *s == '>');
	
	// Check suffix space
	s++;
	if (s != end && isWhitespace(*s)){
		parent->options |= NodeOptions::SPACE_AFTER;
	}
	
	assert(parent->child == nullptr);
	size_t totalLen = size_t(_s - beg);
	
	// Append text nodes back to front
	while (totalLen > 0){
		Node* txt = allocNode(NodeType::TEXT);
		txt->next = parent->child;
		parent->child = txt;
		txt->parent = parent;
		
		// Chunk of text
		size_t len = min(totalLen, size_t(UINT32_MAX));
		txt->value_len = uint32_t(len);
		txt->value_p = _s - len;
		
		totalLen -= len;
		_s -= len;
	}
	
	return s;
}


inline const char* parse_string(Parser& state, const char* s, const char* end, NodeOptions& out_opts){
	assert(s != end && isQuote(*s));
	const char* beg = s;
	const char quote = *beg;
	
	s++;
	while (true){
		if (s == end)
			throw ParseError(ParseStatus::UNCLOSED_STRING, string_view(beg, s));
		else if (*s == quote)
			break;
		else if (*s == '{')
			out_opts |= NodeOptions::INTERPOLATE;
		s++;
	}
	
	assert(s != end && isQuote(*s));
	return s + 1;
}


static const char* parse_comment(Parser& state, const char* s, const char* end){
	assert(s != end && s+1 != end && s+2 != end && s+3 != end);
	assert(s[0] == '<' && s[1] == '!' && s[2] == '-' && s[3] == '-');
	
	const char* beg = s;
	s += 4;
	
	while (true){
		if (s == end || s+1 == end || s+2 == end)
			throw ParseError(ParseStatus::UNCLOSED_COMMENT, string_view(beg, s));
		else if (s[0] == '-' && s[1] == '-' && s[2] == '>')
			break;
		s++;
	}
	
	// -->
	beg += 4;
	s += 2;
	assert(s != end && *s == '>');
	
	Node* node = addChild(state, allocNode(NodeType::COMMENT));
	node->value_len = clampLen<uint32_t>(beg, s-2);
	node->value_p = beg;
	return s + 1;
}


static const char* parse_exclamation(Parser& state, const char* s, const char* end){
	assert(s != end && s[0] == '<');
	assert(s+1 != end && s[1] == '!');
	
	// Parse comment
	if (s+2 != end && s+3 != end && s[2] == '-' && s[3] == '-'){
		return parse_comment(state, s, end);
	}
	
	// Parse directive
	s += 1;
	const char* beg = s;
	NodeOptions __trash;
	
	while (true){
		if (s == end)
			throw ParseError(ParseStatus::UNCLOSED_TAG, string_view(beg - 1, 2));
		else if (*s == '>')
			break;
		else if (isQuote(*s))
			s = parse_string(state, s, end, __trash);
		s++;
	}
	
	assert(s != end && *s == '>');
	
	Node* node = addChild(state, allocNode(NodeType::DIRECTIVE));
	node->value_len = clampLen<uint32_t>(beg, s);
	node->value_p = beg;
	return s + 1;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static const char* parse_attribute(Parser& state, const char* s, const char* end, Attr& attr){
	assert(s != end && isTagChar(*s));
	
	// Parse name
	attr.name_p = s++;
	while (s != end && isTagChar(*s)) s++;
	attr.name_len = clampLen<uint16_t>(attr.name_p, s);
	
	// Check for value
	s = parseWhitespace(s, end);
	if (s == end || *s != '='){
		return s;
	}
	
	s = parseWhitespace(s+1, end);
	if (s == end){
		throw ParseError(ParseStatus::MISSING_ATTR_VALUE, string_view(attr.name_p, attr.name_len));
	}
	
	// Parse value
	switch (*s){
		case '\'':
			attr.options |= NodeOptions::SINGLE_QUOTE;
		case '"':
			attr.value_p = s + 1;
			s = parse_string(state, s, end, attr.options);
			attr.value_len = clampLen<uint32_t>(attr.value_p, s - 1);
			break;
		default:
			throw ParseError(ParseStatus::MISSING_ATTR_VALUE, string_view(attr.name_p, attr.name_len));
	}
	
	return s;
}


static const char* parse_openTag(Parser& state, const char* const beg, const char* const end, NodeOptions options){
	assert(beg != end && beg[0] == '<');
	assert(beg+1 != end && isTagChar(beg[1]));
	const char* s = beg + 2;
	
	// Parse name
	while (s != end && isTagChar(*s)){
		s++;
	}
	
	Attr* lastAttr = nullptr;
	html::Node* node = addChild(state, allocNode(NodeType::TAG));
	node->options |= options;
	node->value_len = clampLen<uint32_t>(beg + 1, s);
	node->value_p = beg + 1;
	
	if (s == end){ err_unclosed:
		throw ParseError(ParseStatus::UNCLOSED_TAG, string_view(beg, node->name().end()));
	}
	
	// Self close
	else if (s[0] == '/'){ tag_self_close:
		if (s+1 == end || s[1] != '>'){
			goto err_unclosed;
		}
		
		assert(s[0] == '/' && s[1] == '>');
		s += 2;
		
		node->options |= NodeOptions::SELF_CLOSE;
		if (s != end && isWhitespace(*s)){
			node->options |= NodeOptions::SPACE_AFTER;
		}
		
		if (node->name() == "MACRO"){
			state.macros.emplace_back(node);
		}
		
		return s; 
	}
	
	// End
	else if (s[0] == '>'){ tag_end:
		s++;
		
		const string_view name = node->name();
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
		return parse_rawpcData(state, node, s, end);
		
		macro_tag:
		state.macros.emplace_back(node);
		regular_tag:
		push(state, node);
		return s;
		
		void_tag:
		node->options |= NodeOptions::SELF_CLOSE;
		return s;
	}
	
	// Check for attributes
	else if (!isWhitespace(s[0])){ err_unexpected_char:
		throw ParseError(ParseStatus::UNEXPECTED_CHAR, string_view(s, 1));
	}
	
	s++;
	while (true){
		s = parseWhitespace(s, end);
		
		// Check for self-close
		if (s == end){
			goto err_unclosed;
		} else if (*s == '/'){
			goto tag_self_close;
		} else if (*s == '>'){
			goto tag_end;
		} else if (!isTagChar(*s)){
			goto err_unexpected_char;
		}
		
		assert(isTagChar(*s));
		
		// Append attribute
		Attr* attr = allocAttr();
		if (node->attribute == nullptr){
			node->attribute = attr;
			lastAttr = attr;
		} else {
			lastAttr->next = attr;
			lastAttr = attr;
		}
		
		s = parse_attribute(state, s, end, *attr);
	}
	
	assert(false);
	goto err_unclosed;
}


static const char* parse_closeTag(Parser& state, const char* s, const char* end){
	assert(s != end && s[0] == '<');
	assert(s+1 != end && s[1] == '/');
	assert(state.current != nullptr);
	
	const char* const beg = s;
	s += 2;
	
	Node* node = state.current;
	const size_t name_len = node->value_len;
	const char* const name_p = node->value_p;
	
	// Report invalid end tag name
	if (name_len <= 0 || s == end){
		while (s != end && isTagChar(*s)) s++;
		throw ParseError(ParseStatus::INVALID_END_TAG, string_view(beg, s));
	}
	
	for (size_t i = 0 ; i < name_len ; i++){
		if (s+i == end || s[i] != name_p[i])
			goto check_void_tag;
	}
	
	s += name_len; 
	assert(string_view(beg+2, s) == string_view(name_p, name_len));
	
	// '>'
	if (s == end){
		throw ParseError(ParseStatus::INVALID_END_TAG, string_view(beg, s));
	} else if (*s == '>') [[likely]] {
		pop(state);
		s++;
		goto suffix;
	}
	
	// ' >'
	else if (!isWhitespace(*s)){
		goto check_void_tag;
	} else {
		s = parseWhitespace(s+1, end);
		
		if (s != end && *s == '>'){
			pop(state);
			s++;
			goto suffix;
		}
		
		throw ParseError(ParseStatus::INVALID_END_TAG, string_view(beg, s));
	}
	
	check_void_tag: {
		s = beg + 2;
		
		// Parse tag name
		while (s != end && isTagChar(*s)) s++;
		const char* const name_end = s;
		
		if (s == end){
			goto err_invalid_end;
		} else if (isWhitespace(*s)){
			s = parseWhitespace(s, end);
			if (s == end)
				throw ParseError(ParseStatus::INVALID_END_TAG, string_view(beg, name_end));
		}
		
		if (*s != '>'){
			throw ParseError(ParseStatus::UNEXPECTED_CHAR, string_view(s, s+1));
		} else if (isVoidTag(string_view(beg + 2, name_end))){
			s++;
			goto suffix;
		}
		
		err_invalid_end:
		throw ParseError(ParseStatus::INVALID_END_TAG, string_view(beg, s));
	}
	
	suffix:
	if (s != end && isWhitespace(*s)){
		node->options |= NodeOptions::SPACE_AFTER;
	}
	
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


const char* parse_all(Parser& state, const char* s, const char* end){
	while (true){
		const char* beg = s;
		s = parseWhitespace(s, end);
		
		repeat_nospace:
		if (s == end){
			break;
		}
		
		else if (*s != '<'){
			s = parse_pcData(state, beg, s, end);
			beg = s;
			goto repeat_nospace;
		}
		
		else if (s+1 == end){
			throw ParseError(ParseStatus::INVALID_TAG_NAME, string_view(s, s+1));
		}
		
		// Tag
		else if (isTagChar(s[1])) [[likely]] {
			NodeOptions opts = (beg != s) ? NodeOptions::SPACE_BEFORE : NodeOptions::NONE;
			s = parse_openTag(state, s, end, opts);
			continue;
		}
		
		// Close tag
		else if (s[1] == '/'){
			s = parse_closeTag(state, s, end);
			continue;
		}
		
		else if (s[1] == '!'){
			s = parse_exclamation(state, s, end);
			continue;
		}
		
		throw ParseError(ParseStatus::INVALID_TAG_NAME, string_view(s, s+2));
	}
	
	// Check if parsed completely
	if (state.current->parent != nullptr){
		assert(state.current != nullptr && state.current->value_p != nullptr);
		throw ParseError(ParseStatus::MISSING_END_TAG, state.current->value());
	}
	
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static ParseResult parse(Document& doc, string_view txt){
	try {
		Parser parser = {
			.current = &doc,
			.lastChild = {nullptr},	// Initial root child.
			.macros = {}
		};
		
		parse_all(parser, txt.begin(), txt.end());
		
		return ParseResult {
			.status = ParseStatus::OK,
			.macros = move(parser.macros)
		};
		
	}
	catch (const bad_alloc&){
		return ParseResult(ParseStatus::MEMORY);
	}
	catch (const ParseError& err){
		return ParseResult(err.status, err.mark);
	}
	catch (...){}
	
	return ParseResult(ParseStatus::ERROR);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


ParseResult Document::parseBuff(shared_ptr<const string> buff){
	reset();
	this->buffer = move(buff);
	return parse(*this, string_view(*this->buffer));
}


ParseResult Document::parseFile(shared_ptr<const filesystem::path> path){
	assert(path != nullptr);
	unique_ptr<string> buff = make_unique<string>();
	
	if (!fs::readFile(*path, *buff)){
		return ParseResult {
			.status = ParseStatus::IO
		};
	}
	
	reset();
	this->buffer = move(buff);
	this->srcFile = move(path);
	return parse(*this, string_view(*this->buffer));
}


// ------------------------------------------------------------------------------------------ //