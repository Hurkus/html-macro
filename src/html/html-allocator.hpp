#pragma once
#include <cstring>
#include <string_view>


namespace html {
	struct node;
	struct attr;
}


namespace html {
// ----------------------------------- [ Functions ] ---------------------------------------- //


/**
 * @brief Allocate new node using a custom allocator.
 * @throws `bad_alloc` on failed memory allocation.
 * @return New instance of `html::node`.
 */
html::node* newNode();

/**
 * @brief Allocate new attribute using a custom allocator.
 * @throws `bad_alloc` on failed memory allocation.
 * @return New instance of `html::attr`.
 */
html::attr* newAttr();


/**
 * @brief Delete node object obtained by `html::newNode()`.
 * @param p Pointer to `html::node` previously allocated with `html::newNode()`.
 * @throws `bad_alloc` if `p` does not belong to this allocator.
 */
void del(html::node* p);

/**
 * @brief Delete attribute object obtained by `html::newAttr()`.
 * @param p Pointer to `html::attr` previously allocated with `html::newAttr()`.
 * @throws `bad_alloc` if `p` does not belong to this allocator.
 */
void del(html::attr* p);


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline char* newStr(size_t len){
	return new char[len];
}

inline char* newStr(std::string_view str){
	char* s = newStr(str.length() + 1);
	memcpy(s, str.begin(), str.length());
	return s;
}


inline void del(char* str){
	delete[] str;
}


// ------------------------------------------------------------------------------------------ //
}
