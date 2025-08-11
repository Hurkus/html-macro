#pragma once
#include <cstdint>
#include <string_view>
#include <memory>


namespace html {
	enum class node_type : uint8_t;
	
	struct rd_node;
	struct rd_attr;
	class rd_document;
	
	enum class parse_status;
	struct parse_result;
	
	parse_result parse(const char* path);
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
};
#endif


struct html::parse_result {
	parse_status status;
	const char* s = nullptr;	// Last parsing position.
	uint32_t row;				// Last parsed row.
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


class html::rd_document {
// ----------------------------------- [ Structures ] --------------------------------------- //
public:
	template<typename T>
	struct allocation {
		std::unique_ptr<T[]> alloc;				// Allocation array.
		std::unique_ptr<allocation<T>> next;	// Linked list of allocations.
	};
	
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	std::unique_ptr<char[]> buffer;		// Terminated input text.
	std::size_t buffer_len;				// Length of `buffer`.
	
	std::unique_ptr<allocation<rd_node>> nodeAlloc; // Allocations for node structs.
	std::unique_ptr<allocation<rd_attr>> attrAlloc;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	const rd_node* root() const {
		if (nodeAlloc != nullptr)
			return &nodeAlloc->alloc[0];
		else
			return nullptr;
	}
	
// ------------------------------------------------------------------------------------------ //
};
