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
	
	const Macro* macro = nullptr;
	
	if (attr_name == nullptr){
		HERE(warn_missing_attr(op, "NAME"));
		return;
	} else if (attr_name->value().empty()){
		error_missing_attr_value(op, *attr_name);
		return;
	}
	
	// Evaluate expression
	else if (attr_name->options % NodeOptions::SINGLE_QUOTE){
		pExpr expr = Expression::parse(attr_name->value());
		if (expr == nullptr){
			error_expression_parse(op, *attr_name);
			return;
		}
		
		string buff;
		Expression::str(expr->eval(MacroEngine::variables), buff);
		
		macro = Macro::get(buff);
		if (macro == nullptr){
			error_macro_not_found(op, *attr_name, buff.c_str());
			return;
		}
		
	}
	
	// Interpolate
	else if (attr_name->options % NodeOptions::INTERPOLATE){
		
		string buff;
		interpolate(attr_name->value(), MacroEngine::variables, buff);
		
		macro = Macro::get(buff);
		if (macro == nullptr){
			error_macro_not_found(op, *attr_name, buff.c_str());
			return;
		}
		
	}
	
	// Plain text
	else {
		macro = Macro::get(attr_name->value());
		if (macro == nullptr){
			error_macro_not_found(op, *attr_name);
			return;
		}
	}
	
	assert(macro != nullptr);
	exec(*macro, dst);
}


void MacroEngine::call(const Node& op, const Attr& attr, Node& dst){
	const Macro* macro = nullptr;
	
	if (attr.value().empty()){
		error_missing_attr_value(op, attr);
		return;
	}
	
	// Evaluate expression
	else if (attr.options % NodeOptions::SINGLE_QUOTE){
		pExpr expr = Expression::parse(attr.value());
		if (expr == nullptr){
			error_expression_parse(op, attr);
			return;
		}
		
		string buff;
		Expression::str(expr->eval(MacroEngine::variables), buff);
		
		macro = Macro::get(buff);
		if (macro == nullptr){
			error_macro_not_found(op, attr, buff.c_str());
			return;
		}
		
	}
	
	// Interpolate
	else if (attr.options % NodeOptions::INTERPOLATE){
		
		string buff;
		interpolate(attr.value(), MacroEngine::variables, buff);
		
		macro = Macro::get(buff);
		if (macro == nullptr){
			error_macro_not_found(op, attr, buff.c_str());
			return;
		}
		
	}
	
	// Plain text
	else {
		macro = Macro::get(attr.value());
		if (macro == nullptr){
			error_macro_not_found(op, attr);
			return;
		}
	}
	
	assert(macro != nullptr);
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
			warn_ignored_attribute(op, *attr);
		}
		
	}
	
	string buff;
	string_view path_view;
	
	if (attr_src == nullptr){
		error_missing_attr(op, "SRC");
		return;
	} else if (attr_src->value().empty()){
		warn_missing_attr_value(op, *attr_src);
		return;
	}
	
	// Evaluate expression
	else if (attr_src->options % NodeOptions::SINGLE_QUOTE){
		pExpr expr = Expression::parse(attr_src->value());
		if (expr == nullptr){
			error_expression_parse(op, *attr_src);
			return;
		}
		
		Expression::str(expr->eval(MacroEngine::variables), buff);
		path_view = buff;
	}
	
	// Interpolate
	else if (attr_src->options % NodeOptions::INTERPOLATE){
		interpolate(attr_src->value(), MacroEngine::variables, buff);
		path_view = buff;
	}
	
	// Plain text
	else {
		path_view = attr_src->value();
	}
	
	
	filepath path = MacroEngine::resolve(path_view);
	if (!filesystem::exists(path)){
		error_file_not_found(op, *attr_src, path.c_str());
		return;
	}
	
	// Fetch and execute macro
	const Macro* macro = Macro::loadFile(path, false);
	if (macro == nullptr){
		::error(op, "rip");
		return;
	}
	
	exec(*macro, dst);
}


// ------------------------------------------------------------------------------------------ //