#pragma once
#include <iostream>
#include "ANSI.h"

namespace html {
	struct Node;
	struct Attr;
};


// ---------------------------------- [ Definitions ] --------------------------------------- //


#ifdef NDEBUG
	#undef DEBUG
#else
	#define DEBUG
#endif


// ---------------------------------- [ Definitions ] --------------------------------------- //


#define PRINT_HERE(...)	\
	std::fprintf(stderr, ANSI_BOLD "[%s:%ld]" ANSI_RESET "\n ", __FILE__, size_t(__LINE__))


#ifdef DEBUG
	#define ERROR(...)	{\
		PRINT_HERE(); \
		::error(__VA_ARGS__); \
	}
	#define WARN(...)	{\
		PRINT_HERE(); \
		::warn(__VA_ARGS__); \
	}
	#define INFO(...)	{\
		PRINT_HERE(); \
		::info(__VA_ARGS__); \
	}
#else
	#define ERROR(...)	::error(__VA_ARGS__)
	#define WARN(...)	::warn(__VA_ARGS__)
	#define INFO(...)	::info(__VA_ARGS__)
#endif


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


void error(const html::Node& node, const char* msg);
void warn(const html::Node& node, const char* msg);
void info(const html::Node& node, const char* msg);

void error_missing_attr(const html::Node& node, const char* name);
void warn_missing_attr(const html::Node& node, const char* name);
void warn_missing_attr_value(const html::Node& node, const html::Attr& attr);

void warn_unknown_macro(const html::Node& node);
void warn_unknown_attribute(const html::Node& node, const html::Attr& attr);
void warn_ignored_attribute(const html::Node& node, const html::Attr& attr);
void warn_macro_not_found(const html::Node& node, const html::Attr& attr);
void warn_double_quote(const html::Node& node, const html::Attr& attr);
void warn_ignored_attr_value(const html::Node& node, const html::Attr& attr);

void error_expression_parse(const html::Node& node, const html::Attr& attr);
void error_duplicate_attr(const html::Node& node, const html::Attr& attr_1, const html::Attr& attr_2);

void warn_shell_exit(const html::Node& node, int status);

// ------------------------------------------------------------------------------------------ //
