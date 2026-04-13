#include "MacroEngine.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr string_view trim(string_view s){
	const char* beg = s.begin();
	const char* end = beg + s.length();
	
	while (beg != end && isspace(beg[+0])) beg++;
	while (end != beg && isspace(end[-1])) end--;
	
	return string_view(beg, end);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::info(const Node& op){
	assert(macro != nullptr);
	
	// Get text node
	const Node* txt_node = op.child;
	if (txt_node == nullptr || txt_node->type != NodeType::TEXT){
		return;
	} else if (txt_node->next != nullptr){
		HERE(warn_ignored_child(*macro, op, txt_node->next));
	}
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		HERE(warn_ignored_attr(*macro, *attr));
	}
	
	string_view txt = trim(txt_node->value());
	
	// Interpolate msg
	if (txt_node->options % NodeOptions::INTERPOLATE){
		string buff;
		eval_string_interpolate(txt, buff);
		info_node(*macro, op, buff);
		return;
	}
	
	// Raw string msg
	info_node(*macro, op, txt);
}


void MacroEngine::warn(const Node& op){
	assert(macro != nullptr);
	
	// Get text node
	const Node* txt_node = op.child;
	if (txt_node == nullptr || txt_node->type != NodeType::TEXT){
		return;
	} else if (txt_node->next != nullptr){
		HERE(warn_ignored_child(*macro, op, txt_node->next));
	}
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		HERE(warn_ignored_attr(*macro, *attr));
	}
	
	string_view txt = trim(txt_node->value());
	
	// Interpolate msg
	if (txt_node->options % NodeOptions::INTERPOLATE){
		string buff;
		eval_string_interpolate(txt, buff);
		warn_node(*macro, op, buff);
		return;
	}
	
	// Raw string msg
	warn_node(*macro, op, txt);
}


void MacroEngine::error(const Node& op){
	assert(macro != nullptr);
	
	// Get text node
	const Node* txt_node = op.child;
	if (txt_node == nullptr || txt_node->type != NodeType::TEXT){
		return;
	} else if (txt_node->next != nullptr){
		HERE(warn_ignored_child(*macro, op, txt_node->next));
	}
	
	for (const Attr* attr = op.attribute ; attr != nullptr ; attr = attr->next){
		
		// Check IF, ELIF, ELSE
		switch (try_eval_attr_if_elif_else(op, *attr)){
			case Branch::FAILED: return;
			case Branch::PASSED: continue;
			case Branch::NONE: break;
		}
		
		HERE(warn_ignored_attr(*macro, *attr));
	}
	
	string_view txt = trim(txt_node->value());
	
	// Interpolate msg
	if (txt_node->options % NodeOptions::INTERPOLATE){
		string buff;
		eval_string_interpolate(txt, buff);
		error_node(*macro, op, buff);
		return;
	}
	
	// Raw string msg
	error_node(*macro, op, txt);
}


// ------------------------------------------------------------------------------------------ //