#include "Expression.hpp"
#include "ExpressionAllocator.hpp"
#include <cmath>
#include "Debug.hpp"

using namespace std;
using Operation = Expression::Operation;


// ---------------------------------- [ Definitions ] --------------------------------------- //


#define IS_TYPE(e, t)	(std::is_same_v<std::decay_t<decltype(e)>, t>)
#define IS_STR(e)		(IS_TYPE(e, string))
#define IS_LONG(e)		(IS_TYPE(e, long))
#define IS_DOUBLE(e)	(IS_TYPE(e, double))
#define IS_NUM(e)		(IS_LONG(e) || IS_DOUBLE(e))

Value eval(const Operation& op, const VariableMap& vars, const Debugger& dbg);
Value eval(const Function& op, const VariableMap& vars, const Debugger& dbg);


// ---------------------------------- [ Constructors ] -------------------------------------- //


Expression::~Expression(){
	delete alloc;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void eraseAll(string& src, string& word){
	string buff;
	buff.reserve(src.length());
	
	size_t a = 0;
	size_t b = src.find(word, a);
	while (b != string::npos){
		buff.append(src, a, b - a);
		a = b + word.length();
		b = src.find(word, a);
	}
	
	buff.append(src, a);
	swap(src, buff);
}

static void mul(string& src, long n){
	size_t len = src.length();
	if (n <= 0){
		src.clear();
	} else {
		src.reserve(len * n);
		while (n-- > 1)
			src.append(src, 0, len);
	}
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value eval(const Variable& var, const VariableMap& vars, const Debugger& dbg){
	const Value* val = vars.get(var.name);
	if (val != nullptr){
		return *val;
	} else {
		string_view name = var.name;
		HERE(dbg.warn(name, "Undefined variable " ANSI_PURPLE "'%.*s'" ANSI_RESET " defaulted to 0.\n", name.length(), name.data()));
		return Value(0);
	}
}


static Value nott(const UnaryOperation& unop, const VariableMap& vars, const Debugger& dbg){
	assert(unop.arg != nullptr);
	
	auto cast = [](auto&& val){
		if constexpr IS_STR(val)
			return Value(val.empty() ? 1l : 0l);
		else
			return Value(val == 0 ? 1l : 0l);
	};
	
	return visit(cast, eval(*unop.arg, vars, dbg));
}


static Value neg(const UnaryOperation& unop, const VariableMap& vars, const Debugger& dbg){
	assert(unop.arg != nullptr);
	Value val = eval(*unop.arg, vars, dbg);
	
	if (long* n = get_if<long>(&val)){
		return Value(-(*n));
	} else if (double* n = get_if<double>(&val)){
		return Value(-(*n));
	} else {
		return val;
	}
	
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Value add(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value arg_1 = eval(*binop.arg_1, vars, dbg);
	Value arg_2 = eval(*binop.arg_2, vars, dbg);
	
	auto cast = [&](auto& a, auto& b){
		if constexpr (IS_STR(a) && IS_STR(b)){
			a.append(b);
			return move(arg_1);
		} else if constexpr (IS_STR(a) && IS_NUM(b)) {
			a.append(to_string(b));
			return move(arg_1);
		} else if constexpr (IS_NUM(a) && IS_STR(b)) {
			b.insert(0, to_string(a));
			return move(arg_2);
		} else {
			return Value(a + b);
		}
	};
	
	return visit(cast, arg_1, arg_2);
}


static Value sub(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value arg_1 = eval(*binop.arg_1, vars, dbg);
	Value arg_2 = eval(*binop.arg_2, vars, dbg);
	
	auto cast = [&](auto& a, auto& b){
		if constexpr (IS_STR(a) && IS_STR(b)){
			eraseAll(a, b);
		} else if constexpr (IS_STR(a) && IS_NUM(b)) {
			long n = long(b);
			if (n <= 0);
			else if (size_t(n) > a.length())
				a.clear();
			else
				a.resize(a.length() - size_t(n));
		} else if constexpr (IS_NUM(a) && IS_STR(b)) {
			// TODO
		} else {
			return Value(a - b);
		}
		return move(arg_1);
	};
	
	return visit(cast, arg_1, arg_2);
}


static Value mul(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value arg_1 = eval(*binop.arg_1, vars, dbg);
	Value arg_2 = eval(*binop.arg_2, vars, dbg);
	
	auto cast = [&](auto& a, auto& b){
		if constexpr (IS_STR(a) && IS_STR(b)){
			// TODO
		} else if constexpr (IS_STR(a) && IS_NUM(b)) {
			mul(a, long(b));
		} else if constexpr (IS_NUM(a) && IS_STR(b)) {
			mul(b, long(a));
			return move(arg_2);
		} else {
			return Value(a * b);
		}
		return move(arg_1);
	};
	
	return visit(cast, arg_1, arg_2);
}


static Value div(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value arg_1 = eval(*binop.arg_1, vars, dbg);
	Value arg_2 = eval(*binop.arg_2, vars, dbg);
	
	auto cast = [&](auto& a, auto& b){
		if constexpr (IS_STR(a) && IS_STR(b)){
			return move(arg_1);
		} else if constexpr (IS_STR(a) && IS_NUM(b)) {
			return move(arg_1);
		} else if constexpr (IS_NUM(a) && IS_STR(b)) {
			return move(arg_2);
		} else {
			return Value(a / b);
		}
	};
	
	return visit(cast, arg_1, arg_2);
}


static Value mod(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value arg_1 = eval(*binop.arg_1, vars, dbg);
	Value arg_2 = eval(*binop.arg_2, vars, dbg);
	
	auto cast = [&](auto& a, auto& b){
		if constexpr (IS_STR(a) && IS_STR(b)){
			return move(arg_1);
		} else if constexpr (IS_STR(a) && IS_NUM(b)) {
			return move(arg_1);
		} else if constexpr (IS_NUM(a) && IS_STR(b)) {
			return move(arg_2);
		} else if constexpr (IS_LONG(a) && IS_LONG(b)) {
			return Value(a % b);
		} else {
			return Value(remainder(a, b));
		}
	};
	
	return visit(cast, arg_1, arg_2);
}


static Value xxor(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value arg_1 = eval(*binop.arg_1, vars, dbg);
	Value arg_2 = eval(*binop.arg_2, vars, dbg);
	
	auto cast = [&](auto& a, auto& b){
		if constexpr (IS_STR(a) && IS_STR(b)){
		} else if constexpr (IS_STR(a) && IS_NUM(b)) {
		} else if constexpr (IS_NUM(a) && IS_STR(b)) {
		} else if constexpr (IS_LONG(a) && IS_LONG(b)) {
			return Value(a ^ b);
		}
		return move(arg_1);;
	};
	
	return visit(cast, arg_1, arg_2);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template<typename OP>
static Value equals(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value arg_1 = eval(*binop.arg_1, vars, dbg);
	Value arg_2 = eval(*binop.arg_2, vars, dbg);
	
	auto cast = [](const auto& a, const auto& b) -> bool {
		if constexpr (IS_STR(a) && IS_STR(b)){
			return OP{}(a, b);
		} else if constexpr (IS_STR(a) && IS_NUM(b)) {
			return OP{}(a, to_string(b));
		} else if constexpr (IS_NUM(a) && IS_STR(b)) {
			return OP{}(to_string(a), b);
		} else {
			return OP{}(a, b);
		}
	};
	
	return visit(cast, arg_1, arg_2);
}


template<typename OP>
static Value cmp(const BinaryOperation& binop, const VariableMap& vars, const Debugger& dbg){
	assert(binop.arg_1 != nullptr && binop.arg_2 != nullptr);
	Value arg_1 = eval(*binop.arg_1, vars, dbg);
	Value arg_2 = eval(*binop.arg_2, vars, dbg);
	
	auto cast = [](const auto& a, const auto& b) -> bool {
		if constexpr (IS_STR(a) && IS_STR(b)){
			return OP{}(a.length(), b.length());
		} else if constexpr (IS_STR(a) && IS_NUM(b)) {
			return OP{}(a.length(), to_string(b));
		} else if constexpr (IS_NUM(a) && IS_STR(b)) {
			return OP{}(to_string(a), b.length());
		} else {
			return OP{}(a, b);
		}
	};
	
	bool r = visit(cast, arg_1, arg_2);
	return Value(r ? 1L : 0L);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Value eval(const Operation& op, const VariableMap& vars, const Debugger& dbg){
	switch (op.type){
		case Operation::Type::LONG:
			return static_cast<const Long&>(op).n;
		case Operation::Type::DOUBLE:
			return static_cast<const Double&>(op).n;
		case Operation::Type::STRING:
			return Value(in_place_type<string>, static_cast<const String&>(op).s);
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
		case Operation::Type::EQ:
			return equals<equal_to<>>(static_cast<const BinaryOperation&>(op), vars, dbg);
		case Operation::Type::NEQ:
			return equals<not_equal_to<>>(static_cast<const BinaryOperation&>(op), vars, dbg);
		case Operation::Type::LT:
			return equals<less<>>(static_cast<const BinaryOperation&>(op), vars, dbg);
		case Operation::Type::LTE:
			return equals<less_equal<>>(static_cast<const BinaryOperation&>(op), vars, dbg);
		case Operation::Type::GT:
			return equals<greater<>>(static_cast<const BinaryOperation&>(op), vars, dbg);
		case Operation::Type::GTE:
			return equals<greater_equal<>>(static_cast<const BinaryOperation&>(op), vars, dbg);
		case Operation::Type::FUNC:
			return eval(static_cast<const Function&>(op), vars, dbg);
		case Operation::Type::ERROR:
			dbg.error(dbg.mark(), "Bad expression.\n");
			assert(false);
			break;
	}
	return Value(0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


Value Expression::eval(const VariableMap& vars, const Debugger& dbg) const noexcept {
	if (op == nullptr){
		dbg.error(dbg.mark(), "Bad expression.\n");
		assert(op != nullptr);
		return Value(0);
	}
	return ::eval(*op, vars, dbg);
}


// ------------------------------------------------------------------------------------------ //