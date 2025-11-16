#include <iostream>
#include <fstream>

#include "fs.hpp"
#include "cli.hpp"
#include "MacroEngine.hpp"
#include "Paths.hpp"
#include "Write.hpp"
#include "Debug.hpp"

using namespace std;
using namespace html;
using namespace MacroEngine;


// ----------------------------------- [ Prototypes ] --------------------------------------- //


bool printDependencies(const char* path);


// ----------------------------------- [ Functions ] ---------------------------------------- //


#define B(s)		ANSI_BOLD s ANSI_RESET
#define Y(s)		ANSI_YELLOW s ANSI_RESET
#define P(s)		ANSI_PURPLE s ANSI_RESET
#define VERSION 	"Version 0.12.0 Alpha"


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
	LOG_STDOUT("  " Y("--help") ", " Y("-h") " .................... Print help.\n");
	LOG_STDOUT("  " Y("--include <path>") ", " Y("-i <path>") " ... Add folder to list of path searches when including files with relative paths.\n");
	LOG_STDOUT("  " Y("--output <path>") ", " Y("-o <path>") " .... Write output to file instead of stdout.\n");
	LOG_STDOUT("  " Y("--compress <type>") ", " Y("-c <type>") " .. Compress output, where <type> can be `none`, `html`, `css` or `all`.\n");
	LOG_STDOUT("                                   This option can be supplied multiple times to fill out the type enum.\n");
	LOG_STDOUT("                                   (default: none)\n");
	LOG_STDOUT("  " Y("-x") " ............................ Do not output any results; only errors.\n");
	LOG_STDOUT("  " Y("-d") ", " Y("--dependencies") " ............ Print list of file paths on which the input file depens on.\n");
	LOG_STDOUT("                                   The paths are extracted from <INCLUDE/> macros.\n");
	LOG_STDOUT("                                   Only non-expression attribute values are considered.\n");
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


static bool execMacro(Macro& macro, ostream* out){
	switch (macro.type){
		case Macro::Type::CSS: {
			if (macro.txt == nullptr){
				assert(macro.txt != nullptr);
				return false;
			} else if (out == nullptr){
				return true;
			}
			
			string_view txt = *macro.txt;
			if (opt.compress_css){
				return compressCSS(*out, txt.begin(), txt.end());
			} else {
				return bool(*out << txt);
			}
			
		}
		
		case Macro::Type::HTML: {
			if (!macro.parseHTML()){
				return false;
			}
			
			html::Document doc = {};
			MacroEngine::exec(macro, doc);
			
			WriteOptions wropt = WriteOptions::NONE;
			wropt |= (opt.compress_css) ? WriteOptions::COMPRESS_CSS : WriteOptions::NONE;
			wropt |= (opt.compress_html) ? WriteOptions::COMPRESS_HTML : WriteOptions::NONE;
			
			if (out != nullptr){
				return write(*out, doc, wropt);
			}
			
		}
		
		case Macro::Type::JS: {
			assert(macro.txt != nullptr);
			if (macro.txt == nullptr)
				return false;
			else if (out == nullptr)
				return true;
			else
				return bool(*out << *macro.txt);
		}
		
		default: {
			ERROR("Unsupported input type.");
			return false;
		}
		
	}
}



static bool run(){
	const bool voidOutput = (opt.outFilePath == nullptr);
	
	MacroEngine::reset();
	MacroCache::clear();
	Paths::cwd = make_unique<filepath>(fs::current_path());
	
	// Open output file
	ofstream outf;
	if (!voidOutput && opt.outFilePath != "-"sv){
		outf = ofstream(opt.outFilePath);
		if (!outf.is_open()){
			ERROR("Failed to open output file `%s`.", opt.outFilePath);
			return false;
		}
	}
	
	filepath src_path = opt.inFilePath;
	if (!fs::exists(src_path)){
		ERROR("Input file `%s` not found.", src_path.c_str());
		return false;
	}
	
	if (!setDefinedVariables(opt.defines)){
		return false;
	}
	
	// Load input file
	Macro* root_macro = MacroCache::load(src_path);
	if (root_macro == nullptr){
		ERROR("Failed to read file `%s`.", src_path.c_str());
		return false;
	}
	
	ostream& out = (outf.is_open()) ? outf : cout;
	bool ret = execMacro(*root_macro, !voidOutput ? &out : nullptr);
	out.flush();
	
	// Cleanup
	MacroCache::clear();
	assert(html::assertDeallocations());
	return ret;
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	stdout_isTTY = (isatty(fileno(stdout)) == 1);
	stderr_isTTY = (isatty(fileno(stderr)) == 1);
	
	#ifdef DEBUG
		const char* _argv[] = {
			argv[0],
			"test/test-0.html",
			// "test/test-2.in.html",
			// "-c", "css",
			// "-c", "html",
			"-c", "all",
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
		filepath& inc = Paths::includeDirs.emplace_back(file);
		if (inc.empty())
			Paths::includeDirs.pop_back();
	}
	
	// Run
	if (opt.printDependencies){
		if (!printDependencies(opt.inFilePath))
			return 2;
	} else if (!run()){
		return 2;
	}
	
	return 0;
}


// ------------------------------------------------------------------------------------------ //
