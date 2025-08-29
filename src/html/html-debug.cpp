#include "html-debug.hpp"
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


// ----------------------------------- [ Functions ] ---------------------------------------- //


template<typename T>
inline const char* _name(const T* obj){
	if (obj == nullptr)
		return nullptr;
	return obj->name().data();
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void html::error(const Node* node, const char* msg){
	const Document& doc = node->root();
	long row = doc.row(_name(node));
	long col = doc.col(_name(node));
	_ERROR(doc.file(), row, col, "%s", msg);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void html::error_missing_attr(const Node& node, const char* name){
	const Document& doc = node.root();
	long row = doc.row(node.value_p);
	long col = doc.col(node.value_p);
	_ERROR(doc.file(), row, col, "Missing attribute '" PURPLE "%s" RS "'.", name);
}


void html::warn_missing_attr(const Node& node, const char* name){
	const Document& doc = node.root();
	long row = doc.row(node.value_p);
	long col = doc.col(node.value_p);
	_WARN(doc.file(), row, col, "Tag <%s> missing attribute '" PURPLE "%s" RS "'.", string(node.name()).c_str(), name);
}


// ------------------------------------------------------------------------------------------ //