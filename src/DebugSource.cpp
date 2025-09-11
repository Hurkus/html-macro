#include "DebugSource.hpp"
#include <cassert>
#include <string>
#include "ANSI.h"

using namespace std;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void print(const linepos& pos){
	if (pos.file != nullptr){
		if (pos.row > 0 && pos.col > 0)
			fprintf(stderr, ANSI_BOLD "%s:%ld:%ld: " ANSI_RESET, pos.file, pos.row, pos.col);
		else if (pos.row > 0)
			fprintf(stderr, ANSI_BOLD "%s:%ld: " ANSI_RESET, pos.file, pos.row);
		else
			fprintf(stderr, ANSI_BOLD "%s: " ANSI_RESET, pos.file);
	}
}


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


string getCodeView(const linepos& line, string_view mark, string_view color){
	string_view linev = line.line;
	if (linev.empty()){
		return {};
	}
	
	// Clip line
	if (linev.length() > 100){
		long left = mark.begin() - linev.begin();
		if (left > 4)
			linev.remove_prefix(left - 4);
		if (linev.length() > 100)
			linev = linev.substr(0, 100);
	}
	
	const size_t capacity = (10 + 3)*2 + (linev.length() + color.length() + sizeof(ANSI_RESET))*2 + 1;
	
	// Truncate mark
	const bool at_end = (linev.end() == mark.begin() && mark.length() > 0);
	mark = string_view(max(mark.begin(), linev.begin()), min(mark.end(), linev.end()));
	
	// Append line number
	string str = to_string(line.row);
	const size_t row_str_len = str.length();
	
	str.reserve(capacity);
	str.append(" | ");
	
	// Colorize mark
	if (!mark.empty()){
		str.append(linev.begin(), mark.begin());
		str.append(color).append(mark).append(ANSI_RESET);
		str.append(mark.end(), linev.end());
	} else {
		str.append(linev);
	}
	
	// Convert tabs
	for (char& c : str){
		if (c == '\t')
			c = ' ';
	}
	
	// Add mark tick and squiggly underline
	str.push_back('\n');
	str.append(row_str_len, ' ').append(" | ");
	
	if (at_end){
		str.append(mark.begin() - linev.begin(), ' ');
		str.append(color);
		str.push_back('^');
		str.append(ANSI_RESET);
	} else if (!mark.empty()){
		str.append(mark.begin() - linev.begin(), ' ');
		str.append(color);
		str.push_back('^');
		str.append(mark.length() - 1, '~');
		str.append(ANSI_RESET);
	}
	
	// Assert capacity prediction
	assert(str.size() <= capacity && str.capacity() <= capacity);
	return str;
}


// ------------------------------------------------------------------------------------------ //