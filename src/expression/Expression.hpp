#pragma once
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include "str_map.hpp"


namespace Expression {
// ----------------------------------- [ Structures ] --------------------------------------- //


using StrValue = std::string;
using NumValue = std::variant<long,double>;
using Value = std::variant<long,double,std::string>;

struct Expr;
using pExpr = std::unique_ptr<Expr>;

using VariableMap = str_map<Value>;


// ----------------------------------- [ Structures ] --------------------------------------- //


enum class ParseStatus {
	OK,
	UNEXPECTED_SYMBOL,
	UNCLOSED_STRING,
	UNCLOSED_EXPRESSION,
	INVALID_EXPRESSION,
	INVALID_BINARY_EXPRESSION,
	INVALID_INT,
	INVALID_FLOAT,
	INVALID_IDENTIFIER,
	INVALID_UNARY_OP,
	INVALID_BINARY_OP,
	ERROR
};

struct ParseResult {
	ParseStatus status;
	pExpr expr;
	std::string_view errMark;
};


struct Expr {
	virtual ~Expr(){}
	virtual Value eval(const VariableMap& vars) noexcept {
		return 0;
	}
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool boolEval(const Value& val);
void str(const Value& val, std::string& buff);
void str(Value&& val, std::string& buff);

ParseResult parse(std::string_view str) noexcept;
pExpr try_parse(std::string_view str) noexcept;

std::string serialize(const pExpr& e);


// ------------------------------------------------------------------------------------------ //
}