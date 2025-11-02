#pragma once
#include <filesystem>

using filepath = std::filesystem::path;


namespace fs {
// ----------------------------------- [ Functions ] ---------------------------------------- //


inline filepath current_path() noexcept {
	try {
		return std::filesystem::current_path();
	} catch (...) {
		return filepath(".");
	}
}


inline bool exists(const filepath& path) noexcept {
	try {
		return std::filesystem::exists(path);
	} catch (...) {
		return false;
	}
}


// ------------------------------------------------------------------------------------------ //
}