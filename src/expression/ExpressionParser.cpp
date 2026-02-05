#include "Expression.hpp"
#include "ExpressionOperation.hpp"
#include "ExpressionAllocator.hpp"
#include <cassert>
#include <vector>
#include <charconv>

#include "Debug.hpp"
#include "DebugSource.hpp"

using namespace std;
using Operation = Expression::Operation;
using Allocator = Expression::Allocator;
using Status = Expression::Status;


// ----------------------------------- [ Structures ] --------------------------------------- //


enum class Expression::Status {
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
	string_view mark;
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


constexpr const char* parseWhitespace(const char* s, const char* end) noexcept {
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
	const char* const beg = s;
	
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
		Double* c = alc.alloc<Double>();
		out = c;
		c->type = Operation::Type::DOUBLE;
		
		from_chars_result res = from_chars(beg, s, c->n);
		
		if (res.ec != errc()){
			throw Error(Status::INVALID_FLOAT, string_view(beg, s));
		}
		
	} else {
		Long* c = alc.alloc<Long>();
		out = c;
		c->type = Operation::Type::LONG;
		
		from_chars_result res = from_chars(beg, s, c->n);
		
		if (res.ec != errc()){
			throw Error(Status::INVALID_INT, string_view(beg, s));
		}
		
	}
	
	out->len = s - beg;
	out->pos = beg;
	return s;
}


static const char* parse_str(const char* s, const char* end, Allocator& alc, Operation*& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(isQuot(*s));
	
	const char* const beg = s;
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
	
	assert(isQuot(*beg) && isQuot(*s) && *beg == *s);
	s++;
	
	String* c = alc.alloc<String>();
	c->type = Operation::Type::STRING;
	c->len = s - beg;
	c->pos = beg;
	
	out = c;
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static const char* parse_unaryExpression(const char* s, const char* end, Allocator& alc, Operation*& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(isUnaryOp(*s));
	
	const char* const beg = s;
	const char op = *s;
	
	s = parseWhitespace(s + 1, end);
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
	
	unop->len = s - beg;
	unop->pos = beg;
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
			if (s+1 != end && s[1] == '|'){
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
	binop->len = (b->pos + b->len) - (a->pos);
	binop->pos = a->pos;
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


static Operation* prattBinopTree(Allocator& alc, vector<Operation*>& args, vector<Operation::Type>& ops){
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
	s = parseWhitespace(s, end);
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
	
	s = parseWhitespace(s, end);
	if (s == end){
		throw Error(Status::INVALID_BINARY_EXPRESSION, string_view(beg, s));
	}
	
	Operation* second = nullptr;
	s = parse_singleExpression(s, end, alc, second);
	assert(second != nullptr);
	
	// Lookahead for more operations
	s = parseWhitespace(s, end);
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
		s = parseWhitespace(s, end);
		if (s == end){
			throw Error(Status::INVALID_BINARY_EXPRESSION, string_view(mark, s));
		}
		
		Operation* arg = nullptr;
		s = parse_singleExpression(s, end, alc, arg);
		assert(arg != nullptr);
		args.emplace_back(arg);
		
		// Look ahead for more binops
		s = parseWhitespace(s, end);
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
	
	Operation* argv[Function::MAX_ARGS];
	int argc = 0;
	
	// Parse arguments
	const char* const beg = s++;
	while (true){
		s = parseWhitespace(s, end);
		
		if (s == end){ unclosed:
			throw Error(Status::UNCLOSED_EXPRESSION, string_view(beg, s));
		} else if (*s == ')'){
			break;
		} else if (*s == ','){
			if (argc == 0){
				throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
			}
			
			s = parseWhitespace(s+1, end);
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
			argv[argc++] = arg;
		}
		
	}
	
	assert(*s == ')');
	s++;
	
	// Create function
	Function* f = static_cast<Function*>(alc.alloc(sizeof(Function) + sizeof(*Function::argv)*argc));
	f->type = Operation::Type::FUNC;
	f->len = s - name.begin();
	f->pos = name.begin();
	f->name_len = uint32_t(name.length());
	f->argc = argc;
	copy(argv, argv + argc, f->argv);
	
	out = f;
	return s;
}


static const char* parse_varOrFunc(const char* s, const char* end, Allocator& alc, Operation*& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(isFirstIdentifier(*s));
	
	string_view id;
	s = parse_identifier(s, end, id);
	assert(!id.empty());
	
	// Check if function
	s = parseWhitespace(s, end);
	if (s != end && *s == '('){
		return parse_func(s, end, alc, id, out);
	}
	
	// Is variable
	else {
		Variable* var = alc.alloc<Variable>();
		var->type = Operation::Type::VAR;
		var->len = uint32_t(id.length());
		var->pos = id.begin();
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
		
		s = parseWhitespace(s + 1, end);
		s = parse_binaryExpressionChain(s, end, alc, out);
		assert(out != nullptr);
		
		s = parseWhitespace(s, end);
		if (s == end || *s != ')'){
			throw Error(Status::UNCLOSED_EXPRESSION, string_view(beg, s));
		}
		
		return s + 1;
	}
	
	throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void report(const Macro* origin, const Error& err){
	if (origin == nullptr){
		return;
	}
	
	linepos pos = findLine(*origin, err.mark.begin());
	print(pos);
	LOG_STDERR(ANSI_BOLD ANSI_RED "error: " ANSI_RESET);
	
	switch (err.status){
		case Status::UNEXPECTED_SYMBOL:
			LOG_STDERR("Unexpected symbol: " PURPLE("`%.*s`") "\n", VA_STRV(err.mark));
			break;
		case Status::UNCLOSED_STRING:
			LOG_STDERR("Unterminated string literal in expression.\n");
			break;
		case Status::UNCLOSED_EXPRESSION:
			LOG_STDERR("Missing closing bracket " PURPLE("`(`") " in expression.\n");
			break;
		case Status::INVALID_BINARY_EXPRESSION:
			LOG_STDERR("Missing second operand in binary epxression.\n");
			break;
		case Status::INVALID_INT:
			LOG_STDERR("Failed to parse integer.\n");
			break;
		case Status::INVALID_FLOAT:
			LOG_STDERR("Failed to parse float.\n");
			break;
		case Status::INVALID_UNARY_EXPRESSION:
			LOG_STDERR("Missing operand in unary expression.\n");
			break;
		case Status::FUNC_ARG_OVERFLOW:
			LOG_STDERR("Too many arguments in function. Maximum allowed is: " PURPLE("%d") "\n", Function::MAX_ARGS);
			break;
		case Status::MEMORY:
			LOG_STDERR("Ran out of memory when parsing expression.\n");
			break;
		case Status::ERROR:
			LOG_STDERR("Failed to parse expression.\n");
			break;
	}
	
	printCodeView(pos, err.mark, ANSI_RED);
}


Expression Expression::parse(string_view str, const shared_ptr<Macro>& origin) noexcept {
	if (str.empty()){
		return {};
	}
	
	try {
		Expression expr;
		expr.alloc = new Expression::Allocator();
		expr.origin = origin;
		
		const char* s = str.begin();
		const char* end = str.end();
		
		s = parseWhitespace(s, end);
		if (s == end){
			return expr;
		}
		
		s = parse_binaryExpressionChain(s, end, *expr.alloc, expr.op);
		s = parseWhitespace(s, end);
		assert(expr.op != nullptr);
		
		if (s != end){
			throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
		}
		
		return expr;
	} catch (const Error& e){
		report(origin.get(), e);
	} catch (const bad_alloc&){
		report(origin.get(), Error(Status::MEMORY, str));
	}catch (...){
		report(origin.get(), Error(Status::ERROR, str));
	}
	
	return {};
}


// ------------------------------------------------------------------------------------------ //