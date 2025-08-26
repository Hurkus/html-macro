#include "Expression.hpp"
#include <cmath>

#include "Debug.hpp"

using namespace std;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline Value _eval(const VariableMap& vars, const pExpr& e){
	if (e == nullptr)
		return Value(0);
	else
		return e->eval(vars);
}


template <typename T>
constexpr bool isNum(const T& e){
	return is_same_v<T,long> || is_same_v<T,double>;
}

template <typename T>
constexpr bool isStr(const T& e){
	return is_same_v<T,string>;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Value Expr::Var::eval(const VariableMap& vars) noexcept {
	try {
		auto p = vars.find(this->var);
		if (p != vars.end())
			return p->second;
	} catch (const exception& e){}
	
	WARN("Undefined varialbe '%s' defaulted to 0.", this->var.c_str());
	return Value(0);
}


Value Expr::Neg::eval(const VariableMap& vars) noexcept {
	Value e = _eval(vars, this->e);
	
	if (const long* arg = get_if<long>(&e)){
		return Value(-(*arg));
	} else if (const double* arg = get_if<double>(&e)){
		return Value(-(*arg));
	}
	
	return e;
}


Value Expr::Not::eval(const VariableMap& vars) noexcept {
	Value e = _eval(vars, this->e);
	
	if (const long* arg = get_if<long>(&e)){
		return Value(*arg == 0 ? 1l : 0l);
	} else if (const double* arg = get_if<double>(&e)){
		return Value(*arg == 0 ? 1l : 0l);
	} else if (const string* arg = get_if<string>(&e)){
		return Value(arg->empty() ? 0l : 1l);
	}
	
	return e;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template <typename OP>
inline Value _arith_op(const VariableMap& vars, const Expr::BinaryOp& ab){
	Value _a = _eval(vars, ab.a);
	Value _b = _eval(vars, ab.b);
	
	auto f = [&](const auto& a, const auto& b) -> Value {
		if constexpr (isNum(a) && isNum(b))
			return OP{}(a, b);
		else if constexpr (isStr(a))
			return move(_a);
		else
			return move(_b);
	};
	
	return visit(f, _a, _b);
}


Value Expr::Add::eval(const VariableMap& vars) noexcept {
	Value _a = _eval(vars, this->a);
	Value _b = _eval(vars, this->b);
	
	auto op = [](auto&& a, auto&& b) -> Value {
		if constexpr (isNum(a) && isStr(b))
			return to_string(a) + move(b);
		else if constexpr (isStr(a) && isNum(b))
			return move(a) + to_string(b);
		else
			return move(a) + move(b);
	};
	
	return visit(op, move(_a), move(_b));
}


Value Expr::Sub::eval(const VariableMap& vars) noexcept {
	return _arith_op<std::minus<>>(vars, *this);
}

Value Expr::Mul::eval(const VariableMap& vars) noexcept {
	return _arith_op<std::multiplies<>>(vars, *this);
}

Value Expr::Div::eval(const VariableMap& vars) noexcept {
	return _arith_op<std::divides<>>(vars, *this);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template <typename OP>
inline Value _comp_op(const VariableMap& vars, const Expr::BinaryOp& ab){
	Value _a = _eval(vars, ab.a);
	Value _b = _eval(vars, ab.b);
	
	auto f = [](auto&& a, auto&& b) -> Value {
		if constexpr (isStr(a) && isNum(b))
			return OP{}(a.length(), b);
		else if constexpr (isNum(a) && isStr(b))
			return OP{}(a, b.length());
		else
			return OP{}(move(a), move(b));
	};
	
	return visit(f, move(_a), move(_b));
}


Value Expr::Eq::eval(const VariableMap& vars) noexcept {
	return _comp_op<std::equal_to<>>(vars, *this);
}

Value Expr::Neq::eval(const VariableMap& vars) noexcept {
	return _comp_op<std::not_equal_to<>>(vars, *this);
}

Value Expr::Lt::eval(const VariableMap& vars) noexcept {
	return _comp_op<std::less<>>(vars, *this);
}

Value Expr::Lte::eval(const VariableMap& vars) noexcept {
	return _comp_op<std::less_equal<>>(vars, *this);
}

Value Expr::Gt::eval(const VariableMap& vars) noexcept {
	return _comp_op<std::greater<>>(vars, *this);
}

Value Expr::Gte::eval(const VariableMap& vars) noexcept {
	return _comp_op<std::greater_equal<>>(vars, *this);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value _f_int(const VariableMap& vars, const vector<pExpr>& args) noexcept {
	if (args.size() < 1){
		return Value(0);
	}
	
	auto cast = [](const auto& v) -> long {
		if constexpr (isStr(v))
			return atol(v.c_str());
		else
			return long(v);
	};
	
	return visit(cast, _eval(vars, args[0]));
}


static Value _f_float(const VariableMap& vars, const vector<pExpr>& args){
	if (args.size() < 1){
		return Value(0.0);
	}
	
	auto cast = [](const auto& v) -> double {
		if constexpr (isStr(v))
			return atof(v.c_str());
		else
			return double(v);
	};
	
	return visit(cast, _eval(vars, args[0]));
}


static Value _f_str(const VariableMap& vars, const vector<pExpr>& args){
	if (args.size() < 1){
		return Value(in_place_type<string>);
	}
	
	auto cast = [&](auto&& v) -> string {
		if constexpr (isStr(v))
			return move(v);
		else
			return to_string(v);
	};
	
	return visit(cast, _eval(vars, args[0]));
}


static Value _f_len(const VariableMap& vars, const vector<pExpr>& args){
	if (args.size() < 1){
		return Value(0);
	}
	
	auto cast = [](auto&& v) -> Value {
		if constexpr (isStr(v))
			return long(v.length());
		else
			return abs(v);
	};
	
	return visit(cast, _eval(vars, args[0]));
}


Value Expr::Func::eval(const VariableMap& vars) noexcept {
	if (name == "int")
		return _f_int(vars, args);
	else if (name == "float")
		return _f_float(vars, args);
	else if (name == "str")
		return _f_str(vars, args);
	else if (name == "len" || name == "abs")
		return _f_len(vars, args);
	return 0;
}


// ------------------------------------------------------------------------------------------ //