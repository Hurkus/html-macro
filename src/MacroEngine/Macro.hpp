#pragma once
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include "pugixml.hpp"


class Macro {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	pugi::xml_document root;
	std::string name;
	std::shared_ptr<std::filesystem::path> srcFile;
	
// ------------------------------------------------------------------------------------------ //
};


class XHTMLFile {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	pugi::xml_document doc;
	std::filesystem::path path;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	std::vector<std::unique_ptr<Macro>> convertToMacroSet();
	static std::vector<std::unique_ptr<Macro>> extractMacros(pugi::xml_document&& doc);
	
// ------------------------------------------------------------------------------------------ //
};
