#include "html-allocator.hpp"
#include <cassert>
#include <cstring>

#include "html.hpp"
#include "BlockAllocator.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;


// ----------------------------------- [ Structures ] --------------------------------------- //


template<size_t S, size_t A>
BlockAllocator<S,A> BlockAllocator<S,A>::heap;


// ----------------------------------- [ Variables ] ---------------------------------------- //


#ifdef DEBUG
template<typename T>
static long allocCount = 0;

template<typename T>
static long deallocCount = 0;
#endif


#ifdef DEBUG
	#define INC(var)	(var++)
#else
	#define INC(var)	{}
#endif


// ----------------------------------- [ Functions ] ---------------------------------------- //


template<typename T>
inline void* _alloc(){
	void* p = BlockAllocator<sizeof(T),alignof(T)>::heap.allocate();
	if (p == nullptr){
		throw bad_alloc();
	}
	
	INC(allocCount<T>);
	return p;
}


template<typename T>
inline void _dealloc(void* p) noexcept {
	assert(p != nullptr);
	BlockAllocator<sizeof(T),alignof(T)>::heap.deallocate(p);
	INC(deallocCount<T>);
}


// ----------------------------------- [ Operators ] ---------------------------------------- //


void* html::Node::operator new(size_t size){
	assert(size == sizeof(Node));
	return _alloc<Node>();
}

void html::Node::operator delete(void* p) noexcept {
	_dealloc<Node>(p);
}


// ----------------------------------- [ Operators ] ---------------------------------------- //


void* html::Attr::operator new(size_t size){
	assert(size == sizeof(Attr));
	return _alloc<Attr>();
}

void html::Attr::operator delete(void* p) noexcept {
	_dealloc<Attr>(p);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void* html::Document::operator new(size_t size){
	assert(size == sizeof(Document));
	INC(allocCount<Document>);
	return (Document*)::operator new(sizeof(Document), align_val_t(alignof(Document)));
}

void html::Document::operator delete(void* p) noexcept {
	INC(deallocCount<Document>);
	::operator delete(p);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


char* html::newStr(size_t len){
	INC(allocCount<char[]>);
	return new char[len];
}

char* html::newStr(std::string_view str){
	char* s = newStr(str.length() + 1);
	memcpy(s, str.begin(), str.length());
	s[str.length()] = 0;
	return s;
}

void html::del(char* str) noexcept {
	if (str != nullptr)
		INC(deallocCount<char[]>);
	delete[] str;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool html::assertDeallocations(){
	bool pass = true;
	
	#ifdef DEBUG
	pass &= (allocCount<Document> == deallocCount<Document>);
	pass &= (allocCount<Node> == deallocCount<Node>);
	pass &= (allocCount<Attr> == deallocCount<Attr>);
	pass &= (allocCount<char[]> == deallocCount<char[]>);
	
	if (!pass){
		string msg = "HTML allocator encountered missmatching deallocation count:\n";
		msg += "  html::Document: " + to_string(deallocCount<Document>) + "/" + to_string(allocCount<Document>) + "\n";
		msg += "  html::Node: " + to_string(deallocCount<Node>) + "/" + to_string(allocCount<Node>) + "\n";
		msg += "  html::Attr: " + to_string(allocCount<Attr>) + "/" + to_string(deallocCount<Attr>) + "\n";
		msg += "  html::Str:  " + to_string(allocCount<char[]>) + "/" + to_string(deallocCount<char[]>);
		ERROR("%s", msg.c_str());
	}
	#endif
	
	return pass;
}


// ------------------------------------------------------------------------------------------ //