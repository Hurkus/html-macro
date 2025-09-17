#pragma once
#include <string_view>
#include <vector>


extern struct Opt {
	const char* program = "html-macro";
	bool help = false;
	
	bool out_void = false;
	const char* outFilePath = nullptr;
	
	std::vector<const char*> includes;
	std::vector<const char*> defines;
	std::vector<const char*> files;
} opt;


bool parseCLI(char const* const* argv, int argc);

