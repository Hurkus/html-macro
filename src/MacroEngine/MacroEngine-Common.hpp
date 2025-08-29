#pragma once
#include "MacroEngine.hpp"
#include "ExpressionParser.hpp"
#include "Debug.hpp"


// ----------------------------------- [ Functions ] ---------------------------------------- //


static optbool _cond(const MacroEngineObject& self, const char* exprstr){
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


inline bool _attr_if(const MacroEngineObject& self, const pugi::xml_node& op, const pugi::xml_attribute& attr){
	optbool b = _cond(self, attr.value());
	
	if (b.empty()){
		WARN("%s: Invalid expression in attribute [%s=\"%s\"]. Defaulting to false.", op.name(), attr.name(), attr.value());
		return false;
	}
	
	return b.get();
}


inline void _attr_interpolate(const MacroEngineObject& self, const pugi::xml_node& op, const pugi::xml_attribute& attr, bool& out){
	optbool b = _cond(self, attr.value());
			
	if (b.hasValue()){
		out = b.get();
	} else {
		WARN("%s: Ignored invalid expression in attribute [%s=\"%s\"].", op.name(), attr.name(), attr.value());
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline void _attr_ignore(const pugi::xml_node& op, const pugi::xml_attribute& attr){
	WARN("%s: Ignored attribute '%s'.", op.name(), attr.name());
}


inline void _attr_missing(const pugi::xml_node& op, const char* name){
	ERROR("%s: Missing attribute '%s'.", op.name(), name);
}


inline void _attr_duplicate(const pugi::xml_node& op, const pugi::xml_attribute& a1, const pugi::xml_attribute& a2){
	WARN("%s: Duplicate attribute [%s=\"%s\"] and [%s=\"%s\"].", op.name(), a1.name(), a1.value(), a2.name(), a2.value());
}


// ------------------------------------------------------------------------------------------ //