#include "Expression.hpp"
#include "ExpressionAllocator.hpp"
#include <cmath>

#include "Debug.hpp"

using namespace std;
using Operation = Expression::Operation;
using Type = Value::Type;


// ----------------------------------- [ Prototypes ] --------------------------------------- //


Value eval(const Operation& op, const VariableMap& vars, const Debugger& dbg);
Value eval(const Function& op, const VariableMap& vars, const Debugger& dbg);


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


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value eval(const Variable& var, const VariableMap& vars, const Debugger& dbg){
	const Value* val = vars.get(var.name);
	if (val != nullptr){
		return Value(*val);
	} else {
		string_view name = var.name;
		HERE(dbg.warn(name, "Undefined variable " ANSI_PURPLE "'%.*s'" ANSI_RESET " defaulted to 0.\n", int(name.length()), name.data()));
		return Value(0L);
	}
}


static Value nott(const UnaryOperation& unop, const VariableMap& vars, const Debugger& dbg){
	assert(unop.arg != nullptr);
	Value val = eval(*unop.arg, vars, dbg);
	
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


static Value neg(const UnaryOperation& unop, const VariableMap& vars, const Debugger& dbg){
	assert(unop.arg != nullptr);
	Value val = eval(*unop.arg, vars, dbg);
	
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


static Value add(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(*binop.arg_1, vars, dbg);
	Value v2 = eval(*binop.arg_2, vars, dbg);
	
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


static Value sub(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(*binop.arg_1, vars, dbg);
	Value v2 = eval(*binop.arg_2, vars, dbg);
	
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


static Value mul(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(*binop.arg_1, vars, dbg);
	Value v2 = eval(*binop.arg_2, vars, dbg);
	
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


static Value div(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(*binop.arg_1, vars, dbg);
	Value v2 = eval(*binop.arg_2, vars, dbg);
	
	if (v1.type == Type::LONG){
		if (v2.type == Type::LONG)
			v1.data.l /= v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1 = v1.data.l / v2.data.d;
	}
	
	else if (v1.type == Type::DOUBLE){
		if (v2.type == Type::LONG)
			v1.data.d -= v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1.data.d -= v2.data.d;
	}
	
	return v1;
}


static Value mod(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(*binop.arg_1, vars, dbg);
	Value v2 = eval(*binop.arg_2, vars, dbg);
	
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


static Value xxor(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(*binop.arg_1, vars, dbg);
	Value v2 = eval(*binop.arg_2, vars, dbg);
	
	if (v1.type == Type::LONG){
		if (v2.type == Type::LONG)
			v1.data.l ^= v2.data.l;
	}
	
	return v1;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template<typename OP>
static Value logical(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(*binop.arg_1, vars, dbg);
	Value v2 = eval(*binop.arg_2, vars, dbg);
	return OP{}(v1.toBool(), v2.toBool()) ? 1L : 0L;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool equals(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(*binop.arg_1, vars, dbg);
	Value v2 = eval(*binop.arg_2, vars, dbg);
	
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
static bool cmp(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(*binop.arg_1, vars, dbg);
	Value v2 = eval(*binop.arg_2, vars, dbg);
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


Value eval(const Operation& op, const VariableMap& vars, const Debugger& dbg){
	switch (op.type){
		case Operation::Type::LONG:
			return static_cast<const Long&>(op).n;
		case Operation::Type::DOUBLE:
			return static_cast<const Double&>(op).n;
		case Operation::Type::STRING:
			return static_cast<const String&>(op).s;
		case Operation::Type::VAR:
			return eval(static_cast<const Variable&>(op), vars, dbg);
		case Operation::Type::NOT:
			return nott(static_cast<const UnaryOperation&>(op), vars, dbg);
		case Operation::Type::NEG:
			return neg(static_cast<const UnaryOperation&>(op), vars, dbg);
		case Operation::Type::ADD:
			return add(static_cast<const BinaryOperation&>(op), vars, dbg);
		case Operation::Type::SUB:
			return sub(static_cast<const BinaryOperation&>(op), vars, dbg);
		case Operation::Type::MUL:
			return mul(static_cast<const BinaryOperation&>(op), vars, dbg);
		case Operation::Type::DIV:
			return div(static_cast<const BinaryOperation&>(op), vars, dbg);
		case Operation::Type::MOD:
			return mod(static_cast<const BinaryOperation&>(op), vars, dbg);
		case Operation::Type::XOR:
			return xxor(static_cast<const BinaryOperation&>(op), vars, dbg);
		case Operation::Type::AND:
			return logical<logical_and<>>(static_cast<const BinaryOperation&>(op), vars, dbg);
		case Operation::Type::OR:
			return logical<logical_or<>>(static_cast<const BinaryOperation&>(op), vars, dbg);
		case Operation::Type::EQ:
			return equals(static_cast<const BinaryOperation&>(op), vars, dbg) ? 1L : 0L;
		case Operation::Type::NEQ:
			return equals(static_cast<const BinaryOperation&>(op), vars, dbg) ? 0L : 1L;
		case Operation::Type::LT:
			return cmp<less<>>(static_cast<const BinaryOperation&>(op), vars, dbg) ? 1L : 0L;
		case Operation::Type::LTE:
			return cmp<less_equal<>>(static_cast<const BinaryOperation&>(op), vars, dbg) ? 1L : 0L;
		case Operation::Type::GT:
			return cmp<greater<>>(static_cast<const BinaryOperation&>(op), vars, dbg) ? 1L : 0L;
		case Operation::Type::GTE:
			return cmp<greater_equal<>>(static_cast<const BinaryOperation&>(op), vars, dbg) ? 1L : 0L;
		case Operation::Type::FUNC:
			return eval(static_cast<const Function&>(op), vars, dbg);
		case Operation::Type::ERROR:
			dbg.error(dbg.mark(), "Bad expression.\n");
			assert(false);
			break;
	}
	return {};
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Value Expression::eval(const VariableMap& vars, const Debugger& dbg) const noexcept {
	if (op == nullptr){
		dbg.error(dbg.mark(), "Bad expression.\n");
		assert(op != nullptr);
		return Value();
	}
	return ::eval(*op, vars, dbg);
}


// ------------------------------------------------------------------------------------------ //