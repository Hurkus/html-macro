#pragma once
#include <string_view>
#include <vector>


extern struct Opt {
	const char* program = "html-macro";
	bool help = false;
	
	bool outVoid = false;
	const char* outFilePath = nullptr;
	
	std::vector<const char*> includes;
	std::vector<const char*> defines;
	const char* file = nullptr;
} opt;


bool parseCLI(char const* const* argv, int argc);

