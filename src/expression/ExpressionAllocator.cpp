#include "ExpressionAllocator.hpp"
#include <cassert>

using namespace std;


// ---------------------------------- [ Constructors ] -------------------------------------- //


Expression::Allocator::~Allocator(){
	Page* p = this->page;
	
	while (p != nullptr){
		auto* next = p->next;
		delete p;
		p = next;
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void* Expression::Allocator::alloc(size_t size){
	// Convert to unit, roudn up
	size = (size + UNIT - 1) / UNIT;
	
	if (size > PAGE_SIZE){
		throw bad_alloc();
	} else if (this->page == nullptr){ new_page:
		Page* page = new Page;
		page->count = 0;
		page->next = this->page;
		this->page = page;
	} else if (this->page->count + size > PAGE_SIZE){
		goto new_page;
	}
	
	assert(page != nullptr);
	void* p = &page->memory[page->count];
	page->count += size;
	return p;
}


// ------------------------------------------------------------------------------------------ //