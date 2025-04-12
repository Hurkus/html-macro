#pragma once
#include <cstddef>
#include <cassert>


struct optbool {
// ------------------------------------[ Properties ] --------------------------------------- //
private:
	static constexpr char NONE = -1;
	static_assert(char(false) != char(true));
	static_assert(NONE != char(false) && NONE != char(true));
	
	union {
		bool bval;
		char cval;
	};
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	constexpr optbool() : cval{NONE} {}
	constexpr optbool(nullptr_t) : cval{NONE} {}
	constexpr optbool(bool val) : bval{val} {}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	constexpr bool empty() const {
		return cval == NONE;
	}
	
	constexpr bool hasValue() const {
		return !empty();
	}
	
	constexpr void reset(){
		this->cval = NONE;
	}
	
public:
	constexpr bool get() const {
		assert(hasValue());
		return bval;
	}
	
	constexpr bool get_or(bool b) const {
		if (cval == NONE)
			return b;
		else
			return bval;
	}
	
	constexpr void set(nullptr_t){
		this->cval = NONE;
	}
	
	constexpr void set(bool b){
		this->bval = b;
	}
	
// ----------------------------------- [ Operators ] ---------------------------------------- //
public:
	constexpr optbool& operator=(nullptr_t){
		cval = NONE;
		return *this;
	}
	
	constexpr optbool& operator=(bool b){
		bval = b;
		return *this;
	}
	
public:
	constexpr bool& operator*(){
		assert(hasValue());
		return bval;
	}
	
	constexpr const bool& operator*() const {
		assert(hasValue());
		return bval;
	}
	
public:
	constexpr bool operator==(nullptr_t) const {
		return cval == NONE;
	}
	
	constexpr bool operator!=(nullptr_t) const {
		return cval != NONE;
	}
	
	constexpr bool operator==(bool b) const {
		return cval == char(b);
	}
	
	constexpr bool operator!=(bool b) const {
		return cval != char(b);
	}
	
// ------------------------------------------------------------------------------------------ //
};