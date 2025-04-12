#include "MacroEngine.hpp"
#include "MacroParser.hpp"

#include "DEBUG.hpp"

using namespace std;
using namespace pugi;


// ----------------------------------- [ Functions ] ---------------------------------------- //


Macro* MacroEngine::getMacro(string_view name) const {
	auto p = macros.find(name);
	if (p != macros.end())
		return p->second.get();
	else
		return nullptr;
}


static void cacheMacros(MacroEngine& self, vector<unique_ptr<Macro>>&& newMacros){
	for (auto& pmacro : newMacros){
		assert(pmacro != nullptr);
		self.macros.emplace(pmacro->name, move(pmacro));
	}
	newMacros.clear();
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
	if (!parseFile(dom.path.c_str(), dom.doc)){
		return nullptr;
	}
	
	// Extract and cache all sub-macros
	vector<unique_ptr<Macro>> macroList = dom.convertToMacroSet();
	Macro* ret = macroList.back().get();
	cacheMacros(*this, move(macroList));
	
	return ret;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::execBuff(string_view buff, xml_node dst){
	xml_document doc;
	if (!parseBuffer(buff, doc)){
		return false;
	}
	
	vector<unique_ptr<Macro>> macroList = XHTMLFile::extractMacros(move(doc));
	unique_ptr<Macro> main = move(macroList.back());
	macroList.pop_back();
	
	cacheMacros(*this, move(macroList));
	
	assert(main != nullptr);
	exec(*main, dst);
	return true;
}


// ------------------------------------------------------------------------------------------ //