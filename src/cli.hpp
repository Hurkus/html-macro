#pragma once
#include <string_view>
#include <vector>


extern struct Opt {
	const char* program = "html-macro";
	bool help = false;
	bool printDependencies = false;
	
	bool compress_css = false;
	bool compress_html = false;
	
	const char* outFilePath = "-";		// "-" means stdout
	const char* inFilePath = nullptr;
	
	std::vector<const char*> includes;
	std::vector<const char*> defines;
} opt;


bool parseCLI(char const* const* argv, int argc);

