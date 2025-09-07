#pragma once
#include <string_view>
#include <vector>


extern struct Opt {
	const char* program = "html-macro";
	std::vector<const char*> files;
	bool help = false;
	const char* outFilePath = nullptr;
	std::vector<const char*> includes;
} opt;


bool parseCLI(char const* const* argv, int argc);

