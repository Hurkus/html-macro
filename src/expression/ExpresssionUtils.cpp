#include "ExpressionOperation.hpp"

using namespace std;
using Operation = Expression::Operation;


// ----------------------------------- [ Prototypes ] --------------------------------------- //


#define IS_TYPE(e, t)	(std::is_same_v<std::decay_t<decltype(e)>, t>)
#define IS_STR(e)		(IS_TYPE(e, string))
#define IS_LONG(e)		(IS_TYPE(e, long))
#define IS_DOUBLE(e)	(IS_TYPE(e, double))
#define IS_NUM(e)		(IS_LONG(e) || IS_DOUBLE(e))


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool toBool(const Value& val){
	auto f = [](const auto& val) -> bool {
		if constexpr IS_STR(val)
			return bool(val.length() != 0);
		else
			return bool(val != 0);
	};
	return visit(f, val);
}


string toStr(const Value& val){
	auto f = [&](const auto& val){
		if constexpr IS_STR(val)
			return val;
		else
			return to_string(val);
	};
	return visit(f, val);
}

string toStr(Value&& val){
	auto f = [&](const auto& val){
		if constexpr IS_STR(val)
			return move(val);
		else
			return to_string(val);
	};
	return visit(f, val);
}


void toStr(const Value& val, string& buff){
	auto f = [&](const auto& val){
		if constexpr IS_STR(val)
			buff.append(val);
		else
			buff.append(to_string(val));
	};
	visit(f, val);
}

void toStr(Value&& val, string& buff){
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


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void serialize(const Operation* op, string& buff);


static void serialize(const Variable& c, string& buff){
	buff.append(c.name);
}

static void serialize(const UnaryOperation& c, string& buff, string_view op){
	buff.append(op).push_back('(');
	serialize(c.arg, buff);
	buff.push_back(')');
}

static void serialize(const BinaryOperation& c, string& buff, string_view op){
	buff.push_back('(');
	serialize(c.arg_1, buff);
	buff.append(op);
	serialize(c.arg_2, buff);
	buff.push_back(')');
}

static void serialize(const Function& c, string& buff){
	buff.append(c.name).push_back('(');
	for (int i = 0 ; i < c.argc ; i++){
		if (i > 0)
			buff.push_back(',');
		serialize(c.argv[i].expr, buff);
	}
	buff.push_back(')');
}


static void serialize(const Operation* op, string& buff){
	if (op == nullptr){
		goto err;
	}
	
	switch (op->type){
		case Operation::Type::LONG:
			buff.append(to_string(static_cast<const Long&>(*op).n));
			return;
		case Operation::Type::DOUBLE:
			buff.append(to_string(static_cast<const Double&>(*op).n));
			return;
		case Operation::Type::STRING:
			buff.append(static_cast<const String&>(*op).s);
			return;
		case Operation::Type::VAR:
			return serialize(static_cast<const Variable&>(*op), buff);
		case Operation::Type::NOT:
			return serialize(static_cast<const UnaryOperation&>(*op), buff, "!");
		case Operation::Type::NEG:
			return serialize(static_cast<const UnaryOperation&>(*op), buff, "-");
		case Operation::Type::ADD:
			return serialize(static_cast<const BinaryOperation&>(*op), buff, "+");
		case Operation::Type::SUB:
			return serialize(static_cast<const BinaryOperation&>(*op), buff, "-");
		case Operation::Type::MUL:
			return serialize(static_cast<const BinaryOperation&>(*op), buff, "*");
		case Operation::Type::DIV:
			return serialize(static_cast<const BinaryOperation&>(*op), buff, "/");
		case Operation::Type::MOD:
			return serialize(static_cast<const BinaryOperation&>(*op), buff, "%");
		case Operation::Type::XOR:
			return serialize(static_cast<const BinaryOperation&>(*op), buff, "^");
		case Operation::Type::EQ:
			return serialize(static_cast<const BinaryOperation&>(*op), buff, "==");
		case Operation::Type::NEQ:
			return serialize(static_cast<const BinaryOperation&>(*op), buff, "!=");
		case Operation::Type::LT:
			return serialize(static_cast<const BinaryOperation&>(*op), buff, "<");
		case Operation::Type::LTE:
			return serialize(static_cast<const BinaryOperation&>(*op), buff, "<=");
		case Operation::Type::GT:
			return serialize(static_cast<const BinaryOperation&>(*op), buff, ">");
		case Operation::Type::GTE:
			return serialize(static_cast<const BinaryOperation&>(*op), buff, ">=");
		case Operation::Type::AND:
			return serialize(static_cast<const BinaryOperation&>(*op), buff, "&&");
		case Operation::Type::OR:
			return serialize(static_cast<const BinaryOperation&>(*op), buff, "||");
		case Operation::Type::FUNC:
			return serialize(static_cast<const Function&>(*op), buff);
		case Operation::Type::ERROR:
			buff.append("(NaN)");
			return;
	}
	
	err:
	buff.append("(null)");
	return;
}



string Expression::serialize() const {
	string s;
	::serialize(op, s);
	return s;
}


// ------------------------------------------------------------------------------------------ //