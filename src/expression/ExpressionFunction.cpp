#include "ExpressionOperation.hpp"
#include <cmath>
#include <algorithm>
#include <regex>

#include "Debug.hpp"

using namespace std;
using Operation = Expression::Operation;
using Type = Value::Type;


// ----------------------------------- [ Prototypes ] --------------------------------------- //


#define P(s) ANSI_PURPLE s ANSI_RESET
#define VAL_STR(val)	(val.toStr().c_str())


Value eval(const Operation& op, const VariableMap& vars, const Debugger& dbg);


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value f_defined(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("int(e)") ".\n"));
		return 0L;
	} else if (f.argc > 1){
		HERE(dbg.warn(f.argv[1].mark, "Too many arguments (%d/1) in function " P("int(e)") ".\n", f.argc));
	}
	
	const Operation* arg0 = f.argv[0].expr;
	
	assert(arg0 != nullptr);
	if (arg0->type != Operation::Type::VAR){
		HERE(dbg.error(f.argv[0].mark, "Expected variable name.\n"));
		return 0L;
	}
	
	const Variable& var = static_cast<const Variable&>(*arg0);
	return (vars.get(var.name) != nullptr) ? 1L : 0L;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value f_int(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("int(e)") ".\n"));
		return Value(0L);
	} else if (f.argc > 1){
		HERE(dbg.warn(f.argv[1].mark, "Too many arguments (%d/1) in function " P("int(e)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
	}
	
	Value val = eval(*f.argv[0].expr, vars, dbg);
	
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


static Value f_float(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("float(e)") ".\n"));
		return Value(0.0);
	} else if (f.argc > 1){
		HERE(dbg.warn(f.argv[1].mark, "Too many arguments (%d/1) in function " P("float(e)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
	}
	
	Value val = eval(*f.argv[0].expr, vars, dbg);
	
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


static Value f_str(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("str(e)") ".\n"));
		return string_view();
	} else if (f.argc > 1){
		HERE(dbg.warn(f.argv[1].mark, "Too many arguments (%d/1) in function " P("str(e)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
	}
	
	Value val = eval(*f.argv[0].expr, vars, dbg);
	
	switch (val.type){
		case Type::LONG:
			val = Value(to_string(val.data.l));
			break;
		case Type::DOUBLE:
			val = Value(to_string(val.data.d));
			break;
		case Type::STRING: [[unlikely]]
			break;
	}
	
	return val;
}


static Value f_len(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.warn(f.name, "Missing argument in function " P("%.*s(e)") ".\n", int(f.name.length()), f.name.data()));
		return Value(0L);
	} else if (f.argc > 1){
		HERE(dbg.warn(f.argv[1].mark, "Too many arguments (%d/1) in function " P("%.*s(e)") ".\n", f.argc, int(f.name.length()), f.name.data()));
	} else {
		assert(f.argv[0].expr != nullptr);
	}
	
	Value val = eval(*f.argv[0].expr, vars, dbg);
	
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


static Value f_min(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.warn(f.name, "Missing argument in function " P("min(a,...)") ".\n"));
		return {};
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
		return {};
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
		return {};
	} else if (f.argc > 3){
		HERE(dbg.warn(f.argv[3].mark, "Too many arguments (%d/3) in function " P("if(cond,true_expr,false_expr)") ".\n", f.argc));
	}
	
	assert(f.argv[0].expr != nullptr);
	Value cond = eval(*f.argv[0].expr, vars, dbg);
	
	if (cond.toBool()){
		assert(f.argv[1].expr != nullptr);
		return eval(*f.argv[1].expr, vars, dbg);
	} else {
		assert(f.argv[2].expr != nullptr);
		return eval(*f.argv[2].expr, vars, dbg);
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


static Value f_lower(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("lower(str)") ".\n"));
		return string_view();
	} else if (f.argc > 1){
		HERE(dbg.warn(f.argv[1].mark, "Too many arguments (%d/1) in function " P("lower(str)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
	}
	
	Value val = eval(*f.argv[0].expr, vars, dbg);
	
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


static Value f_upper(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 1){
		HERE(dbg.error(f.name, "Missing argument in function " P("upper(str)") ".\n"));
		return string_view();
	} else if (f.argc > 1){
		HERE(dbg.warn(f.argv[1].mark, "Too many arguments (%d/1) in function " P("upper(str)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
	}
	
	Value val = eval(*f.argv[0].expr, vars, dbg);
	
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


static Value f_substr(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 3){
		HERE(dbg.error(f.name, "Missing arguments (%d/3) in function " P("substr(str,beg,end)") ".\n", f.argc));
		return string_view();
	} else if (f.argc > 3){
		HERE(dbg.warn(f.argv[3].mark, "Too many arguments (%d/3) in function " P("substr(str,beg,end)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
		assert(f.argv[1].expr != nullptr);
		assert(f.argv[2].expr != nullptr);
	}
	
	Value arg_0 = eval(*f.argv[0].expr, vars, dbg);
	Value arg_1 = eval(*f.argv[1].expr, vars, dbg);
	Value arg_2 = eval(*f.argv[2].expr, vars, dbg);
	
	// Parse arg 1, [beg]
	long beg = arg_1.data.l;
	if (arg_1.type != Type::LONG){
		HERE(dbg.error(f.argv[1].mark, "Argument " P("beg={%s}") " in function " P("substr(str,beg,end)") " must be an integer.\n", VAL_STR(arg_1)));
		return arg_0;
	}
	
	// Parse arg 2, [end]
	long end = arg_2.data.l;
	if (arg_2.type != Type::LONG){
		HERE(dbg.error(f.argv[2].mark, "Argument " P("end={%s}") " in function " P("substr(str,beg,end)") " must be an integer.\n", VAL_STR(arg_2)));
		return arg_0;
	}
	
	switch (arg_0.type){
		case Type::LONG: {
			string s = to_string(arg_0.data.l);
			arg_0 = substr(s, beg, end);
		} break;
		
		case Type::DOUBLE: {
			string s = to_string(arg_0.data.d);
			arg_0 = substr(s, beg, end);
		} break;
			
		case Type::STRING: [[likely]] {
			string_view sub = substr(arg_0.sv(), beg, end);
			if (sub.length() != size_t(arg_0.data_len))
				arg_0 = Value(sub);
		} break;
	}
	
	return arg_0;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value f_match(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 2){
		HERE(dbg.error(f.name, "Missing arguments (%d/2) in function " P("match(str,reg)") ".\n", f.argc));
		return 0L;
	} else if (f.argc > 2){
		HERE(dbg.warn(f.argv[2].mark, "Too many arguments (%d/2) in function " P("match(str,reg)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
		assert(f.argv[1].expr != nullptr);
	}
	
	// Parse argument 0 [str]
	Value arg_str = eval(*f.argv[0].expr, vars, dbg);
	if (arg_str.type != Type::STRING){
		HERE(dbg.error(f.argv[0].mark, "Argument " P("str={%s}") " in function " P("match(str,reg)") " must be a string.\n", VAL_STR(arg_str)));
		return 0L;
	}
	
	// Parse argument 1, [reg]
	Value arg_reg = eval(*f.argv[1].expr, vars, dbg);
	if (arg_reg.type != Type::STRING){
		HERE(dbg.error(f.argv[1].mark, "Argument " P("reg={%s}") " in function " P("match(str,reg)") " must be a regular expression string.\n", VAL_STR(arg_reg)));
		return 0L;
	}
	
	// Create regex
	regex reg;
	try {
		reg = regex(arg_reg.data.s, arg_reg.data_len);
	} catch (...){
		HERE(dbg.error(f.argv[1].mark, "Invalid regular expression " P("reg={%.*s}") " in function " P("match(str,reg)") ".\n", arg_reg.data_len, arg_reg.data.s));
		return 0L;
	}
	
	try {
		bool m = regex_match(arg_str.data.s, arg_str.data.s + arg_str.data_len, reg);
		return Value(m ? 1L : 0L);
	} catch (...){
		HERE(dbg.error(f.name, "Failed to evaluate function " P("match(str,reg)") ":\n str=" P("'%.*s'") ", reg=" P("'%.*s'") "\n", arg_str.data_len, arg_str.data.s, arg_reg.data_len, arg_reg.data.s));
		return 0L;
	}
	
}


static Value f_replace(const Function& f, const VariableMap& vars, const Debugger& dbg){
	if (f.argc < 3){
		HERE(dbg.error(f.name, "Missing arguments (%d/3) in function " P("replace(str,reg,rep)") ".\n", f.argc));
		return Value(""sv);
	} else if (f.argc > 3){
		HERE(dbg.warn(f.argv[3].mark, "Too many arguments (%d/3) in function " P("replace(str,reg,rep)") ".\n", f.argc));
	} else {
		assert(f.argv[0].expr != nullptr);
		assert(f.argv[1].expr != nullptr);
		assert(f.argv[2].expr != nullptr);
	}
	
	// Parse argument 0 [str]
	Value arg_str = eval(*f.argv[0].expr, vars, dbg);
	if (arg_str.type != Type::STRING){
		HERE(dbg.error(f.argv[0].mark, "Argument " P("str={%s}") " in function " P("replace(str,reg,rep)") " must be a string.\n", VAL_STR(arg_str)));
		return arg_str;
	}
	
	// Parse argument 1, [reg]
	Value arg_reg = eval(*f.argv[1].expr, vars, dbg);
	if (arg_reg.type != Type::STRING){
		HERE(dbg.error(f.argv[1].mark, "Argument " P("reg={%s}") " in function " P("replace(str,reg,rep)") " must be a regular expression string.\n", VAL_STR(arg_reg)));
		return arg_str;
	}
	
	// Parse argument 2, [rep]
	Value arg_rep = eval(*f.argv[2].expr, vars, dbg);
	if (arg_rep.type != Type::STRING){
		HERE(dbg.error(f.argv[2].mark, "Argument " P("rep={%s}") " in function " P("replace(str,reg,rep)") " must be a string.\n", VAL_STR(arg_rep)));
		return arg_str;
	}
	
	// Create regex
	regex reg;
	try {
		reg = regex(arg_reg.data.s, arg_reg.data_len);
	} catch (...){
		HERE(dbg.error(f.argv[1].mark, "Invalid regular expression " P("reg={%s}") " in function " P("replace(str,reg,rep)") ".\n", arg_reg.data_len, arg_reg.data.s));
		return arg_str;
	}
	
	try {
		assert(arg_str.data.s[arg_str.data_len] == 0);
		assert(arg_rep.data.s[arg_rep.data_len] == 0);
		string buff = regex_replace(arg_str.data.s, reg, arg_rep.data.s);
		arg_str = Value(move(buff));
	} catch (...){
		HERE(dbg.error(f.name,
			"Failed to evaluate function " P("replace(str,reg,rep)") ":\n str=" P("'%s'") ", reg=" P("'%s'") ", rep=" P("'%s'") "\n",
			arg_str.data_len, arg_str.data.s,
			arg_reg.data_len, arg_reg.data.s,
			arg_rep.data_len, arg_rep.data.s
		));
	}
	
	return arg_str;
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
		return {};
	}
	
	HERE(dbg.warn(name, "Undefined function " ANSI_PURPLE "'%.*s()'" ANSI_RESET " defaulted to 0.\n", int(name.length()), name.data()));
	return {};
}


// ------------------------------------------------------------------------------------------ //
