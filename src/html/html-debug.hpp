#pragma once


namespace html {
	struct Node;
	struct Attr;
};


namespace html {
// ----------------------------------- [ Functions ] ---------------------------------------- //


void error(const html::Node* node, const char* msg);

void error_missing_attr(const html::Node& node, const char* name);
void warn_missing_attr(const html::Node& node, const char* name);


// ------------------------------------------------------------------------------------------ //
};