#include "html.hpp"
#include <cassert>

using namespace std;
using namespace html;


// ---------------------------------- [ Constructors ] -------------------------------------- //


inline void _free_str(node& n){
	if (isSet(n.options, node_options::OWNED_VALUE | node_options::OWNED_NAME))
		html::del((char*)n.value_p);
}


inline void _free_str(attr& a){
	if (isSet(a.options, node_options::OWNED_VALUE))
		html::del((char*)a.value_p);
	if (isSet(a.options, node_options::OWNED_NAME))
		html::del((char*)a.name_p);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void node::value(nullptr_t){
	if (isSet(options, node_options::OWNED_VALUE | node_options::OWNED_NAME)){
		html::del((char*)value_p);
		options &= ~(node_options::OWNED_VALUE | node_options::OWNED_NAME);
	}
	value_p = nullptr;
	value_len = 0;
}


void node::value(string_view str){
	if (isSet(options, node_options::OWNED_VALUE | node_options::OWNED_NAME)){
		html::del((char*)value_p);
		options &= ~(node_options::OWNED_VALUE | node_options::OWNED_NAME);
	}
	value_p = str.data();
	value_len = uint32_t(min(str.length(), size_t(UINT32_MAX)));
}


void node::value(char* str, size_t len){
	if (isSet(options, node_options::OWNED_VALUE | node_options::OWNED_NAME)){
		html::del((char*)value_p);
	}
	options |= (node_options::OWNED_VALUE | node_options::OWNED_NAME);
	value_p = str;
	value_len = uint32_t(min(len, size_t(UINT32_MAX)));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


node& node::appendChild(node_type type){
	assert(!isSet(this->options, node_options::LIST_FORWARDS));
	node* c = html::newNode();
	c->type = type;
	c->parent = this;
	c->next = this->child;
	this->child = c;
	return *c;
}


attr& node::appendAttribute(){
	attr* a = html::newAttr();
	a->next = this->attribute;
	this->attribute = a;
	return *a;
}


attr* node::extractAttr(attr* a){
	attr* prev = nullptr;
	attr* curr = this->attribute;
	
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


bool node::removeAttr(attr* a){
	attr* prev = nullptr;
	attr* curr = this->attribute;
	
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


void node::removeAttributes(){
	attr* a = this->attribute;
	this->attribute = nullptr;
	
	while (a != nullptr){
		attr* _a = a;
		a = a->next;
		_free_str(*_a);
		html::del(_a);
	}
	
}


node* node::extractChild(node* child){
	node* prev = nullptr;
	node* curr = this->child;
	
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


bool node::removeChild(node* child){
	node* prev = nullptr;
	node* curr = this->child;
	
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


void node::removeChildren(){
	node* stack = this->child;
	this->child = nullptr;
	
	while (stack != nullptr){
		// Pop 1
		node* p = stack;
		stack = p->next;
		
		// Push children to stack
		node* c = p->child;
		while (c != nullptr){
			node* _c = c;
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


void attr::value(nullptr_t){
	if (isSet(options, node_options::OWNED_VALUE)){
		html::del((char*)value_p);
		options &= ~node_options::OWNED_VALUE;
	}
	value_p = nullptr;
	value_len = 0;
}


void attr::value(string_view str){
	if (isSet(options, node_options::OWNED_VALUE)){
		html::del((char*)value_p);
		options &= ~node_options::OWNED_VALUE;
	}
	value_p = str.data();
	value_len = uint32_t(min(str.length(), size_t(UINT32_MAX)));
}


void attr::value(char* str, size_t len){
	if (isSet(options, node_options::OWNED_VALUE)){
		html::del((char*)value_p);
	}
	options |= node_options::OWNED_VALUE;
	value_p = str;
	value_len = uint32_t(min(len, size_t(UINT32_MAX)));
}


void attr::name(nullptr_t){
	if (isSet(options, node_options::OWNED_NAME)){
		html::del((char*)name_p);
		options &= ~node_options::OWNED_NAME;
	}
	name_p = nullptr;
	name_len = 0;
}


void attr::name(string_view str){
	if (isSet(options, node_options::OWNED_NAME)){
		html::del((char*)name_p);
		options &= ~node_options::OWNED_NAME;
	}
	name_p = str.data();
	name_len = uint32_t(min(str.length(), size_t(UINT16_MAX)));
}


void attr::name(char* str, size_t len){
	if (isSet(options, node_options::OWNED_NAME)){
		html::del((char*)name_p);
	}
	options |= node_options::OWNED_NAME;
	name_p = str;
	name_len = uint32_t(min(len, size_t(UINT16_MAX)));
}


// ------------------------------------------------------------------------------------------ //