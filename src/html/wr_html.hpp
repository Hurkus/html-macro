#pragma once
#include <cstdint>
#include <string_view>
#include <memory>
#include <vector>

#include "html_allocator.hpp"


namespace html {
	enum class node_type : uint8_t;
	
	struct wr_node;
	struct wr_attr;
	class wr_document;
}


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


/* Writeable node for building an HTML structure. */
struct html::wr_node {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	node_type type = node_type::TAG;
	uint32_t value_len = 0;				// Length of `value_p`.
	const char* value_p = nullptr;		// Unterminated name/value string.
	
	html::wr_node* parent = nullptr;
	html::wr_node* child = nullptr;		// Last child in linked list.
	html::wr_node* prev = nullptr;		// Linked list.
	
	html::wr_attr* attribute = nullptr;	// Last attribute in linked list.
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	std::string_view value() const {
		return std::string_view(value_p, value_len);
	}
	
	std::string_view name() const {
		return value();
	}
	
	void value(std::string_view value);	// Set name.
	void name(std::string_view value);	// Set value.
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	wr_node* appendChild(std::string_view value = "", node_type type = node_type::TAG);
	wr_attr* appendAttr(std::string_view name = "", std::string_view value = "");
	
// ------------------------------------------------------------------------------------------ //
};



struct html::wr_attr {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	const char* name_p = nullptr;
	const char* value_p = nullptr;
	uint32_t name_len = 0;
	uint32_t value_len = 0;
	
	html::wr_attr* prev = nullptr;	// Linked list.
	
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




class html::wr_document {
	template<typename T>
	struct allocator;
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	// allocator<wr_node> nodeAlloc;
	// allocator<wr_attr> attrAlloc;
	// allocator<char> strAlloc;
	wr_node* _root = nullptr;
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	~wr_document();
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	wr_node* allocNode();
	wr_attr* allocAttr();
	// char* allocStr(size_t length);
	
public:
	wr_node* root(){
		// if (_root == nullptr)
			// _root = allocNode();
		return _root;
	}
	
// ------------------------------------------------------------------------------------------ //
};
