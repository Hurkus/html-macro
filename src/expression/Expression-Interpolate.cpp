#include "Expression.hpp"
#include <cassert>
#include <cstring>

#include "ExpressionParser.hpp"
#include "Debug.hpp"

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


void Expression::str(Value&& val, string& buff){
	auto f = [&](const auto& val){
		if constexpr (isStr(val))
			buff = move(val);
		else
			buff.append(to_string(val));
	};
	visit(f, val);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


// bool Expression::hasInterpolation(const char* str, size_t* out_len){
// 	assert(str != nullptr);
	
// 	const char* p = str;
// 	while (*p != 0){
// 		p = strchrnul(p, '{');
// 		if (*p == 0 || p == str || p[-1] != '\\'){
// 			break;
// 		}
		
// 		// Count escapes
// 		size_t n = 0;
// 		const char* esc = p - 1;
// 		while (esc != str && *esc == '\\'){
// 			esc--;
// 			n++;
// 		}
		
// 		if (n % 2 == 0){
// 			break;
// 		}
		
// 		p++;
// 	}
	
// 	if (out_len != nullptr){
// 		*out_len = p - str;
// 	}
	
// 	return (*p == '{');
// }


bool Expression::interpolate(string_view str, const VariableMap& vars, string& buff){
	const char* beg = str.begin();
	const char* end = beg + str.length();
	
	while (beg != end){
		
		// Find starting poing {
		const char* a = beg;
		while (a != end && *a != '{') a++;
		
		// Find end point }
		const char* b = a;
		while (b != end){
			if (*b == '}'){
				break;
			} else if (*b == '\n'){
				ERROR("Newline not allowed in interpolated expression [%s].", string(beg, b).c_str());
				return false;
			}
			b++;
		}
		
		// Append prefix
		if (b == end){
			buff.append(beg, b);
			return true;
		} else {
			buff.append(beg, a);
		}
		
		// Evaluate expression
		pExpr expr = Parser().parse(string_view(a+1, b));
		if (expr != nullptr){
			// Don't use Value&& because it must be *appended* to buff.
			Value val = expr->eval(vars);
			Expression::str(val, buff);
		} else {
			ERROR("Failed to parse expression [%s].", string(a, b+1).c_str());
			return false;
		}
		
		beg = b+1;
	}
	
	return false;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


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


// ------------------------------------------------------------------------------------------ //