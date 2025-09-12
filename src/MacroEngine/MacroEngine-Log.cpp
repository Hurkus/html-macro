#include "MacroEngine.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Functions ] ---------------------------------------- //


static string_view trim_whitespace(string_view s){
	const char* beg = s.begin();
	const char* end = beg + s.length();
	
	while (beg != end && isspace(beg[+0])) beg++;
	while (end != beg && isspace(end[-1])) end--;
	
	return string_view(beg, end);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


struct LogInfo {
	string msg = {};
};


static bool eval(const Node& op, LogInfo& out){
	Interpolate interp = MacroEngine::currentInterpolation;
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		string_view name = attr->name();
		
		if (name == "IF"){
			if (!eval_attr_if(op, *attr))
				return false;
		} else if (name == "INTERPOLATE"){
			interp = eval_attr_interp(op, *attr);
		} else {
			HERE(warn_ignored_attribute(op, *attr));
		}
		
	}
	
	const Node* txt = op.child;
	if (txt != nullptr && txt->type != NodeType::TEXT){
		error(*txt, "Expected plain text node.");
		return false;
	}
	
	string_view msg = trim_whitespace(txt->value());
	
	// Interpolate
	if (interp % Interpolate::CONTENT){
		eval_string(op, msg, out.msg);
	} else {
		out.msg = msg;
	}
	
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::info(const Node& op){
	LogInfo info = {};
	if (!eval(op, info)){
		return;
	}
	
	string msg = string(info.msg);
	::info(op, msg.c_str());
}


void MacroEngine::warn(const Node& op){
	LogInfo info = {};
	if (!eval(op, info)){
		return;
	}
	
	string msg = string(info.msg);
	::warn(op, msg.c_str());
}


void MacroEngine::error(const Node& op){
	LogInfo info = {};
	if (!eval(op, info)){
		return;
	}
	
	string msg = string(info.msg);
	::error(op, msg.c_str());
}


// ------------------------------------------------------------------------------------------ //