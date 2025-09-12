#include "MacroEngine.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::eval_attr_if(const Node& op, const Attr& attr){
	string_view expr_str = attr.value();
	const NodeDebugger dbg = NodeDebugger(op);
	
	if (expr_str.empty()){
		HERE(warn_missing_attr_value(op, attr));
		return false;
	} else if (!(attr.options % NodeOptions::SINGLE_QUOTE)){
		HERE(warn_attr_double_quote(op, attr));
	}
	
	Expression expr = Expression::parse(expr_str, dbg);
	if (expr == nullptr){
		return false;
	}
	
	bool e = toBool(expr.eval(MacroEngine::variables, dbg));
	return e;
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
	return toBool(val) == value;
}

bool MacroEngine::eval_attr_true(const Node& op, const Attr& attr){
	return eval_attr_bool(op, attr, true);
}

bool MacroEngine::eval_attr_false(const Node& op, const Attr& attr){
	return eval_attr_bool(op, attr, false);
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
		
		toStr(expr.eval(MacroEngine::variables, dbg), buff);
		result = buff;
	}
	
	// Interpolate
	else if (attr.options % NodeOptions::INTERPOLATE){
		if (!eval_string(op, attr.value(), buff))
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
	return toBool(res) ? Interpolate::ALL : Interpolate::NONE;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::eval_string(const Node& op, string_view str, string& buff){
	const char* const end = str.end();
	const char* beg = str.begin();
	const NodeDebugger dbg = NodeDebugger(op);
	
	while (beg != end){
		const char* a;
		const char* b;
		
		// Find starting poing {
		a = beg;
		while (a != end && *a != '{') a++;
		
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
			toStr(expr.eval(MacroEngine::variables, dbg), buff);
		} else {
			return false;
		}
		
		beg = b+1;
	}
	
	return true;
}


// ------------------------------------------------------------------------------------------ //