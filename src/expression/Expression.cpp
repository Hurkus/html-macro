#include "Expression-impl.hpp"
#include <cmath>
#include <type_traits>

#include "Debug.hpp"

using namespace std;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


#define IS_TYPE(e, t)	(std::is_same_v<std::decay_t<decltype(e)>, t>)
#define IS_STR(e)		(IS_TYPE(e, StrValue))
#define IS_NUM(e)		(IS_TYPE(e, long) || IS_TYPE(e, double))


inline Value _eval(const VariableMap& vars, const pExpr& e, const Debugger& dbg){
	if (e != nullptr){
		return e->eval(vars, dbg);
	} else {
		HERE(dbg.warn(dbg.mark(), "Internal error; faulty expression tree defaulted to 0.\n"));
		assert(e != nullptr);
		return Value(0);
	}
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Value Expression::Var::eval(const VariableMap& vars, const Debugger& dbg) noexcept {
	const Value* val = vars.get(this->name);
	if (val != nullptr){
		return *val;
	}
	
	HERE(dbg.warn(name, "Undefined variable " ANSI_PURPLE "'%.*s'" ANSI_RESET " defaulted to 0.\n", name.length(), name.data()));
	return Value(0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Value Expression::Neg::eval(const VariableMap& vars, const Debugger& dbg) noexcept {
	Value e = _eval(vars, this->e, dbg);
	
	if (const long* arg = get_if<long>(&e)){
		return Value(-(*arg));
	} else if (const double* arg = get_if<double>(&e)){
		return Value(-(*arg));
	}
	
	return e;
}


Value Expression::Not::eval(const VariableMap& vars, const Debugger& dbg) noexcept {
	Value e = _eval(vars, this->e, dbg);
	
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
inline Value _arith_op(const VariableMap& vars, const Expression::BinaryOp& ab, const Debugger& dbg){
	Value _a = _eval(vars, ab.a, dbg);
	Value _b = _eval(vars, ab.b, dbg);
	
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


Value Expression::Add::eval(const VariableMap& vars, const Debugger& dbg) noexcept {
	Value _a = _eval(vars, this->a, dbg);
	Value _b = _eval(vars, this->b, dbg);
	
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


Value Expression::Sub::eval(const VariableMap& vars, const Debugger& dbg) noexcept {
	return _arith_op<std::minus<>>(vars, *this, dbg);
}

Value Expression::Mul::eval(const VariableMap& vars, const Debugger& dbg) noexcept {
	return _arith_op<std::multiplies<>>(vars, *this, dbg);
}

Value Expression::Div::eval(const VariableMap& vars, const Debugger& dbg) noexcept {
	return _arith_op<std::divides<>>(vars, *this, dbg);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template <typename OP>
inline Value _comp_op(const VariableMap& vars, const Expression::BinaryOp& ab, const Debugger& dbg){
	Value _a = _eval(vars, ab.a, dbg);
	Value _b = _eval(vars, ab.b, dbg);
	
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


Value Expression::Eq::eval(const VariableMap& vars, const Debugger& dbg) noexcept {
	return _comp_op<std::equal_to<>>(vars, *this, dbg);
}

Value Expression::Neq::eval(const VariableMap& vars, const Debugger& dbg) noexcept {
	return _comp_op<std::not_equal_to<>>(vars, *this, dbg);
}

Value Expression::Lt::eval(const VariableMap& vars, const Debugger& dbg) noexcept {
	return _comp_op<std::less<>>(vars, *this, dbg);
}

Value Expression::Lte::eval(const VariableMap& vars, const Debugger& dbg) noexcept {
	return _comp_op<std::less_equal<>>(vars, *this, dbg);
}

Value Expression::Gt::eval(const VariableMap& vars, const Debugger& dbg) noexcept {
	return _comp_op<std::greater<>>(vars, *this, dbg);
}

Value Expression::Gte::eval(const VariableMap& vars, const Debugger& dbg) noexcept {
	return _comp_op<std::greater_equal<>>(vars, *this, dbg);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool Expression::toBool(const Value& val){
	auto f = [](const auto& val) -> bool {
		if constexpr IS_STR(val)
			return bool(val.length() != 0);
		else
			return bool(val != 0);
	};
	return visit(f, val);
}


string Expression::toStr(const Value& val){
	auto f = [&](const auto& val){
		if constexpr IS_STR(val)
			return val;
		else
			return to_string(val);
	};
	return visit(f, val);
}

string Expression::toStr(Value&& val){
	auto f = [&](const auto& val){
		if constexpr IS_STR(val)
			return move(val);
		else
			return to_string(val);
	};
	return visit(f, val);
}


void Expression::toStr(const Value& val, string& buff){
	auto f = [&](const auto& val){
		if constexpr IS_STR(val)
			buff.append(val);
		else
			buff.append(to_string(val));
	};
	visit(f, val);
}

void Expression::toStr(Value&& val, string& buff){
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