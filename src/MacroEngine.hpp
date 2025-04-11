#pragma once
#include <memory>
#include <filesystem>

#include "string_map.hpp"
#include "Macro.hpp"
#include "Expression.hpp"


class MacroEngine {
// ----------------------------------- [ Variables ] ---------------------------------------- //
public:
	pugi::xml_document doc;
	string_map<std::unique_ptr<Macro>> macros;
	Expression::VariableMap variables;
	
public:
	bool branch = false;
	
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
	void set(const pugi::xml_node op);
	
	bool branch_if(const pugi::xml_node op, pugi::xml_node dst);
	bool branch_elif(const pugi::xml_node op, pugi::xml_node dst);
	bool branch_else(const pugi::xml_node op, pugi::xml_node dst);
	
	void info(const pugi::xml_node op);
	void warn(const pugi::xml_node op);
	void error(const pugi::xml_node op);
	
// ------------------------------------------------------------------------------------------ //
};
