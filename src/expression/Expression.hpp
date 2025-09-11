#pragma once
#include <memory>
#include <string>
#include <vector>
#include <variant>

#include "str_map.hpp"
#include "Debugger.hpp"


namespace Expression {
// ----------------------------------- [ Structures ] --------------------------------------- //


using StrValue = std::string;
using NumValue = std::variant<long,double>;
using Value = std::variant<long,double,std::string>;

struct Expr;
using pExpr = std::unique_ptr<Expr>;

using VariableMap = str_map<Value>;


// ----------------------------------- [ Structures ] --------------------------------------- //


struct Expr {
	virtual ~Expr(){}
	virtual Value eval(const VariableMap& vars, const Debugger&) noexcept = 0;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


pExpr parse(std::string_view str, const Debugger&) noexcept;


bool boolEval(const Value& val);
void str(const Value& val, std::string& buff);
void str(Value&& val, std::string& buff);


std::string serialize(const pExpr& e);


// ------------------------------------------------------------------------------------------ //
}