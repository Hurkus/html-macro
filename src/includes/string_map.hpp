#pragma once
#include <string>
#include <string_view>
#include <unordered_map>


struct string_hash {
	using is_transparent = std::true_type;
	
	size_t operator()(const std::string& o) const {
		return std::hash<std::string>{}(o);
	}
	
	size_t operator()(const std::string_view& o) const {
		return std::hash<std::string_view>{}(o);
	}
	
	size_t operator()(const char* o) const {
		return std::hash<std::string_view>{}(o);
	}
	
};


template <typename T>
using string_map = std::unordered_map<std::string,T,string_hash,std::equal_to<>>;
