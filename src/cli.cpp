#include "cli.hpp"
#include <cassert>
#include <string_view>
#include <array>
#include <iostream>

#include "Debug.hpp"

using namespace std;


// ----------------------------------- [ Variables ] ---------------------------------------- //


Opt opt;


// ----------------------------------- [ Structures ] --------------------------------------- //


enum class OptId {
	HELP,
	DEPENDENCIES,
	VOID,
	OUTPUT,
	INCLUDE,
};

struct OptInfo {
	string_view name1;
	string_view name2;
	OptId id;
	bool hasValue = false;
};

constexpr array options = {
	OptInfo { "-h", "--help",         OptId::HELP,         false },
	OptInfo { "-d", "--dependencies", OptId::DEPENDENCIES, false },
	OptInfo { "-x", "",               OptId::VOID,         false },
	OptInfo { "-o", "--out",          OptId::OUTPUT,       true  },
	OptInfo { "-i", "--include",      OptId::INCLUDE,      true  },
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool onOption(OptId id, const char* value){
	switch (id){
		case OptId::HELP:
			opt.help = true;
			return true;
		
		case OptId::DEPENDENCIES:
			opt.printDependencies = true;
			return true;
		
		case OptId::VOID:
			opt.outFilePath = nullptr;
			return true;
		
		case OptId::OUTPUT:
			if (opt.outFilePath == nullptr)
				return true;
			else if (opt.outFilePath != nullptr && opt.outFilePath != "-"sv){
				ERROR("Multiple output files are not allowed.");
				return false;
			} else {
				opt.outFilePath = value;
				return true;
			}
		
		case OptId::INCLUDE:
			opt.includes.emplace_back(value);
			return true;
		
	}
	return false;
}

static bool onFile(const char* arg){
	assert(arg != nullptr);
	
	// Check if variable
	for (const char* s = arg ; *s != 0 ; s++){
		if (*s == '='){
			opt.defines.emplace_back(arg);
			return true;
		}
	}
	
	if (arg[0] == 0){
		return true;
	} else if (opt.inFilePath != nullptr){
		ERROR("Too many input files: `%s`", arg);
		return false;
	}
	
	// Is file
	opt.inFilePath = arg;
	return true;
}


static bool err_invalidOption(const char* arg){
	ERROR("Invalid option '%s'.", arg);
	return false;
}

static bool err_invalidValue(const char* arg){
	ERROR("Option '%s' does not expect a value.", arg);
	return false;
}

static bool err_missingValue(const char* arg){
	ERROR("Option '%s' is missing a value.", arg);
	return false;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr bool isSwitchChar(char c){
	return c == '-' || c == '+';
}

constexpr bool isOptChar(char c){
	return c != '=' && isgraph(c);
}


constexpr const OptInfo* getOptInfo(const char* arg, int& len){
	assert(isSwitchChar(arg[0]));
	
	// Parse long option.
	if (arg[1] == arg[0]){
		len = 2;
		while (isOptChar(arg[len])){
			len++;
		}
		
		const string_view name = string_view(arg, len);
		
		for (const OptInfo& i : options){
			if (name == i.name2)
				return &i;
		}
		
		return nullptr;
	}
	
	// Parse short option.
	else if (arg[1] != 0){
		len = 2;
		const string_view name = string_view(arg, 2);
		
		for (const OptInfo& i : options){
			if (name == i.name1)
				return &i;
		}
		
	}
	
	return nullptr;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool parseCLI(char const* const* argv, int argc){
	if (argc <= 0 || argv[0] == nullptr){
		return false;
	}
	
	opt.program = argv[0];
	argc--, argv++;
	
	while (argc > 0 && *argv != nullptr){
		const char* arg = *argv;
		argc--, argv++;
		
		// Check if switch
		if (!isSwitchChar(arg[0])){
			onFile(arg);
			continue;
		}
		
		// Parse switch
		int len = 0;
		const OptInfo* info = getOptInfo(arg, len);
		
		// End of options or error
		if (info == nullptr){
			// --
			if (len != 2){
				return err_invalidOption(arg);
			}
			
			// '--  '
			for (int i = 2 ; arg[i] != 0 ; i++){
				if (!isspace(arg[i]))
					return err_invalidOption(arg);
			}
			
			break;
		}
		
		// No value
		if (!info->hasValue){
			if (arg[len] != 0)
				return err_invalidValue(arg);
			else if (onOption(info->id, nullptr))
				continue;
			else
				return false;
		}
		
		// Short opt
		else if (len == 2){
			// Inline value
			if (arg[len] != 0){
				if (onOption(info->id, arg + 2))
					continue;
				else
					return false;
			}
			
			goto val_next_arg;
		}
		
		// Long inline value
		else if (arg[len] == '='){
			if (onOption(info->id, arg + len + 1))
				continue;
			else
				return false;
		}
		
		// Look for value in next argument
		val_next_arg:
		if (argc > 0 && *argv != nullptr && !isSwitchChar(argv[0][0])){
			if (!onOption(info->id, *argv))
				return false;
			else
				argc--, argv++;
			continue;
		}
			
		return err_missingValue(arg);
	}
	
	// Consume the rest as file names.
	while (argc > 0 && *argv != nullptr){
		if (!onFile(*argv))
			return false;
		argc--, argv++;
	}
	
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


consteval bool verifyOptions(){
	for (const OptInfo& i : options){
		if (i.name1.empty() && i.name2.empty()){
			return false;
		}
		
		if (!i.name1.empty()){
			if (i.name1.length() != 2)
				return false;
			else if (!isSwitchChar(i.name1[0]))
				return false;
		}
		
		if (!i.name2.empty()){
			if (i.name2.length() < 3)
				return false;
			else if (!isSwitchChar(i.name2[0]) && i.name2[0] != i.name2[1])
				return false;
		}
		
	}
	
	return true;
}

static_assert(verifyOptions());


// ------------------------------------------------------------------------------------------ //
