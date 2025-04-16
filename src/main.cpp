#include <iostream>
#include <fstream>
#include <string_view>

#include "MacroEngine.hpp"
#include "MacroParser.hpp"
#include "Expression.hpp"
#include "ExpressionParser.hpp"
#include "CLI.hpp"
#include "DEBUG.hpp"

using namespace std;


// ----------------------------------- [ Functions ] ---------------------------------------- //


#define Y(s)	ANSI_YELLOW s ANSI_RESET


static void version(){
	INFO(ANSI_BOLD PROGRAM_NAME ": " ANSI_RESET ANSI_YELLOW "v0.0" ANSI_RESET ", by " ANSI_CYAN "Hurkus" ANSI_RESET ".");
}


static void help(){
	version();
	INFO(ANSI_BOLD "Usage:" ANSI_RESET " %s [options] <files>", CLI::name());
	INFO(ANSI_BOLD "Options:" ANSI_RESET);
	INFO("  " Y("--help") ", " Y("-h") " .................... Print help.");
	INFO("  " Y("--version") ", " Y("-v") " ................. Print program version.");
	INFO("  " Y("--output <path>") ", " Y("-o <path>") " .... Write output to file instead of stdout.");
	INFO("");
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool run(const vector<filesystem::path>& files){
	ofstream outf;	// Delay opening of file.
	
	// Execute all macro files
	for (const filesystem::path& file : files){
		MacroEngine engine = {};
		engine.setVariableConstants();
		
		shared_ptr<Macro> main = engine.loadFile(file);
		if (main == nullptr){
			return false;
		}
		
		engine.exec(*main, engine.doc);
		
		// Open output file
		if (!outf.is_open() && !CLI::options.outPath.empty()){
			outf = ofstream(CLI::options.outPath);
			if (outf.fail()){
				ERROR("Failed to open file '%s'.", CLI::options.outPath.c_str());
				return false;
			}
		}
		
		// Select output stream and write
		ostream& out = (outf.is_open()) ? outf : cout;
		write(engine.doc, out);
		out.flush();
	}
	
	return true;
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	try {
		CLI::parse(argc, argv);
	} catch (const exception& e){
		ERROR("%s\nUse '%s --help' for help.", e.what(), CLI::name());
	}
	
	#ifdef DEBUG
		// CLI::options.outPath = "obj/main.html";
		if (CLI::options.files.empty()){
			CLI::options.files.emplace_back("./assets/test-1.html");
			// CLI::options.files.emplace_back("./assets/test-2.html");
		}
	#endif
	
	if (CLI::options.help){
		help();
		return 0;
	} else if (CLI::options.version){
		version();
		return 0;
	} else if (CLI::options.files.empty()){
		ERROR("No input files.\nUse '%s --help' for help.", CLI::options.callName.c_str());
		return 1;
	}
	
	if (!run(CLI::options.files)){
		return 1;
	}
	
	return 0;
}


// ------------------------------------------------------------------------------------------ //
