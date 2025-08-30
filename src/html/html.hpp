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
};

template<> inline constexpr bool has_enum_operators<html::NodeOptions> = true;


struct html::ParseResult {
	ParseStatus status;
	const char* pos = nullptr;	// Last parsing position. Usefull for `document::row()`.
	std::vector<Node*> macros;	// Pointers to nodes `<MACRO>`
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
	void appendChild(Node* child);
	Attr& appendAttribute();
	
	bool removeChild(Node* child);
	void removeChildren();
	Node* extractChild(Node* child);
	
	bool removeAttr(Attr* attr);
	void removeAttributes();
	Attr* extractAttr(Attr* child);
	
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
	std::shared_ptr<char> buffer_owned = nullptr;		// c-string
	const char* buffer_unowned = nullptr;
	
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
		reset();
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Parse HTML from string.
	 * @param buff String buffer, whose lifetime must exceede that of `this` object.
	 * @return `parse_result` containing parsing status.
	 */
	ParseResult parseBuff(const char* buff);
	
	/**
	 * @brief Open file from `path` and parse HTML.
	 *        `buffer` is owned by `this` object.
	 * @param path Path to file.
	 * @return `parse_result` containing parsing status.
	 */
	ParseResult parseFile(std::string&& path){
		srcFile = std::make_shared<std::string>(std::move(path));
		return parseFile();
	}
	
private:
	ParseResult parseFile();
	
public:
	/**
	 * @brief Delete/reset all owned data (child nodes, buffer, etc.).
	 */
	void reset(){
		Node::clear();
		buffer_owned = nullptr;
		buffer_unowned = nullptr;
	}
	
	/**
	 * @brief Disown all child nodes without freeing memory (leak memory).
	 * @note Really think about it before using this.
	 */
	void releaseNodes(){
		this->child = nullptr;
		this->value_p = nullptr;
		this->value_len = 0;
		this->next = nullptr;
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Find line number of `p` relative to beggining of the source `buffer`.
	 *        This function is very slow (iterates over `buffer`).
	 * @note Recommended for error reporting only.
	 * @param p Pointer to first character of string for which to find the line number.
	 * @return Line number or `-1` if not found.
	 */
	long row(const char* p) const noexcept;
	
	/**
	 * @brief Find colum number of `p` relative to beggining of the line in `buffer`.
	 * @note Recommended for error reporting only.
	 * @param p Pointer to first character of string for which to find the colum number.
	 * @return Colum number or `-1` if not found.
	 */
	long col(const char* p) const noexcept;
	
	
	const char* file() const {
		if (srcFile != nullptr)
			return srcFile->c_str();
		return "";
	}
	
// ------------------------------------------------------------------------------------------ //
};
