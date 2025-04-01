#include "MacroCache.hpp"
#include "MacroParser.hpp"

#include "DEBUG.hpp"

using namespace std;


// ----------------------------------- [ Variables ] ---------------------------------------- //


string_map<unique_ptr<Macro>> MacroCache::cache;


// ----------------------------------- [ Functions ] ---------------------------------------- //


Macro* MacroCache::getMacro(string_view name){
	auto p = cache.find(name);
	if (p != cache.end())
		return p->second.get();
	else
		return nullptr;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Macro* MacroCache::loadFile(const std::filesystem::path& path){
	XHTMLFile dom = {};
	
	try {
		if (path.is_absolute())
			dom.path = filesystem::canonical(path);
		else
			dom.path = path.relative_path();
	} catch (const exception& e){
		ERROR_L1("Could not open file '%s'.", path.c_str());
		return nullptr;
	}
	
	// Check if file is already cached.
	auto p = cache.find(dom.path);
	if (p != cache.end()){
		unique_ptr<Macro>& pmacro = p->second;
		assert(pmacro != nullptr);
		return pmacro.get();
	}
	
	// Parse new file
	try {
		parseMacro(dom.path.c_str(), dom.doc);
	} catch (const ParsingException& e){
		if (e.row > 0)
			errorf(path.c_str(), e.row, "%s", e.what());
		else
			error(path.c_str(), "%s", e.what());
		return nullptr;
	}
	
	// Extract and cache all sub-macros
	vector<std::unique_ptr<Macro>> macros = dom.convertToMacroSet();
	
	Macro* ret = nullptr;
	for (auto& uptr : macros){
		assert(uptr != nullptr);
		ret = uptr.get();
		cache.emplace(uptr->name, move(uptr));
	}
	
	return ret;
}


// ------------------------------------------------------------------------------------------ //