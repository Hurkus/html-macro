#include "MacroEngine.hpp"
#include <cmath>
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Variables ] ---------------------------------------- //


Expression::VariableMap MacroEngine::variables;
Document MacroEngine::doc;

const Macro* MacroEngine::currentMacro = nullptr;
Branch MacroEngine::currentBranch_block = Branch::NONE;
Branch MacroEngine::currentBranch_inline = Branch::NONE;
Interpolate MacroEngine::currentInterpolation = Interpolate::ALL;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::setVariableConstants(){
	MacroEngine::variables["false"] = 0L;
	MacroEngine::variables["true"] = 1L;
	MacroEngine::variables["pi"] = M_PI;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::run(const Node& op, Node& dst){
	switch (op.type){
		case NodeType::TAG:
			break;
		
		case NodeType::ROOT:
			runChildren(op, dst);
			return;
		
		case NodeType::PI:
		case NodeType::DIRECTIVE:
		case NodeType::TEXT:
		case NodeType::COMMENT:
			text(op, dst);
			return;
		
		default: return;
	}
	
	// Check for macro
	string_view name = op.name();
	if (name.length() >= 1 && isupper(name[0])){
		
		if (name == "SET"){
			set(op);
		} else if (name == "IF"){
			branch_if(op, dst);
		} else if (name == "ELSE-IF"){
			branch_elif(op, dst);
		} else if (name == "ELSE"){
			branch_else(op, dst);
		} else if (name == "FOR"){
			loop_for(op, dst);
		} else if (name == "WHILE"){
			loop_while(op, dst);
		}
		
		// else if (name == "SET-ATTR"){
		// 	setAttr(op, dst);
		// } else if (name == "GET-ATTR"){
		// 	getAttr(op, dst);
		// } else if (name == "DEL-ATTR"){
		// 	delAttr(op, dst);
		// } else if (name == "SET-TAG"){
		// 	setTag(op, dst);
		// } else if (name == "GET-TAG"){
		// 	getTag(op, dst);
		// }
		
		if (name == "CALL"){
			call(op, dst);
		}
		// else if (name == "INCLUDE"){
		// 	include(op, dst);
		// }  else if (name == "SHELL"){
		// 	shell(op, dst);
		// }
		
		else if (name == "INFO"){
			info(op);
		} else if (name == "WARN"){
			warn(op);
		} else if (name == "ERROR"){
			error(op);
		}
		
		else {
			warn_unknown_macro(op);
			tag(op, dst);
		}
		
		return;
	}
	
	// Regular tag
	tag(op, dst);
}


void MacroEngine::runChildren(const Node& macroParent, Node& dst){
	auto _branch_1 = MacroEngine::currentBranch_block;
	auto _branch_2 = MacroEngine::currentBranch_inline;
	MacroEngine::currentBranch_block = Branch::NONE;
	MacroEngine::currentBranch_inline = Branch::NONE;
	
	const Node* macroChild = macroParent.child;
	while (macroChild != nullptr){
		run(*macroChild, dst);
		macroChild = macroChild->next;
	}
	
	MacroEngine::currentBranch_block = _branch_1;
	MacroEngine::currentBranch_inline = _branch_2;
}


void MacroEngine::exec(const Macro& macro, Node& dst){
	auto _macro = MacroEngine::currentMacro;
	auto _branch_1 = MacroEngine::currentBranch_block;
	auto _branch_2 = MacroEngine::currentBranch_inline;
	auto _interp = MacroEngine::currentInterpolation;
	
	MacroEngine::currentMacro = &macro;
	MacroEngine::currentBranch_block = Branch::NONE;
	MacroEngine::currentBranch_inline = Branch::NONE;
	MacroEngine::currentInterpolation = Interpolate::ALL;
	
	runChildren(macro.doc, dst);
	
	MacroEngine::currentMacro = _macro;
	MacroEngine::currentBranch_block = _branch_1;
	MacroEngine::currentBranch_inline = _branch_2;
	MacroEngine::currentInterpolation = _interp;
}


// ------------------------------------------------------------------------------------------ //