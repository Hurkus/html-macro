#include "html.hpp"
#include <cassert>

using namespace std;
using namespace html;


// ----------------------------------- [ Functions ] ---------------------------------------- //


template<bool OPT = true>
inline void _free_name(Node& self) noexcept {
	if (self.options % (NodeOptions::OWNED_VALUE | NodeOptions::OWNED_NAME)){
		html::del((char*)self.value_p);
		if constexpr (OPT)
			self.options &= ~(NodeOptions::OWNED_VALUE | NodeOptions::OWNED_NAME);
	}
}


template<bool OPT = true>
inline void _free_name(Attr& self) noexcept {
	if (self.options % NodeOptions::OWNED_NAME){
		html::del((char*)self.name_p);
		if constexpr (OPT)
			self.options &= ~(NodeOptions::OWNED_NAME);
	}
}


template<bool OPT = true>
inline void _free_value(Attr& self) noexcept {
	if (self.options % NodeOptions::OWNED_VALUE){
		html::del((char*)self.value_p);
		if constexpr (OPT)
			self.options &= ~(NodeOptions::OWNED_VALUE);
	}
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void Node::value(nullptr_t){
	_free_name(*this);
	value_p = nullptr;
	value_len = 0;
}


void Node::value(string_view str){
	_free_name(*this);
	value_p = str.data();
	value_len = uint32_t(min(str.length(), size_t(UINT32_MAX)));
}


void Node::value(char* str, size_t len){
	_free_name<false>(*this);
	options |= (NodeOptions::OWNED_VALUE | NodeOptions::OWNED_NAME);
	value_p = str;
	value_len = uint32_t(min(len, size_t(UINT32_MAX)));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Node& Node::appendChild(NodeType type){
	assert(!(this->options % NodeOptions::LIST_FORWARDS));
	Node* child = new Node();
	child->type = type;
	child->parent = this;
	child->next = this->child;
	this->child = child;
	return *child;
}

void Node::appendChild(Node* child) noexcept {
	assert(child != nullptr);
	assert(!(this->options % NodeOptions::LIST_FORWARDS));
	
	if (child->parent != nullptr) [[unlikely]] {
		child->parent->extractChild(child);
	}
	
	child->parent = this;
	child->next = this->child;
	this->child = child;
}


Node* Node::extractChild(Node* child) noexcept {
	Node* prev = nullptr;
	Node* curr = this->child;
	
	if (child == nullptr){
		return nullptr;
	}
	
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


bool Node::removeChild(Node* child){
	child = extractChild(child);
	delete child;
	return child != nullptr;
}


void Node::removeChildren(){
	Node* stack = this->child;
	this->child = nullptr;
	
	while (stack != nullptr){
		// Pop 1
		Node* node = stack;
		stack = node->next;
		
		// Push children to stack
		for (Node* c = node->child ; c != nullptr ; ){
			Node* next = c->next;
			
			c->next = stack;
			stack = c;
			
			c = next;
		}
		
		// Skip destructor
		node->removeAttributes();
		_free_name(*node);
		Node::operator delete(node);
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void Attr::value(nullptr_t){
	_free_value(*this);
	value_p = nullptr;
	value_len = 0;
}


void Attr::value(string_view str){
	_free_value(*this);
	value_p = str.data();
	value_len = uint32_t(min(str.length(), size_t(UINT32_MAX)));
}


void Attr::value(char* str, size_t len){
	_free_value<false>(*this);
	options |= NodeOptions::OWNED_VALUE;
	value_p = str;
	value_len = uint32_t(min(len, size_t(UINT32_MAX)));
}


void Attr::name(nullptr_t){
	_free_name(*this);
	name_p = nullptr;
	name_len = 0;
}


void Attr::name(string_view str){
	_free_name(*this);
	name_p = str.data();
	name_len = uint32_t(min(str.length(), size_t(UINT16_MAX)));
}


void Attr::name(char* str, size_t len){
	_free_name<false>(*this);
	options |= NodeOptions::OWNED_NAME;
	name_p = str;
	name_len = uint32_t(min(len, size_t(UINT16_MAX)));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Attr& Node::appendAttribute(){
	assert(!(this->options % NodeOptions::LIST_FORWARDS));
	Attr* a = new Attr();
	a->next = this->attribute;
	this->attribute = a;
	return *a;
}


Attr* Node::extractAttr(Attr* attr) noexcept {
	Attr* prev = nullptr;
	Attr* curr = this->attribute;
	
	if (attr == nullptr){
		return nullptr;
	}
	
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


static Attr* extractAttr(Node& self, string_view name){
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


bool Node::removeAttr(Attr* attr){
	attr = extractAttr(attr);
	delete attr;
	return attr != nullptr;
}


bool Node::removeAttr(string_view name){
	Attr* attr = ::extractAttr(*this, name);
	delete attr;
	return attr != nullptr;
}


void Node::removeAttributes(){
	for (Attr* attr = this->attribute ; attr != nullptr ; ){
		Attr* next = attr->next;
		
		// Skip destructor
		_free_value(*attr);
		_free_name(*attr);
		Attr::operator delete(attr);
		
		attr = next;
	}
	this->attribute = nullptr;
}


// ---------------------------------- [ Constructors ] -------------------------------------- //


Node::~Node(){
	assert(this->next == nullptr);
	assert(this->parent == nullptr);
	
	this->removeChildren();
	this->removeAttributes();
	_free_name<false>(*this);
}


Attr::~Attr(){
	assert(this->next == nullptr);
	
	_free_value<false>(*this);
	_free_name<false>(*this);
}


// ------------------------------------------------------------------------------------------ //