#pragma once
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

#include "html.hpp"
#include "pugixml.hpp"


namespace MacroEngine {
	class Macro;
};


class MacroEngine::Macro {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	html::Document doc;
	std::string_view name;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	static const Macro* get(std::string_view name);
	static const Macro* load(std::string_view filePath);
	
// ------------------------------------------------------------------------------------------ //
};




class MacroObject {
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
	std::vector<std::unique_ptr<MacroObject>> convertToMacroSet();
	static std::vector<std::unique_ptr<MacroObject>> extractMacros(pugi::xml_document&& doc);
	
// ------------------------------------------------------------------------------------------ //
};
