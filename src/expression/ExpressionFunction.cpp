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


#define VAL_STR(val)	(val.toStr().c_str())


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
	return (vars.get(var.name()) != nullptr) ? 1L : 0L;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value f_int(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "int(e)"));
		return Value(0L);
	} else if (f.argc > 1){
		HERE(warn_arg_overflow(self, f, 1, "int(e)"));
	}
	
	assert(f.argv[0] != nullptr);
	Value val = eval(self, *f.argv[0], vars);
	
	switch (val.type){
		case Type::STRING: [[likely]]
			val = atol(val.data.s);
			break;
		case Type::DOUBLE:
			val = long(val.data.d);
			break;
		case Type::LONG: [[unlikely]]
			break;
	}
	
	return val;
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
	
	switch (val.type){
		case Type::STRING: [[likely]]
			val = atof(val.data.s);
			break;
		case Type::LONG:
			val = double(val.data.l);
			break;
		case Type::DOUBLE: [[unlikely]]
			break;
	}
	
	return val;
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
	
	switch (val.type){
		case Type::LONG:
		case Type::DOUBLE:
			val = Value(val.toStr());
			break;
		case Type::STRING: [[unlikely]]
			break;
	}
	
	return val;
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
		case Type::LONG:
			val.data.l = abs(val.data.l);
			break;
		case Type::DOUBLE:
			val.data.d = abs(val.data.d);
			break;
		case Type::STRING:
			val = long(val.data_len);
			break;
	}
	
	return val;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool toNum(const Value& val, long& out_i, double& out_f){
	switch (val.type){
		case Type::LONG:
			out_i = val.data.l;
			return true;
		case Type::DOUBLE:
			out_f = val.data.d;
			return false;
		case Type::STRING:
			out_i = long(val.data_len);
			return true;
	}
	out_i = 0;
	return true;
}


static Value f_min(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "min(x, ...)"));
		return {};
	}
	
	long min_int;
	double min_flt;
	Value min_val = eval(self, *f.argv[0], vars);
	bool min_isint = toNum(min_val, min_int, min_flt);
	
	for (uint32_t i = 1 ; i < f.argc ; i++){
		Value val = eval(self, *f.argv[i], vars);
		long val_int;
		double val_flt;
		
		if (toNum(val, val_int, val_flt)){
			if ((min_isint && val_int < min_int) || (!min_isint && val_int < min_flt)){
				min_isint = true;
				min_int = val_int;
				min_val = move(val);
			}
		} else {
			if ((min_isint && val_flt < min_int) || (!min_isint && val_flt < min_flt)){
				min_isint = false;
				min_flt = val_flt;
				min_val = move(val);
			}
		}
		
	}
	
	return min_val;
}


static Value f_max(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 1){
		HERE(error_arg_underflow(self, f, 1, "max(x, ...)"));
		return {};
	}
	
	long max_int;
	double max_flt;
	Value max_val = eval(self, *f.argv[0], vars);
	bool max_isint = toNum(max_val, max_int, max_flt);
	
	for (uint32_t i = 1 ; i < f.argc ; i++){
		Value val = eval(self, *f.argv[i], vars);
		long val_int;
		double val_flt;
		
		if (toNum(val, val_int, val_flt)){
			if ((max_isint && val_int > max_int) || (!max_isint && val_int > max_flt)){
				max_isint = true;
				max_int = val_int;
				max_val = move(val);
			}
		} else {
			if ((max_isint && val_flt > max_int) || (!max_isint && val_flt > max_flt)){
				max_isint = false;
				max_flt = val_flt;
				max_val = move(val);
			}
		}
		
	}
	
	return max_val;
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
		case Type::STRING:
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
		case Type::STRING:
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
	
	if (cond.toBool()){
		assert(f.argv[1] != nullptr);
		return eval(self, *f.argv[1], vars);
	} else {
		assert(f.argv[2] != nullptr);
		return eval(self, *f.argv[2], vars);
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline void lowercase(char* beg, uint32_t len){
	transform(beg, beg + len, beg, [](unsigned char c){
		return tolower(c);
	});
}

inline void uppercase(char* beg, uint32_t len){
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
		case Type::LONG:
			val = Value(to_string(val.data.l));
			break;
		case Type::DOUBLE:
			val = Value(to_string(val.data.d));
			break;
		case Type::STRING: [[likely]]
			lowercase(val.data.s, val.data_len);
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
		case Type::LONG:
			val = Value(to_string(val.data.l));
			break;
		case Type::DOUBLE:
			val = Value(to_string(val.data.d));
			break;
		case Type::STRING: [[likely]]
			uppercase(val.data.s, val.data_len);
			break;
	}
	
	return val;
}


static Value f_substr(const Expression& self, const Function& f, const VariableMap& vars){
	if (f.argc < 2){
		HERE(error_arg_underflow(self, f, 2, "substr(str, beg, [len])"));
		return string_view();
	} else if (f.argc > 3){
		HERE(warn_arg_overflow(self, f, 3, "substr(str, beg, [len])"));
	}
	
	assert(f.argv[0] != nullptr);
	assert(f.argv[1] != nullptr);
	Value arg_0 = eval(self, *f.argv[0], vars);
	Value arg_1 = eval(self, *f.argv[1], vars);
	Value arg_2;
	
	const bool hasLen = (f.argc >= 3);
	if (hasLen){
		assert(f.argv[2] != nullptr);
		arg_2 = eval(self, *f.argv[2], vars);
	}
	
	string str_buff;
	string_view str;
	long beg;
	long len;
	
	// Extract argument 1 [beg]
	switch (arg_1.type){
		case Type::LONG:
			beg = arg_1.data.l;
			break;
		case Type::DOUBLE:
			beg = long(arg_1.data.d);
			break;
		default:
			HERE(error_arg_expected_int(self, *f.argv[1], "beg", "substr(str, beg, [len])"));
			return arg_0;
	}
	
	// Extract argument 2 [len]
	if (hasLen){
		switch (arg_2.type){
			case Type::LONG:
				len = arg_2.data.l;
				break;
			case Type::DOUBLE:
				len = long(arg_2.data.d);
				break;
			default:
				HERE(error_arg_expected_int(self, *f.argv[2], "len", "substr(str, beg, [len])"));
				return arg_0;
		}
	}
	
	// Extract argument 0 [str]
	switch (arg_0.type){
		case Type::LONG:
			str_buff = to_string(arg_0.data.l);
			str = str_buff;
			break;
		case Type::DOUBLE:
			str_buff = to_string(arg_0.data.d);
			str = str_buff;
			break;
		case Type::STRING:
			str = arg_0.sv();
			break;
	}
	
	// Normalize beg and end
	beg = (beg >= 0) ? beg : long(str.length()) + beg;
	long end = (hasLen) ? beg + len : INT64_MAX;
	
	if (beg > end){
		swap(beg, end);
	}
	
	beg = max(0L, beg);
	end = min(end, long(str.length()));
	
	// Substring
	if (beg >= end)
		return Value(""sv);
	else if (beg == 0 && end == long(str.length()) && arg_0.type == Type::STRING)
		return arg_0;
	else
		return Value(str.substr(beg, end - beg));
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
		reg = regex(arg_reg.data.s, arg_reg.data_len);
	} catch (...){
		HERE(error_arg_expected_regex(self, *f.argv[1], "reg", "match(str, reg)"));
		return 0L;
	}
	
	try {
		bool m = regex_match(arg_str.data.s, arg_str.data.s + arg_str.data_len, reg);
		return Value(m ? 1L : 0L);
	} catch (...){
		HERE(error_arg_expected_regex(self, *f.argv[1], "reg", "match(str, reg)"));
		return 0L;
	}
	
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
	
	// Create regex
	regex reg;
	try {
		reg = regex(arg_reg.data.s, arg_reg.data_len);
	} catch (...){
		HERE(error_arg_expected_regex(self, *f.argv[1], "reg", "replace(str, reg, rep)"));
		return arg_str;
	}
	
	try {
		assert(arg_str.data.s[arg_str.data_len] == 0);
		assert(arg_rep.data.s[arg_rep.data_len] == 0);
		string buff = regex_replace(arg_str.data.s, reg, arg_rep.data.s);
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
			case 5:
				if (name == "float")
					return f_float(self, f, vars);
				else if (name == "lower")
					return f_lower(self, f, vars);
				else if (name == "upper")
					return f_upper(self, f, vars);
				else if (name == "match")
					return f_match(self, f, vars);
				break;
			case 6:
				if (name == "substr")
					return f_substr(self, f, vars);
				break;
			case 7:
				if (name == "replace")
					return f_replace(self, f, vars);
				else if (name == "defined")
					return f_defined(self, f, vars);
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
