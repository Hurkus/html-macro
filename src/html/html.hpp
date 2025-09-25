#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include "html-allocator.hpp"
#include "EnumOperators.hpp"


namespace html {
	enum class NodeType : uint8_t;
	enum class NodeOptions : uint8_t;
	
	enum class ParseStatus;
	struct ParseResult;
	
	struct Node;
	struct Attr;
	class Document;
	
	const char* errstr(ParseStatus status) noexcept;
}


enum class html::ParseStatus {
	OK,
	UNCLOSED_TAG,			// ...>
	UNCLOSED_STRING,		// ..."
	UNCLOSED_QUESTION,		// ...?>
	UNCLOSED_COMMENT,		// ...-->
	INVALID_TAG_NAME,		// <...
	INVALID_TAG_CHAR,		// <...>
	MISSING_END_TAG,		// Some tags are unclosed: </tag>
	INVALID_END_TAG,		// Too many </tag>.
	MISSING_ATTR_VALUE,		// attr=
	MEMORY,					// Out of memory.
	IO,						// Failed to read file.
	ERROR					// Unknown error.
};


enum class html::NodeType : uint8_t {
	TAG,		// <tag ... >
	PI,			// <? ... ?>
	DIRECTIVE,	// <! ... >
	COMMENT,	// <!-- ... -->
	TEXT,		// <...>text</...>
	ROOT		// Internal for marking root.
};


enum class html::NodeOptions : uint8_t {
	NONE          = 0,
	LIST_FORWARDS = 1 << 0,	// Direction of node child linked list and attribute linked list.
	OWNED_NAME    = 1 << 1,
	OWNED_VALUE   = 1 << 2,
	INTERPOLATE   = 1 << 3,	// Text content of `value` should be interpolated for expressions `{}`.
	SINGLE_QUOTE  = 1 << 4, // Attribute value is in single quotes.
	SELF_CLOSE    = 1 << 5, // <tag/>
	SPACE_BEFORE  = 1 << 6, // Node is prefixed by whitespace.
	SPACE_AFTER   = 1 << 7, // Node is suffixed with whitespace.
};

template<> inline constexpr bool has_enum_operators<html::NodeOptions> = true;


struct html::ParseResult {
	ParseStatus status;
	std::string_view pos;			// Last parsing context.
	std::vector<Node*> macros;		// Pointers to nodes `<MACRO>`
};




/* Readonly node for marking HTML structure in a source text. */
struct html::Node {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	NodeType type = NodeType::TAG;
	NodeOptions options = NodeOptions::NONE;
	
	uint32_t value_len = 0;			// Length of `value_p`.
	const char* value_p = nullptr;	// Unterminated name/value string.
	
	Node* parent = nullptr;
	Node* child = nullptr;			// First/last child in linked list.
	Node* next = nullptr;			// Linked list.
	
	Attr* attribute = nullptr;		// First/last attribute in linked list.
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	// Get node value.
	std::string_view value() const {
		return std::string_view(value_p, value_len);
	}
	
	// Remove (maybe deallocate) value string.
	void value(nullptr_t);
	
	// Set value to const string.
	void value(std::string_view str);
	
	// Set value to owned string.
	void value(char* str, size_t len);
	
public:
	// Get node name
	std::string_view name() const {
		return this->value();
	}
	
	// Remove (maybe deallocate) name string.
	void name(nullptr_t){
		this->value(nullptr);
	}
	
	// Set name to const string.
	void name(std::string_view str){
		this->value(str);
	}
	
	// Set name to const string.
	void name(char* str, size_t len){
		this->value(str, len);
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	Node& appendChild(NodeType type = NodeType::TAG);
	void appendChild(Node* child) noexcept;
	Attr& appendAttribute();
	
	bool removeChild(Node* child);
	void removeChildren();
	Node* extractChild(Node* child) noexcept;
	
	bool removeAttr(Attr* attr);
	bool removeAttr(std::string_view name);
	void removeAttributes();
	Attr* extractAttr(Attr* child) noexcept;
	
	void clear(){
		removeAttributes();
		removeChildren();
		value(nullptr);
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	Document& root(){
		if (parent != nullptr)
			return parent->root();
		assert(type == NodeType::ROOT);
		return *(Document*)this;
	}
	
	const Document& root() const {
		if (parent != nullptr)
			return parent->root();
		assert(type == NodeType::ROOT);
		return *(Document*)this;
	}
	
public:
	static void del(Node* node){
		assert(node != nullptr);
		if (node->parent != nullptr){
			node->parent->removeChild(node);
		} else {
			node->clear();
			html::del(node);
		}
	}
	
public:
	struct deleter {
		void operator()(Node* p){
			if (p != nullptr){
				p->clear();
				html::del(p);
			}
		}
	};
	
// ------------------------------------------------------------------------------------------ //
};


struct html::Attr {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	NodeOptions options = NodeOptions::NONE;
	
	uint16_t name_len = 0;
	uint32_t value_len = 0;
	const char* name_p = nullptr;
	const char* value_p = nullptr;
	
	Attr* next = nullptr;	// Linked list.
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	// Get node value.
	std::string_view value() const {
		return std::string_view(value_p, value_len);
	}
	
	// Remove (maybe deallocate) value string.
	void value(nullptr_t);
	
	// Set value to const string.
	void value(std::string_view str);
	
	// Set value to owned string.
	void value(char* str, size_t len);
	
public:
	// Get node name
	std::string_view name() const {
		return std::string_view(name_p, name_len);
	}
	
	// Remove (maybe deallocate) name string.
	void name(nullptr_t);
	
	// Set name to const string.
	void name(std::string_view str);
	
	// Set name to const string.
	void name(char* str, size_t len);
	
// ------------------------------------------------------------------------------------------ //
};


class html::Document : public Node {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	std::shared_ptr<const std::string> buffer;
	std::shared_ptr<const std::string> srcFile;			// Path to source file.
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	Document(){
		Node::type = NodeType::ROOT;
	}
	
	Document(Document&& o){
		std::swap(*this, o);
	}
	
	Document& operator=(Document&& o){
		std::swap(*this, o);
		return *this;
	}
	
	~Document(){
		Node::clear();
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Parse HTML from string.
	 * @param buff String buffer.
	 * @return `parse_result` containing parsing status.
	 */
	ParseResult parseBuff(std::shared_ptr<const std::string>&& buff);
	
	/**
	 * @brief Open file from `path` and parse HTML.
	 *        Content buffer is owned by `this` object.
	 * @param path Path to file.
	 * @return `parse_result` containing parsing status.
	 */
	ParseResult parseFile(std::string&& path);
	
public:
	/**
	 * @brief Delete/reset all owned data (child nodes, buffer, etc.).
	 */
	void reset(){
		Node::clear();
		buffer.reset();
		srcFile.reset();
	}
	
	/**
	 * @brief Disown all child nodes without freeing memory (leak memory).
	 * @note Really think about it before using this.
	 */
	// void releaseMemory(){
	// 	this->child = nullptr;
	// 	this->attribute = nullptr;
	// 	this->value_p = nullptr;
	// 	this->value_len = 0;
	// 	this->next = nullptr;
	// }
	
// ------------------------------------------------------------------------------------------ //
};
