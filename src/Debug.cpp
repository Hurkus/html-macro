#include "Debug.hpp"
#include "Macro.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ---------------------------------- [ Definitions ] --------------------------------------- //


#define COLOR_ERROR	ANSI_RED
#define COLOR_WARN	ANSI_YELLOW

#define RESET	ANSI_RESET
#define BOLD 	ANSI_BOLD
#define PURPLE	ANSI_PURPLE


#define CNAME(node)		(string(node.name()).c_str())
#define CVALUE(node)	(string(node.value()).c_str())


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr string_view trim_whitespace(string_view s){
	const char* beg = s.begin();
	const char* end = beg + s.length();
	
	while (beg != end && isspace(beg[+0])) beg++;
	while (end != beg && isspace(end[-1])) end--;
	
	return string_view(beg, end);
}

constexpr string_view expand(string_view s, int padding){
	return string_view(s.begin() - padding, s.end() + padding);
}

constexpr string_view inclusiveValue(const Attr& attr){
	return string_view(attr.value_p - 1, attr.value_p + attr.value_len + 1);
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


linepos findLine(const char* beg, const char* end, const char* p) noexcept {
	if (beg == nullptr || end == nullptr || end <= beg || p == nullptr || p < beg || p >= end){
		return {};
	}
	
	long col = 1;
	long row = 1;
	bool tabs = false;
	const char* line_beg;
	
	// Find line begining and row
	const char* b = beg;
	while (p > b){
		if (*b == '\n'){
			row++;
			b++;
			line_beg = b;
			tabs = false;
			continue;
		} else if (*b == 0){
			return {};
		} else {
			tabs |= (*b == '\t');
		}
		b++;
	}
	
	// Find line ending
	b = p;
	while (*b != 0 && *b != '\n') b++;
	const char* line_end = b;
	
	// Find colum
	if (!tabs){
		col = p - line_beg + 1;
	} else {
		for (b = line_beg ; b != p ; b++){
			col++;
			if (*b == '\t')
				col = ((col + 2) & ~0b11L) + 1;
		}
	}
	
	return linepos {
		.beg = line_beg,
		.end = line_end,
		.row = row,
		.col = col,
	};
}


string getCodeView(const linepos& line, string_view mark, string_view color){
	string_view line_view = string_view(line.beg, line.end);
	if (line_view.empty()){
		return {};
	}
	
	const size_t capacity = (10 + 3)*2 + (line_view.length() + color.length() + sizeof(ANSI_RESET))*2;
	
	// Truncate mark
	mark = string_view(max(mark.begin(), line.beg), min(mark.end(), line.end));
	
	// Append line number
	string str = to_string(line.row);
	const size_t row_str_len = str.length();
	
	str.reserve(capacity);
	str.append(" | ");
	
	// Colorize mark
	if (!mark.empty()){
		str.append(line_view.begin(), mark.begin());
		str.append(color).append(mark).append(ANSI_RESET);
		str.append(mark.end(), line_view.end());
	} else {
		str.append(line_view);
	}
	
	// Convert tabs
	for (char& c : str){
		if (c == '\t')
			c = ' ';
	}
	
	// Add mark tick and squiggly underline
	str.push_back('\n');
	str.append(row_str_len, ' ').append(" | ");
	
	if (!mark.empty()){
		str.append(mark.begin() - line_view.begin(), ' ');
		str.append(color);
		str.push_back('^');
		str.append(mark.length() - 1, '~');
		str.append(ANSI_RESET);
	}
	
	// Assert capacity prediction
	assert(str.size() <= capacity && str.capacity() <= capacity);
	return str;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void print_file_loc(const linepos& pos){
	if (pos.row > 0 && pos.col > 0)
		fprintf(stderr, BOLD "%s:%ld:%ld: " RESET, pos.file, pos.row, pos.col);
	else if (pos.row > 0)
		fprintf(stderr, BOLD "%s:%ld: " RESET, pos.file, pos.row);
	else
		fprintf(stderr, BOLD "%s: " RESET, pos.file);
}


static void print_error_pfx(const linepos& pos){
	if (pos.row > 0 && pos.col > 0)
		fprintf(stderr, BOLD "%s:%ld:%ld: " COLOR_ERROR "error: " RESET, pos.file, pos.row, pos.col);
	else if (pos.row > 0)
		fprintf(stderr, BOLD "%s:%ld: " COLOR_ERROR "error: " RESET, pos.file, pos.row);
	else
		fprintf(stderr, BOLD "%s: " COLOR_ERROR "error: " RESET, pos.file);
}

static void print_warn_pfx(const linepos& pos){
	if (pos.row > 0 && pos.col > 0)
		fprintf(stderr, BOLD "%s:%ld:%ld: " COLOR_WARN "warn: " RESET, pos.file, pos.row, pos.col);
	else if (pos.row > 0)
		fprintf(stderr, BOLD "%s:%ld: " COLOR_WARN "warn: " RESET, pos.file, pos.row);
	else
		fprintf(stderr, BOLD "%s: " COLOR_WARN "warn: " RESET, pos.file);
}

static void print_info_pfx(const linepos& pos){
	print_file_loc(pos);
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
	print_info_pfx(findLine(node.root(), node.value_p));
	fprintf(stderr, "%s\n", msg);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_missing_attr(const Node& node, const char* attr_name){
	linepos pos = findLine(node.root(), node.value_p);
	print_error_pfx(pos);
	fprintf(stderr, "Tag <%s> missing attribute '" PURPLE "%s" RESET "'.\n", CNAME(node), attr_name);
	print_error_codeView(pos, node.name());
}

void warn_missing_attr(const Node& node, const char* attr_name){
	linepos pos = findLine(node.root(), node.value_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Tag <%s> missing attribute '" PURPLE "%s" RESET "'.\n", CNAME(node), attr_name);
	print_warn_codeView(pos, node.name());
}


void error_missing_attr_value(const Node& node, const Attr& attr){
	const Document& doc = node.root();
	linepos pos = findLine(doc, attr.name_p);
	
	print_error_pfx(pos);
	fprintf(stderr, "Attribute " PURPLE "'%s'" RESET " missing value.\n", CNAME(attr));
	
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
	fprintf(stderr, "Attribute " PURPLE "'%s'" RESET " missing value.\n", CNAME(attr));
	
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
	fprintf(stderr, "Duplicate attribute " ANSI_PURPLE "'%s'" ANSI_RESET ".\n", CNAME(attr_1));
	print_error_codeView(pos1, attr_1.name());
	
	print_error_pfx(pos2);
	fprintf(stderr, "Duplicate attribute " ANSI_PURPLE "'%s'" ANSI_RESET ".\n", CNAME(attr_2));
	print_error_codeView(pos1, attr_2.name());
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_macro_not_found(const Node& node, const Attr& attr, const char* macroName){
	linepos pos = findLine(node.root(), attr.value_p);
	print_error_pfx(pos);
	fprintf(stderr, "Macro " PURPLE "'%s'" RESET " not found.\n", macroName);
	print_error_codeView(pos, inclusiveValue(attr));
	
}

void error_macro_not_found(const Node& node, const Attr& attr){
	error_macro_not_found(node, attr, CVALUE(attr));
}


void error_file_not_found(const Node& node, const Attr& attr, const char* fileName){
	linepos pos = findLine(node.root(), attr.value_p);
	print_error_pfx(pos);
	fprintf(stderr, "File " PURPLE "'%s'" RESET " not found.\n", fileName);
	print_error_codeView(pos, inclusiveValue(attr));
}

void error_file_not_found(const Node& node, const Attr& attr){
	error_file_not_found(node, attr, CVALUE(attr));
}


void warn_file_include(const Node& node, const Attr& attr, const char* fileName){
	linepos pos = findLine(node.root(), attr.value_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Failed to include file " PURPLE "'%s'" RESET ".\n", fileName);
	print_warn_codeView(pos, inclusiveValue(attr));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void error_expression_parse(const Node& node, const Attr& attr){
	linepos pos = findLine(node.root(), attr.value_p);
	print_error_pfx(pos);
	fprintf(stderr, "Failed to parse expression.\n");
	print_error_codeView(pos, inclusiveValue(attr));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void warn_unknown_macro_tag(const Node& node){
	linepos pos = findLine(node.root(), node.value_p);
	print_error_pfx(pos);
	fprintf(stderr, "Unknown macro " PURPLE "<%s>" RESET " treated as regular HTML tag.\n", CNAME(node));
	print_warn_codeView(pos, node.name());
}

void warn_unknown_macro_attribute(const Node& node, const Attr& attr){
	linepos pos = findLine(node.root(), attr.name_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Unknown macro attribute " PURPLE "'%s'" RESET " treated as regular HTML attribute.\n", CNAME(attr));
	print_warn_codeView(pos, attr.name());
}


void warn_ignored_attribute(const Node& node, const Attr& attr){
	linepos pos = findLine(node.root(), attr.name_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Ignored attribute " PURPLE "'%s'" RESET ".\n", CNAME(attr));
	print_warn_codeView(pos, attr.name());
}

void warn_ignored_attr_value(const Node& node, const Attr& attr){
	assert(attr.value_p != nullptr);
	linepos pos = findLine(node.root(), attr.value_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Value of attribute " PURPLE "%s" RESET " in ignored.\n", CNAME(attr));
	print_warn_codeView(pos, inclusiveValue(attr));
}


void warn_attr_double_quote(const Node& node, const Attr& attr){
	assert(attr.value_p != nullptr);
	linepos pos = findLine(node.root(), attr.value_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Attribute " PURPLE "%s" RESET " expected single quotes. Value is always interpreted as an expression.\n", CNAME(attr));
	print_warn_codeView(pos, inclusiveValue(attr));
}


void warn_shell_exit(const Node& node, int i){
	linepos pos = findLine(node.root(), node.value_p);
	print_warn_pfx(pos);
	fprintf(stderr, "Shell existed with status (%d).\n", i);
}


// ------------------------------------------------------------------------------------------ //