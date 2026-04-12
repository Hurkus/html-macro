#pragma once
#include "Macro.hpp"
#include "Expression.hpp"
#include <memory>


class MacroEngine {
// ----------------------------------- [ Structures ] --------------------------------------- //
public:
	enum class Branch {
		NONE,
		PASSED,
		FAILED
	};
	
// ----------------------------------- [ Variables ] ---------------------------------------- //
public:
	std::shared_ptr<Macro> macro;
	std::shared_ptr<VariableMap> variables;
	Branch currentBranch_block = Branch::NONE;
	Branch currentBranch_inline = Branch::NONE;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	void setVariableConstants();
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Spawns a scoped copy of the current MacroEngine and evaluate macro as HTML.
	 *        Current working directory `Paths::cwd` is set to `macro::srcPath` durring evaluation.
	 * @note `macro.html != nullptr`
	 * @param macro Macro containing the source HTML macro.
	 * @param dst Parent node for any created nodes.
	 */
	void exec(const std::shared_ptr<Macro>& macro, html::Node& dst);
	
public:
	/**
	 * @brief Evaluate single line (node) of a macro.
	 *        Regular HTML tags are copied using `tag`, `attribute`, `text` and so on.
	 *        Depending on the macro/regular tag, child nodes are recursively evaluated with `evalChildren()`.
	 *        Engine state can be modified.
	 * @param op Operation node for evaluation.
	 * @param dst Parent node for any created nodes.
	 */
	void eval(const html::Node& op, html::Node& dst);
	
	/**
	 * @brief Evaluate all child nodes using `eval()`.
	 *        State such as branching is reset before running and restored after completion.
	 * @param parent Parent whose child elements will be evaluated.
	 * @param dst Parent node for any created nodes.
	 */
	void evalChildren(const html::Node& parent, html::Node& dst);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	html::Node* newNode(html::NodeType type);
	html::Attr* newAttr();
	char* newStr(size_t len);
	char* newStr(std::string_view str);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Copy text content and interpolate for expressions.
	 * @param src Source node containing text.
	 * @param dst Parent node for any created text nodes.
	 * @note `src.type == NodeType::TXT`
	 */
	void text(const html::Node& src, html::Node& dst);
	
	/**
	 * @brief Copy HTML tag and attributes.
	 *        Attributes are also interpolated for expressions.
	 *        Children of `op` are recursively evaluated with `run`.
	 * @param op Source node for copying.
	 * @param dst Parent node for any created nodes.
	 * @note `src.type == NodeType::TAG`
	 */
	void tag(const html::Node& op, html::Node& dst);
	
	/**
	 * @brief Copy attribute and interpolate any expressions in the value.
	 * @param op Parent node of `op_attr`
	 * @param op_attr Source attribute for copying.
	 * @return New attribute that should be later appended or deleted.
	 */
	html::Attr* attribute(const html::Node& op, const html::Attr& attr);
	
public:
	void setAttr(const html::Node& op, html::Node& dst);
	void getAttr(const html::Node& op, html::Node& dst);
	void delAttr(const html::Node& op, html::Node& dst);
	void setTag(const html::Node& op, html::Node& dst);
	void getTag(const html::Node& op, html::Node& dst);
	void delTag(const html::Node& op, html::Node& dst);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Set variable.
	 * @param op Operation node from which to get name and value of new variables.
	 */
	void set(const html::Node& op);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Shorthand for <CALL> where the macro name is the tag.
	 * @param op Operation node from which to extract call information.
	 * @param dst Destination parent node for any created nodes.
	 * @return `false` if the macro does not exist.
	 */
	bool call_userElementMacro(const html::Node& op, html::Node& dst);
	
	/**
	 * @brief Shorthand for CALL attribute macro where the macro name is the attribute name.
	 * @param op Calling operation node from which `attr` originates.
	 * @param attr Attribute invoking the macro call.
	 * @param dst Destination parent node for any created nodes.
	 * @return `false` if the macro does not exist.
	 */
	bool call_userAttrMacro(const html::Node& op, const html::Attr& attr, html::Node& dst);
	
	/**
	 * @brief Element macro for invoking a macro.
	 * @param op Calling operation node from which to extract call information.
	 * @param dst Destination parent node for any created nodes.
	 */
	void call(const html::Node& op, html::Node& dst);
	
	/**
	 * @brief Macro attribute for invoking a macro.
	 * @param op Calling operation node from which `opAttr` originates.
	 * @param opAttr Attribute invoking the macro call.
	 * @param dst Destination parent node for any created nodes.
	 */
	void call(const html::Node& op, const html::Attr& opAttr, html::Node& dst);
	
	/**
	 * @brief Include all macros defined in a file. Other tags are executed as a macro immediately.
	 * @param op Operation node from which to extract file path and options.
	 * @param dst Destination parent node for any created nodes.
	 */
	void include(const html::Node& op, html::Node& dst);
	
	/**
	 * @brief Include all macros defined in a file. Other tags are executed as a macro immediately.
	 * @param op Calling operation node from which `opAttr` originates.
	 * @param opAttr Attribute invoking the include call.
	 * @param dst Destination parent node for any created nodes.
	 */
	void include(const html::Node& op, const html::Attr& opAttr, html::Node& dst);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	void branch_if(const html::Node& op, html::Node& dst);
	void branch_elif(const html::Node& op, html::Node& dst);
	void branch_else(const html::Node& op, html::Node& dst);
	long loop_for(const html::Node& op, html::Node& dst);
	long loop_while(const html::Node& op, html::Node& dst);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Execute content of operation node as a shell command and
	 *         include results in the document.
	 * @param op Operation node from which to extract the shell command.
	 * @param dst Destination parent node for any created nodes.
	 */
	void shell(const html::Node& op, html::Node& dst);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Print a message to stderr.
	 * @param op Operation node from which the message is extracted.
	 *           Child element must be a `NodeType::TEXT`. 
	 */
	void info(const html::Node& op);
	
	/**
	 * @brief Print a warning message to stderr.
	 * @param op Operation node from which the message is extracted.
	 *           Child element must be a `NodeType::TEXT`. 
	 */
	void warn(const html::Node& op);
	
	/**
	 * 
	 * @brief Print an error message to stderr.
	 * @param op Operation node from which the message is extracted.
	 *           Child element must be a `NodeType::TEXT`. 
	 */
	void error(const html::Node& op);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	/**
	 * @brief Transfer leading and trailing whitespace.
	 * @param dst Node with new child elements (reversed list).
	 * @param prev_last_child Original last child before more child elements were added.
	 */
	static void transfer_parent_space(const html::Node& op, html::Node& dst, const html::Node* prev_last_child);
	
	/**
	 * @brief Check if attribute is an `IF`, `ELIF` or `ELSE` attribute macro, and evaluate.
	 * @param op Parent node of `attr`.
	 * @param attr Attribute to check for the attribute macro.
	 * @return `Branch::None` if it's none of the attribute macros.
	 * @return `Branch::PASS` if evaluated to `true`. 
	 * @return `Branch::FAIL` if evaluated to `false`. 
	 */
	Branch try_eval_attr_if_elif_else(const html::Node& op, const html::Attr& attr);
	
	/**
	 * @brief Evaluate attribute value as an expression to determine bool value.
	 * @param op Parent node of `attr`.
	 * @param attr Attribute containing the expression.
	 * @param result Output for evaluated bool. Valid only when function returns `true`.
	 * @return `true` if evaluation was successfull.
	 */
	bool eval_attr_bool(const html::Node& op, const html::Attr& attr, bool& result);
	
	/**
	 * @brief Evaluate attribute value as an expression and check if it evaluates to `true`.
	 * @param op Parent node of `attr`.
	 * @param attr Attribute containing the expression.
	 * @return `true` if the expression evaluates to `true` and did not fail.
	 */
	bool eval_attr_true(const html::Node& op, const html::Attr& attr);
	
	/**
	 * @brief Evaluate attribute value as an expression and check if it evaluates to `false`.
	 * @param op Parent node of `attr`.
	 * @param attr Attribute containing the expression.
	 * @return `true` if the expression evaluates to `false` and did not fail.
	 */
	bool eval_attr_false(const html::Node& op, const html::Attr& attr);
	
	/**
	 * @brief Evaluate attribute value as an expression, interpolated string or const string.
	 *        All of these evaluate into a `Value`.
	 * @param op Parent node of `attr`.
	 * @param attr The attribute that is to be evaluated.
	 * @param out_result The resulting value if evaluation succeeded.
	 * @return `false` if an error occured.
	 */
	bool eval_attr_value(const html::Node& op, const html::Attr& attr, Value& out_result);
	bool eval_attr_value(const html::Node& op, const html::Attr& attr, std::string& result_buff, std::string_view& result);
	
	/**
	 * @brief Interpolate string for any expressions.
	 * @param op Parent containing the interpolated string (either by value or within an attribute).
	 * @param str The string to interpolate.
	 * @param buff The output buffer for the resulting interpolated string.
	 *             If an error occurs, the buffer is not cleared.
	 * @return `true` If no errors occured with interpolation.
	 */
	bool eval_string_interpolate(const html::Node& op, std::string_view str, std::string& buff);
	
	/**
	 * @brief Check if variable, named the same as an attribute, equals the attribute value.
	 * @param op Operational node holding the `attr`ibute. 
	 * @param attr The attribute, which shares the name with the variable.
	 *             The attribute value is compared to the value of the variable.
	 * @return MacroEngine::Branch 
	 */
	Branch attr_equals_variable(const html::Node& op, const html::Attr& attr);

// ------------------------------------------------------------------------------------------ //
};
