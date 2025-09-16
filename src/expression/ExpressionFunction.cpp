#include "ExpressionOperation.hpp"
#include <cmath>
#include <algorithm>
#include <regex>

#include "Debug.hpp"

using namespace std;
using Operation = Expression::Operation;


// ----------------------------------- [ Functions ] ---------------------------------------- //


#define P(s) ANSI_PURPLE s ANSI_RESET
#define VAL_STR(val)	(toStr(val).c_str())

#define IS_TYPE(e, t)	(std::is_same_v<std::decay_t<decltype(e)>, t>)
#define IS_STR(e)		(IS_TYPE(e, string))
#define IS_LONG(e)		(IS_TYPE(e, long))
#define IS_DOUBLE(e)	(IS_TYPE(e, double))
#define IS_NUM(e)		(IS_LONG(e) || IS_DOUBLE(e))


Value eval(const Operation& op, const VariableMap& vars, const Debugger& dbg);


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value f_defined(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("int(e)") ".\n"));
		return Value(0);
	} else if (f.argc > 1){
		HERE(dbg.warn(f.argv[1].mark, "Too many arguments (%d/1) in function " P("int(e)") ".\n", f.argc));
	}
	
	const Operation* arg0 = f.argv[0].expr;
	
	assert(arg0 != nullptr);
	if (arg0->type != Operation::Type::VAR){
		HERE(dbg.error(f.argv[0].mark, "Expected variable name.\n"));
		return Value(0);
	}
	
	const Variable& var = static_cast<const Variable&>(*arg0);
	const Value* val = vars.get(var.name);
	return Value(val != nullptr ? 1L : 0L);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value f_int(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("int(e)") ".\n"));
		return Value(0);
	} else if (f.argc > 1){
		HERE(dbg.warn(f.argv[1].mark, "Too many arguments (%d/1) in function " P("int(e)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
	}
	
	auto cast = [](const auto& v) -> long {
		if constexpr (IS_STR(v))
			return atol(v.c_str());
		else
			return long(v);
	};
	
	return visit(cast, eval(*f.argv[0].expr, vars, dbg));
}


static Value f_float(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("float(e)") ".\n"));
		return Value(0.0);
	} else if (f.argc > 1){
		HERE(dbg.warn(f.argv[1].mark, "Too many arguments (%d/1) in function " P("float(e)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
	}
	
	auto cast = [](const auto& v) -> double {
		if constexpr (IS_STR(v))
			return atof(v.c_str());
		else
			return double(v);
	};
	
	return visit(cast, eval(*f.argv[0].expr, vars, dbg));
}


static Value f_str(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("str(e)") ".\n"));
		return Value(in_place_type<string>);
	} else if (f.argc > 1){
		HERE(dbg.warn(f.argv[1].mark, "Too many arguments (%d/1) in function " P("str(e)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
	}
	
	auto cast = [&](auto&& v) -> string {
		if constexpr (IS_STR(v))
			return move(v);
		else
			return to_string(v);
	};
	
	return visit(cast, eval(*f.argv[0].expr, vars, dbg));
}


static Value f_len(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.warn(f.name, "Missing argument in function " P("%.*s(e)") ".\n", int(f.name.length()), f.name.data()));
		return Value(0);
	} else if (f.argc > 1){
		HERE(dbg.warn(f.argv[1].mark, "Too many arguments (%d/1) in function " P("%.*s(e)") ".\n", f.argc, int(f.name.length()), f.name.data()));
	} else {
		assert(f.argv[0].expr != nullptr);
	}
	
	auto cast = [](auto&& v) -> Value {
		if constexpr IS_STR(v)
			return long(v.length());
		else
			return abs(v);
	};
	
	return visit(cast, eval(*f.argv[0].expr, vars, dbg));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool toNum(const Value& val, long& out_i, double& out_f){
	auto cast = [&](const auto& v) -> bool {
		if constexpr IS_STR(v) {
			out_i = long(v.length());
			return true;
		} else if IS_LONG(v) {
			out_i = v;
			return true;
		} else {
			out_f = v;
			return false;
		}
	};
	return visit(cast, val);
}


static Value f_min(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.warn(f.name, "Missing argument in function " P("min(a,...)") ".\n"));
		return Value(0);
	}
	
	long min_int;
	double min_flt;
	Value min_val = eval(*f.argv[0].expr, vars, dbg);
	bool min_isint = toNum(min_val, min_int, min_flt);
	
	for (int i = 1 ; i < f.argc ; i++){
		Value val = eval(*f.argv[i].expr, vars, dbg);
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


static Value f_max(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.warn(f.name, "Missing argument in function " P("max(a,...)") ".\n"));
		return Value(0);
	}
	
	long max_int;
	double max_flt;
	Value max_val = eval(*f.argv[0].expr, vars, dbg);
	bool max_isint = toNum(max_val, max_int, max_flt);
	
	for (int i = 1 ; i < f.argc ; i++){
		Value val = eval(*f.argv[i].expr, vars, dbg);
		long val_int;
		double val_flt;
		
		if (toNum(val, val_int, val_flt)){
			if ((max_isint && val_int < max_int) || (!max_isint && val_int < max_flt)){
				max_isint = true;
				max_int = val_int;
				max_val = move(val);
			}
		} else {
			if ((max_isint && val_flt < max_int) || (!max_isint && val_flt < max_flt)){
				max_isint = false;
				max_flt = val_flt;
				max_val = move(val);
			}
		}
		
	}
	
	return max_val;
}


static Value f_if(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 3){
		HERE(dbg.error(f.name, "Missing arguments (%d/3) in function " P("if(cond,true_expr,false_expr)") ".\n", f.argc));
		return Value(in_place_type<string>);
	} else if (f.argc > 3){
		HERE(dbg.warn(f.argv[3].mark, "Too many arguments (%d/3) in function " P("if(cond,true_expr,false_expr)") ".\n", f.argc));
	}
	
	assert(f.argv[0].expr != nullptr);
	if (toBool(eval(*f.argv[0].expr, vars, dbg))){
		assert(f.argv[1].expr != nullptr);
		return eval(*f.argv[1].expr, vars, dbg);
	} else {
		assert(f.argv[2].expr != nullptr);
		return eval(*f.argv[2].expr, vars, dbg);
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline void tolower(string& s){
	transform(s.begin(), s.end(), s.begin(), [](unsigned char c){
		return tolower(c);
	});
}

inline void toupper(string& s){
	transform(s.begin(), s.end(), s.begin(), [](unsigned char c){
		return toupper(c);
	});
}

inline string substr(string&& s, size_t i, long len){
	if (i > s.length()){
		return string();
	} else if (len < 0){
		long end = long(s.length()) + len + 1;
		if (end < long(i))
			return s.substr(end, i - end);
		else
			return s.substr(i, end - i);
	} else {
		return s.substr(i, len);
	}
}


static Value f_lower(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("lower(str)") ".\n"));
		return Value(in_place_type<string>);
	} else if (f.argc > 1){
		HERE(dbg.warn(f.argv[1].mark, "Too many arguments (%d/1) in function " P("lower(str)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
	}
	
	Value arg_0 = eval(*f.argv[0].expr, vars, dbg);
	
	if (string* str = get_if<string>(&arg_0)){
		tolower(*str);
		return arg_0;
	} else if (long* n = get_if<long>(&arg_0)){
		string str = to_string(*n);
		tolower(str);
		return Value(move(str));
	} else if (double* n = get_if<double>(&arg_0)){
		string str = to_string(*n);
		tolower(str);
		return Value(move(str));
	}
	
	assert(false);
	return Value(0);
}


static Value f_upper(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("upper(str)") ".\n"));
		return Value(in_place_type<string>);
	} else if (f.argc > 1){
		HERE(dbg.warn(f.argv[1].mark, "Too many arguments (%d/1) in function " P("upper(str)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
	}
	
	Value arg_0 = eval(*f.argv[0].expr, vars, dbg);
	
	if (string* str = get_if<string>(&arg_0)){
		toupper(*str);
		return arg_0;
	} else if (long* n = get_if<long>(&arg_0)){
		string str = to_string(*n);
		toupper(str);
		return Value(move(str));
	} else if (double* n = get_if<double>(&arg_0)){
		string str = to_string(*n);
		toupper(str);
		return Value(move(str));
	}
	
	assert(false);
	return Value(0);
}


static Value f_substr(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 3){
		HERE(dbg.error(f.name, "Missing arguments (%d/3) in function " P("substr(str,i,len)") ".\n", f.argc));
		return Value(in_place_type<string>);
	} else if (f.argc > 3){
		HERE(dbg.warn(f.argv[3].mark, "Too many arguments (%d/3) in function " P("substr(str,i,len)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
		assert(f.argv[1].expr != nullptr);
		assert(f.argv[2].expr != nullptr);
	}
	
	Value arg_0 = eval(*f.argv[0].expr, vars, dbg);
	Value arg_1 = eval(*f.argv[1].expr, vars, dbg);
	Value arg_2 = eval(*f.argv[2].expr, vars, dbg);
	
	// Parse arg 1, [i]
	size_t i = 0;
	if (long* n = get_if<long>(&arg_1)){
		if (*n > 0)
			i = size_t(*n);
	} else {
		HERE(dbg.error(f.argv[1].mark, "Argument " P("i={%s}") " in function " P("substr(str,i,len)") " must be an integer.\n", VAL_STR(arg_1)));
		return arg_0;
	}
	
	// Parse arg 2, [len]
	long len;
	if (long* n = get_if<long>(&arg_2)){
		len = *n;
	} else {
		HERE(dbg.error(f.argv[2].mark, "Argument " P("len={%s}") " in function " P("substr(str,i,len)") " must be an integer.\n", VAL_STR(arg_2)));
		return arg_0;
	}
	
	// Substring
	auto cast = [i,len](auto&& val){
		if constexpr IS_STR(val)
			return substr(move(val), i, len);
		else
			return substr(to_string(val), i, len);
	};
	
	return visit(cast, move(arg_0));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value f_match(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 2){
		HERE(dbg.error(f.name, "Missing arguments (%d/2) in function " P("match(str,reg)") ".\n", f.argc));
		return Value(0);
	} else if (f.argc > 2){
		HERE(dbg.warn(f.argv[2].mark, "Too many arguments (%d/2) in function " P("match(str,reg)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
		assert(f.argv[1].expr != nullptr);
	}
	
	// Parse argument 0 [str]
	Value arg_str = eval(*f.argv[0].expr, vars, dbg);
	string* str = get_if<string>(&arg_str);
	if (str == nullptr){
		HERE(dbg.error(f.argv[0].mark, "Argument " P("str={%s}") " in function " P("match(str,reg)") " must be a string.\n", VAL_STR(arg_str)));
		return Value(0);
	}
	
	// Parse argument 1, [reg]
	Value arg_reg = eval(*f.argv[1].expr, vars, dbg);
	string* reg_s = get_if<string>(&arg_reg);
	if (reg_s == nullptr){
		HERE(dbg.error(f.argv[1].mark, "Argument " P("reg={%s}") " in function " P("match(str,reg)") " must be a regular expression string.\n", VAL_STR(arg_reg)));
		return Value(0);
	}
	
	// Create regex
	regex reg;
	try {
		reg = regex(*reg_s);
	} catch (...){
		assert(reg_s != nullptr);
		HERE(dbg.error(f.argv[1].mark, "Invalid regular expression " P("reg={%s}") " in function " P("match(str,reg)") ".\n", reg_s->c_str()));
		return Value(0);
	}
	
	try {
		bool m = regex_match(*str, reg);
		return Value(m ? 1L : 0L);
	} catch (...){
		assert(str != nullptr && reg_s != nullptr);
		HERE(dbg.error(f.name, "Failed to evaluate function " P("match(str,reg)") ":\n str=" P("'%s'") ", reg=" P("'%s'") "\n", str->c_str(), reg_s->c_str()));
		return Value(0);
	}
	
}


static Value f_replace(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 3){
		HERE(dbg.error(f.name, "Missing arguments (%d/3) in function " P("replace(str,reg,rep)") ".\n", f.argc));
		return Value("");
	} else if (f.argc > 3){
		HERE(dbg.warn(f.argv[3].mark, "Too many arguments (%d/3) in function " P("replace(str,reg,rep)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
		assert(f.argv[1].expr != nullptr);
		assert(f.argv[2].expr != nullptr);
	}
	
	// Parse argument 0 [str]
	Value arg_str = eval(*f.argv[0].expr, vars, dbg);
	string* str = get_if<string>(&arg_str);
	if (str == nullptr){
		HERE(dbg.error(f.argv[0].mark, "Argument " P("str={%s}") " in function " P("replace(str,reg,rep)") " must be a string.\n", VAL_STR(arg_str)));
		return arg_str;
	}
	
	// Parse argument 1, [reg]
	Value arg_reg = eval(*f.argv[1].expr, vars, dbg);
	string* reg_s = get_if<string>(&arg_reg);
	if (reg_s == nullptr){
		HERE(dbg.error(f.argv[1].mark, "Argument " P("reg={%s}") " in function " P("replace(str,reg,rep)") " must be a regular expression string.\n", VAL_STR(arg_reg)));
		return arg_str;
	}
	
	// Parse argument 2, [rep]
	Value arg_rep = eval(*f.argv[2].expr, vars, dbg);
	string* rep = get_if<string>(&arg_rep);
	if (rep == nullptr){
		HERE(dbg.error(f.argv[2].mark, "Argument " P("rep={%s}") " in function " P("replace(str,reg,rep)") " must be a string.\n", VAL_STR(arg_rep)));
		return arg_str;
	}
	
	// Create regex
	regex reg;
	try {
		reg = regex(*reg_s);
	} catch (...){
		assert(reg_s != nullptr);
		HERE(dbg.error(f.argv[1].mark, "Invalid regular expression " P("reg={%s}") " in function " P("replace(str,reg,rep)") ".\n", reg_s->c_str()));
		return arg_str;
	}
	
	try {
		return Value(regex_replace(*str, reg, *rep));
	} catch (...){
		assert(str != nullptr && reg_s != nullptr && rep != nullptr);
		HERE(dbg.error(f.name, "Failed to evaluate function " P("replace(str,reg,rep)") ":\n str=" P("'%s'") ", reg=" P("'%s'") ", rep=" P("'%s'") "\n", str->c_str(), reg_s->c_str(), rep->c_str()));
		return arg_str;
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Value eval(const Function& f, const VariableMap& vars, const Debugger& dbg){
	string_view name = f.name;
	try {
		switch (name.length()){
			case 2:
				if (name == "if")
					return f_if(f, vars, dbg);
				break;
			case 3:
				if (name == "int")
					return f_int(f, vars, dbg);
				else if (name == "str")
					return f_str(f, vars, dbg);
				else if (name == "len" || name == "abs")
					return f_len(f, vars, dbg);
				else if (name == "min")
					return f_min(f, vars, dbg);
				else if (name == "max")
					return f_max(f, vars, dbg);
				break;
			case 5:
				if (name == "float")
					return f_float(f, vars, dbg);
				else if (name == "lower")
					return f_lower(f, vars, dbg);
				else if (name == "upper")
					return f_upper(f, vars, dbg);
				else if (name == "match")
					return f_match(f, vars, dbg);
				break;
			case 6:
				if (name == "substr")
					return f_substr(f, vars, dbg);
				break;
			case 7:
				if (name == "replace")
					return f_replace(f, vars, dbg);
				else if (name == "defined")
					return f_defined(f, vars, dbg);
				break;
		}
	} catch (...) {
		HERE(dbg.warn(name, "Failed to evaluate function " ANSI_PURPLE "'%.*s()'" ANSI_RESET " due to internal exception.\n", int(name.length()), name.data()));
		return 0;
	}
	
	HERE(dbg.warn(name, "Undefined function " ANSI_PURPLE "'%.*s()'" ANSI_RESET " defaulted to 0.\n", int(name.length()), name.data()));
	return 0;
}


// ------------------------------------------------------------------------------------------ //
