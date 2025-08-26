#include "MacroEngine.hpp"
#include <cmath>

#include "MacroParser.hpp"
#include "Debug.hpp"

using namespace std;
using namespace pugi;


// ----------------------------------- [ Functions ] ---------------------------------------- //


shared_ptr<Macro> MacroEngine::getMacro(string_view name) const {
	auto p = macros.find(name);
	if (p != macros.end()){
		assert(p->second != nullptr);
		return p->second;
	} else {
		return nullptr;
	}
}


static void cacheMacros(MacroEngine& self, vector<unique_ptr<Macro>>&& newMacros){
	for (auto& pmacro : newMacros){
		assert(pmacro != nullptr);
		self.macros.emplace(pmacro->name, move(pmacro));
	}
	newMacros.clear();
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


shared_ptr<Macro> MacroEngine::loadFile(const filesystem::path& path){
	XHTMLFile dom = {};
	
	try {
		if (path.is_absolute())
			dom.path = filesystem::canonical(path);
		else
			dom.path = path.relative_path();
	} catch (const exception& e){
		ERROR("Could not open file '%s'.", path.c_str());
		return nullptr;
	}
	
	// Check if file is already cached.
	shared_ptr<Macro> p = getMacro(dom.path.native());
	if (p != nullptr){
		return p;
	}
	
	// Parse new file
	if (!parseFile(dom.path.c_str(), dom.doc)){
		return nullptr;
	}
	
	// Extract and cache all sub-macros
	vector<unique_ptr<Macro>> macroList = dom.convertToMacroSet();
	p = move(macroList.back());
	macroList.pop_back();
	
	cacheMacros(*this, move(macroList));
	macros.emplace(p->name, p);
	
	return p;
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


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::setVariableConstants(){
	variables["false"] = 0L;
	variables["true"] = 1L;
	variables["pi"] = M_PI;
}


// ------------------------------------------------------------------------------------------ //