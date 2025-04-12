#include "MacroEngine.hpp"
#include "ExpressionParser.hpp"
#include "DEBUG.hpp"

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
		
		Value v = expr->eval(variables);
		variables.emplace(attr.name(), move(v));
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::branch_if(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	
	Parser parser = {};
	bool hasExpr = false;
	
	// Chain of `&&` statements
	for (const xml_attribute attr : op.attributes()){
		string_view name = attr.name();
		bool expected;
		
		if (name == "TRUE"){
			expected = true;
		} else if (name == "FALSE"){
			expected = false;
		} else {
			WARNING_L1("IF: Ignored attribute '%s'.", attr.name());
			continue;
		}
		
		pExpr expr = parser.parse(attr.value());
		if (expr == nullptr){
			WARNING_L1("IF: Invalid expression [%s].", attr.value());
			goto fail;
		}
		
		Value val = expr->eval(variables);
		if (Expression::boolEval(val) != expected){
			goto fail;
		}
		
		hasExpr = true;
	}
	
	if (hasExpr){
		runChildren(op, dst);
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
		return false;
	}
	
	runChildren(op, dst);
	this->branch = nullptr;
	return true;
}


// ------------------------------------------------------------------------------------------ //