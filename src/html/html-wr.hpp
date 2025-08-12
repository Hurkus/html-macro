#pragma once
#include <cstdint>
#include <string_view>
#include "html-allocator.hpp"
#include "EnumOperators.hpp"


namespace html {
	enum class node_type : uint8_t;
	enum class node_options : uint8_t;
	
	struct wr_node;
	struct wr_attr;
	class wr_document;
	
	struct nocopy {};
}


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


enum class html::node_options : uint8_t {
	NONE        = 0 << 0,
	CONST_NAME  = 1 << 0,
	CONST_VALUE = 1 << 1
};
ENUM_OPERATORS(html::node_options);


/* Writeable node for building an HTML structure. */
struct html::wr_node {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	node_type type = node_type::TAG;
	node_options options = node_options::NONE;
	
	uint32_t value_len = 0;				// Length of `value_p`.
	const char* value_p = nullptr;		// Unterminated value/name string.
	
	html::wr_node* parent = nullptr;
	html::wr_node* child = nullptr;		// Last child in linked list.
	html::wr_node* prev = nullptr;		// Linked list.
	
	html::wr_attr* attribute = nullptr;	// Last attribute in linked list.
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	// Get node value.
	std::string_view value() const {
		return std::string_view(value_p, value_len);
	}
	
	// Set value by allocating and copying the string.
	void value(wr_document& root, std::string_view value) noexcept;
	
	// Set value to const string.
	void value(wr_document& root, std::string_view value, nocopy) noexcept;
	
public:
	// Get node name
	std::string_view name() const {
		return std::string_view(value_p, value_len);
	}
	
	// Set name by allocating and copying the string.
	void name(wr_document& root, std::string_view name) noexcept {
		this->value(root, name);
	}
	
	// Set name to const string.
	void name(wr_document& root, std::string_view name, nocopy) noexcept {
		this->value(root, name, nocopy{});
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	wr_node* appendChild(wr_document& root, node_type type = node_type::TAG) noexcept;
	wr_attr* appendAttr(wr_document& root) noexcept;
	
	bool removeChild(wr_document& root, wr_node& child) noexcept;
	bool removeAttr(wr_document& root, wr_attr& attr) noexcept;
	
	bool removeAllAttr(wr_document& root) noexcept;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	wr_document& root(){
		if (parent != nullptr)
			return parent->root();
		assert(type == node_type::ROOT);
		return *(wr_document*)this;
	}
	
// ------------------------------------------------------------------------------------------ //
};



struct html::wr_attr {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	node_options options = node_options::NONE;
	
	uint8_t name_len = 0;
	uint32_t value_len = 0;
	const char* name_p = nullptr;
	const char* value_p = nullptr;
	
	html::wr_attr* prev = nullptr;	// Linked list.
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	// Get node value.
	std::string_view value() const {
		return std::string_view(value_p, value_len);
	}
	
	// Set value by allocating and copying the string.
	void value(wr_document& root, std::string_view value) noexcept;
	
	// Set value to const string.
	void value(wr_document& root, std::string_view value, nocopy) noexcept;
	
public:
	// Get node name
	std::string_view name() const {
		return std::string_view(value_p, value_len);
	}
	
	// Set name by allocating and copying the string.
	void name(wr_document& root, std::string_view name) noexcept;
	
	// Set name to const string.
	void name(wr_document& root, std::string_view name, nocopy) noexcept;
	
// ------------------------------------------------------------------------------------------ //
};




class html::wr_document : public wr_node {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	char_allocator strAlloc;
	block_allocator<wr_node> nodeAlloc;
	block_allocator<wr_attr> attrAlloc;
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	wr_document(){
		wr_node::type = node_type::ROOT;
	}
	
// ------------------------------------------------------------------------------------------ //
};
