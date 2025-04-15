#pragma once
#include <filesystem>
#include <string_view>
#include "pugixml.hpp"


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool parseFile(const char* file, pugi::xml_document& out_doc);
bool parseBuffer(std::string_view buff, pugi::xml_node dst);

void write(const pugi::xml_node root, std::ostream& out);


// ------------------------------------------------------------------------------------------ //