#pragma once
#include <iostream>
#include "Debug-Line.hpp"
#include "ANSI.h"


// ---------------------------------- [ Definitions ] --------------------------------------- //


#ifdef NDEBUG
	#undef DEBUG
#else
	#define DEBUG
#endif


// ---------------------------------- [ Definitions ] --------------------------------------- //


#ifdef DEBUG
	#define HERE(...)	{\
		std::fprintf(stderr, ANSI_BOLD "[%s:%ld]" ANSI_RESET "\n", __FILE__, size_t(__LINE__)); \
		__VA_ARGS__; \
	}
#else
	#define HERE(...) { __VA_ARGS__; }
#endif


#define ERROR(...)	HERE(::error(__VA_ARGS__))
#define WARN(...)	HERE(::warn(__VA_ARGS__))
#define INFO(...)	HERE(::info(__VA_ARGS__))


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void print(const linepos& pos){
	if (pos.row > 0 && pos.col > 0)
		fprintf(stderr, ANSI_BOLD "%s:%ld:%ld: " ANSI_RESET, pos.file, pos.row, pos.col);
	else if (pos.row > 0)
		fprintf(stderr, ANSI_BOLD "%s:%ld: " ANSI_RESET, pos.file, pos.row);
	else
		fprintf(stderr, ANSI_BOLD "%s: " ANSI_RESET, pos.file);
}


template <typename ...T>
static void error(const char* fmt, T... arg){
	std::fprintf(stderr, ANSI_RED "error: " ANSI_RESET);
	std::fprintf(stderr, fmt, arg...);
	std::fprintf(stderr, "\n");
}

template <typename ...T>
static void error(const linepos& line, const char* fmt, T... arg){
	print(line);
	error(fmt, std::forward<T>(arg)...);
}

template <typename ...T>
static void warn(const char* fmt, T... arg){
	std::fprintf(stderr, ANSI_YELLOW ANSI_BOLD "warn: " ANSI_RESET);
	std::fprintf(stderr, fmt, arg...);
	std::fprintf(stderr, "\n");
}

template <typename ...T>
static void warn(const linepos& line, const char* fmt, T... arg){
	print(line);
	warn(fmt, std::forward<T>(arg)...);
}

template <typename ...T>
static void info(const char* fmt, T... arg){
	std::fprintf(stderr, fmt, arg...);
	std::fprintf(stderr, "\n");
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


namespace html {
	struct Node;
	struct Attr;
};

namespace Expression {
	struct ParseResult;
};


void error(const html::Node& node, const char* msg);
void warn(const html::Node& node, const char* msg);
void info(const html::Node& node, const char* msg);

void error_missing_attr(const html::Node& node, const char* name);
void error_missing_attr_value(const html::Node& node, const html::Attr& attr);
void warn_missing_attr(const html::Node& node, const char* name);
void warn_missing_attr_value(const html::Node& node, const html::Attr& attr);
void warn_ignored_attribute(const html::Node& node, const html::Attr& attr);
void warn_ignored_attr_value(const html::Node& node, const html::Attr& attr);
void error_duplicate_attr(const html::Node& node, const html::Attr& attr_1, const html::Attr& attr_2);
void warn_attr_double_quote(const html::Node& node, const html::Attr& attr);

void error_macro_not_found(const html::Node& node, const html::Attr& attr);
void error_macro_not_found(const html::Node& node, const html::Attr& attr, const char* name);
void error_file_not_found(const html::Node& node, const html::Attr& attr);
void error_file_not_found(const html::Node& node, const html::Attr& attr, const char* name);
void warn_file_include(const html::Node& node, const html::Attr& attr, const char* filePath);

void warn_unknown_macro_tag(const html::Node& node);
void warn_unknown_macro_attribute(const html::Node& node, const html::Attr& attr);
void warn_shell_exit(const html::Node& node, int status);

void error_expression_parse(const html::Node& node, const html::Attr& attr);
void error_expression_parse(const html::Node& node, const Expression::ParseResult& err);

void error_newline(const html::Node& node, const char* p);

void warn_expr_var_not_found(const linepos& pos, std::string_view mark, std::string_view name);


// ------------------------------------------------------------------------------------------ //
