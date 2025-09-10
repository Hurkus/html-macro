#pragma once
#include <string_view>


// ----------------------------------- [ Structures ] --------------------------------------- //


struct linepos {
	const char* file = nullptr;
	std::string_view line;
	long row = -1;
	long col = -1;
};

// Functor for passing debug information to functions
class LineDebugger {
	public:
	virtual linepos line(const char* p) const noexcept { return {}; }
	virtual std::string_view mark() const noexcept { return {}; }
};


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