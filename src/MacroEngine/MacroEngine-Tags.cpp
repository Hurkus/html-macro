#include "MacroEngine.hpp"
#include "ExpressionParser.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::text(const Node& src, Node& dst){
	assert(src.type != NodeType::ROOT);
	Node& txt = dst.appendChild(src.type);
	
	if (src.options % NodeOptions::INTERPOLATE){
		string s = {};
		Expression::interpolate(src.value(), MacroEngine::variables, s);
		txt.value(html::newStr(s), s.length());
	} else {
		txt.value(src.value());
	}
	
}


void MacroEngine::attribute(const Node& op, const Attr& op_attr, Node& dst){
	Attr& attr = dst.appendAttribute();
	attr.name(op_attr.name());
	
	string buff;
	string_view buff_view;
	
	if (!eval_attr_value(op, op_attr, buff, buff_view)){
		return;
	} else if (!buff.empty()){
		attr.value(html::newStr(buff), buff.length());
	} else {
		attr.value(buff_view);
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
			attribute(op, *attr, *child);
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
			HERE(warn_unknown_macro_attribute(op, *attr));
			attribute(op, *attr, *child);
		}
		
	}
	
	child->type = NodeType::TAG;
	child->options |= (op.options & (NodeOptions::SELF_CLOSE | NodeOptions::SPACE_AFTER | NodeOptions::SPACE_BEFORE));
	child->name(op.name());
	
	// Link child to parent
	Node* child_p = child.release();
	dst.appendChild(child_p);
	
	if (attr_call_before != nullptr){
		call(op, *attr_call_before, *child_p);
	}
	
	// Resolve children
	runChildren(op, *child_p);
	
	if (attr_call_after != nullptr){
		call(op, *attr_call_after, *child_p);
	}
	
	// Restore state
	MacroEngine::currentInterpolation = _interp;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::setAttr(const Node& op, Node& dst){
	// Check conditional
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		if (attr->name() == "IF"){
			if (!eval_attr_if(op, *attr))
				return;
		}
	}
	
	string buff;
	string_view buff_view;
	
	// Copy or set attributes
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		if (name == "IF"){
			continue;
		}
		
		// Evaluate value
		buff.clear();
		if (!eval_attr_value(op, *attr, buff, buff_view)){
			continue;
		}
		
		// Create attribute
		Attr& a = dst.appendAttribute();
		a.name(name);
		
		if (!buff.empty()){
			a.value(html::newStr(buff), buff.length());
		} else {
			a.value(buff_view);
		}
		
	}
	
}


void MacroEngine::getAttr(const Node& op, Node& dst){
	// Check conditional
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		if (attr->name() == "IF"){
			if (!eval_attr_if(op, *attr))
				return;
		}
	}
	
	string attrName_buff;
	string_view attrName;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view varName = attr->name();
		if (varName == "IF"){
			continue;
		}
		
		// Evaluate attribute name
		attrName_buff.clear();
		if (!eval_attr_value(op, *attr, attrName_buff, attrName)){
			continue;
		}
		
		// Find attribute and copy value to variable
		for (const Attr* a = dst.attribute ; a != nullptr ; a = a->next){
			if (a->name() == attrName){
				MacroEngine::variables.insert(varName, in_place_type<string>, a->value());
				break;
			}
		}
		
	}
	
}


void MacroEngine::delAttr(const Node& op, Node& dst){
	// Check conditional
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		if (attr->name() == "IF"){
			if (!eval_attr_if(op, *attr))
				return;
		}
	}
	
	// Delete attributes
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		if (attr->name() != "IF")
			dst.removeAttr(attr->name());
	}
	
}


void MacroEngine::setTag(const Node& op, Node& dst){
	const Attr* attr_nameName = nullptr;
	const Attr* attr_valName = nullptr;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name == "IF"){
			if (!eval_attr_if(op, *attr))
				return;
		} else if (name == "NAME"){
			if (attr_nameName != nullptr){
				HERE(error_duplicate_attr(op, *attr_nameName, *attr));
				return;
			} else if (attr_valName != nullptr){
				HERE(error_duplicate_attr(op, *attr_valName, *attr));
				return;
			}  else {
				attr_valName = attr;
			}
		} else {
			if (attr_nameName != nullptr){
				HERE(error_duplicate_attr(op, *attr_nameName, *attr));
				return;
			} else if (attr_valName != nullptr){
				HERE(error_duplicate_attr(op, *attr_valName, *attr));
				return;
			}  else {
				attr_nameName = attr;
			}
		}
		
	}
	
	// Set simple name
	if (attr_nameName != nullptr){
		dst.name(attr_nameName->name());
		return;
	} else if (attr_valName == nullptr){
		HERE(error_missing_attr(op, "NAME"));
		return;
	}
	
	string newName_buff;
	string_view newName;
	if (!eval_attr_value(op, *attr_valName, newName_buff, newName)){
		return;
	} else if (!newName_buff.empty()){
		dst.name(html::newStr(newName), newName.length());
	} else {
		dst.name(newName);
	}
	
}


void MacroEngine::getTag(const Node& op, Node& dst){
	// Check conditional
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		if (attr->name() == "IF"){
			if (!eval_attr_if(op, *attr))
				return;
		}
	}
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view varName = attr->name();
		if (varName == "IF"){
			continue;
		}
		
		if (attr->value_len > 0){
			HERE(warn_ignored_attr_value(op, *attr));
		}
		
		MacroEngine::variables[varName] = Value(in_place_type<string>, dst.name());
	}
	
}


void MacroEngine::delTag(const Node& op, Node& dst){
	::error(op, "Not yet implemented.");
	assert(false);
}


// ------------------------------------------------------------------------------------------ //