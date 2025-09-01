#pragma once
#include "html.hpp"


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
	static const Macro* loadFile(std::string_view filePath);
	static std::unique_ptr<Macro> loadBuffer(std::shared_ptr<const std::string>&& buff);
	
// ------------------------------------------------------------------------------------------ //
};
