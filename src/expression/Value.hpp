#pragma once
#include <cassert>
#include <cstdint>
#include <string_view>


struct Value {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	enum class Type : uint32_t {
		LONG, DOUBLE, STRING
	} type = Type::LONG;
	
	uint32_t data_len = 0;
	
	union {
		long l = 0;
		double d;
		char* s;
	} data;
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	Value() = default;
	
	Value(long n) : type{Type::LONG}, data{.l = n} {}
	Value(double n) : type{Type::DOUBLE}, data{.d = n} {}
	Value(char* s, uint32_t len) : type{Type::STRING}, data_len{len}, data{.s = s} {}
	
	Value(std::string_view s) : type{Type::STRING} {
		size_t size = s.length();
		data_len = (uint32_t)std::min(size, size_t(INT32_MAX));
		
		data.s = new char[size_t(data_len) + 1];
		std::copy(s.begin(), s.end(), data.s);
		data.s[data_len] = 0;
	}
	
	Value(std::string_view s1, std::string_view s2) : type{Type::STRING} {
		size_t size = s1.length() + s2.length();
		data_len = (uint32_t)std::min(size, size_t(INT32_MAX));
		
		data.s = new char[size + 1];
		std::copy(s1.begin(), s1.end(), data.s + 0);
		std::copy(s2.begin(), s2.end(), data.s + s1.length());
		data.s[data_len] = 0;
	}
	
public:
	Value(const Value& o) : type{o.type}, data_len{o.data_len}, data{o.data} {
		if (type == Type::STRING){
			data.s = new char[data_len + 1];
			std::copy(o.data.s, o.data.s + data_len, data.s);
			data.s[data_len] = 0;
		}
	}
	
	Value(Value&& o){
		std::swap(this->type, o.type);
		std::swap(this->data_len, o.data_len);
		std::swap(this->data, o.data);
	}
	
	~Value(){
		if (type == Type::STRING)
			delete[] data.s;
	}
	
// ----------------------------------- [ Operators ] ---------------------------------------- //
public:
	void operator=(const Value& o){
		if (this == &o){
			return;
		}
		
		this->~Value();
		type = o.type;
		data_len = o.data_len;
		data = o.data;
		
		if (type == Type::STRING){
			data.s = new char[data_len + 1];
			std::copy(o.data.s, o.data.s + data_len, data.s);
			data.s[data_len] = 0;
		}
		
	}
	
	void operator=(Value&& o){
		std::swap(this->type, o.type);
		std::swap(this->data_len, o.data_len);
		std::swap(this->data, o.data);
	}

// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	bool toBool() const noexcept;
	std::string toStr() const;
	std::string& toStr(std::string& buff) const;
	
public:
	std::string_view sv() const {
		assert(type == Type::STRING);
		return std::string_view(data.s, data_len);
	}
	
// ----------------------------------- [ Operators ] ---------------------------------------- //
public:
	bool operator==(const Value& o) const noexcept;
	bool semanticEquals(const Value& o) const noexcept;
	bool semanticEquals(std::string_view s) const noexcept;
	
// ------------------------------------------------------------------------------------------ //	
};