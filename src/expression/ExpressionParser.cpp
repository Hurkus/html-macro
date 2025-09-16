#include "Expression.hpp"
#include "ExpressionOperation.hpp"
#include "ExpressionAllocator.hpp"
#include <cassert>
#include <vector>
#include <charconv>
#include <type_traits>

#include "Debug.hpp"

using namespace std;
using Operation = Expression::Operation;
using Allocator = Expression::Allocator;


// ---------------------------------- [ Definitions ] --------------------------------------- //


#define IS_TYPE(e, t)	(std::is_same_v<std::decay_t<decltype(e)>, t>)
#define IS_STR(e)		IS_TYPE(e, StrValue)


// ----------------------------------- [ Structures ] --------------------------------------- //


enum class Status {
	UNEXPECTED_SYMBOL,
	UNCLOSED_STRING,			// Missing quote "
	UNCLOSED_EXPRESSION,		// Missing bracket )
	INVALID_BINARY_EXPRESSION,	// Missing second operand in binary epxression.
	INVALID_UNARY_EXPRESSION,	// Missing operand in unary expression.
	INVALID_INT,
	INVALID_FLOAT,
	FUNC_ARG_OVERFLOW,			// Too many function arguments.
	MEMORY,
	ERROR,
};


struct Error {
	Status status;
	std::string_view mark;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr bool isSpace(char c){
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

constexpr bool isQuot(char c){
	return c == '"' || c == '\'' || c == '`';
}

constexpr bool isDigit(char c){
	return '0' <= c && c <= '9';
}

constexpr bool isFirstIdentifier(char c){
	return isalpha(c);
}

constexpr bool isIdentifier(char c){
	return isalnum(c) || c == '_';
}

constexpr bool isUnaryOp(int c){
	return c == '+' || c == '-' || c == '!';
}


constexpr const char* parse_whiteSpace(const char* s, const char* end) noexcept {
	assert(s != nullptr && end != nullptr);
	while (s != end && isSpace(*s)) s++;
	return s;
}


// ----------------------------------- [ Prototypes ] --------------------------------------- //


static const char* parse_singleExpression(const char* s, const char* end, Allocator& alc, Operation*& out);
static const char* parse_binaryExpressionChain(const char* s, const char* end, Allocator& alc, Operation*& out);


// ----------------------------------- [ Functions ] ---------------------------------------- //


static const char* parse_num(const char* s, const char* end, Allocator& alc, Operation*& out){
	assert(s != nullptr && end != nullptr && s != end);
	const char* beg = s;
	
	if (*s == '+' || *s == '-'){
		s++;
	}
	
	bool dot = false;
	while (s != end){
		
		if (isDigit(*s)){
			goto next;
		} else if (*s == '.'){
			if (dot)
				throw Error(Status::INVALID_FLOAT, string_view(beg, s+1));
			dot = true;
		} else {
			break;
		}
		
		next:
		s++;
	}
	
	if (dot){
		Double* c = (Double*&)out = alc.alloc<Double>();
		c->type = Operation::Type::DOUBLE;
		from_chars_result res = from_chars(beg, s, c->n);
		
		if (res.ec != errc()){
			throw Error(Status::INVALID_FLOAT, string_view(beg, s));
		}
		
	} else {
		Long* c = (Long*&)out = alc.alloc<Long>();
		c->type = Operation::Type::LONG;
		from_chars_result res = from_chars(beg, s, c->n);
		
		if (res.ec != errc()){
			throw Error(Status::INVALID_INT, string_view(beg, s));
		}
		
	}
	
	return s;
}


static const char* parse_str(const char* s, const char* end, Allocator& alc, Operation*& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(isQuot(*s));
	
	const char* beg = s;
	const char quot = *beg;
	bool escape = false;
	
	while (true){
		s++;
		
		if (s == end){
			throw Error(Status::UNCLOSED_STRING, string_view(beg, s));
		} else if (*s == '\\'){
			escape = !escape;
		} else if (*s == quot){
			if (!escape){
				break;
			}
		} else {
			escape = false;
		}
		
	}
	
	assert(isQuot(*beg) && *beg == *s);
	String* c = (String*&)out = alc.alloc<String>();
	c->type = Operation::Type::STRING;
	c->s = string_view(beg + 1, s);
	
	return s + 1;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static const char* parse_unaryExpression(const char* s, const char* end, Allocator& alc, Operation*& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(isUnaryOp(*s));
	
	const char* beg = s;
	const char op = *s;
	
	s = parse_whiteSpace(s + 1, end);
	if (s == end){
		throw Error(Status::INVALID_UNARY_EXPRESSION, string_view(beg, s+1));
	}
	
	Operation* subexpr = nullptr;
	s = parse_singleExpression(s, end, alc, subexpr);
	
	UnaryOperation* unop = alc.alloc<UnaryOperation>();
	switch (op){
		case '-':
			unop->type = Operation::Type::NEG;
			unop->arg = subexpr;
			break;
		case '!':
			unop->type = Operation::Type::NOT;
			unop->arg = subexpr;
			break;
		case '+':
			break;
		default:
			assert(false);
			break;
	}
	
	out = unop;
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static Operation::Type try_parse_binaryOp(const char*& s, const char* end) noexcept {
	assert(s != nullptr && end != nullptr && s != end);
	switch (*s){
		case '+':
			s++;
			return Operation::Type::ADD;
		case '-':
			s++;
			return Operation::Type::SUB;
		case '*':
			s++;
			return Operation::Type::MUL;
		case '/':
			s++;
			return Operation::Type::DIV;
		case '%':
			s++;
			return Operation::Type::MOD;
		case '^':
			s++;
			return Operation::Type::XOR;
		
		case '|':
			if (s+1 != end && s[1] == '&'){
				s += 2;
				return Operation::Type::OR;
			}
			return Operation::Type::ERROR;
		
		case '&':
			if (s+1 != end && s[1] == '&'){
				s += 2;
				return Operation::Type::AND;
			}
			return Operation::Type::ERROR;
		
		case '>':
			s++;
			if (s != end && *s == '='){
				s++;
				return Operation::Type::GTE;
			}
			return Operation::Type::GT;
			
		case '<':
			s++;
			if (s != end && *s == '='){
				s++;
				return Operation::Type::LTE;
			}
			return Operation::Type::LT;
		
		case '!':
			if (s+1 != end && s[1] == '='){
				s += 2;
				return Operation::Type::NEQ;
			}
			return Operation::Type::ERROR;
			
		case '=':
			if (s+1 != end && s[1] == '='){
				s += 2;
				return Operation::Type::EQ;
			}
			return Operation::Type::ERROR;
		
		default:
			return Operation::Type::ERROR;
	}
}


inline BinaryOperation* _binop(Allocator& alc, Operation* a, Operation* b, Operation::Type type){
	assert(a != nullptr && b != nullptr);
	BinaryOperation* binop = alc.alloc<BinaryOperation>();
	binop->type = type;
	binop->arg_1 = a;
	binop->arg_2 = b;
	return binop;
}


constexpr int _bindPower(Operation::Type type){
	switch (type){
		case Operation::Type::OR:  return 10;
		case Operation::Type::AND: return 20;
		case Operation::Type::EQ:  return 30;
		case Operation::Type::NEQ: return 30;
		case Operation::Type::LT:  return 30;
		case Operation::Type::LTE: return 30;
		case Operation::Type::GT:  return 30;
		case Operation::Type::GTE: return 30;
		case Operation::Type::ADD: return 40;
		case Operation::Type::SUB: return 40;
		case Operation::Type::MUL: return 50;
		case Operation::Type::DIV: return 50;
		case Operation::Type::MOD: return 50;
		case Operation::Type::XOR: return 50;
		default:
			assert(false);
			return 0;
	}
}


Operation* prattBinopTree(Allocator& alc, vector<Operation*>& args, vector<Operation::Type>& ops){
	if (args.size() == 0){
		return nullptr;
	}
	
	assert(args.size() == ops.size()+1);
	size_t i = 0;
	
	auto capture = [&](auto& capture, int bp) -> Operation* {
		Operation* left = args[i];
		
		while (i < ops.size()){
			const Operation::Type op = ops[i];
			const int bp_left = _bindPower(op);
			const int bp_right = bp_left + 1;
			
			if (bp_left < bp){
				break;
			}
			
			i++;
			Operation* right = capture(capture, bp_right);
			left = _binop(alc, left, right, op);
		}
		
		return left;
	};
	
	return capture(capture, 0);
}


static const char* parse_binaryExpressionChain(const char* s, const char* end, Allocator& alc, Operation*& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(!isSpace(*s));
	const char* beg = s;
	
	Operation* first = nullptr;
	s = parse_singleExpression(s, end, alc, first);
	s = parse_whiteSpace(s, end);
	assert(first != nullptr);
	
	// No binary operation
	if (s == end){
		out = first;
		return s;
	}
	
	Operation::Type binop = try_parse_binaryOp(s, end);
	if (binop == Operation::Type::ERROR){
		out = first;
		return s;
	}
	
	s = parse_whiteSpace(s, end);
	if (s == end){
		throw Error(Status::INVALID_BINARY_EXPRESSION, string_view(beg, s));
	}
	
	Operation* second = nullptr;
	s = parse_singleExpression(s, end, alc, second);
	assert(second != nullptr);
	
	// Lookahead for more operations
	s = parse_whiteSpace(s, end);
	if (s == end){ one_bin:
		out = _binop(alc, first, second, binop);
		return s;
	}
	
	// Check for binop chain
	const char* mark = s;
	Operation::Type binop_2 = try_parse_binaryOp(s, end);
	if (binop_2 == Operation::Type::ERROR){
		goto one_bin;
	}
	
	// Chain binops
	vector<Operation*> args;
	vector<Operation::Type> ops;
	args.reserve(8);
	args.emplace_back(move(first));
	args.emplace_back(move(second));
	ops.reserve(args.size() - 1);
	ops.push_back(binop);
	ops.push_back(binop_2);
	
	// e1 + e2 + ...
	while (true){
		s = parse_whiteSpace(s, end);
		if (s == end){
			throw Error(Status::INVALID_BINARY_EXPRESSION, string_view(mark, s));
		}
		
		Operation* arg = nullptr;
		s = parse_singleExpression(s, end, alc, arg);
		assert(arg != nullptr);
		args.emplace_back(arg);
		
		// Look ahead for more binops
		s = parse_whiteSpace(s, end);
		if (s == end){
			break;
		}
		
		// Check for next binop
		mark = s;
		Operation::Type binop = try_parse_binaryOp(s, end);
		if (binop == Operation::Type::ERROR){
			break;
		}
		
		ops.push_back(binop);
	}
	
	// Build expression tree
	assert(args.size() == ops.size()+1);
	out = prattBinopTree(alc, args, ops);
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static const char* parse_identifier(const char* s, const char* end, string_view& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(isFirstIdentifier(*s));
	
	const char* beg = s++;
	while (s != end && isIdentifier(*s)) s++;
	
	out = string_view(beg, s);
	return s;
}


static const char* parse_func(const char* s, const char* end, Allocator& alc, string_view name, Operation*& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(*s == '(');
	
	Function::Arg argv[Function::MAX_ARGS];
	int argc = 0;
	
	// Parse arguments
	const char* beg = s++;
	while (true){
		s = parse_whiteSpace(s, end);
		
		if (s == end){ unclosed:
			throw Error(Status::UNCLOSED_EXPRESSION, string_view(beg, s));
		} else if (*s == ')'){
			break;
		} else if (*s == ','){
			if (argc == 0){
				throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
			}
			
			s = parse_whiteSpace(s+1, end);
			if (s == end){
				goto unclosed;
			}
			
		} else if (argc > 0){
			throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
		}
		
		Operation* arg = nullptr;
		const char* arg_beg = s;
		s = parse_binaryExpressionChain(s, end, alc, arg);
		assert(arg != nullptr);
		
		if (argc >= Function::MAX_ARGS){
			throw Error(Status::FUNC_ARG_OVERFLOW, string_view(arg_beg, s));
		} else {
			argv[argc++] = Function::Arg {
				.mark = string_view(arg_beg, s),
				.expr = arg
			};
		}
		
	}
	
	// Create function
	Function* f = static_cast<Function*>(alc.alloc(sizeof(Function) + sizeof(*Function::argv)*argc));
	f->type = Operation::Type::FUNC;
	f->name = name;
	f->argc = argc;
	copy(argv, argv + argc, f->argv);
	
	assert(*s == ')');
	out = f;
	return s + 1;
}


static const char* parse_varOrFunc(const char* s, const char* end, Allocator& alc, Operation*& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(isFirstIdentifier(*s));
	
	string_view id;
	s = parse_identifier(s, end, id);
	assert(!id.empty());
	
	// Check if function
	s = parse_whiteSpace(s, end);
	if (s != end && *s == '('){
		return parse_func(s, end, alc, id, out);
	}
	
	// Is variable
	else {
		Variable* var = alc.alloc<Variable>();
		var->type = Operation::Type::VAR;
		var->name = id;
		out = var;
	}
	
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static const char* parse_singleExpression(const char* s, const char* end, Allocator& alc, Operation*& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(!isSpace(*s));
	
	if (isDigit(*s)){
		return parse_num(s, end, alc, out);
	} else if (isQuot(*s)){
		return parse_str(s, end, alc, out);
	} else if (isFirstIdentifier(*s)){
		return parse_varOrFunc(s, end, alc, out);
	} else if (isUnaryOp(*s)){
		return parse_unaryExpression(s, end, alc, out);
	}
	
	else if (*s == '('){
		const char* beg = s;
		
		s = parse_whiteSpace(s + 1, end);
		s = parse_binaryExpressionChain(s, end, alc, out);
		assert(out != nullptr);
		
		s = parse_whiteSpace(s + 1, end);
		if (s == end || *s != ')'){
			throw Error(Status::UNCLOSED_EXPRESSION, string_view(beg, s));
		}
		
		return s;
	}
	
	throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void report(const Error& err, const Debugger& dbg){
	#define P(s) ANSI_PURPLE s ANSI_RESET
	const string_view& m = err.mark;
	
	switch (err.status){
		case Status::UNEXPECTED_SYMBOL:
			HERE(dbg.error(m, "Unexpected symbol " P("`%.*s`") " in expression.\n", int(m.length()), m.data()));
			break;
		case Status::UNCLOSED_STRING:
			HERE(dbg.error(m, "Unterminated string literal in expression.\n"));
			break;
		case Status::UNCLOSED_EXPRESSION:
			HERE(dbg.error(m, "Missing closing bracket " P("`(`") " in expression.\n"));
			break;
		case Status::INVALID_BINARY_EXPRESSION:
			HERE(dbg.error(m, "Missing second operand in binary epxression.\n"));
			break;
		case Status::INVALID_INT:
			HERE(dbg.error(m, "Failed to parse integer.\n"));
			break;
		case Status::INVALID_FLOAT:
			HERE(dbg.error(m, "Failed to parse float.\n"));
			break;
		case Status::INVALID_UNARY_EXPRESSION:
			HERE(dbg.error(m, "Missing operand in unary expression.\n"));
			break;
		case Status::FUNC_ARG_OVERFLOW:
			HERE(dbg.error(m, "Too many arguments in function. Maximum allowed is %d.\n", Function::MAX_ARGS));
			break;
		case Status::MEMORY:
			HERE(dbg.error(m, "Ran out of memory when parsing expression.\n"));
			break;
		case Status::ERROR:
			HERE(dbg.error(m, "Failed to parse expression.\n"));
			break;
	}
	
}


Expression Expression::parse(string_view str, const Debugger& dbg) noexcept {
	if (str.empty()){
		return {};
	}
	
	try {
		Expression expr;
		expr.alloc = new Expression::Allocator();
		
		const char* s = str.begin();
		const char* end = str.end();
		
		s = parse_whiteSpace(s, end);
		s = parse_binaryExpressionChain(s, end, *expr.alloc, expr.op);
		s = parse_whiteSpace(s, end);
		assert(expr.op != nullptr);
		
		if (s != end){
			throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
		}
		
		return expr;
	} catch (const Error& e){
		report(e, dbg);
	} catch (const bad_alloc&){
		report(Error(Status::MEMORY, str), dbg);
	}catch (...){
		report(Error(Status::ERROR, str), dbg);
	}
	
	return {};
}


// ------------------------------------------------------------------------------------------ //