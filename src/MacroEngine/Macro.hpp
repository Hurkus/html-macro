#pragma once
#include "html.hpp"
#include "Paths.hpp"


namespace MacroEngine {
class Macro {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	html::Document doc;
	std::string_view name;
	std::shared_ptr<const filepath> srcDir;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	static const Macro* get(std::string_view name);
	static const Macro* loadFile(const filepath& filePath, bool resolve = true);
	
	static char* loadRawFile(const filepath& filePath, size_t& out_len);
	
public:
	static void clearCache();
	static void releaseCache();
	
// ------------------------------------------------------------------------------------------ //
};
}
