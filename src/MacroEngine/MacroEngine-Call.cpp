#include "MacroEngine.hpp"
#include "fs.hpp"
#include "stack_vector.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Structures ] --------------------------------------- //


struct var_copy {
	string_view name;
	Value value;
	bool defined = false;
};


enum class IncludeType {
	AUTO, HTML, CSS, JS, TXT
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::call(const Node& op, Node& dst){
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
	
	const Macro* macro = MacroCache::get(macro_name);
	if (macro == nullptr || macro->html == nullptr){
		HERE(error_macro_not_found(op, name_attr->value(), name_attr->value()));
		return;
	}
	
	// Evaluate default parameters
	for (const Attr* param = macro->html->attribute ; param != nullptr ; param = param->next){
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
	
	const Macro* macro = MacroCache::get(macro_name);
	if (macro == nullptr || macro->html == nullptr){
		HERE(error_macro_not_found(op, attr.value(), macro_name));
		return;
	}
	
	// Global variable backup, local variables shadow global
	stack_vector<var_copy,2> backup;
	
	// Evaluate default parameters
	for (const Attr* param = macro->html->attribute ; param != nullptr ; param = param->next){
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


/**
 * @brief Transfer leading and trailing whitespace.
 * @param dst Node with new child elements (reversed list).
 * @param dst_original_last Original first child before include added more child elements.
 */
static void _include_transfer_space(const Node& op, Node& dst, Node* dst_original_last){
	if (dst.child == nullptr){
		return;
	}
	
	// Transfer trailing space to last new child
	if (dst.child != dst_original_last){
		dst.child->options |= (op.options & NodeOptions::SPACE_AFTER);
	}
	
	// Find first new child and transfer leading space
	for (Node* child = dst.child ; child != nullptr ; child = child->next){
		if (child->next == dst_original_last){
			child->options |= (op.options & NodeOptions::SPACE_BEFORE);
			break;
		}
	}
	
}


static void _include_macro(const Node& op, const Attr& src, stack_vector<var_copy,2>& args, bool wrap, Node& dst){
	// Evaluate src path
	filepath path;
	{
		string path_sv_buff;
		string_view path_sv;
		
		if (src.value_len <= 0){
			HERE(error_missing_attr_value(op, src));
			return;
		} else if (!eval_attr_value(op, src, path_sv_buff, path_sv)){
			return;
		}
		
		path = path_sv;
	}
	
	Macro* macro = MacroCache::load(path);
	if (macro == nullptr){
		if (!fs::exists(path))
			HERE(error_file_not_found(op, src.value(), path.c_str()))
		else
			HERE(error_include_fail(op, src.value(), path.c_str()))
		return;
	}
	
	Node* const original_first = dst.child;
	
	// Execute as HTML macro
	if (macro->type == Macro::Type::HTML){
		if (macro->html == nullptr && !macro->parseHTML()){
			HERE(error_include_fail(op, src.value(), path.c_str()));
			return;
		}
		
		// Apply arguments to variable list
		for (var_copy& arg : args){
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
		for (var_copy& arg : args){
			if (arg.defined)
				MacroEngine::variables.insert(arg.name, move(arg.value));
			else
				MacroEngine::variables.remove(arg.name);
		}
		
	}
	
	// Execute as CSS
	else if (macro->type == Macro::Type::CSS){
		if (macro->txt == nullptr){
			HERE(error_include_fail(op, src.value(), path.c_str()));
			return;
		}
		
		Node* parent = &dst;
		if (wrap){
			parent = &dst.appendChild(NodeType::TAG);
			parent->name("style");
		}
		
		Node& txt = parent->appendChild(NodeType::TEXT);
		txt.value(*macro->txt);
	}
	
	// Execute as JS
	else if (macro->type == Macro::Type::JS){
		if (macro->txt == nullptr){
			HERE(error_include_fail(op, src.value(), path.c_str()));
			return;
		}
		
		Node* parent = &dst;
		if (wrap){
			parent = &dst.appendChild(NodeType::TAG);
			parent->name("script");
		}
		
		Node& txt = parent->appendChild(NodeType::TEXT);
		txt.value(*macro->txt);
	}
	
	
	// Text
	else if (macro->txt == nullptr){
		HERE(error_include_fail(op, src.value(), path.c_str()));
		return;
	} else {
		Node& txt = dst.appendChild(NodeType::TEXT);
		txt.value(*macro->txt);
	}
	
	_include_transfer_space(op, dst, original_first);
}



void MacroEngine::include(const Node& op, Node& dst){
	stack_vector<var_copy,2> args;
	const Attr* src_attr = nullptr;
	bool wrap = true;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name == "SRC"){
			if (src_attr == nullptr)
				src_attr = attr;
			else
				HERE(warn_duplicate_attr(op, *src_attr, *attr));
			continue;
		}
		
		else if (name == "NO-WRAP"){
			if (attr->value_p != nullptr)
				HERE(warn_ignored_attr_value(op, *attr));
			wrap = false;
			continue;
		}
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		// Push argument
		var_copy& var = args.emplace_back(name);
		
		if (attr->value_p == nullptr){
			Value* gvar = MacroEngine::variables.get(name);
			if (gvar != nullptr)
				var.value = *gvar;
		} else if (!eval_attr_value(op, *attr, var.value)){
			return;
		}
		
	}
	
	if (src_attr == nullptr){
		HERE(warn_missing_attr(op, "SRC"));
		return;
	}
	
	_include_macro(op, *src_attr, args, wrap, dst);
}


void MacroEngine::include(const Node& op, const Attr& attr, Node& dst){
	stack_vector<var_copy,2> args;
	_include_macro(op, attr, args, false, dst);
}


// ------------------------------------------------------------------------------------------ //