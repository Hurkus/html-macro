#include "Expression-impl.hpp"
#include <type_traits>
#include <algorithm>
#include <regex>

#include "Debug.hpp"

using namespace std;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


#define P(s) ANSI_PURPLE s ANSI_RESET

#define IS_TYPE(e, t)	(std::is_same_v<std::decay_t<decltype(e)>, t>)
#define IS_STR(e)		(IS_TYPE(e, StrValue))
#define IS_NUM(e)		(IS_TYPE(e, long) || IS_TYPE(e, double))

#define VAL_STR(val)	(toStr(val).c_str())


inline Value _eval(const VariableMap& vars, const pExpr& e, const Debugger& dbg){
	if (e != nullptr){
		return e->eval(vars, dbg);
	} else {
		HERE(dbg.error(dbg.mark(), "Internal error; faulty expression tree defaulted to 0.\n"));
		assert(e != nullptr);
		return Value(0);
	}
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value f_int(const Func& f, const VariableMap& vars, const Debugger& dbg) noexcept {
	if (f.args.size() < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("int(e)") ".\n"));
		return Value(0);
	} else if (f.args.size() > 1){
		HERE(dbg.warn(f.name, "Too many arguments (%ld/1) in function " P("int(e)") ".\n", f.args.size()));
	}
	
	auto cast = [](const auto& v) -> long {
		if constexpr (IS_STR(v))
			return atol(v.c_str());
		else
			return long(v);
	};
	
	return visit(cast, _eval(vars, f.args[0], dbg));
}


static Value f_float(const Func& f, const VariableMap& vars, const Debugger& dbg){
	if (f.args.size() < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("float(e)") ".\n"));
		return Value(0.0);
	} else if (f.args.size() > 1){
		HERE(dbg.warn(f.name, "Too many arguments (%ld/1) in function " P("float(e)") ".\n", f.args.size()));
	}
	
	auto cast = [](const auto& v) -> double {
		if constexpr (IS_STR(v))
			return atof(v.c_str());
		else
			return double(v);
	};
	
	return visit(cast, _eval(vars, f.args[0], dbg));
}


static Value f_str(const Func& f, const VariableMap& vars, const Debugger& dbg){
	if (f.args.size() < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("str(e)") ".\n"));
		return Value(in_place_type<string>);
	} else if (f.args.size() > 1){
		HERE(dbg.warn(f.name, "Too many arguments (%ld/1) in function " P("str(e)") ".\n", f.args.size()));
	}
	
	auto cast = [&](auto&& v) -> string {
		if constexpr (IS_STR(v))
			return move(v);
		else
			return to_string(v);
	};
	
	return visit(cast, _eval(vars, f.args[0], dbg));
}


static Value f_len(const Func& f, const VariableMap& vars, const Debugger& dbg){
	if (f.args.size() < 1){
		HERE(dbg.warn(f.name, "Missing argument in function " P("%.*s(e)") ".\n", f.name.length(), f.name.data()));
		return Value(0);
	} else if (f.args.size() > 1){
		HERE(dbg.warn(f.name, "Too many arguments (%ld/1) in function " P("%.*s(e)") ".\n", f.args.size(), f.name.length(), f.name.data()));
	}
	
	auto cast = [](auto&& v) -> Value {
		if constexpr (IS_STR(v))
			return long(v.length());
		else
			return abs(v);
	};
	
	return visit(cast, _eval(vars, f.args[0], dbg));
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


static Value f_lower(const Func& f, const VariableMap& vars, const Debugger& dbg){
	if (f.args.size() < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("lower(str)") ".\n"));
		return Value(in_place_type<string>);
	} else if (f.args.size() > 1){
		HERE(dbg.warn(f.name, "Too many arguments (%ld/1) in function " P("lower(str)") ".\n", f.args.size()));
	}
	
	Value arg_0 = _eval(vars, f.args[0], dbg);
	
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


static Value f_upper(const Func& f, const VariableMap& vars, const Debugger& dbg){
	if (f.args.size() < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("upper(str)") ".\n"));
		return Value(in_place_type<string>);
	} else if (f.args.size() > 1){
		HERE(dbg.warn(f.name, "Too many arguments (%ld/1) in function " P("upper(str)") ".\n", f.args.size()));
	}
	
	Value arg_0 = _eval(vars, f.args[0], dbg);
	
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


static Value f_substr(const Func& f, const VariableMap& vars, const Debugger& dbg){
	if (f.args.size() < 3){
		HERE(dbg.error(f.name, "Missing argument in function " P("substr(str,i,len)") ".\n"));
		return Value(in_place_type<string>);
	} else if (f.args.size() > 3){
		HERE(dbg.warn(f.name, "Too many arguments (%ld/3) in function " P("substr(str,i,len)") ".\n", f.args.size()));
	}
	
	Value arg_0 = _eval(vars, f.args[0], dbg);
	Value arg_1 = _eval(vars, f.args[1], dbg);
	Value arg_2 = _eval(vars, f.args[2], dbg);
	
	// Parse arg 1, [i]
	size_t i = 0;
	if (long* n = get_if<long>(&arg_1)){
		if (*n > 0)
			i = size_t(*n);
	} else {
		HERE(dbg.error(f.name, "Argument " P("i={%s}") " in function " P("substr(str,i,len)") " must be an integer.\n", VAL_STR(arg_1)));
		return arg_0;
	}
	
	// Parse arg 1, [i]
	long len;
	if (long* n = get_if<long>(&arg_2)){
		len = *n;
	} else {
		HERE(dbg.error(f.name, "Argument " P("len={%s}") " in function " P("substr(str,i,len)") " must be an integer.\n", VAL_STR(arg_2)));
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


static Value f_match(const Func& f, const VariableMap& vars, const Debugger& dbg){
	if (f.args.size() < 2){
		HERE(dbg.error(f.name, "Missing arguments (%ld/2) in function " P("match(str,reg)") ".\n", f.args.size()));
		return Value(0);
	} else if (f.args.size() > 2){
		HERE(dbg.warn(f.name, "Too many arguments (%ld/2) in function " P("match(str,reg)") ".\n", f.args.size()));
	}
	
	// Parse argument 0 [str]
	Value arg_str = _eval(vars, f.args[0], dbg);
	string* str = get_if<string>(&arg_str);
	if (str == nullptr){
		HERE(dbg.error(f.name, "Argument " P("str={%s}") " in function " P("match(str,reg)") " must be a string.\n", VAL_STR(arg_str)));
		return Value(0);
	}
	
	// Parse argument 1, [reg]
	Value arg_reg = _eval(vars, f.args[1], dbg);
	string* reg_s = get_if<string>(&arg_reg);
	if (reg_s == nullptr){
		HERE(dbg.error(f.name, "Argument " P("reg={%s}") " in function " P("match(str,reg)") " must be a regular expression string.\n", VAL_STR(arg_reg)));
		return Value(0);
	}
	
	// Create regex
	regex reg;
	try {
		reg = regex(*reg_s);
	} catch (...){
		assert(reg_s != nullptr);
		HERE(dbg.error(f.name, "Invalid regular expression " P("reg={%s}") " in function " P("match(str,reg)") ".\n", reg_s->c_str()));
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


static Value f_replace(const Func& f, const VariableMap& vars, const Debugger& dbg){
	if (f.args.size() < 3){
		HERE(dbg.error(f.name, "Missing arguments (%ld/3) in function " P("replace(str,reg,rep)") ".\n", f.args.size()));
		return Value("");
	} else if (f.args.size() > 3){
		HERE(dbg.warn(f.name, "Too many arguments (%ld/3) in function " P("replace(str,reg,rep)") ".\n", f.args.size()));
	}
	
	// Parse argument 0 [str]
	Value arg_str = _eval(vars, f.args[0], dbg);
	string* str = get_if<string>(&arg_str);
	if (str == nullptr){
		HERE(dbg.error(f.name, "Argument " P("str={%s}") " in function " P("replace(str,reg,rep)") " must be a string.\n", VAL_STR(arg_str)));
		return arg_str;
	}
	
	// Parse argument 1, [reg]
	Value arg_reg = _eval(vars, f.args[1], dbg);
	string* reg_s = get_if<string>(&arg_reg);
	if (reg_s == nullptr){
		HERE(dbg.error(f.name, "Argument " P("reg={%s}") " in function " P("replace(str,reg,rep)") " must be a regular expression string.\n", VAL_STR(arg_reg)));
		return arg_str;
	}
	
	// Parse argument 2, [rep]
	Value arg_rep = _eval(vars, f.args[2], dbg);
	string* rep = get_if<string>(&arg_rep);
	if (rep == nullptr){
		HERE(dbg.error(f.name, "Argument " P("rep={%s}") " in function " P("replace(str,reg,rep)") " must be a string.\n", VAL_STR(arg_rep)));
		return arg_str;
	}
	
	// Create regex
	regex reg;
	try {
		reg = regex(*reg_s);
	} catch (...){
		assert(reg_s != nullptr);
		HERE(dbg.error(f.name, "Invalid regular expression " P("reg={%s}") " in function " P("replace(str,reg,rep)") ".\n", reg_s->c_str()));
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


Value Expression::Func::eval(const VariableMap& vars, const Debugger& dbg) noexcept {
	try {
		switch (name.length()){
			case 3:
				if (name == "int")
					return f_int(*this, vars, dbg);
				else if (name == "str")
					return f_str(*this, vars, dbg);
				else if (name == "len" || name == "abs")
					return f_len(*this, vars, dbg);
				break;
			case 5:
				if (name == "float")
					return f_float(*this, vars, dbg);
				else if (name == "lower")
					return f_lower(*this, vars, dbg);
				else if (name == "upper")
					return f_upper(*this, vars, dbg);
				else if (name == "match")
					return f_match(*this, vars, dbg);
				break;
			case 6:
				if (name == "substr")
					return f_substr(*this, vars, dbg);
				break;
			case 7:
				if (name == "replace")
					return f_replace(*this, vars, dbg);
				break;
		}
	} catch (...) {
		HERE(dbg.warn(name, "Failed to evaluate function " ANSI_PURPLE "'%.*s()'" ANSI_RESET " due to internal exception.\n", name.length(), name.data()));
		return 0;
	}
	
	HERE(dbg.warn(name, "Undefined function " ANSI_PURPLE "'%.*s()'" ANSI_RESET " defaulted to 0.\n", name.length(), name.data()));
	return 0;
}


// ------------------------------------------------------------------------------------------ //