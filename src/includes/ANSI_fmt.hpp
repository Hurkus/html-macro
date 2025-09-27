#pragma once


// Storage for two variants of a format string: colored and uncolored
template<const char* FMT>
struct fmt_ansi {
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	static constexpr long len(const char* s){
		const char* beg = s;
		while (*s != 0) s++;
		return s - beg;
	}
	
	static constexpr long ansi_len(const char* s){
		long n = 0;
		int ansi = 0;
		
		const char* beg = s;
		while (*s != 0){
			// Parse ANSI color code
			if (ansi == 0)
				ansi = (*s == '\e') ? 1 : 0;
			else if (ansi == 1)
				ansi = (*s == '[') ? 2 : 0;
			else if (ansi >= 2){
				ansi++;
				if (!('0' <= *s && *s <= '9')){
					n += (*s == 'm') ? ansi : 0;
					ansi = 0;
				}
			}
			
			s++;
		}
		
		return (n == 0) ? -1 : s - beg - n;
	}
	
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	static constexpr long color_len = len(FMT);
	static constexpr const char* color = FMT;
	
	static constexpr long plain_len = ansi_len(FMT);
	char plain[plain_len + 1] = {};
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	consteval fmt_ansi(){
		if (plain_len <= 0){
			return;
		}
		
		char buff[color_len];
		const char* src = FMT;
		char* dst = buff;
		int ansi = 0;
		
		while (*src != 0){
			*dst = *src;
			dst++;
			
			// Parse ANSI color code
			if (ansi == 0)
				ansi = (*src == '\e') ? 1 : 0;
			else if (ansi == 1)
				ansi = (*src == '[') ? 2 : 0;
			else if (ansi >= 2){
				ansi++;
				if (!('0' <= *src && *src <= '9')){
					dst -= (*src == 'm') ? ansi : 0;
					ansi = 0;
				}
			}
			
			src++;
		}
		
		for (long i = 0 ; i < plain_len ; i++){
			plain[i] = buff[i];
		}
		
		plain[plain_len] = 0;
	}

// ----------------------------------- [ Operators ] ---------------------------------------- //
public:
	constexpr const char* operator()(bool colored) const {
		if (plain_len < 0)
			return color;
		return colored ? color : plain;
	}
	
// ------------------------------------------------------------------------------------------ //
};
