#include "Expression.hpp"
#include <cstring>

#include "ExpressionParser.hpp"
#include "DEBUG.hpp"

using namespace std;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


template <typename T>
constexpr bool isStr(const T& e){
	return is_same_v<T,string>;
}


bool Expression::boolEval(const Value& val){
	auto f = [](const auto& val) -> bool {
		if constexpr (isStr(val))
			return bool(val.length() != 0);
		else
			return bool(val != 0);
	};
	return visit(f, val);
}


void Expression::str(const Value& val, string& buff){
	auto f = [&](const auto& val){
		if constexpr (isStr(val))
			buff.append(val);
		else
			buff.append(to_string(val));
	};
	visit(f, val);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool Expression::hasInterpolation(const char* str, size_t* out_len){
	const char* p = strchrnul(str, '{');
	if (out_len != nullptr)
		*out_len = p - str;
	return (*p == '{');
}


void Expression::interpolate(const char* str, const VariableMap& vars, string& buff){
	assert(str != nullptr);
	
	const char* beg = strchrnul(str, '{');
	if (*beg == 0){
		buff.append(str, beg);
		return;
	}
	
	const char* end = strchrnul(beg + 1, '}');
	if (*end == 0){
		buff.append(str, end);
		return;
	}
	
	Parser parser = {};
	
	while (*beg != 0 && *end != 0){
		buff.append(str, beg);
		
		pExpr expr = parser.parse(string_view(beg + 1, end));
		if (expr != nullptr){
			Value val = expr->eval(vars);
			Expression::str(val, buff);
		} else {
			WARNING_L1("TEXT: Failed to parse expression [%s].", string(beg, end+1).c_str());
		}
		
		// Next expression
		str = end + 1;
		beg = strchrnul(str, '{');
		end = strchrnul(beg, '}');
	}
	
	buff.append(str, end);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //
#ifdef DEBUG


// static void _str(const Value& value, string& s){
// 	auto f = [&](const auto& v){
// 		if constexpr (isStr(v))
// 			s += v;
// 		else
// 			s += to_string(v);
// 	};
// 	visit(f, value);
// }


// static void _str(const Expr* expr, string& s){
// 	auto __binstr = [&](const Expr::BinaryOp* b, const char* op){
// 		s.push_back('(');
// 		_str(b->a.get(), s);
// 		s.append(op);
// 		_str(b->b.get(), s);
// 		s.push_back(')');
// 	};
	
// 	if (const Expr::Const* e = dynamic_cast<const Expr::Const*>(expr)){
// 		_str(e->value, s);
// 	} else if (const Expr::Var* e = dynamic_cast<const Expr::Var*>(expr)){
// 		s.append(e->var);
// 	} else if (const Expr::Not* e = dynamic_cast<const Expr::Not*>(expr)){
// 		s.append("!(");
// 		_str(e->e.get(), s);
// 		s.push_back(')');
// 	} else if (const Expr::Neg* e = dynamic_cast<const Expr::Neg*>(expr)){
// 		s.append("-(");
// 		_str(e->e.get(), s);
// 		s.push_back(')');
// 	} else if (const Expr::Add* e = dynamic_cast<const Expr::Add*>(expr)){
// 		__binstr(e, "+");
// 	} else if (const Expr::Sub* e = dynamic_cast<const Expr::Sub*>(expr)){
// 		__binstr(e, "-");
// 	} else if (const Expr::Mul* e = dynamic_cast<const Expr::Mul*>(expr)){
// 		__binstr(e, "*");
// 	} else if (const Expr::Div* e = dynamic_cast<const Expr::Div*>(expr)){
// 		__binstr(e, "/");
// 	} else if (const Expr::Eq* e = dynamic_cast<const Expr::Eq*>(expr)){
// 		__binstr(e, "==");
// 	} else if (const Expr::Neq* e = dynamic_cast<const Expr::Neq*>(expr)){
// 		__binstr(e, "!=");
// 	} else if (const Expr::Lt* e = dynamic_cast<const Expr::Lt*>(expr)){
// 		__binstr(e, "<");
// 	} else if (const Expr::Lte* e = dynamic_cast<const Expr::Lte*>(expr)){
// 		__binstr(e, "<=");
// 	} else if (const Expr::Gt* e = dynamic_cast<const Expr::Gt*>(expr)){
// 		__binstr(e, ">");
// 	} else if (const Expr::Gte* e = dynamic_cast<const Expr::Gte*>(expr)){
// 		__binstr(e, ">=");
// 	} else if (const Expr::Func* e = dynamic_cast<const Expr::Func*>(expr)){
// 		s.append(e->name).push_back('(');
// 		for (size_t i = 0 ; i < e->args.size() ; i++){
// 			if (i > 0)
// 				s.push_back(',');
// 			_str(e->args[i].get(), s);
// 		}
// 		s.push_back(')');
		
// 	} else {
// 		s.append("null");
// 	}
// }


// string Expression::str(const Expr* expr){
// 	string s = {};
// 	_str(expr, s);
// 	return s;
// }


#endif
// ------------------------------------------------------------------------------------------ //