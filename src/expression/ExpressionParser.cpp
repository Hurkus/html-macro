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
	UNCLOSED_OBJECT,			// Missing bracket ]
	UNCLOSED_INDEXER,			// Missing bracket ]
	INVALID_BINARY_EXPRESSION,	// Missing second operand in binary epxression.
	INVALID_UNARY_EXPRESSION,	// Missing operand in unary expression.
	INVALID_INT,
	INVALID_FLOAT,
	MEMORY,
	ERROR,
};


struct Error {
	Status status;
	string_view mark;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr bool isWhitespace(char c){
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
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
	while (s != end && isWhitespace(*s)) s++;
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


static const char* parse_index(const char* beg, const char* end, Allocator& alc, Operation*& in_out){
	assert(beg != nullptr && end != nullptr && beg != end);
	assert(*beg == '[');
	
	const char* s = parseWhitespace(beg + 1, end);
	if (s == end){
		throw Error(Status::UNCLOSED_INDEXER, string_view(beg, s));
	}
	
	Operation* i = nullptr;
	s = parse_binaryExpressionChain(s, end, alc, i);
	assert(i != nullptr);
	
	s = parseWhitespace(s, end);
	if (s == end || *s != ']'){
		throw Error(Status::UNCLOSED_INDEXER, string_view(beg, s));
	}
	
	assert(*s == ']');
	s++;
	
	assert(in_out != nullptr);
	Index* idx = alc.alloc<Index>();
	idx->type = Operation::Type::INDEX;
	idx->pos = beg;
	idx->len = s - beg;
	idx->obj = in_out;
	idx->index = i;
	in_out = idx;
	
	// Check if multi-dimensional indexer.
	if (s != end && *s == '['){
		return parse_index(s, end, alc, in_out);
	}
	
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
	assert(!isWhitespace(*s));
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


static const char* parse_object(const char* beg, const char* end, Allocator& alc, Operation*& out){
	assert(beg != nullptr && end != nullptr && beg != end);
	assert(*beg == '[');
	const char* s = beg + 1;
	
	vector<Object::Entry> v;
	v.reserve(8);
	
	while (true){
		s = parseWhitespace(s, end);
		
		if (s == end){
			throw Error(Status::UNCLOSED_OBJECT, string_view(beg, s));
		} else if (*s == ','){
			throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
		} else if (*s == ']'){
			break;
		}
		
		// Parse key or value
		Object::Entry& e = v.emplace_back();
		s = parse_binaryExpressionChain(s, end, alc, e.value);
		assert(e.value != nullptr);
		
		// Check if dictionary entry or array element.
		s = parseWhitespace(s, end);
		if (s == end){
			throw Error(Status::UNCLOSED_OBJECT, string_view(beg, s));
		} else if (*s == ','){
			s++;
			continue;
		} else if (*s == ']'){
			break;
		} else if (*s != ':'){
			throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
		}
		
		const char* _s = s;
		s = parseWhitespace(s + 1, end);
		if (s == end){
			throw Error(Status::UNCLOSED_OBJECT, string_view(beg, s));
		} else if (*s == ']'){
			throw Error(Status::UNEXPECTED_SYMBOL, string_view(_s, 1));
		}
		
		// Parse value
		e.key = e.value;
		s = parse_binaryExpressionChain(s, end, alc, e.value);
		assert(e.value != e.key && e.value != nullptr);
		
		// Next element
		s = parseWhitespace(s, end);
		if (s == end){
			throw Error(Status::UNCLOSED_OBJECT, string_view(beg, s));
		} else if (*s == ']'){
			break;
		} else if (*s != ','){
			throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
		}
		
		assert(*s == ',');
		s++;
	}
	
	assert(*s == ']');
	s++;
	
	// Create Object expression.
	const uint32_t count = uint32_t(min(v.size(), size_t(UINT32_MAX)));
	void* mem = alc.alloc(sizeof(Object) + count * sizeof(*Object::elements));
	
	Object* obj = new (mem) Object();
	obj->type = Operation::Type::OBJECT;
	obj->pos = beg;
	obj->len = s - beg;
	obj->count = count;
	copy(v.begin(), v.begin() + count, obj->elements);
	
	out = obj;
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


static const char* parse_func(const char* s, const char* end, string_view name, Allocator& alc, Operation*& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(*s == '(');
	
	vector<Operation*> argv;
	argv.reserve(8);
	
	// Parse arguments
	const char* const beg = s++;
	while (true){
		s = parseWhitespace(s, end);
		
		if (s == end){ unclosed:
			throw Error(Status::UNCLOSED_EXPRESSION, string_view(beg, s));
		} else if (*s == ')'){
			break;
		} else if (*s == ','){
			if (argv.empty()){
				throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
			}
			
			s = parseWhitespace(s+1, end);
			if (s == end){
				goto unclosed;
			}
			
		} else if (!argv.empty()){
			throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
		}
		
		Operation* arg = nullptr;
		s = parse_binaryExpressionChain(s, end, alc, arg);
		assert(arg != nullptr);
		argv.emplace_back(arg);
	}
	
	assert(*s == ')');
	s++;
	
	// Create function
	const uint32_t argc = uint32_t(min(argv.size(), size_t(UINT32_MAX)));
	Function* f = static_cast<Function*>(alc.alloc(sizeof(Function) + argc * sizeof(*Function::argv)));
	f->type = Operation::Type::FUNC;
	f->len = s - name.begin();
	f->pos = name.begin();
	f->name_len = uint32_t(name.length());
	f->argc = argc;
	copy(argv.begin(), argv.begin() + argc, f->argv);
	
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
		return parse_func(s, end, id, alc, out);
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


static const char* parse_singleExpression(const char* beg, const char* end, Allocator& alc, Operation*& out){
	assert(beg != nullptr && end != nullptr && beg != end);
	assert(!isWhitespace(*beg));
	const char* s = beg;
	
	if (isDigit(*s)){
		return parse_num(s, end, alc, out);
	} else if (isQuot(*s)){
		s = parse_str(s, end, alc, out);
		goto _check_index;
	} else if (isFirstIdentifier(*s)){
		s = parse_varOrFunc(s, end, alc, out);
		goto _check_index;
	} else if (isUnaryOp(*s)){
		s = parse_unaryExpression(s, end, alc, out);
		goto _check_index;
	}
	
	// Sub-expressions
	else if (*s == '('){
		const char* beg = s;
		
		s = parseWhitespace(s + 1, end);
		s = parse_binaryExpressionChain(s, end, alc, out);
		assert(out != nullptr);
		
		s = parseWhitespace(s, end);
		if (s == end || *s != ')'){
			throw Error(Status::UNCLOSED_EXPRESSION, string_view(beg, s));
		}
		
		s++;
		goto _check_index;
	}
	
	// Arrays and dictionaries
	else if (*s == '['){
		s = parse_object(s, end, alc, out);
		goto _check_index;
	} else {
		throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
	}
	
	_check_index:
	s = parseWhitespace(s, end);
	if (s != end && *s == '['){
		return parse_index(s, end, alc, out);
	}
	
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void report(const Macro* origin, const Error& err){
	if (origin == nullptr){
		return;
	}
	
	linepos pos = findLine(*origin, err.mark.begin());
	HERE(print(pos));
	LOG_STDERR(ANSI_BOLD ANSI_RED "error: " ANSI_RESET);
	
	switch (err.status){
		case Status::UNEXPECTED_SYMBOL:
			LOG_STDERR("Unexpected symbol: `" PURPLE("%.*s") "`\n", VA_STRV(err.mark));
			break;
		case Status::UNCLOSED_STRING:
			LOG_STDERR("Unterminated string literal in expression.\n");
			break;
		case Status::UNCLOSED_EXPRESSION:
			LOG_STDERR("Missing closing bracket `" PURPLE(")") "` in expression.\n");
			break;
		case Status::UNCLOSED_OBJECT:
			LOG_STDERR("Missing closing bracket `" PURPLE("]") "` in map/array expression.\n");
			break;
		case Status::UNCLOSED_INDEXER:
			LOG_STDERR("Missing closing bracket `" PURPLE("]") "` in indexer expression.\n");
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