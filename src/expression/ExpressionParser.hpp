#pragma once
#include <string_view>

#include "Expression.hpp"


namespace Expression {
class Parser {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	const char* s;
	int i = 0;
	int e = 0;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	pExpr parse(std::string_view s);
	
public:
	static pExpr makeTree(std::vector<pExpr>& args, std::vector<char>& ops);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
private:
	std::unique_ptr<Expr> parseExpression();
	std::unique_ptr<Expr::Const> parseNum();
	std::unique_ptr<Expr::Const> parseStr();
	
	std::unique_ptr<Expr> parseUnaryOp();
	std::unique_ptr<Expr> parseExpressionChain();
	
	std::unique_ptr<Expr::Var> parseVar();
	std::unique_ptr<Expr::Func> parseFunArgs();
	std::unique_ptr<Expr> parseVarOrFun();
	
	std::string getErrorLine(int from, int to = 0, int ptr = 0);
	
// ------------------------------------------------------------------------------------------ //
};
}
