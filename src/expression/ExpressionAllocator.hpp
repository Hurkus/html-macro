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


struct Expression::Allocator {
// ----------------------------------- [ Constants ] ---------------------------------------- //
public:
	static constexpr size_t UNIT = maxAlign<Operation,Long,Double,String,Variable,UnaryOperation,BinaryOperation,Function,Operation*>();
	static constexpr size_t AVGSIZE = sizeof(BinaryOperation) / UNIT;	// Expected avg size of each allocation
	static constexpr size_t PAGE_SIZE = AVGSIZE * 32;					// Size in units
	static constexpr size_t MAX_SIZE = PAGE_SIZE * UNIT;

// ----------------------------------- [ Structures ] --------------------------------------- //
public:
	struct Page {
		Page* next;
		size_t count;
		alignas(UNIT) uint8_t memory[PAGE_SIZE][UNIT];
	};

// ----------------------------------- [ Variables ] ---------------------------------------- //
public:
	Page* page = nullptr;

// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	Allocator() = default;
	Allocator(const Allocator&) = delete;
	Allocator(Allocator&&) = delete;
	
	~Allocator();
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	void* alloc(size_t size);
	
	template<typename T>
	T* alloc(){
		return static_cast<T*>(alloc(sizeof(T)));
	}

// ------------------------------------------------------------------------------------------ //
};
