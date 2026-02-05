#pragma once
#include <iostream>
#include "ANSI_fmt.hpp"


// ---------------------------------- [ Definitions ] --------------------------------------- //


#ifndef DEBUG
	#ifndef NDEBUG
		#define DEBUG 1
	#else
		#define DEBUG 0
	#endif
#endif


#ifndef DEBUG_LOCATION
	#define DEBUG_LOCATION DEBUG
#endif


#if DEBUG
	#define IN_DEBUG(statement)	do { statement; } while (false)
#else
	#define IN_DEBUG(statement)
#endif


// ---------------------------------- [ Definitions ] --------------------------------------- //


extern bool stderr_isTTY;
extern bool stdout_isTTY;

// Switch between ANSI text and plain text: `stderr_isTTY`
#define LOG_STDERR(format, ...) do {                              \
	static constexpr char fmt_str[] = format;                     \
	static constexpr fmt_ansi<fmt_str> fmt = {};                  \
	fprintf(stderr, fmt(stderr_isTTY) __VA_OPT__(,) __VA_ARGS__); \
} while (false)

// Switch between ANSI text and plain text: `stdout_isTTY`
#define LOG_STDOUT(format, ...) do {                              \
	static constexpr char fmt_str[] = format;                     \
	static constexpr fmt_ansi<fmt_str> fmt = {};                  \
	fprintf(stderr, fmt(stdout_isTTY) __VA_OPT__(,) __VA_ARGS__); \
} while (false)


// Quick unpack for `std::string_view` or `std::string` when using `%.*` format.
#define VA_STRV(str)	int(str.length()), str.data()


// ---------------------------------- [ Definitions ] --------------------------------------- //


// Print source code location before executing argument
#if DEBUG_LOCATION
	#define HERE(...) {                                                                      \
		if (stderr_isTTY)                                                                    \
			fprintf(stderr, ANSI_BOLD "[%s:%ld]" ANSI_RESET "\n", __FILE__, long(__LINE__)); \
		__VA_ARGS__;                                                                         \
	}
#else
	#define HERE(...) { __VA_ARGS__; }
#endif


#define ERROR_PFX	ANSI_BOLD ANSI_RED "error: " ANSI_RESET
#define WARN_PFX	ANSI_BOLD ANSI_YELLOW "warn: " ANSI_RESET


#define ERROR(fmt, ...) do {                   \
	HERE();                                    \
	LOG_STDERR(ERROR_PFX fmt __VA_OPT__(,) __VA_ARGS__); \
	LOG_STDERR("\n");                          \
} while (false)

#define WARN(fmt, ...) do {                    \
	HERE();                                    \
	LOG_STDERR(WARN_PFX fmt __VA_OPT__(,) __VA_ARGS__); \
	LOG_STDERR("\n");                          \
} while (false)

#define INFO(fmt, ...) do {                    \
	HERE();                                    \
	LOG_STDERR(fmt __VA_OPT__(,) __VA_ARGS__); \
	LOG_STDERR("\n");                          \
} while (false)


// ----------------------------------- [ Functions ] ---------------------------------------- //


class Macro;
namespace html {
	struct Node;
	struct Attr;
};


void error_node(const Macro& macro, const html::Node& node, std::string_view msg);
void warn_node(const Macro& macro, const html::Node& node, std::string_view msg);
void info_node(const Macro& macro, const html::Node& node, std::string_view msg);

// ---------------------------------------------------------------- //

// Child element of <tag> ignored.
void warn_ignored_child(const Macro& macro, const html::Node& node, const html::Node* child = nullptr);

// Element <tag> expected text content.
void error_missing_text(const Macro& macro, const html::Node& node);
void warn_missing_text(const Macro& macro, const html::Node& node);

// Tag <tag> missing attribute `attr`.
void error_missing_attr(const Macro& macro, const html::Node& node, std::string_view name);
void warn_missing_attr(const Macro& macro, const html::Node& node, std::string_view name);

// Attribute <attr> missing value.
void error_missing_attr_value(const Macro& macro, const html::Attr& attr);
void warn_missing_attr_value(const Macro& macro, const html::Attr& attr);

// Duplicate attribute `attr`.
void error_duplicate_attr(const Macro& macro, const html::Attr& attr_1, const html::Attr& attr_2);
void warn_duplicate_attr(const Macro& macro, const html::Attr& attr_1, const html::Attr& attr_2);

// Attribute `attr` ignored.
void warn_ignored_attr(const Macro& macro, const html::Attr& attr);
// Attribute `attr` value ignored.
void warn_ignored_attr_value(const Macro& macro, const html::Attr& attr);

// Attribute `attr` value expected "".
void warn_expected_attr_double_quote(const Macro& macro, const html::Attr& attr);
// Attribute `attr` value expected ''.
void warn_expected_attr_single_quote(const Macro& macro, const html::Attr& attr);

// ---------------------------------------------------------------- //

// Unsupported node type.
void error_unsupported_type(const Macro& macro, const html::Node& node);
void warn_unsupported_type(const Macro& macro, const html::Node& node);

// Macro <TAG> is unknown.
void warn_unknown_element_macro(const Macro& macro, const html::Node& node);
// Attribute macro `ATTR` is unknown.
void warn_unknown_attribute_macro(const Macro& macro, const html::Attr& attr);

// ---------------------------------------------------------------- //

// Shell exited with an error code (!= 0).
void warn_shell_exit(const Macro& macro, const html::Node& node, int status);

// Macro not found.
void error_macro_not_found(const Macro& macro, std::string_view name, std::string_view mark);

// Macro is not invokable.
void warn_macro_not_invokable(const Macro& macro, std::string_view name, std::string_view mark);
// Macro is not invokable.
void warn_macro_not_invokable(const Macro& macro, std::string_view name, std::string_view mark);

// File referenced by INCLUDE macro not found.
void error_include_file_not_found(const Macro& macro, const char* file, std::string_view mark);
// Included file could not be parsed as target type. Downcasting to txt file.
void warn_include_file_type_downcast(const Macro& macro, const char* file, std::string_view mark);
// Included file does not have required content.
void error_include_file_empty(const Macro& macro, const char* file, std::string_view mark);
// File include failed.
void error_include_file_fail(const Macro& macro, const char* file, std::string_view mark);

// An <IF> sibling node is missing.
void error_missing_preceeding_if_node(const Macro& macro, const html::Node& node);
// An IF=[] attribute on a sibling node is missing.
void error_missing_preceeding_if_attr(const Macro& macro, const html::Node& node, const html::Attr& attr);

// ---------------------------------------------------------------- //

// Expressions in string interpolations must not contain newlines.
void error_string_interpolation_newline(const Macro& macro, const char* nl_char);

// ------------------------------------------------------------------------------------------ //
