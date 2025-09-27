#pragma once
#include <string_view>
#include "Debug.hpp"


// ----------------------------------- [ Structures ] --------------------------------------- //


struct linepos {
	const char* file = nullptr;
	std::string_view line;
	long row = -1;
	long col = -1;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline void print(const linepos& lp){
	printErrSrc(lp.file, lp.row, lp.col);
}


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
 * @brief Print line of source code to stderr and underline marked focus section.
 *        Usefull for printing parsing errors.
 * @param line Line position information. Use `findLine()`.
 * @param mark Substring of `line` which should be marked.
 * @param color ANSI color code for marked section.
 */
void printCodeView(const linepos& line, std::string_view mark, std::string_view color);


// ------------------------------------------------------------------------------------------ //