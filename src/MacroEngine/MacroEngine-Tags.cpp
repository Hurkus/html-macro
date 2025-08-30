#include "MacroEngine.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::text(const Node& src, Node& dst){
	assert(src.type != NodeType::ROOT);
	Node& txt = dst.appendChild(src.type);
	
	if (isSet(src.options, NodeOptions::INTERPOLATE)){
		// TODO: interpolate
	}
	
	txt.value(src.value());
}


void MacroEngine::attribute(const Attr& src, Node& dst){
	Attr& attr = dst.appendAttribute();
	attr.name(src.name());
	attr.value(src.value());
}


void MacroEngine::tag(const Node& op, Node& dst){
	assert(op.type == NodeType::TAG);
	
	Node& child = dst.appendChild(NodeType::TAG);
	child.name(op.name());
	
	// Copy attributes
	Attr* attr = op.attribute;
	while (attr != nullptr){
		string_view name = attr->name();
		
		// Regular attribute
		if (name.length() < 1 || !isupper(name[0])){
			attribute(*attr, child);
		}
		
		// else if (name == "IF"){
		// 	if (!_attr_if(*this, op, attr)){
		// 		dst.remove_child(node);
		// 		return {};
		// 	}
		// }
		// else if (name == "INTERPOLATE"){
		// 	_attr_interpolate(*this, op, attr, _interp);
		// }
		// else if (name == "CALL"){
		// 	attr_call = attr;
		// }
		else {
			warn_unknown_attribute(op, *attr);
			attribute(*attr, child);
		}
		
		attr = attr->next;
	}
	
	// if (!attr_call.empty()){
	// 	this->interpolateText = _interp;
	// 	call(attr_call.value(), node);
	// }
	
	// Resolve children
	runChildren(op, child);
}


// ------------------------------------------------------------------------------------------ //









// ----------------------------------- [ Functions ] ---------------------------------------- //


// void MacroEngineObject::interpolateAttr(const char* str, pugi::xml_attribute dst){
// 	assert(str != nullptr);
	
// 	// Check if value requires interpolation
// 	size_t len;
// 	if (!hasInterpolation(str, &len)){
// 		dst.set_value(str, len);
// 		return;
// 	}
	
// 	// Interpolate
// 	string buff = {};
// 	buff.append(str, len);
// 	interpolate(str + len, this->variables, buff);
// 	dst.set_value(buff.c_str(), buff.length());
// }


// xml_attribute MacroEngineObject::attribute(const xml_attribute src, xml_node dst){
// 	assert(!src.empty());
	
// 	if (!src.empty()){
// 		xml_attribute attr = dst.append_attribute(src.name());
// 		interpolateAttr(src.value(), attr);
// 		return attr;
// 	}
	
// 	return {};
// }


// ----------------------------------- [ Functions ] ---------------------------------------- //


// bool MacroEngineObject::setAttr(const xml_node op, xml_node dst){
// 	// Check IF conditional
// 	for (const xml_attribute attr : op.attributes()){
// 		if (attr.name() == "IF"sv){
// 			if (!_attr_if(*this, op, attr))
// 				return false;
// 		}
// 	}
	
// 	// Copy or set attributes
// 	for (const xml_attribute src_attr : op.attributes()){
// 		const char* cname = src_attr.name();
// 		if (cname == "IF"sv){
// 			continue;
// 		}
		
// 		xml_attribute dst_attr = dst.attribute(cname);
// 		if (dst_attr.empty()){
// 			dst_attr = dst.append_attribute(cname);
// 		}
		
// 		interpolateAttr(src_attr.value(), dst_attr);
// 	}
	
// 	return true;
// }


// bool MacroEngineObject::getAttr(const xml_node op, xml_node dst){
// 	// Check IF conditional
// 	for (const xml_attribute attr : op.attributes()){
// 		if (attr.name() == "IF"sv){
// 			if (!_attr_if(*this, op, attr))
// 				return false;
// 		}
// 	}
	
// 	for (const xml_attribute attr : op.attributes()){
// 		const char* var_name = attr.name();
// 		if (var_name == "IF"sv){
// 			continue;
// 		}
		
// 		const char* attr_name = attr.value();
// 		variables[var_name] = dst.attribute(attr_name).value();
// 	}
	
// 	return true;
// }


// bool MacroEngineObject::delAttr(const xml_node op, xml_node dst){
// 	// Check IF conditional
// 	for (const xml_attribute attr : op.attributes()){
// 		if (attr.name() == "IF"sv){
// 			if (!_attr_if(*this, op, attr))
// 				return false;
// 		}
// 	}
	
// 	// Copy or set attributes
// 	for (const xml_attribute attr : op.attributes()){
// 		const char* cname = attr.name();
// 		if (cname != "IF"sv)
// 			dst.remove_attribute(attr.value());
// 	}
	
// 	return true;
// }


// bool MacroEngineObject::setTag(const xml_node op, xml_node dst){
// 	xml_attribute name_attr;
	
// 	// Check IF conditional
// 	for (const xml_attribute attr : op.attributes()){
// 		string_view name = attr.name();
		
// 		if (name == "IF"){
// 			if (!_attr_if(*this, op, attr))
// 				return false;
// 		} else if (name == "NAME"){
// 			if (name_attr.empty())
// 				name_attr = attr;
// 			else
// 				_attr_duplicate(op, name_attr, attr);
// 		} else {
// 			_attr_ignore(op, attr);
// 		}
		
// 	}
	
// 	if (name_attr.empty()){
// 		_attr_missing(op, "NAME");
// 		return false;
// 	}
	
// 	const char* cname = name_attr.value();
// 	if (cname[0] == 0){
// 		ERROR("%s: Attribute 'NAME' cannot be empty.", op.name());
// 		return false;
// 	}
	
// 	string buff;
// 	interpolate(cname, variables, buff);
// 	return dst.set_name(buff.c_str(), buff.length());
// }


// bool MacroEngineObject::getTag(const xml_node op, xml_node dst){
// 	// Check IF conditional
// 	for (const xml_attribute attr : op.attributes()){
// 		if (attr.name() == "IF"sv){
// 			if (!_attr_if(*this, op, attr))
// 				return false;
// 		}
// 	}
	
// 	for (const xml_attribute attr : op.attributes()){
// 		const char* var_name = attr.name();
// 		if (var_name == "IF"sv){
// 			continue;
// 		}
		
// 		variables[var_name] = dst.name();
// 	}
	
// 	return true;
// }


// bool MacroEngineObject::delTag(const xml_node op, xml_node dst){
// 	assert(false);
// 	return false;
// }


// ------------------------------------------------------------------------------------------ //