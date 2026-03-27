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


/**
 * @brief If the object is not unique (`refs > 1`), it is cloned
 *        and the value is updated to hold the cloned object.
 * @return Reference to the unique object. It might be new.
 */
static Value::Object& uniq_obj(Value& val){
	assert(val.type == Type::OBJECT);
	assert(val.data.o != nullptr);
	
	if (val.data.o->refs > 1){
		unique_ptr<Value::Object> o2 = Value::Object::create();
		o2->arr = val.data.o->arr;
		val.data.o->dict.copy(o2->dict);
		val.data.o->unref();
		val.data.o = o2.release()->ref();
	}
	
	return *val.data.o;
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

static void warn_value_not_indexable(const Expression& self, const Operation& op){
	if (self.origin == nullptr){
		return;
	}
	
	string_view mark = op.view();
	linepos pos = findLine(*self.origin, mark.begin());
	
	print(pos);
	LOG_STDERR(WARN_PFX "Value cannot be indexed.\n");
	printCodeView(pos, mark, ANSI_YELLOW);
}

static void error_invalid_property_type(const Expression& self, const Operation& key){
	if (self.origin == nullptr){
		return;
	}
	
	string_view mark = key.view();
	linepos pos = findLine(*self.origin, mark.begin());
	
	print(pos);
	LOG_STDERR(WARN_PFX "Invalid key type. Only numbers and strings are allowed.\n");
	printCodeView(pos, mark, ANSI_YELLOW);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value var(const Expression& self, const Variable& var, const VariableMap& vars){
	const Value* val = vars.get(var.name());
	if (val != nullptr){
		return Value(*val);
	} else {
		HERE(warn_undefined_variable(self, var));
		return Value();
	}
}


static Value index(const Expression& self, const Index& idx, const VariableMap& vars){
	assert(idx.obj != nullptr);
	assert(idx.index != nullptr);
	Value obj = eval(self, *idx.obj, vars);
	Value index = eval(self, *idx.index, vars);
	
	if (obj.type == Type::NONE){
		return Value();
	} else if (obj.type != Type::OBJECT){
		HERE(warn_value_not_indexable(self, *idx.obj));
		return Value();
	}
	
	Value::Object& o = *obj.data.o;
	Value* el = nullptr;
	
	switch (index.type){
		case Type::LONG:
			el = o.get(index.data.l);
			break;
		case Type::DOUBLE:
			el = o.get(size_t(index.data.d));
			break;
		case Type::STRING:
			el = o.get(index.data.s->sv());
			break;
		case Type::OBJECT:
			HERE(error_invalid_property_type(self, *idx.index));
			break;
		case Type::NONE:
			break;
	}
	
	if (el != nullptr){
		return *el;
	}
	
	return Value();
}


static Value object(const Expression& self, const Object& objop, const VariableMap& vars){
	unique_ptr<Value::Object> obj = Value::Object::create();
	
	for (uint32_t i = 0 ; i < objop.count ; i++){
		const Object::Entry& e = objop.elements[i];
		assert(e.value != nullptr);
		
		// Is array element
		if (e.key == nullptr){
			obj->arr.emplace_back(eval(self, *e.value, vars));
		}
		
		// Is dictionary entry
		else {
			Value key = eval(self, *e.key, vars);
			Value val = eval(self, *e.value, vars);
			
			string key_buff;
			string_view key_s;
			
			// Verify type.
			switch (key.type){
				case Type::LONG:
					key_buff = to_string(key.data.l);
					key_s = key_buff;
					break;
				
				case Type::DOUBLE:
					key_buff = to_string(key.data.l);
					key_s = key_buff;
					break;
				
				case Type::STRING:
					key_s = key.data.s->sv();
					break;
				
				case Type::OBJECT:
					HERE(error_invalid_property_type(self, *e.key));
					goto next;
				
				case Type::NONE:
					goto next;
			}
			
			obj->insert(key_s, move(val));
		}
		
		next: continue;
	}
	
	return Value(obj.release());
}


static Value nott(const Expression& self, const UnaryOperation& unop, const VariableMap& vars){
	assert(unop.arg != nullptr);
	Value val = eval(self, *unop.arg, vars);
	return val.toBool() ? 0L : 1L;
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
			uniq_obj(v2).insert(0, move(v1));
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
			uniq_obj(v2).insert(0, move(v1));
			v1 = move(v2);
		}
	}
	
	else if (v1.type == Type::STRING){
		if (v2.type == Type::STRING){
			v1 = Value(v1.data.s->sv(), v2.toStr());
		} else if (v2.type == Type::OBJECT){
			uniq_obj(v2).insert(0, move(v1));
			v1 = move(v2);
		} else if (v2.type != Type::NONE){
			v1 = Value(v1.data.s->sv(), v2.toStr());
		}
	}
	
	else if (v1.type == Type::OBJECT){
		if (v2.type != Type::NONE)
			uniq_obj(v1).arr.emplace_back(move(v2));
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
		Value::Object& o1 = uniq_obj(v1);
		
		// Remove identical elements.
		for (ssize_t i = o1.arr.size() - 1 ; i >= 0 ; i--){
			if (o1.arr[i] == v2)
				o1.arr.erase(o1.arr.begin() + i);
		}
		
	}
	
	else if (v1.type == Type::NONE){
		v1 = move(v2);
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
	
	if (v1.type == Type::LONG){
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
		}
		else if (v2.type == Type::OBJECT){
			uniq_obj(v2).insert(0, move(v1));
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
		}
		else if (v2.type == Type::OBJECT){
			uniq_obj(v2).insert(0, move(v1));
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
			uniq_obj(v2).insert(0, move(v1));
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
				uniq_obj(v1).arr.emplace_back(move(v2));
				break;
			
			case Type::OBJECT:
				uniq_obj(v1).merge(*v2.data.o);
				break;
		}
	}
	
	else if (v1.type == Type::NONE){
		v1 = move(v2);
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
		Value::Object& o1 = uniq_obj(v1);
		
		if (v2.type == Type::LONG)
			o1.remove(static_cast<size_t>(v2.data.l));
		else if (v2.type == Type::DOUBLE)
			o1.remove(static_cast<size_t>(v2.data.d));
		else if (v2.type == Type::STRING)
			o1.remove(v2.data.s->sv());
		else if (v2.type == Type::OBJECT){
			// TODO: optimize by constructing the object without the elements instead of removing them.
			
			// Remove elements and properties indexed from o2 array.
			for (const Value& el : v2.data.o->arr){
				if (el.type == Type::LONG)
					o1.remove(size_t(el.data.l));
				else if (el.type == Type::DOUBLE)
					o1.remove(size_t(el.data.d));
				else if (el.type == Type::STRING)
					o1.remove(el.data.s->sv());
			}
			
			// Remove properties indexed from o2 dictionary.
			for (const auto& pair : v2.data.o->dict){
				o1.remove(pair.key);
			}
			
		}
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
		else if (v2.type == Type::OBJECT){
			const Value* v = v2.data.o->get(v1.data.l);
			if (v != nullptr)
				v1 = *v;
			else
				v1 = Value();
		}
	}
	
	else if (v1.type == Type::DOUBLE){
		if (v2.type == Type::LONG)
			v1 = fmod(v1.data.d, v2.data.l);
		else if (v2.type == Type::DOUBLE)
			v1 = fmod(v1.data.d, v2.data.d);
		else if (v2.type == Type::OBJECT){
			const Value* v = v2.data.o->get(long(v1.data.d));
			if (v != nullptr)
				v1 = *v;
			else
				v1 = Value();
		}
	}
	
	else if (v1.type == Type::STRING){
		if (v2.type == Type::OBJECT){
			const Value* v = v2.data.o->get(v1.data.s->sv());
			v1 = (v != nullptr) ? *v : Value();
		} else {
			v1 = Value();
		}
	}
	
	else if (v1.type == Type::OBJECT){
		Value::Object& o1 = *v1.data.o;
		
		if (v2.type == Type::LONG){
			Value* v = o1.get(static_cast<size_t>(v2.data.l));
			v1 = (v != nullptr) ? *v : Value();
		}
		else if (v2.type == Type::DOUBLE){
			Value* v = o1.get(static_cast<size_t>(v2.data.l));
			v1 = (v != nullptr) ? *v : Value();
		}
		else if (v2.type == Type::STRING){
			Value* v = o1.get(v2.data.s->sv());
			v1 = (v != nullptr) ? *v : Value();
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
	
	else if (v1.type == Type::NONE){
		v1 = move(v2);
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
		case Operation::Type::OBJECT:
			return object(self, static_cast<const Object&>(op), vars);
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
		case Operation::Type::INDEX:
			return index(self, static_cast<const Index&>(op), vars);
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