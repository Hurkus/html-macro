#include "MacroEngine.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::text(const Node& src, Node& dst){
	assert(src.type != NodeType::ROOT);
	Node& txt = dst.appendChild(src.type);
	
	if (src.options % NodeOptions::INTERPOLATE){
		string s = {};
		Expression::interpolate(src.value(), MacroEngine::variables, s);
		txt.value(s.data(), s.length());
	} else {
		txt.value(src.value());
	}
	
}


void MacroEngine::attribute(const Attr& src, Node& dst){
	Attr& attr = dst.appendAttribute();
	attr.name(src.name());
	
	if (MacroEngine::currentInterpolation % Interpolate::ATTRIBUTE){
		string s = {};
		Expression::interpolate(src.value(), MacroEngine::variables, s);
		attr.value(s.data(), s.length());
	} else {
		attr.value(src.value());
	}
	
}


void MacroEngine::tag(const Node& op, Node& dst){
	Interpolate _interp = MacroEngine::currentInterpolation;
	const Attr* attr_call_before = nullptr;
	const Attr* attr_call_after = nullptr;
	
	// Create child, unlinked
	assert(op.type == NodeType::TAG);
	unique_ptr child = unique_ptr<Node,Node::deleter>(html::newNode());
	
	// Copy attributes
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name.length() < 1 || !isupper(name[0])){
			attribute(*attr, *child);
		} else if (name == "IF"){
			if (!eval_attr_if(op, *attr))
				return;
		} else if (name == "INTERPOLATE"){
			MacroEngine::currentInterpolation = eval_attr_interp(op, *attr);
		} else if (name == "CALL" || name == "CALL-BEFORE"){
			attr_call_before = attr;
		} else if (name == "CALL-AFTER"){
			attr_call_after = attr;
		} else {
			warn_unknown_attribute(op, *attr);
			attribute(*attr, *child);
		}
		
	}
	
	child->type = NodeType::TAG;
	child->name(op.name());
	
	// Link child to parent
	Node* child_p = child.release();
	dst.appendChild(child_p);
	
	if (attr_call_before != nullptr){
		call(op, *attr_call_before, dst);
	}
	
	// Resolve children
	runChildren(op, *child_p);
	
	if (attr_call_after != nullptr){
		call(op, *attr_call_after, dst);
	}
	
	// Restore state
	MacroEngine::currentInterpolation = _interp;
}


// ------------------------------------------------------------------------------------------ //









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