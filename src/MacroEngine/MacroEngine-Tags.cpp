#include "MacroEngine.hpp"
#include "DEBUG.hpp"

using namespace std;
using namespace pugi;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


xml_text MacroEngine::text(const char* str, xml_node dst){
	xml_text txtnode = dst.append_child(xml_node_type::node_pcdata).text();
	
	// Check if value requires interpolation
	size_t len;
	if (!hasInterpolation(str, &len)){
		txtnode.set(str, len);
		return txtnode;
	}
	
	// Interpolate
	string buff = {};
	buff.append(str, len);
	interpolate(str + len, this->variables, buff);
	
	txtnode.set(buff.c_str(), buff.length());
	return txtnode;
}


xml_attribute MacroEngine::attribute(const xml_attribute src, xml_node dst){
	xml_attribute attr = dst.append_attribute(src.name());
	const char* str = src.value();
	
	// Check if value requires interpolation
	size_t len;
	if (!hasInterpolation(str, &len)){
		attr.set_value(str, len);
		return attr;
	}
	
	// Interpolate
	string buff = {};
	buff.append(str, len);
	interpolate(str + len, this->variables, buff);
	
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