#include "Expression.hpp"
#include "ExpressionAllocator.hpp"
#include <cmath>

#include "Debug.hpp"
#include "DebugSource.hpp"

using namespace std;
using Operation = Expression::Operation;
using Type = Value::Type;


// ----------------------------------- [ Prototypes ] --------------------------------------- //


Value eval(const Expression& self, const Operation& op, const VariableMap& vars);
Value eval(const Expression& self, const Function& op, const VariableMap& vars);


// ---------------------------------- [ Constructors ] -------------------------------------- //


Expression::~Expression(){
	delete alloc;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template<typename T>
constexpr T clamp(T x, T min, T max){
	if (x < min)
		return min;
	else if (x > max)
		return max;
	else
		return x;
}


static void warn_undefined_variable(const Expression& self, const Variable& var){
	if (self.origin == nullptr){
		return;
	}
	
	string_view mark = var.view();
	linepos pos = findLine(*self.origin, mark.begin());
	
	print(pos);
	LOG_STDERR(WARN_PFX "Undefined variable `" PURPLE("%.*s") "`. Defaulted to " PURPLE("0") ".\n", VA_STRV(var.name()));
	printCodeView(pos, mark, ANSI_YELLOW);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value var(const Expression& self, const Variable& var, const VariableMap& vars){
	const Value* val = vars.get(var.name());
	if (val != nullptr){
		return Value(*val);
	} else {
		warn_undefined_variable(self, var);
		return Value(0L);
	}
}


static Value nott(const Expression& self, const UnaryOperation& unop, const VariableMap& vars){
	assert(unop.arg != nullptr);
	Value val = eval(self, *unop.arg, vars);
	
	switch (val.type){
		case Type::LONG:
			val = (val.data.l == 0) ? 1L : 0L;
			break;
		case Type::DOUBLE:
			val = (val.data.d == 0) ? 1L : 0L;
			break;
		case Type::STRING:
			val = (val.data_len == 0) ? 1L : 0L;
			break;
	}
	
	return val;
}


static Value neg(const Expression& self, const UnaryOperation& unop, const VariableMap& vars){
	assert(unop.arg != nullptr);
	Value val = eval(self, *unop.arg, vars);
	
	switch (val.type){
		case Type::LONG:
			val = -val.data.l;
			break;
		case Type::DOUBLE:
			val = -val.data.d;
			break;
		case Type::STRING:
			break;
	}
	
	return val;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value add(const Expression& self, const BinaryOperation& binop, const VariableMap& vars){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(self, *binop.arg_1, vars);
	Value v2 = eval(self, *binop.arg_2, vars);
	
	if (v1.type == Type::LONG){
		if (v2.type == Type::LONG)
			v1.data.l += v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1 = v1.data.l + v2.data.d;
		else if (v2.type == Type::STRING){
			string s1 = to_string(v1.data.l);
			v1 = Value(s1, v2.sv());
		}
	}
	
	else if (v1.type == Type::DOUBLE){
		if (v2.type == Type::LONG)
			v1.data.d += v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1.data.d += v2.data.d;
		else if (v2.type == Type::STRING){
			string s1 = to_string(v1.data.l);
			v1 = Value(s1, v2.sv());
		}
	}
	
	else if (v1.type == Type::STRING){
		string_view s1 = string_view(v1.data.s, v1.data_len);
		
		if (v2.type == Type::LONG)
			v1 = Value(s1, to_string(v2.data.l));
		else if (v2.type == Type::DOUBLE)
			v1 = Value(s1, to_string(v2.data.d));
		else if (v2.type == Type::STRING){
			v1 = Value(s1, v2.sv());
		}
		
	}
	
	return v1;
}


static Value sub(const Expression& self, const BinaryOperation& binop, const VariableMap& vars){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(self, *binop.arg_1, vars);
	Value v2 = eval(self, *binop.arg_2, vars);
	
	if (v1.type == Type::LONG){
		if (v2.type == Type::LONG)
			v1.data.l -= v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1 = v1.data.l - v2.data.d;
	}
	
	else if (v1.type == Type::DOUBLE){
		if (v2.type == Type::LONG)
			v1.data.d -= v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1.data.d -= v2.data.d;
	}
	
	else if (v1.type == Type::STRING){
		if (v2.type == Type::LONG)
			v1.data_len = (uint32_t)clamp(long(v1.data_len) - v2.data.l, 0L, long(UINT32_MAX));
		else if (v2.type == Type::DOUBLE)
			v1.data_len = (uint32_t)clamp(v1.data_len - v2.data.d, 0.0, double(UINT32_MAX));
	}
	
	return v1;
}


static Value mul(const Expression& self, const BinaryOperation& binop, const VariableMap& vars){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(self, *binop.arg_1, vars);
	Value v2 = eval(self, *binop.arg_2, vars);
	
	auto mul = [](string_view sv, long n){
		assert(n > 0);
		size_t len = sv.length() * size_t(n);
		
		char* s = new char[len + 1];
		for (long i = 0 ; i < n ; i++){
			copy(sv.begin(), sv.end(), s + i * sv.length());
		}
		
		s[len] = 0;
		return Value(s, (uint32_t)min(len, size_t(UINT32_MAX)));
	};
	
	if (v1.type == Type::LONG){
		if (v2.type == Type::LONG)
			v1.data.l *= v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1 = v1.data.l * v2.data.d;
		else if (v2.type == Type::STRING){
			if (v1.data.l <= 0)
				v1 = Value(""sv);
			else if (v1.data.l == 1)
				v1 = move(v2);
			else
				v1 = mul(v2.sv(), v1.data.l);
			return v1;
		}
	}
	
	else if (v1.type == Type::DOUBLE){
		if (v2.type == Type::LONG)
			v1.data.d *= v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1.data.d *= v2.data.d;
		else if (v2.type == Type::STRING){
			if (v1.data.l <= 0)
				v1 = Value(""sv);
			else if (v1.data.l == 1)
				v1 = move(v2);
			else
				v1 = mul(v2.sv(), long(v1.data.d));
			return v1;
		}
	}
	
	else if (v1.type == Type::STRING){
		if (v2.type == Type::STRING){
			return v1;
		}
		
		long n = 0;
		if (v2.type == Type::LONG)
			n = v2.data.l;
		else if (v2.type == Type::DOUBLE)
			n = long(v2.data.d);
		
		v1 = mul(v1.sv(), n);
		return v1;
	}
	
	return v1;
}


static Value div(const Expression& self, const BinaryOperation& binop, const VariableMap& vars){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(self, *binop.arg_1, vars);
	Value v2 = eval(self, *binop.arg_2, vars);
	
	if (v1.type == Type::LONG){
		if (v2.type == Type::LONG)
			v1.data.l /= v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1 = v1.data.l / v2.data.d;
	}
	
	else if (v1.type == Type::DOUBLE){
		if (v2.type == Type::LONG)
			v1.data.d /= v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1.data.d /= v2.data.d;
	}
	
	return v1;
}


static Value mod(const Expression& self, const BinaryOperation& binop, const VariableMap& vars){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(self, *binop.arg_1, vars);
	Value v2 = eval(self, *binop.arg_2, vars);
	
	if (v1.type == Type::LONG){
		if (v2.type == Type::LONG)
			v1.data.l %= v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1 = fmod(v1.data.l, v2.data.d);
	}
	
	else if (v1.type == Type::DOUBLE){
		if (v2.type == Type::LONG)
			v1 = fmod(v1.data.d, v2.data.l);
		else if (v2.type == Type::DOUBLE)
			v1 = fmod(v1.data.d, v2.data.d);
	}
	
	return v1;
}


static Value xxor(const Expression& self, const BinaryOperation& binop, const VariableMap& vars){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(self, *binop.arg_1, vars);
	Value v2 = eval(self, *binop.arg_2, vars);
	
	if (v1.type == Type::LONG){
		if (v2.type == Type::LONG)
			v1.data.l ^= v2.data.l;
	}
	
	return v1;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template<typename OP>
static Value logical(const Expression& self, const BinaryOperation& binop, const VariableMap& vars){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(self, *binop.arg_1, vars);
	Value v2 = eval(self, *binop.arg_2, vars);
	return OP{}(v1.toBool(), v2.toBool()) ? 1L : 0L;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool equals(const Expression& self, const BinaryOperation& binop, const VariableMap& vars){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(self, *binop.arg_1, vars);
	Value v2 = eval(self, *binop.arg_2, vars);
	
	if (v1.type == Type::LONG){
		if (v2.type == Type::LONG)
			return v1.data.l == v2.data.l;
		else if (v2.type == Type::DOUBLE)
			return v1.data.l == v2.data.d;
		else if (v2.type == Type::STRING)
			return to_string(v1.data.l) == v2.sv();
	}
	
	else if (v1.type == Type::DOUBLE){
		if (v2.type == Type::LONG)
			return v1.data.d == v2.data.l;
		else if (v2.type == Type::DOUBLE)
			return v1.data.d == v2.data.d;
		else if (v2.type == Type::STRING)
			return to_string(v1.data.d) == v2.sv();
	}
	
	else if (v1.type == Type::STRING){
		if (v2.type == Type::LONG)
			return v1.sv() == to_string(v2.data.l);
		else if (v2.type == Type::DOUBLE)
			return v1.sv() == to_string(v2.data.d);
		else if (v2.type == Type::STRING)
			return v1.sv() == v2.sv();
	}
	
	return false;
}


template<typename OP>
static bool cmp(const Expression& self, const BinaryOperation& binop, const VariableMap& vars){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(self, *binop.arg_1, vars);
	Value v2 = eval(self, *binop.arg_2, vars);
	OP op = {};
	
	if (v1.type == Type::LONG){
		if (v2.type == Type::LONG)
			return op(v1.data.l, v2.data.l);
		else if (v2.type == Type::DOUBLE)
			return op(v1.data.l, v2.data.d);
		else if (v2.type == Type::STRING)
			return op(v1.data.l, v2.data_len);
	}
	
	else if (v1.type == Type::DOUBLE){
		if (v2.type == Type::LONG)
			return op(v1.data.d, v2.data.l);
		else if (v2.type == Type::DOUBLE)
			return op(v1.data.d, v2.data.d);
		else if (v2.type == Type::STRING)
			return op(v1.data.d, v2.data_len);
	}
	
	else if (v1.type == Type::STRING){
		if (v2.type == Type::LONG)
			return op(v1.data_len, v2.data.l);
		else if (v2.type == Type::DOUBLE)
			return op(v1.data_len, v2.data.d);
		else if (v2.type == Type::STRING)
			return op(v1.data_len, v2.data_len);
	}
	
	return false;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Value eval(const Expression& self, const Operation& op, const VariableMap& vars){
	switch (op.type){
		case Operation::Type::LONG:
			return static_cast<const Long&>(op).n;
		case Operation::Type::DOUBLE:
			return static_cast<const Double&>(op).n;
		case Operation::Type::STRING:
			return static_cast<const String&>(op).str();
		case Operation::Type::VAR:
			return var(self, static_cast<const Variable&>(op), vars);
		case Operation::Type::NOT:
			return nott(self, static_cast<const UnaryOperation&>(op), vars);
		case Operation::Type::NEG:
			return neg(self, static_cast<const UnaryOperation&>(op), vars);
		case Operation::Type::ADD:
			return add(self, static_cast<const BinaryOperation&>(op), vars);
		case Operation::Type::SUB:
			return sub(self, static_cast<const BinaryOperation&>(op), vars);
		case Operation::Type::MUL:
			return mul(self, static_cast<const BinaryOperation&>(op), vars);
		case Operation::Type::DIV:
			return div(self, static_cast<const BinaryOperation&>(op), vars);
		case Operation::Type::MOD:
			return mod(self, static_cast<const BinaryOperation&>(op), vars);
		case Operation::Type::XOR:
			return xxor(self, static_cast<const BinaryOperation&>(op), vars);
		case Operation::Type::AND:
			return logical<logical_and<>>(self, static_cast<const BinaryOperation&>(op), vars);
		case Operation::Type::OR:
			return logical<logical_or<>>(self, static_cast<const BinaryOperation&>(op), vars);
		case Operation::Type::EQ:
			return equals(self, static_cast<const BinaryOperation&>(op), vars) ? 1L : 0L;
		case Operation::Type::NEQ:
			return equals(self, static_cast<const BinaryOperation&>(op), vars) ? 0L : 1L;
		case Operation::Type::LT:
			return cmp<less<>>(self, static_cast<const BinaryOperation&>(op), vars) ? 1L : 0L;
		case Operation::Type::LTE:
			return cmp<less_equal<>>(self, static_cast<const BinaryOperation&>(op), vars) ? 1L : 0L;
		case Operation::Type::GT:
			return cmp<greater<>>(self, static_cast<const BinaryOperation&>(op), vars) ? 1L : 0L;
		case Operation::Type::GTE:
			return cmp<greater_equal<>>(self, static_cast<const BinaryOperation&>(op), vars) ? 1L : 0L;
		case Operation::Type::FUNC:
			return eval(self, static_cast<const Function&>(op), vars);
		case Operation::Type::ERROR:
			assert(false);
			break;
	}
	return {};
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Value Expression::eval(const VariableMap& vars) const noexcept {
	if (op == nullptr){
		assert(op != nullptr);
		return Value();
	}
	return ::eval(*this, *op, vars);
}


// ------------------------------------------------------------------------------------------ //