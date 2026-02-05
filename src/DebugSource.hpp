#pragma once
#include <string_view>
#include "Debug.hpp"


// ----------------------------------- [ Structures ] --------------------------------------- //


struct linepos {
	const char* file = nullptr;
	std::string_view line;
	size_t row = 0;
	size_t col = 0;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline void print(const linepos& lp){
	if (lp.file != nullptr && lp.file[0] != 0){
		if (lp.row > 0 && lp.col > 0)
			LOG_STDERR(BOLD("%s:%ld:%ld: "), lp.file, lp.row, lp.col);
		else if (lp.row > 0)
			LOG_STDERR(BOLD("%s:%ld: "), lp.file, lp.row);
		else
			LOG_STDERR(BOLD("%s: "), lp.file);
	}
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
 * @brief Find line containing `p` relative to beggining of the macro's source `txt` buffer.
 *        This function is very slow (iterates over the whole buffer).
 * @note Recommended for error reporting only.
 * @param macro The macro holding the buffer from which `p` originates.
 * @param p Pointer to character within the `macro.txt` buffer for which to find line information.
 * @return Line, row number of `p` and colum number of `p` or empty if not found.
 */
linepos findLine(const Macro& macro, const char* p) noexcept;


/**
 * @brief Print line of source code to stderr and underline marked focus section.
 *        Usefull for printing parsing errors.
 * @param line Line position information. Use `findLine()`.
 * @param mark Substring of `line` which should be marked.
 * @param color ANSI color code for marked section.
 */
void printCodeView(const linepos& line, std::string_view mark, std::string_view color);


// ------------------------------------------------------------------------------------------ //