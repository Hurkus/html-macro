#include "Write.hpp"
#include <cassert>
#include <string_view>

using namespace std;


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr bool isWhitespace(char c) noexcept {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

constexpr bool isQuote(char c) noexcept {
	return c == '"' || c == '\'';
}

// Doesnt need whitespace
constexpr bool isTokenChar(char c) noexcept {
	return c == ';' || c == ',' || c == ':'
		|| c == '{' || c == '}'
		|| c == '(' || c == ')'
		|| c == '[' || c == ']'; 
}

constexpr bool isComment(const char* s, const char* end) noexcept {
	assert(s != end);
	return s[0] == '/' && s+1 != end && s[1] == '*';
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr const char* skipWhitespace(const char* s, const char* end) noexcept {
	while (s != end && isWhitespace(*s)) s++;
	return s;
}


constexpr const char* skipComment(const char* s, const char* end) noexcept {
	assert(s != end && s+1 != end);
	assert(s[0] == '/' && s[1] == '*');
	
	s += 2;
	while (s != end){
		if (s[0] == '*' && s+1 != end && s[1] == '/')
			return s+2;
		s++;
	}
	
	return s;
}


constexpr const char* skipString(const char* s, const char* end) noexcept {
	assert(s != end && isQuote(*s));
	const char quot = *s;
	
	s++;
	while (s != end && *s != quot){
		if (*s == '\\'){
			if (++s == end)
				break;
		}
		s++;
	}
	
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool compressCSS(ostream& out, const char* beg, const char* end){
	const char* s = skipWhitespace(beg, end);
	
	repeat:
	while (s != end){
		assert(!isWhitespace(*s));
		const char* left = s;
		
		// Parse solid part
		while (s != end){
			if (isWhitespace(*s)){
				break;
			} else if (isQuote(*s)){
				s = skipString(s, end);
				continue;
			} else if (isComment(s, end)){
				out.write(left, s - left);
				s = skipComment(s, end);
				s = skipWhitespace(s, end);
				goto repeat;
			}
			s++;
		}
		
		// Write solid part
		assert(s - left >= 1);
		out.write(left, s - left);
		left = s - 1;
		
		// Lookahead whitespace
		assert(s == end || isWhitespace(*s));
		s = skipWhitespace(s, end);
		if (s == end){
			break;
		}
		
		// Preserve whitespace on id characters
		assert(left != s);
		if (!isTokenChar(*left) && !isTokenChar(*s)){
			out << ' ';
		}
		
		continue;
	}
	
	return true;
}


// ------------------------------------------------------------------------------------------ //