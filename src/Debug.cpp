#include "Debug.hpp"
#include <cstdarg>

#include "DebugSource.hpp"
#include "Macro.hpp"
#include "Expression.hpp"

using namespace std;
using namespace html;


// ---------------------------------- [ Definitions ] --------------------------------------- //


#define BOLD(s) 	ANSI_BOLD s ANSI_RESET
#define RED(s)		ANSI_RED s ANSI_RESET
#define YELLOW(s)	ANSI_YELLOW s ANSI_RESET
#define PURPLE(s)	ANSI_PURPLE s ANSI_RESET


// ----------------------------------- [ Variables ] ---------------------------------------- //


bool stderr_isTTY = false;
bool stdout_isTTY = false;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void printErrSrc(const char* file, long row, long col) noexcept {
	if (file != nullptr && file[0] != 0){
		if (row > 0 && col > 0)
			LOG_STDERR(BOLD("%s:%ld:%ld: "), file, row, col)
		else if (row > 0)
			LOG_STDERR(BOLD("%s:%ld: "), file, row)
		else
			LOG_STDERR(BOLD("%s: "), file)
	}
}


void printErrTag() noexcept {
	LOG_STDERR(ANSI_RED "error: " ANSI_RESET);
}


void printWarnTag() noexcept {
	LOG_STDERR(ANSI_YELLOW "warn: " ANSI_RESET);
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
	print(pos);
	printErrTag();
}

static void print_warn_pfx(const linepos& pos){
	print(pos);
	printWarnTag();
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_node(const Node& node, string_view msg){
	print_error_pfx(findLine(node.root(), node.value_p));
	LOG_STDERR("%.*s\n", int(msg.length()), msg.data());
}


void warn_node(const Node& node, string_view msg){
	print_warn_pfx(findLine(node.root(), node.value_p));
	LOG_STDERR("%.*s\n", int(msg.length()), msg.data());
}


void info_node(const Node& node, string_view msg){
	print(findLine(node.root(), node.value_p));
	LOG_STDERR("%.*s\n", int(msg.length()), msg.data());
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_missing_attr(const Node& node, const char* attr_name){
	linepos pos = findLine(node.root(), node.value_p);
	print_error_pfx(pos);
	LOG_STDERR("Tag " PURPLE("<%.*s>") " missing attribute " PURPLE("`%s`") ".\n", int(node.value_len), node.value_p, attr_name);
	printCodeView(pos, node.name(), ANSI_RED);
}

void warn_missing_attr(const Node& node, const char* attr_name){
	linepos pos = findLine(node.root(), node.value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Tag " PURPLE("<%.*s>") " missing attribute " PURPLE("`%s`") ".\n", int(node.value_len), node.value_p, attr_name);
	printCodeView(pos, node.name(), ANSI_YELLOW);
}


// Attribute <attr> missing value.
void error_missing_attr_value(const Node& node, const Attr& attr){
	assert(attr.name_p != nullptr);
	const Document& doc = node.root();
	
	// Point to empty attribute value
	if (attr.value_p != nullptr){
		linepos pos = findLine(doc, attr.value_p);
		print_error_pfx(pos);
		LOG_STDERR("Attribute " PURPLE("`%.*s`") " missing value.\n", int(attr.name_len), attr.name_p);
		printCodeView(pos, inclusiveValue(attr), ANSI_RED);
	}
	
	// Point to attribute name
	else {
		linepos pos = findLine(doc, attr.name_p);
		print_error_pfx(pos);
		LOG_STDERR("Attribute " PURPLE("`%.*s`") " missing value.\n", int(attr.name_len), attr.name_p);
		printCodeView(pos, attr.name(), ANSI_RED);
	}
	
}

void warn_missing_attr_value(const Node& node, const Attr& attr){
	const Document& doc = node.root();
	linepos pos = findLine(doc, attr.name_p);
	
	print_warn_pfx(pos);
	LOG_STDERR("Attribute " PURPLE("`%.*s`") " missing value.\n", int(attr.name_len), attr.name_p);
	
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
	LOG_STDERR("Duplicate attribute " PURPLE("`%.*s`") ".\n", int(attr_1.name_len), attr_1.name_p);
	printCodeView(pos1, attr_1.name(), ANSI_RED);
	
	print_error_pfx(pos2);
	LOG_STDERR("Duplicate attribute " PURPLE("`%.*s`") ".\n", int(attr_2.name_len), attr_2.name_p);
	printCodeView(pos1, attr_2.name(), ANSI_RED);
}


void warn_duplicate_attr(const Node& node, const Attr& attr_1, const Attr& attr_2){
	const Document& doc = node.root();
	linepos pos1 = findLine(doc, attr_1.name_p);
	linepos pos2 = findLine(doc, attr_2.name_p);
	
	print_warn_pfx(pos1);
	LOG_STDERR("Duplicate attribute " PURPLE("`%.*s`") ".\n", int(attr_1.name_len), attr_1.name_p);
	printCodeView(pos1, attr_1.name(), ANSI_YELLOW);
	
	print_warn_pfx(pos2);
	LOG_STDERR("Duplicate attribute " PURPLE("`%.*s`") ".\n", int(attr_2.name_len), attr_2.name_p);
	printCodeView(pos1, attr_2.name(), ANSI_YELLOW);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_macro_not_found(const Node& node, string_view mark, string_view macroName){
	linepos pos = findLine(node.root(), mark.data());
	print_error_pfx(pos);
	LOG_STDERR("Macro " PURPLE("`%.*s`") " not found.\n", int(macroName.length()), macroName.data());
	printCodeView(pos, mark, ANSI_RED);
}


// File <file> not found.
void error_file_not_found(const Node& node, string_view mark, const char* file){
	linepos pos = findLine(node.root(), mark.data());
	print_error_pfx(pos);
	LOG_STDERR("File " PURPLE("`%s`") " not found.\n", file);
	printCodeView(pos, mark, ANSI_RED);
}


// Failed to include file <file>.
void error_include_fail(const Node& node, string_view mark, const char* file){
	linepos pos = findLine(node.root(), mark.data());
	print_error_pfx(pos);
	LOG_STDERR("Failed to include file " PURPLE("`%s`") ".\n", file);
	printCodeView(pos, mark, ANSI_RED);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_unsupported_type(const Node& node){
	linepos pos = findLine(node.root(), node.value_p);
	print_error_pfx(pos);
	LOG_STDERR("Unsupported tag type.\n");
	printCodeView(pos, node.value_p, ANSI_RED);
}


void warn_unknown_macro_tag(const Node& node){
	linepos pos = findLine(node.root(), node.value_p);
	print_error_pfx(pos);
	LOG_STDERR("Unknown macro " PURPLE("<%.*s>") " treated as regular HTML tag.\n", int(node.value_len), node.value_p);
	printCodeView(pos, node.name(), ANSI_YELLOW);
}

void warn_unknown_macro_attribute(const Node& node, const Attr& attr){
	linepos pos = findLine(node.root(), attr.name_p);
	print_warn_pfx(pos);
	LOG_STDERR("Unknown macro attribute " PURPLE("`%.*s`") " treated as regular HTML attribute.\n", int(attr.name_len), attr.name_p);
	printCodeView(pos, attr.name(), ANSI_YELLOW);
}


void warn_ignored_attribute(const Node& node, const Attr& attr){
	linepos pos = findLine(node.root(), attr.name_p);
	print_warn_pfx(pos);
	LOG_STDERR("Ignored attribute " PURPLE("`%.*s`") ".\n", int(attr.name_len), attr.name_p);
	printCodeView(pos, attr.name(), ANSI_YELLOW);
}

void warn_ignored_attr_value(const Node& node, const Attr& attr){
	assert(attr.value_p != nullptr);
	linepos pos = findLine(node.root(), attr.value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Value of attribute " PURPLE("`%.*s`") " in ignored.\n", int(attr.name_len), attr.name_p);
	printCodeView(pos, inclusiveValue(attr), ANSI_YELLOW);
}


void warn_attr_single_quote(const Node& node, const Attr& attr){
	assert(attr.name_p != nullptr && attr.value_p != nullptr);
	linepos pos = findLine(node.root(), attr.value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Attribute " PURPLE("`%.*s`") " expected double quotes (\"\"). Value is always interpreted as a string.\n", int(attr.name_len), attr.name_p);
	printCodeView(pos, inclusiveValue(attr), ANSI_YELLOW);
}


void warn_attr_double_quote(const Node& node, const Attr& attr){
	assert(attr.value_p != nullptr);
	linepos pos = findLine(node.root(), attr.value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Attribute " PURPLE("`%.*s`") " expected single quotes (''). Value is always interpreted as an expression.\n", int(attr.name_len), attr.name_p);
	printCodeView(pos, inclusiveValue(attr), ANSI_YELLOW);
}


void warn_shell_exit(const Node& node, int i){
	linepos pos = findLine(node.root(), node.value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Shell existed with status (%d).\n", i);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_missing_preceeding_if_node(const Node& node){
	linepos pos = findLine(node.root(), node.value_p);
	print_error_pfx(pos);
	LOG_STDERR("Missing preceeding " PURPLE("<IF>") " macro.\n");
	printCodeView(pos, node.value_p, ANSI_RED);
}


void error_missing_preceeding_if_attr(const Node& node, const Attr& attr){
	linepos pos = findLine(node.root(), attr.name_p);
	print_error_pfx(pos);
	LOG_STDERR("Missing preceeding " PURPLE("`IF`") " attribute in a sibling node.\n");
	printCodeView(pos, attr.name(), ANSI_RED);
}


void error_undefined_variable(const html::Node& node, const string_view name){
	const Document& doc = node.root();
	linepos pos = findLine(doc, name.data());
	
	if (!pos.line.empty()){
		print_error_pfx(pos);
		LOG_STDERR("Undefined variable " PURPLE("`%.*s`") ".\n", int(name.length()), name.data());
		printCodeView(pos, name, ANSI_RED);
	} else {
		pos = findLine(doc, node.value_p);
		print_error_pfx(pos);
		LOG_STDERR("Undefined variable " PURPLE("`%.*s`") ".\n", int(name.length()), name.data());
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_newline(const Node& node, const char* p){
	linepos pos = findLine(node.root(), p);
	print_error_pfx(pos);
	LOG_STDERR("Unexpected newline.\n");
	printCodeView(pos, string_view(p, 1), ANSI_RED);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void warn_text_missing(const Node& node){
	linepos pos = findLine(node.root(), node.value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Macro " PURPLE("`%.*s`") " requires text content.\n", int(node.value_len), node.value_p);
}


void warn_child_ignored(const Node& node){
	const Document& doc = node.root();
	
	linepos pos = findLine(doc, node.value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Content of macro " PURPLE("`%.*s`") " ignored.\n", int(node.value_len), node.value_p);
	
	if (node.child != nullptr){
		string_view mark = trimWhitespace(node.child->value());
		pos = findLine(doc, mark.begin());
		printCodeView(pos, mark, ANSI_YELLOW);
	}
}


// ------------------------------------------------------------------------------------------ //