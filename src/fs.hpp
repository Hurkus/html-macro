#pragma once
#include <filesystem>

using filepath = std::filesystem::path;


namespace fs {
// ----------------------------------- [ Functions ] ---------------------------------------- //


inline filepath cwd() noexcept {
	try {
		return std::filesystem::current_path();
	} catch (...) {
		return filepath("./");
	}
}


inline bool cwd(const filepath& newCWD) noexcept {
	std::error_code err;
	std::filesystem::current_path(newCWD, err);
	return err == std::error_code();
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline bool exists(const filepath& path) noexcept {
	std::error_code ec;
	return std::filesystem::exists(path, ec);
}


inline bool is_dir(const filepath& path) noexcept {
	std::error_code err;
	return std::filesystem::is_directory(path, err);
}


inline bool is_file(const filepath& path) noexcept {
	std::error_code err;
	return std::filesystem::is_regular_file(path, err);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool readStream(std::istream& in, std::string& buff);
bool readFile(const filepath& path, std::string& buff);
std::string readFile(const filepath& path);


// ------------------------------------------------------------------------------------------ //
}
