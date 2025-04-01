#include "MacroCache.hpp"
#include "MacroParser.hpp"

#include "DEBUG.hpp"

using namespace std;


// ----------------------------------- [ Variables ] ---------------------------------------- //


unordered_map<filesystem::path,unique_ptr<Macro>> MacroCache::cache;


// ----------------------------------- [ Functions ] ---------------------------------------- //


Macro* MacroCache::load(const std::filesystem::path& path){
	filesystem::path cpath;
	
	try {
		cpath = filesystem::canonical(path);
	} catch (const exception& e){
		ERROR_L1("Could not open file '%s'.", path.c_str());
		return nullptr;
	}
	
	// Check if file is already cached.
	auto p = cache.find(cpath);
	if (p != cache.end()){
		unique_ptr<Macro>& pmacro = p->second;
		assert(pmacro != nullptr);
		return pmacro.get();
	}
	
	// Parse new file
	unique_ptr<Macro> m = make_unique<Macro>();
	try {
		parseMacro(cpath.c_str(), m->doc);
	} catch (const ParsingException& e){
		if (e.row > 0)
			errorf(path.c_str(), e.row, "%s", e.what());
		else
			error(path.c_str(), "%s", e.what());
		return nullptr;
	}
	
	// Cache new macro
	Macro* ret = m.get();
	cache.emplace(move(cpath), move(m));
	return ret;
}


// ------------------------------------------------------------------------------------------ //