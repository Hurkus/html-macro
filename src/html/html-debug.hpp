#pragma once


namespace html {
	struct Node;
	struct Attr;
};


namespace html {
// ----------------------------------- [ Functions ] ---------------------------------------- //


void error(const html::Node& node, const char* msg);
void warn(const html::Node& node, const char* msg);
void info(const html::Node& node, const char* msg);

void error_missing_attr(const html::Node& node, const char* name);
void warn_missing_attr(const html::Node& node, const char* name);

void warn_unknown_macro(const html::Node& node);
void warn_unknown_attribute(const html::Node& node, const html::Attr& attr);
void warn_ignored_attribute(const html::Node& node, const html::Attr& attr);
void warn_macro_not_found(const html::Node& node, const html::Attr& attr);


// ------------------------------------------------------------------------------------------ //
};