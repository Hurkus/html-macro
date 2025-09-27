#include "DebugSource.hpp"
#include "Debug.hpp"
#include <cassert>
#include <string>
#include "ANSI.h"

using namespace std;


// ----------------------------------- [ Functions ] ---------------------------------------- //


linepos findLine(const char* beg, const char* end, const char* p) noexcept {
	if (beg == nullptr || end == nullptr || end <= beg || p == nullptr || p < beg || p >= end){
		return {};
	}
	
	long col = 1;
	long row = 1;
	bool tabs = false;
	const char* line_beg = beg;
	
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
		.line = string_view(line_beg, line_end),
		.row = row,
		.col = col,
	};
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void printCodeView(const linepos& line, string_view mark, string_view color){
	string_view linev = line.line;
	if (linev.empty()){
		return;
	}
	
	// Clip line
	if (linev.length() > 100){
		long left = mark.begin() - linev.begin();
		if (left > 4)
			linev.remove_prefix(left - 4);
		if (linev.length() > 100)
			linev = linev.substr(0, 100);
	}
	
	// Truncate mark
	{
		const char* a = max(mark.begin(), linev.begin());
		const char* b = min(mark.end(), linev.end());
		mark = string_view(a, max(a, b));
	}
	
	// Append line number
	fprintf(stderr, "%5ld | ", line.row);
	
	// Colorize mark
	if (!mark.empty()){
		fwrite(linev.begin(), sizeof(char), mark.begin() - linev.begin(), stderr);
		
		if (stderr_isTTY){
			fwrite(color.begin(), sizeof(char), color.length(), stderr);
			fwrite(mark.begin(), sizeof(char), mark.length(), stderr);
			fwrite(ANSI_RESET, sizeof(char), sizeof(ANSI_RESET) - 1, stderr);
		} else {
			fwrite(mark.begin(), sizeof(char), mark.length(), stderr);
		}
		
		fwrite(mark.end(), sizeof(char), linev.end() - mark.end(), stderr);
	} else {
		fwrite(linev.begin(), sizeof(char), linev.length(), stderr);
	}
	
	// // Convert tabs
	// for (char& c : str){
	// 	if (c == '\t')
	// 		c = ' ';
	// }
	
	
	// Add mark tick and squiggly underline
	fprintf(stderr, "\n      | %*s",  int(mark.begin() - linev.begin()), "");
	if (stderr_isTTY){
		fwrite(color.data(), sizeof(char), color.length(), stderr);
	}
	
	fputc('^', stderr);
	for (int i = 0 ; i < int(mark.length()) - 1 ; i++){
		fputc('~', stderr);
	}
	
	if (stderr_isTTY){
		fwrite(ANSI_RESET, sizeof(char), sizeof(ANSI_RESET) - 1, stderr);
	}
	
	fputc('\n', stderr);
	fflush(stderr);
}


// ------------------------------------------------------------------------------------------ //