#pragma once
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include "string_map.hpp"


namespace Expression {
// ----------------------------------- [ Structures ] --------------------------------------- //


using StrValue = std::string;
using NumValue = std::variant<long,double>;
using Value = std::variant<long,double,std::string>;

struct Expr;
using pExpr = std::unique_ptr<Expr>;


using VariableMap = string_map<Value>;


// ----------------------------------- [ Structures ] --------------------------------------- //


struct Expr {
	struct UnaryOp;
	struct BinaryOp;
	
	struct Const;
	struct Var;
	struct Not;
	struct Neg;
	struct Func;
	
	struct Add;
	struct Sub;
	struct Mul;
	struct Div;
	
	struct Eq;
	struct Neq;
	struct Lt;
	struct Lte;
	struct Gt;
	struct Gte;
	
	virtual ~Expr(){}
	virtual Value eval(const VariableMap& vars) noexcept { return 0; }
};


struct Expr::UnaryOp : public Expr {
	pExpr e;
};


struct Expr::BinaryOp : public Expr {
	pExpr a;
	pExpr b;
};


struct Expr::Const : public Expr {
	Value value;
	Value eval(const VariableMap& vars) noexcept override { return value; }
};


struct Expr::Var : public Expr {
	std::string var;
	Value eval(const VariableMap& vars) noexcept override;
};


struct Expr::Func : public Expr {
	std::string name;
	std::vector<pExpr> args;
	Value eval(const VariableMap& vars) noexcept override;
};


// ----------------------------------- [ Structures ] --------------------------------------- //


struct Expr::Not : public Expr::UnaryOp {
	Value eval(const VariableMap& vars) noexcept override;
};

struct Expr::Neg : public Expr::UnaryOp {
	Value eval(const VariableMap& vars) noexcept override;
};

struct Expr::Add : public Expr::BinaryOp {
	Value eval(const VariableMap& vars) noexcept override;
};

struct Expr::Sub : public Expr::BinaryOp {
	Value eval(const VariableMap& vars) noexcept override;
};

struct Expr::Mul : public Expr::BinaryOp {
	Value eval(const VariableMap& vars) noexcept override;
};

struct Expr::Div : public Expr::BinaryOp {
	Value eval(const VariableMap& vars) noexcept override;
};


// ----------------------------------- [ Structures ] --------------------------------------- //


struct Expr::Eq : public Expr::BinaryOp {
	Value eval(const VariableMap& vars) noexcept override;
};

struct Expr::Neq : public Expr::BinaryOp {
	Value eval(const VariableMap& vars) noexcept override;
};

struct Expr::Lt : public Expr::BinaryOp {
	Value eval(const VariableMap& vars) noexcept override;
};

struct Expr::Lte : public Expr::BinaryOp {
	Value eval(const VariableMap& vars) noexcept override;
};

struct Expr::Gt : public Expr::BinaryOp {
	Value eval(const VariableMap& vars) noexcept override;
};

struct Expr::Gte : public Expr::BinaryOp {
	Value eval(const VariableMap& vars) noexcept override;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool boolEval(const Value& val);
std::string str(const Expr* expr);


// ----------------------------------- [ Functions ] ---------------------------------------- //


template <typename C>
inline std::unique_ptr<Expr::Const> val(C&& c){
	auto pconst = std::make_unique<Expr::Const>();
	pconst->value = c;
	return pconst;
}

template <typename ...S>
inline std::unique_ptr<Expr::Var> var(S&&... c){
	auto pvar = std::make_unique<Expr::Var>();
	pvar->var = std::string(std::forward<S>(c)...);
	return pvar;
}


template <typename OP>
inline std::unique_ptr<OP> unop(pExpr&& a){
	auto expr = std::make_unique<OP>();
	expr->e = std::move(a);
	return expr;
}

inline std::unique_ptr<Expr::Not> nott(pExpr&& a){
	return unop<Expr::Not>(std::move(a));
}

inline std::unique_ptr<Expr::Neg> neg(pExpr&& a){
	return unop<Expr::Neg>(std::move(a));
}


template <typename OP>
inline std::unique_ptr<OP> binop(pExpr&& a, pExpr&& b){
	auto expr = std::make_unique<OP>();
	expr->a = std::move(a);
	expr->b = std::move(b);
	return expr;
}

inline std::unique_ptr<Expr::Add> add(pExpr&& a, pExpr&& b){
	return binop<Expr::Add>(std::move(a), std::move(b));
}

inline std::unique_ptr<Expr::Sub> sub(pExpr&& a, pExpr&& b){
	return binop<Expr::Sub>(std::move(a), std::move(b));
}

inline std::unique_ptr<Expr::Mul> mul(pExpr&& a, pExpr&& b){
	return binop<Expr::Mul>(std::move(a), std::move(b));
}

inline std::unique_ptr<Expr::Div> div(pExpr&& a, pExpr&& b){
	return binop<Expr::Div>(std::move(a), std::move(b));
}

inline std::unique_ptr<Expr::Eq> eq(pExpr&& a, pExpr&& b){
	return binop<Expr::Eq>(std::move(a), std::move(b));
}

inline std::unique_ptr<Expr::Neq> neq(pExpr&& a, pExpr&& b){
	return binop<Expr::Neq>(std::move(a), std::move(b));
}

inline std::unique_ptr<Expr::Lt> lt(pExpr&& a, pExpr&& b){
	return binop<Expr::Lt>(std::move(a), std::move(b));
}

inline std::unique_ptr<Expr::Lte> lte(pExpr&& a, pExpr&& b){
	return binop<Expr::Lte>(std::move(a), std::move(b));
}

inline std::unique_ptr<Expr::Gt> gt(pExpr&& a, pExpr&& b){
	return binop<Expr::Gt>(std::move(a), std::move(b));
}

inline std::unique_ptr<Expr::Gte> gte(pExpr&& a, pExpr&& b){
	return binop<Expr::Gte>(std::move(a), std::move(b));
}


template <typename ...ARG>
inline std::unique_ptr<Expr::Func> fun(auto&& name, ARG&&... arg){
	auto f = std::make_unique<Expr::Func>();
	f->name = std::string(std::forward<decltype(name)>(name));
	f->args.emplace_back(std::move(arg)...);
	return f;
}


// ------------------------------------------------------------------------------------------ //
}