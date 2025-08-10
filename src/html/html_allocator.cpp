#include "html_allocator.hpp"
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


// ------------------------------------------------------------------------------------------ //