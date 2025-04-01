#pragma once
#include <memory>
#include <filesystem>

#include "pugixml.hpp"
#include "string_map.hpp"
#include "Macro.hpp"


namespace MacroCache {
// ----------------------------------- [ Variables ] ---------------------------------------- //


extern string_map<std::unique_ptr<Macro>> cache;


// ----------------------------------- [ Functions ] ---------------------------------------- //


Macro* getMacro(std::string_view name);
Macro* loadFile(const std::filesystem::path& path);


// ------------------------------------------------------------------------------------------ //
}
