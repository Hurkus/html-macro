#pragma once
#include <string_view>


namespace html {
	struct Node;
	struct Attr;
}


namespace html {
// ----------------------------------- [ Functions ] ---------------------------------------- //


/**
 * @brief Allocate new node using a custom allocator.
 * @throws `bad_alloc` on failed memory allocation.
 * @return New instance of `html::node`.
 */
html::Node* newNode();

/**
 * @brief Allocate new attribute using a custom allocator.
 * @throws `bad_alloc` on failed memory allocation.
 * @return New instance of `html::attr`.
 */
html::Attr* newAttr();


/**
 * @brief Delete node object obtained by `html::newNode()`.
 * @param p Pointer to `html::node` previously allocated with `html::newNode()`.
 */
void del(html::Node* p);

/**
 * @brief Delete attribute object obtained by `html::newAttr()`.
 * @param p Pointer to `html::attr` previously allocated with `html::newAttr()`.
 */
void del(html::Attr* p);


// ----------------------------------- [ Functions ] ---------------------------------------- //


char* newStr(size_t len);

char* newStr(std::string_view str);

void del(char* str);


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool assertDeallocations();


// ------------------------------------------------------------------------------------------ //
}
