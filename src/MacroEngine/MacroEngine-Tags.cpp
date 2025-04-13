#include "MacroEngine.hpp"
#include "DEBUG.hpp"

using namespace std;
using namespace pugi;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


xml_text MacroEngine::text(const char* str, xml_node dst){
	xml_text txtnode = dst.append_child(xml_node_type::node_pcdata).text();
	
	if (!interpolateText){
		txtnode.set(str);
		return txtnode;
	}
	
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
	
	xml_node node = dst.append_child(op.name());
	xml_attribute attr_call = {};
	bool _interpolateText = this->interpolateText;
	
	if (op.name() == "style"sv){
		_interpolateText = false;
	}
	
	// Copy attributes
	for (const xml_attribute attr : op.attributes()){
		const char* cname = attr.name();
		
		// Regular attribute
		if (!isupper(cname[0])){
			__copy:
			attribute(attr, node);
			continue;
		}
		
		string_view name = cname;
		
		if (name == "CALL"){
			attr_call = attr;
		} else if (name == "IF"){
			optbool val = evalCond(attr.value());
			
			if (val.empty()){
				WARNING_L1("%s: Invalid expression in macro attribute [%s=\"%s\"]. Defaulting to false.", op.name(), attr.name(), attr.value());
				return {};
			} else if (val == false){
				dst.remove_child(node);
				return {};
			} else {
				continue;
			}
			
		} else if (name == "INTERPOLATE"){
			optbool val = evalCond(attr.value());
			
			if (val.empty()){
				WARNING_L1("%s: Invalid expression in macro attribute [%s=\"%s\"].", op.name(), attr.name(), attr.value());
			}
			
			_interpolateText = val.get();
		} else {
			WARNING_L1("Macro: Unknown macro '%s' treated as regular HTML tag.", cname);
			goto __copy;
		}
		
	}
	
	swap(this->interpolateText, _interpolateText);
	if (!attr_call.empty()){
		call(attr_call.value(), node);
	}
	
	// Resolve children
	runChildren(op, node);
	this->interpolateText = _interpolateText;
	return node;
}


// ------------------------------------------------------------------------------------------ //