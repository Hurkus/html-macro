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
	
	// Evaluate expression
	if (op_attr.options % NodeOptions::SINGLE_QUOTE){
		pExpr expr = Expression::parse(op_attr.value());
		if (expr == nullptr){
			HERE(error_expression_parse(op, op_attr));
			return;
		}
		
		string buff;
		Expression::str(expr->eval(MacroEngine::variables), buff);
		attr.value(html::newStr(buff), buff.length());
	}
	
	// Interpolate
	else if (op_attr.options % NodeOptions::INTERPOLATE){
		string buff;
		Expression::interpolate(op_attr.value(), MacroEngine::variables, buff);
		attr.value(html::newStr(buff), buff.length());
	}
	
	// Text value
	else {
		attr.value(op_attr.value());
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
	
	string buff = {};
	
	// Copy or set attributes
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		if (name == "IF"){
			continue;
		}
		
		Attr& a = dst.appendAttribute();
		a.name(name);
		
		// Evaluate expression
		if (attr->options % NodeOptions::SINGLE_QUOTE){
			pExpr expr = Expression::parse(attr->value());
			if (expr == nullptr){
				dst.removeAttr(&a);
				HERE(error_expression_parse(op, *attr));
				return;
			}
			
			buff.clear();
			Expression::str(expr->eval(MacroEngine::variables), buff);
			a.value(html::newStr(buff), buff.length());
		}
		
		// Interpolate
		else if (attr->options % NodeOptions::INTERPOLATE){
			buff.clear();
			interpolate(attr->value(), MacroEngine::variables, buff);
			a.value(html::newStr(buff), buff.length());
		}
		
		// Plain text
		else {
			a.value(attr->value());
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
	
	string buff;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view varName = attr->name();
		if (varName == "IF"){
			continue;
		}
		
		string_view attrName = attr->value();
		
		// Evaluate expression
		if (attr->options % NodeOptions::SINGLE_QUOTE){
			pExpr expr = Expression::parse(attrName);
			if (expr == nullptr){
				HERE(error_expression_parse(op, *attr));
				return;
			}
			
			buff.clear();
			Expression::str(expr->eval(MacroEngine::variables), buff);
			attrName = buff;
		}
		
		// Interpolate
		else if (attr->options % NodeOptions::INTERPOLATE){
			buff.clear();
			Expression::interpolate(attrName, MacroEngine::variables, buff);
			attrName = buff;
		}
		
		// Find attribute and copy value to variable
		for (const Attr* a = dst.attribute ; a != nullptr ; a = a->next){
			if (a->name() == attrName){
				variables[varName] = Value(in_place_type<string>, a->value());
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
	
	string newName = {};
	
	// Evaluate expression
	if (attr_valName->options % NodeOptions::SINGLE_QUOTE){
		pExpr expr = Expression::parse(attr_valName->value());
		if (expr == nullptr){
			HERE(error_expression_parse(op, *attr_valName));
			return;
		}
		
		Expression::str(expr->eval(MacroEngine::variables), newName);
	}
		
	// Interpolate
	else if (attr_valName->options % NodeOptions::INTERPOLATE){
		Expression::interpolate(attr_valName->value(), MacroEngine::variables, newName);
	}
	
	dst.name(html::newStr(newName), newName.length());
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