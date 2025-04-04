#pragma once
#include <memory>
#include <filesystem>

#include "string_map.hpp"
#include "Macro.hpp"


class MacroEngine {
// ----------------------------------- [ Variables ] ---------------------------------------- //
public:
	pugi::xml_document doc;
	string_map<std::unique_ptr<Macro>> macros;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	Macro* getMacro(std::string_view name) const;
	Macro* loadFile(const std::filesystem::path& path);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	void run(const Macro& macro, pugi::xml_node dst);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	void resolve(const pugi::xml_node op, pugi::xml_node dst);
	pugi::xml_node tag(const pugi::xml_node op, pugi::xml_node dst);
	void call(const pugi::xml_node op, pugi::xml_node dst);
	void call_inline(const pugi::xml_node op, pugi::xml_node dst);
	
// ------------------------------------------------------------------------------------------ //
};
