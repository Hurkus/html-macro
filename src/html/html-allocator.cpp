#include "html-allocator.hpp"
#include <cassert>
#include <memory>
#include <vector>

#include "html.hpp"

using namespace std;
using namespace html;


// ----------------------------------- [ Structures ] --------------------------------------- //


template<int SIZE, int ALIGN>
struct heap_element_t {
	static constexpr size_t size = SIZE;
	static constexpr size_t align = ALIGN;
};

template<typename T>
using heap_t = heap_element_t<sizeof(T),alignof(T)>;	// H


// ----------------------------------- [ Structures ] --------------------------------------- //


template<class H>
struct page {
	int size;
	int count;
	
	union block {
		block* next;
		alignas(H::align)
		uint8_t obj[H::size];
	};
	
	alignas(H::align)
	block memory[];
	
};


template<class H>
struct block_allocator {
	using page = ::page<H>;
	using block = page::block;
	
	vector<unique_ptr<page>> pages;
	size_t nextSize = 0;
	block* emptyList = nullptr;		// Linked list of empty blocks.
	
	unique_ptr<page> createPage();
	void* allocate();
	void deallocate(void*);
};


// ----------------------------------- [ Variables ] ---------------------------------------- //


constexpr size_t MIN_PAGE_SIZE = 1024;
constexpr size_t MAX_PAGE_SIZE = 1024;
constexpr size_t PAGE_SIZE_INC = 0;

template<class H>
static block_allocator<H> heap;


// ----------------------------------- [ Functions ] ---------------------------------------- //


template<class H>
unique_ptr<page<H>> block_allocator<H>::createPage(){
	size_t n = max(min(MIN_PAGE_SIZE, nextSize), MAX_PAGE_SIZE);
	
	// Try alloc page at different sizes
	page* p;
	while (true){
		p = (page*)::operator new(sizeof(page) + n*H::size, align_val_t(H::align), nothrow_t{});
		
		if (p != nullptr)
			break;
		else if (n > MIN_PAGE_SIZE)
			n = (n/2 < MIN_PAGE_SIZE) ? MIN_PAGE_SIZE : n/2;
		else
			throw bad_alloc();
		
	}
	
	nextSize = n + PAGE_SIZE_INC;
	p->count = 0;
	p->size = n;
	return unique_ptr<page>(p);
}


template<class H>
void* block_allocator<H>::allocate(){
	// Reuse
	if (emptyList != nullptr){
		block* b = emptyList;
		emptyList = emptyList->next;
		return &b->obj;
	}
	
	// New or initial page
	else if (pages.empty() || pages.back()->count >= pages.back()->size){
		try {
			pages.emplace_back(createPage());
		} catch (...) {
			throw bad_alloc();
		}
	}
	
	// Take from last page
	page& p = *pages.back();
	block* b = &p.memory[p.count++];
	return &b->obj;
}


template<class H>
void block_allocator<H>::deallocate(void* p){
	assert(p != nullptr);
	
	// Find page which contains `p`
	for (unique_ptr<page>& page : pages){
		const block* beg = &page->memory[0];
		const block* end = beg + page->size;
		if (beg <= p && p < end)
			goto found;
	}
	
	// Not found
	throw bad_alloc();
	
	found:
	block* b = (block*)p;
	b->next = emptyList;
	emptyList = b;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


node* html::newNode(){
	using H = heap_t<node>;
	void* e = heap<H>.allocate();
	return new (e) node();
}


attr* html::newAttr(){
	using H = heap_t<attr>;
	void* e = heap<H>.allocate();
	return new (e) attr();
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void html::del(node* p){
	using H = heap_t<decltype(*p)>;
	if (p != nullptr)
		heap<H>.deallocate(p);
}


void html::del(attr* p){
	using H = heap_t<decltype(*p)>;
	if (p != nullptr)
		heap<H>.deallocate(p);
}


// ------------------------------------------------------------------------------------------ //