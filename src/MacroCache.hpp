#pragma once
#include "pugixml.hpp"
#include <memory>
#include <unordered_map>
#include <filesystem>

#include "Macro.hpp"


namespace MacroCache {
// ----------------------------------- [ Variables ] ---------------------------------------- //


extern std::unordered_map<std::filesystem::path,std::unique_ptr<Macro>> cache;


// ----------------------------------- [ Functions ] ---------------------------------------- //


Macro* load(const std::filesystem::path& path);


// ------------------------------------------------------------------------------------------ //
}
