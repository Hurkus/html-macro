#include "MacroEngine.hpp"
#include "MacroParser.hpp"

#include "DEBUG.hpp"

using namespace std;


// ----------------------------------- [ Functions ] ---------------------------------------- //


Macro* MacroEngine::getMacro(string_view name) const {
	auto p = macros.find(name);
	if (p != macros.end())
		return p->second.get();
	else
		return nullptr;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Macro* MacroEngine::loadFile(const filesystem::path& path){
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
	auto p = macros.find(dom.path);
	if (p != macros.end()){
		unique_ptr<Macro>& pmacro = p->second;
		assert(pmacro != nullptr);
		return pmacro.get();
	}
	
	// Parse new file
	try {
		parseMacro(dom.path.c_str(), dom.doc);
	} catch (const ParsingException& e){
		if (e.row > 0)
			::errorf(path.c_str(), e.row, "%s", e.what());
		else
			::error(path.c_str(), "%s", e.what());
		return nullptr;
	}
	
	// Extract and cache all sub-macros
	vector<std::unique_ptr<Macro>> macroList = dom.convertToMacroSet();
	
	Macro* ret = nullptr;
	for (auto& uptr : macroList){
		assert(uptr != nullptr);
		ret = uptr.get();
		macros.emplace(uptr->name, move(uptr));
	}
	
	return ret;
}


// ------------------------------------------------------------------------------------------ //