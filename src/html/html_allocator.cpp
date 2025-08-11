#include "html_allocator.hpp"
#include "wr_html.hpp"
#include <cstring>

using namespace std;
using namespace html;


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool newPage(char_allocator& self, size_t minSize){
	using page = char_allocator::page;
	
	minSize = (minSize < self.PAGE_MIN_SIZE ? self.PAGE_MIN_SIZE : minSize);
	if (minSize > UINT32_MAX){
		return false;
	}
	
	size_t size = size_t(self.pageSize) * 2;
	size = (size > self.PAGE_MAX_SIZE ? self.PAGE_MAX_SIZE : size);
	size = (size < minSize ? minSize : size);
	
	// Try allocate memory page at different sizes.
	page* p;
	while (true){
		p = (page*) ::operator new (sizeof(page) + size, align_val_t(alignof(page)), nothrow_t());
		if (p != nullptr){
			break;
		}
		
		// Reduce size
		if (size > minSize){
			size /= 2;
			size = (size < minSize ? minSize : size);
			continue;
		}
		
		return false;
	}

	try {
		self.pages.emplace_back(p);
	} catch (...) {
		delete p;
		return false;
	}
	
	assert(size <= UINT16_MAX);
	self.pageSize = size;
	p->space = size;
	p->size = size;
	p->deleted = 0;
	return true;
}


char* char_allocator::alloc(uint16_t len) noexcept {
	if (len <= 0 || len >= MAX_LEN){
		assert(len <= MAX_LEN);
		return nullptr;
	}
	
	const size_t reqLen = size_t(len) + sizeof(uint16_t);
	
	// Create new page
	if (pages.size() <= 0 || pages.back()->space < reqLen){
		if (!newPage(*this, reqLen))
			return nullptr;
	}
	
	assert(pages.back()->space >= reqLen);
	
	// Reserve chunk of memory.
	page& p = *pages.back();
	char* s = &p.memory[p.size - p.space];
	p.space -= reqLen;
	
	// Bubble sort
	size_t i = pages.size() - 1;
	while (i >= 1 && pages[i]->space < pages[i-1]->space){
		swap(pages[i], pages[i-1]);
		i--;
	}
	
	s[0] = uint8_t((len >> 0) & 0xFF);
	s[1] = uint8_t((len >> 8) & 0xFF);
	return s + 2;
}


string_view char_allocator::alloc(string_view str) noexcept {
	char* s = alloc(str.length());
	
	if (s != nullptr){
		memcpy(s, str.begin(), str.length());
		return string_view(s, str.length());
	}
	
	return {};
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool char_allocator::dealloc(char* str) noexcept {
	// Find page
	size_t i;
	for (i = 0 ; i < pages.size() ; i++){
		const char* beg = pages[i]->memory;
		const char* end = beg + pages[i]->size;
		if (beg <= str && str <= end)
			goto found;
	}
	
	return false;
	found:
	
	// Mark deleted
	str -= 2;
	const uint16_t len = (uint16_t(str[0]) << 0) | (uint16_t(str[1]) << 8);
	pages[i]->deleted += len + 2;
	
	// Check if not completely empty
	if (pages[i]->deleted < (pages[i]->size - pages[i]->space)){
		return true;
	}
	
	pages[i]->deleted = 0;
	pages[i]->space = pages[i]->size;
	
	if (pages.size() <= 1){
		return true;
	}
	
	// Extract page
	unique_ptr<page> page = move(pages[i]);
	pages.erase(pages.begin() + i);
	
	// Reinsert at top
	if (pages.back()->space < pages.back()->size){
		pages.emplace_back(move(page));
	}
	
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template<typename T>
static bool addPage(block_allocator<T>& self) noexcept {
	using SELF = block_allocator<T>;
	using page = block_allocator<T>::page;
	
	size_t size = self.pageSize + SELF::PAGE_SIZE_INC;
	size = (size > SELF::MAX_PAGE_SIZE) ? SELF::MAX_PAGE_SIZE : size;
	size = (size < SELF::MIN_PAGE_SIZE) ? SELF::MIN_PAGE_SIZE : size;
	
	// Try alloc page at different sizes
	page* p;
	while (true){
		p = static_cast<page*>(::operator new(sizeof(page) + sizeof(T)*size));
		
		if (p != nullptr)
			break;
		else if (size == SELF::MIN_PAGE_SIZE)
			return false;
		else
			size = (size/2 < SELF::MIN_PAGE_SIZE) ? SELF::MIN_PAGE_SIZE : size/2;
		
	}
	
	try {
		self.pages.emplace_back(p);
		self.pageSize = size;
	} catch (...){
		delete p;
		return false;
	}
	
	p->count = 0;
	p->size = size;
	p->free_idx = 0;
	p->memory[0].next = -1;
	p->memory[0].size = size;
	return true;
}


template<typename T> requires std::is_trivially_destructible<T>::value
T* block_allocator<T>::alloc() noexcept {
	// Find page with free block
	assert(pageHint >= 0);
	while (pageHint < int(pages.size())){
		if (pages[pageHint]->free_idx >= 0)
			goto emplace;
		pageHint++;
	}
	
	// New page
	if (!addPage<T>(*this)){
		return nullptr;
	} else {
		pageHint = int(pages.size()) - 1;
	}
	
	emplace:
	assert(pageHint < int(pages.size()));
	assert(pages[pageHint]->free_idx >= 0);
	page& p = *pages[pageHint];
	block& b = p.memory[p.free_idx];
	
	// Shrink block
	assert(b.size > 0);
	if (b.size > 1){
		p.free_idx++;
		block& b2 = p.memory[p.free_idx];
		b2.next = b.next;
		b2.size = b.size - 1;
	} else {
		p.free_idx = b.next;
	}
	
	// Construct element
	p.count++;
	T* element = new (&b.element) T();
	return element;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template<typename T> requires std::is_trivially_destructible<T>::value
bool block_allocator<T>::dealloc(T* element) noexcept {
	assert(element != nullptr);
	block* b = (block*)element;
	
	int pi;
	int bi;
	for (pi = 0 ; pi < int(pages.size()) ; pi++){
		const block* beg = pages[pi]->memory;
		const block* end = beg + pages[pi]->size;
		
		if (beg <= b && b < end){
			bi = b - beg;
			goto found;
		}
		
	}
	
	return false;
	found:
	page& p = *pages[pi];
	
	// Destroy element
	element->~T();
	b->size = 1;
	b->next = p.free_idx;
	p.free_idx = bi;
	p.count--;
	
	// Destroy page
	assert(p.count >= 0);
	if (p.count <= 0){
		pages[pi] = move(pages.back());
		pages.pop_back();
	}
	
	pageHint = (pi < pageHint) ? pi : pageHint;
	return true;
}


// ------------------------------------------------------------------------------------------ //


template wr_node* block_allocator<wr_node>::alloc();
template wr_attr* block_allocator<wr_attr>::alloc();
template bool block_allocator<wr_node>::dealloc(wr_node*);
template bool block_allocator<wr_attr>::dealloc(wr_attr*);


// ------------------------------------------------------------------------------------------ //