#include "html-wr.hpp"
#include <cassert>

using namespace std;
using namespace html;


// ----------------------------------- [ Functions ] ---------------------------------------- //


html::wr_node* wr_node::appendChild(wr_document& root, node_type type) noexcept {
	assert(&root == &this->root());
	
	wr_node* node = root.nodeAlloc.alloc();
	if (node != nullptr){
		node->type = type;
		node->parent = this;
		node->prev = this->child;
		this->child = node;
	}
	
	return node;
}


bool wr_node::removeChild(wr_document& root, wr_node& child) noexcept {
	if (!removeAllAttr(root)){
		assert(&this->root() == &root);
		return false;
	}
	
	wr_node* prev = this->child;
	wr_node* next = nullptr;
	
	while (prev != &child){
		if (prev == nullptr)
			return false;
		next = prev;
		prev = prev->prev;
	}
	
	prev = child.prev;
	
	// Dealloc
	if (!root.nodeAlloc.dealloc(&child)){
		assert(&this->root() == &root);
		return false;
	}
	
	// Fix linked list
	if (next != nullptr){
		next->prev = prev;
	} else {
		this->child = prev;
	}
	
	return true;
}


bool wr_node::removeAllAttr(wr_document& root) noexcept {
	wr_attr* attr = this->attribute;
	
	while (attr != nullptr){
		wr_attr* next = attr->prev;
		
		if (!root.attrAlloc.dealloc(attr)){
			this->attribute = attr;
			return false;
		}
		
		attr = next;
	}
	
	this->attribute = nullptr;
	return true;
}


html::wr_attr* wr_node::appendAttr(wr_document& root) noexcept {
	assert(&root == &this->root());
	
	wr_attr* attr = root.attrAlloc.alloc();
	if (attr != nullptr){
		attr->prev = this->attribute;
		this->attribute = attr;
	}
	
	return attr;
}


bool wr_node::removeAttr(wr_document& root, wr_attr& attr) noexcept {
	wr_attr* prev = this->attribute;
	wr_attr* next = nullptr;
	
	while (prev != &attr){
		if (prev == nullptr)
			return false;
		next = prev;
		prev = prev->prev;
	}
	
	prev = attr.prev;
	
	// Dealloc
	if (!root.attrAlloc.dealloc(&attr)){
		assert(&this->root() == &root);
		return false;
	}
	
	// Fix linked list
	if (next != nullptr){
		next->prev = prev;
	} else {
		this->attribute = prev;
	}
	
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template<node_options OPT>
inline void set_str(char_allocator& allocator, node_options& options, const char*& str_p, auto& str_len, string_view value){
	if (!isSet(options, OPT)){
		if (str_p == nullptr){
			goto alloc;
		}
		
		// Shrink existing allocation
		else if (value.length() <= str_len){
			str_len = value.length();
			goto cpy;
		}
		
		// Deallocate
		else if (!allocator.dealloc(str_p)){
			assert(false && "Failed to deallocate string.");
			return;
		}
		
	} else {
		options &= ~OPT;
	}
		
	alloc:
	str_len = value.length();
	str_p = allocator.alloc(value.length());
	
	if (str_p == nullptr){
		assert(false && "Failed to allocate string.");
		return;
	}
	
	cpy:
	copy(value.begin(), value.end(), (char*)str_p);
}


template<node_options OPT>
inline void set_str_const(char_allocator& allocator, node_options& options, const char*& str_p, auto& str_len, string_view value){
	// Deallocate
	if (str_p != nullptr && !isSet(options, OPT)){
		
		if (!allocator.dealloc(str_p)){
			assert(false && "Failed to deallocate string.");
			return;
		}
		
	}
	
	str_p = value.data();
	str_len = value.length();
	options |= OPT;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void wr_node::value(wr_document& root, string_view newValue) noexcept {
	assert(&this->root() == &root);
	assert(newValue.length() <= root.strAlloc.MAX_LEN && newValue.length() <= UINT32_MAX);
	newValue = newValue.substr(0, min(root.strAlloc.MAX_LEN, size_t(UINT32_MAX)));
	set_str<node_options::CONST_VALUE>(root.strAlloc, options, value_p, value_len, newValue);
}

void wr_node::value(wr_document& root, std::string_view newValue, nocopy) noexcept {
	assert(&this->root() == &root);
	assert(newValue.length() <= UINT32_MAX);
	newValue = newValue.substr(0, size_t(UINT32_MAX));
	set_str_const<node_options::CONST_VALUE>(root.strAlloc, options, value_p, value_len, newValue);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void wr_attr::value(wr_document& root, string_view newValue) noexcept {
	assert(newValue.length() <= root.strAlloc.MAX_LEN && newValue.length() <= UINT32_MAX);
	newValue = newValue.substr(0, min(root.strAlloc.MAX_LEN, size_t(UINT32_MAX)));
	set_str<node_options::CONST_VALUE>(root.strAlloc, options, value_p, value_len, newValue);
}

void wr_attr::value(wr_document& root, string_view newValue, nocopy) noexcept {
	assert(newValue.length() <= UINT32_MAX);
	newValue = newValue.substr(0, UINT32_MAX);
	set_str_const<node_options::CONST_VALUE>(root.strAlloc, options, value_p, value_len, newValue);
}


void wr_attr::name(wr_document& root, string_view newName) noexcept {
	assert(newName.length() <= root.strAlloc.MAX_LEN && newName.length() <= UINT8_MAX);
	newName = newName.substr(0, min(root.strAlloc.MAX_LEN, size_t(UINT8_MAX)));
	set_str<node_options::CONST_NAME>(root.strAlloc, options, name_p, name_len, newName);
}

void wr_attr::name(wr_document& root, string_view newName, nocopy) noexcept {
	assert(newName.length() <= UINT8_MAX);
	newName = newName.substr(0, UINT8_MAX);
	set_str<node_options::CONST_NAME>(root.strAlloc, options, name_p, name_len, newName);
}


// ------------------------------------------------------------------------------------------ //