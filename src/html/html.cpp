#include "html.hpp"
#include <cassert>

using namespace std;
using namespace html;


// ---------------------------------- [ Constructors ] -------------------------------------- //


inline void _free_str(Node& n){
	if (isSet(n.options, NodeOptions::OWNED_VALUE | NodeOptions::OWNED_NAME))
		html::del((char*)n.value_p);
}


inline void _free_str(Attr& a){
	if (isSet(a.options, NodeOptions::OWNED_VALUE))
		html::del((char*)a.value_p);
	if (isSet(a.options, NodeOptions::OWNED_NAME))
		html::del((char*)a.name_p);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void Node::value(nullptr_t){
	if (isSet(options, NodeOptions::OWNED_VALUE | NodeOptions::OWNED_NAME)){
		html::del((char*)value_p);
		options &= ~(NodeOptions::OWNED_VALUE | NodeOptions::OWNED_NAME);
	}
	value_p = nullptr;
	value_len = 0;
}


void Node::value(string_view str){
	if (isSet(options, NodeOptions::OWNED_VALUE | NodeOptions::OWNED_NAME)){
		html::del((char*)value_p);
		options &= ~(NodeOptions::OWNED_VALUE | NodeOptions::OWNED_NAME);
	}
	value_p = str.data();
	value_len = uint32_t(min(str.length(), size_t(UINT32_MAX)));
}


void Node::value(char* str, size_t len){
	if (isSet(options, NodeOptions::OWNED_VALUE | NodeOptions::OWNED_NAME)){
		html::del((char*)value_p);
	}
	options |= (NodeOptions::OWNED_VALUE | NodeOptions::OWNED_NAME);
	value_p = str;
	value_len = uint32_t(min(len, size_t(UINT32_MAX)));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Node& Node::appendChild(NodeType type){
	assert(!isSet(this->options, NodeOptions::LIST_FORWARDS));
	Node* c = html::newNode();
	c->type = type;
	c->parent = this;
	c->next = this->child;
	this->child = c;
	return *c;
}


Attr& Node::appendAttribute(){
	Attr* a = html::newAttr();
	a->next = this->attribute;
	this->attribute = a;
	return *a;
}


Attr* Node::extractAttr(Attr* a){
	Attr* prev = nullptr;
	Attr* curr = this->attribute;
	
	// Find previous attribute
	while (curr != a){
		if (curr == nullptr)
			return nullptr;
		prev = curr;
		curr = curr->next;
	}
	
	if (prev == nullptr){
		this->attribute = a->next;
	} else {
		prev->next = a->next;
	}
	
	a->next = nullptr;
	return a;
}


bool Node::removeAttr(Attr* a){
	Attr* prev = nullptr;
	Attr* curr = this->attribute;
	
	// Find previous attribute
	while (curr != a){
		if (curr == nullptr)
			return false;
		prev = curr;
		curr = curr->next;
	}
	
	// Remove from linked list
	if (prev == nullptr){
		this->attribute = a->next;
	} else {
		prev->next = a->next;
	}
	
	_free_str(*a);
	html::del(a);
	return true;
}


void Node::removeAttributes(){
	Attr* a = this->attribute;
	this->attribute = nullptr;
	
	while (a != nullptr){
		Attr* _a = a;
		a = a->next;
		_free_str(*_a);
		html::del(_a);
	}
	
}


Node* Node::extractChild(Node* child){
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


bool Node::removeChild(Node* child){
	Node* prev = nullptr;
	Node* curr = this->child;
	
	// Find previous node
	while (curr != child){
		if (curr == nullptr)
			return false;
		prev = curr;
		curr = curr->next;
	}
	
	// Remove from linked list
	if (prev == nullptr){
		this->child = child->next;
	} else {
		prev->next = child->next;
	}
	
	child->removeAttributes();
	child->removeChildren();
	_free_str(*child);
	html::del(child);
	return true;
}


void Node::removeChildren(){
	Node* stack = this->child;
	this->child = nullptr;
	
	while (stack != nullptr){
		// Pop 1
		Node* p = stack;
		stack = p->next;
		
		// Push children to stack
		Node* c = p->child;
		while (c != nullptr){
			Node* _c = c;
			c = c->next;
			
			// Add child to stack
			_c->next = stack;
			stack = _c;
		}
		
		_free_str(*p);
		p->removeAttributes();
		html::del(p);
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void Attr::value(nullptr_t){
	if (isSet(options, NodeOptions::OWNED_VALUE)){
		html::del((char*)value_p);
		options &= ~NodeOptions::OWNED_VALUE;
	}
	value_p = nullptr;
	value_len = 0;
}


void Attr::value(string_view str){
	if (isSet(options, NodeOptions::OWNED_VALUE)){
		html::del((char*)value_p);
		options &= ~NodeOptions::OWNED_VALUE;
	}
	value_p = str.data();
	value_len = uint32_t(min(str.length(), size_t(UINT32_MAX)));
}


void Attr::value(char* str, size_t len){
	if (isSet(options, NodeOptions::OWNED_VALUE)){
		html::del((char*)value_p);
	}
	options |= NodeOptions::OWNED_VALUE;
	value_p = str;
	value_len = uint32_t(min(len, size_t(UINT32_MAX)));
}


void Attr::name(nullptr_t){
	if (isSet(options, NodeOptions::OWNED_NAME)){
		html::del((char*)name_p);
		options &= ~NodeOptions::OWNED_NAME;
	}
	name_p = nullptr;
	name_len = 0;
}


void Attr::name(string_view str){
	if (isSet(options, NodeOptions::OWNED_NAME)){
		html::del((char*)name_p);
		options &= ~NodeOptions::OWNED_NAME;
	}
	name_p = str.data();
	name_len = uint32_t(min(str.length(), size_t(UINT16_MAX)));
}


void Attr::name(char* str, size_t len){
	if (isSet(options, NodeOptions::OWNED_NAME)){
		html::del((char*)name_p);
	}
	options |= NodeOptions::OWNED_NAME;
	name_p = str;
	name_len = uint32_t(min(len, size_t(UINT16_MAX)));
}


// ------------------------------------------------------------------------------------------ //