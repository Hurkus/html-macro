#include "MacroEngine.hpp"
#include "Debug.hpp"

#include <cmath>

using namespace std;
using namespace html;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::setVariableConstants(){
	MacroEngine::variables->insert("null", nullptr);
	MacroEngine::variables->insert("false", 0L);
	MacroEngine::variables->insert("true", 1L);
	MacroEngine::variables->insert("pi", M_PI);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::set(const Node& op){
	assert(macro != nullptr);
	assert(variables != nullptr);
	
	if (op.child != nullptr){
		HERE(warn_ignored_child(*macro, op));
	}
	
	string buff;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		string_view value = attr->value();
		assert(!name.empty());
		
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		// Expression
		if (attr->options % NodeOptions::SINGLE_QUOTE){
			Expression expr = Expression::parse(value, macro);
			if (!expr){
				return;
			}
			
			variables->insert(name, expr.eval(*variables));
		}
		
		// Interpolate
		else if (attr->options % NodeOptions::INTERPOLATE){
			buff.clear();
			if (eval_string_interpolate(value, buff))
				variables->insert(name, move(buff));
		}
		
		// Plain text
		else {
			variables->insert(name, value);
		}
		
	}
	
	return;
}


// ------------------------------------------------------------------------------------------ //