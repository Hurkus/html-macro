#pragma once
#include "Expression.hpp"


// ----------------------------------- [ Structures ] --------------------------------------- //


struct Expression::Operation {
	enum class Type : uint32_t {
		ERROR,
		LONG, DOUBLE, STRING, OBJECT, VAR,
		NOT, NEG,
		ADD, SUB, MUL, DIV, MOD,
		AND, OR,
		EQ, NEQ, LT, LTE, GT, GTE,
		INDEX, FUNC,
	} type;
	
	uint32_t len;		// Length of `pos`.
	const char* pos;	// Position in the source text.
	
	std::string_view view() const {
		return std::string_view(pos, len);
	}
	
};


// ----------------------------------- [ Structures ] --------------------------------------- //


struct Long : public Expression::Operation {
	long n;
};


struct Double : public Expression::Operation {
	double n;
};


struct String : public Expression::Operation {
	std::string_view str() const {
		if (len >= 2) [[likely]]
			return std::string_view(pos + 1, len - 2);
		return view();
	}
};


struct Object : public Expression::Operation {
	struct Entry {
		Operation* key = nullptr;		// `null` if `value` is an array element.
		Operation* value = nullptr;		// Array element or dictionary entry value.
	};
	
	uint32_t count;			// Number of elements in `elements`.
	Entry elements[];
};


struct Variable : public Expression::Operation {
	std::string_view name() const {
		return std::string_view(pos, len);
	}
};


struct UnaryOperation : public Expression::Operation {
	Operation* arg;
};


struct BinaryOperation : public Expression::Operation {
	Operation* arg_1;
	Operation* arg_2;
};


struct Index : public Expression::Operation {
	Operation* obj;
	Operation* index;
};


struct Function : public Expression::Operation {
	uint32_t name_len;
	uint32_t argc;			// Number of arguments in `argv`.
	Operation* argv[];
	
	std::string_view name() const {
		return std::string_view(pos, name_len);
	}
	
};


// ------------------------------------------------------------------------------------------ //