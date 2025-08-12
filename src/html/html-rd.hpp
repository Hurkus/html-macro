#pragma once
#include <cstdint>
#include <string_view>

#include "html-allocator.hpp"


namespace html {
	enum class node_type : uint8_t;
	
	struct rd_node;
	struct rd_attr;
	class rd_document;
	
	enum class parse_status;
	struct parse_result;
}


enum class html::parse_status {
	OK,
	UNCLOSED_TAG,
	UNCLOSED_STRING,
	UNCLOSED_QUESTION,		// ?>
	UNCLOSED_COMMENT,		// -->
	MISSING_TAG_GT,			// />
	INVALID_TAG_NAME,		// <...
	INVALID_TAG_CHAR,		// <...>
	INVALID_TAG_CLOSE,
	MISSING_ATTR_VALUE,		// attr=
	MEMORY,					// Out of memory.
	IO,						// Failed to read file.
	ERROR					// Unknown error.
};

#ifndef H_HTML_NODE_TYPE
#define H_HTML_NODE_TYPE
enum class html::node_type : uint8_t {
	TAG,		// <tag ... >
	PI,			// <? ... ?>
	DIRECTIVE,	// <! ... >
	COMMENT,	// <!-- ... -->
	TEXT,		// <...>text</...>
	ROOT		// Internal for marking root.
};
#endif


struct html::parse_result {
	parse_status status;
	const char* s = nullptr;	// Last parsing position.
	uint64_t row;				// Last parsed row.
};




/* Readonly node for marking HTML structure in a source text. */
struct html::rd_node {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	node_type type = node_type::TAG;
	uint32_t value_len = 0;				// Length of `value_p`.
	const char* value_p = nullptr;		// Unterminated name/value string.
	
	html::rd_node* parent = nullptr;
	html::rd_node* child = nullptr;		// First child in linked list.
	html::rd_node* next = nullptr;		// Linked list.
	
	html::rd_attr* attribute = nullptr;	// First attribute in linked list.
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	std::string_view value() const {
		return std::string_view(value_p, value_len);
	}
	
	std::string_view name() const {
		return value();
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	rd_document& root(){
		if (parent != nullptr)
			return parent->root();
		assert(type == node_type::ROOT);
		return *(rd_document*)this;
	}
	
// ------------------------------------------------------------------------------------------ //
};


struct html::rd_attr {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	const char* name_p = nullptr;
	const char* value_p = nullptr;
	uint32_t name_len = 0;
	uint32_t value_len = 0;
	
	html::rd_attr* next = nullptr;	// Linked list.
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	std::string_view name() const {
		return std::string_view(name_p, name_len);
	};
	
	std::string_view value() const {
		return std::string_view(value_p, value_len);
	};
	
// ------------------------------------------------------------------------------------------ //
};


class html::rd_document : public rd_node {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	const char* buffer = nullptr;	// Terminated input text.
	bool buffer_owned = false;		// Is the buffer owned by `this` object.
	
	html::const_allocator<rd_node> nodeAlloc;	// Node memory.
	html::const_allocator<rd_attr> attrAlloc;	// Attribute memory.
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	rd_document(){
		rd_node::type = node_type::ROOT;
	}
	
	~rd_document(){
		if (buffer_owned)
			delete buffer;
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Open file from `path` and parse HTML.
	 * @param path Path to file.
	 * @return `parse_result` containing parsing status.
	 */
	parse_result parse(const char* path);
	
	/**
	 * @brief Parse HTML from string.
	 * @param buff String buffer, whose lifetime must exceede that of the document.
	 * @return `parse_result` containing parsing status.
	 */
	parse_result parseBuff(const char* buff);
	
public:
	void clear(){
		if (buffer_owned)
			delete buffer;
		buffer = nullptr;
		nodeAlloc.clear();
		attrAlloc.clear();
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Find line number of `p` relative to beggining of the source `buffer`.
	 *        This function is very slow (iterates over `buffer`).
	 *        Recommended for error reporting only.
	 * @param p Pointer to first character of string for which to find the line number.
	 * @return Line number or -1 if not found.
	 */
	long lineof(const char* p) noexcept;
	
// ------------------------------------------------------------------------------------------ //
};
