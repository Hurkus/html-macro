#include "Expression-impl.hpp"
#include <cmath>
#include <type_traits>

#include "Debug.hpp"

using namespace std;
using namespace Expression;


// ---------------------------------- [ Definitions ] --------------------------------------- //


#define IS_TYPE(e, t)	(std::is_same_v<std::decay_t<decltype(e)>, t>)
#define IS_STR(e)		(IS_TYPE(e, StrValue))
#define IS_NUM(e)		(IS_TYPE(e, long) || IS_TYPE(e, double))


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline Value _eval(const VariableMap& vars, const pExpr& e){
	if (e == nullptr)
		return Value(0);
	else
		return e->eval(vars);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Value Expression::Var::eval(const VariableMap& vars) noexcept {
	const Value* val = vars.get(this->var);
	if (val != nullptr){
		return *val;
	}
	
	WARN("Undefined varialbe '%s' defaulted to 0.", this->var.c_str());
	return Value(0);
}


Value Expression::Neg::eval(const VariableMap& vars) noexcept {
	Value e = _eval(vars, this->e);
	
	if (const long* arg = get_if<long>(&e)){
		return Value(-(*arg));
	} else if (const double* arg = get_if<double>(&e)){
		return Value(-(*arg));
	}
	
	return e;
}


Value Expression::Not::eval(const VariableMap& vars) noexcept {
	Value e = _eval(vars, this->e);
	
	if (const long* arg = get_if<long>(&e)){
		return Value(*arg == 0 ? 1l : 0l);
	} else if (const double* arg = get_if<double>(&e)){
		return Value(*arg == 0 ? 1l : 0l);
	} else if (const string* arg = get_if<string>(&e)){
		return Value(arg->empty() ? 1l : 0l);
	}
	
	return e;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template <typename OP>
inline Value _arith_op(const VariableMap& vars, const Expression::BinaryOp& ab){
	Value _a = _eval(vars, ab.a);
	Value _b = _eval(vars, ab.b);
	
	auto f = [&](const auto& a, const auto& b) -> Value {
		if constexpr (IS_NUM(a) && IS_NUM(b))
			return OP{}(a, b);
		else if constexpr (IS_STR(a))
			return move(_a);
		else
			return move(_b);
	};
	
	return visit(f, _a, _b);
}


Value Expression::Add::eval(const VariableMap& vars) noexcept {
	Value _a = _eval(vars, this->a);
	Value _b = _eval(vars, this->b);
	
	auto op = [](auto&& a, auto&& b) -> Value {
		if constexpr (IS_NUM(a) && IS_STR(b))
			return to_string(a) + move(b);
		else if constexpr (IS_STR(a) && IS_NUM(b))
			return move(a) + to_string(b);
		else
			return move(a) + move(b);
	};
	
	return visit(op, move(_a), move(_b));
}


Value Expression::Sub::eval(const VariableMap& vars) noexcept {
	return _arith_op<std::minus<>>(vars, *this);
}

Value Expression::Mul::eval(const VariableMap& vars) noexcept {
	return _arith_op<std::multiplies<>>(vars, *this);
}

Value Expression::Div::eval(const VariableMap& vars) noexcept {
	return _arith_op<std::divides<>>(vars, *this);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template <typename OP>
inline Value _comp_op(const VariableMap& vars, const Expression::BinaryOp& ab){
	Value _a = _eval(vars, ab.a);
	Value _b = _eval(vars, ab.b);
	
	auto f = [](auto&& a, auto&& b) -> Value {
		if constexpr (IS_STR(a) && IS_NUM(b))
			return OP{}(a.length(), b);
		else if constexpr (IS_NUM(a) && IS_STR(b))
			return OP{}(a, b.length());
		else
			return OP{}(move(a), move(b));
	};
	
	return visit(f, move(_a), move(_b));
}


Value Expression::Eq::eval(const VariableMap& vars) noexcept {
	return _comp_op<std::equal_to<>>(vars, *this);
}

Value Expression::Neq::eval(const VariableMap& vars) noexcept {
	return _comp_op<std::not_equal_to<>>(vars, *this);
}

Value Expression::Lt::eval(const VariableMap& vars) noexcept {
	return _comp_op<std::less<>>(vars, *this);
}

Value Expression::Lte::eval(const VariableMap& vars) noexcept {
	return _comp_op<std::less_equal<>>(vars, *this);
}

Value Expression::Gt::eval(const VariableMap& vars) noexcept {
	return _comp_op<std::greater<>>(vars, *this);
}

Value Expression::Gte::eval(const VariableMap& vars) noexcept {
	return _comp_op<std::greater_equal<>>(vars, *this);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value _f_int(const VariableMap& vars, const vector<pExpr>& args) noexcept {
	if (args.size() < 1){
		return Value(0);
	}
	
	auto cast = [](const auto& v) -> long {
		if constexpr (IS_STR(v))
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
		if constexpr (IS_STR(v))
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
		if constexpr (IS_STR(v))
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
		if constexpr (IS_STR(v))
			return long(v.length());
		else
			return abs(v);
	};
	
	return visit(cast, _eval(vars, args[0]));
}


Value Expression::Func::eval(const VariableMap& vars) noexcept {
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


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool Expression::boolEval(const Value& val){
	auto f = [](const auto& val) -> bool {
		if constexpr IS_STR(val)
			return bool(val.length() != 0);
		else
			return bool(val != 0);
	};
	return visit(f, val);
}


void Expression::str(const Value& val, string& buff){
	auto f = [&](const auto& val){
		if constexpr IS_STR(val)
			buff.append(val);
		else
			buff.append(to_string(val));
	};
	visit(f, val);
}


void Expression::str(Value&& val, string& buff){
	auto f = [&](const auto& val){
		if constexpr IS_STR(val){
			if (buff.empty())
				buff = move(val);
			else
				buff.append(val);
		} else {
			buff.append(to_string(val));
		}
	};
	visit(f, val);
}


// ------------------------------------------------------------------------------------------ //