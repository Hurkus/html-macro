#include "Debug.hpp"
#include "DebugSource.hpp"
#include "Macro.hpp"
#include "Expression.hpp"

using namespace std;
using namespace html;


// ----------------------------------- [ Variables ] ---------------------------------------- //


bool stderr_isTTY = false;
bool stdout_isTTY = false;


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr bool isWhitespace(char c){
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static string_view trim(string_view s){
	const char* beg = s.begin();
	const char* end = s.end();
	
	while (beg != end && isWhitespace(beg[+0])) beg++;
	while (end != beg && isWhitespace(end[-1])) end--;
	
	return string_view(beg, end);
}


constexpr string_view quotedValue(const linepos& line, const Attr& attr){
	const char* left = line.line.begin();
	const char* right = line.line.end();
	const char* beg = attr.value_p;
	const char* end = attr.value_p + attr.value_len;
	
	if (left <= (beg - 1) && (end + 1) < right){
		beg -= 1;
		end += 1;
	}
	
	return string_view(beg, end);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void print_error_pfx(const linepos& pos){
	print(pos);
	LOG_STDERR(ANSI_BOLD ANSI_RED "error: " ANSI_RESET);
}

static void print_warn_pfx(const linepos& pos){
	print(pos);
	LOG_STDERR(ANSI_BOLD ANSI_YELLOW "warn: " ANSI_RESET);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_node(const Macro& macro, const Node& node, string_view msg){
	print_error_pfx(findLine(macro, node.value_p));
	LOG_STDERR("%.*s\n", VA_STRV(msg));
}

void warn_node(const Macro& macro, const Node& node, string_view msg){
	print_warn_pfx(findLine(macro, node.value_p));
	LOG_STDERR("%.*s\n", VA_STRV(msg));
}

void info_node(const Macro& macro, const Node& node, string_view msg){
	print(findLine(macro, node.value_p));
	LOG_STDERR("%.*s\n", VA_STRV(msg));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void warn_ignored_child(const Macro& macro, const Node& node, const Node* child){
	child = (child == nullptr) ? node.child : child;
	assert(child != nullptr);
	
	linepos pos = findLine(macro, child->value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Child element of " PURPLE("<%.*s>") " ignored.\n", VA_STRV(node.name()));
	
	string_view mark = trim(child->value());
	pos = findLine(macro, mark.begin());
	printCodeView(pos, mark, ANSI_YELLOW);
}


void error_missing_text(const Macro& macro, const Node& node){
	linepos pos = findLine(macro, node.value_p);
	print_error_pfx(pos);
	LOG_STDERR("Element " PURPLE("<%.*s>") " requires text content.\n", VA_STRV(node.name()));
	printCodeView(pos, node.name(), ANSI_RED);
}

void warn_missing_text(const Macro& macro, const Node& node){
	linepos pos = findLine(macro, node.value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Element " PURPLE("<%.*s>") " requires text content.\n", VA_STRV(node.name()));
	printCodeView(pos, node.name(), ANSI_YELLOW);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_missing_attr(const Macro& macro, const Node& node, string_view name){
	linepos pos = findLine(macro, node.value_p);
	print_error_pfx(pos);
	LOG_STDERR("Element " PURPLE("<%.*s>") " missing attribute " PURPLE("`%.*s`") ".\n", VA_STRV(node.name()), VA_STRV(name));
	printCodeView(pos, node.name(), ANSI_RED);
}

void warn_missing_attr(const Macro& macro, const Node& node, string_view name){
	linepos pos = findLine(macro, node.value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Element " PURPLE("<%.*s>") " missing attribute " PURPLE("`%.*s`") ".\n", VA_STRV(node.name()), VA_STRV(name));
	printCodeView(pos, node.name(), ANSI_YELLOW);
}


void error_missing_attr_value(const Macro& macro, const Attr& attr){
	if (attr.value_p != nullptr){
		linepos pos = findLine(macro, attr.value_p);
		print_error_pfx(pos);
		LOG_STDERR("Attribute " PURPLE("`%.*s`") " missing value.\n", VA_STRV(attr.name()));
		printCodeView(pos, quotedValue(pos, attr), ANSI_RED);
	} else {
		linepos pos = findLine(macro, attr.name_p);
		print_error_pfx(pos);
		LOG_STDERR("Attribute " PURPLE("`%.*s`") " missing value.\n", VA_STRV(attr.name()));
		printCodeView(pos, attr.name(), ANSI_RED);
	}
}

void warn_missing_attr_value(const Macro& macro, const Attr& attr){
	if (attr.value_p != nullptr){
		linepos pos = findLine(macro, attr.value_p);
		print_warn_pfx(pos);
		LOG_STDERR("Attribute " PURPLE("`%.*s`") " missing value.\n", VA_STRV(attr.name()));
		printCodeView(pos, quotedValue(pos, attr), ANSI_YELLOW);
	} else {
		linepos pos = findLine(macro, attr.name_p);
		print_warn_pfx(pos);
		LOG_STDERR("Attribute " PURPLE("`%.*s`") " missing value.\n", VA_STRV(attr.name()));
		printCodeView(pos, attr.name(), ANSI_YELLOW);
	}
}


void error_duplicate_attr(const Macro& macro, const Attr& attr_1, const Attr& attr_2){
	linepos pos1 = findLine(macro, attr_1.name_p);
	linepos pos2 = findLine(macro, attr_2.name_p);
	
	print_error_pfx(pos1);
	LOG_STDERR("Duplicate attribute " PURPLE("`%.*s`") ".\n", VA_STRV(attr_1.name()));
	printCodeView(pos1, attr_1.name(), ANSI_RED);
	
	print_error_pfx(pos2);
	LOG_STDERR("Duplicate attribute " PURPLE("`%.*s`") ".\n", VA_STRV(attr_2.name()));
	printCodeView(pos1, attr_2.name(), ANSI_RED);
}

void warn_duplicate_attr(const Macro& macro, const Attr& attr_1, const Attr& attr_2){
	linepos pos1 = findLine(macro, attr_1.name_p);
	linepos pos2 = findLine(macro, attr_2.name_p);
	
	print_warn_pfx(pos1);
	LOG_STDERR("Duplicate attribute " PURPLE("`%.*s`") ".\n", VA_STRV(attr_1.name()));
	printCodeView(pos1, attr_1.name(), ANSI_YELLOW);
	
	print_warn_pfx(pos2);
	LOG_STDERR("Duplicate attribute " PURPLE("`%.*s`") ".\n", VA_STRV(attr_2.name()));
	printCodeView(pos1, attr_2.name(), ANSI_YELLOW);
}


void warn_ignored_attr(const Macro& macro, const Attr& attr){
	linepos pos = findLine(macro, attr.name_p);
	print_warn_pfx(pos);
	LOG_STDERR("Ignored attribute " PURPLE("`%.*s`") ".\n", VA_STRV(attr.name()));
	printCodeView(pos, attr.name(), ANSI_YELLOW);
}


void warn_ignored_attr_value(const Macro& macro, const Attr& attr){
	linepos pos = findLine(macro, attr.value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Value of attribute " PURPLE("`%.*s`") " is ignored.\n", VA_STRV(attr.name()));
	printCodeView(pos, quotedValue(pos, attr), ANSI_YELLOW);
}


void warn_expected_attr_double_quote(const Macro& macro, const Attr& attr){
	linepos pos = findLine(macro, attr.value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Attribute " PURPLE("`%.*s`") " expected double quotes (" PURPLE("\"\"") "). Value is always interpreted as a string.\n", VA_STRV(attr.name()));
	printCodeView(pos, quotedValue(pos, attr), ANSI_YELLOW);
}


void warn_expected_attr_single_quote(const Macro& macro, const Attr& attr){
	linepos pos = findLine(macro, attr.value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Attribute " PURPLE("`%.*s`") " expected single quotes (" PURPLE("''") "). Value is always interpreted as an expression.\n", VA_STRV(attr.name()));
	printCodeView(pos, quotedValue(pos, attr), ANSI_YELLOW);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_unsupported_type(const Macro& macro, const Node& node){
	linepos pos = findLine(macro, node.value_p);
	print_error_pfx(pos);
	LOG_STDERR("Unsupported tag type: " PURPLE("%u") "\n", (unsigned int)node.type);
	printCodeView(pos, node.name(), ANSI_RED);
}

void warn_unsupported_type(const Macro& macro, const Node& node){
	linepos pos = findLine(macro, node.value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Unsupported tag type: " PURPLE("%u") "\n", (unsigned int)node.type);
	printCodeView(pos, node.name(), ANSI_YELLOW);
}


void warn_unknown_element_macro(const Macro& macro, const Node& node){
	linepos pos = findLine(macro, node.value_p);
	print_error_pfx(pos);
	LOG_STDERR("Unknown element macro " PURPLE("<%.*s>") " treated as regular HTML tag.\n", VA_STRV(node.name()));
	printCodeView(pos, node.name(), ANSI_YELLOW);
}

void warn_unknown_attribute_macro(const Macro& macro, const Attr& attr){
	linepos pos = findLine(macro, attr.name_p);
	print_warn_pfx(pos);
	LOG_STDERR("Unknown attribute macro " PURPLE("`%.*s`") " treated as regular HTML attribute.\n", VA_STRV(attr.name()));
	printCodeView(pos, attr.name(), ANSI_YELLOW);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void warn_shell_exit(const Macro& macro, const Node& node, int i){
	linepos pos = findLine(macro, node.value_p);
	print_warn_pfx(pos);
	LOG_STDERR("Shell existed with status: " PURPLE("%i") "\n", i);
}


void error_macro_not_found(const Macro& macro, string_view name, string_view mark){
	linepos pos = findLine(macro, &mark[0]);
	print_error_pfx(pos);
	LOG_STDERR("Macro " PURPLE("`%.*s`") " not found.\n", VA_STRV(name));
	printCodeView(pos, mark, ANSI_RED);
}


void warn_macro_not_invokable(const Macro& macro, string_view name, string_view mark){
	linepos pos = findLine(macro, &mark[0]);
	print_warn_pfx(pos);
	LOG_STDERR("Macro " PURPLE("`%.*s`") " cannot be invoked.\n", VA_STRV(name));
	printCodeView(pos, mark, ANSI_YELLOW);
}


void error_include_file_not_found(const Macro& macro, const char* file, string_view mark){
	linepos pos = findLine(macro, &mark[0]);
	print_error_pfx(pos);
	LOG_STDERR("File " PURPLE("`%s`") " not found.\n", file);
	printCodeView(pos, mark, ANSI_RED);
}

void warn_include_file_type_downcast(const Macro& macro, const char* file, string_view mark){
	linepos pos = findLine(macro, &mark[0]);
	print_warn_pfx(pos);
	LOG_STDERR("File " PURPLE("`%s`") " could not be included as their target type. File included as a plain text file.\n", file);
	printCodeView(pos, mark, ANSI_YELLOW);
}

void error_include_file_empty(const Macro& macro, const char* file, string_view mark){
	linepos pos = findLine(macro, &mark[0]);
	print_error_pfx(pos);
	LOG_STDERR("File " PURPLE("`%s`") " could not be included. File content missing.\n", file);
	printCodeView(pos, mark, ANSI_RED);
}

void error_include_file_fail(const Macro& macro, const char* file, string_view mark){
	linepos pos = findLine(macro, &mark[0]);
	print_error_pfx(pos);
	LOG_STDERR("File " PURPLE("`%s`") " could not be included.\n", file);
	printCodeView(pos, mark, ANSI_RED);
}


void error_missing_preceeding_if_node(const Macro& macro, const Node& node){
	linepos pos = findLine(macro, node.value_p);
	print_error_pfx(pos);
	LOG_STDERR("Missing preceeding " PURPLE("<IF>") " macro.\n");
	printCodeView(pos, node.value_p, ANSI_RED);
}


void error_missing_preceeding_if_attr(const Macro& macro, const Node& node, const Attr& attr){
	linepos pos = findLine(macro, attr.name_p);
	print_error_pfx(pos);
	LOG_STDERR("Missing " PURPLE("`IF`") " attribute on a preceeding sibling element.\n");
	printCodeView(pos, attr.name(), ANSI_RED);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_string_interpolation_newline(const Macro& macro, const char* p){
	linepos pos = findLine(macro, p);
	print_error_pfx(pos);
	LOG_STDERR("Unexpected newline `" PURPLE("\\n") "` in expression of a interpolated string.\n");
	printCodeView(pos, string_view(p, 1), ANSI_RED);
}


// ------------------------------------------------------------------------------------------ //