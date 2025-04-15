#include "CLI.hpp"
#include <cassert>
#include <cstring>
#include <vector>
#include <stdexcept>

extern "C" {
	#include <getopt.h>
}

#include "Format.hpp"


using namespace std;
using namespace CLI;


// ------------------------------------[ Variables ] ---------------------------------------- //


Options CLI::options;

static enum OptionID : int {
	NONE,
	HELP,
	VERSION,
	OUTPUT
} selected_opt;


// ----------------------------------- [ Constants ] ---------------------------------------- //


// First char must be ':'
const char* const short_options = ":" "h" "v" "o:";


const struct option long_options[] = {
	{"help",    no_argument,       (int*)&selected_opt, OptionID::HELP    },
	{"version", no_argument,       (int*)&selected_opt, OptionID::VERSION },
	{"output",  required_argument, (int*)&selected_opt, OptionID::OUTPUT  },
	{0, 0, 0, 0}
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


static OptionID shortOptionToLong(char c){
	switch (c){
		case 'h':
			return OptionID::HELP;
		case 'v':
			return OptionID::VERSION;
		case 'o':
			return OptionID::OUTPUT;
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
		
		case OptionID::OUTPUT: {
			assert(arg != nullptr);
			options.outPath = arg;
		} break;
		
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
	assert(argc >= 0);
	options.files.reserve(options.files.size() + argc);
	
	for (int i = 0 ; i < argc && argv[i] != nullptr ; i++){
		try {
			filesystem::path p = argv[i];
			options.files.emplace_back(p.lexically_normal());
		} catch (const exception& e){
			throw cli_error(format("Failed to recognise file path '%s'.", argv[i]));
		}
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
	vector<const char*> v = vector<const char*>(argv, argv + argc);
	
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
		applyPostOptions(argc - optind, v.data() + optind);
	}
	
	return;
}


// ------------------------------------------------------------------------------------------ //