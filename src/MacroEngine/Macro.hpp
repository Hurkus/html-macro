#pragma once
#include "html.hpp"
#include "Paths.hpp"


class Macro {
// ----------------------------------- [ Structures ] --------------------------------------- //
public:
	enum class Type {
		NONE, TXT, HTML, CSS, JS
	};
	
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	std::string name;							// readonly
	std::shared_ptr<const filepath> srcFile;	// readonly
	std::shared_ptr<const filepath> srcDir;		// readonly
	
	// Dominant macro type based on `srcFile` extension.
	Type type = Type::TXT;
	
	std::shared_ptr<const std::string> txt;		// Type::TXT
	std::unique_ptr<html::Document> html;		// Type::HTML
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	bool parse();
	bool parseHTML();
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	static Type getType(const filepath& path);
	
// ------------------------------------------------------------------------------------------ //
};


namespace MacroCache {
	/**
	 * @brief Get macro from cache.
	 * @param name Name of the macro.
	 * @return Pointer to macro or `nullptr` if it is not cached. 
	 */
	Macro* get(std::string_view name) noexcept;

	/**
	 * @brief Get cached macro file or load new one into the cache.
	 * @param filePath File path of the macro. If macro is not cached, this file is parsed and the new macro is cached.
	 *        Path is normalized and resolved based on the list of include directories.
	 * @return Pointer to the (possibly newly) cached macro. `nullptr` if it is not cached and parsing failed.
	 */
	Macro* load(filepath& filePath);


	/**
	 * @brief Clear cache. This can be dangerous since a lot of `html` objects use raw pointers.
	 *        This should be done at the end of the program or when deleting all `html` objects.
	 */
	void clear();
	
};
