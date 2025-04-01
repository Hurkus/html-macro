#include "CLI.hpp"
#include <cassert>
#include <cstring>
#include <vector>
#include <stdexcept>

extern "C" {
	#include <getopt.h>
}


using namespace std;
using namespace CLI;


// ------------------------------------[ Variables ] ---------------------------------------- //


Options CLI::options;

static enum OptionID : int {
	NONE,
	HELP,
	VERSION
} selected_opt;


// ----------------------------------- [ Constants ] ---------------------------------------- //


// First char must be ':'
const char* const short_options = ":" "h" "v";


const struct option long_options[] = {
	{"help",    no_argument, (int*)&selected_opt, OptionID::HELP    },
	{"version", no_argument, (int*)&selected_opt, OptionID::VERSION },
	{0, 0, 0, 0}
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


static OptionID shortOptionToLong(char c){
	switch (c){
		case 'h':
			return OptionID::HELP;
		case 'v':
			return OptionID::VERSION;
		default:
			return OptionID::NONE;
	}
}


static void applyOption(OptionID opt, const char* arg = nullptr){
	switch (opt){
		
		case OptionID::HELP:
			options.help = true;
			break;
		
		case OptionID::VERSION:
			options.version = true;
			break;
		
		// case OptionID::N: {
		// 	int n;
		// 	if (arg == nullptr || sscanf(arg, "%d", &n) != 1)
		// 		throw runtime_error("Could not parse number.");
		// } break;
		
		case OptionID::NONE:
			break;
		
	}
}


// Handle residual options (eg. file names)
static void applyPostOptions(int argc, char const* const* argv){
	for (int i = 0 ; i < argc ; i++){
		// printf("%s\n", argv[i]);
	}
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void CLI::parse(int argc, char const* const* argv){
	assert(argv != nullptr);
	if (argc < 1 || argv[0] == nullptr){
		throw cli_error("Missing program arguments.");
	}
	
	options.callName = argv[0];
	if (argc == 1){
		return;
	}
	
	// Reset getopt state machine.
	opterr = 0;
	optind = (optind == 0) ? 0 : 1;
	
	// Initialize and create modifyable copy of argv.
	vector<const char*> v = vector<const char*>(&argv[0], &argv[argc]);
	
	int prevOpt = optind;
	while (true){
		const int c = getopt_long(argc, (char* const*)v.data(), short_options, long_options, NULL);
		
		if (c == -1){
			break;
		} else if (c == '?'){
			throw cli_error(string("Unrecognized option '") + v[prevOpt] + "'.");
		} else if (c == ':'){
			throw cli_error(string("Missing option argument '") + v[optind-1] + "'.");
		} else if (c > 0){
			selected_opt = shortOptionToLong(c);
		}
		
		applyOption(selected_opt, optarg);
		prevOpt = optind;
	}
	
	// Non-option arguments
	if (optind < argc){
		applyPostOptions(argc - optind, &(v.data())[optind]);
	}
	
	return;
}


// ------------------------------------------------------------------------------------------ //