#include "Debug.hpp"
#include "DebugSource.hpp"
#include "Macro.hpp"
#include "Expression.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ---------------------------------- [ Definitions ] --------------------------------------- //


#define COLOR_ERROR	ANSI_RED
#define COLOR_WARN	ANSI_YELLOW

#define RESET		ANSI_RESET
#define BOLD 		ANSI_BOLD
#define PURPLE(s)	ANSI_PURPLE s ANSI_RESET


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
		fprintf(stderr, BOLD "%s:%ld:%ld: " COLOR_ERROR "error: " RESET, pos.file, pos.row, pos.col);
	else if (pos.row > 0)
		fprintf(stderr, BOLD "%s:%ld: " COLOR_ERROR "error: " RESET, pos.file, pos.row);
	else if (pos.file != nullptr)
		fprintf(stderr, BOLD "%s: " COLOR_ERROR "error: " RESET, pos.file);
	else
		fprintf(stderr, COLOR_ERROR "error: " RESET);
}

static void print_warn_pfx(const linepos& pos){
	if (pos.row > 0 && pos.col > 0)
		fprintf(stderr, BOLD "%s:%ld:%ld: " COLOR_WARN "warn: " RESET, pos.file, pos.row, pos.col);
	else if (pos.row > 0)
		fprintf(stderr, BOLD "%s:%ld: " COLOR_WARN "warn: " RESET, pos.file, pos.row);
	else if (pos.file != nullptr)
		fprintf(stderr, BOLD "%s: " COLOR_WARN "warn: " RESET, pos.file);
	else
		fprintf(stderr, COLOR_WARN "warn: " RESET);
}

static void print_error_codeView(const linepos& line, string_view mark){
	fprintf(stderr, "%s\n", getCodeView(line, mark, COLOR_ERROR).c_str());
}

static void print_warn_codeView(const linepos& line, string_view mark){
	fprintf(stderr, "%s\n", getCodeView(line, mark, COLOR_WARN).c_str());
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error(const Node& node, const char* msg){
	print_error_pfx(findLine(node.root(), node.value_p));
	fprintf(stderr, "%s\n", msg);
}


void warn(const Node& node, const char* msg){
	print_warn_pfx(findLine(node.root(), node.value_p));
	fprintf(stderr, "%s\n", msg);
}


void info(const Node& node, const char* msg){
	print(findLine(node.root(), node.value_p));
	fprintf(stderr, "%s\n", msg);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_missing_attr(const Node& node, const char* attr_name){
	linepos pos = findLine(node.root(), node.value_p);
	print_error_pfx(pos);
	fprintf(stderr, "Tag <%.*s> missing attribute " PURPLE("`%s`") ".\n", int(node.value_len), node.value_p, attr_name);
	print_error_codeView(pos, node.name());
}

void warn_missing_attr(const Node& node, const char* attr_name){
	linepos pos = findLine(node.root(), node.value_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Tag <%.*s> missing attribute " PURPLE("`%s`") ".\n", int(node.value_len), node.value_p, attr_name);
	print_warn_codeView(pos, node.name());
}


void error_missing_attr_value(const Node& node, const Attr& attr){
	const Document& doc = node.root();
	linepos pos = findLine(doc, attr.name_p);
	
	print_error_pfx(pos);
	fprintf(stderr, "Attribute " PURPLE("`%.*s`") " missing value.\n", int(attr.name_len), attr.name_p);
	
	if (attr.value_p != nullptr){
		pos = findLine(doc, attr.value_p);
		print_error_codeView(pos, inclusiveValue(attr));
	} else {
		print_error_codeView(pos, attr.name());
	}
	
}

void warn_missing_attr_value(const Node& node, const Attr& attr){
	const Document& doc = node.root();
	linepos pos = findLine(doc, attr.name_p);
	
	print_warn_pfx(pos);
	fprintf(stderr, "Attribute " PURPLE("`%.*s`") " missing value.\n", int(attr.name_len), attr.name_p);
	
	if (attr.value_p != nullptr){
		pos = findLine(doc, attr.value_p);
		print_warn_codeView(pos, inclusiveValue(attr));
	} else {
		print_warn_codeView(pos, attr.name());
	}
	
}


void error_duplicate_attr(const Node& node, const Attr& attr_1, const Attr& attr_2){
	const Document& doc = node.root();
	linepos pos1 = findLine(doc, attr_1.name_p);
	linepos pos2 = findLine(doc, attr_2.name_p);
	
	print_error_pfx(pos1);
	fprintf(stderr, "Duplicate attribute " PURPLE("`%.*s`") ".\n", int(attr_1.name_len), attr_1.name_p);
	print_error_codeView(pos1, attr_1.name());
	
	print_error_pfx(pos2);
	fprintf(stderr, "Duplicate attribute " PURPLE("`%.*s`") ".\n", int(attr_2.name_len), attr_2.name_p);
	print_error_codeView(pos1, attr_2.name());
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_macro_not_found(const Node& node, const Attr& attr, string_view macroName){
	linepos pos = findLine(node.root(), attr.value_p);
	print_error_pfx(pos);
	fprintf(stderr, "Macro " PURPLE("`%.*s`") " not found.\n", int(macroName.length()), macroName.data());
	print_error_codeView(pos, inclusiveValue(attr));
	
}

void error_macro_not_found(const Node& node, const Attr& attr){
	error_macro_not_found(node, attr, attr.value());
}


void error_file_not_found(const Node& node, const Attr& attr, string_view fileName){
	linepos pos = findLine(node.root(), attr.value_p);
	print_error_pfx(pos);
	fprintf(stderr, "File " PURPLE("`%.*s`") " not found.\n", int(fileName.length()), fileName.data());
	print_error_codeView(pos, inclusiveValue(attr));
}

void error_file_not_found(const Node& node, const Attr& attr){
	error_file_not_found(node, attr, attr.value());
}


void warn_file_include(const Node& node, const Attr& attr, const char* fileName){
	linepos pos = findLine(node.root(), attr.value_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Failed to include file " PURPLE("`%s`") ".\n", fileName);
	print_warn_codeView(pos, inclusiveValue(attr));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void warn_unknown_macro_tag(const Node& node){
	linepos pos = findLine(node.root(), node.value_p);
	print_error_pfx(pos);
	fprintf(stderr, "Unknown macro " PURPLE("<%.*s>") " treated as regular HTML tag.\n", int(node.value_len), node.value_p);
	print_warn_codeView(pos, node.name());
}

void warn_unknown_macro_attribute(const Node& node, const Attr& attr){
	linepos pos = findLine(node.root(), attr.name_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Unknown macro attribute " PURPLE("`%.*s`") " treated as regular HTML attribute.\n", int(attr.name_len), attr.name_p);
	print_warn_codeView(pos, attr.name());
}


void warn_ignored_attribute(const Node& node, const Attr& attr){
	linepos pos = findLine(node.root(), attr.name_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Ignored attribute " PURPLE("`%.*s`") ".\n", int(attr.name_len), attr.name_p);
	print_warn_codeView(pos, attr.name());
}

void warn_ignored_attr_value(const Node& node, const Attr& attr){
	assert(attr.value_p != nullptr);
	linepos pos = findLine(node.root(), attr.value_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Value of attribute " PURPLE("`%.*s`") " in ignored.\n", int(attr.name_len), attr.name_p);
	print_warn_codeView(pos, inclusiveValue(attr));
}


void warn_attr_double_quote(const Node& node, const Attr& attr){
	assert(attr.value_p != nullptr);
	linepos pos = findLine(node.root(), attr.value_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Attribute " PURPLE("`%.*s`") " expected single quotes. Value is always interpreted as an expression.\n", int(attr.name_len), attr.name_p);
	print_warn_codeView(pos, inclusiveValue(attr));
}


void warn_shell_exit(const Node& node, int i){
	linepos pos = findLine(node.root(), node.value_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Shell existed with status (%d).\n", i);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_newline(const Node& node, const char* p){
	linepos pos = findLine(node.root(), p);
	print_error_pfx(pos);
	fprintf(stderr, "Unexpected newline.\n");
	print_error_codeView(pos, string_view(p, 1));
}


void warn_child_ignored(const Node& node){
	const Document& doc = node.root();
	
	linepos pos = findLine(doc, node.value_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Content of node " PURPLE("`%.*s`") " ignored.\n", int(node.value_len), node.value_p);
	
	if (node.child != nullptr){
		string_view mark = trimWhitespace(node.child->value());
		pos = findLine(doc, mark.begin());
		print_warn_codeView(pos, mark);
	}
}


// ------------------------------------------------------------------------------------------ //