#pragma once
#include <memory>
#include <vector>
#include <filesystem>

using filepath = std::filesystem::path;


namespace Paths {
	extern std::shared_ptr<const filepath> cwd;
	extern std::vector<filepath> includeDirs;
	
	bool resolve(filepath& path) noexcept;
};
