#include "Paths.hpp"
#include "fs.hpp"
#include <cassert>

using namespace std;


// ----------------------------------- [ Variables ] ---------------------------------------- //


shared_ptr<const filepath> Paths::cwd = make_unique<filepath>(".");
vector<filepath> Paths::includeDirs;


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool Paths::resolve(filepath& path, const filepath& cwd) noexcept {
	try {
		if (path.is_absolute()){
			path = filesystem::canonical(path);
			return true;
		}
		
		// Check cwd
		filepath p1 = cwd / path;
		if (fs::exists(p1)){
			path = filesystem::relative(p1);
			return true;
		}
		
		// Check include paths
		filepath p2;
		for (const filepath& sp : includeDirs){
			p2 = sp / path;
			
			if (fs::exists(p2)){
				path = filesystem::relative(p2);
				return true;
			}
			
		}
		
		path = filesystem::proximate(p1);
		return true;
	} catch (...){}
	
	return false;
}


// ------------------------------------------------------------------------------------------ //