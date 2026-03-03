#include <iostream>
#include <fstream>

#include "fs.hpp"
#include "cli.hpp"
#include "MacroEngine.hpp"
#include "Paths.hpp"
#include "Write.hpp"
#include "Debug.hpp"

using namespace std;


// ----------------------------------- [ Prototypes ] --------------------------------------- //


bool printDependencies(const char* path);


// ----------------------------------- [ Functions ] ---------------------------------------- //


#define B(s)		ANSI_BOLD s ANSI_RESET
#define Y(s)		ANSI_YELLOW s ANSI_RESET
#define C(s)		ANSI_CYAN s ANSI_RESET

#if DEBUG
	#define VERSION 	"Version 0.13.1 (Alpha) (Debug)"
#else
	#define VERSION 	"Version 0.13.1"
#endif


void help(){
	LOG_STDOUT(B("html-macro: ") Y(VERSION) ", by " ANSI_CYAN "Hurkus" ANSI_RESET ".\n");
	LOG_STDOUT(B("Usage:") " %s [options] [<variable>=<value>] <file>\n", opt.program);
	
	LOG_STDOUT("\n");
	LOG_STDOUT(B("Variables:\n"));
	LOG_STDOUT("  Sets expression variables for use within macro expressions\n");
	LOG_STDOUT("  The argument must be in the form of " PURPLE("`name=value`") ".\n");
	LOG_STDOUT("  Example: `%s " PURPLE("n=10") " in.html -o out.html`\n", opt.program);
	
	LOG_STDOUT("\n");
	LOG_STDOUT(B("Options:\n"));
	LOG_STDOUT("  " Y("--help") ", " Y("-h") " .................... Print help.\n");
	LOG_STDOUT("  " Y("--include <path>") ", " Y("-i <path>") " ... Add folder to list of path searches when including files with relative paths.\n");
	LOG_STDOUT("  " Y("--output <path>") ", " Y("-o <path>") " .... Write output to file instead of stdout.\n");
	LOG_STDOUT("  " Y("--type <type>") ", " Y("-t <type>") " ...... Force input file type, where " Y("<type>") " can be " C("html") ", " C("css") ", " C("js") " or " C("txt") ".\n");
	LOG_STDOUT("  " Y("--compress <type>") ", " Y("-c <type>") " .. Compress output, where " Y("<type>") " can be " C("none") ", " C("html") ", " C("css") " or " C("all") ".\n");
	LOG_STDOUT("                                   This option can be supplied multiple times to fill out the type enum.\n");
	LOG_STDOUT("                                   (default: " C("none") ")\n");
	LOG_STDOUT("  " Y("--nostdout") ", " Y("-x") " ................ Do not output any results; only errors and warnings.\n");
	LOG_STDOUT("  " Y("--dependencies") ", " Y("-d")  " ............ Print list of file paths on which the input file depens on.\n");
	LOG_STDOUT("                                   The paths are extracted from " PURPLE("<INCLUDE/>") " macros.\n");
	LOG_STDOUT("                                   Only non-expression attribute values are considered.\n");
	LOG_STDOUT("\n");
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool setDefinedVariables(const vector<const char*>& defines, VariableMap& vars){
	for (const char* s : defines){
		const char* name_beg = s;
		const char* name_end = nullptr;
		
		while (true){
			if (*s == 0){
				ERROR("Defined variable " PURPLE("`%s`") " missing value. Proper format is " PURPLE("name=value") ".", name_beg);
				assert(*s != 0);
				return false;
			} else if (*s == '='){
				name_end = s;
				break;
			} else if (isspace(*s)){
				if (name_end == nullptr)
					name_end = s;
			} else if (name_end != nullptr){
				ERROR("Invalid format of variable definition " PURPLE("`%s`") ". Proper format is " PURPLE("name=value") ".", name_beg);
				return false;
			}
			s++;
		}
		
		assert(*s == '=');
		s++;
		
		const char* val_beg = s;
		vars.insert(string_view(name_beg, name_end), val_beg);
	}
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool execMacro(shared_ptr<Macro>&& macro, ostream* out){
	assert(macro != nullptr);
	Macro::Type type = (opt.inFileType != Macro::Type::NONE) ? opt.inFileType : macro->type;
	
	switch (type){
		case Macro::Type::HTML: {
			if (macro->html == nullptr && !macro->parseHTML()){
				return false;
			}
			
			// Setup engine
			MacroEngine engine = {};
			engine.variables = make_shared<VariableMap>();
			engine.setVariableConstants();
			if (!setDefinedVariables(opt.defines, *engine.variables)){
				return false;
			}
			
			html::Document doc = {};
			engine.exec(macro, doc);
			
			if (out != nullptr){
				return write(*out, doc, opt.compress);
			}
			
			return true;
		}
		
		case Macro::Type::CSS: {
			if (macro->txt == nullptr){
				return false;
			} else if (out == nullptr){
				return true;
			}
			
			string_view txt = *macro->txt;
			if (opt.compress % WriteOptions::COMPRESS_CSS){
				return compressCSS(*out, txt.begin(), txt.end());
			} else {
				*out << txt;
				return bool(out);
			}
			
		}
		
		case Macro::Type::JS:
		case Macro::Type::TXT: {
			if (macro->txt == nullptr)
				return false;
			else if (out == nullptr)
				return true;
			
			*out << *macro->txt;
			return bool(out);
		}
		
		case Macro::Type::NONE:
			break;
		
	}
	
	ERROR("Unsupported input type.");
	return false;
}


static bool run(){
	MacroCache::clear();
	Paths::cwd = make_unique<filepath>(fs::cwd());
	
	ostream* out = (opt.outFilePath != nullptr && opt.outFilePath == "-"sv ? &cout : nullptr);
	ofstream outf;
	
	// Open output file
	if (opt.outFilePath != nullptr && out == nullptr){
		outf = ofstream(opt.outFilePath);
		out = &outf;
		
		if (!outf.is_open()){
			ERROR("Failed to open output file: " PURPLE("`%s`"), opt.outFilePath);
			return false;
		}
		
	}
	
	filepath src_path = opt.inFilePath;
	if (!fs::exists(src_path)){
		ERROR("Input file not found: " PURPLE("`%s`"), src_path.c_str());
		return false;
	}
	
	// Load input file
	shared_ptr<Macro> root_macro = MacroCache::load(src_path);
	if (root_macro == nullptr){
		ERROR("Failed to read file: " PURPLE("`%s`"), src_path.c_str());
		return false;
	}
	
	bool ret = execMacro(move(root_macro), out);
	if (out != nullptr){
		out->flush();
	}
	
	// Cleanup
	MacroCache::clear();
	return ret;
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	stdout_isTTY = (isatty(fileno(stdout)) == 1);
	stderr_isTTY = (isatty(fileno(stderr)) == 1);
	
	#if DEBUG
		const char* _argv[] = {
			argv[0],
			"test/test-0.html",
			// "test/test-6.in.html",
			// "-c", "all",
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
