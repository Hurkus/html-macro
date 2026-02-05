#include "MacroEngine.hpp"
#include "stack_vector.hpp"
#include "fs.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;


// ----------------------------------- [ Structures ] --------------------------------------- //


struct var_copy {
	string_view name;
	Value value;
	bool defined = false;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void _invoke_macro(MacroEngine& self, const Node& op, shared_ptr<Macro>&& macro, stack_vector<var_copy,2>& args, Node& dst){
	assert(macro != nullptr);
	assert(self.variables != nullptr);
	
	if (macro->html == nullptr){
		return;
	}
	
	// Evaluate default parameters
	for (const Attr* param = macro->html->attribute ; param != nullptr ; param = param->next){
		string_view name = param->name();
		assert(!name.empty());
		
		if (name == "NAME"){
			continue;
		}
		
		// Check if default value needed
		for (var_copy& v : args){
			if (v.name == name)
				goto next;
		}
		
		// Clone variable
		if (param->value_p == nullptr){
			Value* var = self.variables->get(name);
			
			if (var != nullptr)
				args.emplace_back(name, *var, true);
			else
				args.emplace_back(name);
		}
		
		// Evaluate default value
		else {
			Value& arg = args.emplace_back(name).value;
			if (!self.eval_attr_value(op, *param, arg))
				return;
		}
		
		next: continue;
	}
	
	// Apply arguments to variable list
	for (var_copy& arg : args){
		Value* var = self.variables->get(arg.name);
		
		if (var != nullptr){
			swap(arg.value, *var);
			arg.defined = true;
		} else {
			self.variables->insert(arg.name, move(arg.value));
		}
		
	}
	
	self.exec(move(macro), dst);
	
	// Restore argument list
	for (var_copy& arg : args){
		if (arg.defined)
			self.variables->insert(arg.name, move(arg.value));
		else
			self.variables->remove(arg.name);
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::call_userElementMacro(const Node& op, Node& dst){
	assert(macro != nullptr);
	
	// Verify macro existance
	shared_ptr<Macro> target_macro = MacroCache::get(op.name());
	if (target_macro == nullptr){
		return false;
	} else if (target_macro->html == nullptr){
		HERE(warn_macro_not_invokable(*macro, op.name(), op.name()));
		return false;
	}
	
	stack_vector<var_copy,2> args;
	
	// Parse operation attributes
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *attr)){
			case Branch::FAILED: return true;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		var_copy& var = args.emplace_back(name);
		
		// Clone global variable or evaluate new local
		if (attr->value_p == nullptr){
			Value* gvar = variables->get(name);
			if (gvar != nullptr)
				var.value = *gvar;
		} else if (!eval_attr_value(op, *attr, var.value)){
			return true;
		}
		
	}
	
	_invoke_macro(*this, op, move(target_macro), args, dst);
	return true;
}


bool MacroEngine::call_userAttrMacro(const Node& op, const Attr& attr, Node& dst){
	assert(!attr.name().empty());
	
	shared_ptr<Macro> target_macro = MacroCache::get(attr.name());
	if (target_macro == nullptr ){
		return false;
	} else if (target_macro->html == nullptr){
		HERE(warn_macro_not_invokable(*macro, attr.name(), attr.name()));
		return false;
	}
	
	stack_vector<var_copy,2> args;
	
	// Store attribute value as argument `VALUE`
	if (attr.value_p != nullptr){
		var_copy& arg = args.emplace_back("VALUE");
		if (!eval_attr_value(op, attr, arg.value))
			return true;
	}
	
	_invoke_macro(*this, op, move(target_macro), args, dst);
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::call(const Node& op, Node& dst){
	assert(macro != nullptr);
	assert(variables != nullptr);
	
	const Attr* name_attr = nullptr;
	stack_vector<var_copy,2> args;
	
	// Parse operation attributes
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name == "NAME"){
			if (name_attr == nullptr)
				name_attr = attr;
			else
				HERE(warn_duplicate_attr(*macro, *name_attr, *attr));
			continue;
		}
		
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		var_copy& var = args.emplace_back(name);
		
		// Clone global variable or evaluate new local
		if (attr->value_p == nullptr){
			Value* gvar = variables->get(name);
			if (gvar != nullptr)
				var.value = *gvar;
		} else if (!eval_attr_value(op, *attr, var.value)){
			return;
		}
		
	}
	
	// Eval macro name
	string buff;
	string_view macro_name;
	
	if (name_attr == nullptr){
		HERE(error_missing_attr(*macro, op, "NAME"));
		return;
	} else if (name_attr->value_len <= 0){
		HERE(error_missing_attr_value(*macro, *name_attr));
		return;
	} else if (!eval_attr_value(op, *name_attr, buff, macro_name)){
		return;
	}
	
	shared_ptr<Macro> target_macro = MacroCache::get(macro_name);
	if (target_macro == nullptr){
		HERE(error_macro_not_found(*macro, macro_name, name_attr->value()));
		return;
	} else if (target_macro->html == nullptr){
		HERE(warn_macro_not_invokable(*macro, macro_name, name_attr->value()));
		return;
	}
	
	_invoke_macro(*this, op, move(target_macro), args, dst);
}


void MacroEngine::call(const Node& op, const Attr& attr, Node& dst){
	assert(macro != nullptr);
	assert(variables != nullptr);
	
	string buff;
	string_view macro_name;
	
	if (attr.value_len <= 0){
		HERE(error_missing_attr_value(*macro, attr));
		return;
	} else if (!eval_attr_value(op, attr, buff, macro_name)){
		return;
	}
	
	shared_ptr<Macro> target_macro = MacroCache::get(macro_name);
	if (target_macro == nullptr){
		HERE(error_macro_not_found(*macro, macro_name, attr.value()));
		return;
	} else if (target_macro->html == nullptr){
		HERE(warn_macro_not_invokable(*macro, macro_name, attr.value()));
		return;
	}
	
	stack_vector<var_copy,2> args;
	_invoke_macro(*this, op, move(target_macro), args, dst);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void _include_header(MacroEngine& self, const Node& op, const Attr& src){
	assert(self.macro != nullptr);
	
	// Evaluate src path
	filepath path;
	{
		string buff;
		string_view path_sv;
		
		if (src.value_len <= 0){
			HERE(error_missing_attr_value(*self.macro, src));
			return;
		} else if (!self.eval_attr_value(op, src, buff, path_sv)){
			return;
		}
		
		path = path_sv;
	}
	
	shared_ptr<Macro> file_macro = MacroCache::load(path);
	if (file_macro == nullptr){
		HERE(error_include_file_not_found(*self.macro, path.c_str(), src.value()));
		return;
	}
	
	switch (file_macro->type){
		case Macro::Type::HTML: {
			if (file_macro->html == nullptr && !file_macro->parseHTML())
				HERE(error_include_file_fail(*self.macro, path.c_str(), src.value()));
		} break;
		
		default: {
			if (file_macro->txt == nullptr)
				HERE(error_include_file_fail(*self.macro, path.c_str(), src.value()));
		} break;
		
	}
	
	return;
}


/**
 * @brief Transfer leading and trailing whitespace.
 * @param dst Node with new child elements (reversed list).
 * @param dst_original_last Original first child before include added more child elements.
 */
static void _include_transfer_space(const Node& op, Node& dst, const Node* dst_original_last){
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


static void _include_macro(MacroEngine& self, const Node& op, const Attr& src, stack_vector<var_copy,2>& args, bool wrap, Node& dst){
	assert(self.macro != nullptr);
	assert(self.macro->html != nullptr);
	assert(self.variables != nullptr);
	
	// Evaluate src path
	filepath path;
	{
		string buff;
		string_view path_sv;
		
		if (src.value_len <= 0){
			HERE(error_missing_attr_value(*self.macro, src));
			return;
		} else if (!self.eval_attr_value(op, src, buff, path_sv)){
			return;
		}
		
		path = path_sv;
	}
	
	shared_ptr<Macro> file_macro = MacroCache::load(path);
	if (file_macro == nullptr){
		HERE(error_include_file_not_found(*self.macro, path.c_str(), src.value()))
		return;
	}
	
	Node* const original_first = dst.child;
	
	switch (file_macro->type){
		case Macro::Type::HTML: {
			if (file_macro->html == nullptr && !file_macro->parseHTML()){
				HERE(warn_include_file_type_downcast(*self.macro, path.c_str(), src.value()));
				goto _type_txt;
			}
			
			// Apply arguments to variable list
			for (var_copy& arg : args){
				Value* var = self.variables->get(arg.name);
				
				if (var != nullptr){
					swap(arg.value, *var);
					arg.defined = true;
				} else {
					self.variables->insert(arg.name, move(arg.value));
				}
				
			}

			assert(file_macro->html != nullptr);
			self.exec(file_macro, dst);

			// Restore argument list
			for (var_copy& arg : args){
				if (arg.defined)
					self.variables->insert(arg.name, move(arg.value));
				else
					self.variables->remove(arg.name);
			}
			
		} break;
		
		case Macro::Type::CSS: {
			if (file_macro->txt == nullptr){
				goto _fail;
			}
			
			// Wrap in <style></style>
			Node* parent = &dst;
			if (wrap){
				parent = dst.appendChild(self.newNode(NodeType::TAG));
				parent->name(*self.macro->html, string_view("style"));
			}
			
			Node& txt = *parent->appendChild(self.newNode(NodeType::TEXT));
			txt.value_p = file_macro->txt->c_str();
			txt.value_len = uint32_t(min(file_macro->txt->length(), size_t(UINT32_MAX)));
		} break;
		
		case Macro::Type::JS: {
			if (file_macro->txt == nullptr){
				goto _fail;
			}
			
			// Wrap in <script></script>
			Node* parent = &dst;
			if (wrap){
				parent = dst.appendChild(self.newNode(NodeType::TAG));
				parent->name(*self.macro->html, string_view("script"));
			}
			
			Node& txt = *parent->appendChild(self.newNode(NodeType::TEXT));
			txt.value_p = file_macro->txt->c_str();
			txt.value_len = uint32_t(min(file_macro->txt->length(), size_t(UINT32_MAX)));
		} break;
		
		_type_txt:
		case Macro::Type::TXT: {
			if (file_macro->txt == nullptr){
				goto _fail;
			}
			
			Node& txt = *dst.appendChild(self.newNode(NodeType::TEXT));
			txt.value_p = file_macro->txt->c_str();
			txt.value_len = uint32_t(min(file_macro->txt->length(), size_t(UINT32_MAX)));
		} break;
		
		_fail:
		default: {
			HERE(error_include_file_fail(*self.macro, path.c_str(), src.value()));
			return;
		};
		
	}
	
	_include_transfer_space(op, dst, original_first);
}


void MacroEngine::include(const Node& op, Node& dst){
	assert(macro != nullptr);
	assert(variables != nullptr);
	
	stack_vector<var_copy,2> args;
	const Attr* src_attr = nullptr;
	bool header = false;
	bool wrap = true;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name == "SRC"){
			if (src_attr == nullptr)
				src_attr = attr;
			else
				HERE(warn_duplicate_attr(*macro, *src_attr, *attr));
			continue;
		}
		
		else if (name == "HEADER"){
			if (attr->value_p != nullptr)
				HERE(warn_ignored_attr_value(*macro, *attr));
			header = true;
			continue;
		}
		
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		// All remaining attributes are irrelevant for headers
		if (header){
			HERE(warn_ignored_attr(*macro, *attr));
			continue;
		}
		
		else if (name == "NO-WRAP"){
			if (attr->value_p != nullptr)
				HERE(warn_ignored_attr_value(*macro, *attr));
			wrap = false;
			continue;
		}
		
		// Push argument
		var_copy& var = args.emplace_back(name);
		
		if (attr->value_p == nullptr){
			Value* gvar = variables->get(name);
			if (gvar != nullptr)
				var.value = *gvar;
		} else if (!eval_attr_value(op, *attr, var.value)){
			return;
		}
		
	}
	
	if (src_attr == nullptr){
		HERE(warn_missing_attr(*macro, op, "SRC"));
	} else if (header){
		_include_header(*this, op, *src_attr);
	} else {
		_include_macro(*this, op, *src_attr, args, wrap, dst);
	}
	
}


void MacroEngine::include(const Node& op, const Attr& attr, Node& dst){
	stack_vector<var_copy,2> args;
	_include_macro(*this, op, attr, args, false, dst);
}


// ------------------------------------------------------------------------------------------ //