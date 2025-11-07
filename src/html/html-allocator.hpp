#pragma once
#include <string_view>


namespace html {
	struct Node;
	struct Attr;
}


namespace html {
// ----------------------------------- [ Functions ] ---------------------------------------- //


char* newStr(size_t len);

char* newStr(std::string_view str);

void del(char* str) noexcept;


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool assertDeallocations();


// ------------------------------------------------------------------------------------------ //
}
