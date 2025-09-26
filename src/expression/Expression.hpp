#pragma once
#include "Value.hpp"
#include "str_map.hpp"
#include "Debugger.hpp"


using VariableMap = str_map<Value>;


class Expression {
// ----------------------------------- [ Structures ] --------------------------------------- //
public:
	struct Allocator;
	struct Operation;
	
// ------------------------------------[ Properties ] --------------------------------------- //
private:
	Allocator* alloc = nullptr;
	Operation* op = nullptr;
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	Expression() = default;
	
	Expression(Expression&& o) : alloc{o.alloc}, op{o.op} {
		o.alloc = nullptr;
		o.op = nullptr;
	}
	
	Expression& operator=(Expression&& o){
		std::swap(this->alloc, o.alloc);
		std::swap(this->op, o.op);
		return *this;
	}
	
public:
	~Expression();
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	Value eval(const VariableMap& vars, const Debugger&) const noexcept;
	
public:
	static Expression parse(std::string_view str, const Debugger&) noexcept;
	std::string serialize() const; 
	
// ----------------------------------- [ Operators ] ---------------------------------------- //
public:
	bool operator==(nullptr_t) const {
		return op == nullptr;
	}
	
	bool operator!=(nullptr_t) const {
		return op != nullptr;
	}
	
// ------------------------------------------------------------------------------------------ //
};


