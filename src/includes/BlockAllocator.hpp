#pragma once
#include <cassert>
#include <cstdint>
#include <algorithm>


template<size_t BLOCK_SIZE, size_t BLOCK_ALIGN>
struct BlockAllocator {
// ----------------------------------- [ Constants ] ---------------------------------------- //
public:
	static constexpr size_t MIN_ELEMENTS_PER_PAGE = 16;	// elements
	static constexpr size_t MAX_PAGE_SIZE = 8*1024;		// bytes
	
// ----------------------------------- [ Structures ] --------------------------------------- //
private:
	// Either linked list of empty blocks or occupied element
	union Block {
		Block* next;
		
		alignas(BLOCK_ALIGN)
		uint8_t obj[BLOCK_SIZE];
	};
	
private:
	struct Page {
		Page* next = nullptr;
		size_t size = 0;
		size_t count = 0;
		
		alignas(BLOCK_ALIGN)
		Block memory[];
	};
	
// ------------------------------------[ Properties ] --------------------------------------- //
public:	
	Page* pages = nullptr;			// Linked list of pages.
	Block* emptyBlocks = nullptr;	// Linked list of empty blocks.
	
public:
	static BlockAllocator heap;		// Global heap of a specific allocator.
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	~BlockAllocator(){
		Page* p = pages;
		while (p != nullptr){
			Page* next = p->next;
			delete p;
			p = next;
		}
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	void* allocate() noexcept {
		// Reuse
		if (emptyBlocks != nullptr){
			Block* b = emptyBlocks;
			emptyBlocks = emptyBlocks->next;
			return &b->obj;
		}
		
		// New or initial page
		else if (pages == nullptr || pages->count >= pages->size){
			Page* newPage = createPage();
			if (newPage == nullptr)
				return nullptr;
			newPage->next = pages;
			pages = newPage;
		}
		
		// Take from last page
		Block& b = pages->memory[pages->count++];
		return &b.obj;
	}
	
public:
	void deallocate(void* p) noexcept {
		assert(p != nullptr);
		assert(contains(p) && "Allocation not found.");
		Block* b = reinterpret_cast<Block*>(p);
		b->next = emptyBlocks;
		emptyBlocks = b;
	}
	
public:
	bool contains(void* p) const noexcept {
		if (p == nullptr){
			return false;
		}
		
		for (const Page* page = pages ; page != nullptr ; page = page->next){
			const Block* beg = page->memory;
			const Block* end = beg + page->size;
			if (beg <= p && p < end)
				return true;
		}
		
		return false;
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
private:
	Page* createPage() const noexcept {
		constexpr size_t mem_available = std::max(MAX_PAGE_SIZE - sizeof(Page), MIN_ELEMENTS_PER_PAGE * BLOCK_SIZE);
		constexpr size_t count = mem_available / BLOCK_SIZE;
		constexpr size_t mem_size = sizeof(Page) + (count *BLOCK_SIZE);
		
		Page* p = (Page*)::operator new(mem_size, std::align_val_t(BLOCK_ALIGN), std::nothrow_t());
		if (p != nullptr){
			p->size = count;
			p->count = 0;
		}
		
		return p;
	}

// ------------------------------------------------------------------------------------------ //
};
