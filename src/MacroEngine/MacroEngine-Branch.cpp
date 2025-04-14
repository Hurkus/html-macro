#include "MacroEngine.hpp"
#include "ExpressionParser.hpp"
#include "MacroEngine-Common.hpp"

using namespace std;
using namespace pugi;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::set(const xml_node op){
	Expression::Parser parser = {};
	
	for (const xml_attribute attr : op.attributes()){
		string_view value = attr.value();
		if (value.empty()){
			continue;
		}
		
		pExpr expr = parser.parse(value);
		if (expr == nullptr){
			WARNING_L1("SET: Failed to parse expression [%s].", attr.value());
			continue;
		}
		
		variables[attr.name()] = expr->eval(variables);
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::branch_if(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	
	Parser parser = {};
	bool _interp = this->interpolateText;
	bool hasExpr = false;
	
	auto __eval = [&](const xml_attribute attr, bool expected){
		hasExpr = true;
		
		pExpr expr = parser.parse(attr.value());
		if (expr == nullptr){
			ERROR_L1("%s: Invalid expression in attribute [%s=\"%s\"].", op.name(), attr.name(), attr.value());
			return false;
		}
		
		Value val = expr->eval(variables);
		return Expression::boolEval(val) == expected;
	};
	
	// Chain of `&&` statements
	for (const xml_attribute attr : op.attributes()){
		string_view name = attr.name();
		
		if (name == "TRUE"){
			if (!__eval(attr, true))
				goto fail;
		} else if (name == "FALSE"){
			if (!__eval(attr, false))
				goto fail;
		} else if (name == "INTERPOLATE"){
			_attr_interpolate(*this, op, attr, _interp);
		} else {
			_attr_ignore(op, attr);
		}
		
	}
	
	if (hasExpr){
		swap(this->interpolateText, _interp);
		runChildren(op, dst);
		this->interpolateText = _interp;
		this->branch = true;
		return true;
	} else {
		WARNING_L1("IF: Missing any TRUE or FALSE expression attributes.");
		goto fail;
	}
	
	fail:
	this->branch = false;
	return false;
}


bool MacroEngine::branch_elif(const xml_node op, xml_node dst){
	if (this->branch.empty()){
		WARNING_L1("ELSE-IF: Missing preceding IF tag.");
		return false;
	} else if (this->branch == true){
		return false;
	} else {
		return branch_if(op, dst);
	}
}


bool MacroEngine::branch_else(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	
	if (branch.empty()){
		WARNING_L1("ELSE: Missing preceding IF tag.");
		return false;
	} else if (branch == true){
		this->branch = nullptr;
		return false;
	}
	
	bool _interp = this->interpolateText;
	
	for (xml_attribute attr : op.attributes()){
		if (attr.name() == "INTERPOLATE"sv)
			_attr_interpolate(*this, op, attr, _interp);
		else
			_attr_ignore(op, attr);
	}
	
	// Run
	swap(this->interpolateText, _interp);
	runChildren(op, dst);
	this->interpolateText = _interp;
	this->branch = nullptr;
	
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


int MacroEngine::loop_for(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	bool _interpolate = this->interpolateText;
	xml_attribute setup_attr;
	xml_attribute cond_attr;
	xml_attribute inc_attr;
	
	for (const xml_attribute attr : op.attributes()){
		string_view name = attr.name();
		
		if (name == "IF"){
			if (!_attr_if(*this, op, attr))
				return 0;
		}
		else if (name == "TRUE" || name == "FALSE"){
			if (cond_attr.empty())
				cond_attr = attr;
			else
				WARNING_L1("FOR: Duplicate condition attribute [%s=\"%s\"] and [%s=\"%s\"].", cond_attr.name(), cond_attr.value(), attr.name(), attr.value());
		}
		else if (name == "INTERPOLATE"){
			_attr_interpolate(*this, op, attr, _interpolate);
		}
		else if (cond_attr.empty()){
			if (setup_attr.empty())
				setup_attr = attr;
			else
				WARNING_L1("FOR: Duplicate setup attribute [%s=\"%s\"] and [%s=\"%s\"].", setup_attr.name(), setup_attr.value(), attr.name(), attr.value());
		}
		else {
			if (inc_attr.empty())
				inc_attr = attr;
			else
				WARNING_L1("FOR: Duplicate increment attribute [%s=\"%s\"] and [%s=\"%s\"].", inc_attr.name(), inc_attr.value(), attr.name(), attr.value());
		}
		
		continue;
	}
	
	
	// Parse expressions
	Expression::Parser parser = {};
	pExpr setup_expr = nullptr;
	pExpr cond_expr = nullptr;
	pExpr inc_expr = nullptr;
	
	if (!cond_attr.empty()){
		cond_expr = parser.parse(cond_attr.value());
		if (cond_expr == nullptr){
			ERROR_L1("FOR: Invalid expression in attribute [%s=\"%s\"].", cond_attr.name(), cond_attr.value());
			return 0;
		}
	} else {
		ERROR_L1("FOR: Missing loop condition attribute [TRUE=<expr>] or [FALSE=<expr>].");
		return 0;
	}
	
	if (!setup_attr.empty()){
		setup_expr = parser.parse(setup_attr.value());
		if (setup_expr == nullptr){
			ERROR_L1("FOR: Ignored invalid expression in attribute [%s=\"%s\"].", setup_attr.name(), setup_attr.value());
			return 0;
		}
	}
	
	if (!inc_attr.empty()){
		inc_expr = parser.parse(inc_attr.value());
		if (inc_expr == nullptr){
			ERROR_L1("FOR: Ignored invalid expression in attribute [%s=\"%s\"].", inc_attr.name(), inc_attr.value());
			return 0;
		}
	}
	
	
	// Run
	if (setup_expr != nullptr){
		variables[setup_attr.name()] = setup_expr->eval(variables);
	}
	
	const bool _interpolate2 = this->interpolateText; 
	int i = 0;
	
	assert(cond_expr != nullptr);
	while (boolEval(cond_expr->eval(variables))){
		this->interpolateText = _interpolate;
		this->runChildren(op, dst);
		
		if (inc_expr != nullptr){
			variables[inc_attr.name()] = inc_expr->eval(variables);
		}
		
		i++;
	}
	
	this->interpolateText = _interpolate2;
	return i;
}


int MacroEngine::loop_while(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	bool _interpolate = this->interpolateText;
	xml_attribute cond_attr;
	
	for (const xml_attribute attr : op.attributes()){
		string_view name = attr.name();
		
		if (name == "IF"){
			if (!_attr_if(*this, op, attr))
				return 0;
		}
		else if (name == "TRUE" || name == "FALSE"){
			if (cond_attr.empty())
				cond_attr = attr;
			else
				WARNING_L1("WHILE: Duplicate condition attribute [%s=\"%s\"] and [%s=\"%s\"].", cond_attr.name(), cond_attr.value(), attr.name(), attr.value());
		}
		else if (name == "INTERPOLATE"){
			_attr_interpolate(*this, op, attr, _interpolate);
		}
		else {
			WARNING_L1("WHILE: Ignored unknown macro attribute [%s=\"%s\"].", attr.name(), attr.value());
		}
		
		continue;
	}
	
	
	// Parse expressions
	Expression::Parser parser = {};
	pExpr cond_expr = nullptr;
	
	if (!cond_attr.empty()){
		cond_expr = parser.parse(cond_attr.value());
		if (cond_expr == nullptr){
			ERROR_L1("WHILE: Invalid expression in attribute [%s=\"%s\"].", cond_attr.name(), cond_attr.value());
			return 0;
		}
	} else {
		ERROR_L1("WHILE: Missing loop condition attribute [TRUE=<expr>] or [FALSE=<expr>].");
		return 0;
	}
	
	
	// Run
	const bool _interpolate2 = this->interpolateText;
	int i = 0;
	
	assert(cond_expr != nullptr);
	while (boolEval(cond_expr->eval(variables))){
		this->interpolateText = _interpolate;
		this->runChildren(op, dst);
		i++;
	}
	
	this->interpolateText = _interpolate2;
	return i;
}


// ------------------------------------------------------------------------------------------ //