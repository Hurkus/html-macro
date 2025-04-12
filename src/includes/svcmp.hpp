#pragma once
#include <cassert>
#include <string_view>
#include <cstring>


// ----------------------------------- [ Functions ] ---------------------------------------- //


/**
 * @brief Compare string view with a cstring using `strncmp`.
 * @return `true` when strings are equal, otherwise `false`.
 */
constexpr bool operator==(const std::string_view& sv, const char* cstr){
	assert(cstr != nullptr);
	return std::strncmp(sv.data(), cstr, sv.length()) == 0;
}


/**
 * @brief Compare string view with a cstring using `strncmp`.
 * @return `true` when strings are equal, otherwise `false`.
 */
constexpr bool operator==(const char* cstr, const std::string_view& sv){
	assert(cstr != nullptr);
	return std::strncmp(sv.data(), cstr, sv.length()) == 0;
}


// ------------------------------------------------------------------------------------------ //