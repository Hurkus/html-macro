#include <iostream>
#include <string_view>

#include "MacroEngine.hpp"
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
	
	MacroEngine engine = {};
	engine.setVariableConstants();
	Macro* root = engine.loadFile(files[0]);
	
	if (root != nullptr){
		engine.exec(*root, engine.doc);
	}
	
	
	// cout << "-----------------" << endl;
	// for (auto& p : engine.macros){
	// 	cout << ANSI_YELLOW <<  p.second->name << ANSI_RESET << endl;
	// 	p.second->root.print(cout);
	// 	cout << "-----------------" << endl;
	// }
	
	// cout << "--------------------------------" << endl;
	engine.doc.print(cout);
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
	// CLI::options.files.emplace_back("./assets/test.html");
	CLI::options.files.emplace_back("./assets/style-test.html");
	#endif
	
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
