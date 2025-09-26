#include "Value.hpp"
#include <string>
#include <charconv>

using namespace std;


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool try_parse(string_view s, auto& out_n) noexcept {
	from_chars_result res = from_chars(s.begin(), s.end(), out_n);
	if (res.ec != errc()){
		return false;
	}
	
	// Parse trailing whitespace
	const char* end = s.end();
	const char* p = res.ptr;
	while (p != end && isspace(*p)) p++;
	
	return (p == end);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool Value::toBool() const noexcept {
	switch (type){
		case Type::LONG:
			return (data.l != 0);
		case Type::DOUBLE:
			return (data.d != 0);
		case Type::STRING:
			return (data_len != 0);
	}
	return false;
}


string Value::toStr() const {
	switch (type){
		case Type::LONG:
			return to_string(data.l);
		case Type::DOUBLE:
			return to_string(data.d);
		case Type::STRING:
			return string(data.s, data_len);
	}
	return {};
}


string& Value::toStr(string& buff) const {
	switch (type){
		case Type::LONG:
			buff += to_string(data.l);
			break;
		case Type::DOUBLE:
			buff += to_string(data.d);
			break;
		case Type::STRING:
			buff.append(data.s, data_len);
			break;
	}
	return buff;
}


// ----------------------------------- [ Operators ] ---------------------------------------- //


bool Value::semanticEquals(string_view s) const noexcept {
	switch (type){
		case Type::STRING: {
			[[likely]]
			return sv() == s;
		}
		
		case Type::LONG: {
			long n;
			if (try_parse(s, n))
				return data.l == n;
			else
				return false;
		}
		
		case Type::DOUBLE: {
			double n;
			if (try_parse(s, n))
				return data.d == n;
			else
				return false;
		}
		
	}
	return false;
}


bool Value::semanticEquals(const Value& o) const noexcept {
	if (o.type == Type::STRING){
		return semanticEquals(o.sv());
	}
	
	switch (type){
		case Type::LONG: {
			if (o.type == Type::LONG)
				return data.l == o.data.l;
			else if (o.type == Type::DOUBLE)
				return data.l == o.data.d;
		}
		
		case Type::DOUBLE: {
			if (o.type == Type::LONG)
				return data.d == o.data.l;
			else if (o.type == Type::DOUBLE)
				return data.d == o.data.d;
		}
		
		case Type::STRING: {
			return o.semanticEquals(sv());
		}
		
	}
	
	return false;
}


bool Value::operator==(const Value& o) const noexcept {
	if (type == Type::LONG){
		if (o.type == Type::LONG)
			return data.l == o.data.l;
		else if (o.type == Type::DOUBLE)
			return data.l == o.data.d;
	}
	
	else if (type == Type::DOUBLE){
		if (o.type == Type::LONG)
			return data.d == o.data.l;
		else if (o.type == Type::DOUBLE)
			return data.d == o.data.d;
	}
	
	else if (type == Type::STRING){
		if (o.type == Type::STRING)
			return sv() == o.sv();
	}
	
	return false;
}


// ------------------------------------------------------------------------------------------ //