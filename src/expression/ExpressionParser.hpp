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
	static pExpr makeTree(std::vector<pExpr>& args, std::vector<int>& ops);
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
private:
	std::unique_ptr<Expr> parseSingleExpression();
	std::unique_ptr<Expr::Const> parseNum();
	std::unique_ptr<Expr::Const> parseStr();
	
	std::unique_ptr<Expr> parseBinopChain();
	std::unique_ptr<Expr> parseUnaryExpression();
	
	std::unique_ptr<Expr::Var> parseVar();
	std::unique_ptr<Expr::Func> parseFunArgs();
	std::unique_ptr<Expr> parseVarOrFun();
	
	std::string getErrorLine(int from, int to = 0, int ptr = 0);
	
// ------------------------------------------------------------------------------------------ //
};
}


namespace Expression {
	inline pExpr parse(std::string_view str){
		return Parser().parse(str);
	}
}