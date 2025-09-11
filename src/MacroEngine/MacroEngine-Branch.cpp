#include "MacroEngine.hpp"
#include "Debug.hpp"
#include "Debugger.hpp"

using namespace std;
using namespace html;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::set(const Node& op){
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		string_view value = attr->value();
		
		if (name.empty()){
			continue;
		} else if (name == "IF" && !eval_attr_if(op, *attr)){
			return;
		}
		
		// Expression
		if (attr->options % NodeOptions::SINGLE_QUOTE){
			const NodeDebugger dbg = NodeDebugger(op);
			
			pExpr expr = Expression::parse(value, dbg);
			if (expr == nullptr){
				return;
			}
			
			MacroEngine::variables.insert(name, expr->eval(MacroEngine::variables, dbg));
		}
		
		// Interpolate
		else if (attr->options % NodeOptions::INTERPOLATE){
			Value val;
			string& s = val.emplace<string>();
			
			if (!eval_string(op, attr->value(), s)){
				return;
			}
			
			MacroEngine::variables.insert(name, move(val));
		}
		
		// Plain text
		else {
			MacroEngine::variables.insert(name, in_place_type<string>, attr->value());
		}
		
	}
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::branch_if(const Node& op, Node& dst){
	auto _interp = MacroEngine::currentInterpolation;
	
	// Chain of `&&` statements
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name == "TRUE"){
			if (!eval_attr_true(op, *attr))
				goto fail;
		} else if (name == "FALSE"){
			if (!eval_attr_false(op, *attr))
				goto fail;
		} else if (name == "INTERPOLATE"){
			MacroEngine::currentInterpolation = eval_attr_interp(op, *attr);
		} else {
			HERE(warn_ignored_attribute(op, *attr));
		}
		
	}
	
	// pass:
	runChildren(op, dst);
	MacroEngine::currentInterpolation = _interp;
	MacroEngine::currentBranch_block = Branch::END;
	return;
	
	fail:
	MacroEngine::currentInterpolation = _interp;
	MacroEngine::currentBranch_block = Branch::ELSE;
}


void MacroEngine::branch_elif(const Node& op, Node& dst){
	switch (MacroEngine::currentBranch_block){
		case Branch::NONE:
			HERE(::error(op, "Missing preceding " ANSI_PURPLE "<IF>" ANSI_RESET " tag."));
		case Branch::END:
			return;
		case Branch::ELSE:
			branch_if(op, dst);
	}
}


void MacroEngine::branch_else(const Node& op, Node& dst){
	switch (MacroEngine::currentBranch_block){
		case Branch::NONE:
			HERE(::error(op, "Missing preceding " ANSI_PURPLE "<IF>" ANSI_RESET " tag."));
		case Branch::END:
			MacroEngine::currentBranch_block = Branch::NONE;
			return;
		case Branch::ELSE:
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
		
		if (name == "IF"){
			if (!eval_attr_if(op, *attr))
				return 0;
		} else if (name == "INTERPOLATE"){
			MacroEngine::currentInterpolation = eval_attr_interp(op, *attr);
		}
		
		// Second argument: condition
		else if (name == "TRUE" || name == "FALSE"){
			cond_expected = (name[0] == 'T');
			
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
	Expression::pExpr expr_setup = nullptr;
	Expression::pExpr expr_cond = nullptr;
	Expression::pExpr expr_inc = nullptr;
	
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
		variables.insert(attr_setup->name(), expr_setup->eval(variables, dbg));
	}
	
	// Run loop
	const auto _interp_2 = MacroEngine::currentInterpolation; 
	long i = 0;
	
	assert(expr_cond != nullptr);
	while (Expression::boolEval(expr_cond->eval(variables, dbg)) == cond_expected){
		MacroEngine::currentInterpolation = _interp_2;
		runChildren(op, dst);
		
		// Increment
		if (expr_inc != nullptr){
			variables.insert(attr_inc->name(), expr_inc->eval(variables, dbg));
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
		
		if (name == "IF"){
			if (!eval_attr_if(op, *attr))
				return 0;
		} else if (name == "INTERPOLATE"){
			MacroEngine::currentInterpolation = eval_attr_interp(op, *attr);
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
	Expression::pExpr expr_cond = nullptr;
	
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
	
	while (Expression::boolEval(expr_cond->eval(variables, dbg)) == cond_expected){
		MacroEngine::currentInterpolation = _interp_2;
		runChildren(op, dst);
		i++;
	}
	
	// Restore state
	MacroEngine::currentInterpolation = _interp;
	return i;
}


// ------------------------------------------------------------------------------------------ //