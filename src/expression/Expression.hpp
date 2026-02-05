#pragma once
#include "Value.hpp"
#include "Macro.hpp"
#include "str_map.hpp"


using VariableMap = str_map<Value>;


class Expression {
// ----------------------------------- [ Structures ] --------------------------------------- //
public:
	struct Allocator;
	struct Operation;
	enum class Status;
	
// ------------------------------------[ Properties ] --------------------------------------- //
private:
	Allocator* alloc = nullptr;
	Operation* op = nullptr;
	
public:
	std::shared_ptr<Macro> origin;
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	Expression() = default;
	
	Expression(Expression&& o){
		swap(*this, o);
	}
	
public:
	~Expression();
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	Value eval(const VariableMap& vars) const noexcept;
	
public:
	static Expression parse(std::string_view str, const std::shared_ptr<Macro>& origin) noexcept;
	std::string serialize() const; 
	
// ----------------------------------- [ Operators ] ---------------------------------------- //
public:
	operator bool() const {
		return op != nullptr;
	}
	
	Expression& operator=(Expression&& o){
		swap(*this, o);
		return *this;
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	friend void swap(Expression& a, Expression& b) noexcept {
		std::swap(a.alloc, b.alloc);
		std::swap(a.op, b.op);
		std::swap(a.origin, b.origin);
	}
	
// ------------------------------------------------------------------------------------------ //
};


