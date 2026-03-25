#pragma once
#include <cstdint>
#include "ExpressionOperation.hpp"


template<typename A, typename ...T>
consteval size_t maxAlign(){
	if constexpr (sizeof...(T) <= 0){
		return alignof(A);
	} else {
		size_t t = maxAlign<T...>();
		return (alignof(A) > t) ? alignof(A) : t;
	}
}


/**
 * @brief Arena allocator for all expression operation structs.
 */
struct Expression::Allocator {
// ----------------------------------- [ Constants ] ---------------------------------------- //
public:
	static constexpr size_t UNIT = maxAlign<
		Operation,Variable,Long,Double,String,Object,Object::Entry,
		UnaryOperation,BinaryOperation,
		Index,Function
	>();
	static constexpr size_t MIN_CAPACITY = 128 * sizeof(std::byte);

// ----------------------------------- [ Structures ] --------------------------------------- //
public:
	struct Page {
		Page* next = nullptr;
		size_t capacity = 0;
		size_t size = 0;
		alignas(UNIT)
		std::byte memory[];
	};

// ----------------------------------- [ Variables ] ---------------------------------------- //
public:
	Page* page = nullptr;

// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	Allocator() = default;
	Allocator(Allocator&) = delete;
	Allocator(Allocator&&) = delete;
	
	~Allocator();
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	void* alloc(size_t size);
	
	template<typename T>
	T* alloc(){
		void* mem = alloc(sizeof(T));
		return new (mem) T();
	}

// ------------------------------------------------------------------------------------------ //
};
