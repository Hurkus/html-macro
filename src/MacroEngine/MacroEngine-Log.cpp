#include "MacroEngine.hpp"
// #include "MacroEngine-Common.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;
using namespace Expression;


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
	string_view msg = {};
	bool interpolate = false;
};


static bool eval(const Node& op, LogInfo& out){
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		// string_view name = attr->name();
		
		// if (name == "IF"){
		// 	if (!_attr_if(*this, op, attr))
		// 		return;
		// } else if (name == "INTERPOLATE"){
		// 	_attr_interpolate(*this, op, attr, intrp);
		// } else {
		// 	_attr_ignore(op, attr);
		// }
		
		warn_ignored_attribute(op, *attr);
	}
	
	const Node* txt = op.child;
	if (txt != nullptr && txt->type != NodeType::TEXT){
		error(*txt, "Expected plain text node.");
		return false;
	}
	
	out.msg = trim_whitespace(txt->value());
	out.interpolate = isSet(txt->options, NodeOptions::INTERPOLATE);
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