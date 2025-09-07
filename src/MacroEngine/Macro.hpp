#pragma once
#include "html.hpp"


namespace MacroEngine {
class Macro {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	html::Document doc;
	std::string_view name;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	static const Macro* get(std::string_view name);
	static const Macro* loadFile(std::string_view filePath);
	
public:
	static void clearCache();
	static void releaseCache();
	
// ------------------------------------------------------------------------------------------ //
};
}
