#include "MacroEngine.hpp"
#include "MacroEngine-Common.hpp"

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
	const char* tag_cname = op.name();
	string_view tag_name = tag_cname;
	
	xml_node node = dst.append_child(tag_cname);
	xml_attribute attr_call = {};
	
	bool _interp = this->interpolateText;
	if (tag_name == "style" || tag_name == "script"){
		_interp = false;
	}
	
	// Copy attributes
	for (const xml_attribute attr : op.attributes()){
		const char* cname = attr.name();
		
		// Regular attribute
		if (!isupper(cname[0])){
			attribute(attr, node);
			continue;
		}
		
		string_view name = cname;
		if (name == "IF"){
			if (!_attr_if(*this, op, attr)){
				dst.remove_child(node);
				return {};
			}
		} else if (name == "INTERPOLATE"){
			_attr_interpolate(*this, op, attr, _interp);
		} else if (name == "CALL"){
			attr_call = attr;
		} else {
			WARNING_L1("%s: Unknown attribute '%s' treated as regular HTML attribute.", op.name(), cname);
			attribute(attr, node);
		}
		
	}
	
	const bool _interp2 = this->interpolateText;
	
	if (!attr_call.empty()){
		this->interpolateText = _interp;
		call(attr_call.value(), node);
	}
	
	// Resolve children
	this->interpolateText = _interp;
	runChildren(op, node);
	this->interpolateText = _interp2;
	return node;
}


// ------------------------------------------------------------------------------------------ //