#pragma once
#include <cassert>
#include <memory>
#include <vector>
#include "fs.hpp"


namespace Paths {
// ------------------------------------[ Properties ] --------------------------------------- //


extern std::shared_ptr<const filepath> cwd;
extern std::vector<filepath> includeDirs;


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool resolve(filepath& path, const filepath& cwd) noexcept;

inline bool resolve(filepath& path) noexcept {
	assert(cwd != nullptr);
	return resolve(path, *cwd);
}


// ------------------------------------------------------------------------------------------ //
};
