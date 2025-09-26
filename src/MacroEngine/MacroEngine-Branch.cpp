#include "MacroEngine.hpp"
#include "Debug.hpp"
#include "Debugger.hpp"

using namespace std;
using namespace html;


// ----------------------------------- [ Functions ] ---------------------------------------- //


#define IS_TYPE(e, t)	(std::is_same_v<std::decay_t<decltype(e)>, t>)
#define IS_STR(e)		(IS_TYPE(e, string))

constexpr bool isUpper(char c){
	return 'A' <= c && c <= 'Z';
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::set(const Node& op){
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		string_view value = attr->value();
		
		if (name.empty()){
			assert(!name.empty());
			continue;
		}
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		// Expression
		if (attr->options % NodeOptions::SINGLE_QUOTE){
			const NodeDebugger dbg = NodeDebugger(op);
			
			Expression expr = Expression::parse(value, dbg);
			if (expr == nullptr){
				return;
			}
			
			MacroEngine::variables.insert(name, expr.eval(MacroEngine::variables, dbg));
		}
		
		// Interpolate
		else if (attr->options % NodeOptions::INTERPOLATE){
			string buff;
			if (!eval_string(op, attr->value(), buff)){
				return;
			}
			
			MacroEngine::variables.insert(name, move(buff));
		}
		
		// Plain text
		else {
			MacroEngine::variables.insert(name, attr->value());
		}
		
	}
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static MacroEngine::Branch attr_equals_variable(const Node& op, const Attr& attr){
	string_view var_name = attr.name();
	const Value* var = MacroEngine::variables.get(var_name);
	
	bool pass;
	
	// Variable exists or is empty
	if (attr.value_p == nullptr){
		pass = (var != nullptr && var->toBool());
	}
	
	// Evaluate expression
	else if (attr.options % NodeOptions::SINGLE_QUOTE){
		const NodeDebugger dbg = NodeDebugger(op);
		
		Expression expr = Expression::parse(attr.value(), dbg);
		if (expr == nullptr){
			return MacroEngine::Branch::NONE;
		}
		
		Value val = expr.eval(MacroEngine::variables, dbg);
		pass = (var == nullptr && !val.toBool()) || (*var == val);
	}
	
	// Interpolate
	else if (attr.options % NodeOptions::INTERPOLATE){
		if (var != nullptr && var->type != Value::Type::STRING){
			pass = false;
		}
		
		// Compare only strings
		else {
			string buff;
			if (!MacroEngine::eval_string(op, attr.value(), buff))
				return MacroEngine::Branch::NONE;
			pass = (var == nullptr && buff.empty()) || (var->sv() == buff);
		}
	
	}
	
	// Plain text
	else {
		string_view buff = attr.value();
		if (var == nullptr)
			pass = buff.empty();
		else if (var->type == Value::Type::STRING)
			pass = (var->sv() == buff);
		else
			pass = false;
	}
	
	return (pass) ? MacroEngine::Branch::PASSED : MacroEngine::Branch::FAILED;
}


void MacroEngine::branch_if(const Node& op, Node& dst){
	auto _interp = MacroEngine::currentInterpolation;
	
	// Chain of `&&` statements
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name.empty()){
			continue;
		} else if (!isUpper(name[0])){
			goto cmp_var;
		}
		
		else if (name == "TRUE"){
			if (!eval_attr_true(op, *attr))
				goto fail;
			continue;
		} else if (name == "FALSE"){
			if (!eval_attr_false(op, *attr))
				goto fail;
			continue;
		} else if (name == "INTERPOLATE"){
			MacroEngine::currentInterpolation = eval_attr_interp(op, *attr);
			continue;
		}
		
		// Compare attribute to variable
		cmp_var:
		switch (attr_equals_variable(op, *attr)){
			case Branch::NONE:
			case Branch::FAILED:
				goto fail;
			case Branch::PASSED:
				continue;
		}
		
	}
	
	// pass:
	runChildren(op, dst);
	MacroEngine::currentInterpolation = _interp;
	MacroEngine::currentBranch_block = Branch::PASSED;
	return;
	
	fail:
	MacroEngine::currentInterpolation = _interp;
	MacroEngine::currentBranch_block = Branch::FAILED;
}


void MacroEngine::branch_elif(const Node& op, Node& dst){
	switch (MacroEngine::currentBranch_block){
		case Branch::NONE:
			HERE(error_missing_preceeding_if_node(op));
		case Branch::PASSED:
			return;
		case Branch::FAILED:
			branch_if(op, dst);
	}
}


void MacroEngine::branch_else(const Node& op, Node& dst){
	switch (MacroEngine::currentBranch_block){
		case Branch::NONE:
			HERE(error_missing_preceeding_if_node(op));
		case Branch::PASSED:
			MacroEngine::currentBranch_block = Branch::NONE;
			return;
		case Branch::FAILED:
			break;
	}
	
	auto _interp = MacroEngine::currentInterpolation;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		if (attr->name() == "INTERPOLATE"){
			MacroEngine::currentInterpolation = eval_attr_interp(op, *attr);
		} else {
			HERE(warn_ignored_attribute(op, *attr));
		}
	}
	
	// Run
	runChildren(op, dst);
	MacroEngine::currentInterpolation = _interp;
	MacroEngine::currentBranch_block = Branch::NONE;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


long MacroEngine::loop_for(const Node& op, Node& dst){
	const auto _interp = MacroEngine::currentInterpolation;
	
	bool cond_expected = true;
	const Attr* attr_setup = nullptr;
	const Attr* attr_cond = nullptr;
	const Attr* attr_inc = nullptr;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return 0;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		if (name == "INTERPOLATE"){
			MacroEngine::currentInterpolation = eval_attr_interp(op, *attr);
			continue;
		}
		
		// Second argument: condition
		else if (name == "TRUE" || name == "FALSE"){
			cond_expected = (name[0] != 'F');
			
			if (attr_cond != nullptr){
				HERE(error_duplicate_attr(op, *attr_cond, *attr));
				return 0;
			}
			
			attr_cond = attr;
		}
		
		// First argument: setup
		else if (attr_cond == nullptr){
			if (attr_setup != nullptr){
				HERE(error_duplicate_attr(op, *attr_setup, *attr));
				return 0;
			}
			attr_setup = attr;
		}
		
		// Third argument: increment
		else {
			if (attr_inc != nullptr){
				HERE(error_duplicate_attr(op, *attr_inc, *attr));
				return 0;
			}
			attr_inc = attr;
		}
		
		continue;
	}
	
	// Parse expressions
	const NodeDebugger dbg = NodeDebugger(op);
	Expression expr_setup = {};
	Expression expr_cond = {};
	Expression expr_inc = {};
	
	if (attr_setup != nullptr){
		if (attr_setup->options % NodeOptions::SINGLE_QUOTE == false){
			HERE(warn_attr_double_quote(op, *attr_setup));
		}
		
		expr_setup = Expression::parse(attr_setup->value(), dbg);
		if (expr_setup == nullptr){
			return 0;
		}
	}
	
	if (attr_cond != nullptr){
		if (attr_cond->options % NodeOptions::SINGLE_QUOTE == false){
			HERE(warn_attr_double_quote(op, *attr_cond));
		}
		
		expr_cond = Expression::parse(attr_cond->value(), dbg);
		if (expr_cond == nullptr){
			return 0;
		}
		
	} else {
		HERE(error_missing_attr(op, "TRUE"));
		return 0;
	}
	
	if (attr_inc != nullptr){
		if (attr_inc->options % NodeOptions::SINGLE_QUOTE == false){
			HERE(warn_attr_double_quote(op, *attr_inc));
		}
		
		expr_inc = Expression::parse(attr_inc->value(), dbg);
		if (expr_inc == nullptr){
			return 0;
		}
		
	}
	
	// Run setup
	if (expr_setup != nullptr){
		variables.insert(attr_setup->name(), expr_setup.eval(variables, dbg));
	}
	
	// Run loop
	const auto _interp_2 = MacroEngine::currentInterpolation; 
	long i = 0;
	
	assert(expr_cond != nullptr);
	while (expr_cond.eval(variables, dbg).toBool() == cond_expected){
		MacroEngine::currentInterpolation = _interp_2;
		runChildren(op, dst);
		
		// Increment
		if (expr_inc != nullptr){
			variables.insert(attr_inc->name(), expr_inc.eval(variables, dbg));
		}
		
		i++;
	}
	
	
	// Restore state
	MacroEngine::currentInterpolation = _interp;
	return i;
}


long MacroEngine::loop_while(const Node& op, Node& dst){
	const auto _interp = MacroEngine::currentInterpolation;
	
	bool cond_expected = true;
	const Attr* attr_cond = nullptr;
	
	// Parse attributes
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return 0;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		if (name == "INTERPOLATE"){
			MacroEngine::currentInterpolation = eval_attr_interp(op, *attr);
			continue;
		}
		
		else if (name == "TRUE" || name == "FALSE"){
			cond_expected = (name[0] == 'T');
			
			if (attr_cond != nullptr){
				HERE(error_duplicate_attr(op, *attr_cond, *attr));
				return 0;
			}
			
			attr_cond = attr;
		}
		
		else {
			HERE(warn_ignored_attribute(op, *attr));
		}
		
		continue;
	}
	
	// Parse expressions
	const NodeDebugger dbg = NodeDebugger(op);
	Expression expr_cond = {};
	
	if (attr_cond != nullptr){
		if (attr_cond->options % NodeOptions::SINGLE_QUOTE == false){
			HERE(warn_attr_double_quote(op, *attr_cond));
		}
		
		expr_cond = Expression::parse(attr_cond->value(), dbg);
		if (expr_cond == nullptr){
			return 0;
		}
		
	} else {
		HERE(error_missing_attr(op, "TRUE"));
		return 0;
	}
	
	// Run
	assert(expr_cond != nullptr);
	const auto _interp_2 = MacroEngine::currentInterpolation;
	long i = 0;
	
	while (expr_cond.eval(variables, dbg).toBool() == cond_expected){
		MacroEngine::currentInterpolation = _interp_2;
		runChildren(op, dst);
		i++;
	}
	
	// Restore state
	MacroEngine::currentInterpolation = _interp;
	return i;
}


// ------------------------------------------------------------------------------------------ //