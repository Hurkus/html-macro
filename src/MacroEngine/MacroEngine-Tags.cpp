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
		eval_string(src, src.value(), buff);
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
	const Attr* attr_call_first = nullptr;
	const Attr* attr_call_last = nullptr;
	const Attr* attr_incl_first = nullptr;
	const Attr* attr_incl_last = nullptr;
	
	// Create child, unlinked
	assert(op.type == NodeType::TAG);
	unique_ptr child = unique_ptr<Node,Node::deleter>(html::newNode());
	
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
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		if (name.starts_with("CALL")){
			if (name == "CALL" || name == "CALL-FIRST"){
				if (attr_call_first != nullptr)
					warn_duplicate_attr(op, *attr_call_first, *attr);
				else
					attr_call_first = attr;
				continue;
			} else if (name == "CALL-LAST"){
				if (attr_call_last != nullptr)
					warn_duplicate_attr(op, *attr_call_last, *attr);
				else
					attr_call_last = attr;
				continue;
			}
			goto unknown;
		}
		
		if (name.starts_with("INCLUDE")){
			if (name == "INCLUDE" || name == "INCLUDE-FIRST"){
				if (attr_incl_first != nullptr)
					warn_duplicate_attr(op, *attr_incl_first, *attr);
				else
					attr_incl_first = attr;
				continue;
			} else if (name == "INCLUDE-LAST"){
				if (attr_incl_last != nullptr)
					warn_duplicate_attr(op, *attr_incl_last, *attr);
				else
					attr_incl_last = attr;
				continue;
			}
			goto unknown;
		}
		
		unknown:
		HERE(warn_unknown_macro_attribute(op, *attr));
		attribute(op, *attr, *child);
	}
	
	child->type = NodeType::TAG;
	child->options |= (op.options & (NodeOptions::SELF_CLOSE | NodeOptions::SPACE_AFTER | NodeOptions::SPACE_BEFORE));
	child->name(op.name());
	
	// Link child to parent
	Node* newNode = child.release();
	dst.appendChild(newNode);
	
	if (attr_call_first != nullptr)
		call(op, *attr_call_first, *newNode);
	if (attr_incl_first != nullptr)
		include(op, *attr_incl_first, *newNode);
	
	// Resolve children
	runChildren(op, *newNode);
	
	if (attr_call_last != nullptr)
		call(op, *attr_call_last, *newNode);
	if (attr_incl_last != nullptr)
		include(op, *attr_incl_last, *newNode);
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


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
		
		MacroEngine::variables.insert(varName, in_place_type<string>, dst.name());
	}
	
}


// void MacroEngine::delTag(const Node& op, Node& dst){
// 	::error(op, "Not yet implemented.");
// 	assert(false);
// }


// ------------------------------------------------------------------------------------------ //