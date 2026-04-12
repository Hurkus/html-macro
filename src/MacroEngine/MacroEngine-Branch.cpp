#include "MacroEngine.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::branch_if(const Node& op, Node& dst){
	assert(macro != nullptr);
	
	// Chain of `&&` statements
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		assert(!name.empty());
		
		// Check if expression evaluates to `true`
		if (name == "TRUE"){
			if (!eval_attr_true(op, *attr))
				goto fail;
			continue;
		}
		
		// Check if expression evaluates to `false`
		else if (name == "FALSE"){
			if (!eval_attr_false(op, *attr))
				goto fail;
			continue;
		}
		
		// Compare attribute to variable
		switch (attr_equals_variable(op, *attr)){
			case Branch::NONE:
			case Branch::FAILED:
				goto fail;
			case Branch::PASSED:
				continue;
		}
		
	}
	
	// pass:
	{
		Node* _last = dst.child;
		evalChildren(op, dst);
		transfer_parent_space(op, dst, _last);
		
		MacroEngine::currentBranch_block = Branch::PASSED;
		return;
	}
	
	fail:
	MacroEngine::currentBranch_block = Branch::FAILED;
}


void MacroEngine::branch_elif(const Node& op, Node& dst){
	assert(macro != nullptr);
	
	switch (MacroEngine::currentBranch_block){
		case Branch::NONE:
			HERE(error_missing_preceeding_if_node(*macro, op));
		case Branch::PASSED:
			return;
		case Branch::FAILED:
			branch_if(op, dst);
	}
	
}


void MacroEngine::branch_else(const Node& op, Node& dst){
	assert(macro != nullptr);
	
	switch (MacroEngine::currentBranch_block){
		case Branch::NONE:
			HERE(error_missing_preceeding_if_node(*macro, op));
		case Branch::PASSED:
			MacroEngine::currentBranch_block = Branch::NONE;
			return;
		case Branch::FAILED:
			break;
	}
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		HERE(warn_ignored_attr(*macro, *attr));
	}
	
	// Run
	evalChildren(op, dst);
	MacroEngine::currentBranch_block = Branch::NONE;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


long MacroEngine::loop_for(const Node& op, Node& dst){
	assert(macro != nullptr);
	assert(variables != nullptr);
	
	bool cond_expected = true;
	const Attr* attr_setup = nullptr;
	const Attr* attr_cond = nullptr;
	const Attr* attr_inc = nullptr;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *attr)){
			case Branch::FAILED: return 0;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		// Second argument: condition
		if (name == "TRUE" || name == "FALSE"){
			cond_expected = (name[0] != 'F');
			
			if (attr_cond != nullptr){
				HERE(error_duplicate_attr(*macro, *attr_cond, *attr));
				return 0;
			}
			
			attr_cond = attr;
		}
		
		// First argument: setup
		else if (attr_cond == nullptr){
			if (attr_setup != nullptr){
				HERE(error_duplicate_attr(*macro, *attr_setup, *attr));
				return 0;
			}
			attr_setup = attr;
		}
		
		// Third argument: increment
		else {
			if (attr_inc != nullptr){
				HERE(error_duplicate_attr(*macro, *attr_inc, *attr));
				return 0;
			}
			attr_inc = attr;
		}
		
		continue;
	}
	
	// Parse expressions
	Expression expr_setup = {};
	Expression expr_cond = {};
	Expression expr_inc = {};
	
	if (attr_setup != nullptr){
		if (attr_setup->options % NodeOptions::SINGLE_QUOTE == false){
			HERE(warn_expected_attr_single_quote(*macro, *attr_setup));
		}
		
		expr_setup = Expression::parse(attr_setup->value(), macro);
		if (!expr_setup){
			return 0;
		}
	}
	
	if (attr_cond != nullptr){
		if (attr_cond->options % NodeOptions::SINGLE_QUOTE == false){
			HERE(warn_expected_attr_single_quote(*macro, *attr_cond));
		}
		
		expr_cond = Expression::parse(attr_cond->value(), macro);
		if (!expr_cond){
			return 0;
		}
		
	} else {
		HERE(error_missing_attr(*macro, op, "TRUE"));
		return 0;
	}
	
	if (attr_inc != nullptr){
		if (attr_inc->options % NodeOptions::SINGLE_QUOTE == false){
			HERE(warn_expected_attr_single_quote(*macro, *attr_inc));
		}
		
		expr_inc = Expression::parse(attr_inc->value(), macro);
		if (!expr_inc){
			return 0;
		}
		
	}
	
	// Run setup
	if (expr_setup){
		variables->insert(attr_setup->name(), expr_setup.eval(*variables));
	}
	
	// Run loop
	long i = 0;
	while (expr_cond.eval(*variables).getBool() == cond_expected){
		evalChildren(op, dst);
		
		// Increment
		if (expr_inc){
			variables->insert(attr_inc->name(), expr_inc.eval(*variables));
		}
		
		i++;
	}
	
	// Restore state
	return i;
}


long MacroEngine::loop_while(const Node& op, Node& dst){
	assert(macro != nullptr);
	assert(variables != nullptr);
	
	bool cond_expected = true;
	const Attr* attr_cond = nullptr;
	
	// Parse attributes
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *attr)){
			case Branch::FAILED: return 0;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		if (name == "TRUE" || name == "FALSE"){
			cond_expected = (name[0] == 'T');
			
			if (attr_cond != nullptr){
				HERE(error_duplicate_attr(*macro, *attr_cond, *attr));
				return 0;
			}
			
			attr_cond = attr;
		}
		
		else {
			HERE(warn_ignored_attr(*macro, *attr));
		}
		
		continue;
	}
	
	// Parse expressions
	Expression expr_cond = {};
	
	if (attr_cond != nullptr){
		if (attr_cond->options % NodeOptions::SINGLE_QUOTE == false){
			HERE(warn_expected_attr_single_quote(*macro, *attr_cond));
		}
		
		expr_cond = Expression::parse(attr_cond->value(), macro);
		if (!expr_cond){
			return 0;
		}
		
	} else {
		HERE(error_missing_attr(*macro, op, "TRUE"));
		return 0;
	}
	
	// Run
	long i = 0;
	while (expr_cond.eval(*variables).getBool() == cond_expected){
		evalChildren(op, dst);
		i++;
	}
	
	// Restore state
	return i;
}


// ------------------------------------------------------------------------------------------ //