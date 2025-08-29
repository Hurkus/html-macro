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
	enum class node_type : uint8_t;
	enum class node_options : uint8_t;
	
	enum class parse_status;
	struct parse_result;
	
	struct node;
	struct attr;
	class document;
	
	const char* errstr(parse_status status) noexcept;
}


enum class html::parse_status {
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


enum class html::node_type : uint8_t {
	TAG,		// <tag ... >
	PI,			// <? ... ?>
	DIRECTIVE,	// <! ... >
	COMMENT,	// <!-- ... -->
	TEXT,		// <...>text</...>
	ROOT		// Internal for marking root.
};


enum class html::node_options : uint8_t {
	NONE          = 0,
	LIST_FORWARDS = 1 << 0,	// Direction of node child linked list and attribute linked list.
	OWNED_NAME    = 1 << 1,
	OWNED_VALUE   = 1 << 2,
	INTERPOLATE   = 1 << 3,	// Text content of `value` should be interpolated for expressions `{}`.
};
ENUM_OPERATORS(html::node_options);


struct html::parse_result {
	parse_status status;
	const char* pos = nullptr;	// Last parsing position. Usefull for `document::row()`.
	std::vector<node*> macros;	// Pointers to nodes `<MACRO>`
};




/* Readonly node for marking HTML structure in a source text. */
struct html::node {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	node_type type = node_type::TAG;
	node_options options = node_options::NONE;
	
	uint32_t value_len = 0;			// Length of `value_p`.
	const char* value_p = nullptr;	// Unterminated name/value string.
	
	node* parent = nullptr;
	node* child = nullptr;			// First/last child in linked list.
	node* next = nullptr;			// Linked list.
	
	attr* attribute = nullptr;		// First/last attribute in linked list.
	
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
	node& appendChild(node_type type = node_type::TAG);
	attr& appendAttribute();
	
	bool removeChild(node* child);
	void removeChildren();
	node* extractChild(node* child);
	
	bool removeAttr(attr* attr);
	void removeAttributes();
	attr* extractAttr(attr* child);
	
	void clear(){
		removeAttributes();
		removeChildren();
		value(nullptr);
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	document& root(){
		if (parent != nullptr)
			return parent->root();
		assert(type == node_type::ROOT);
		return *(document*)this;
	}
	
	const document& root() const {
		if (parent != nullptr)
			return parent->root();
		assert(type == node_type::ROOT);
		return *(document*)this;
	}
	
public:
	static void del(node* node){
		assert(node != nullptr);
		if (node->parent != nullptr){
			node->parent->removeChild(node);
		} else {
			node->clear();
			html::del(node);
		}
	}
	
// ------------------------------------------------------------------------------------------ //
};


struct html::attr {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	node_options options = node_options::NONE;
	
	uint16_t name_len = 0;
	uint32_t value_len = 0;
	const char* name_p = nullptr;
	const char* value_p = nullptr;
	
	attr* next = nullptr;	// Linked list.
	
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


class html::document : public node {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	std::shared_ptr<char> buffer_owned = nullptr;		// c-string
	const char* buffer_unowned = nullptr;
	
	std::shared_ptr<const std::string> srcFile;			// Path to source file.
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	document(){
		node::type = node_type::ROOT;
	}
	
	document(document&& o){
		std::swap(*this, o);
	}
	
	document& operator=(document&& o){
		std::swap(*this, o);
		return *this;
	}
	
	~document(){
		reset();
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Parse HTML from string.
	 * @param buff String buffer, whose lifetime must exceede that of `this` object.
	 * @return `parse_result` containing parsing status.
	 */
	parse_result parseBuff(const char* buff);
	
	/**
	 * @brief Open file from `path` and parse HTML.
	 *        `buffer` is owned by `this` object.
	 * @param path Path to file.
	 * @return `parse_result` containing parsing status.
	 */
	parse_result parseFile(std::string&& path){
		srcFile = std::make_shared<std::string>(std::move(path));
		return parseFile();
	}
	
private:
	parse_result parseFile();
	
public:
	/**
	 * @brief Delete/reset all owned data (child nodes, buffer, etc.).
	 */
	void reset(){
		node::clear();
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
