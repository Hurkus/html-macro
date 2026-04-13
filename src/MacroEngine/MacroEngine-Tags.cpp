#include "MacroEngine.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr bool isMacroChar(char c){
	return ('A' <= c && c <= 'Z') || c == '_' || c == '-' || c == ':';
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::text(const Node& src, Node& dst){
	assert(src.type != NodeType::ROOT);
	assert(macro != nullptr);
	
	Node& txt = *dst.appendChild(newNode(src.type));
	txt.value_len = src.value_len;
	txt.value_p = src.value_p;
	
	// Interpolate value
	if (src.options % NodeOptions::INTERPOLATE){
		string buff;
		eval_string_interpolate(src.value(), buff);
		txt.options |= NodeOptions::OWNED_VALUE;
		txt.value_len = uint32_t(min(buff.length(), size_t(UINT32_MAX)));
		txt.value_p = newStr(buff);
	}
	
}


Attr* MacroEngine::attribute(const Node& op, const Attr& op_attr){
	assert(macro != nullptr);
	
	Attr& attr = *newAttr();
	attr.name_len = op_attr.name_len;
	attr.value_len = op_attr.value_len;
	attr.name_p = op_attr.name_p;
	attr.value_p = op_attr.value_p;
	
	// Evaluate or interpolate value
	if (op_attr.options % (NodeOptions::SINGLE_QUOTE | NodeOptions::INTERPOLATE)){
		string buff;
		string_view view;
		if (eval_attr_value(op, op_attr, buff, view)){
			attr.options |= NodeOptions::OWNED_VALUE;
			attr.value_len = uint32_t(min(view.length(), size_t(UINT32_MAX)));
			attr.value_p = newStr(view);
		}
	}
	
	return &attr;
}


// <tag [CALL='name'] [INCLUDE='src'] tag=[value] ...>
void MacroEngine::tag(const Node& op, Node& dst){
	assert(op.type == NodeType::TAG);
	assert(macro != nullptr);
	assert(macro->html != nullptr);
	
	Node& node = *newNode(op.type);
	node.options = op.options & (NodeOptions::SELF_CLOSE | NodeOptions::SPACE_AFTER | NodeOptions::SPACE_BEFORE);
	node.value_len = op.value_len;
	node.value_p = op.value_p;
	
	// Copy attributes
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		assert(!name.empty());
		
		// Check regular attribute
		if (!isMacroChar(name[0])){ regular_attr:
			node.appendAttribute(attribute(op, *attr));
			continue;
		}
		
		else if (name == "CALL"){
			call(op, *attr, node);
			continue;
		}
		
		else if (name.starts_with("INCLUDE")){
			include(op, *attr, node);
			continue;
		}
		
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *attr)){
			case Branch::FAILED:
				node.remove(*macro->html);
				return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		// User macro
		if (call_userAttrMacro(op, *attr, node)){
			continue;
		}
		
		HERE(warn_unknown_attribute_macro(*macro, *attr));
		goto regular_attr;
	}
	
	// Link child to parent
	dst.appendChild(&node);
	
	// Evaluate child elements
	evalChildren(op, node);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


// <SET-ATTR [IF=''] [parent_attr_name]=[value] .../>
void MacroEngine::setAttr(const Node& op, Node& dst){
	assert(macro != nullptr);
	assert(macro->html != nullptr);
	
	if (op.child != nullptr){
		HERE(warn_ignored_child(*macro, op));
	}
	
	string buff;
	string_view newValue;
	
	// Copy or set attributes
	for (const Attr* op_attr = op.attribute ; op_attr != nullptr ; op_attr = op_attr->next){
		string_view name = op_attr->name();
		
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *op_attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		// Find existing attribute
		Attr* dst_attr;
		for (dst_attr = dst.attribute ; dst_attr != nullptr ; dst_attr = dst_attr->next){
			if (dst_attr->name() == name)
				goto found;
		}
		
		// Create new attr
		dst_attr = dst.appendAttribute(newAttr());
		dst_attr->name_len = op_attr->name_len;
		dst_attr->name_p = op_attr->name_p;
		
		found: // Value could be from an existing attribute so it needs to be cleared.
		
		// Evaluate or interpolate value
		if (op_attr->options % (NodeOptions::SINGLE_QUOTE | NodeOptions::INTERPOLATE)){
			buff.clear();
			if (eval_attr_value(op, *op_attr, buff, newValue)){
				dst_attr->value(*macro->html, newStr(newValue), newValue.length());
				continue;
			}
		}
		
		// Value as external string
		dst_attr->value(*macro->html, op_attr->value());
		continue;
	}
	
	return;
}


void MacroEngine::getAttr(const Node& op, Node& dst){
	assert(macro != nullptr);
	assert(variables != nullptr);
	
	if (op.child != nullptr){
		HERE(warn_ignored_child(*macro, op));
	}
	
	string buff;
	
	for (const Attr* op_attr = op.attribute ; op_attr != nullptr ; op_attr = op_attr->next){
		string_view var_name = op_attr->name();
		
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *op_attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		// Evaluate attribute name
		buff.clear();
		string_view attr_name;
		if (!eval_attr_value(op, *op_attr, buff, attr_name)){
			return;
		}
		
		// Find attribute and copy value to variable
		for (const Attr* dst_attr = dst.attribute ; dst_attr != nullptr ; dst_attr = dst_attr->next){
			if (dst_attr->name() == attr_name){
				variables->insert(var_name, dst_attr->value());
				goto next;
			}
		}
		
		// Not found
		MacroEngine::variables->insert(var_name, 0L);
		
		next: continue;
	}
	
}


void MacroEngine::delAttr(const Node& op, Node& dst){
	assert(macro != nullptr);
	assert(macro->html != nullptr);
	
	if (op.child != nullptr){
		HERE(warn_ignored_child(*macro, op));
	}
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		if (attr->value_len > 0){
			HERE(warn_ignored_attr_value(*macro, *attr));
		}
		
		dst.removeAttr(*macro->html, attr->name());
	}
	
}


void MacroEngine::setTag(const Node& op, Node& dst){
	assert(macro != nullptr);
	assert(macro->html != nullptr);
	
	if (op.child != nullptr){
		HERE(warn_ignored_child(*macro, op));
	}
	
	const Attr* name_attr = nullptr;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		// New tag name retrieved from attribute value
		if (name == "NAME"){
			if (attr->value_len <= 0){
				HERE(error_missing_attr_value(*macro, *attr));
				continue;
			}
		}
		
		// New tag name retrieved from attribute name
		if (name_attr != nullptr){
			HERE(error_duplicate_attr(*macro, *name_attr, *attr));
		}
		
		name_attr = attr;
		break;
	}
	
	if (name_attr == nullptr){
		HERE(error_missing_attr(*macro, op, "NAME"));
		return;
	}
	
	// New tag name retrieved from attribute name
	else if (name_attr->value_len <= 0){
		dst.name(*macro->html, name_attr->name());
		return;
	}
	
	// New tag name retrieved from attribute NAME=[value]
	else if (name_attr->options % (NodeOptions::SINGLE_QUOTE | NodeOptions::INTERPOLATE)){
		string buff;
		string_view view;
		if (eval_attr_value(op, *name_attr, buff, view)){
			dst.name(*macro->html, newStr(view), view.length());
		}
		return;	// Don't assign name to failed expression, tag name characters remain valid.
	}
	
	// Name is an extern string
	dst.name(*macro->html, name_attr->value());
}


void MacroEngine::getTag(const Node& op, Node& dst){
	assert(macro != nullptr);
	assert(variables != nullptr);
	
	if (op.child != nullptr){
		HERE(warn_ignored_child(*macro, op));
	}
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view var_name = attr->name();
		
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		if (attr->value_len > 0){
			HERE(warn_ignored_attr_value(*macro, *attr));
		}
		
		MacroEngine::variables->insert(var_name, dst.name());
	}
	
}


// ------------------------------------------------------------------------------------------ //