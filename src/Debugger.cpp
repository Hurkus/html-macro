#include "Debugger.hpp"
#include "DebugSource.hpp"
#include <iostream>
#include <cstdarg>

#include "html.hpp"
#include "ANSI.h"

using namespace std;


// ----------------------------------- [ Functions ] ---------------------------------------- //


#define E_PRINTF(fmt) {\
	va_list argptr; \
	va_start(argptr, fmt); \
	vfprintf(stderr, fmt, argptr); \
	va_end(argptr); \
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void Debugger::error(string_view mark, const char* fmt, ...) const {
	if (mark.empty()){ simple:
		fprintf(stderr, ANSI_RED "error: " ANSI_RESET);
		E_PRINTF(fmt);
		return;
	}
	
	const char* file = nullptr;
	string_view buffer = {};
	source(file, buffer);
	
	if (file == nullptr){
		goto simple;
	}
	
	linepos lp = findLine(buffer.begin(), buffer.end(), mark.begin());
	lp.file = file;
	
	print(lp);
	fprintf(stderr, ANSI_RED "error: " ANSI_RESET);
	E_PRINTF(fmt);
	
	if (!buffer.empty()){
		string cv = getCodeView(lp, mark, ANSI_RED);
		fprintf(stderr, "%s\n", cv.c_str());
	}
	
}


void Debugger::warn(string_view mark, const char* fmt, ...) const {
	if (mark.empty()){ simple:
		fprintf(stderr, ANSI_YELLOW "warn: " ANSI_RESET);
		E_PRINTF(fmt);
		return;
	}
	
	const char* file = nullptr;
	string_view buffer = {};
	source(file, buffer);
	
	if (file == nullptr){
		goto simple;
	}
	
	linepos lp = findLine(buffer.begin(), buffer.end(), mark.begin());
	print(lp);
	fprintf(stderr, ANSI_YELLOW "warn: " ANSI_RESET);
	E_PRINTF(fmt);
	
	if (!buffer.empty()){
		string cv = getCodeView(lp, mark, ANSI_YELLOW);
		fprintf(stderr, "%s\n", cv.c_str());
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void NodeDebugger::source(const char*& file, string_view& buffer) const {
	const html::Document& doc = node.root();
	if (doc.srcFile != nullptr)
		file = doc.srcFile->c_str();
	if (doc.buffer != nullptr)
		buffer = string_view(*doc.buffer);
}


string_view NodeDebugger::mark() const {
	return node.value();
}


// ------------------------------------------------------------------------------------------ //
