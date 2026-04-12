#include "Value.hpp"
#include <string>
#include <cstring>
#include <charconv>

using namespace std;


// ---------------------------------- [ Constructors ] -------------------------------------- //


Value::Value(String* str){
	if (str == nullptr){
		type = Type::NONE;
	} else {
		type = Type::STRING;
		data.s = str->ref();
	}
}


Value::Value(Object* obj){
	if (obj == nullptr){
		type = Type::NONE;
	} else {
		type = Type::OBJECT;
		data.o = obj->ref();
	}
}


Value::Value(string_view s){
	this->type = Type::STRING;
	this->data.s = String::create(s).release()->ref();
}


Value::Value(string_view s1, string_view s2) : type{Type::STRING} {
	this->type = Type::STRING;
	this->data.s = String::create(s1, s2).release()->ref();
}


Value::Value(const Value& o){
	this->type = o.type;
	
	switch (o.type){
		case Type::NONE:
		case Type::LONG:
		case Type::DOUBLE:
			this->data = o.data;
			break;
		
		case Type::STRING:
			assert(o.data.s != nullptr);
			this->data.s = o.data.s->ref();
			break;
		
		case Type::OBJECT:
			assert(o.data.o != nullptr);
			this->data.o = o.data.o->ref();
			break;
			
	}
	
}


Value::~Value(){
	switch (type){
		case Type::NONE:
		case Type::LONG:
		case Type::DOUBLE:
			break;
		
		case Type::STRING:
			assert(data.s != nullptr);
			data.s->unref();
			break;
		
		case Type::OBJECT:
			assert(data.o != nullptr);
			data.o->unref();
			break;
			
	}
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


unique_ptr<Value::String> Value::String::create(uint32_t len){
	len = min(len, UINT32_MAX - 1);
	
	void* mem = ::operator new(sizeof(String) + sizeof(*String::str)*(len + 1), align_val_t(alignof(String)));
	unique_ptr str = unique_ptr<Value::String>(new (mem) String());
	
	str->str[0] = 0;
	str->str[len] = 0;
	str->len = len;
	return str;
}

unique_ptr<Value::String> Value::String::create(string_view s1){
	const size_t l1 = min(s1.length(), size_t(UINT32_MAX - 1));
	unique_ptr<Value::String> s = create(uint32_t(l1));
	copy(s1.begin(), s1.begin() + l1, s->str);
	return s;
}

unique_ptr<Value::String> Value::String::create(string_view s1, string_view s2){
	const size_t l1 = min(s1.length(), size_t(UINT32_MAX - 1));
	const size_t l2 = min(s2.length(), size_t(UINT32_MAX - 1) - l1);
	assert(l1 + l2 <= UINT32_MAX - 1);
	
	unique_ptr<Value::String> s = create(uint32_t(l1 + l2));
	copy(s1.begin(), s1.begin() + l1, s->str + 0);
	copy(s2.begin(), s2.begin() + l2, s->str + l1);
	return s;
}


unique_ptr<Value::String> Value::String::fmt(long val){
	unique_ptr<Value::String> s = create(24 - 1);
	int n = snprintf(s->str, s->len, "%li", val);
	
	if (n < 0){
		s->len = 0;
		return s;
	} else if (uint32_t(n) < s->len){
		s->len = uint32_t(n);
		return s;
	}
	
	// Resize buffer and retry.
	s = create(uint32_t(n) + 1);
	n = snprintf(s->str, s->len, "%li", val);
	
	if (n < 0){
		s->len = 0;
	} else if (uint32_t(n) < s->len){
		s->len = uint32_t(n);
	}
	
	return s;
}


unique_ptr<Value::String> Value::String::fmt(double val){
	int len = 32 - 1;
	
	unique_ptr<Value::String> s = create(uint32_t(len));
	int n = snprintf(s->str, s->len, "%lf", val);
	
	if (n < 0){
		s->len = 0;
		return s;
	}
	
	// Resize buffer and retry.
	else if (n >= len){
		len = n + 1;
		s = create(uint32_t(len));
		n = snprintf(s->str, s->len, "%lf", val);
		n = min(n, len);
		
		if (n < 0){
			s->len = 0;
			return s;
		}
		
	}
	
	// Trimm trailing 0.
	while (n > 2 && s->str[n-1] == '0' && s->str[n-2] != '.'){
		n--;
	}
	
	s->len = uint32_t(n);
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr bool isWhitespace(auto c){
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}


static bool try_parse(string_view s, auto& out_n) noexcept {
	from_chars_result res = from_chars(s.begin(), s.end(), out_n);
	if (res.ec != errc()){
		return false;
	}
	
	// Parse trailing whitespace
	const char* end = s.end();
	const char* p = res.ptr;
	while (p != end && isWhitespace(*p)) p++;
	
	return (p == end);
}


static string str(double n){
	string s = to_string(n);
	
	while (s.length() > 2){
		const char* c = &s.back();
		if (c[0] == '0' && c[-1] != '.')
			s.pop_back();
		else
			break;
	}
	
	return s;
}


static string str(const Value::Object& o){
	string s = "[";
	bool comma = false;
	
	for (const Value& el : o.arr){
		if (comma)
			s.push_back(',');
		comma = true;
		
		if (el.type == Value::Type::STRING){
			s.append("\"");
			el.toStr(s);
			s.push_back('"');
		} else {
			el.toStr(s);
		}
	}
	
	for (const auto& pair : o.dict){
		if (comma)
			s.push_back(',');
		comma = true;
		
		s.append("\"").append(pair.key).append("\":");
		
		if (pair.value.type == Value::Type::STRING){
			s.append("\"");
			pair.value.toStr(s);
			s.push_back('"');
		} else {
			pair.value.toStr(s);
		}
	}
	
	s.push_back(']');
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


void Value::Object::merge(const Object& o2){
	if (this == &o2){
		return;
	}
	
	arr.insert(arr.end(), o2.arr.begin(), o2.arr.end());
	for (auto p : o2.dict){
		dict.insert(p.key, p.value);
	}
	
}


bool Value::Object::operator==(const Value::Object& o) const {
	if (this == &o){
		return true;
	} else if (arr.size() != o.arr.size() || dict.size() != o.dict.size()){
		return false;
	}
	
	for (size_t i = 0 ; i < arr.size() ; i++){
		if (arr[i] != o.arr[i])
			return false;
	}
	
	for (const auto& pair : dict){
		const Value* v1 = &pair.value;
		const Value* v2 = o.get(pair.key);
		if (v1 == nullptr || v2 == nullptr){
			if (v1 != v2)
				return false;
		} else if (*v1 != *v2){
			return false;
		}
	}
	
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool Value::getBool() const noexcept {
	switch (type){
		case Type::NONE:
			return false;
		case Type::LONG:
			return (data.l != 0);
		case Type::DOUBLE:
			return (data.d != 0);
		case Type::OBJECT:
			return (!data.o->arr.empty() || !data.o->dict.empty());
		
		case Type::STRING: {
			string_view s = data.s->sv();
			
			if (s.length() == 4){
				return strncasecmp(s.data(), "true", 4) == 0;
			} else if (s.length() == 5){
				return strncasecmp(s.data(), "false", 5) != 0;
			}
			
			return !s.empty();
		}
		
	}
	return false;
}


string Value::toStr() const {
	switch (type){
		case Type::NONE:
			return string();
		case Type::LONG:
			return to_string(data.l);
		case Type::DOUBLE:
			return str(data.d);
		case Type::STRING:
			return string(string_view(*data.s));
		case Type::OBJECT:
			return str(*data.o);
	}
	return {};
}


string& Value::toStr(string& buff) const {
	switch (type){
		case Type::NONE:
			return buff;
		case Type::LONG:
			return buff.append(to_string(data.l));
		case Type::DOUBLE:
			return buff.append(str(data.d));
		case Type::STRING:
			return buff.append(string_view(*data.s));
		case Type::OBJECT:
			return buff.append(str(*data.o));
	}
	return buff;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Value Value::cast_bool() const noexcept {
	return Value(getBool() ? 1L : 0L);
}


Value Value::cast_int() const noexcept {
	switch (type){
		case Type::NONE:
			break;
		
		case Type::LONG:
			return Value(data.l);
			
		case Type::DOUBLE:
			return Value(static_cast<long>(data.d));
		
		case Type::STRING:
			assert(data.s != nullptr);
			return Value(atol(data.s->begin()));
		
		case Type::OBJECT:
			assert(data.o != nullptr);
			return Value(static_cast<long>(data.o->arr.size()));
		
	}
	return Value(0L);
}


Value Value::cast_float() const noexcept {
	switch (type){
		case Type::NONE:
			break;
		
		case Type::LONG:
			return Value(static_cast<double>(data.l));
			
		case Type::DOUBLE:
			return Value(data.d);
		
		case Type::STRING:
			assert(data.s != nullptr);
			return Value(atof(data.s->begin()));
		
		case Type::OBJECT:
			assert(data.o != nullptr);
			return Value(static_cast<double>(data.o->arr.size()));
		
	}
	return Value(0.0);
}


Value Value::cast_str() const {
	switch (type){
		case Type::NONE:
			break;
		
		case Type::LONG:
			return Value(Value::String::fmt(data.l).release());
		
		case Type::DOUBLE:
			return Value(Value::String::fmt(data.d).release());
		
		case Type::STRING:
			return Value(data.s);
		
		case Type::OBJECT: {
			assert(data.o != nullptr);
			string x = str(*data.o);
			return Value(Value::String::create(x).release());
		} break;
		
	}
	return Value(Value::String::create(0).release());
}


Value Value::cast_obj() const {
	switch (type){
		case Type::NONE:
			break;
		
		case Type::LONG:
		case Type::DOUBLE:
		case Type::STRING: {
			unique_ptr<Value::Object> obj = Value::Object::create();
			obj->arr.emplace_back(*this);
			return Value(obj.release());
		}
		
		case Type::OBJECT:
			return Value(data.o);
		
	}
	return Value(Value::Object::create().release());
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool Value::equals(const Value& o) const noexcept {
	if (type != o.type){
		return false;
	}
	
	switch (type){
		case Type::NONE:
			return true;
		
		case Type::LONG: {
			if (o.type == Type::LONG)
				return data.l == o.data.l;
			else if (o.type == Type::DOUBLE)
				return data.l == o.data.d;
			else
				return false;
		}
		
		case Type::DOUBLE: {
			if (o.type == Type::LONG)
				return data.d == o.data.l;
			else if (o.type == Type::DOUBLE)
				return data.d == o.data.d;
			else
				return false;
		}
		
		case Type::STRING: {
			if (o.type == Type::STRING)
				return string_view(*data.s) == string_view(*o.data.s);
			else
				return false;
		}
		
		case Type::OBJECT: {
			if (o.type == Type::OBJECT)
				return *data.o == *o.data.o;
			else
				return false;
		}
		
	}
	
	return false;
}


bool Value::semanticEquals(string_view s) const noexcept {
	switch (type){
		case Type::NONE: {
			return s.empty();
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
		
		case Type::STRING: [[likely]] {
			assert(data.s != nullptr);
			return (data.s->sv() == s);
		}
		
		case Type::OBJECT: {
			long n = 0;
			if (s.empty())
				return (data.o->arr.empty() && data.o->dict.empty());
			else if (try_parse(s, n))
				return static_cast<long>(data.o->arr.size()) == n;
			else
				return false;
		}
		
	}
	return false;
}


bool Value::semanticEquals(const Value& o) const noexcept {
	if (o.type == Type::STRING){
		return semanticEquals(string_view(*data.s));
	}
	
	switch (type){
		case Type::NONE: {
			if (o.type == Type::NONE)
				return true;
			else if (o.type == Type::LONG)
				return o.data.l == 0;
			else if (o.type == Type::DOUBLE)
				return o.data.d == 0;
			else if (o.type == Type::OBJECT)
				return o.data.o->arr.empty() && o.data.o->dict.empty();
		} break;
		
		case Type::LONG: {
			if (o.type == Type::NONE)
				return data.l == 0;
			else if (o.type == Type::LONG)
				return data.l == o.data.l;
			else if (o.type == Type::DOUBLE)
				return data.l == o.data.d;
			else if (o.type == Type::OBJECT)
				return data.l == static_cast<long>(o.data.o->arr.size());
		} break;
		
		case Type::DOUBLE: {
			if (o.type == Type::NONE)
				return data.d == 0;
			else if (o.type == Type::LONG)
				return data.d == o.data.l;
			else if (o.type == Type::DOUBLE)
				return data.d == o.data.d;
			else if (o.type == Type::OBJECT)
				return data.d == static_cast<long>(o.data.o->arr.size());
		} break;
		
		case Type::STRING: {
			return o.semanticEquals(string_view(*data.s));
		} break;
		
		case Type::OBJECT: {
			if (o.type == Type::NONE)
				return data.o->arr.empty() && data.o->dict.empty();
			else if (o.type == Type::LONG)
				return static_cast<long>(data.o->arr.size()) == o.data.l;
			else if (o.type == Type::DOUBLE)
				return data.o->arr.size() == o.data.d;
			else if (o.type == Type::OBJECT)
				return *data.o == *o.data.o;
		} break;
		
	}
	
	return false;
}


// ------------------------------------------------------------------------------------------ //