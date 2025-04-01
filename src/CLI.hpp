#include <string>
#include <stdexcept>

#define PROGRAM_NAME "html-macro"


namespace CLI {
// ------------------------------------[ Variables ] ---------------------------------------- //


extern struct Options {
	std::string callName = PROGRAM_NAME;
	bool help = false;
	bool version = false;
} options;


// ----------------------------------- [ Structures ] --------------------------------------- //


class cli_error : public std::runtime_error {
	using std::runtime_error::runtime_error;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


/**
 * @brief 
 * @exception `cli_error` 
 * @param argc Number of elements in `argv`.
 * @param argv Array of string options.
 */
void parse(int argc, char const* const* argv);


inline const char* name(){
	return options.callName.c_str();
}


/**
 * @brief Reset CLI options back to default.
 */
inline void clear(){
	CLI::options = {};
}


// ------------------------------------------------------------------------------------------ //
}
