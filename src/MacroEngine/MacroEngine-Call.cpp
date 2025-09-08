#include "MacroEngine.hpp"
#include "ExpressionParser.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;
using namespace Expression;


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
		} else if (name == "IF"){
			if (!eval_attr_if(op, *attr))
				return;
		} else {
			HERE(warn_ignored_attribute(op, *attr));
		}
		
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
	const Attr* attr_src = nullptr;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name == "SRC"){
			attr_src = attr;
		} else if (name == "IF"){
			if (!eval_attr_if(op, *attr))
				return;
		} else {
			HERE(warn_ignored_attribute(op, *attr));
		}
		
	}
	
	string buff;
	string_view path_view;
	
	if (attr_src == nullptr){
		HERE(warn_missing_attr(op, "SRC"));
		return;
	} else if (attr_src->value().empty()){
		HERE(warn_missing_attr_value(op, *attr_src));
		return;
	} else if (!eval_attr_value(op, *attr_src, buff, path_view)){
		return;
	}
	
	filepath path = MacroEngine::resolve(path_view);
	if (!filesystem::exists(path)){
		HERE(error_file_not_found(op, *attr_src, path.c_str()));
		return;
	}
	
	// Fetch and execute macro
	const Macro* macro = Macro::loadFile(path, false);
	if (macro == nullptr){
		HERE(warn_file_include(op, *attr_src, path.c_str()));
		return;
	}
	
	exec(*macro, dst);
}


// ------------------------------------------------------------------------------------------ //