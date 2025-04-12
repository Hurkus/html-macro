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
		string_view name = attr.name();
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
		variables.emplace(name, move(v));
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::branch_if(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	
	using namespace Expression;
	Parser parser = {};
	bool pass = true;
	
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
			pass = false;
			break;
		}
		
		Value val = expr->eval(variables);
		if (Expression::boolEval(val) != expected){
			pass = false;
			break;
		}
		
	}
	
	if (pass){
		for (const xml_node child_op : op.children()){
			this->branch = true;	// Each child gets a fresh result.
			resolve(child_op, dst);
		}
	}
	
	this->branch = pass;
	return pass;
}


bool MacroEngine::branch_elif(const xml_node op, xml_node dst){
	if (!this->branch)
		return branch_if(op, dst);
	return false;
}


bool MacroEngine::branch_else(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	if (this->branch){
		return false;
	}
	
	for (const xml_node child_op : op.children()){
		this->branch = true;	// Each child gets a fresh result.
		resolve(child_op, dst);
	}
	
	this->branch = true;
	return true;
}


// ------------------------------------------------------------------------------------------ //