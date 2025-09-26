#include "Debug.hpp"
#include <cstdarg>

#include "DebugSource.hpp"
#include "Macro.hpp"
#include "Expression.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ---------------------------------- [ Definitions ] --------------------------------------- //


#define BOLD(s) 	ANSI_BOLD s ANSI_RESET
#define RED(s)		ANSI_RED s ANSI_RESET
#define YELLOW(s)	ANSI_YELLOW s ANSI_RESET
#define PURPLE(s)	ANSI_PURPLE s ANSI_RESET


// ----------------------------------- [ Variables ] ---------------------------------------- //


bool log_stderr_isTTY = false;
bool log_stdout_isTTY = false;


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr bool skipANSIColor(const char*& beg){
	assert(*beg == '\e');
	const char* s = beg;
	
	if (*(++s) != '['){
		return false;
	}
	
	s++;
	for (int i = 0 ; i < 3 ; i++, s++){
		if (!('0' <= *s && *s <= '9'))
			break;
	}
	
	if (*s == 'm'){
		beg = s + 1;
		return true;
	}
	
	return false;
}


void log(FILE* stream, const char* fmt, ...){
	assert(fmt != nullptr);
	va_list args;
	va_start(args, fmt);
	
	// Has colors
	if ((stream == stdout && log_stdout_isTTY) || (stream == stderr && log_stderr_isTTY)){
		vfprintf(stream, fmt, args);
		va_end(args);
		return;
	}
	
	constexpr int flush = 110;	// Flush before %fmt
	constexpr int size = 126;
	char buff[size + 2];
	int i = 0;
	
	const char* p = fmt;
	while (*p != 0){
		
		if (*p == '\e'){
			if (skipANSIColor(p))
				continue;
		}
		
		else if (*p == '%'){
			if (i > flush){
				goto _flush;
			}
			
			// %%
			else if (p[1] == '%'){
				buff[i++] = *p;
				p++;
			}
			
		}
		
		buff[i++] = *p;
		p++;
		
		// Flush
		if (i >= size){ _flush:
			buff[i] = 0;
			vfprintf(stream, buff, args);
			i = 0;
		}
		
	}
	
	if (i > 0){
		buff[i] = 0;
		vfprintf(stream, buff, args);
	}
	
	va_end(args);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr string_view expand(string_view s, int padding){
	return string_view(s.begin() - padding, s.end() + padding);
}

constexpr string_view inclusiveValue(const Attr& attr){
	return string_view(attr.value_p - 1, attr.value_p + attr.value_len + 1);
}

static string_view trimWhitespace(string_view s){
	const char* beg = s.begin();
	const char* end = beg + s.length();
	
	while (beg != end && isspace(beg[+0])) beg++;
	while (end != beg && isspace(end[-1])) end--;
	
	return string_view(beg, end);
}

static linepos findLine(const Document& doc, const char* p){
	linepos l = {};
	
	if (doc.buffer != nullptr){
		string_view buff = string_view(*doc.buffer);
		l = findLine(buff.begin(), buff.end(), p);
	}
	
	l.file = (doc.srcFile != nullptr) ? doc.srcFile->c_str() : nullptr;
	return l;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void print_error_pfx(const linepos& pos){
	if (pos.row > 0 && pos.col > 0)
		log(stderr, BOLD("%s:%ld:%ld: ") RED("error: "), pos.file, pos.row, pos.col);
	else if (pos.row > 0)
		log(stderr, BOLD("%s:%ld: ") RED("error: "), pos.file, pos.row);
	else if (pos.file != nullptr)
		log(stderr, BOLD("%s: ") RED("error: "), pos.file);
	else
		log(stderr, RED("error: "));
}

static void print_warn_pfx(const linepos& pos){
	if (pos.row > 0 && pos.col > 0)
		log(stderr, BOLD("%s:%ld:%ld: ") YELLOW("warn: "), pos.file, pos.row, pos.col);
	else if (pos.row > 0)
		log(stderr, BOLD("%s:%ld: ") YELLOW("warn: "), pos.file, pos.row);
	else if (pos.file != nullptr)
		log(stderr, BOLD("%s: ") YELLOW("warn: "), pos.file);
	else
		log(stderr, YELLOW("warn: "));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_node(const Node& node, string_view msg){
	print_error_pfx(findLine(node.root(), node.value_p));
	log(stderr, "%.*s\n", int(msg.length()), msg.data());
}


void warn_node(const Node& node, string_view msg){
	print_warn_pfx(findLine(node.root(), node.value_p));
	log(stderr, "%.*s\n", int(msg.length()), msg.data());
}


void info_node(const Node& node, string_view msg){
	print(findLine(node.root(), node.value_p));
	log(stderr, "%.*s\n", int(msg.length()), msg.data());
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_missing_attr(const Node& node, const char* attr_name){
	linepos pos = findLine(node.root(), node.value_p);
	print_error_pfx(pos);
	log(stderr, "Tag <%.*s> missing attribute " PURPLE("`%s`") ".\n", int(node.value_len), node.value_p, attr_name);
	printCodeView(pos, node.name(), ANSI_RED);
}

void warn_missing_attr(const Node& node, const char* attr_name){
	linepos pos = findLine(node.root(), node.value_p);
	print_warn_pfx(pos);
	log(stderr, "Tag <%.*s> missing attribute " PURPLE("`%s`") ".\n", int(node.value_len), node.value_p, attr_name);
	printCodeView(pos, node.name(), ANSI_YELLOW);
}


void error_missing_attr_value(const Node& node, const Attr& attr){
	const Document& doc = node.root();
	linepos pos = findLine(doc, attr.name_p);
	
	print_error_pfx(pos);
	log(stderr, "Attribute " PURPLE("`%.*s`") " missing value.\n", int(attr.name_len), attr.name_p);
	
	if (attr.value_p != nullptr){
		pos = findLine(doc, attr.value_p);
		printCodeView(pos, inclusiveValue(attr), ANSI_RED);
	} else {
		printCodeView(pos, attr.name(), ANSI_RED);
	}
	
}

void warn_missing_attr_value(const Node& node, const Attr& attr){
	const Document& doc = node.root();
	linepos pos = findLine(doc, attr.name_p);
	
	print_warn_pfx(pos);
	log(stderr, "Attribute " PURPLE("`%.*s`") " missing value.\n", int(attr.name_len), attr.name_p);
	
	if (attr.value_p != nullptr){
		pos = findLine(doc, attr.value_p);
		printCodeView(pos, inclusiveValue(attr), ANSI_YELLOW);
	} else {
		printCodeView(pos, attr.name(), ANSI_YELLOW);
	}
	
}


void error_duplicate_attr(const Node& node, const Attr& attr_1, const Attr& attr_2){
	const Document& doc = node.root();
	linepos pos1 = findLine(doc, attr_1.name_p);
	linepos pos2 = findLine(doc, attr_2.name_p);
	
	print_error_pfx(pos1);
	log(stderr, "Duplicate attribute " PURPLE("`%.*s`") ".\n", int(attr_1.name_len), attr_1.name_p);
	printCodeView(pos1, attr_1.name(), ANSI_RED);
	
	print_error_pfx(pos2);
	log(stderr, "Duplicate attribute " PURPLE("`%.*s`") ".\n", int(attr_2.name_len), attr_2.name_p);
	printCodeView(pos1, attr_2.name(), ANSI_RED);
}


void warn_duplicate_attr(const Node& node, const Attr& attr_1, const Attr& attr_2){
	const Document& doc = node.root();
	linepos pos1 = findLine(doc, attr_1.name_p);
	linepos pos2 = findLine(doc, attr_2.name_p);
	
	print_warn_pfx(pos1);
	log(stderr, "Duplicate attribute " PURPLE("`%.*s`") ".\n", int(attr_1.name_len), attr_1.name_p);
	printCodeView(pos1, attr_1.name(), ANSI_YELLOW);
	
	print_warn_pfx(pos2);
	log(stderr, "Duplicate attribute " PURPLE("`%.*s`") ".\n", int(attr_2.name_len), attr_2.name_p);
	printCodeView(pos1, attr_2.name(), ANSI_YELLOW);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_macro_not_found(const Node& node, string_view mark, string_view macroName){
	linepos pos = findLine(node.root(), mark.data());
	print_error_pfx(pos);
	log(stderr, "Macro " PURPLE("`%.*s`") " not found.\n", int(macroName.length()), macroName.data());
	printCodeView(pos, mark, ANSI_RED);
}


void error_file_not_found(const Node& node, string_view mark, const char* file){
	linepos pos = findLine(node.root(), mark.data());
	print_error_pfx(pos);
	log(stderr, "File " PURPLE("`%s`") " not found.\n", file);
	printCodeView(pos, mark, ANSI_RED);
}


void error_invalid_include_type(const Node& node, string_view mark){
	linepos pos = findLine(node.root(), mark.data());
	print_error_pfx(pos);
	log(stderr, "Invalid include type " PURPLE("`%.*s`") ".\n", int(mark.length()), mark.data());
	printCodeView(pos, mark, ANSI_RED);
}


void error_include_fail(const Node& node, string_view mark, const char* file){
	linepos pos = findLine(node.root(), mark.data());
	print_error_pfx(pos);
	log(stderr, "Failed to include file " PURPLE("`%s`") ".\n", file);
	printCodeView(pos, mark, ANSI_RED);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_unsupported_type(const Node& node){
	linepos pos = findLine(node.root(), node.value_p);
	print_error_pfx(pos);
	log(stderr, "Unsupported tag type.\n");
	printCodeView(pos, node.value_p, ANSI_RED);
}


void warn_unknown_macro_tag(const Node& node){
	linepos pos = findLine(node.root(), node.value_p);
	print_error_pfx(pos);
	log(stderr, "Unknown macro " PURPLE("<%.*s>") " treated as regular HTML tag.\n", int(node.value_len), node.value_p);
	printCodeView(pos, node.name(), ANSI_YELLOW);
}

void warn_unknown_macro_attribute(const Node& node, const Attr& attr){
	linepos pos = findLine(node.root(), attr.name_p);
	print_warn_pfx(pos);
	log(stderr, "Unknown macro attribute " PURPLE("`%.*s`") " treated as regular HTML attribute.\n", int(attr.name_len), attr.name_p);
	printCodeView(pos, attr.name(), ANSI_YELLOW);
}


void warn_ignored_attribute(const Node& node, const Attr& attr){
	linepos pos = findLine(node.root(), attr.name_p);
	print_warn_pfx(pos);
	log(stderr, "Ignored attribute " PURPLE("`%.*s`") ".\n", int(attr.name_len), attr.name_p);
	printCodeView(pos, attr.name(), ANSI_YELLOW);
}

void warn_ignored_attr_value(const Node& node, const Attr& attr){
	assert(attr.value_p != nullptr);
	linepos pos = findLine(node.root(), attr.value_p);
	print_warn_pfx(pos);
	log(stderr, "Value of attribute " PURPLE("`%.*s`") " in ignored.\n", int(attr.name_len), attr.name_p);
	printCodeView(pos, inclusiveValue(attr), ANSI_YELLOW);
}


void warn_attr_double_quote(const Node& node, const Attr& attr){
	assert(attr.value_p != nullptr);
	linepos pos = findLine(node.root(), attr.value_p);
	print_warn_pfx(pos);
	log(stderr, "Attribute " PURPLE("`%.*s`") " expected single quotes. Value is always interpreted as an expression.\n", int(attr.name_len), attr.name_p);
	printCodeView(pos, inclusiveValue(attr), ANSI_YELLOW);
}


void warn_shell_exit(const Node& node, int i){
	linepos pos = findLine(node.root(), node.value_p);
	print_warn_pfx(pos);
	log(stderr, "Shell existed with status (%d).\n", i);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_missing_preceeding_if_node(const Node& node){
	linepos pos = findLine(node.root(), node.value_p);
	print_error_pfx(pos);
	log(stderr, "Missing preceeding " PURPLE("<IF>") " macro.\n");
	printCodeView(pos, node.value_p, ANSI_RED);
}


void error_missing_preceeding_if_attr(const Node& node, const Attr& attr){
	linepos pos = findLine(node.root(), attr.name_p);
	print_error_pfx(pos);
	log(stderr, "Missing preceeding " PURPLE("`IF`") " attribute in a sibling node.\n");
	printCodeView(pos, attr.name(), ANSI_RED);
}


void error_undefined_variable(const html::Node& node, const string_view name){
	const Document& doc = node.root();
	linepos pos = findLine(doc, name.data());
	
	if (!pos.line.empty()){
		print_error_pfx(pos);
		log(stderr, "Undefined variable " PURPLE("`%.*s`") ".\n", int(name.length()), name.data());
		printCodeView(pos, name, ANSI_RED);
	} else {
		pos = findLine(doc, node.value_p);
		print_error_pfx(pos);
		log(stderr, "Undefined variable " PURPLE("`%.*s`") ".\n", int(name.length()), name.data());
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_newline(const Node& node, const char* p){
	linepos pos = findLine(node.root(), p);
	print_error_pfx(pos);
	log(stderr, "Unexpected newline.\n");
	printCodeView(pos, string_view(p, 1), ANSI_RED);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void warn_text_missing(const Node& node){
	linepos pos = findLine(node.root(), node.value_p);
	print_warn_pfx(pos);
	log(stderr, "Macro " PURPLE("`%.*s`") " requires text content.\n", int(node.value_len), node.value_p);
}


void warn_child_ignored(const Node& node){
	const Document& doc = node.root();
	
	linepos pos = findLine(doc, node.value_p);
	print_warn_pfx(pos);
	log(stderr, "Content of macro " PURPLE("`%.*s`") " ignored.\n", int(node.value_len), node.value_p);
	
	if (node.child != nullptr){
		string_view mark = trimWhitespace(node.child->value());
		pos = findLine(doc, mark.begin());
		printCodeView(pos, mark, ANSI_YELLOW);
	}
}


// ------------------------------------------------------------------------------------------ //