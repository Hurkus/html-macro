#include "MacroEngine.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::eval_attr_if(const Node& op, const Attr& attr){
	string_view expr_str = attr.value();
	
	if (expr_str.empty()){
		HERE(warn_missing_attr_value(op, attr));
		return false;
	} else if (!(attr.options % NodeOptions::SINGLE_QUOTE)){
		warn_attr_double_quote(op, attr);
	}
	
	pExpr expr = parse_expr(op, expr_str);
	if (expr == nullptr){
		return false;
	}
	
	bool e = Expression::boolEval(expr->eval(MacroEngine::variables));
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
	
	pExpr expr = Expression::try_parse(expr_str);
	if (expr == nullptr){
		HERE(error_expression_parse(op, attr));
		return false;
	}
	
	Expression::Value val = expr->eval(MacroEngine::variables);
	return Expression::boolEval(val) == value;
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
		return true;
	}
	
	// Evaluate expression
	else if (attr.options % NodeOptions::SINGLE_QUOTE){
		pExpr expr = parse_expr(op, attr.value());
		if (expr == nullptr){
			return false;
		}
		
		Expression::str(expr->eval(MacroEngine::variables), buff);
		result = buff;
		return true;
	}
	
	// Interpolate
	else if (attr.options % NodeOptions::INTERPOLATE){
		eval_string(op, attr.value(), buff);
		result = buff;
		return true;
	}
	
	// Plain text
	else {
		result = attr.value();
		return true;
	}
	
	return false;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Interpolate MacroEngine::eval_attr_interp(const Node& op, const Attr& attr){
	string_view expr_str = attr.value();
	if (expr_str.empty()){
		HERE(warn_missing_attr_value(op, attr));
		return Interpolate::NONE;
	}
	
	pExpr expr = Expression::try_parse(expr_str);
	if (expr == nullptr){
		HERE(error_expression_parse(op, attr));
		return Interpolate::NONE;
	}
		
	Value res = expr->eval(MacroEngine::variables);
	return Expression::boolEval(res) ? Interpolate::ALL : Interpolate::NONE;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Expression::pExpr MacroEngine::parse_expr(const Node& op, string_view str){
	Expression::ParseResult res = Expression::parse(str);
	if (res.status != Expression::ParseStatus::OK){
		HERE(error_expression_parse(op, res));
		return nullptr;
	}
	return move(res.expr);
}


bool MacroEngine::eval_string(const Node& op, string_view str, string& buff){
	const char* const end = str.end();
	const char* beg = str.begin();
	
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
		Expression::pExpr expr = parse_expr(op, string_view(a+1, b));
		
		if (expr != nullptr){
			Expression::str(expr->eval(MacroEngine::variables), buff);
		} else {
			return false;
		}
		
		beg = b+1;
	}
	
	return true;
}


// ------------------------------------------------------------------------------------------ //