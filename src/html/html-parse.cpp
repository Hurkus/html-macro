#include "html.hpp"

using namespace std;
using namespace html;

using Status = ParseResult::Status;


// ----------------------------------- [ Structures ] --------------------------------------- //
namespace {


struct Parser {
	Document& doc;
	Node* current = nullptr;	// Current parsing parent node.
	vector<Node*> lastChild;	// Stack of last child of `current`. `null` if no children.
	vector<Node*> macros;		// <MACRO> nodes
	
	Node* node(NodeType);
	Attr* attr();
	Node* addChild(Node*) noexcept;
	Node* push(Node*);
	Node* pop() noexcept;
};


struct Error {
	Status status = Status::ERROR;
	string_view mark;
};


}
// ----------------------------------- [ Functions ] ---------------------------------------- //


const char* ParseResult::msg(Status status) noexcept {
	switch (status){
		case Status::OK:
			return "";
		case Status::ERROR:
			return "Internal error.";
		case Status::MEMORY:
			return "Out of memory.";
		case Status::UNEXPECTED_CHAR:
			return "Unexpected character.";
		case Status::UNCLOSED_TAG:
			return "Missing closing bracket '>'.";
		case Status::UNCLOSED_STRING:
			return "Unterminated string. Multiline strings are not permitted.";
		case Status::UNCLOSED_COMMENT:
			return "Missing end bracket '-->' for comment.";
		case Status::INVALID_TAG_NAME:
			return "Invalid character in tag name.";
		case Status::INVALID_TAG_CHAR:
			return "Invalid character in tag.";
		case Status::MISSING_END_TAG:
			return "Missing end tag.";
		case Status::INVALID_END_TAG:
			return "Invalid end tag doesn't close any opened tag.";
		case Status::MISSING_ATTR_VALUE:
			return "Missing value of attribute.";
	}
	assert(false);
	return nullptr;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Node* Parser::node(NodeType type){
	assert(doc.nodeAlloc != nullptr);
	Node* p = doc.nodeAlloc->create();
	p->type = type;
	return p;
}


Attr* Parser::attr(){
	assert(doc.attrAlloc != nullptr);
	return doc.attrAlloc->create();
}


Node* Parser::addChild(Node* node) noexcept {
	node->parent = current;
	
	assert(!lastChild.empty());
	Node*& prev = lastChild.back();
	
	// Append sibling
	if (prev != nullptr)
		prev->next = node;
	else
		node->parent->child = node;

	prev = node;
	return node;
}


Node* Parser::push(Node* node){
	current = node;
	lastChild.emplace_back(node->child);
	return node;
}


Node* Parser::pop() noexcept {
	assert(current->parent != nullptr);
	Node* n = current;
	current = current->parent;
	lastChild.pop_back();
	return n;
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
constexpr bool isTagChar(char c){
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
static const char* parse_pcData(Parser& ctx, const char* beg, const char* s, const char* end){
	NodeOptions opts = NodeOptions::NONE;
	
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
		Node* txt = ctx.addChild(ctx.node(NodeType::TEXT));
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
static const char* parse_rawpcData(Parser& ctx, Node* parent, const char* const beg, const char* const end){
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
		throw Error(Status::MISSING_END_TAG, string_view(name, name_len));
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
		Node* txt = ctx.node(NodeType::TEXT);
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
			throw Error(Status::UNCLOSED_STRING, string_view(beg, s));
		else if (*s == quote)
			break;
		else if (*s == '{')
			out_opts |= NodeOptions::INTERPOLATE;
		s++;
	}
	
	assert(s != end && isQuote(*s));
	return s + 1;
}


static const char* parse_comment(Parser& ctx, const char* s, const char* end){
	assert(s != end && s+1 != end && s+2 != end && s+3 != end);
	assert(s[0] == '<' && s[1] == '!' && s[2] == '-' && s[3] == '-');
	
	const char* beg = s;
	s += 4;
	
	while (true){
		if (s == end || s+1 == end || s+2 == end)
			throw Error(Status::UNCLOSED_COMMENT, string_view(beg, s));
		else if (s[0] == '-' && s[1] == '-' && s[2] == '>')
			break;
		s++;
	}
	
	// -->
	beg += 4;
	s += 2;
	assert(s != end && *s == '>');
	
	Node* node = ctx.addChild(ctx.node(NodeType::COMMENT));
	node->value_len = clampLen<uint32_t>(beg, s-2);
	node->value_p = beg;
	return s + 1;
}


static const char* parse_exclamation(Parser& ctx, const char* s, const char* end){
	assert(s != end && s[0] == '<');
	assert(s+1 != end && s[1] == '!');
	
	// Parse comment
	if (s+2 != end && s+3 != end && s[2] == '-' && s[3] == '-'){
		return parse_comment(ctx, s, end);
	}
	
	// Parse directive
	s += 1;
	const char* beg = s;
	NodeOptions __trash;
	
	while (true){
		if (s == end)
			throw Error(Status::UNCLOSED_TAG, string_view(beg - 1, 2));
		else if (*s == '>')
			break;
		else if (isQuote(*s))
			s = parse_string(ctx, s, end, __trash);
		s++;
	}
	
	assert(s != end && *s == '>');
	
	Node* node = ctx.addChild(ctx.node(NodeType::DIRECTIVE));
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
		throw Error(Status::MISSING_ATTR_VALUE, string_view(attr.name_p, attr.name_len));
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
			throw Error(Status::MISSING_ATTR_VALUE, string_view(attr.name_p, attr.name_len));
	}
	
	return s;
}


static const char* parse_openTag(Parser& ctx, const char* const beg, const char* const end, NodeOptions options){
	assert(beg != end && beg[0] == '<');
	assert(beg+1 != end && isTagChar(beg[1]));
	const char* s = beg + 2;
	
	// Parse name
	while (s != end && isTagChar(*s)){
		s++;
	}
	
	Attr* lastAttr = nullptr;
	Node* node = ctx.addChild(ctx.node(NodeType::TAG));
	node->options |= options;
	node->value_len = clampLen<uint32_t>(beg + 1, s);
	node->value_p = beg + 1;
	
	if (s == end){ err_unclosed:
		throw Error(Status::UNCLOSED_TAG, string_view(beg, node->name().end()));
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
			ctx.macros.emplace_back(node);
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
		return parse_rawpcData(ctx, node, s, end);
		
		macro_tag:
		ctx.macros.emplace_back(node);
		regular_tag:
		ctx.push(node);
		return s;
		
		void_tag:
		node->options |= NodeOptions::SELF_CLOSE;
		return s;
	}
	
	// Check for attributes
	else if (!isWhitespace(s[0])){ err_unexpected_char:
		throw Error(Status::UNEXPECTED_CHAR, string_view(s, 1));
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
		Attr* attr = ctx.attr();
		if (node->attribute == nullptr){
			node->attribute = attr;
			lastAttr = attr;
		} else {
			lastAttr->next = attr;
			lastAttr = attr;
		}
		
		s = parse_attribute(ctx, s, end, *attr);
	}
	
	assert(false);
	goto err_unclosed;
}


static const char* parse_closeTag(Parser& ctx, const char* s, const char* end){
	assert(s != end && s[0] == '<');
	assert(s+1 != end && s[1] == '/');
	assert(ctx.current != nullptr);
	
	const char* const beg = s;
	s += 2;
	
	Node* node = ctx.current;
	const size_t name_len = node->value_len;
	const char* const name_p = node->value_p;
	
	// Report invalid end tag name
	if (name_len <= 0 || s == end){
		while (s != end && isTagChar(*s)) s++;
		throw Error(Status::INVALID_END_TAG, string_view(beg, s));
	}
	
	for (size_t i = 0 ; i < name_len ; i++){
		if (s+i == end || s[i] != name_p[i])
			goto check_void_tag;
	}
	
	s += name_len; 
	assert(string_view(beg+2, s) == string_view(name_p, name_len));
	
	// '>'
	if (s == end){
		throw Error(Status::INVALID_END_TAG, string_view(beg, s));
	} else if (*s == '>') [[likely]] {
		ctx.pop();
		s++;
		goto suffix;
	}
	
	// ' >'
	else if (!isWhitespace(*s)){
		goto check_void_tag;
	} else {
		s = parseWhitespace(s+1, end);
		
		if (s != end && *s == '>'){
			ctx.pop();
			s++;
			goto suffix;
		}
		
		throw Error(Status::INVALID_END_TAG, string_view(beg, s));
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
				throw Error(Status::INVALID_END_TAG, string_view(beg, name_end));
		}
		
		if (*s != '>'){
			throw Error(Status::UNEXPECTED_CHAR, string_view(s, s+1));
		} else if (isVoidTag(string_view(beg + 2, name_end))){
			s++;
			goto suffix;
		}
		
		err_invalid_end:
		throw Error(Status::INVALID_END_TAG, string_view(beg, s));
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
			throw Error(Status::INVALID_TAG_NAME, string_view(s, s+1));
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
		
		throw Error(Status::INVALID_TAG_NAME, string_view(s, s+2));
	}
	
	// Check if parsed completely
	if (state.current->parent != nullptr){
		assert(state.current != nullptr && state.current->value_p != nullptr);
		throw Error(Status::MISSING_END_TAG, state.current->value());
	}
	
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


ParseResult Document::parse(const shared_ptr<const string>& buff) noexcept {
	assert(buff != nullptr);
	
	this->buffer = buff;
	string_view txt = string_view(*buff);
	
	try {
		Parser parser = {
			.doc = *this,
			.current = this,
			.lastChild = {nullptr},	// Initial root child.
			.macros = {}
		};
		
		parse_all(parser, txt.begin(), txt.end());
		
		return ParseResult {
			.status = Status::OK,
			.mark = txt,
			.macros = move(parser.macros)
		};
		
	}
	catch (const bad_alloc&){
		return ParseResult(Status::MEMORY);
	}
	catch (const Error& err){
		return ParseResult(err.status, err.mark);
	}
	catch (...){}
	
	return ParseResult(Status::ERROR);
}


// ------------------------------------------------------------------------------------------ //