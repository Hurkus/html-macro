#pragma once
#include <cassert>
#include <vector>
#include <memory>
#include <type_traits>


namespace html {
	class char_allocator;
	
	template<typename T>
	class block_allocator;
}



/**
 * @brief Allocator for strings where allocations are fast and deallocations are slow.
 */
class html::char_allocator {
// ----------------------------------- [ Constants ] ---------------------------------------- //
public:
	static constexpr size_t PAGE_MIN_SIZE = 128;
	static constexpr size_t PAGE_MAX_SIZE = UINT16_MAX;	// 64 kB
	static constexpr size_t MAX_LEN = UINT16_MAX - sizeof(uint16_t);
	
// ----------------------------------- [ Structures ] --------------------------------------- //
public:
	struct page {
		uint16_t space;
		uint16_t size;
		uint16_t deleted;
		char memory[];
	};
	
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	std::vector<std::unique_ptr<page>> pages;
	size_t pageSize = PAGE_MIN_SIZE;			// Previous page size.
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	char* alloc(uint16_t len) noexcept;
	std::string_view alloc(std::string_view str) noexcept;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	bool dealloc(char* str) noexcept;
	
	bool dealloc(std::string_view str) noexcept {
		return dealloc((char*)str.data());
	}
	
// ------------------------------------------------------------------------------------------ //
};

