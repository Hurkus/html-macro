#pragma once
#include <cassert>
#include <vector>


class Buffer {
// ------------------------------------------------------------------------------------------ //
public:
	size_t size = 0;
	char* data = nullptr;
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	Buffer() = default;
	Buffer(const Buffer&) = delete;
	
	Buffer(Buffer&& o) : size{o.size}, data{o.data} {
		o.data = nullptr;
	}
	
	~Buffer(){
		delete[] data;
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	void alloc(size_t size){
		delete[] data;
		data = new char[size];
	}
	
// ----------------------------------- [ Operators ] ---------------------------------------- //
public:
	char& operator[](auto i){
		assert(0 <= i && size_t(i) < size);
		return data[i];
	}
	
	const char& operator[](auto i) const {
		assert(0 <= i && size_t(i) < size);
		return data[i];
	}
	
// ------------------------------------------------------------------------------------------ //
};

bool readFile(const char* path, Buffer& out_buff);
