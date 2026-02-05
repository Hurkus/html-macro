#include <new>


namespace html {
	template<typename T>
	class Allocator;
}


template<typename T>
class html::Allocator {
// ----------------------------------- [ Constants ] ---------------------------------------- //
public:
	static constexpr size_t MAX_PAGE = 1 + (1024*1024)/sizeof(T);
	static constexpr size_t MIN_PAGE = (64 + sizeof(T) - 1)/sizeof(T);
	
// ------------------------------------[ Properties ] --------------------------------------- //
private:
	struct Page {
		Page* next = nullptr;
		size_t capacity = 0;
		size_t size = 0;
		T mem[];
	};
	
	union EmptyList {
		EmptyList* next;
		T obj;
	};
	static_assert(sizeof(T) == sizeof(EmptyList));
	
private:
	Page* pages = nullptr;
	EmptyList* empty = nullptr;
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	Allocator() = default;
	
	Allocator(Allocator&& o){
		std::swap(pages, o.pages);
		std::swap(empty, o.empty);
	}
	
	void operator=(Allocator&& o){
		std::swap(pages, o.pages);
		std::swap(empty, o.empty);
	}
	
	~Allocator(){
		Page* p = pages;
		while (p != nullptr){
			Page* next = p->next;
			delete p;
			p = next;
		}
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	template<typename ...U>
	T* create(U&&... args){
		void* mem = alloc();
		return new (mem) T(std::forward<U>(args)...);
	}
	
	void destroy(T* obj){
		obj->~T();
		dealloc(obj);
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	void* alloc(){
		if (empty != nullptr){
			T* mem = &empty->obj;
			empty = empty->next;
			return mem;
		}
		
		else if (pages == nullptr || pages->size >= pages->capacity){
			const size_t cap = (pages != nullptr) ? pages->capacity*2 : MIN_PAGE;
			const size_t mem = (cap < MAX_PAGE ? cap : MAX_PAGE) * sizeof(T);
			
			Page* page = reinterpret_cast<Page*>(operator new(sizeof(Page) + mem, std::align_val_t(alignof(Page))));
			page->capacity = cap;
			page->size = 0;
			page->next = pages;
			pages = page;
		}
		
		return &pages->mem[pages->size++];
	}
	
public:
	void dealloc(void* p) noexcept {
		if (p != nullptr){
			EmptyList* e = reinterpret_cast<EmptyList*>(p);
			e->next = empty;
			empty = e;
		}
	}
	
// ------------------------------------------------------------------------------------------ //
};
