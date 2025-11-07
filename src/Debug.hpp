#pragma once
#include <iostream>
#include "ANSI.h"
#include "ANSI_fmt.hpp"


// ---------------------------------- [ Definitions ] --------------------------------------- //


#ifdef NDEBUG
	#undef DEBUG
#else
	#define DEBUG
#endif


#ifdef DEBUG
	#define IN_DEBUG(statement)	statement
#else
	#define IN_DEBUG(statement)
#endif


// ---------------------------------- [ Definitions ] --------------------------------------- //


extern bool stderr_isTTY;
extern bool stdout_isTTY;

// Switch between ANSI text and plain text: `stderr_isTTY`
#define LOG_STDERR(fmt, ...) {                                  \
	static constexpr char f_str[] = fmt;                        \
	static constexpr fmt_ansi<f_str> f = {};                    \
	fprintf(stderr, f(stderr_isTTY) __VA_OPT__(,) __VA_ARGS__); \
}                                                               \

// Switch between ANSI text and plain text: `stdout_isTTY`
#define LOG_STDOUT(fmt, ...) {                                  \
	static constexpr char f_str[] = fmt;                        \
	static constexpr fmt_ansi<f_str> f = {};                    \
	fprintf(stderr, f(stdout_isTTY) __VA_OPT__(,) __VA_ARGS__); \
}                                                               \


// ---------------------------------- [ Definitions ] --------------------------------------- //


// Print source code location before executing argument
#ifdef DEBUG
	#define HERE(...) {                                                                      \
		if (stderr_isTTY)                                                                    \
			fprintf(stderr, ANSI_BOLD "[%s:%ld]" ANSI_RESET "\n", __FILE__, long(__LINE__)); \
		__VA_ARGS__;                                                                         \
	}
#else
	#define HERE(...) { __VA_ARGS__; }
#endif


#define ERROR(fmt, ...) {                    \
	HERE();                                   \
	LOG_STDERR(ANSI_RED "error: " ANSI_RESET); \
	LOG_STDERR(fmt __VA_OPT__(,) __VA_ARGS__); \
	LOG_STDERR("\n");                         \
}                                            \

#define WARN(fmt, ...) {                       \
	HERE();                                     \
	LOG_STDERR(ANSI_YELLOW "warn: " ANSI_RESET); \
	LOG_STDERR(fmt __VA_OPT__(,) __VA_ARGS__);  \
	LOG_STDERR("\n");                          \
}       

#define INFO(fmt, ...) {                       \
	HERE();                                     \
	LOG_STDERR(fmt __VA_OPT__(,) __VA_ARGS__);  \
	LOG_STDERR("\n");                          \
}                                             \


// ----------------------------------- [ Functions ] ---------------------------------------- //


void printErrSrc(const char* file, long row = 0, long col = 0) noexcept;
void printErrTag() noexcept;
void printWarnTag() noexcept;


// ----------------------------------- [ Functions ] ---------------------------------------- //


namespace html {
	struct Node;
	struct Attr;
};


void error_node(const html::Node& node, std::string_view msg);
void warn_node(const html::Node& node, std::string_view msg);
void info_node(const html::Node& node, std::string_view msg);

void error_missing_attr(const html::Node& node, const char* name);
void error_missing_attr_value(const html::Node& node, const html::Attr& attr);
void warn_missing_attr(const html::Node& node, const char* name);
void warn_missing_attr_value(const html::Node& node, const html::Attr& attr);
void warn_ignored_attribute(const html::Node& node, const html::Attr& attr);
void warn_ignored_attr_value(const html::Node& node, const html::Attr& attr);
void error_duplicate_attr(const html::Node& node, const html::Attr& attr_1, const html::Attr& attr_2);
void warn_duplicate_attr(const html::Node& node, const html::Attr& attr_1, const html::Attr& attr_2);
void warn_attr_single_quote(const html::Node& node, const html::Attr& attr);
void warn_attr_double_quote(const html::Node& node, const html::Attr& attr);

// CALL and INCLUDE
void error_macro_not_found(const html::Node& node, std::string_view mark, std::string_view name);
void error_file_not_found(const html::Node& node, std::string_view mark, const char* file);
void error_include_fail(const html::Node& node, std::string_view mark, const char* file);

void error_unsupported_type(const html::Node& node);
void warn_unknown_macro_tag(const html::Node& node);
void warn_unknown_macro_attribute(const html::Node& node, const html::Attr& attr);
void warn_shell_exit(const html::Node& node, int status);

void error_missing_preceeding_if_node(const html::Node& node);
void error_missing_preceeding_if_attr(const html::Node& node, const html::Attr& attr);
void error_undefined_variable(const html::Node& node, const std::string_view name);

void error_newline(const html::Node& node, const char* p);

void warn_text_missing(const html::Node& node);
void warn_child_ignored(const html::Node& node);


// ------------------------------------------------------------------------------------------ //
