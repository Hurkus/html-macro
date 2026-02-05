#pragma once
#include <cassert>
#include <cstdint>
#include <string_view>
#include <cstring>
#include <vector>


namespace html {
	struct CharAllocator;
}


class html::CharAllocator {
// ------------------------------------[ Properties ] --------------------------------------- //
private:
	std::vector<char*> allocations;
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	~CharAllocator(){
		for (char* s : allocations)
			delete[] s;
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	char* create(std::string_view init){
		char* str = alloc(init.length() + 1);
		memcpy(str, init.data(), init.length());
		str[init.length()] = 0;
		return str;
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	char* alloc(size_t len){
		char* str = new char[len];
		allocations.push_back(str);
		return str;
	}
	
	void dealloc(char* p) noexcept {
		if (p == nullptr){
			return;
		}
		
		for (size_t i = 0 ; i < allocations.size() ; i++){
			if (allocations[i] == p){
				allocations[i] = allocations.back();
				allocations.pop_back();
				delete[] p;
				return;
			}
		}
		
		assert(false);
	}
	
// ------------------------------------------------------------------------------------------ //
};