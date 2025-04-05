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
	Macro* root = engine.loadFile(files[0]);
	
	
	if (root != nullptr){
		engine.run(*root, engine.doc);
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
	// try {
	// 	CLI::parse(argc, argv);
	// } catch (const exception& e){
	// 	ERROR("%s\nUse '%s --help' for help.", e.what(), CLI::name());
	// }
	
	// if (CLI::options.help){
	// 	help();
	// 	return 0;
	// } else if (CLI::options.version){
	// 	version();
	// 	return 0;
	// }
	
	// if (!run(CLI::options.files)){
	// 	return 1;
	// }
	
	using namespace Expression;
	
	
	// string_view expr = "len('hello')";
	string_view expr = "str(1 + 2*(2 + 1)/3.0) + ' | ' + str(10/1.5)";
	
	
	
	
	Parser p;
	auto e = p.parse(expr);
	if (e == nullptr){
		ERROR("Nope.");
		return 1;
	}
	
	
	// auto e = fun("int", val(-66.2));
	// auto e = move(p.expr);
	Value res = e->eval({});
	
	
	// Print result
	std::visit([](const auto& x){
		cout << "[" ANSI_GREEN;
		if constexpr (is_same_v<remove_const_t<remove_reference_t<decltype(x)>>,string>)
			cout <<  ANSI_YELLOW "'" << x << "'" ANSI_GREEN;
		else
			cout << x;
		cout << ANSI_RESET "]" << endl;
	}, res);
	
	
	return 0;
}


// ------------------------------------------------------------------------------------------ //
