#include "html-allocator.hpp"
#include <cassert>
#include <cstring>
#include <memory>
#include <vector>

#include "html.hpp"
#include "Debug.hpp"

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
struct Page {
	int size;
	int count;
	
	union Block {
		Block* next;
		alignas(H::align)
		uint8_t obj[H::size];
	};
	
	alignas(H::align)
	Block memory[];
	
};


template<class H>
struct block_allocator {
	using page = ::Page<H>;
	using block = page::Block;
	
	vector<unique_ptr<page>> pages;
	size_t nextSize = 0;
	block* emptyList = nullptr;		// Linked list of empty blocks.
	
	unique_ptr<page> createPage();
	void* allocate();
	void deallocate(void*);
	bool contains(void*) const noexcept;
};


// ----------------------------------- [ Variables ] ---------------------------------------- //


constexpr size_t MIN_PAGE_SIZE = 1024;
constexpr size_t MAX_PAGE_SIZE = 1024;
constexpr size_t PAGE_SIZE_INC = 0;

template<class H>
static block_allocator<H> heap;


// ----------------------------------- [ Variables ] ---------------------------------------- //


#ifdef DEBUG
namespace html {
	long nodeAllocCount = 0;
	long nodeDeallocCount = 0;
	long attrAllocCount = 0;
	long attrDeallocCount = 0;
	long strAllocCount = 0;
	long strDeallocCount = 0;
}
#endif


#ifdef DEBUG
	#define INC(var)	(var++)
#else
	#define INC(var)	{}
#endif


// ----------------------------------- [ Functions ] ---------------------------------------- //


template<class H>
unique_ptr<Page<H>> block_allocator<H>::createPage(){
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
	assert(contains(p) && "Allocation not found.");
	block* b = (block*)p;
	b->next = emptyList;
	emptyList = b;
}


template<class H>
bool block_allocator<H>::contains(void* p) const noexcept {
	if (p == nullptr){
		return false;
	}
	
	for (const unique_ptr<page>& page : pages){
		const block* beg = &page->memory[0];
		const block* end = beg + page->size;
		if (beg <= p && p < end)
			return true;
	}
	
	return false;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Node* html::newNode(){
	using H = heap_t<Node>;
	INC(nodeAllocCount);
	void* e = heap<H>.allocate();
	return new (e) Node();
}


Attr* html::newAttr(){
	using H = heap_t<Attr>;
	INC(attrAllocCount);
	void* e = heap<H>.allocate();
	return new (e) Attr();
}


char* html::newStr(size_t len){
	INC(strAllocCount);
	return new char[len];
}

char* html::newStr(std::string_view str){
	char* s = newStr(str.length() + 1);
	memcpy(s, str.begin(), str.length());
	s[str.length()] = 0;
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void html::del(Node* p){
	using H = heap_t<decltype(*p)>;
	if (p != nullptr){
		heap<H>.deallocate(p);
		INC(nodeDeallocCount);
	}
}


void html::del(Attr* p){
	using H = heap_t<decltype(*p)>;
	if (p != nullptr){
		heap<H>.deallocate(p);
		INC(attrDeallocCount);
	}
}


void html::del(char* str){
	if (str != nullptr)
		INC(strDeallocCount);
	delete[] str;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool html::assertDeallocations(){
	bool pass = true;
	
	#ifdef DEBUG
	pass &= (nodeAllocCount == nodeDeallocCount);
	pass &= (attrAllocCount == attrDeallocCount);
	pass &= (strAllocCount == strDeallocCount);
	
	if (!pass){
		string msg = "HTML allocator encountered missmatching deallocation count:\n";
		msg += "  html::Node: " + to_string(nodeDeallocCount) + "/" + to_string(nodeAllocCount) + "\n";
		msg += "  html::Attr: " + to_string(attrDeallocCount) + "/" + to_string(attrAllocCount) + "\n";
		msg += "  html::Str:  " + to_string(strDeallocCount) + "/" + to_string(strAllocCount);
		ERROR("%s", msg.c_str());
	}
	#endif
	
	return pass;
}


// ------------------------------------------------------------------------------------------ //