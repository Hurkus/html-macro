#include "MacroEngine.hpp"
#include "DEBUG.hpp"

#include <iostream>

using namespace std;
using namespace pugi;


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::call(const xml_node op, xml_node dst){
	xml_attribute attr = op.attribute("NAME");
	if (attr.empty()){
		WARNING_L1("CALL: Missing 'NAME' attribute.");
		return true;
	}
	
	const char* cname = attr.value();
	Macro* macro = getMacro(cname);
	
	if (macro != nullptr){
		run(*macro, dst);
	} else {
		WARNING_L1("CALL: Macro '%s' not found.", cname);
	}
	
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


xml_node MacroEngine::tag(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	dst = dst.append_child(op.name());
	
	// Resolve attributes
	for (const xml_attribute attr : op.attributes()){
		
		const char* cname = op.name();
		if (isupper(cname[0])){
			string_view name = cname;
			WARNING_L1("Macro: Unknown macro '%s' treated as regular HTML tag.", cname);
		}
		
		// Regular attribute
		dst.append_copy(attr);
	}
	
	// Resolve children
	for (const xml_node opc : op.children()){
		resolve(opc, dst);
	}
	
	return dst;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::resolve(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	
	// Not tag
	if (op.type() != xml_node_type::node_element){
		dst.append_copy(op);
		return true;
	}
	
	// Check for macro
	const char* cname = op.name();
	if (isupper(cname[0])){
		string_view name = cname;
		
		if (name == "CALL"){
			return call(op, dst);
		} else {
			WARNING_L1("Macro: Unknown macro '%s' treated as regular HTML tag.", cname);
		}
		
	}
	
	// Regular tag
	tag(op, dst);
	return true;
}


bool MacroEngine::run(const Macro& macro, xml_node dst){
	const xml_node mroot = macro.root.root();
	
	// cout << "[" << ANSI_GREEN;
	// mroot.print(cout);
	// cout << ANSI_RESET << "]";
	
	bool res = true;
	for (const xml_node mchild : mroot.children()){
		res &= resolve(mchild, dst);
	}
	
	return res;
}


// ------------------------------------------------------------------------------------------ //