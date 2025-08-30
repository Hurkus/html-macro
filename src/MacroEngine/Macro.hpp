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




// TODO: REMOVE
class MacroObject {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	pugi::xml_document root;
	std::string name;
	std::shared_ptr<std::filesystem::path> srcFile;
	
// ------------------------------------------------------------------------------------------ //
};
