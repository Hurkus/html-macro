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
				error_duplicate_attr(op, *attr_name, *attr);
				return;
			} else {
				attr_name = attr;
			}
		} else if (name == "IF"){
			if (!eval_attr_if(op, *attr))
				return;
		} else {
			warn_ignored_attribute(op, *attr);
		}
		
	}
	
	if (attr_name == nullptr){
		warn_missing_attr(op, "NAME");
		return;
	} else if (attr_name->value().empty()){
		error_missing_attr_value(op, *attr_name);
		return;
	}
	
	const Macro* macro = nullptr;
	
	// Evaluate expression
	if (attr_name->options % NodeOptions::SINGLE_QUOTE){
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