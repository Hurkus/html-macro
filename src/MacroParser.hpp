#pragma once
#include <filesystem>
#include "pugixml.hpp"


void parseMacro(const char* file, pugi::xml_document& out_doc);


class ParsingException : public std::runtime_error {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	std::filesystem::path file;
	int row = 0;
	int col = 0;
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	using std::runtime_error::runtime_error;
	
	ParsingException(const char* file, const std::string& msg) :
		runtime_error(msg), file{file}, row{row}, col{col} {};
	
	ParsingException(const char* file, int row, int col, const std::string& msg) :
		runtime_error(msg), file{file}, row{row}, col{col} {};
	
// ------------------------------------------------------------------------------------------ //
};
