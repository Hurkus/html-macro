#pragma once
#include <string_view>
#include <vector>

#include "Macro.hpp"
#include "Write.hpp"


extern struct Opt {
	const char* program = "html-macro";
	bool help = false;
	bool printDependencies = false;
	
	
	const char* inFilePath = nullptr;
	Macro::Type inFileType = Macro::Type::NONE;
	
	const char* outFilePath = nullptr;
	WriteOptions compress = WriteOptions::NONE;
	bool noOutput = false;
	
	std::vector<const char*> includes;
	std::vector<const char*> defines;
} opt;


bool parseCLI(char const* const* argv, int argc);

