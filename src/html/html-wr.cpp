#include "html-wr.hpp"
#include <cassert>

using namespace std;
using namespace html;


// ----------------------------------- [ Constants ] ---------------------------------------- //


template<typename T>
constexpr int PAGE_SIZE = 64;
template<>
constexpr int PAGE_SIZE<char> = 4096;


// ----------------------------------- [ Structures ] --------------------------------------- //


template<typename T>
union segment {
	struct {
		int size;
		int next;
	};
	T element;
};


template<typename T>
struct wr_document::allocator {
	unique_ptr<segment<T>> memory;
	unique_ptr<allocator<T>> prev;	// Linked list of allocations.
	allocator<T>* next;				// Linked list of allocations.
	int free_i;						// Index of first unallocated segment.
};

// template<>
// struct wr_document::allocator<char> {
// 	unique_ptr<char[]> memory;
// 	unique_ptr<allocator<char>> prev;
// 	allocator<char>* next;
// 	int free_i;
// };


// ---------------------------------- [ Constructors ] -------------------------------------- //


wr_document::~wr_document(){
	// delete nodeAlloc;
	// delete attrAlloc;
	// delete strAlloc;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template<typename T>
static wr_document::allocator<T>* newPage(){
	try {
		wr_document::allocator<T>* page = new wr_document::allocator<T>();
		page->next = nullptr;
		page->prev = nullptr;
		
		constexpr size_t size = sizeof(T) * PAGE_SIZE<T>;
		constexpr size_t alignment = alignof(segment<T>);
		void* mem = ::operator new(size, align_val_t(alignment));

		page->free_i = 0;
		page->memory = unique_ptr<segment<T>>((segment<T>*)mem);
		page->memory.get()[0].size = PAGE_SIZE<T>;
		page->memory.get()[0].next = -1;
		
		return page;
	}
	catch (...){}
	
	return nullptr;
}


// template<>
// wr_document::allocator<char>* newPage(){
// 	try {
// 		wr_document::allocator<char>* page = new wr_document::allocator<char>();
// 		page->next = nullptr;
// 		page->prev = nullptr;
		
// 		constexpr size_t size = sizeof(char) * PAGE_SIZE<char>;
// 		constexpr size_t alignment = alignof(segment<char>);
// 		void* mem = ::operator new[](size, align_val_t(alignment));
		
// 		// page->free_i = 0;
// 		// page->memory = unique_ptr<segment<char>>((segment<char>*)mem);
// 		// page->memory.get()[0].size = PAGE_SIZE<T>;
// 		// page->memory.get()[0].next = -1;
		
// 		return page;
// 	}
// 	catch (...){}
	
// 	return nullptr;
// }


template<typename T>
static T* allocate(wr_document::allocator<T>*& alloc){
	// Initial allocation
	if (alloc == nullptr){
		alloc = newPage<T>();
		if (alloc == nullptr)
			return nullptr;
	}
	
	// Find next free
	while (alloc->free_i < 0){
		
		// Out of pages
		if (alloc->next == nullptr){
			wr_document::allocator<T>* page = newPage<T>();
			if (page == nullptr){
				return nullptr;
			}
			
			alloc->next = page;
			page->prev.reset(alloc);
			alloc = page;
			break;
		}
		
		alloc = alloc->next;
	}
	
	// Construct new element.
	segment<T>* segv = alloc->memory.get();
	
	// Shrink segment
	if (segv[alloc->free_i].size >= 2){
		const int i = alloc->free_i++;
		segv[alloc->free_i].size = segv[i].size - 1;
		segv[alloc->free_i].next = segv[i].next;
		return new (&segv[i].element) T();
	}
	
	// Consume segment
	else {
		const int i = alloc->free_i;
		alloc->free_i = segv[i].next;
		return new (&segv[i].element) T();
	}
	
	return nullptr;
}


wr_node* wr_document::allocNode(){
	// wr_node* node = allocate<wr_node>(nodeAlloc);
	// if (node != nullptr)
		// node->doc = this;
	// return node;
	return nullptr;
}


wr_attr* wr_document::allocAttr(){
	// return allocate<wr_attr>(attrAlloc);
	return nullptr;
}


// char* wr_document::allocStr(size_t n){
// 	wr_document::allocator<char>*& alloc = strAlloc;
// 	n = max(1,2);
	
// 	// Initial allocation
// 	if (alloc == nullptr){
// 		alloc = newPage<char>();
// 		if (alloc == nullptr)
// 			return nullptr;
// 	}
	
// 	// Find next free
// 	while (alloc->free_i < 0){
		
// 		// Out of pages
// 		if (alloc->next == nullptr){
// 			wr_document::allocator<char>* page = newPage<char>();
// 			if (page == nullptr){
// 				return nullptr;
// 			}
			
// 			alloc->next = page;
// 			page->prev.reset(alloc);
// 			alloc = page;
// 			break;
// 		}
		
// 		alloc = alloc->next;
// 	}
	
	
	
// }


// ----------------------------------- [ Functions ] ---------------------------------------- //


// wr_node* wr_node::appendChild(string_view value, node_type type){
// 	// assert(doc != nullptr);
	
// 	// wr_node* node = doc->allocNode();
// 	// if (node == nullptr){
// 	// 	return nullptr;
// 	}
	
// 	// node->type = type;
// 	// node->parent = this;
	
// 	// // Amend linked list
// 	// if (this->child == nullptr){
// 	// 	this->child = node;
// 	// 	this->child->prev = node;
// 	// } else {
// 	// 	assert(this->child->prev != nullptr);
// 	// 	node->prev = this->child->prev;
// 	// 	node->prev->next = node;
// 	// 	this->child->prev = node;
// 	// }
	
// 	// node->value_p = value.data();
// 	// node->value_len = value.length();
// 	return node;
// }


// wr_attr* appendAttr(std::string_view name = "", std::string_view value = "");


// ------------------------------------------------------------------------------------------ //