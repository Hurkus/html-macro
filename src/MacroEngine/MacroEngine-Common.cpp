#include "MacroEngine.hpp"
#include "ExpressionParser.hpp"
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
	}
	
	pExpr expr = Parser().parse(expr_str);
	if (expr == nullptr){
		HERE(error_expression_parse(op, attr));
		return false;
	}
	
	Expression::Value val = expr->eval(MacroEngine::variables);
	return Expression::boolEval(val);
}


static bool eval_attr_bool(const Node& op, const Attr& attr, bool value){
	string_view expr_str = attr.value();
	
	if (expr_str.empty()){
		HERE(warn_missing_attr_value(op, attr));
		return false;
	} else if (!(attr.options % NodeOptions::SINGLE_QUOTE)){
		HERE(warn_attr_double_quote(op, attr));
	}
	
	pExpr expr = Parser().parse(expr_str);
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
		
		pExpr expr = Expression::parse(attr.value());
		if (expr == nullptr){
			HERE(error_expression_parse(op, attr));
			return false;
		}
		
		Expression::str(expr->eval(MacroEngine::variables), buff);
		result = buff;
		return true;
	}
	
	// Interpolate
	else if (attr.options % NodeOptions::INTERPOLATE){
		interpolate(attr.value(), MacroEngine::variables, buff);
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
	
	pExpr expr = Parser().parse(expr_str);
	if (expr == nullptr){
		HERE(error_expression_parse(op, attr));
		return Interpolate::NONE;
	}
		
	Value res = expr->eval(MacroEngine::variables);
	return Expression::boolEval(res) ? Interpolate::ALL : Interpolate::NONE;
}


// ------------------------------------------------------------------------------------------ //