#include "MacroEngine.hpp"
#include <cmath>

#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Variables ] ---------------------------------------- //


VariableMap MacroEngine::variables;

Branch MacroEngine::currentBranch_block = Branch::NONE;
Branch MacroEngine::currentBranch_inline = Branch::NONE;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::setVariableConstants(){
	MacroEngine::variables["false"] = 0L;
	MacroEngine::variables["true"] = 1L;
	MacroEngine::variables["pi"] = M_PI;
}


constexpr bool isUpperCase(char c){
	return 'A' <= c && c <= 'Z';
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
		
		default:
			HERE(error_unsupported_type(op));
			return;
	}
	
	string_view name = op.name();
	if (name.empty()){
		assert(!name.empty());
		return;
	}
	
	// Regular tag
	else if (!isUpperCase(name[0])){ regular_tag:
		tag(op, dst);
		return;
	}
	
	// Macro tag
	switch (name.length()){
		case 2: {
			if (name == "IF")
				branch_if(op, dst);
			else
				goto unknown;
		} break;
		
		case 3: {
			if (name == "SET")
				set(op);
			else if (name == "FOR")
				loop_for(op, dst);
			else
				goto unknown;
		} break;
			
		case 4: {
			if (name == "CALL")
				call(op, dst);
			else if (name == "ELIF")
				branch_elif(op, dst);
			else if (name == "ELSE")
				branch_else(op, dst);
			else if (name == "INFO")
				info(op);
			else if (name == "WARN")
				warn(op);
			else
				goto unknown;
		} break;
		
		case 5: {
			if (name == "SHELL")
				shell(op, dst);
			else if (name == "WHILE")
				loop_while(op, dst);
			else if (name == "ERROR")
				error(op);
			else
				goto unknown;
		} break;
		
		case 7: {
			if (name == "INCLUDE")
				include(op, dst);
			else if (name == "GET-TAG")
				getTag(op, dst);
			else if (name == "SET-TAG")
				setTag(op, dst);
			else
				goto unknown;
		} break;
		
		case 8: {
			if (name == "SET-ATTR")
				setAttr(op, dst);
			else if (name == "GET-ATTR")
				getAttr(op, dst);
			else if (name == "DEL-ATTR")
				delAttr(op, dst);
			else
				goto unknown;
		} break;
		
		default: { unknown:
			HERE(warn_unknown_macro_tag(op));
			goto regular_tag;
		} break;
		
	}
	
	return;
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
	if (macro.html == nullptr){
		assert(macro.html != nullptr);
		return;
	}
	
	// Backup state
	auto _branch_1 = MacroEngine::currentBranch_block;
	auto _branch_2 = MacroEngine::currentBranch_inline;
	auto _cwd = move(Paths::cwd);
	
	// Set new state
	if (macro.srcDir != nullptr){
		Paths::cwd = macro.srcDir;
	} else {
		assert(macro.srcDir != nullptr);
		Paths::cwd = _cwd;
	}
	
	MacroEngine::currentBranch_block = Branch::NONE;
	MacroEngine::currentBranch_inline = Branch::NONE;
	
	runChildren(*macro.html, dst);
	
	// Restore state
	Paths::cwd = move(_cwd);
	MacroEngine::currentBranch_inline = _branch_2;
	MacroEngine::currentBranch_block = _branch_1;
}


// ------------------------------------------------------------------------------------------ //