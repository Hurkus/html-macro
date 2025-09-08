#include "MacroEngine.hpp"
#include "ExpressionParser.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::set(const Node& op){
	Expression::Parser parser = {};
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		string_view value = attr->value();
		
		if (name.empty()){
			continue;
		} else if (name == "IF" && !eval_attr_if(op, *attr)){
			return;
		}
		
		Value& var = MacroEngine::variables[name];
		
		// Expression
		if (attr->options % NodeOptions::SINGLE_QUOTE){
			pExpr expr = parser.parse(value);
			if (expr == nullptr){
				HERE(error_expression_parse(op, *attr));
				continue;
			}
			
			var = expr->eval(MacroEngine::variables);
		}
		
		// Interpolate
		else if (attr->options % NodeOptions::INTERPOLATE){
			string& s = var.emplace<string>();
			interpolate(attr->value(), MacroEngine::variables, s);
		}
		
		// Plain text
		else {
			var.emplace<string>(attr->value());
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
			} else if (attr->options % NodeOptions::SINGLE_QUOTE == false){
				HERE(warn_attr_double_quote(op, *attr));
			}
			
			attr_cond = attr;
		}
		
		// First argument: setup
		else if (attr_cond == nullptr){
			if (attr_setup != nullptr){
				HERE(error_duplicate_attr(op, *attr_setup, *attr));
				return 0;
			} else if (attr->options % NodeOptions::SINGLE_QUOTE == false){
				HERE(warn_attr_double_quote(op, *attr));
			}
			attr_setup = attr;
		}
		
		// Third argument: increment
		else {
			if (attr_inc != nullptr){
				HERE(error_duplicate_attr(op, *attr_inc, *attr));
				return 0;
			} else if (attr->options % NodeOptions::SINGLE_QUOTE == false){
				HERE(warn_attr_double_quote(op, *attr));
			}
			attr_inc = attr;
		}
		
		continue;
	}
	
	
	// Parse expressions
	pExpr expr_setup = nullptr;
	pExpr expr_cond = nullptr;
	pExpr expr_inc = nullptr;
	
	if (attr_setup != nullptr){
		expr_setup = Parser().parse(attr_setup->value());
		if (expr_setup == nullptr){
			HERE(error_expression_parse(op, *attr_cond));
			return 0;
		}
	}
	
	if (attr_cond != nullptr){
		expr_cond = Parser().parse(attr_cond->value());
		if (expr_cond == nullptr){
			HERE(error_expression_parse(op, *attr_cond));
			return 0;
		}
	} else {
		HERE(error_missing_attr(op, "TRUE"));
		return 0;
	}
	
	if (attr_inc != nullptr){
		expr_inc = Parser().parse(attr_inc->value());
		if (expr_inc == nullptr){
			HERE(error_expression_parse(op, *attr_cond));
			return 0;
		}
	}
	
	
	// Run setup
	if (expr_setup != nullptr){
		variables[attr_setup->name()] = expr_setup->eval(variables);
	}
	
	// Run loop
	const auto _interp_2 = MacroEngine::currentInterpolation; 
	long i = 0;
	
	assert(expr_cond != nullptr);
	while (boolEval(expr_cond->eval(variables)) == cond_expected){
		MacroEngine::currentInterpolation = _interp_2;
		runChildren(op, dst);
		
		// Increment
		if (expr_inc != nullptr){
			variables[attr_inc->name()] = expr_inc->eval(variables);
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
			} else if (attr->options % NodeOptions::SINGLE_QUOTE == false){
				HERE(warn_attr_double_quote(op, *attr));
			}
			
			attr_cond = attr;
		}
		
		else {
			HERE(warn_ignored_attribute(op, *attr));
		}
		
		continue;
	}
	
	
	// Parse expressions
	pExpr expr_cond = nullptr;
	
	if (attr_cond != nullptr){
		expr_cond = Parser().parse(attr_cond->value());
		if (expr_cond == nullptr){
			HERE(error_expression_parse(op, *attr_cond));
			return 0;
		}
	} else {
		HERE(error_missing_attr(op, "TRUE"));
		return 0;
	}
	
	
	// Run
	const auto _interp_2 = MacroEngine::currentInterpolation;
	long i = 0;
	
	assert(expr_cond != nullptr);
	while (boolEval(expr_cond->eval(variables)) == cond_expected){
		MacroEngine::currentInterpolation = _interp_2;
		runChildren(op, dst);
		i++;
	}
	
	
	// Restore state
	MacroEngine::currentInterpolation = _interp;
	return i;
}


// ------------------------------------------------------------------------------------------ //