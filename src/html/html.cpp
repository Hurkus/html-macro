#include "html.hpp"
#include <cassert>

using namespace std;
using namespace html;


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline void _free_name(CharAllocator& alloc, Node& self) noexcept {
	if (self.options % (NodeOptions::OWNED_NAME | NodeOptions::OWNED_VALUE)){
		alloc.dealloc((char*)self.value_p);
		self.options &= ~(NodeOptions::OWNED_NAME | NodeOptions::OWNED_VALUE);
	}
}


inline void _free_name(CharAllocator& alloc, Attr& self) noexcept {
	if (self.options % NodeOptions::OWNED_NAME){
		alloc.dealloc((char*)self.name_p);
		self.options &= ~NodeOptions::OWNED_NAME;
	}
}


inline void _free_value(CharAllocator& alloc, Attr& self) noexcept {
	if (self.options % NodeOptions::OWNED_VALUE){
		alloc.dealloc((char*)self.value_p);
		self.options &= ~NodeOptions::OWNED_VALUE;
	}
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void Node::value(Document& root, nullptr_t) noexcept {
	_free_name(*root.charAlloc, *this);
	value_p = nullptr;
	value_len = 0;
}


void Node::value(Document& root, string_view str) noexcept {
	_free_name(*root.charAlloc, *this);
	value_p = str.data();
	value_len = uint32_t(min(str.length(), size_t(UINT32_MAX)));
}


void Node::value(Document& root, char* str, size_t len) noexcept {
	_free_name(*root.charAlloc, *this);
	options |= NodeOptions::OWNED_VALUE;
	value_p = str;
	value_len = uint32_t(min(len, size_t(UINT32_MAX)));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void Node::remove(Document& root) noexcept {
	if (parent != nullptr){
		Node* n = parent->extractChild(this);
		assert(n == this);
	}
	clear(root);
	root.nodeAlloc->dealloc(this);
}


Node* Node::appendChild(Node* child) noexcept {
	assert(child != nullptr);
	assert(child->parent == nullptr);
	assert(child->next == nullptr);
	child->parent = this;
	child->next = this->child;
	this->child = child;
	return child;
}


Node* Node::extractChild(Node* child) noexcept {
	assert(child != nullptr);
	Node* prev = nullptr;
	Node* curr = this->child;
	
	// Find previous node
	while (curr != child){
		if (curr == nullptr)
			return nullptr;
		prev = curr;
		curr = curr->next;
	}
	
	// Remove from linked list
	if (prev == nullptr){
		this->child = child->next;
	} else {
		prev->next = child->next;
	}
	
	child->next = nullptr;
	child->parent = nullptr;
	return child;
}


void Node::removeChildren(Document& root) noexcept {
	for (Node* stack = this->child ; stack != nullptr ; ){
		// Pop 1
		Node* node = stack;
		stack = node->next;
		
		// Push children to stack
		for (Node* child = node->child ; child != nullptr ; ){
			Node* next = child->next;
			child->next = stack;
			stack = child;
			child = next;
		}
		
		// Delete attributes
		for (Attr* attr = node->attribute ; attr != nullptr ; ){
			Attr* next = attr->next;
			_free_name(*root.charAlloc, *attr);
			_free_value(*root.charAlloc, *attr);
			root.attrAlloc->dealloc(attr);
			attr = next;
		}
		
		_free_name(*root.charAlloc, *node);
		root.nodeAlloc->dealloc(node);
	}
	this->child = nullptr;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void Attr::value(Document& root, nullptr_t) noexcept {
	_free_value(*root.charAlloc, *this);
	value_p = nullptr;
	value_len = 0;
}


void Attr::value(Document& root, string_view str) noexcept {
	_free_value(*root.charAlloc, *this);
	value_p = str.data();
	value_len = uint32_t(min(str.length(), size_t(UINT32_MAX)));
}


void Attr::value(Document& root, char* str, size_t len) noexcept {
	_free_value(*root.charAlloc, *this);
	options |= NodeOptions::OWNED_VALUE;
	value_p = str;
	value_len = uint32_t(min(len, size_t(UINT32_MAX)));
}


void Attr::name(Document& root, nullptr_t) noexcept {
	_free_name(*root.charAlloc, *this);
	name_p = nullptr;
	name_len = 0;
}


void Attr::name(Document& root, string_view str) noexcept {
	_free_name(*root.charAlloc, *this);
	name_p = str.data();
	name_len = uint16_t(min(str.length(), size_t(UINT16_MAX)));
}


void Attr::name(Document& root, char* str, size_t len) noexcept {
	_free_name(*root.charAlloc, *this);
	options |= NodeOptions::OWNED_NAME;
	name_p = str;
	name_len = uint16_t(min(len, size_t(UINT16_MAX)));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Attr* Node::appendAttribute(Attr* attr) noexcept {
	assert(attr != nullptr);
	attr->next = this->attribute;
	this->attribute = attr;
	return attr;
}


Attr* Node::extractAttr(Attr* attr) noexcept {
	assert(attr != nullptr);
	Attr* prev = nullptr;
	Attr* curr = this->attribute;
	
	// Find previous attribute
	while (curr != attr){
		if (curr == nullptr)
			return nullptr;
		prev = curr;
		curr = curr->next;
	}
	
	if (prev == nullptr){
		this->attribute = attr->next;
	} else {
		prev->next = attr->next;
	}
	
	attr->next = nullptr;
	return attr;
}


static Attr* extractAttr(Node& self, string_view name) noexcept {
	Attr* prev = nullptr;
	Attr* attr = self.attribute;
	
	// Find previous attribute
	while (attr != nullptr && attr->name() != name){
		prev = attr;
		attr = attr->next;
	}
	
	// Extract from linked list
	if (attr != nullptr){
		if (prev == nullptr)
			self.attribute = attr->next;
		else
			prev->next = attr->next;
		
		attr->next = nullptr;
	}
	
	return attr;
}


bool Node::removeAttr(Document& root, string_view name) noexcept {
	Attr* attr = ::extractAttr(*this, name);
	if (attr != nullptr){
		attr->clear(root);
		root.attrAlloc->dealloc(attr);
		return true;
	}
	return false;
}


void Node::removeAttributes(Document& root) noexcept {
	for (Attr* attr = this->attribute ; attr != nullptr ; ){
		Attr* next = attr->next;
		_free_name(*root.charAlloc, *attr);
		_free_value(*root.charAlloc, *attr);
		root.attrAlloc->dealloc(attr);
		attr = next;
	}
	this->attribute = nullptr;
}


// ------------------------------------------------------------------------------------------ //