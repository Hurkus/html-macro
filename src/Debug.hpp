#pragma once
#include <iostream>
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


template <typename ...T>
static void error(const char* fmt, T... arg){
	std::fprintf(stderr, ANSI_RED "error: " ANSI_RESET);
	std::fprintf(stderr, fmt, arg...);
	std::fprintf(stderr, "\n");
}

template <typename ...T>
static void warn(const char* fmt, T... arg){
	std::fprintf(stderr, ANSI_YELLOW ANSI_BOLD "warn: " ANSI_RESET);
	std::fprintf(stderr, fmt, arg...);
	std::fprintf(stderr, "\n");
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


void error(const html::Node& node, std::string_view msg);
void warn(const html::Node& node, std::string_view msg);
void info(const html::Node& node, std::string_view msg);

void error_missing_attr(const html::Node& node, const char* name);
void error_missing_attr_value(const html::Node& node, const html::Attr& attr);
void warn_missing_attr(const html::Node& node, const char* name);
void warn_missing_attr_value(const html::Node& node, const html::Attr& attr);
void warn_ignored_attribute(const html::Node& node, const html::Attr& attr);
void warn_ignored_attr_value(const html::Node& node, const html::Attr& attr);
void error_duplicate_attr(const html::Node& node, const html::Attr& attr_1, const html::Attr& attr_2);
void warn_duplicate_attr(const html::Node& node, const html::Attr& attr_1, const html::Attr& attr_2);
void warn_attr_double_quote(const html::Node& node, const html::Attr& attr);

// CALL and INCLUDE
void error_macro_not_found(const html::Node& node, std::string_view mark, std::string_view name);
void error_file_not_found(const html::Node& node, std::string_view mark, const char* file);
void error_invalid_include_type(const html::Node& node, std::string_view mark);
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
