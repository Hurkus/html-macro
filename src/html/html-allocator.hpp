#pragma once
#include <cassert>
#include <vector>
#include <memory>
#include <type_traits>


namespace html {
	class char_allocator;
	
	template<typename T> requires std::is_trivially_destructible<T>::value
	class block_allocator;
	
	template<typename T> requires std::is_trivially_destructible<T>::value
	class const_allocator;
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
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	char_allocator() = default;
	char_allocator(const char_allocator&) = delete;
	char_allocator(char_allocator&&) = default;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	char* alloc(size_t len) noexcept;
	char* alloc(std::string_view str) noexcept;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	bool dealloc(char* str) noexcept;
	
	bool dealloc(std::string_view str) noexcept {
		return dealloc((char*)str.data());
	}
	
// ------------------------------------------------------------------------------------------ //
};




/**
 * @brief Allocator for larger structs where allocations are fast and deallocations are slow.
 */
template<typename T> requires std::is_trivially_destructible<T>::value
class html::block_allocator {
// ----------------------------------- [ Constants ] ---------------------------------------- //
public:
	static constexpr size_t MIN_PAGE_SIZE = 32;
	static constexpr size_t MAX_PAGE_SIZE = 2048;
	static constexpr size_t PAGE_SIZE_INC = 32;
	
// ----------------------------------- [ Structures ] --------------------------------------- //
public:
	union block {
		struct {
			int16_t next;
			uint16_t size;
		};
		T element;
	};
	
	struct page {
		int count;
		int free_idx;
		int size;
		block memory[];
	};
	
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	std::vector<std::unique_ptr<page>> pages;
	int pageHint = 0;
	int pageSize = MIN_PAGE_SIZE;
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	block_allocator() = default;
	block_allocator(const block_allocator&) = delete;
	block_allocator(block_allocator&&) = default;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	T* alloc() noexcept;
	bool dealloc(T* element) noexcept;
	
// ------------------------------------------------------------------------------------------ //	
};




/**
 * @brief Allocator that only allows allocations.
 */
template<typename T> requires std::is_trivially_destructible<T>::value
class html::const_allocator {
// ----------------------------------- [ Constants ] ---------------------------------------- //
public:
	static constexpr size_t MIN_PAGE_SIZE = 32;
	static constexpr size_t MAX_PAGE_SIZE = 2048;
	static constexpr size_t PAGE_SIZE_INC = 32;
	
// ----------------------------------- [ Structures ] --------------------------------------- //
public:
	struct page {
		page* next;	// Linked list.
		int size;
		int count;
		T memory[];
	};
	
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	page* rootPage = nullptr;
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	const_allocator() = default;
	const_allocator(const const_allocator&) = delete;
	const_allocator(const_allocator&& o) = delete;
	
	~const_allocator(){
		clear();
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	T* alloc() noexcept;
	
	void clear(){
		page* p = rootPage;
		
		while (p != nullptr){
			page* _p = p->next;
			delete p;
			p = _p;
		}
		
		rootPage = nullptr;
	}
	
// ------------------------------------------------------------------------------------------ //	
};
