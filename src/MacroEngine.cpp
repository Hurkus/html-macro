#include "MacroEngine.hpp"
#include "ExpressionParser.hpp"
#include "DEBUG.hpp"

using namespace std;
using namespace pugi;


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void _call(MacroEngine& engine, const char* name, xml_node dst){
	if (name == nullptr || name[0] == 0){
		WARNING_L1("CALL: Missing macro name.");
		return;
	}
	
	Macro* macro = engine.getMacro(name);
	if (macro == nullptr){
		WARNING_L1("CALL: Macro '%s' not found.", name);
		return;
	}
	
	engine.run(*macro, dst);
}


void MacroEngine::call(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	
	xml_attribute attr = op.attribute("NAME");
	if (attr.empty()){
		WARNING_L1("CALL: Missing 'NAME' attribute.");
		return;
	}
	
	_call(*this, attr.value(), dst);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


xml_node MacroEngine::tag(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	dst = dst.append_child(op.name());
	
	xml_attribute attr_call = {};
	
	// Copy attributes
	for (const xml_attribute attr : op.attributes()){
		
		const char* cname = attr.name();
		if (isupper(cname[0])){
			string_view name = cname;
			
			if (name == "CALL"){
				attr_call = attr;
				continue;
			} else {
				WARNING_L1("Macro: Unknown macro '%s' treated as regular HTML tag.", cname);
			}
			
		}
		
		// Regular attribute
		dst.append_copy(attr);
	}
	
	if (!attr_call.empty()){
		_call(*this, attr_call.value(), dst);
	}
	
	// Resolve children
	for (const xml_node opc : op.children()){
		resolve(opc, dst);
	}
	
	return dst;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::set(const xml_node op){
	using namespace Expression;
	Parser parser = {};
	
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


void MacroEngine::resolve(const xml_node op, xml_node dst){
	assert(op.root() != dst.root());
	
	// Not tag
	if (op.type() != xml_node_type::node_element){
		dst.append_copy(op);
		return;
	}
	
	// Check for macro
	const char* cname = op.name();
	if (isupper(cname[0])){
		string_view name = cname;
		
		if (name == "CALL"){
			return call(op, dst);
		} else if (name == "SET"){
			return set(op);
		} else {
			WARNING_L1("Macro: Unknown macro '%s' treated as regular HTML tag.", cname);
		}
		
	}
	
	// Regular tag
	tag(op, dst);
}


void MacroEngine::run(const Macro& macro, xml_node dst){
	const xml_node mroot = macro.root.root();
	
	// cout << "[" << ANSI_GREEN;
	// mroot.print(cout);
	// cout << ANSI_RESET << "]";
	
	for (const xml_node mchild : mroot.children()){
		resolve(mchild, dst);
	}
	
}


// ------------------------------------------------------------------------------------------ //