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
	LOG_STDERR(WARN_PFX "Undefined variable `" PURPLE("%.*s") "`.\n", VA_STRV(var.name()));
	printCodeView(pos, mark, ANSI_YELLOW);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value var(const Expression& self, const Variable& var, const VariableMap& vars){
	const Value* val = vars.get(var.name());
	if (val != nullptr){
		return Value(*val);
	} else {
		warn_undefined_variable(self, var);
		return Value();
	}
}


static Value nott(const Expression& self, const UnaryOperation& unop, const VariableMap& vars){
	assert(unop.arg != nullptr);
	Value val = eval(self, *unop.arg, vars);
	
	switch (val.type){
		case Type::NONE:
			break;
		case Type::LONG:
			val = (val.data.l == 0) ? 1L : 0L;
			break;
		case Type::DOUBLE:
			val = (val.data.d == 0) ? 1L : 0L;
			break;
		case Type::STRING:
			val = (val.data.s->len == 0) ? 1L : 0L;
			break;
		case Type::OBJECT:
			val = (val.data.o->arr.empty() && val.data.o->dict.empty()) ? 1L : 0L;
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
		case Type::NONE:
		case Type::STRING:
		case Type::OBJECT:
			break;
	}
	
	return val;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value add(const Expression& self, const BinaryOperation& binop, const VariableMap& vars){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(self, *binop.arg_1, vars);
	Value v2 = eval(self, *binop.arg_2, vars);
	
	if (v1.type == Type::NONE) [[unlikely]] {
		v1 = move(v2);
	}
	
	else if (v1.type == Type::LONG){
		if (v2.type == Type::LONG)
			v1.data.l += v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1 = v1.data.l + v2.data.d;
		else if (v2.type == Type::STRING)
			v1 = Value(to_string(v1.data.l), v2.data.s->sv());
		else if (v2.type == Type::OBJECT){
			v2.data.o->insert(0, move(v1));
			v1 = move(v2);
		}
	}
	
	else if (v1.type == Type::DOUBLE){
		if (v2.type == Type::LONG)
			v1.data.d += v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1.data.d += v2.data.d;
		else if (v2.type == Type::STRING)
			v1 = Value(to_string(v1.data.l), v2.data.s->sv());
		else if (v2.type == Type::OBJECT){
			v2.data.o->insert(0, move(v1));
			v1 = move(v2);
		}
	}
	
	else if (v1.type == Type::STRING){
		if (v2.type == Type::STRING){
			v1 = Value(v1.data.s->sv(), v2.toStr());
		} else if (v2.type == Type::OBJECT){
			v2.data.o->insert(0, move(v1));
			v1 = move(v2);
		} else if (v2.type != Type::NONE){
			v1 = Value(v1.data.s->sv(), v2.toStr());
		}
	}
	
	else if (v1.type == Type::OBJECT){
		if (v2.type != Type::NONE)
			v1.data.o->insert(v1.data.o->arr.size(), move(v2));
	}
	
	return v1;
}


static Value sub(const Expression& self, const BinaryOperation& binop, const VariableMap& vars){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(self, *binop.arg_1, vars);
	Value v2 = eval(self, *binop.arg_2, vars);
	
	if (v1.type == Type::NONE) [[unlikely]] {
		v1 = move(v2);
	}
	
	else if (v1.type == Type::LONG){
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
		Value::String& s = *v1.data.s;
		
		// Downcast double to long.
		if (v2.type == Type::DOUBLE){
			v2.type = Type::LONG;
			v2.data.l = long(v2.data.d);
		}
		
		// Shorten string.
		if (v2.type == Type::LONG){
			const size_t len = (v2.data.l < s.len) ? s.len - v2.data.l : 0;
			
			if (v1.data.s->refs <= 1){
				v1.data.s->len = uint32_t(len);
			} else {
				v1 = Value(string_view(s.str, len));
			}
			
		}
		
	}
	
	else if (v1.type == Type::OBJECT){
		vector<Value>& arr = v1.data.o->arr;
		
		// Remove identical elements.
		for (ssize_t i = arr.size() - 1 ; i >= 0 ; i--){
			if (arr[i] == v2)
				arr.erase(arr.begin() + i);
		}
		
	}
	
	return v1;
}


static Value mul(const Expression& self, const BinaryOperation& binop, const VariableMap& vars){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(self, *binop.arg_1, vars);
	Value v2 = eval(self, *binop.arg_2, vars);
	
	auto mul = [](string_view sv, long n){
		assert(n > 0);
		
		string buff;
		buff.reserve(sv.length() * n);
		
		for (long i = 0 ; i < n ; i++){
			buff.append(sv);
		}
		
		return Value(buff);
	};
	
	if (v1.type == Type::NONE){
		v1 = move(v2);
	}
	
	else if (v1.type == Type::LONG){
		if (v2.type == Type::NONE)
			v1 = Value();
		else if (v2.type == Type::LONG)
			v1.data.l *= v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1 = v1.data.l * v2.data.d;
		else if (v2.type == Type::STRING){
			if (v1.data.l <= 0)
				v1 = Value(""sv);
			else if (v1.data.l == 1)
				v1 = move(v2);
			else
				v1 = mul(v2.data.s->sv(), v1.data.l);
			return v1;
		}
		else if (v2.type == Type::OBJECT){
			v2.data.o->insert(0, move(v1));
			v1 = move(v2);
		}
	}
	
	else if (v1.type == Type::DOUBLE){
		if (v2.type == Type::NONE)
			v1 = Value();
		else if (v2.type == Type::LONG)
			v1.data.d *= v2.data.l;
		else if (v2.type == Type::DOUBLE)
			v1.data.d *= v2.data.d;
		else if (v2.type == Type::STRING){
			if (v1.data.d <= 0)
				v1 = Value(""sv);
			else if (v1.data.d == 1)
				v1 = move(v2);
			else
				v1 = mul(v2.data.s->sv(), long(v1.data.d));
			return v1;
		}
		else if (v2.type == Type::OBJECT){
			v2.data.o->insert(0, move(v1));
			v1 = move(v2);
		}
	}
	
	else if (v1.type == Type::STRING){
		if (v2.type == Type::NONE)
			v1 = Value();
		else if (v2.type == Type::LONG)
			v1 = mul(v1.data.s->sv(), v2.data.l);
		else if (v2.type == Type::DOUBLE)
			v1 = mul(v1.data.s->sv(), long(v2.data.d));
		else if (v2.type == Type::OBJECT){
			v2.data.o->insert(0, move(v1));
			v1 = move(v2);
		}
	}
	
	else if (v1.type == Type::OBJECT){
		switch (v2.type){
			case Type::NONE:
				break;
			
			case Type::LONG:
			case Type::DOUBLE:
			case Type::STRING:
				v1.data.o->insert(v1.data.o->arr.size(), move(v2));
				break;
			
			case Type::OBJECT:
				v1.data.o->merge(move(*v2.data.o));
				break;
		}
	}
	
	return v1;
}


static Value div(const Expression& self, const BinaryOperation& binop, const VariableMap& vars){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(self, *binop.arg_1, vars);
	Value v2 = eval(self, *binop.arg_2, vars);
	
	if (v1.type == Type::NONE){
		v1 = move(v2);
	}
	
	else if (v1.type == Type::LONG){
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
	
	else if (v1.type == Type::OBJECT){
		if (v2.type == Type::LONG)
			v1.data.o->remove(static_cast<size_t>(v2.data.l));
		else if (v2.type == Type::DOUBLE)
			v1.data.o->remove(static_cast<size_t>(v2.data.d));
		else if (v2.type == Type::STRING)
			v1.data.o->remove(v2.data.s->sv());
		else if (v2.type == Type::OBJECT){
			
			// Remove properties indexed from o2 dictionary.
			for (const auto& pair : v2.data.o->dict){
				v1.data.o->remove(pair.key);
			}
			
			// Remove elements and properties indexed from o2 array.
			for (const Value& el : v2.data.o->arr){
				if (el.type == Type::LONG)
					v1.data.o->remove(size_t(el.data.l));
				else if (el.type == Type::DOUBLE)
					v1.data.o->remove(size_t(el.data.d));
				else if (el.type == Type::STRING)
					v1.data.o->remove(el.data.s->sv());
			}
			
		}
	}
	
	return v1;
}


static Value mod(const Expression& self, const BinaryOperation& binop, const VariableMap& vars){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value v1 = eval(self, *binop.arg_1, vars);
	Value v2 = eval(self, *binop.arg_2, vars);
	
	if (v1.type == Type::NONE){
		v1 = move(v2);
	}
	
	else if (v1.type == Type::LONG){
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
	
	else if (v1.type == Type::OBJECT){
		Value::Object& o1 = *v1.data.o;
		
		if (v2.type == Type::LONG){
			Value* el = o1.get(static_cast<size_t>(v2.data.l));
			if (el != nullptr)
				v1 = move(*el);
		}
		else if (v2.type == Type::DOUBLE){
			Value* el = o1.get(static_cast<size_t>(v2.data.l));
			if (el != nullptr)
				v1 = move(*el);
		}
		else if (v2.type == Type::STRING){
			Value* el = o1.get(v2.data.s->sv());
			if (el != nullptr)
				v1 = move(*el);
		}
		else if (v2.type == Type::OBJECT){
			Value::Object& o1 = *v1.data.o;
			Value::Object& o2 = *v2.data.o;
			unique_ptr<Value::Object> o3 = make_unique<Value::Object>();
			
			// Build array by extracting elements from o1 and indecies from o2.
			for (const Value& el : o2.arr){
				if (el.type == Type::LONG){
					Value* v = o1.get(static_cast<size_t>(el.data.l));
					if (v != nullptr)
						o3->arr.emplace_back(*v);
				}
				else if (el.type == Type::DOUBLE){
					Value* v = o1.get(static_cast<size_t>(el.data.d));
					if (v != nullptr)
						o3->arr.emplace_back(*v);
				}
				else if (el.type == Type::STRING){
					Value* v = o1.get(el.data.s->sv());
					if (v != nullptr)
						o3->arr.emplace_back(*v);
				}
			}
			
			// Build dictionary by extracting entries from o1 and keys from o2.
			for (const auto& pair : o2.dict){
				Value* v = o1.get(pair.key);
				if (v != nullptr)
					o3->dict.insert(pair.key, *v);
			}
			
			v1 = Value(o3.release());
		}
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
			return to_string(v1.data.l) == v2.data.s->sv();
		else if (v2.type == Type::OBJECT)
			return v2.data.o->arr.size() == size_t(v1.data.l);
	}
	
	else if (v1.type == Type::DOUBLE){
		if (v2.type == Type::LONG)
			return v1.data.d == v2.data.l;
		else if (v2.type == Type::DOUBLE)
			return v1.data.d == v2.data.d;
		else if (v2.type == Type::STRING)
			return to_string(v1.data.d) == v2.data.s->sv();
		else if (v2.type == Type::OBJECT)
			return v2.data.o->arr.size() == v1.data.d;
	}
	
	else if (v1.type == Type::STRING){
		if (v2.type == Type::LONG)
			return v1.data.s->sv() == to_string(v2.data.l);
		else if (v2.type == Type::DOUBLE)
			return v1.data.s->sv() == to_string(v2.data.d);
		else if (v2.type == Type::STRING)
			return v1.data.s->sv() == v2.data.s->sv();
	}
	
	else if (v1.type == Type::OBJECT){
		if (v2.type == Type::LONG)
			return v1.data.o->arr.size() == size_t(v2.data.l);
		else if (v2.type == Type::DOUBLE)
			return v1.data.o->arr.size() == v2.data.d;
		else if (v2.type == Type::OBJECT)
			return *v1.data.o == *v2.data.o;
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
			return op(v1.data.l, v2.data.s->len);
		else if (v2.type == Type::OBJECT)
			return op(v1.data.l, v2.data.o->arr.size());
	}
	
	else if (v1.type == Type::DOUBLE){
		if (v2.type == Type::LONG)
			return op(v1.data.d, v2.data.l);
		else if (v2.type == Type::DOUBLE)
			return op(v1.data.d, v2.data.d);
		else if (v2.type == Type::STRING)
			return op(v1.data.d, v2.data.s->len);
		else if (v2.type == Type::OBJECT)
			return op(v1.data.d, v2.data.o->arr.size());
	}
	
	else if (v1.type == Type::STRING){
		if (v2.type == Type::LONG)
			return op(v1.data.s->len, v2.data.l);
		else if (v2.type == Type::DOUBLE)
			return op(v1.data.s->len, v2.data.d);
		else if (v2.type == Type::STRING)
			return op(v1.data.s->sv(), v2.data.s->sv());
	}
	
	else if (v1.type == Type::OBJECT){
		if (v2.type == Type::LONG)
			return op(v1.data.o->arr.size(), v2.data.l);
		else if (v2.type == Type::DOUBLE)
			return op(v1.data.o->arr.size(), v2.data.d);
		else if (v2.type == Type::OBJECT)
			return op(v1.data.o->arr.size(), v2.data.o->arr.size());
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