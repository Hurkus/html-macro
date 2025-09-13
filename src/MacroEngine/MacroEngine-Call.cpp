#include "MacroEngine.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Functions ] ---------------------------------------- //


// TODO: multiple NAME
void MacroEngine::call(const Node& op, Node& dst){
	const Attr* attr_name = nullptr;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name == "NAME"){
			if (attr_name != nullptr){
				HERE(error_duplicate_attr(op, *attr_name, *attr));
				return;
			} else {
				attr_name = attr;
			}
			continue;
		}
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		HERE(warn_ignored_attribute(op, *attr));
	}
	
	string buff;
	string_view macro_name;
	
	if (attr_name == nullptr){
		HERE(warn_missing_attr(op, "NAME"));
		return;
	} else if (attr_name->value().empty()){
		HERE(warn_missing_attr_value(op, *attr_name));
		return;
	} else if (!eval_attr_value(op, *attr_name, buff, macro_name)){
		return;
	}
	
	const Macro* macro = Macro::get(macro_name);
	if (macro == nullptr){
		if (buff.empty())
			HERE(error_macro_not_found(op, *attr_name))
		else
			HERE(error_macro_not_found(op, *attr_name, buff.c_str()))
		return;
	}
	
	exec(*macro, dst);
}


void MacroEngine::call(const Node& op, const Attr& attr, Node& dst){
	string buff;
	string_view macro_name;
	
	if (attr.value().empty()){
		HERE(error_missing_attr_value(op, attr));
		return;
	} else if (!eval_attr_value(op, attr, buff, macro_name)){
		return;
	}
	
	const Macro* macro = Macro::get(macro_name);
	if (macro == nullptr){
		if (buff.empty())
			HERE(error_macro_not_found(op, attr))
		else
			HERE(error_macro_not_found(op, attr, buff.c_str()))
		return;
	}
	
	exec(*macro, dst);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::include(const Node& op, Node& dst){
	const Attr* src_attr = nullptr;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name == "SRC"){
			src_attr = attr;
			continue;
		} 
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE:
				HERE(warn_ignored_attribute(op, *attr));
				break;
		}
		
	}
	
	string src_buff;
	string_view src;
	
	if (src_attr == nullptr){
		HERE(warn_missing_attr(op, "SRC"));
		return;
	} else if (src_attr->value().empty()){
		HERE(warn_missing_attr_value(op, *src_attr));
		return;
	} else if (!eval_attr_value(op, *src_attr, src_buff, src)){
		return;
	}
	
	filepath path = MacroEngine::resolve(src);
	if (!filesystem::exists(path)){
		HERE(error_file_not_found(op, *src_attr, path.c_str()));
		return;
	}
	
	// Fetch and execute macro
	const Macro* macro = Macro::loadFile(path, false);
	if (macro == nullptr){
		HERE(warn_file_include(op, *src_attr, path.c_str()));
		return;
	}
	
	exec(*macro, dst);
}


// ------------------------------------------------------------------------------------------ //