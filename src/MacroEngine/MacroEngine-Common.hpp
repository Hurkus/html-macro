#pragma once
#include "MacroEngine.hpp"
#include "ExpressionParser.hpp"
#include "DEBUG.hpp"


// ----------------------------------- [ Functions ] ---------------------------------------- //


static optbool _cond(const MacroEngine& self, const char* exprstr){
	assert(exprstr != nullptr);
	if (exprstr[0] == 0){
		return nullptr;
	}
	
	Expression::Parser parser = {};
	Expression::pExpr expr = parser.parse(exprstr);
	if (expr == nullptr){
		return nullptr;
	}
	
	Expression::Value val = expr->eval(self.variables);
	return Expression::boolEval(val);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline bool _attr_if(const MacroEngine& self, pugi::xml_node node, pugi::xml_attribute attr){
	optbool b = _cond(self, attr.value());
	
	if (b.empty()){
		WARNING_L1("%s: Invalid expression in attribute [%s=\"%s\"]. Defaulting to false.", node.name(), attr.name(), attr.value());
		return false;
	}
	
	return b.get();
}


inline void _attr_interpolate(const MacroEngine& self, pugi::xml_node node, pugi::xml_attribute attr, bool& out){
	optbool b = _cond(self, attr.value());
			
	if (b.hasValue()){
		out = b.get();
	} else {
		WARNING_L1("%s: Ignored invalid expression in attribute [%s=\"%s\"].", node.name(), attr.name(), attr.value());
	}
	
}


inline void _attr_ignore(pugi::xml_node node, pugi::xml_attribute attr){
	WARNING_L1("%s: Ignored attribute '%s'.", node.name(), attr.name());
}


// ------------------------------------------------------------------------------------------ //