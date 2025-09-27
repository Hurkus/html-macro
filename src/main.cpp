#include <iostream>
#include <fstream>

#include "cli.hpp"
#include "MacroEngine.hpp"
#include "Paths.hpp"
#include "Write.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Functions ] ---------------------------------------- //


#define B(s)		ANSI_BOLD s ANSI_RESET
#define Y(s)		ANSI_YELLOW s ANSI_RESET
#define P(s)		ANSI_PURPLE s ANSI_RESET
#define VERSION 	"Version 0.10.0"


void help(){
	LOG_STDOUT(B("html-macro: ") Y(VERSION) ", by " ANSI_CYAN "Hurkus" ANSI_RESET ".\n");
	LOG_STDOUT(B("Usage:") " %s [options] [<variable>=<value>] <file>\n", opt.program);
	
	LOG_STDOUT("\n");
	LOG_STDOUT(B("Variables:\n"));
	LOG_STDOUT("  Sets expression variables for use within macro expressions\n");
	LOG_STDOUT("  The argument must be in the form of " P("`name=value`") ".\n");
	LOG_STDOUT("  Example: `%s " P("n=10") " in.html -o out.html`\n", opt.program);
	
	LOG_STDOUT("\n");
	LOG_STDOUT(B("Options:\n"));
	LOG_STDOUT("  " Y("--help") ", " Y("-h") " ................... Print help.\n");
	LOG_STDOUT("  " Y("--include <path>") ", " Y("-i <path>") " .. Add folder to list of path searches when including files with relative paths.\n");
	LOG_STDOUT("  " Y("--output <path>") ", " Y("-o <path>") " ... Write output to file instead of stdout.\n");
	LOG_STDOUT("  " Y("-x") " ........................... Do not output any results; only errors.\n");
	LOG_STDOUT("\n");
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool setDefinedVariables(const vector<const char*>& defines){
	for (const char* s : defines){
		const char* name_beg = s;
		const char* name_end = nullptr;
		
		while (true){
			if (*s == 0){
				ERROR("Defined variable " P("`%s`") " missing value. Proper format is `name=value`.", name_beg);
				assert(*s != 0);
				return false;
			} else if (*s == '='){
				name_end = s;
				break;
			} else if (isspace(*s)){
				if (name_end == nullptr)
					name_end = s;
			} else if (name_end != nullptr){
				ERROR("Invalid format of variable definition " P("`%s`") ". Proper format is `name=value`.", name_beg);
				return false;
			}
			s++;
		}
		
		assert(*s == '=');
		s++;
		
		const char* val_beg = s;
		MacroEngine::variables.insert(string_view(name_beg, name_end), val_beg);
	}
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool run(const char* file){
	MacroEngine::reset();
	MacroEngine::cwd = make_unique<filepath>(filesystem::current_path());
	Macro::clearCache();
	
	// Open output file
	ofstream outf;
	if (opt.outFilePath != nullptr && opt.outFilePath != "-"sv){
		outf = ofstream(opt.outFilePath);
		if (!outf.is_open()){
			ERROR("Failed to open output file `%s`.", opt.outFilePath);
			return false;
		}
	}
	
	// Parse input file
	const Macro* m = Macro::loadFile(file);
	if (m == nullptr){
		return false;
	}
	
	if (!setDefinedVariables(opt.defines)){
		return false;
	}
	
	// Run
	html::Document doc = {};
	MacroEngine::exec(*m, doc);
	
	// Select output stream and write
	bool ret = true;
	if (opt.outFilePath != nullptr){
		ostream& out = (outf.is_open()) ? outf : cout;
		ret = write(out, doc);
		out.flush();
	}
	
	// Cleanup
	doc.clear();
	Macro::clearCache();
	assert(html::assertDeallocations());
	
	return ret;
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	stdout_isTTY = (isatty(fileno(stdout)) == 1);
	stderr_isTTY = (isatty(fileno(stderr)) == 1);
	
	#ifdef DEBUG
		// log_stdout_isTTY = 0;
		// log_stderr_isTTY = 0;
		
		const char* _argv[] = {
			argv[0],
			"test/test-0.html"
			// "test/test-1.in.html"
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
	} else if (opt.inFilePath == nullptr){
		ERROR("No input files.");
		return 1;
	}
	
	for (const char* file : opt.includes){
		filepath& inc = MacroEngine::paths.emplace_back(file);
		if (inc.empty())
			MacroEngine::paths.pop_back();
	}
	
	if (!run(opt.inFilePath)){
		return 2;
	}
	
	return 0;
}


// ------------------------------------------------------------------------------------------ //
