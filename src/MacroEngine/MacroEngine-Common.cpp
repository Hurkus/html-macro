#include "MacroEngine.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Functions ] ---------------------------------------- //


Branch MacroEngine::check_attr_if(const Node& op, const Attr& attr){
	string_view name = attr.name();
	
	switch(name.length()){
		case 2:
			if (name == "IF")
				goto _if;
			else
				return Branch::NONE;
		case 4:
			if (name == "ELIF")
				goto _elif;
			else if (name == "ELSE")
				goto _else;
			else
				return Branch::NONE;
		default:
			return Branch::NONE;
	} assert(false);
	
	// -------------------------------- //
	_else:
	if (attr.value_len > 0){
		HERE(warn_ignored_attr_value(op, attr));
	}
	
	switch (MacroEngine::currentBranch_inline){
		case Branch::FAILED:
			MacroEngine::currentBranch_inline = Branch::NONE;
			return Branch::PASSED;
		case Branch::PASSED:
			return Branch::FAILED;
		case Branch::NONE:
			HERE(error_missing_preceeding_if_attr(op, attr));
			return Branch::FAILED;
	} assert(false);
	
	// -------------------------------- //
	_elif:
	switch (MacroEngine::currentBranch_inline){
		case Branch::FAILED:
			goto _if;
		case Branch::PASSED:
			return Branch::FAILED;
		case Branch::NONE:
			HERE(error_missing_preceeding_if_attr(op, attr));
			return Branch::FAILED;
	} assert(false);
	
	// -------------------------------- //
	
	_if:
	string_view expr_str = attr.value();
	
	// Check and warn
	if (expr_str.empty()){
		HERE(warn_missing_attr_value(op, attr));
		return MacroEngine::currentBranch_inline = Branch::FAILED;
	} else if (!(attr.options % NodeOptions::SINGLE_QUOTE)){
		HERE(warn_attr_double_quote(op, attr));
	}
	
	const NodeDebugger dbg = NodeDebugger(op);
	const Expression expr = Expression::parse(expr_str, dbg);
	if (expr == nullptr){
		return MacroEngine::currentBranch_inline = Branch::FAILED;
	}
	
	const bool b = expr.eval(MacroEngine::variables, dbg).toBool();
	return MacroEngine::currentBranch_inline = b ? Branch::PASSED : Branch::FAILED;
}


static bool eval_attr_bool(const Node& op, const Attr& attr, bool value){
	string_view expr_str = attr.value();
	
	if (expr_str.empty()){
		HERE(warn_missing_attr_value(op, attr));
		return false;
	} else if (!(attr.options % NodeOptions::SINGLE_QUOTE)){
		HERE(warn_attr_double_quote(op, attr));
	}
	
	Expression expr = Expression::parse(expr_str, NodeDebugger(op));
	if (expr == nullptr){
		return false;
	}
	
	Value val = expr.eval(MacroEngine::variables, NodeDebugger(op));
	return val.toBool() == value;
}

bool MacroEngine::eval_attr_true(const Node& op, const Attr& attr){
	return eval_attr_bool(op, attr, true);
}

bool MacroEngine::eval_attr_false(const Node& op, const Attr& attr){
	return eval_attr_bool(op, attr, false);
}


bool MacroEngine::eval_attr_value(const html::Node& op, const html::Attr& attr, Value& out_result){
	if (attr.value_p == nullptr){
		return false;
	}
	
	// Evaluate expression
	else if (attr.options % NodeOptions::SINGLE_QUOTE){
		const NodeDebugger dbg = NodeDebugger(op);
		
		Expression expr = Expression::parse(attr.value(), dbg);
		if (expr == nullptr){
			return false;
		}
		
		out_result = expr.eval(MacroEngine::variables, dbg);
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
		const NodeDebugger dbg = NodeDebugger(op);
		
		Expression expr = Expression::parse(attr.value(), dbg);
		if (expr == nullptr){
			return false;
		}
		
		expr.eval(MacroEngine::variables, dbg).toStr(buff);
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


Interpolate MacroEngine::eval_attr_interp(const Node& op, const Attr& attr){
	string_view expr_str = attr.value();
	if (expr_str.empty()){
		HERE(warn_missing_attr_value(op, attr));
		return Interpolate::NONE;
	}
	
	Expression expr = Expression::parse(expr_str, NodeDebugger(op));
	if (expr == nullptr){
		return Interpolate::NONE;
	}
		
	Value res = expr.eval(MacroEngine::variables, NodeDebugger(op));
	return res.toBool() ? Interpolate::ALL : Interpolate::NONE;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::eval_string_interpolate(const Node& op, string_view str, string& buff){
	const char* const end = str.end();
	const char* beg = str.begin();
	const NodeDebugger dbg = NodeDebugger(op);
	
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
				HERE(error_newline(op, b));
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
		Expression expr = Expression::parse(string_view(a+1, b), dbg);
		
		if (expr != nullptr){
			expr.eval(MacroEngine::variables, dbg).toStr(buff);
		} else {
			return false;
		}
		
		beg = b+1;
	}
	
	return true;
}


// ------------------------------------------------------------------------------------------ //