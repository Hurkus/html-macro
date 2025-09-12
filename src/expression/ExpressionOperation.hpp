#pragma once
#include "Expression.hpp"


// ----------------------------------- [ Structures ] --------------------------------------- //


struct Expression::Operation {
	enum class Type {
		ERROR,
		LONG, DOUBLE, STRING, VAR,
		NOT, NEG,
		ADD, SUB, MUL, DIV, MOD, XOR,
		EQ, NEQ, LT, LTE, GT, GTE,
		FUNC,
	} type;
};


struct Long : public Expression::Operation {
	long n;
};

struct Double : public Expression::Operation {
	double n;
};

struct String : public Expression::Operation {
	std::string_view s;
};


struct Variable : public Expression::Operation {
	std::string_view name;
};


struct UnaryOperation : public Expression::Operation {
	Operation* arg;
};


struct BinaryOperation : public Expression::Operation {
	Operation* arg_1;
	Operation* arg_2;
};


struct Function : public Expression::Operation {
	int argc;	// Must be first so that it aligns with Operation::type and packs 8 bytes.
	std::string_view name;
	
	struct Arg {
		std::string_view mark;
		Operation* expr;
	} argv[];
	
	static constexpr int MAX_ARGS = 8;
};


// ------------------------------------------------------------------------------------------ //