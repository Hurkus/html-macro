#include <iostream>
#include <string_view>

#include "MacroCache.hpp"
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


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool run(const vector<filesystem::path>& files){
	if (files.empty()){
		return true;
	}
	
	
	Macro* m = MacroCache::load(files[0]);
	if (m == nullptr){
		return false;
	}
	
	
	pugi::xml_node node = m->doc.find_node([](pugi::xml_node node){
		return (node.name() == "title"sv);
	});
	
	INFO("Title: %s", node.child_value());
	
	return true;
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	try {
		CLI::parse(argc, argv);
	} catch (const exception& e){
		ERROR("%s\nUse '%s --help' for help.", e.what(), CLI::name());
	}
	
	if (CLI::options.help){
		help();
		return 0;
	} else if (CLI::options.version){
		version();
		return 0;
	}
	
	if (!run(CLI::options.files)){
		return 1;
	}
	
	return 0;
}


// ------------------------------------------------------------------------------------------ //
