#include "MacroEngine.hpp"
#include <cstring>

#include "ExpressionParser.hpp"
#include "DEBUG.hpp"

using namespace std;
using namespace pugi;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::call(const char* name, xml_node dst){
	if (name == nullptr || name[0] == 0){
		WARNING_L1("CALL: Missing macro name.");
		return;
	}
	
	Macro* macro = getMacro(name);
	if (macro == nullptr){
		WARNING_L1("CALL: Macro '%s' not found.", name);
		return;
	}
	
	exec(*macro, dst);
}


void MacroEngine::call(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	
	xml_attribute attr = op.attribute("NAME");
	if (attr.empty()){
		WARNING_L1("CALL: Missing 'NAME' attribute.");
		return;
	}
	
	call(attr.value(), dst);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::runChildren(const xml_node parent, xml_node dst){
	assert(parent.root() != dst.root());
	auto _branch = this->branch;
	this->branch = nullptr;
	
	for (const xml_node mchild : parent.children()){
		run(mchild, dst);
	}
	
	this->branch = _branch;
}


void MacroEngine::run(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	
	// Not tag
	xml_node_type type = op.type();
	if (type != xml_node_type::node_element){
		if (type == xml_node_type::node_pcdata)
			text(op.value(), dst);
		else
			dst.append_copy(op);
		return;
	}
	
	// Check for macro
	const char* cname = op.name();
	if (isupper(cname[0])){
		string_view name = cname;
		
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
		
		else if (name == "CALL"){
			call(op, dst);
		}  else if (name == "SHELL"){
			shell(op, dst);
		}
		
		else if (name == "INFO"){
			info(op);
		} else if (name == "WARN"){
			warn(op);
		} else if (name == "ERROR"){
			error(op);
		} else {
			WARNING_L1("Macro: Unknown macro '%s' treated as regular HTML tag.", cname);
			goto __tag;
		}
		
		return;
	}
	
	// Regular tag
	__tag:
	tag(op, dst);
}


void MacroEngine::exec(const Macro& macro, xml_node dst){
	const xml_node mroot = macro.root.root();
	
	// cout << "[" << ANSI_GREEN;
	// mroot.print(cout);
	// cout << ANSI_RESET << "]";
	
	runChildren(macro.root, dst);
}


// ------------------------------------------------------------------------------------------ //