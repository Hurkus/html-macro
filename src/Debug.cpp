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


// ------------------------------------------------------------------------------------------ //