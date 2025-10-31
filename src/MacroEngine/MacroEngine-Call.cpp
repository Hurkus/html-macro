#include "MacroEngine.hpp"
#include "stack_vector.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::call(const Node& op, Node& dst){
	struct var_copy {
		string_view name;
		Value value;
		bool defined = false;
	};
	
	const Attr* name_attr = nullptr;
	stack_vector<var_copy,2> backup;
	
	// Parse operation attributes
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name == "NAME"){
			if (name_attr == nullptr)
				name_attr = attr;
			else
				HERE(warn_duplicate_attr(op, *name_attr, *attr));
			continue;
		}
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		var_copy& var = backup.emplace_back(name);
		
		// Clone global variable or evaluate new local
		if (attr->value_p == nullptr){
			Value* gvar = MacroEngine::variables.get(name);
			if (gvar != nullptr)
				var.value = *gvar;
		} else if (!eval_attr_value(op, *attr, var.value)){
			return;
		}
		
	}
	
	// Verify macro name
	if (name_attr == nullptr){
		HERE(error_missing_attr(op, "NAME"));
		return;
	} else if (name_attr->value_len <= 0){
		HERE(error_missing_attr_value(op, *name_attr));
		return;
	}
	
	// Fetch macro
	string macro_name_buff;
	string_view macro_name;
	if (!eval_attr_value(op, *name_attr, macro_name_buff, macro_name)){
		return;
	}
	
	const Macro* macro = Macro::get(macro_name);
	if (macro == nullptr){
		HERE(error_macro_not_found(op, name_attr->value(), name_attr->value()));
		return;
	}
	
	// Evaluate default parameters
	for (const Attr* param = macro->doc.attribute ; param != nullptr ; param = param->next){
		string_view name = param->name();
		if (name.empty() || name == "NAME"sv){
			continue;
		}
		
		// Check if default value needed
		for (var_copy& v : backup){
			if (v.name == name)
				goto next;
		}
		
		// Clone variable
		if (param->value_p == nullptr){
			Value* var = MacroEngine::variables.get(name);
			
			if (var != nullptr){
				backup.emplace_back(name, *var, true);
			} else {
				backup.emplace_back(name);
			}
			
		}
		
		// Evaluate default value
		else {
			Value& arg = backup.emplace_back(name).value;
			if (!eval_attr_value(op, *param, arg))
				return;
		}
		
		next: continue;
	}
	
	// Apply arguments to variable list
	for (var_copy& arg : backup){
		Value* var = MacroEngine::variables.get(arg.name);
		
		if (var != nullptr){
			swap(arg.value, *var);
			arg.defined = true;
		} else {
			MacroEngine::variables.insert(arg.name, move(arg.value));
		}
		
	}
	
	exec(*macro, dst);
	
	// Restore argument list
	for (var_copy& arg : backup){
		if (arg.defined)
			MacroEngine::variables.insert(arg.name, move(arg.value));
		else
			MacroEngine::variables.remove(arg.name);
	}
	
}


void MacroEngine::call(const Node& op, const Attr& attr, Node& dst){
	string macro_name_buff;
	string_view macro_name;
	
	if (attr.value_len <= 0){
		HERE(error_missing_attr_value(op, attr));
		return;
	} else if (!eval_attr_value(op, attr, macro_name_buff, macro_name)){
		return;
	}
	
	const Macro* macro = Macro::get(macro_name);
	if (macro == nullptr){
		HERE(error_macro_not_found(op, attr.value(), macro_name));
		return;
	}
	
	// Global variable backup, local variables shadow global
	struct var_copy {
		string_view name;
		Value value;
		bool defined = false;
	};
	stack_vector<var_copy,2> backup;
	
	// Evaluate default parameters
	for (const Attr* param = macro->doc.attribute ; param != nullptr ; param = param->next){
		string_view name = param->name();
		if (name.empty() || name == "NAME"sv){
			continue;
		}
		
		// Clone variable
		if (param->value_p == nullptr){
			Value* var = MacroEngine::variables.get(name);
			
			if (var != nullptr){
				backup.emplace_back(name, *var, true);
			} else {
				backup.emplace_back(name);
			}
			
		}
		
		// Evaluate default value
		else {
			Value& arg = backup.emplace_back(name).value;
			if (!eval_attr_value(op, *param, arg))
				return;
		}
		
	}
	
	// Apply arguments to variable list
	for (var_copy& arg : backup){
		Value* var = MacroEngine::variables.get(arg.name);
		
		if (var != nullptr){
			swap(arg.value, *var);
			arg.defined = true;
		} else {
			MacroEngine::variables.insert(arg.name, move(arg.value));
		}
		
	}
	
	exec(*macro, dst);
	
	// Restore argument list
	for (var_copy& arg : backup){
		if (arg.defined)
			MacroEngine::variables.insert(arg.name, move(arg.value));
		else
			MacroEngine::variables.remove(arg.name);
	}
	
	return;
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
	
	Node* const original_first = dst.child;
	
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
	
	// Transfer leading and trailing whitespace
	if (dst.child != nullptr){
		
		// Transfer trailing space to last new child
		if (dst.child != original_first){
			dst.child->options |= (op.options & NodeOptions::SPACE_AFTER);
		}
		
		// Transfer leading space to first new child
		if (original_first != nullptr){
			if (original_first->next != nullptr)
				original_first->next->options |= (op.options & NodeOptions::SPACE_BEFORE);
		} else {
			// Find first child
			Node* first = dst.child;
			while (first->next != nullptr)
				first = first->next;
			
			first->options |= (op.options & NodeOptions::SPACE_BEFORE);
		}
		
	}
	
	return true;
}


// ------------------------------------------------------------------------------------------ //