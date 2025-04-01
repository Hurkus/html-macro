#pragma once
#include <cstdio>
#include <string>


template <typename ...ARG>
void format(std::string& dst, const char* fmt, ARG&&... args) noexcept {
	dst.resize(dst.capacity());
	int n = std::snprintf(dst.data(), dst.length() + 1, fmt, args...);
	
	if (n > int(dst.length())){
		dst.resize(n);
		std::snprintf(dst.data(), dst.length() + 1, fmt, args...);
	} else {
		dst.resize(n);
	}
	
}


template <typename ...ARG>
std::string format(const char* fmt, ARG&&... args) noexcept {
	std::string s = {};
	format(s, fmt, args...);
	return s;
}
