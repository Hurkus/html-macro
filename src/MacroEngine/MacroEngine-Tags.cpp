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
		string buff;
		eval_string_interpolate(src, src.value(), buff);
		txt.value(html::newStr(buff), buff.length());
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
	assert(op.type == NodeType::TAG);
	
	// Create child, unlinked
	unique_ptr child = unique_ptr<Node,Node::deleter>(html::newNode());
	child->type = op.type;
	
	// Copy attributes
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		// Check regular attribute
		if (name.empty()){
			assert(!name.empty());
			continue;
		} else if (!isupper(name[0])){
			attribute(op, *attr, *child);
			continue;
		}
		
		if (name == "CALL"){
			call(op, *attr, *child);
			continue;
		}
		
		else if (name.starts_with("INCLUDE")){
			include(op, *attr, *child);
			continue;
		}
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		// Unknown attr macro
		HERE(warn_unknown_macro_attribute(op, *attr));
		attribute(op, *attr, *child);
	}
	
	child->type = NodeType::TAG;
	child->options |= (op.options & (NodeOptions::SELF_CLOSE | NodeOptions::SPACE_AFTER | NodeOptions::SPACE_BEFORE));
	child->name(op.name());
	
	// Link child to parent
	Node* newNode = child.release();
	dst.appendChild(newNode);
	
	// Resolve children
	runChildren(op, *newNode);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


// <SET-ATTR [IF=''] [name]=[value] />
void MacroEngine::setAttr(const Node& op, Node& dst){
	string newValue_buff;
	string_view newValue;
	
	if (op.child != nullptr){
		HERE(warn_child_ignored(op));
	}
	
	// Copy or set attributes
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		// Evaluate value
		newValue_buff.clear();
		if (!eval_attr_value(op, *attr, newValue_buff, newValue)){
			continue;
		}
		
		// Find existing attribute
		Attr* a;
		for (a = dst.attribute ; a != nullptr ; a = a->next){
			if (a->name() == name)
				goto found;
		}
		
		// Create attribute
		a = &dst.appendAttribute();
		a->name(name);
		
		found:
		if (newValue_buff.empty()){
			a->value(newValue);
		} else {
			a->value(html::newStr(newValue), newValue.length());
		}
		
	}
	
}


void MacroEngine::getAttr(const Node& op, Node& dst){
	string attrName_buff;
	string_view attrName;
	
	if (op.child != nullptr){
		HERE(warn_child_ignored(op));
	}
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view varName = attr->name();
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		// Evaluate attribute name
		attrName_buff.clear();
		if (!eval_attr_value(op, *attr, attrName_buff, attrName)){
			return;
		}
		
		// Find attribute and copy value to variable
		for (const Attr* a = dst.attribute ; a != nullptr ; a = a->next){
			if (a->name() == attrName){
				MacroEngine::variables.insert(varName, a->value());
				goto next;
			}
		}
		
		// Not found
		MacroEngine::variables.insert(varName, 0L);
		next:
	}
	
}


void MacroEngine::delAttr(const Node& op, Node& dst){
	if (op.child != nullptr){
		HERE(warn_child_ignored(op));
	}
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		if (attr->value_len > 0){
			HERE(warn_ignored_attr_value(op, *attr));
		}
		
		dst.removeAttr(attr->name());
	}
	
}


void MacroEngine::setTag(const Node& op, Node& dst){
	const Attr* attr_name = nullptr;
	
	if (op.child != nullptr){
		HERE(warn_child_ignored(op));
	}
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		// New tag name retrieved from attribute value
		if (name == "NAME"){
			if (attr->value_len <= 0){
				HERE(error_missing_attr_value(op, *attr));
				return;
			}
		}
		
		// New tag name retrieved from attribute name
		if (attr_name != nullptr){
			HERE(error_duplicate_attr(op, *attr_name, *attr));
			return;
		} else {
			attr_name = attr;
		}
		
	}
	
	// Set simple name
	if (attr_name == nullptr){
		HERE(error_missing_attr(op, "NAME"));
		return;
	}
	
	// New tag name retrieved from attribute name
	else if (attr_name->value_len <= 0){
		dst.name(attr_name->name());
		return;
	}
	
	// New tag name retrieved from attribute value
	string newName_buff;
	string_view newName;
	if (eval_attr_value(op, *attr_name, newName_buff, newName)){
		if (newName_buff.empty())
			dst.name(newName);
		else
			dst.name(html::newStr(newName), newName.length());
	}
	
}


void MacroEngine::getTag(const Node& op, Node& dst){
	if (op.child != nullptr){
		HERE(warn_child_ignored(op));
	}
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view varName = attr->name();
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		if (attr->value_len > 0){
			HERE(warn_ignored_attr_value(op, *attr));
		}
		
		MacroEngine::variables.insert(varName, dst.name());
	}
	
}


// ------------------------------------------------------------------------------------------ //