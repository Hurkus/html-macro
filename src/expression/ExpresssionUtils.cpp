#include "ExpressionOperation.hpp"

using namespace std;
using Operation = Expression::Operation;


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void serialize(const Operation* op, string& buff);


static void serialize(const Variable& var, string& buff){
	buff.append(var.name());
}

static void serialize(const Long& num, string& buff){
	buff.append(to_string(num.n));
}

static void serialize(const Double& num, string& buff){
	buff.append(to_string(num.n));
}

static void serialize(const String& str, string& buff){
	buff.append(str.str());
}

static void serialize(const UnaryOperation& un, string& buff, string_view op){
	buff.append(op).push_back('(');
	serialize(un.arg, buff);
	buff.push_back(')');
}

static void serialize(const BinaryOperation& bin, string& buff, string_view op){
	buff.push_back('(');
	serialize(bin.arg_1, buff);
	buff.append(op);
	serialize(bin.arg_2, buff);
	buff.push_back(')');
}

static void serialize(const Function& fun, string& buff){
	buff.append(fun.name()).push_back('(');
	for (uint32_t i = 0 ; i < fun.argc ; i++){
		if (i > 0)
			buff.push_back(',');
		serialize(fun.argv[i], buff);
	}
	buff.push_back(')');
}


static void serialize(const Operation* op, string& buff){
	if (op == nullptr){
		goto err;
	}
	
	switch (op->type){
		case Operation::Type::LONG:
			return serialize(static_cast<const Long&>(*op), buff);
		case Operation::Type::DOUBLE:
			return serialize(static_cast<const Double&>(*op), buff);
		case Operation::Type::STRING:
			return serialize(static_cast<const String&>(*op), buff);
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