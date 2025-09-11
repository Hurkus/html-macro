#include "Expression-impl.hpp"
#include <cassert>
#include <charconv>
#include <type_traits>

#include "Debug.hpp"

using namespace std;
using namespace Expression;


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

constexpr bool isArithOp(int c){
	return c == '+' || c == '-' || c == '*' || c == '/' || c == '%';
}

constexpr bool isCompOp(int c){
	switch (c){
		case '==':
		case '!=':
		case '<=':
		case '>=':
		case '<':
		case '>':
			return true;
		default:
			return false;
	}
}


constexpr const char* parse_whiteSpace(const char* s, const char* end) noexcept {
	assert(s != nullptr && end != nullptr);
	while (s != end && isSpace(*s)) s++;
	return s;
}


// ----------------------------------- [ Prototypes ] --------------------------------------- //


static const char* parse_singleExpression(const char* s, const char* end, unique_ptr<Expr>& out);


// ----------------------------------- [ Functions ] ---------------------------------------- //


static const char* parse_num(const char* s, const char* end, unique_ptr<Expr>& out){
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
	
	out = make_unique<Const>();
	Const& expr_const = static_cast<Const&>(*out);
	
	if (dot){
		double& n = expr_const.value.emplace<double>();
		from_chars_result res = from_chars(beg, s, n);
		
		if (res.ec != errc()){
			throw Error(Status::INVALID_FLOAT, string_view(beg, s));
		}
		
	} else {
		long& n = expr_const.value.emplace<long>();
		from_chars_result res = from_chars(beg, s, n);
		
		if (res.ec != errc()){
			throw Error(Status::INVALID_INT, string_view(beg, s));
		}
		
	}
	
	return s;
}


static const char* parse_str(const char* s, const char* end, unique_ptr<Expr>& out){
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
	out = make_unique<Const>();
	Const& expr_const = static_cast<Const&>(*out);
	expr_const.value.emplace<string>(beg + 1, s);
	return s + 1;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static const char* parse_unaryExpression(const char* s, const char* end, unique_ptr<Expr>& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(isUnaryOp(*s));
	
	const char* beg = s;
	const char op = *s;
	
	s = parse_whiteSpace(s + 1, end);
	if (s == end){
		throw Error(Status::INVALID_UNARY_EXPRESSION, string_view(beg, s+1));
	}
	
	unique_ptr<Expr> subexpr;
	s = parse_singleExpression(s, end, subexpr);
	
	switch (op){
		case '-':
			out = unop<Neg>(move(subexpr));
			break;
		case '!':
			out = unop<Not>(move(subexpr));
			break;
		case '+':
			out = move(subexpr);
			break;
		default:
			out = move(subexpr);
			assert(false);
			break;
	}
	
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static int try_parse_binaryOp(const char*& s, const char* end) noexcept {
	assert(s != nullptr && end != nullptr && s != end);
	switch (*s){
		case '+':
		case '-':
		case '*':
		case '/':
			s++;
			return s[-1];
		
		case '>':
		case '<':
			s++;
			if (s != end && *s == '='){
				s++;
				return (s[-2] << 8) | s[-1];
			}
			return s[-1];
		
		case '!':
		case '=':
			if (s+1 != end && s[1] == '='){
				s += 2;
				return (s[-2] << 8) | s[-1];
			}
			return 0;
	}
	return 0;
}


static unique_ptr<BinaryOp> _binop(pExpr&& a, pExpr&& b, int op){
	assert(a != nullptr && b != nullptr);
	switch (op){
		case '+':
			return binop<Add>(move(a), move(b));
		case '-':
			return binop<Sub>(move(a), move(b));
		case '*':
			return binop<Mul>(move(a), move(b));
		case '/':
			return binop<Div>(move(a), move(b));
		
		case '==':
			return binop<Eq>(move(a), move(b));
		case '!=':
			return binop<Neq>(move(a), move(b));
		case '>=':
			return binop<Gte>(move(a), move(b));
		case '<=':
			return binop<Lte>(move(a), move(b));
		case '>':
			return binop<Gt>(move(a), move(b));
		case '<':
			return binop<Lt>(move(a), move(b));
		
		default:
			assert(false);
			throw Error(Status::ERROR);
	}
}


void makeBinopTree(vector<unique_ptr<Expr>>& args, vector<int>& ops){
	assert(args.size() == ops.size()+1);
	if (args.size() <= 1){
		return;
	}
	
	// Merge multiplications
	for (long i = 0 ; i < long(ops.size()) ; i++){
		assert(size_t(i+1) < args.size());
		
		if (ops[i] == '*' || ops[i] == '/' || ops[i] == '%'){
			unique_ptr<Expr> e = _binop(move(args[i]), move(args[i+1]), ops[i]);
			args[i] = move(e);
			
			ops.erase(ops.begin() + i);
			args.erase(args.begin() + i + 1);
			i--;
		}
		
	}
	
	// Merge additions
	for (long i = 0 ; i < long(ops.size()) ; i++){
		assert(size_t(i+1) < args.size());
		
		if (ops[i] == '+' || ops[i] == '-'){
			unique_ptr<Expr> e = _binop(move(args[i]), move(args[i+1]), ops[i]);
			args[i] = move(e);
			
			ops.erase(ops.begin() + i);
			args.erase(args.begin() + i + 1);
			i--;
		}
		
	}
	
	// Merge comparisons
	for (long i = 0 ; i < long(ops.size()) ; i++){
		assert(size_t(i+1) < args.size());
		// `args[0]` is cummulative expression
		unique_ptr<Expr> e = _binop(move(args[0]), move(args[i+1]), ops[i]);
		args[0] = move(e);
	}
	
	ops.clear();
	args.resize(1);
}


static const char* parse_binaryExpressionChain(const char* s, const char* end, unique_ptr<Expr>& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(!isSpace(*s));
	const char* beg = s;
	
	unique_ptr<Expr> first;
	s = parse_singleExpression(s, end, first);
	s = parse_whiteSpace(s, end);
	assert(first != nullptr);
	
	// No binary operation
	if (s == end){ no_bin:
		out = move(first);
		return s;
	}
	
	int binop = try_parse_binaryOp(s, end);
	if (binop == 0){
		goto no_bin;
	}
	
	s = parse_whiteSpace(s, end);
	if (s == end){
		throw Error(Status::INVALID_BINARY_EXPRESSION, string_view(beg, s));
	}
	
	unique_ptr<Expr> second;
	s = parse_singleExpression(s, end, second);
	assert(second != nullptr);
	
	// Lookahead for more operations
	s = parse_whiteSpace(s, end);
	if (s == end){ one_bin:
		out = _binop(move(first), move(second), binop);
		return s;
	}
	
	// Check for binop chain
	const char* mark = s;
	int binop_2 = try_parse_binaryOp(s, end);
	if (binop_2 == 0){
		goto one_bin;
	}
	
	// Chain binops
	vector<unique_ptr<Expr>> args;
	vector<int> ops;
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
		
		unique_ptr<Expr>& arg = args.emplace_back();
		s = parse_singleExpression(s, end, arg);
		
		// Look ahead for more binops
		s = parse_whiteSpace(s, end);
		if (s == end){
			break;
		}
		
		// Check for next binop
		mark = s;
		int binop = try_parse_binaryOp(s, end);
		if (binop == 0){
			break;
		}
		
		ops.push_back(binop);
	}
	
	// Build expression tree
	assert(args.size() == ops.size()+1);
	makeBinopTree(args, ops);
	assert(args.size() == 1);
	out = move(args[0]);
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


static const char* parse_funcArgs(const char* s, const char* end, Func& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(*s == '(');
	
	vector<unique_ptr<Expr>>& args = out.args;
	const char* beg = s;
	s++;
	
	while (true){
		s = parse_whiteSpace(s, end);
		
		if (s == end){ unclosed:
			throw Error(Status::UNCLOSED_EXPRESSION, string_view(beg, s));
		} else if (*s == ')'){
			return s + 1;
		} else if (*s == ','){
			if (args.empty())
				throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
			goto ws;
		} else {
			if (!args.empty())
				throw Error(Status::UNEXPECTED_SYMBOL, string_view(s, 1));
			goto no_ws;
		}
		
		ws:
		s = parse_whiteSpace(s, end);
		if (s == end){
			goto unclosed;
		}
		
		no_ws:
		s = parse_binaryExpressionChain(s, end, args.emplace_back());
	}
	
	throw Error(Status::ERROR, string_view(beg, s));
}


static const char* parse_varOrFunc(const char* s, const char* end, unique_ptr<Expr>& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(isFirstIdentifier(*s));
	
	string_view id;
	s = parse_identifier(s, end, id);
	assert(!id.empty());
	
	// Check if function
	s = parse_whiteSpace(s, end);
	if (s != end && *s == '('){
		out = make_unique<Func>();
		Func& func = static_cast<Func&>(*out);
		func.name = id;
		s = parse_funcArgs(s, end, func);
	}
	
	// Is variable
	else {
		out = make_unique<Var>();
		static_cast<Var*>(out.get())->name = id;
	}
	
	return s;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static const char* parse_singleExpression(const char* s, const char* end, unique_ptr<Expr>& out){
	assert(s != nullptr && end != nullptr && s != end);
	assert(!isSpace(*s));
	
	if (isDigit(*s)){
		return parse_num(s, end, out);
	} else if (isQuot(*s)){
		return parse_str(s, end, out);
	} else if (isFirstIdentifier(*s)){
		return parse_varOrFunc(s, end, out);
	} else if (isUnaryOp(*s)){
		return parse_unaryExpression(s, end, out);
	}
	
	else if (*s == '('){
		const char* beg = s;
		
		s = parse_whiteSpace(s + 1, end);
		s = parse_binaryExpressionChain(s, end, out);
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
	size_t ml = m.length();
	const char* ms = m.data();
	
	switch (err.status){
		case Status::UNEXPECTED_SYMBOL:
			HERE(dbg.error(m, "Unexpected symbol " P("'%.*s'") " in expression.\n", ml, ms));
			break;
		case Status::UNCLOSED_STRING:
			HERE(dbg.error(m, "Unterminated string literal in expression.\n"));
			break;
		case Status::UNCLOSED_EXPRESSION:
			HERE(dbg.error(m, "Missing closing bracket " P("'('") " in expression.\n"));
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
		case Status::MEMORY:
			HERE(dbg.error(m, "Ran out of memory when parsing expression.\n"));
			break;
		case Status::ERROR:
			HERE(dbg.error(m, "Failed to parse expression.\n"));
			break;
	}
	
}


pExpr Expression::parse(string_view str, const Debugger& dbg) noexcept {
	if (str.empty()){
		return nullptr;
	}
	
	try {
		pExpr expr;
		const char* s = str.begin();
		const char* end = str.end();
		
		s = parse_whiteSpace(s, end);
		s = parse_binaryExpressionChain(s, end, expr);
		s = parse_whiteSpace(s, end);
		
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
	
	return nullptr;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //
#ifdef DEBUG


static void _serialize(const Expr* expr, string& buff){
	auto _valstr = [&](const auto& v){
		if constexpr (IS_STR(v))
			buff.append(v);
		else
			buff.append(to_string(v));
	};

	auto __binstr = [&](const BinaryOp* b, const char* op){
		buff.push_back('(');
		_serialize(b->a.get(), buff);
		buff.append(op);
		_serialize(b->b.get(), buff);
		buff.push_back(')');
	};
	
	if (const Const* e = dynamic_cast<const Const*>(expr)){
		visit(_valstr, e->value);
	} else if (const Var* e = dynamic_cast<const Var*>(expr)){
		buff.append(e->name);
	} else if (const Not* e = dynamic_cast<const Not*>(expr)){
		buff.append("!(");
		_serialize(e->e.get(), buff);
		buff.push_back(')');
	} else if (const Neg* e = dynamic_cast<const Neg*>(expr)){
		buff.append("-(");
		_serialize(e->e.get(), buff);
		buff.push_back(')');
	} else if (const Add* e = dynamic_cast<const Add*>(expr)){
		__binstr(e, "+");
	} else if (const Sub* e = dynamic_cast<const Sub*>(expr)){
		__binstr(e, "-");
	} else if (const Mul* e = dynamic_cast<const Mul*>(expr)){
		__binstr(e, "*");
	} else if (const Div* e = dynamic_cast<const Div*>(expr)){
		__binstr(e, "/");
	} else if (const Eq* e = dynamic_cast<const Eq*>(expr)){
		__binstr(e, "==");
	} else if (const Neq* e = dynamic_cast<const Neq*>(expr)){
		__binstr(e, "!=");
	} else if (const Lt* e = dynamic_cast<const Lt*>(expr)){
		__binstr(e, "<");
	} else if (const Lte* e = dynamic_cast<const Lte*>(expr)){
		__binstr(e, "<=");
	} else if (const Gt* e = dynamic_cast<const Gt*>(expr)){
		__binstr(e, ">");
	} else if (const Gte* e = dynamic_cast<const Gte*>(expr)){
		__binstr(e, ">=");
	} else if (const Func* e = dynamic_cast<const Func*>(expr)){
		buff.append(e->name).push_back('(');
		
		for (size_t i = 0 ; i < e->args.size() ; i++){
			if (i > 0) buff.push_back(',');
			_serialize(e->args[i].get(), buff);
		}
		
		buff.push_back(')');
	} else {
		buff.append("(null)");
	}
}


string Expression::serialize(const pExpr& expr){
	string s;
	_serialize(expr.get(), s);
	return s;
}


#endif
// ------------------------------------------------------------------------------------------ //