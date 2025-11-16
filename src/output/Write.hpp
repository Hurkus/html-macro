#pragma once
#include <ostream>
#include "EnumOperators.hpp"


namespace html {
	class Document;
};


enum class WriteOptions {
	NONE          = 0,
	COMPRESS_CSS  = 1 << 0,
	COMPRESS_HTML = 1 << 1
};
template<> inline constexpr bool has_enum_operators<WriteOptions> = true;


bool write(std::ostream& out, const html::Document& doc, WriteOptions options = WriteOptions::NONE);
bool compressCSS(std::ostream& out, const char* beg, const char* end);
