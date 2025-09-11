#include "Expression.hpp"
#include <vector>


namespace Expression {
// ----------------------------------- [ Structures ] --------------------------------------- //


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


// ----------------------------------- [ Structures ] --------------------------------------- //


struct UnaryOp : public Expr {
	pExpr e;
};


struct BinaryOp : public Expr {
	pExpr a;
	pExpr b;
};


struct Const : public Expr {
	Value value;
	Value eval(const VariableMap& vars, const Debugger&) noexcept override {
		return value;
	}
};


struct Var : public Expr {
	std::string_view name;
	Value eval(const VariableMap& vars, const Debugger&) noexcept override;
};


struct Func : public Expr {
	std::string_view name;
	std::vector<pExpr> args;
	Value eval(const VariableMap& vars, const Debugger&) noexcept override;
};


// ----------------------------------- [ Structures ] --------------------------------------- //


struct Not : public UnaryOp {
	Value eval(const VariableMap& vars, const Debugger&) noexcept override;
};

struct Neg : public UnaryOp {
	Value eval(const VariableMap& vars, const Debugger&) noexcept override;
};

struct Add : public BinaryOp {
	Value eval(const VariableMap& vars, const Debugger&) noexcept override;
};

struct Sub : public BinaryOp {
	Value eval(const VariableMap& vars, const Debugger&) noexcept override;
};

struct Mul : public BinaryOp {
	Value eval(const VariableMap& vars, const Debugger&) noexcept override;
};

struct Div : public BinaryOp {
	Value eval(const VariableMap& vars, const Debugger&) noexcept override;
};


// ----------------------------------- [ Structures ] --------------------------------------- //


struct Eq : public BinaryOp {
	Value eval(const VariableMap& vars, const Debugger&) noexcept override;
};

struct Neq : public BinaryOp {
	Value eval(const VariableMap& vars, const Debugger&) noexcept override;
};

struct Lt : public BinaryOp {
	Value eval(const VariableMap& vars, const Debugger&) noexcept override;
};

struct Lte : public BinaryOp {
	Value eval(const VariableMap& vars, const Debugger&) noexcept override;
};

struct Gt : public BinaryOp {
	Value eval(const VariableMap& vars, const Debugger&) noexcept override;
};

struct Gte : public BinaryOp {
	Value eval(const VariableMap& vars, const Debugger&) noexcept override;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


template <typename C>
inline std::unique_ptr<Const> val(C&& c){
	auto pconst = std::make_unique<Const>();
	pconst->value = c;
	return pconst;
}

template <typename ...S>
inline std::unique_ptr<Var> var(S&&... c){
	auto pvar = std::make_unique<Var>();
	pvar->name = std::string(std::forward<S>(c)...);
	return pvar;
}


template <typename OP>
inline std::unique_ptr<OP> unop(pExpr&& a){
	auto expr = std::make_unique<OP>();
	expr->e = std::move(a);
	return expr;
}


template <typename OP>
inline std::unique_ptr<OP> binop(pExpr&& a, pExpr&& b){
	auto expr = std::make_unique<OP>();
	expr->a = std::move(a);
	expr->b = std::move(b);
	return expr;
}


template <typename ...ARG>
inline std::unique_ptr<Func> fun(auto&& name, ARG&&... arg){
	auto f = std::make_unique<Func>();
	f->name = std::string(std::forward<decltype(name)>(name));
	f->args.emplace_back(std::forward<ARG>(arg)...);
	return f;
}


// ------------------------------------------------------------------------------------------ //
}