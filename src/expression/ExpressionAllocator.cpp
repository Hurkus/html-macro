#include "ExpressionAllocator.hpp"
#include <cassert>

using namespace std;
using Allocator = Expression::Allocator;
using Page = Expression::Allocator::Page;


// ---------------------------------- [ Constructors ] -------------------------------------- //


Allocator::~Allocator(){
	Page* p = this->page;
	
	while (p != nullptr){
		auto* next = p->next;
		delete p;
		p = next;
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Page* newPage(Page* prev, size_t minCapacity) noexcept {
	size_t capacity;
	if (prev != nullptr){
		capacity = max(prev->capacity * 2, minCapacity);
	} else {
		capacity = Allocator::MIN_CAPACITY;
	}
	
	assert(capacity > 0);
	void* mem = ::operator new(sizeof(Page) + capacity * sizeof(*Page::memory), align_val_t(alignof(Page)), nothrow);
	if (mem == nullptr){
		return nullptr;
	}
	
	Page* page = new (mem) Page();
	page->capacity = capacity;
	return page;
}


void* Allocator::alloc(size_t size){
	if (page == nullptr || (page->size + size) > page->capacity){
		Page* p = newPage(page, size);
		if (p == nullptr){
			throw bad_alloc();
		}
		
		p->next = page;
		page = p;
	}
	
	// Align
	size = ((size + UNIT - 1) / UNIT) * UNIT;
	
	// Alloc
	void* p = &page->memory[page->size];
	page->size += size;
	return p;
}


// ------------------------------------------------------------------------------------------ //