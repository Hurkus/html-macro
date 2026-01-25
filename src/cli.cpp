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
	INCLUDE,
	OUTPUT,
	INPUT_TYPE,
	COMPRESS,
	DISCARD_OUTPUT,
	DEPENDENCIES,
};

struct OptInfo {
	string_view name1;
	string_view name2;
	OptId id;
	bool hasValue = false;
};

constexpr array options = {
	OptInfo { "-h", "--help",         OptId::HELP,           false },
	OptInfo { "-i", "--include",      OptId::INCLUDE,        true  },
	OptInfo { "-o", "--out",          OptId::OUTPUT,         true  },
	OptInfo { "-t", "--type",         OptId::INPUT_TYPE,     true  },
	OptInfo { "-c", "--compress",     OptId::COMPRESS,       true  },
	OptInfo { "-x", "",               OptId::DISCARD_OUTPUT, false },
	OptInfo { "-d", "--dependencies", OptId::DEPENDENCIES,   false },
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
		
		case OptId::DISCARD_OUTPUT:
			opt.noOutput = true;
			return true;
		
		case OptId::OUTPUT: {
			if (opt.outFilePath == nullptr){
				opt.outFilePath = value;
				return true;
			} else {
				ERROR("Multiple output files are not allowed: " PURPLE("`%s`") " and " PURPLE("`%s`"), opt.outFilePath, value);
				return false;
			}
		}
		
		case OptId::INPUT_TYPE: {
			if (value == "html"sv)
				opt.inFileType = Macro::Type::HTML;
			else if (value == "css"sv)
				opt.inFileType = Macro::Type::CSS;
			else if (value == "js"sv)
				opt.inFileType = Macro::Type::JS;
			else if (value == "txt"sv)
				opt.inFileType = Macro::Type::TXT;
			else {
				opt.inFileType = Macro::Type::NONE;
				ERROR("Invalid option value " PURPLE("`%s`") ". Valid values are " CYAN("`html`") ", " CYAN("`css`") ", " CYAN("`js`") " or " CYAN("`txt`") ".", value);
				return false;
			}
			return true;
		}
		
		case OptId::INCLUDE:
			opt.includes.emplace_back(value);
			return true;
		
		case OptId::COMPRESS: {
			assert(value != nullptr);
			
			if (value == "none"sv || value == ""sv){
				opt.compress = WriteOptions::NONE;
			} else if (value == "all"sv){
				opt.compress |= WriteOptions::COMPRESS_HTML;
				opt.compress |= WriteOptions::COMPRESS_CSS;
			} else if (value == "html"sv){
				opt.compress |= WriteOptions::COMPRESS_HTML;
			} else if (value == "css"sv){
				opt.compress |= WriteOptions::COMPRESS_CSS;
			} else {
				ERROR("Invalid option value " PURPLE("`%s`") ". Valid values are " CYAN("`none`") ", " CYAN("`html`") ", " CYAN("`css`") " or " CYAN("`all`") ".", value);
				return false;
			}
			
			return true;
		}
		
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
		ERROR("Too many input files: " PURPLE("`%s`"), arg);
		return false;
	}
	
	// Is file
	opt.inFilePath = arg;
	return true;
}


static bool err_invalidOption(const char* arg){
	ERROR("Invalid option: " PURPLE("`%s`"), arg);
	return false;
}

static bool err_invalidValue(const char* arg){
	ERROR("Option does not expect a value: " PURPLE("`%s`"), arg);
	return false;
}

static bool err_missingValue(const char* arg){
	ERROR("Option is missing a value: " PURPLE("`%s`"), arg);
	return false;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr bool isSwitchChar(char c) noexcept {
	return c == '-' || c == '+';
}

constexpr bool isOptChar(char c) noexcept {
	return c != '=' && isgraph(c);
}


constexpr const OptInfo* getOptInfo(const char* arg, int& type) noexcept {
	assert(isSwitchChar(arg[0]));
	
	// Parse long option.
	if (arg[1] == arg[0]){
		type = 2;
		
		// Parse option name
		int i = 2;
		while (isOptChar(arg[i])) i++;
		const string_view name = string_view(arg, i);
		
		for (const OptInfo& info : options){
			if (name == info.name2)
				return &info;
		}
		
	}
	
	// Parse short option.
	else if (arg[1] != 0){
		const string_view name = string_view(arg, 2);
		type = 1;
		
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
			if (!onFile(arg))
				return false;
			continue;
		}
		
		// Parse switch
		int type = 0;	// short=1, long=2
		const OptInfo* info = getOptInfo(arg, type);
		
		// End of options or error
		if (info == nullptr){
			if (type == 2 && arg[2] == 0)
				break;	// End of all options '--'
			else
				return err_invalidOption(arg);
		}
		
		// Short opt
		else if (type == 1){
			const size_t len = info->name1.length();
			
			// Expect value
			if (info->hasValue){
				if (arg[len] == 0)	// if inline
					goto val_next_arg;
				else if (!onOption(info->id, arg + len))
					return false;
			}
			
			else {
				if (arg[len] != 0)
					return err_invalidValue(arg);
				else if (!onOption(info->id, nullptr))
					return false;
			}
			
			continue;
		}
		
		// Long option
		else if (type == 2){
			const size_t len = info->name2.length();
			
			// No value
			if (!info->hasValue){
				if (arg[len] != 0)
					return err_invalidValue(arg);
				else if (!onOption(info->id, nullptr))
					return false;
				continue;
			}
			
			// Inline value
			else if (arg[len] == '='){
				if (!onOption(info->id, arg + len + 1))
					return false;
				continue;
			}
			
			// Next argument is value
			else if (arg[len] == 0){
				goto val_next_arg;
			}
			
			return err_invalidOption(arg);
		}
		
		// Look for value in next argument
		val_next_arg:
		if (argc < 1 || *argv == nullptr){
			return err_missingValue(arg);
		} else if (!onOption(info->id, *argv)){
			return false;
		} else {
			argc--, argv++;
		}
		
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
