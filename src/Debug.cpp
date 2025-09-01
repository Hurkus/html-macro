#include "Debug.hpp"
#include "Macro.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ---------------------------------- [ Definitions ] --------------------------------------- //


#define RS		ANSI_RESET
#define BOLD 	ANSI_BOLD
#define GREEN	ANSI_GREEN
#define PURPLE	ANSI_PURPLE


#define _ERROR(file, row, col, FMT, ...)	\
	if (row > 0 && col > 0){ \
		ERROR(BOLD "%s:%ld:%ld:" RS " " FMT, file, row, col, __VA_ARGS__); \
	} else if (row > 0){ \
		ERROR(BOLD "%s:%ld:" RS " " FMT, file, row, __VA_ARGS__); \
	} else { \
		ERROR(BOLD "%s:" RS " " FMT, file, __VA_ARGS__); \
	} \

#define _WARN(file, row, col, FMT, ...)	\
	if (row > 0 && col > 0){ \
		WARN(BOLD "%s:%ld:%ld:" RS " " FMT, file, row, col, __VA_ARGS__); \
	} else if (row > 0){ \
		WARN(BOLD "%s:%ld:" RS " " FMT, file, row, __VA_ARGS__); \
	} else { \
		WARN(BOLD "%s:" RS " " FMT, file, __VA_ARGS__); \
	} \

#define _INFO(file, row, col, FMT, ...)	\
	if (row > 0 && col > 0){ \
		INFO(BOLD "%s:%ld:%ld:" RS " " FMT, file, row, col, __VA_ARGS__); \
	} else if (row > 0){ \
		INFO(BOLD "%s:%ld:" RS " " FMT, file, row, __VA_ARGS__); \
	} else { \
		INFO(BOLD "%s:" RS " " FMT, file, __VA_ARGS__); \
	} \

#define CNAME(node)		(string(node.name()).c_str())
#define CVALUE(node)	(string(node.value()).c_str())


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error(const Node& node, const char* msg){
	const Document& doc = node.root();
	long row = doc.row(node.value_p);
	long col = doc.col(node.value_p);
	_ERROR(doc.file(), row, col, "%s", msg);
}


void warn(const Node& node, const char* msg){
	const Document& doc = node.root();
	long row = doc.row(node.value_p);
	long col = doc.col(node.value_p);
	_WARN(doc.file(), row, col, "%s", msg);
}


void info(const Node& node, const char* msg){
	const Document& doc = node.root();
	long row = doc.row(node.value_p);
	long col = doc.col(node.value_p);
	_INFO(doc.file(), row, col, "%s", msg);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void warn_unknown_macro(const Node& node){
	const Document& doc = node.root();
	long row = doc.row(node.value_p);
	long col = doc.col(node.value_p);
	_WARN(doc.file(), row, col,
		"Unknown macro " PURPLE "<%s>" RS " treated as regular HTML tag.",
		CNAME(node)
	);
}


void warn_unknown_attribute(const Node& node, const Attr& attr){
	const Document& doc = node.root();
	long row = doc.row(attr.name_p);
	long col = doc.col(attr.name_p);
	_WARN(doc.file(), row, col,
		"Unknown attribute " PURPLE "'%s'" RS " in tag " PURPLE "<%s>" RS " treated as regular HTML attribute.",
		CNAME(attr), CNAME(node)
	);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_missing_attr(const Node& node, const char* name){
	const Document& doc = node.root();
	long row = doc.row(node.value_p);
	long col = doc.col(node.value_p);
	_ERROR(doc.file(), row, col, "Missing attribute '" PURPLE "%s" RS "'.", name);
}


void warn_missing_attr(const Node& node, const char* name){
	const Document& doc = node.root();
	long row = doc.row(node.value_p);
	long col = doc.col(node.value_p);
	_WARN(doc.file(), row, col, "Tag <%s> missing attribute '" PURPLE "%s" RS "'.", CNAME(node), name);
}


void warn_missing_attr_value(const Node& node, const Attr& attr){
	const Document& doc = node.root();
	long row = doc.row(attr.name_p);
	long col = doc.col(attr.name_p);
	_WARN(doc.file(), row, col,
		"Attribute " PURPLE "'%s'" RS " missing value in tag " PURPLE "<%s>" RS ".",
		CNAME(attr), CNAME(node)
	);
}


void warn_ignored_attribute(const Node& node, const Attr& attr){
	const Document& doc = node.root();
	long row = doc.row(attr.name_p);
	long col = doc.col(attr.name_p);
	_WARN(doc.file(), row, col,
		"Ignored attribute " PURPLE "'%s'" RS " in tag " PURPLE "<%s>" RS ".",
		CNAME(attr), CNAME(node)
	);
}


void warn_macro_not_found(const Node& node, const Attr& attr){
	const Document& doc = node.root();
	long row = doc.row(attr.name_p);
	long col = doc.col(attr.name_p);
	_WARN(doc.file(), row, col,
		"Macro " PURPLE "'%s'" RS " not found from tag " PURPLE "<%s>" RS ".",
		CVALUE(attr), CNAME(node)
	);
}


void warn_double_quote(const Node& node, const Attr& attr){
	const Document& doc = node.root();
	long row = doc.row(attr.name_p);
	long col = doc.col(attr.name_p);
	_WARN(doc.file(), row, col,
		"Attribute " PURPLE "%s" RS " in tag " PURPLE "<%s>" RS " expected single quotes. "
		"Value is always interpreted as an expression.",
		CNAME(attr), CNAME(node)
	);
}


void warn_ignored_attr_value(const Node& node, const Attr& attr){
	const Document& doc = node.root();
	long row = doc.row(attr.name_p);
	long col = doc.col(attr.name_p);
	_WARN(doc.file(), row, col,
		"Value of attribute " PURPLE "%s" RS " in tag " PURPLE "<%s>" RS " ignored.",
		CNAME(attr), CNAME(node)
	);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_expression_parse(const Node& node, const Attr& attr){
	const Document& doc = node.root();
	long row = doc.row(attr.value_p);
	long col = doc.col(attr.value_p);
	_ERROR(doc.file(), row, col,
		"Failed to parse expression " PURPLE "[%s]" RS ".",
		CVALUE(attr)
	);
}


void error_duplicate_attr(const Node& node, const Attr& attr_1, const Attr& attr_2){
	const Document& doc = node.root();
	long row_0 = doc.row(node.value_p);
	long col_0 = doc.col(node.value_p);
	long row_1 = doc.row(attr_1.name_p);
	long col_1 = doc.col(attr_1.name_p);
	long row_2 = doc.row(attr_2.name_p);
	long col_2 = doc.col(attr_2.name_p);
	
	ERROR(
		BOLD "%s:%ld:%ld:" RS
		" Duplicate attribute in tag " PURPLE "<%s>" RS ":\n"
		"  %s:%ld:%ld: [%s='%s']\n"
		"  %s:%ld:%ld: [%s='%s']",
		doc.file(), row_0, col_0, CNAME(node),
		doc.file(), row_1, col_1, CNAME(attr_1), CVALUE(attr_1),
		doc.file(), row_2, col_2, CNAME(attr_2), CVALUE(attr_2)
	);
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void warn_shell_exit(const Node& node, int i){
	const Document& doc = node.root();
	long row = doc.row(node.value_p);
	long col = doc.col(node.value_p);
	_WARN(doc.file(), row, col, "Command exited with status (%d).", i);
}


// ------------------------------------------------------------------------------------------ //