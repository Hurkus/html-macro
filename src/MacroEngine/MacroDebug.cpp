#include "MacroDebug.hpp"
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


void MacroEngine::error(const node* node, const char* msg){
	const document& doc = node->root();
	long row = doc.row(_name(node));
	long col = doc.col(_name(node));
	_ERROR(doc.file(), row, col, "%s", msg);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void MacroEngine::error_missing_attr(const node& node, const char* name){
	const document& doc = node.root();
	long row = doc.row(node.value_p);
	long col = doc.col(node.value_p);
	_ERROR(doc.file(), row, col, "Missing attribute '" PURPLE "%s" RS "'.", name);
}


void MacroEngine::warn_missing_attr(const node& node, const char* name){
	const document& doc = node.root();
	long row = doc.row(node.value_p);
	long col = doc.col(node.value_p);
	_WARN(doc.file(), row, col, "Tag <%s> missing attribute '" PURPLE "%s" RS "'.", string(node.name()).c_str(), name);
}


// ------------------------------------------------------------------------------------------ //