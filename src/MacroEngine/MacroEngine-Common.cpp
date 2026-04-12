#include "MacroEngine.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;

using Branch = MacroEngine::Branch;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::transfer_parent_space(const Node& op, Node& dst, const Node* dst_original_last){
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


// ----------------------------------- [ Functions ] ---------------------------------------- //


Branch MacroEngine::try_eval_attr_if_elif_else(const Node& op, const Attr& attr){
	assert(macro != nullptr);
	string_view name = attr.name();
	
	if (name == "IF"){
		goto _if;
	} else if (name == "ELIF"){
		goto _elif;
	} else if (name == "ELSE"){
		goto _else;
	} else {
		return Branch::NONE;
	}
	
	// -------------------------------- //
	_else:
	if (attr.value_len > 0){
		HERE(warn_ignored_attr_value(*macro, attr));
	}
	
	switch (MacroEngine::currentBranch_inline){
		case Branch::FAILED:
			MacroEngine::currentBranch_inline = Branch::NONE;
			return Branch::PASSED;
		case Branch::PASSED:
			return Branch::FAILED;
		case Branch::NONE:
			HERE(error_missing_preceeding_if_attr(*macro, op, attr));
			return Branch::FAILED;
	}
	
	// -------------------------------- //
	_elif:
	switch (MacroEngine::currentBranch_inline){
		case Branch::FAILED:
			goto _if;
		case Branch::PASSED:
			return Branch::FAILED;
		case Branch::NONE:
			HERE(error_missing_preceeding_if_attr(*macro, op, attr));
			return Branch::FAILED;
	}
	
	// -------------------------------- //
	
	_if:
	string_view expr_str = attr.value();
	
	// Check and warn
	if (expr_str.empty()){
		HERE(error_missing_attr_value(*macro, attr));
		return MacroEngine::currentBranch_inline = Branch::FAILED;
	} else if (!(attr.options % NodeOptions::SINGLE_QUOTE)){
		HERE(warn_expected_attr_single_quote(*macro, attr));
	}
	
	const Expression expr = Expression::parse(expr_str, macro);
	if (!expr){
		return MacroEngine::currentBranch_inline = Branch::FAILED;
	}
	
	const bool b = expr.eval(*variables).getBool();
	return MacroEngine::currentBranch_inline = (b ? Branch::PASSED : Branch::FAILED);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::eval_attr_bool(const Node& op, const Attr& attr, bool& value){
	assert(macro != nullptr);
	string_view expr_str = attr.value();
	
	if (expr_str.empty()){
		HERE(error_missing_attr_value(*macro, attr));
		return false;
	} else if (!(attr.options % NodeOptions::SINGLE_QUOTE)){
		HERE(warn_expected_attr_single_quote(*macro, attr));
	}
	
	Expression expr = Expression::parse(expr_str, macro);
	if (!expr){
		return false;
	}
	
	value = expr.eval(*variables).getBool();
	return true;
}

bool MacroEngine::eval_attr_true(const Node& op, const Attr& attr){
	bool b;
	return eval_attr_bool(op, attr, b) && b;
}

bool MacroEngine::eval_attr_false(const Node& op, const Attr& attr){
	bool b;
	return eval_attr_bool(op, attr, b) && !b;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::eval_attr_value(const Node& op, const Attr& attr, Value& out_result){
	if (attr.value_p == nullptr){
		return false;
	}
	
	// Evaluate expression
	else if (attr.options % NodeOptions::SINGLE_QUOTE){
		Expression expr = Expression::parse(attr.value(), macro);
		if (!expr){
			return false;
		}
		
		out_result = expr.eval(*variables);
	}
	
	// Interpolate
	else if (attr.options % NodeOptions::INTERPOLATE){
		string buff;
		if (!eval_string_interpolate(op, attr.value(), buff)){
			return false;
		}
		
		out_result = Value(buff);
	}
	
	// Plain text
	else {
		out_result = Value(attr.value());
	}
	
	return true;
}


bool MacroEngine::eval_attr_value(const Node& op, const Attr& attr, string& buff, string_view& result){
	if (attr.value_p == nullptr){
		result = {};
	}
	
	// Evaluate expression
	else if (attr.options % NodeOptions::SINGLE_QUOTE){
		Expression expr = Expression::parse(attr.value(), macro);
		if (!expr){
			return false;
		}
		
		expr.eval(*variables).toStr(buff);
		result = buff;
	}
	
	// Interpolate
	else if (attr.options % NodeOptions::INTERPOLATE){
		if (!eval_string_interpolate(op, attr.value(), buff))
			return false;
		result = buff;
	}
	
	// Plain text
	else {
		result = attr.value();
	}
	
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::eval_string_interpolate(const Node& op, string_view str, string& buff){
	assert(macro != nullptr);
	
	const char* const end = str.end();
	const char* beg = str.begin();
	
	while (beg != end){
		const char* a;	// expr begin
		const char* b;	// expr end
		
		// Find starting poing {
		a = beg;
		while (a != end){
			if (*a == '{'){
				break;
			}
			
			else if (*a == '\\'){
				if (++a == end)
					break;
				buff.append(beg, a - 1);
				beg = a;
			}
			
			a++;
		}
		
		// Find end point }
		b = a;
		while (b != end){
			if (*b == '}'){
				break;
			} else if (*b == '\n'){
				HERE(error_string_interpolation_newline(*macro, b));
				return false;
			}
			b++;
		}
		
		// Append prefix
		if (b == end){
			buff.append(beg, b);
			return true;
		} else {
			buff.append(beg, a);
		}
		
		// Evaluate expression
		assert(*a == '{' && *b == '}');
		Expression expr = Expression::parse(string_view(a+1, b), macro);
		
		if (expr){
			expr.eval(*variables).toStr(buff);
		} else {
			return false;
		}
		
		beg = b+1;
	}
	
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Branch MacroEngine::attr_equals_variable(const Node& op, const Attr& attr){
	string_view var_name = attr.name();
	const Value* var = variables->get(var_name);
	bool pass = false;
	
	// Variable exists or is empty
	if (attr.value_p == nullptr){
		pass = (var != nullptr && var->getBool());
	}
	
	// Evaluate expression
	else if (attr.options % NodeOptions::SINGLE_QUOTE){
		Expression expr = Expression::parse(attr.value(), macro);
		if (!expr){
			return MacroEngine::Branch::NONE;
		}
		
		Value val = expr.eval(*variables);
		pass = (var == nullptr && !val.getBool()) || (*var == val);
	}
	
	// Interpolate
	else if (attr.options % NodeOptions::INTERPOLATE){
		string buff;
		if (!MacroEngine::eval_string_interpolate(op, attr.value(), buff)){
			return MacroEngine::Branch::NONE;
		}
		
		if (var == nullptr){
			pass = buff.empty();
		} else {
			pass = var->semanticEquals(buff);
		}
	
	}
	
	// Plain text
	else {
		string_view buff = attr.value();
		if (var == nullptr)
			pass = buff.empty();
		else
			pass = var->semanticEquals(buff);
	}
	
	return (pass) ? MacroEngine::Branch::PASSED : MacroEngine::Branch::FAILED;
}


// ------------------------------------------------------------------------------------------ //