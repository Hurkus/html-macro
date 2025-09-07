#include <iostream>
#include <fstream>
#include <string_view>
#include <array>
#include <algorithm>

#include "cli.hpp"
#include "MacroEngine.hpp"
#include "Paths.hpp"

#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Functions ] ---------------------------------------- //


#define Y(s)		ANSI_YELLOW s ANSI_RESET
#define VERSION 	"Alpha v0.1.0"


void help(){
	info(ANSI_BOLD "html-macro: " ANSI_RESET ANSI_YELLOW VERSION ANSI_RESET ", by " ANSI_CYAN "Hurkus" ANSI_RESET ".");
	info(ANSI_BOLD "Usage:" ANSI_RESET " %s [options] <files>", opt.program);
	info(ANSI_BOLD "Options:" ANSI_RESET);
	info("  " Y("--help") ", " Y("-h") " ................... Print help.");
	info("  " Y("--output <path>") ", " Y("-o <path>") " ... Write output to file instead of stdout.");
	info("  " Y("--include <path>") ", " Y("-i <path>") " .. Add folder to list of path searches when including files with relative paths.");
	info("");
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool write(ostream& out, const Document& doc);


static bool run(const char* file){
	MacroEngine::reset();
	MacroEngine::cwd = make_unique<filepath>(filesystem::current_path());
	Macro::clearCache();
	
	// Open output file
	ofstream outf;
	if (opt.outFilePath != nullptr){
		outf = ofstream(opt.outFilePath);
		if (!outf.is_open()){
			ERROR("Failed to open output file '%s'.", opt.outFilePath);
			return false;
		}
	}
	
	// Parse input file
	const Macro* m = Macro::loadFile(file);
	if (m == nullptr){
		return false;
	}
	
	// Run
	html::Document doc = {};
	MacroEngine::exec(*m, doc);
	
	
	// Select output stream and write
	ostream& out = (outf.is_open()) ? outf : cout;
	if (!write(out, doc)){
		out.flush();
		return false;
	}
	
	out.flush();
	return true;
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	#ifdef DEBUG
		const char* _argv[] = {
			argv[0],
			"test/test-1.in.html"
		};
		if (argc < 2){
			argv = _argv;
			argc = sizeof(_argv) / sizeof(*_argv);
		}
	#endif
	
	
	if (argc < 2){
		help();
		return 1;
	} else if (!parseCLI(argv, argc)){
		return 1;
	}
	
	if (opt.help){
		help();
		return 0;
	} else if (opt.files.empty()){
		ERROR("No input files.");
		return 1;
	} else if (opt.files.size() > 1){
		ERROR("Too many input files: '%s'", opt.files[1]);
		return 1;
	}
	
	for (const char* file : opt.includes){
		filepath& inc = MacroEngine::paths.emplace_back(file);
		if (inc.empty())
			MacroEngine::paths.pop_back();
	}
	
	if (!run(opt.files[0])){
		return 2;
	}
	
	return 0;
}


// ------------------------------------------------------------------------------------------ //
