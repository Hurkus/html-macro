#include "fs.hpp"
#include <cassert>
#include <string>
#include <fstream>

using namespace std;


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool fs::readFile(const filepath& path, string& buff){
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
	
	assert(pos >= 0);
	const size_t size = size_t(pos);
	buff.resize(size);
	size_t count = 0;
	
	while (count < size && in.read(&buff[count], size - count)){
		const streamsize n = in.gcount();
		
		if (n < 0){
			return false;
		} else if (n == 0){
			break;
		}
		
		assert(n >= 0);
		count += size_t(n);
	}
	
	buff.resize(count);
	return true;
}


// ------------------------------------------------------------------------------------------ //