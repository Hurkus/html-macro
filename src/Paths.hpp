#pragma once
#include <cassert>
#include <memory>
#include <vector>
#include <filesystem>

using filepath = std::filesystem::path;


namespace Paths {
	extern std::shared_ptr<const filepath> cwd;
	extern std::vector<filepath> includeDirs;
	
	bool resolve(filepath& path, const filepath& cwd) noexcept;
	
	inline bool resolve(filepath& path) noexcept {
		assert(cwd != nullptr);
		return resolve(path, *cwd);
	}
	
};
