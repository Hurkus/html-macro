#include "MacroEngine.hpp"
#include <cstring>

#include "ExpressionParser.hpp"
#include "DEBUG.hpp"

using namespace std;
using namespace pugi;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void interpolate(const char* str, const VariableMap& vars, string& buff){
	assert(str != nullptr);
	
	const char* beg = strchrnul(str, '{');
	if (*beg == 0){
		buff.append(str, beg);
		return;
	}
	
	const char* end = strchrnul(beg + 1, '}');
	if (*end == 0){
		buff.append(str, end);
		return;
	}
	
	Expression::Parser parser = {};
	
	while (*beg != 0 && *end != 0){
		buff.append(str, beg);
		
		pExpr expr = parser.parse(string_view(beg + 1, end));
		if (expr != nullptr){
			Value val = expr->eval(vars);
			Expression::str(val, buff);
		} else {
			WARNING_L1("TEXT: Failed to parse expression [%s].", string(beg, end+1).c_str());
		}
		
		// Next expression
		str = end + 1;
		beg = strchrnul(str, '{');
		end = strchrnul(beg, '}');
	}
	
	buff.append(str, end);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


xml_text MacroEngine::text(const char* str, xml_node dst){
	xml_text txtnode = dst.append_child(xml_node_type::node_pcdata).text();
	
	// Check if value requires interpolation
	const char* p = strchrnul(str, '{');
	if (*p == 0){
		txtnode.set(str, p - str);
		return txtnode;
	}
	
	// Interpolate
	string buff = {};
	buff.append(str, p);
	interpolate(p, this->variables, buff);
	
	txtnode.set(buff.c_str(), buff.length());
	return txtnode;
}


xml_attribute MacroEngine::attribute(const xml_attribute src, xml_node dst){
	xml_attribute attr = dst.append_attribute(src.name());
	const char* str = src.value();
	
	// Check if value requires interpolation
	const char* p = strchrnul(str, '{');
	if (*p == 0){
		attr.set_value(str, p - str);
		return attr;
	}
	
	// Interpolate
	string buff = {};
	buff.append(str, p);
	interpolate(p, this->variables, buff);
	
	attr.set_value(buff.c_str(), buff.length());
	return attr;
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
		attribute(attr, dst);
	}
	
	if (!attr_call.empty()){
		call(attr_call.value(), dst);
	}
	
	// Resolve children
	runChildren(op, dst);
	return dst;
}


// ------------------------------------------------------------------------------------------ //