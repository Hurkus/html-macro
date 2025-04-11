#include "MacroEngine.hpp"
#include "ExpressionParser.hpp"
#include "DEBUG.hpp"

using namespace std;
using namespace pugi;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::info(const pugi::xml_node op){
	for (const xml_node child : op){
		switch (child.type()){
			case xml_node_type::node_cdata:
			case xml_node_type::node_pcdata:
				INFO(ANSI_BOLD "INFO: " ANSI_RESET "%s", child.value());
				break;
			default:
				break;
		}
	}
}


void MacroEngine::warn(const pugi::xml_node op){
	for (const xml_node child : op){
		switch (child.type()){
			case xml_node_type::node_cdata:
			case xml_node_type::node_pcdata:
				INFO(ANSI_BOLD ANSI_YELLOW "WARN: " ANSI_RESET "%s", child.value());
				break;
			default:
				break;
		}
	}
}


void MacroEngine::error(const pugi::xml_node op){
	for (const xml_node child : op){
		switch (child.type()){
			case xml_node_type::node_cdata:
			case xml_node_type::node_pcdata:
				INFO(ANSI_BOLD ANSI_RED "ERROR: " ANSI_RESET "%s", child.value());
				break;
			default:
				break;
		}
	}
}


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
		
		if (name == "IF"){
			branch_if(op, dst);
		} else if (name == "ELSE-IF"){
			branch_elif(op, dst);
		} else if (name == "ELSE"){
			branch_else(op, dst);
		}
		
		else if (name == "CALL"){
			call(op, dst);
		} else if (name == "SET"){
			set(op);
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