#include "MacroEngine.hpp"
#include "stack_vector.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::call(const Node& op, Node& dst){
	bool one_name = false;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name == "NAME"){
			one_name = true;
			call(op, *attr, dst);
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
	
	if (!one_name){
		HERE(warn_missing_attr(op, "NAME"));
		return;
	}
	
}


void MacroEngine::call(const Node& op, const Attr& attr, Node& dst){
	string macroName_buff;
	string_view macroName;
	
	if (attr.name_len <= 0){
		HERE(error_missing_attr_value(op, attr));
		return;
	} else if (!eval_attr_value(op, attr, macroName_buff, macroName)){
		return;
	}
	
	const Macro* macro = Macro::get(macroName);
	if (macro == nullptr){
		HERE(error_macro_not_found(op, attr.value(), macroName));
		return;
	}
	
	exec(*macro, dst);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


enum class IncludeType {
	AUTO, HTML, TXT, CSS, JS
};

static bool getIncludeType(const Node& op, std::string_view mark, std::string_view sfx, IncludeType& type){
	if (sfx == "")
		type = IncludeType::AUTO;
	else if (sfx == "-HTML")
		type = IncludeType::HTML;
	else if (sfx == "-TXT")
		type = IncludeType::TXT;
	else if (sfx == "-CSS")
		type = IncludeType::CSS;
	else if (sfx == "-JS")
		type = IncludeType::JS;
	else {
		HERE(error_invalid_include_type(op, mark));
		return false;
	}
	return true;
}


void MacroEngine::include(const Node& op, Node& dst){
	bool one_src = false;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name.starts_with("SRC")){
			one_src = true;
			include(op, *attr, dst);
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
	
	if (!one_src){
		HERE(warn_missing_attr(op, "SRC"));
		return;
	}
	
}


bool MacroEngine::include(const Node& op, const Attr& attr, Node& dst){
	// Evaluate source path
	string src_buff;
	string_view src;
	
	if (attr.value_len <= 0){
		HERE(error_missing_attr_value(op, attr));
		return false;
	} else if (!eval_attr_value(op, attr, src_buff, src)){
		return false;
	}
	
	// Get include type
	string_view incSuffix = attr.name();
	if (incSuffix.starts_with("INCLUDE")){
		incSuffix.remove_prefix(string_view("INCLUDE").length());
	} else if (incSuffix.starts_with("SRC")){
		incSuffix.remove_prefix(string_view("SRC").length());
	} else {
		HERE(error_invalid_include_type(op, incSuffix));
		return false;
	}
	
	IncludeType incType;
	if (!getIncludeType(op, attr.name(), incSuffix, incType)){
		return false;
	}
	
	// Resolve path
	filepath path = MacroEngine::resolve(src);
	if (!filesystem::exists(path)){
		HERE(error_file_not_found(op, attr.value(), path.c_str()));
		return false;
	}
	
	// Resolve unspecified include type
	if (incType == IncludeType::AUTO){
		if (src.ends_with(".html"))
			incType = IncludeType::HTML;
	}
	
	// Include HTML as macro
	if (incType == IncludeType::HTML){
		const Macro* macro = Macro::loadFile(path, false);
		if (macro == nullptr){
			HERE(error_include_fail(op, attr.value(), path.c_str()));
			return false;
		}
		
		exec(*macro, dst);
	}
	
	// Raw text include
	else {
		size_t len;
		char* str = Macro::loadRawFile(path, len);
		
		if (str == nullptr){
			HERE(error_include_fail(op, attr.value(), path.c_str()));
			return false;
		}
		
		Node& txt = dst.appendChild(NodeType::TEXT);
		txt.value(str, len);
	}
	
	return true;
}


// ------------------------------------------------------------------------------------------ //