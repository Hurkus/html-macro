#include <iostream>
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
	INFO(ANSI_BOLD "Usage:" ANSI_RESET " %s [options] <file>", CLI::name());
	INFO(ANSI_BOLD "Options:" ANSI_RESET);
	INFO("  " Y("--help") ", " Y("-h") " ...... Print help.");
	INFO("  " Y("--version") ", " Y("-v") " ... Print program version.");
	INFO("");
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	try {
		CLI::parse(argc, argv);
	} catch (const runtime_error& e){
		ERROR("%s\nUse '%s --help' for help.", e.what(), CLI::name());
	}
	
	
	if (CLI::options.help){
		help();
		return 0;
	} else if (CLI::options.version){
		version();
		return 0;
	}
	
	
	printf(ANSI_BOLD "Hello world!" ANSI_RESET "\n");
	return 0;
}


// ------------------------------------------------------------------------------------------ //
