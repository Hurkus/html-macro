#include "MacroEngine.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Functions ] ---------------------------------------- //


// TODO: multiple NAME
void MacroEngine::call(const Node& op, Node& dst){
	string_view macroName;
	
	Attr* attr = op.attribute;
	while (attr != nullptr){
		string_view name = attr->name();
		
		if (name == "NAME"){
			macroName = attr->value();
		}
		// else if (name == "IF"){
		// 	if (!_attr_if(*this, op, attr))
		// 		return;
		// }
		else {
			warn_ignored_attribute(op, *attr);
		}
		
		attr = attr->next;
	}
	
	if (macroName.empty()){
		warn_missing_attr(op, "NAME");
		return;
	}
	
	const Macro* macro = Macro::get(macroName);
	if (macro == nullptr){
		warn_macro_not_found(op, *attr);
		return;
	}
	
	exec(*macro, dst);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


// bool MacroEngineObject::include(const xml_node op, xml_node dst){
// 	assert(op.root() != dst.root());
// 	xml_attribute src_attr;
	
// 	for (const xml_attribute attr : op.attributes()){
// 		string_view name = attr.name();
		
// 		if (name == "SRC"){
// 			src_attr = attr;
// 		} else if (name == "IF"){
// 			if (!_attr_if(*this, op, attr))
// 				return false;
// 		} else {
// 			_attr_ignore(op, attr);
// 		}
		
// 	}
	
// 	if (src_attr.empty()){
// 		_attr_missing(op, "SRC");
// 		return false;
// 	}
	
// 	// Modify relative path to current macro file path.
// 	filesystem::path path;
// 	try {
// 		path = src_attr.value();
		
// 		if (!path.is_absolute()){
// 			assert(currentMacro != nullptr);
// 			if (currentMacro == nullptr || currentMacro->srcFile == nullptr){
// 				ERROR("%s: Missing macro relative path.", op.name());
// 				return false;
// 			}
			
// 			filesystem::path dir = *currentMacro->srcFile;
// 			dir.remove_filename();
// 			path = dir / filesystem::proximate(move(path), dir);
// 		}
		
// 	} catch (const exception& e){
// 		ERROR("%s: Failed to construct path '%s'.", op.name(), src_attr.value());
// 		return false;
// 	}
	
// 	// Fetch and execute macro
// 	shared_ptr<MacroObject> macro = loadFile(path);
// 	if (macro == nullptr){
// 		return false;
// 	}
	
// 	exec(*macro, dst);
// 	return true;
// }


// ------------------------------------------------------------------------------------------ //