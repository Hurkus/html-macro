#pragma once


namespace html {
	struct node;
	struct attr;
};


namespace MacroEngine {
// ----------------------------------- [ Functions ] ---------------------------------------- //


void error(const html::node* node, const char* msg);

void error_missing_attr(const html::node& node, const char* name);
void warn_missing_attr(const html::node& node, const char* name);


// ------------------------------------------------------------------------------------------ //
};