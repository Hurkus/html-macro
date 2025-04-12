#pragma once
#include <filesystem>
#include <string_view>
#include "pugixml.hpp"


bool parseFile(const char* file, pugi::xml_document& out_doc);
bool parseBuffer(std::string_view buff, pugi::xml_node dst);
