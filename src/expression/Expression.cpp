#include "Expression.hpp"
#include <cmath>

#include "DEBUG.hpp"

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


bool Expression::boolEval(const Value& val){
	auto f = [](const auto& val) -> bool {
		if constexpr (isStr(val))
			return bool(val.length() != 0);
		else
			return bool(val != 0);
	};
	return visit(f, val);
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
	
	auto cast = [](const auto& v) -> Value {
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
	
	auto cast = [](const auto& v) -> Value {
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
	
	auto cast = [&](auto&& v) -> Value {
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


// ----------------------------------- [ Functions ] ---------------------------------------- //
#ifdef DEBUG


static void _str(const Value& value, string& s){
	auto f = [&](const auto& v){
		if constexpr (isStr(v))
			s += v;
		else
			s += to_string(v);
	};
	visit(f, value);
}


static void _str(const Expr* expr, string& s){
	auto __binstr = [&](const Expr::BinaryOp* b, const char* op){
		s.push_back('(');
		_str(b->a.get(), s);
		s.append(op);
		_str(b->b.get(), s);
		s.push_back(')');
	};
	
	if (const Expr::Const* e = dynamic_cast<const Expr::Const*>(expr)){
		_str(e->value, s);
	} else if (const Expr::Var* e = dynamic_cast<const Expr::Var*>(expr)){
		s.append(e->var);
	} else if (const Expr::Not* e = dynamic_cast<const Expr::Not*>(expr)){
		s.append("!(");
		_str(e->e.get(), s);
		s.push_back(')');
	} else if (const Expr::Neg* e = dynamic_cast<const Expr::Neg*>(expr)){
		s.append("-(");
		_str(e->e.get(), s);
		s.push_back(')');
	} else if (const Expr::Add* e = dynamic_cast<const Expr::Add*>(expr)){
		__binstr(e, "+");
	} else if (const Expr::Sub* e = dynamic_cast<const Expr::Sub*>(expr)){
		__binstr(e, "-");
	} else if (const Expr::Mul* e = dynamic_cast<const Expr::Mul*>(expr)){
		__binstr(e, "*");
	} else if (const Expr::Div* e = dynamic_cast<const Expr::Div*>(expr)){
		__binstr(e, "/");
	} else if (const Expr::Eq* e = dynamic_cast<const Expr::Eq*>(expr)){
		__binstr(e, "==");
	} else if (const Expr::Neq* e = dynamic_cast<const Expr::Neq*>(expr)){
		__binstr(e, "!=");
	} else if (const Expr::Lt* e = dynamic_cast<const Expr::Lt*>(expr)){
		__binstr(e, "<");
	} else if (const Expr::Lte* e = dynamic_cast<const Expr::Lte*>(expr)){
		__binstr(e, "<=");
	} else if (const Expr::Gt* e = dynamic_cast<const Expr::Gt*>(expr)){
		__binstr(e, ">");
	} else if (const Expr::Gte* e = dynamic_cast<const Expr::Gte*>(expr)){
		__binstr(e, ">=");
	} else if (const Expr::Func* e = dynamic_cast<const Expr::Func*>(expr)){
		s.append(e->name).push_back('(');
		for (size_t i = 0 ; i < e->args.size() ; i++){
			if (i > 0)
				s.push_back(',');
			_str(e->args[i].get(), s);
		}
		s.push_back(')');
		
	} else {
		s.append("null");
	}
}


string Expression::str(const Expr* expr){
	string s = {};
	_str(expr, s);
	return s;
}


#endif
// ------------------------------------------------------------------------------------------ //