#pragma once
#include "Macro.hpp"
#include "Expression.hpp"


namespace MacroEngine {
// ----------------------------------- [ Structures ] --------------------------------------- //


enum class Branch {
	NONE, TRUE, FALSE
};

enum class Interpolate {
	NONE      = 0,
	CONTENT   = 1 << 0,
	ATTRIBUTE = 1 << 1,
	ALL       = CONTENT | ATTRIBUTE,
};


// ----------------------------------- [ Variables ] ---------------------------------------- //


extern Expression::VariableMap variables;
extern html::Document doc;

extern const Macro* currentMacro;
extern Branch currentBranch;
extern Interpolate currentInterpolation;


// ----------------------------------- [ Functions ] ---------------------------------------- //


void setVariableConstants();

inline void reset(){
	variables.clear();
	doc.reset();
	currentMacro = nullptr;
	currentBranch = Branch::NONE;
	currentInterpolation = Interpolate::ALL;
	setVariableConstants();
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


/**
 * @brief Evaluate macro and store results into global `html::Document doc`.
 */
void exec(const Macro& macro, html::Node& dst);


/**
 * @brief Evaluate single line (node) of a macro.
 *        Regular HTML tags are copied using `tag`, `attribute`, `text` and so on.
 *        Depending on the macro/regular tag, child nodes are recursively evaluated with `runChildren()`.
 *        State is modified as per the macros.
 * @param op Operation node for evaluation.
 * @param dst Destination root node for any created nodes as per the operation node.
 */
void run(const html::Node& op, html::Node& dst);


/**
 * @brief Evaluate all child nodes as macros using `run()`.
 *        State such as branching is reset before running and restored after completion.
 * @param parent Parent containing child macros.
 * @param dst Destination root node for any created nodes.
 */
void runChildren(const html::Node& parent, html::Node& dst);


// ----------------------------------- [ Functions ] ---------------------------------------- //


/**
 * @brief Copy text content and interpolate for expressions.
 * @param src Node from which to copy text content into `dst` child node.
 * @param dst Destination parent node for the new text node.
 */
void text(const html::Node& src, html::Node& dst);

/**
 * @brief Copy HTML tag and attributes.
 *        Attributes are also interpolated for expressions.
 *        Children of `op` are recursively evaluated with `run`.
 * @param op Source node for copying.
 * @param dst Destination parent node for any created nodes.
 */
void tag(const html::Node& op, html::Node& dst);

/**
 * @brief Copy attribute and interpolate any expressions in the value.
 * @param attr Source attribute for copying.
 * @param dst Destination node for the copied attribute.
 */
void attribute(const html::Attr& attr, html::Node& dst);


// ----------------------------------- [ Functions ] ---------------------------------------- //


/**
 * @brief Invoke defined macro using information from the calling node.
 * @param op Calling operation node from which to extract call information.
 * @param dst Destination parent node for any created nodes.
 */
void call(const html::Node& op, html::Node& dst);

bool include(const html::Node& op, html::Node& dst);


// ----------------------------------- [ Functions ] ---------------------------------------- //


void info(const html::Node& op);
void warn(const html::Node& op);
void error(const html::Node& op);


// ------------------------------------------------------------------------------------------ //
};






class MacroEngineObject {
// ----------------------------------- [ Variables ] ---------------------------------------- //
public:
	// pugi::xml_document doc;
	// string_map<std::shared_ptr<MacroObject>> macros;
	// Expression::VariableMap variables;
	
public:
	// const MacroObject* currentMacro = nullptr;
	// optbool branch = nullptr;
	// bool interpolateText = true;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	bool execBuff(std::string_view buff, pugi::xml_node dst);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	void set(const pugi::xml_node op);
	bool branch_if(const pugi::xml_node op, pugi::xml_node dst);
	bool branch_elif(const pugi::xml_node op, pugi::xml_node dst);
	bool branch_else(const pugi::xml_node op, pugi::xml_node dst);
	int loop_for(const pugi::xml_node op, pugi::xml_node dst);
	int loop_while(const pugi::xml_node op, pugi::xml_node dst);
	
public:
	bool setAttr(const pugi::xml_node op, pugi::xml_node dst);
	bool getAttr(const pugi::xml_node op, pugi::xml_node dst);
	bool delAttr(const pugi::xml_node op, pugi::xml_node dst);
	bool setTag(const pugi::xml_node op, pugi::xml_node dst);
	bool getTag(const pugi::xml_node op, pugi::xml_node dst);
	bool delTag(const pugi::xml_node op, pugi::xml_node dst);
	
public:
	bool shell(const pugi::xml_node op, pugi::xml_node dst);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Interpolate string for any expressions and store result into the attribute value.
	 * @param str String for interpolation.
	 * @param dst Attribute of which the value is set to the interpolated string.
	 */
	void interpolateAttr(const char* str, pugi::xml_attribute dst);
	
// ------------------------------------------------------------------------------------------ //
};
