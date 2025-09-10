#include "Paths.hpp"
#include <cassert>

using namespace std;
using namespace MacroEngine;


// ----------------------------------- [ Variables ] ---------------------------------------- //


shared_ptr<const filepath> MacroEngine::cwd = make_unique<filepath>(filesystem::current_path());
vector<filepath> MacroEngine::paths;


// ----------------------------------- [ Functions ] ---------------------------------------- //


filepath MacroEngine::resolve(const filepath& path){
	try {
		if (path.is_absolute()){
			return filesystem::canonical(path);
		}
		
		// Check cwd
		assert(cwd != nullptr);
		filepath p1 = *cwd / path;
		if (filesystem::exists(p1)){
			return filesystem::relative(p1);
		}
		
		// Check include paths
		filepath p2;
		for (const filepath& sp : paths){
			p2 = sp / path;
			if (filesystem::exists(p2))
				return filesystem::relative(p2);
		}
		
		return filesystem::proximate(p1);
	} catch (...){}
	return {};
}


// ------------------------------------------------------------------------------------------ //