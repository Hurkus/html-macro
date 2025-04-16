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


void MacroEngine::interpolateAttr(const char* str, pugi::xml_attribute dst){
	assert(str != nullptr);
	
	// Check if value requires interpolation
	size_t len;
	if (!hasInterpolation(str, &len)){
		dst.set_value(str, len);
		return;
	}
	
	// Interpolate
	string buff = {};
	buff.append(str, len);
	interpolate(str + len, this->variables, buff);
	dst.set_value(buff.c_str(), buff.length());
}


xml_attribute MacroEngine::attribute(const xml_attribute src, xml_node dst){
	assert(!src.empty());
	
	if (!src.empty()){
		xml_attribute attr = dst.append_attribute(src.name());
		interpolateAttr(src.value(), attr);
		return attr;
	}
	
	return {};
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool MacroEngine::setAttr(const xml_node op, xml_node dst){
	// Check IF conditional
	for (const xml_attribute attr : op.attributes()){
		if (attr.name() == "IF"sv){
			if (!_attr_if(*this, op, attr))
				return false;
		}
	}
	
	// Copy or set attributes
	for (const xml_attribute src_attr : op.attributes()){
		const char* cname = src_attr.name();
		if (cname == "IF"sv){
			continue;
		}
		
		xml_attribute dst_attr = dst.attribute(cname);
		if (dst_attr.empty()){
			dst_attr = dst.append_attribute(cname);
		}
		
		interpolateAttr(src_attr.value(), dst_attr);
	}
	
	return true;
}


bool MacroEngine::getAttr(const xml_node op, xml_node dst){
	// Check IF conditional
	for (const xml_attribute attr : op.attributes()){
		if (attr.name() == "IF"sv){
			if (!_attr_if(*this, op, attr))
				return false;
		}
	}
	
	for (const xml_attribute attr : op.attributes()){
		const char* var_name = attr.name();
		if (var_name == "IF"sv){
			continue;
		}
		
		const char* attr_name = attr.value();
		variables[var_name] = dst.attribute(attr_name).value();
	}
	
	return true;
}


bool MacroEngine::delAttr(const xml_node op, xml_node dst){
	// Check IF conditional
	for (const xml_attribute attr : op.attributes()){
		if (attr.name() == "IF"sv){
			if (!_attr_if(*this, op, attr))
				return false;
		}
	}
	
	// Copy or set attributes
	for (const xml_attribute attr : op.attributes()){
		const char* cname = attr.name();
		if (cname != "IF"sv)
			dst.remove_attribute(attr.value());
	}
	
	return true;
}


bool MacroEngine::getTag(const xml_node op, xml_node dst){
	// Check IF conditional
	for (const xml_attribute attr : op.attributes()){
		if (attr.name() == "IF"sv){
			if (!_attr_if(*this, op, attr))
				return false;
		}
	}
	
	for (const xml_attribute attr : op.attributes()){
		const char* var_name = attr.name();
		if (var_name == "IF"sv){
			continue;
		}
		
		variables[var_name] = dst.name();
	}
	
	return true;
}


bool MacroEngine::setTag(const xml_node op, xml_node dst){
	xml_attribute name_attr;
	
	// Check IF conditional
	for (const xml_attribute attr : op.attributes()){
		string_view name = attr.name();
		
		if (name == "IF"){
			if (!_attr_if(*this, op, attr))
				return false;
		} else if (name == "NAME"){
			if (name_attr.empty())
				name_attr = attr;
			else
				_attr_duplicate(op, name_attr, attr);
		} else {
			_attr_ignore(op, attr);
		}
		
	}
	
	if (name_attr.empty()){
		_attr_missing(op, "NAME");
		return false;
	}
	
	const char* cname = name_attr.value();
	if (cname[0] == 0){
		ERROR_L1("%s: Attribute 'NAME' cannot be empty.", op.name());
		return false;
	}
	
	return dst.set_name(cname);
}


bool MacroEngine::delTag(const xml_node op, xml_node dst){
	assert(false);
	return false;
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