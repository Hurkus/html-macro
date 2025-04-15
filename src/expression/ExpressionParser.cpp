#include "ExpressionParser.hpp"
#include <charconv>
#include "DEBUG.hpp"

using namespace std;
using namespace Expression;


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr bool isQuot(char c){
	return c == '"' || c == '\'' || c == '`';
}

constexpr bool isUnaryOp(char c){
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


static int parseWhiteSpace(const char* s, int i, int e) noexcept {
	assert(s != nullptr);
	
	int n = i;
	while (i < e && isspace(s[i])){
		i++;
	}
	
	return i - n;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static string_view findLine(const char* s, int pos){
	assert(s != nullptr);
	int nl = 0;
	int i = 0;
	
	while (true){
		
		if (s[i] == 0){
			if (i >= pos)
				return string_view(s + nl, s + i);
			break;
		}
		
		else if (s[i] == '\n'){
			if (i >= pos)
				return string_view(s + nl, s + i);
			nl = i+1;
		}
		
		i++;
	}
	
	return string_view(s, 0);
}


string Parser::getErrorLine(int from, int to, int ptr){
	from = max(from, 0);
	to = max(from, to);
	ptr = min(to, max(ptr, from));
	
	string_view line = findLine(s, from);
	int skip = int(line.data() - s);
	
	from = max(from - skip, 0);
	to = min(to - skip + 1, int(line.length()));
	ptr -= skip;
	
	string str;
	const size_t capacity = line.size()*2 + 32;
	str.reserve(capacity);
	
	str.append(" | ").append(line, 0, from);
	str.append(ANSI_RED).append(line, from, to - from).append(ANSI_RESET);
	str.append(line, to).push_back('\n');
	
	// Add pointer to problematic position
	str.append(" | ");
	if (0 <= ptr && ptr <= int(line.length()) + 1){
		str.append(ptr, ' ');
		str.append(ANSI_RED).append("^"sv).append(ANSI_RESET);
	}
	
	assert(str.size() <= capacity && str.capacity() <= capacity);
	return str;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


unique_ptr<Expr::Const> Parser::parseNum(){
	assert(s != nullptr && i < e);
	
	int ii = this->i;
	bool dot = false;
	
	// Read number
	if (s[ii] == '+'){
		ii++;
	} else if (s[ii] == '-'){
		ii++;
	}
	
	for (ii++ ; ii < e ; ii++){
		
		if (isdigit(s[ii])){
			continue;
		} else if (s[ii] == '.'){
			if (dot){
				ERROR_L1("Expression: Too many decimal points in number.\n%s", getErrorLine(i, ii, ii).c_str());
				return nullptr;
			}
			dot = true;
		} else {
			break;
		}
		
	}
	
	// Parse number
	unique_ptr<Expr::Const> expr = make_unique<Expr::Const>();
	
	if (dot){
		double& val = expr->value.emplace<double>();
		from_chars_result res = from_chars(s + i, s + ii, val);
		
		if (res.ec != errc()){
			ERROR_L1("Expression: Failed to parse float in expression.\n%s", getErrorLine(i, ii).c_str());
			return nullptr;
		}
		
	}
	else {
		long& val = expr->value.emplace<long>();
		from_chars_result res = from_chars(s + i, s + ii, val);
		
		if (res.ec != errc()){
			ERROR_L1("Expression: Failed to parse int in expression.\n%s", getErrorLine(i, ii).c_str());
			return nullptr;
		}
		
	}
		
	this->i = ii;
	return expr;
}


unique_ptr<Expr::Const> Parser::parseStr(){
	assert(s != nullptr && i < e);
	assert(isQuot(s[i]));
	
	int ii = this->i;
	const char quot = s[ii];
	bool escape = false;
	
	for (ii++ ; ii < e ; ii++){
		char c = s[ii];
		
		if (c == '\\'){
			escape = !escape;
		} else if (c == quot){
			if (!escape){
				ii++;
				goto success;
			}
		} else if (c != 0){
			escape = false;
		} else {
			break;
		}
		
	}
	
	// Fail
	ERROR_L1("Expression: Missing closing quote in expression.\n%s", getErrorLine(i, ii).c_str());
	return nullptr;
	
	success:
	unique_ptr<Expr::Const> expr = make_unique<Expr::Const>();
	expr->value.emplace<string>(s + i + 1, s + ii - 1);
	
	this->i = ii;
	return expr;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static unique_ptr<Expr> _unop(pExpr&& e, char op){
	switch (op){
		case '-':
			return neg(move(e));
		case '+':
			return move(e);
		case '!':
			return nott(move(e));
		default:
			assert(false);
			return nullptr;
	}
}


unique_ptr<Expr> Parser::parseUnaryExpression(){
	assert(s != nullptr && i < e);
	const int _i = i;
	
	const char op = s[i];
	if (!isUnaryOp(op)){
		ERROR_L1("Expression: Unknown unary operator.\n%s", getErrorLine(_i).c_str());
		return nullptr;
	}
	
	i++;
	i += parseWhiteSpace(s, i, e);
	if (i >= e || s[i] == 0){
		ERROR_L1("Expression: Missing unary operation argument.\n%s", getErrorLine(_i, i, i-1).c_str());
		return nullptr;
	}
	
	pExpr e = parseSingleExpression();
	if (e == nullptr){
		return nullptr;
	}
	
	return _unop(move(e), op);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static int _parseBinaryOp(const char* s, int& i, int e){
	if (i >= e){
		return 0;
	}
	
	switch (s[i]){
		case '+':
		case '-':
		case '*':
		case '/':
			i++;
			return s[i-1];
		
		case '>':
		case '<':
			i++;
			if (i < e && s[i] == '='){
				i++;
				return (s[i-2] << 8) | s[i-1];
			}
			return s[i-1];
		
		case '!':
		case '=':
			if (i+1 < e && s[i+1] == '='){
				i += 2;
				return (s[i-2] << 8) | s[i-1];
			}
			return 0;
	}
	
	return 0;
}


static unique_ptr<Expr::BinaryOp> _binop(pExpr&& a, pExpr&& b, int op){
	switch (op){
		case '+':
			return add(move(a), move(b));
		case '-':
			return sub(move(a), move(b));
		case '*':
			return mul(move(a), move(b));
		case '/':
			return div(move(a), move(b));
		
		case '==':
			return eq(move(a), move(b));
		case '!=':
			return neq(move(a), move(b));
		case '>=':
			return gte(move(a), move(b));
		case '<=':
			return lte(move(a), move(b));
		case '>':
			return gt(move(a), move(b));
		case '<':
			return lt(move(a), move(b));
		
		default:
			assert(false);
			return nullptr;
	}
}


pExpr Parser::makeTree(vector<pExpr>& args, vector<int>& ops){
	if (args.size() <= 0 || (args.size() - 1) != ops.size()){
		return nullptr;
	} else if (args.size() <= 1){
		return move(args[0]);
	}
	
	// Merge multiplications
	for (int i = 0 ; i < int(ops.size()) ; i++){
		assert(i+1 < args.size());
		
		if (ops[i] == '*' || ops[i] == '/' || ops[i] == '%'){
			pExpr e = _binop(move(args[i]), move(args[i+1]), ops[i]);
			args[i] = move(e);
			
			ops.erase(ops.begin() + i);
			args.erase(args.begin() + i + 1);
			i--;
		}
		
	}
	
	// Merge additions
	for (int i = 0 ; i < int(ops.size()) ; i++){
		assert(i+1 < args.size());
		
		if (ops[i] == '+' || ops[i] == '-'){
			pExpr e = _binop(move(args[i]), move(args[i+1]), ops[i]);
			args[i] = move(e);
			
			ops.erase(ops.begin() + i);
			args.erase(args.begin() + i + 1);
			i--;
		}
		
	}
	
	// Merge comparisons
	for (int i = 0 ; i < int(ops.size()) ; i++){
		assert(i+1 < args.size());
		// `args[0]` is cummulative expression
		pExpr e = _binop(move(args[0]), move(args[i+1]), ops[i]);
		args[0] = move(e);
	}
	
	auto res = move(args[0]);
	ops.clear();
	args.clear();
	return res;
}


pExpr Parser::parseBinopChain(){
	i += parseWhiteSpace(s, i, e);
	if (i >= e){
		return nullptr;
	}
	
	pExpr first = parseSingleExpression();
	if (first == nullptr){
		return nullptr;
	}
	
	i += parseWhiteSpace(s, i, e);
	if (i >= e){
		return first;
	}
	
	int binop = _parseBinaryOp(s, i, e);
	if (binop == 0){
		return first;
	}
	
	int _i = i;
	i += parseWhiteSpace(s, i, e);
	if (i >= e){
		ERROR_L1("Expression: Missing second operand of binary operator.\n%s", getErrorLine(_i, i, _i).c_str());
		return nullptr;
	}
	
	pExpr second = parseSingleExpression();
	if (second == nullptr){
		ERROR_L1("Expression: Missing second operand of binary operator.\n%s", getErrorLine(_i, i, _i).c_str());
		return nullptr;
	}
	
	// Lookahead for more operations
	i += parseWhiteSpace(s, i, e);
	const int binop2 = _parseBinaryOp(s, i, e);
	if (binop2 == 0){
		return _binop(move(first), move(second), binop);
	}
	
	// Chain binops
	vector<pExpr> args;
	vector<int> ops;
	args.reserve(4);
	args.emplace_back(move(first));
	args.emplace_back(move(second));
	ops.reserve(3);
	ops.push_back(binop);
	ops.push_back(binop2);
	
	// e1 + e2 + ...
	while (i < e){
		_i = i-1;	// Error pos at last char of binop symbol.
		i += parseWhiteSpace(s, i, e);
		
		pExpr arg = parseSingleExpression();
		if (arg != nullptr){
			args.emplace_back(move(arg));
		} else {
			ERROR_L1("Expression: Missing second operand of binary operator.\n%s", getErrorLine(_i, i, _i).c_str());
			return nullptr;
		}
		
		// Look ahead for more binops
		i += parseWhiteSpace(s, i, e);
		binop = _parseBinaryOp(s, i, e);
		if (binop == 0){
			break;
		}
		
		_i = i-1;
		ops.push_back(binop);
	}
	
	assert(args.size() == ops.size()+1);
	return makeTree(args, ops);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


unique_ptr<Expr::Var> Parser::parseVar(){
	assert(s != nullptr && i < e);
	int ii = this->i;
	
	// First always alpha
	if (isalpha(s[ii])){
		ii++;
	} else {
		ERROR_L1("Expression: Unexpected character in symbol name.\n%s", getErrorLine(i, ii, ii).c_str());
		return nullptr;
	}
	
	for ( ; ii < e ; ii++){
		if (!isalnum(s[ii]))
			break;
	}
	
	unique_ptr<Expr::Var> expr = make_unique<Expr::Var>();
	expr->var.assign(s + i, s + ii);
	
	this->i = ii;
	return expr;
}


unique_ptr<Expr::Func> Parser::parseFunArgs(){
	assert(s != nullptr && i < e);
	assert(s[i] == '(');
	const int _i = i;
	
	if (s[i] != '('){
		ERROR_L1("Expression: Expected function argument list bracket '('.\n%s", getErrorLine(_i, i, i).c_str());
		return nullptr;
	} else {
		i++;
	}
	
	unique_ptr<Expr::Func> expr = make_unique<Expr::Func>();
	vector<pExpr>& args = expr->args;
	
	bool first = true;
	while (i < e){
		i += parseWhiteSpace(s, i, e);
		
		if (s[i] == ')'){
			break;
		}
		
		else if (first && s[i] == ','){
			ERROR_L1("Expression: Unexpected separator ',' in argument list.\n%s", getErrorLine(_i, i, i).c_str());
			return nullptr;
		} else if (!first && s[i] != ','){
			ERROR_L1("Expression: Missing separator ',' in argument list.\n%s", getErrorLine(_i, i, i).c_str());
			return nullptr;
		} else if (s[i] == ','){
			i++;
		}
		
		args.emplace_back(parseBinopChain());
		if (args.back() == nullptr){
			ERROR_L1("Expression: Failed to parse function argument list.\n%s", getErrorLine(_i, i).c_str());
			return nullptr;
		}
		
		first = false;
	}
	
	i += parseWhiteSpace(s, i, e);
	if (i >= e || s[i] != ')'){
		ERROR_L1("Expression: Missing closing bracket ')' in function argument list.\n%s", getErrorLine(_i, i, i).c_str());
		return nullptr;
	}
	
	i++;
	return expr;
}


unique_ptr<Expr> Parser::parseVarOrFun(){
	unique_ptr<Expr::Var> name = parseVar();
	if (name == nullptr){
		return name;
	}
	
	i += parseWhiteSpace(s, i, e);
	if (i >= e){
		return name;
	}
	
	if (s[i] == '('){
		unique_ptr<Expr::Func> fun = parseFunArgs();
		if (fun == nullptr){
			return nullptr;
		}
		
		fun->name = move(name->var);
		return fun;
		
	}
	
	return name;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


pExpr Parser::parseSingleExpression(){
	assert(s != nullptr);
	assert(i >= e || !isspace(s[i]));
	
	if (i >= e){
		return nullptr;
	}
	
	if (isdigit(s[i])){
		return parseNum();
	} else if (isQuot(s[i])){
		return parseStr();
	} else if (isalpha(s[i])){
		return parseVarOrFun();
	} else if (isUnaryOp(s[i])){
		return parseUnaryExpression();
	}
	
	else if (s[i] == '('){
		int _i = i++;
		i += parseWhiteSpace(s, i, e);
		
		pExpr expr = parseBinopChain();
		if (expr == nullptr){
			ERROR_L1("Expression: Missing expression within brackets '()'.\n%s", getErrorLine(_i, i).c_str());
			return nullptr;
		}
		
		i += parseWhiteSpace(s, i, e);
		if (i >= e || s[i] != ')'){
			ERROR_L1("Expression: Missing closing bracket ')'.\n%s", getErrorLine(_i, i, i).c_str());
			return nullptr;
		}
		
		i++;
		return expr;
	}
	
	ERROR_L1("Expression: Unexpected symbol.\n%s", getErrorLine(i).c_str());
	return nullptr;
}


pExpr Parser::parse(string_view str){
	this->s = str.data();
	this->e = int(str.length());
	this->i = 0;
	
	pExpr expr = parseBinopChain();
	if (expr == nullptr || i >= e){
		return expr;
	}
	
	i += parseWhiteSpace(s, i, e);
	if (i < e){
		ERROR_L1("Expression: Unexpected symbol.\n%s", getErrorLine(i).c_str());
		return nullptr;
	}
	
	return expr;
}


// ------------------------------------------------------------------------------------------ //