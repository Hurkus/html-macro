#pragma once
#include <memory>
#include <filesystem>

#include "optbool.hpp"
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
	optbool branch = nullptr;
	bool interpolateText = true;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	Macro* getMacro(std::string_view name) const;
	Macro* loadFile(const std::filesystem::path& path);
	void setVariableConstants();
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	void exec(const Macro& macro, pugi::xml_node dst);
	bool execBuff(std::string_view buff, pugi::xml_node dst);
	
	/**
	 * @brief Evaluate node as a macro.
	 *        Regular HTML tags are copied using `tag`, `attribute`, `text` and so on.
	 *        Depending on the macro/regular tag, child nodes are recursively evaluated with `runChildren()`.
	 *        State is modified as per the macros.
	 * @param op Operation node for evaluation. Type of the node can be anything.
	 * @param dst Destination root node for any created nodes from the operation node.
	 */
	void run(const pugi::xml_node op, pugi::xml_node dst);
	
	/**
	 * @brief Evaluate all child nodes as macros using `run()`.
	 *        State such as branching is reset before running and restored after completion.
	 * @param parent Parent containing child macros.
	 * @param dst Destination root node for any created nodes.
	 */
	void runChildren(const pugi::xml_node parent, pugi::xml_node dst);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Copy HTML tag and attributes.
	 *        Attributes are also interpolated for expressions.
	 *        Children are recursively evaluated with `run`.
	 * @param op Source node for copying.
	 * @param dst Destination root node for the new node.
	 * @return Created node.
	 */
	pugi::xml_node tag(const pugi::xml_node op, pugi::xml_node dst);
	
	/**
	 * @brief Copy attribute by interpolating any expressions in the value.
	 * @param attribute Source attribute for copying.
	 * @param dst Destination node for the copied attribute.
	 * @return Created attribute.
	 */
	pugi::xml_attribute attribute(const pugi::xml_attribute attribute, pugi::xml_node dst);
	
	/**
	 * @brief Copy plain text and interpolate for expressions.
	 * @param pcdata String for interpolation.
	 * @param dst Destination root node for the new text node.
	 * @return Created text node.
	 */
	pugi::xml_text text(const char* pcdata, pugi::xml_node dst);
	
public:
	void set(const pugi::xml_node op);
	bool branch_if(const pugi::xml_node op, pugi::xml_node dst);
	bool branch_elif(const pugi::xml_node op, pugi::xml_node dst);
	bool branch_else(const pugi::xml_node op, pugi::xml_node dst);
	int loop_for(const pugi::xml_node op, pugi::xml_node dst);
	int loop_while(const pugi::xml_node op, pugi::xml_node dst);
	
public:
	void call(const char* name, pugi::xml_node dst);
	void call(const pugi::xml_node op, pugi::xml_node dst);
	
public:
	bool shell(const pugi::xml_node op, pugi::xml_node dst);
	
public:
	void info(const pugi::xml_node op);
	void warn(const pugi::xml_node op);
	void error(const pugi::xml_node op);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	optbool evalCond(const char* exprstr) const;
	
// ------------------------------------------------------------------------------------------ //
};
