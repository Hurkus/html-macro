#include "MacroEngine.hpp"
#include "Debug.hpp"
#include "DebugSource.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Functions ] ---------------------------------------- //


static string_view trimWhitespace(string_view s){
	const char* beg = s.begin();
	const char* end = beg + s.length();
	
	while (beg != end && isspace(beg[+0])) beg++;
	while (end != beg && isspace(end[-1])) end--;
	
	return string_view(beg, end);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static const Node* eval(const Node& op){
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		
		// Check IF, ELIF, ELSE
		switch (check_attr_if(op, *attr)){
			case Branch::FAILED: return nullptr;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		HERE(warn_ignored_attribute(op, *attr));
	}
	
	const Node* txt = op.child;
	if (txt == nullptr || txt->type != NodeType::TEXT){
		HERE(warn_text_missing(op));
		return nullptr;
	}
	
	return txt;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::error(const Node& op){
	const Node* txt = eval(op);
	if (txt == nullptr){
		return;
	}
	
	string_view msg = trimWhitespace(txt->value());
	if (txt->options % NodeOptions::INTERPOLATE){
		string buff;
		if (eval_string(op, msg, buff))
			::error_node(op, buff);
	} else {
		::error_node(op, msg);
	}
	
}


void MacroEngine::warn(const Node& op){
	const Node* txt = eval(op);
	if (txt == nullptr){
		return;
	}
	
	string_view msg = trimWhitespace(txt->value());
	if (txt->options % NodeOptions::INTERPOLATE){
		string buff;
		if (eval_string(op, msg, buff))
			::warn_node(op, buff);
	} else {
		::warn_node(op, msg);
	}
	
}


void MacroEngine::info(const Node& op){
	const Node* txt = eval(op);
	if (txt == nullptr){
		return;
	}
	
	string_view msg = trimWhitespace(txt->value());
	if (txt->options % NodeOptions::INTERPOLATE){
		string buff;
		if (eval_string(op, msg, buff))
			::info_node(op, buff);
	} else {
		::info_node(op, msg);
	}
	
}


// ------------------------------------------------------------------------------------------ //