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


// ----------------------------------- [ Structures ] --------------------------------------- //


struct linepos {
	const char* file = nullptr;
	const char* beg = nullptr;		// Begining of line.
	const char* end = nullptr;		// End of line (at next '\n' or EOF).
	long row = -1;
	long col = -1;
};


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

void error_macro_not_found(const html::Node& node, const html::Attr& attr);
void error_macro_not_found(const html::Node& node, const html::Attr& attr, const char* name);
void error_file_not_found(const html::Node& node, const html::Attr& attr);
void error_file_not_found(const html::Node& node, const html::Attr& attr, const char* name);
void warn_file_include(const html::Node& node, const html::Attr& attr, const char* filePath);

void warn_unknown_macro_tag(const html::Node& node);
void warn_unknown_macro_attribute(const html::Node& node, const html::Attr& attr);
void warn_shell_exit(const html::Node& node, int status);

void warn_attr_double_quote(const html::Node& node, const html::Attr& attr);
void error_expression_parse(const html::Node& node, const html::Attr& attr);


// ----------------------------------- [ Functions ] ---------------------------------------- //


/**
 * @brief Find line containing `p` relative to beggining of the source buffer.
 *        This function is very slow (iterates over the whole buffer).
 * @note Recommended for error reporting only.
 * @param beg Beginning of the buffer containing `p`.
 * @param end End of the buffer containing `p`.
 * @param p Pointer to character between `beg` and `end` for which to find line information.
 * @return Line, row number of `p` and colum number of `p` or empty if not found.
 */
linepos findLine(const char* beg, const char* end, const char* p) noexcept;


/**
 * @brief Get line of source code and underline marked focus section.
 *        Usefull for printing parsing errors.
 * @param line Line position information. Use `findLine()`.
 * @param mark Substring of `line` which should be marked.
 * @param color ANSI color code for marked section.
 * @return std::string Marked view of source code ready for printing.
 */
std::string getCodeView(const linepos& line, std::string_view mark, std::string_view color);


// ------------------------------------------------------------------------------------------ //
