#include "ExpressionOperation.hpp"
#include <cmath>
#include <algorithm>
#include <regex>

#include "Debug.hpp"
#include "DebugSource.hpp"

using namespace std;
using Operation = Expression::Operation;
using Type = Value::Type;


// ----------------------------------- [ Prototypes ] --------------------------------------- //


Value eval(const Expression& self, const Operation& op, const VariableMap& vars);
Value eval(const Expression& self, const Function& op, const VariableMap& vars);


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void error_undefined_function(const Expression& self, const Function& func){
	if (self.origin == nullptr){
		return;
	}
	
	string_view mark = func.name();
	linepos pos = findLine(*self.origin, mark.begin());
	
	print(pos);
	LOG_STDERR(ERROR_PFX "Undefined function: " PURPLE("'%.*s()'") "\n", VA_STRV(func.name()));
	printCodeView(pos, mark, ANSI_RED);
}

static void error_arg_underflow(const Expression& self, const Function& func, int argc_req, string_view sig){
	if (self.origin == nullptr){
		return;
	}
	
	string_view mark = func.view();
	linepos pos = findLine(*self.origin, mark.begin());
	
	print(pos);
	LOG_STDERR(ERROR_PFX "Missing arguments (" RED("%d") "/" PURPLE("%d") ") in function " PURPLE("%.*s") ".\n", func.argc, argc_req, VA_STRV(sig));
	printCodeView(pos, mark, ANSI_RED);
}

static void warn_arg_overflow(const Expression& self, const Function& func, int argc_req, string_view sig){
	if (self.origin == nullptr){
		return;
	}
	
	string_view mark = func.view();
	if (func.argc > 0 && func.argv[func.argc-1] != nullptr){
		mark = func.argv[func.argc-1]->view();
	}
	
	linepos pos = findLine(*self.origin, mark.begin());
	
	print(pos);
	LOG_STDERR(WARN_PFX "Too many arguments (" YELLOW("%d") "/" PURPLE("%d") ") in function " PURPLE("%.*s") ".\n", func.argc, argc_req, VA_STRV(sig));
	printCodeView(pos, mark, ANSI_YELLOW);
}

static void error_arg_expected_var(const Expression& self, const Operation& arg, string_view arg_name, string_view sig){
	if (self.origin == nullptr){
		return;
	}
	
	string_view mark = arg.view();
	linepos pos = findLine(*self.origin, mark.begin());
	
	print(pos);
	LOG_STDERR(ERROR_PFX "Argument " PURPLE("%.*s") " in function " PURPLE("%.*s") " must be a variable.\n", VA_STRV(arg_name), VA_STRV(sig));
	printCodeView(pos, mark, ANSI_RED);
}

static void error_arg_expected_int(const Expression& self, const Operation& arg, string_view arg_name, string_view sig){
	if (self.origin == nullptr){
		return;
	}
	
	string_view mark = arg.view();
	linepos pos = findLine(*self.origin, mark.begin());
	
	print(pos);
	LOG_STDERR(ERROR_PFX "Argument " PURPLE("%.*s") " in function " PURPLE("%.*s") " must be an integer.\n", VA_STRV(arg_name), VA_STRV(sig));
	printCodeView(pos, mark, ANSI_RED);
}

static void error_arg_expected_str(const Expression& self, const Operation& arg, string_view arg_name, string_view sig){
	if (self.origin == nullptr){
		return;
	}
	
	string_view mark = arg.view();
	linepos pos = findLine(*self.origin, mark.begin());
	
	print(pos);
	LOG_STDERR(ERROR_PFX "Argument " PURPLE("%.*s") " in function " PURPLE("%.*s") " must be a string.\n", VA_STRV(arg_name), VA_STRV(sig));
	printCodeView(pos, mark, ANSI_RED);
}

static void error_arg_expected_arr(const Expression& self, const Operation& arg, string_view arg_name, string_view sig){
	if (self.origin == nullptr){
		return;
	}
	
	string_view mark = arg.view();
	linepos pos = findLine(*self.origin, mark.begin());
	
	print(pos);
	LOG_STDERR(ERROR_PFX "Argument " PURPLE("%.*s") " in function " PURPLE("%.*s") " must be an array.\n", VA_STRV(arg_name), VA_STRV(sig));
	printCodeView(pos, mark, ANSI_RED);
}

static void error_arg_expected_regex(const Expression& self, const Operation& arg, string_view arg_name, string_view sig){
	if (self.origin == nullptr){
		return;
	}
	
	string_view mark = arg.view();
	linepos pos = findLine(*self.origin, mark.begin());
	
	print(pos);
	LOG_STDERR(ERROR_PFX "Argument " PURPLE("%.*s") " in function " PURPLE("%.*s") " must be a regex.\n", VA_STRV(arg_name), VA_STRV(sig));
	printCodeView(pos, mark, ANSI_RED);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value f_defined(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "defined(var)"));
		return 0L;
	} else if (f.argc > 1){
		HERE(warn_arg_overflow(self, f, 1, "defined(var)"));
	}
	
	assert(f.argv[0] != nullptr);
	const Operation* arg0 = f.argv[0];
	
	if (arg0->type != Operation::Type::VAR){
		HERE(error_arg_expected_var(self, *arg0, "var", "defined(var)"));
		return 0L;
	}
	
	const Variable& var = static_cast<const Variable&>(*arg0);
	const Value* val = vars.get(var.name());
	return (val != nullptr && val->type != Value::Type::NONE) ? 1L : 0L;
}


static Value f_coalesce(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "coalesce(x, ...)"));
		return Value();
	}
	
	for (uint32_t i = 0 ; i < f.argc ; i++){
		assert(f.argv[i] != nullptr);
		const Operation& arg = *f.argv[i];
		
		// Check for variable.
		if (arg.type == Operation::Type::VAR){
			const Variable& var = static_cast<const Variable&>(arg);
			const Value* val = vars.get(var.name());
			
			if (val != nullptr && val->type != Type::NONE){
				return *val;
			}
			
		}
		
		// Evaluate expression.
		else {
			Value val = eval(self, arg, vars);
			if (val.type != Type::NONE)
				return val;
		}
		
	}
	
	return Value();
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value f_bool(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "bool(e)"));
		return Value(0L);
	} else if (f.argc > 1){
		HERE(warn_arg_overflow(self, f, 1, "bool(e)"));
	}
	
	assert(f.argv[0] != nullptr);
	Value val = eval(self, *f.argv[0], vars);
	return val.cast_bool();
}


static Value f_int(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "int(e)"));
		return Value(0L);
	} else if (f.argc > 1){
		HERE(warn_arg_overflow(self, f, 1, "int(e)"));
	}
	
	assert(f.argv[0] != nullptr);
	Value val = eval(self, *f.argv[0], vars);
	return val.cast_int();
}


static Value f_float(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "float(e)"));
		return Value(0.0);
	} else if (f.argc > 1){
		HERE(warn_arg_overflow(self, f, 1, "float(e)"));
	}
	
	assert(f.argv[0] != nullptr);
	Value val = eval(self, *f.argv[0], vars);
	return val.cast_float();
}


static Value f_str(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "str(e)"));
		return string_view();
	} else if (f.argc > 1){
		HERE(warn_arg_overflow(self, f, 1, "str(e)"));
	}
	
	assert(f.argv[0] != nullptr);
	Value val = eval(self, *f.argv[0], vars);
	return val.cast_str();
}


static Value f_len(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "len(e)"));
		return Value(0L);
	} else if (f.argc > 1){
		HERE(warn_arg_overflow(self, f, 1, "len(e)"));
	}
	
	assert(f.argv[0] != nullptr);
	Value val = eval(self, *f.argv[0], vars);
	
	switch (val.type){
		case Type::NONE:
			val = Value(0L);
			break;
		case Type::LONG:
			val.data.l = abs(val.data.l);
			break;
		case Type::DOUBLE:
			val.data.d = abs(val.data.d);
			break;
		case Type::STRING:
			val = static_cast<long>(val.data.s->len);
			break;
		case Type::OBJECT:
			val = static_cast<long>(val.data.o->arr.size());
			break;
	}
	
	return val;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool cmp(const Value& v1, const Value& v2, auto op){
	switch (v1.type){
		case Type::NONE:
			return false;
		
		case Type::LONG: {
			switch (v2.type){
				case Type::NONE:
					return true;
				case Type::LONG:
					return op(v1.data.l, v2.data.l);
				case Type::DOUBLE:
					return op(v1.data.l, v2.data.d);
				case Type::STRING:
					return op(v1.data.l, v2.data.s->len);
				case Type::OBJECT:
					return op(v1.data.l, v2.data.o->arr.size());
			}
		} break;
		
		case Type::DOUBLE: {
			switch (v2.type){
				case Type::NONE:
					return true;
				case Type::LONG:
					return op(v1.data.d, v2.data.l);
				case Type::DOUBLE:
					return op(v1.data.d, v2.data.d);
				case Type::STRING:
					return op(v1.data.d, v2.data.s->len);
				case Type::OBJECT:
					return op(v1.data.d, v2.data.o->arr.size());
			}
		} break;
		
		case Type::STRING: {
			switch (v2.type){
				case Type::NONE:
					return true;
				case Type::LONG:
					return op(v1.data.s->len, v2.data.l);
				case Type::DOUBLE:
					return op(v1.data.s->len, v2.data.d);
				case Type::STRING:
					return op(v1.data.s->len, v2.data.s->len);
				case Type::OBJECT:
					return op(v1.data.s->len, v2.data.o->arr.size());
			}
		} break;
		
		case Type::OBJECT: {
			switch (v2.type){
				case Type::NONE:
					return true;
				case Type::LONG:
					return op(v1.data.o->arr.size(), v2.data.l);
				case Type::DOUBLE:
					return op(v1.data.o->arr.size(), v2.data.d);
				case Type::STRING:
					return op(v1.data.o->arr.size(), v2.data.s->len);
				case Type::OBJECT:
					return op(v1.data.o->arr.size(), v2.data.o->arr.size());
			}
		} break;
		
	}
	assert(false);
	return false;
}


static Value f_min(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "min(x, ...)"));
		return {};
	}
	
	Value result;
	auto op = std::less();
	
	for (uint32_t i = 0 ; i < f.argc ; i++){
		assert(f.argv[i] != nullptr);
		Value next = eval(self, *f.argv[i], vars);
		
		// Unroll array argument.
		if (next.type == Type::OBJECT){
			for (const Value& el : next.data.o->arr){
				if (cmp(el, result, op))
					result = el;			// Array must not be modified.
			}
		}
		
		else if (cmp(next, result, op)){
			result = move(next);
		}
		
	}
	
	return result;
}


static Value f_max(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "max(x, ...)"));
		return {};
	}
	
	Value result;
	auto op = std::greater();
	
	for (uint32_t i = 0 ; i < f.argc ; i++){
		assert(f.argv[i] != nullptr);
		Value next = eval(self, *f.argv[i], vars);
		
		// Unroll array argument.
		if (next.type == Type::OBJECT){
			for (const Value& el : next.data.o->arr){
				if (cmp(el, result, op))
					result = el;			// Array must not be modified.
			}
		}
		
		else if (cmp(next, result, op)){
			result = move(next);
		}
		
	}
	
	return result;
}


static Value f_sin(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "sin(x)"));
		return Value(0L);
	} else if (f.argc > 1){
		HERE(warn_arg_overflow(self, f, 1, "sin(x)"));
	}
	
	assert(f.argv[0] != nullptr);
	Value val = eval(self, *f.argv[0], vars);
	
	switch (val.type){
		case Type::LONG:
			val.data.d = sin(double(val.data.l));
			val.type = Type::DOUBLE;
			break;
		case Type::DOUBLE:
			val.data.d = sin(val.data.d);
			break;
		case Type::NONE:
		case Type::STRING:
		case Type::OBJECT:
			break;
	}
	
	return val;
}


static Value f_cos(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "cos(x)"));
		return Value(0L);
	} else if (f.argc > 1){
		HERE(warn_arg_overflow(self, f, 1, "cos(x)"));
	}
	
	assert(f.argv[0] != nullptr);
	Value val = eval(self, *f.argv[0], vars);
	
	switch (val.type){
		case Type::LONG:
			val.data.d = cos(double(val.data.l));
			val.type = Type::DOUBLE;
			break;
		case Type::DOUBLE:
			val.data.d = cos(val.data.d);
			break;
		case Type::NONE:
		case Type::STRING:
		case Type::OBJECT:
			break;
	}
	
	return val;
}


static Value f_if(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 3){
		HERE(error_arg_underflow(self, f, 3, "if(cond, true_expr, false_expr)"));
		return {};
	} else if (f.argc > 3){
		HERE(warn_arg_overflow(self, f, 3, "if(cond, true_expr, false_expr)"));
	}
	
	assert(f.argv[0] != nullptr);
	Value cond = eval(self, *f.argv[0], vars);
	
	if (cond.getBool()){
		assert(f.argv[1] != nullptr);
		return eval(self, *f.argv[1], vars);
	} else {
		assert(f.argv[2] != nullptr);
		return eval(self, *f.argv[2], vars);
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline void lowercase(char* beg, size_t len){
	transform(beg, beg + len, beg, [](unsigned char c){
		return tolower(c);
	});
}

inline void uppercase(char* beg, size_t len){
	transform(beg, beg + len, beg, [](unsigned char c){
		return toupper(c);
	});
}

inline string_view substr(string_view s, long beg, long end){
	beg = (beg >= 0) ? beg : (s.length() + beg);
	end = (end >= 0) ? end : (s.length() + end + 1);
	if (end < beg){
		swap(beg, end);
	}
	
	beg = max(0L, beg);
	end = min(end, long(s.length()));
	
	if (beg >= end)
		return string_view();
	else
		return s.substr(beg, end - beg);
}


static Value f_lower(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "lower(str)"));
		return string_view();
	} else if (f.argc > 1){
		HERE(warn_arg_overflow(self, f, 1, "lower(str)"));
	}
	
	assert(f.argv[0] != nullptr);
	Value val = eval(self, *f.argv[0], vars);
	
	switch (val.type){
		case Type::STRING: [[likely]]
			if (val.data.s->refs > 1)
				val = Value(val.data.s->sv());
			lowercase(val.data.s->str, val.data.s->len);
			break;
		case Type::LONG:
			val = Value(to_string(val.data.l));
			break;
		case Type::DOUBLE:
			val = Value(to_string(val.data.d));
			break;
		case Type::NONE:
		case Type::OBJECT:
			break;
	}
	
	return val;
}


static Value f_upper(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "upper(str)"));
		return string_view();
	} else if (f.argc > 1){
		HERE(warn_arg_overflow(self, f, 1, "upper(str)"));
	}
	
	assert(f.argv[0] != nullptr);
	Value val = eval(self, *f.argv[0], vars);
	
	switch (val.type){
		case Type::STRING: [[likely]]
			if (val.data.s->refs > 1)
				val = Value(val.data.s->sv());
			uppercase(val.data.s->str, val.data.s->len);
			break;
		case Type::LONG:
			val = Value(to_string(val.data.l));
			break;
		case Type::DOUBLE:
			val = Value(to_string(val.data.d));
			break;
		case Type::NONE:
		case Type::OBJECT:
			break;
	}
	
	return val;
}


static Value f_slice(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 2){
		HERE(error_arg_underflow(self, f, 2, "slice(arr, beg, [len])"));
		return string_view();
	} else if (f.argc > 3){
		HERE(warn_arg_overflow(self, f, 3, "slice(arr, beg, [len])"));
	}
	
	assert(f.argv[0] != nullptr);
	assert(f.argv[1] != nullptr);
	Value arg_0 = eval(self, *f.argv[0], vars);
	Value arg_1 = eval(self, *f.argv[1], vars);
	Value arg_2;
	
	if (f.argc >= 3){
		assert(f.argv[2] != nullptr);
		arg_2 = eval(self, *f.argv[2], vars);
	}
	
	int64_t size = 0;
	int64_t beg = 0;
	int64_t end = -1;
	int64_t len = -1;
	
	// Verify argument 0 [arr]
	switch (arg_0.type){
		case Type::STRING:
			size = arg_0.data.s->len;
			break;
		case Type::OBJECT:
			size = arg_0.data.o->arr.size();
			break;
		default:
			HERE(error_arg_expected_arr(self, *f.argv[0], "arr", "slice(arr, beg, [len])"));
			return arg_0;
	}
	
	// Extract argument 1 [beg]
	switch (arg_1.type){
		case Type::LONG:
			beg = arg_1.data.l;
			break;
		case Type::DOUBLE:
			beg = long(arg_1.data.d);
			break;
		default:
			HERE(error_arg_expected_int(self, *f.argv[1], "beg", "slice(arr, beg, [len])"));
			return arg_0;
	}
	
	// Extract argument 2 [len]
	switch (arg_2.type){
		case Type::NONE:
			break;
		case Type::LONG:
			len = arg_2.data.l;
			break;
		case Type::DOUBLE:
			len = long(arg_2.data.d);
			break;
		default:
			HERE(error_arg_expected_int(self, *f.argv[2], "len", "slice(arr, beg, [len])"));
			return arg_0;
	}
	
	// Normalize begining and length.
	beg = (beg < 0) ? size + beg : beg;
	beg = min(max(0L, beg), size);
	end = (len < 0) ? size + len + 1 : beg + len;
	end = min(max(beg, end), size);
	len = end - beg;
	
	switch (arg_0.type){
		case Type::STRING: {
			string_view s = arg_0.data.s->sv();
			return Value(s.substr(beg, len));
		}
		
		case Type::OBJECT: {
			Value o2 = Value(new Value::Object());
			const vector<Value>& v1 = arg_0.data.o->arr;
			vector<Value>& v2 = o2.data.o->arr;
			v2.insert(v2.end(), v1.begin() + beg, v1.begin() + end);
			return o2; 
		}
		
		default:
			break;
	}
	
	assert(false);
	return Value();
}


static Value f_split(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 2){
		HERE(error_arg_underflow(self, f, 2, "split(str, delim)"));
		return Value();
	} else if (f.argc > 2){
		HERE(warn_arg_overflow(self, f, 2, "split(str, delim)"));
	}
	
	// Extract argument 0 [str]
	assert(f.argv[0] != nullptr);
	Value arg_str = eval(self, *f.argv[0], vars);
	
	if (arg_str.type == Type::OBJECT){
		return arg_str;
	}
	
	arg_str = arg_str.cast_str();
	string_view str = arg_str.data.s->sv();
	
	// Extract argument 1 [delim]
	assert(f.argv[1] != nullptr);
	Value arg_delim = eval(self, *f.argv[1], vars);
	string_view delim;
	
	switch (arg_delim.type){
		case Type::NONE:
		case Type::LONG:
		case Type::DOUBLE:
			arg_delim = arg_delim.cast_str();
			delim = arg_delim.data.s->sv();
			break;
			
		case Type::OBJECT:
			HERE(error_arg_expected_regex(self, *f.argv[1], "delim", "split(str, delim)"));
			return Value();
		
		case Type::STRING:
			delim = arg_delim.data.s->sv();
			break;
	}
	
	unique_ptr<Value::Object> obj = Value::Object::create();
	string str_buff;
	
	// Split on every character.
	if (delim.empty()){
		obj->arr.reserve(str.size());
		
		for (char c : str){
			obj->arr.emplace_back(string_view(&c, 1));
		}
		
		goto _ret;
	}
	
	// Split using regex.
	try {
		regex reg = regex(delim.begin(), delim.end());
		cregex_token_iterator tok = cregex_token_iterator(str.begin(), str.end(), reg, -1);
		cregex_token_iterator end = cregex_token_iterator();
		
		for (; tok != end ; tok++){
			string_view s = string_view(tok->first, tok->length());
			obj->arr.emplace_back(s);
		}
		
	} catch (...){
		HERE(error_arg_expected_regex(self, *f.argv[1], "delim", "split(str, delim)"));
		goto _err_ret;
	}
	
	_ret:
	return Value(obj.release());
	
	_err_ret:
	obj->arr.emplace_back(move(arg_str));
	goto _ret;
}


static Value f_join(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "join(arr, [sep])"));
		return Value();
	} else if (f.argc > 2){
		HERE(warn_arg_overflow(self, f, 2, "join(arr, sep)"));
	}
	
	// Extract argument 0 [arr]
	assert(f.argv[0] != nullptr);
	Value arg_arr = eval(self, *f.argv[0], vars);
	
	if (arg_arr.type != Type::OBJECT){
		return arg_arr.cast_str();
	}
	
	const vector<Value>& arr = arg_arr.data.o->arr;
	
	// Extract argument 1 [sep]
	Value arg_sep;
	string_view sep = "";
	
	if (f.argc >= 2){
		assert(f.argv[1] != nullptr);
		arg_sep = eval(self, *f.argv[1], vars).cast_str();
		assert(arg_sep.type == Type::STRING);
		sep = arg_sep.data.s->sv();
	}
	
	string buff;
	for (size_t i = 0 ; i < arr.size() ; i++){
		if (i > 0)
			buff.append(sep);
		arr[i].toStr(buff);
	}
	
	return Value(buff);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value f_match(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 2){
		HERE(error_arg_underflow(self, f, 2, "match(str, reg)"));
		return 0L;
	} else if (f.argc > 2){
		HERE(warn_arg_overflow(self, f, 2, "match(str, reg)"));
	}
	
	assert(f.argv[0] != nullptr);
	assert(f.argv[1] != nullptr);
	
	// Parse argument 0 [str]
	Value arg_str = eval(self, *f.argv[0], vars);
	if (arg_str.type != Type::STRING){
		HERE(error_arg_expected_str(self, *f.argv[0], "str", "match(str, reg)"));
		return 0L;
	}
	
	// Parse argument 1, [reg]
	Value arg_reg = eval(self, *f.argv[1], vars);
	if (arg_reg.type != Type::STRING){
		HERE(error_arg_expected_regex(self, *f.argv[1], "reg", "match(str, reg)"));
		return 0L;
	}
	
	// Create regex
	regex reg;
	try {
		reg = regex(arg_reg.data.s->begin(), arg_reg.data.s->end());
	} catch (...){
		HERE(error_arg_expected_regex(self, *f.argv[1], "reg", "match(str, reg)"));
		return 0L;
	}
	
	try {
		bool m = regex_match(arg_str.data.s->begin(), arg_str.data.s->end(), reg);
		return m ? 1L : 0L;
	} catch (...){
		HERE(error_arg_expected_regex(self, *f.argv[1], "reg", "match(str, reg)"));
	}
	
	return 0L;
}


static Value f_replace(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 3){
		HERE(error_arg_underflow(self, f, 3, "replace(str, reg, rep)"));
		return Value(""sv);
	} else if (f.argc > 3){
		HERE(warn_arg_overflow(self, f, 3, "replace(str, reg, rep)"));
	}
	
	assert(f.argv[0] != nullptr);
	assert(f.argv[1] != nullptr);
	assert(f.argv[2] != nullptr);
	
	// Parse argument 0 [str]
	Value arg_str = eval(self, *f.argv[0], vars);
	if (arg_str.type != Type::STRING){
		HERE(error_arg_expected_str(self, *f.argv[0], "str", "replace(str, reg, rep)"));
		return arg_str;
	}
	
	// Parse argument 1, [reg]
	Value arg_reg = eval(self, *f.argv[1], vars);
	if (arg_reg.type != Type::STRING){
		HERE(error_arg_expected_regex(self, *f.argv[1], "reg", "replace(str, reg, rep)"));
		return arg_str;
	}
	
	// Parse argument 2, [rep]
	Value arg_rep = eval(self, *f.argv[2], vars);
	if (arg_rep.type != Type::STRING){
		HERE(error_arg_expected_str(self, *f.argv[2], "rep", "replace(str, reg, rep)"));
		return arg_str;
	}
	
	// Strings are C-strings
	assert(*arg_str.data.s->end() == '\0');
	assert(*arg_rep.data.s->end() == '\0');
	
	// Create regex
	try {
		regex reg = regex(arg_reg.data.s->begin(), arg_reg.data.s->end());
		string buff = regex_replace(arg_str.data.s->begin(), reg, arg_rep.data.s->begin());
		arg_str = Value(move(buff));
	} catch (...){
		HERE(error_arg_expected_regex(self, *f.argv[1], "reg", "replace(str, reg, rep)"));
	}
	
	return arg_str;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Value eval(const Expression& self, const Function& f, const VariableMap& vars){
	string_view name = f.name();
	try {
		switch (name.length()){
			case 2:
				if (name == "if")
					return f_if(self, f, vars);
				break;
			case 3:
				if (name == "int")
					return f_int(self, f, vars);
				else if (name == "str")
					return f_str(self, f, vars);
				else if (name == "len" || name == "abs")
					return f_len(self, f, vars);
				else if (name == "min")
					return f_min(self, f, vars);
				else if (name == "max")
					return f_max(self, f, vars);
				else if (name == "sin")
					return f_sin(self, f, vars);
				else if (name == "cos")
					return f_cos(self, f, vars);
				break;
			case 4:
				if (name == "bool")
					return f_bool(self, f, vars);
				else if (name == "join")
					return f_join(self, f, vars);
				break;
			case 5:
				if (name == "float")
					return f_float(self, f, vars);
				else if (name == "slice")
					return f_slice(self, f, vars);
				else if (name == "split")
					return f_split(self, f, vars);
				else if (name == "lower")
					return f_lower(self, f, vars);
				else if (name == "upper")
					return f_upper(self, f, vars);
				else if (name == "match")
					return f_match(self, f, vars);
				break;
			case 7:
				if (name == "replace")
					return f_replace(self, f, vars);
				else if (name == "defined")
					return f_defined(self, f, vars);
				break;
			case 8:
				if (name == "coalesce")
					return f_coalesce(self, f, vars);
				break;
		}
	} catch (...) {
		HERE(ERROR("Failed to evaluate function " PURPLE("%.*s()") " due to an internal exception.", VA_STRV(name)));
		return {};
	}
	
	HERE(error_undefined_function(self, f));
	return {};
}


// ------------------------------------------------------------------------------------------ //
