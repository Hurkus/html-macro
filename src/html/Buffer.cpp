#include "Buffer.hpp"
#include <fstream>

using namespace std;


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool readFile(const char* path, Buffer& buff){
	ifstream in = ifstream(path);
	if (!in){
		return false;
	}
	
	if (!in.seekg(0, ios_base::end)){
		return false;
	}
	
	const streampos pos = in.tellg();
	
	if (pos < 0 || !in.seekg(0, ios_base::beg)){
		return false;
	}
	
	const size_t size = size_t(pos);
	size_t total = 0;
	buff.alloc(size + 1);
	
	while (total < size && in.read(buff.data, size - total)){
		
		const streamsize n = in.gcount();
		if (n <= 0){
			break;
		}
		
		total += size_t(n);
	}
	
	if (total <= size){
		buff.size = total;
	}
	
	buff.data[total] = 0;
	return true;
}


// ------------------------------------------------------------------------------------------ //