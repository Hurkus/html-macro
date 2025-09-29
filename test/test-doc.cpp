#include "test.hpp"
using namespace std;

#define NL "\n"


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER("doc_expressions_functions", test_doc_expressions_functions);
bool test_doc_expressions_functions(){
	TmpFile in = TmpFile(
		"<p>{int('7.234')}</p>" NL
		"<p>{float('122.5') + 1}</p>" NL
		"<p>{str(12) + ' apples'}</p>" NL
		"" NL
		"<p>{len('hello')}</p>" NL
		"<p>{abs(-4.5)}</p>" NL
		"<p>{min(1, 2.5, 'three')}</p>" NL
		"<p>{max(1, 2.5, 'three')}</p>" NL
		"" NL
		"<p>{lower('World!')}</p>" NL
		"<p>{upper('World!')}</p>" NL
		"<p>{match('01:20:13', '\\d?\\d:\\d\\d:\\d\\d')}</p>" NL
		"<p>{replace('12 apples', '(\\d+)\\s+(\\w+)', '$1 green $2')}</p>" NL
		"" NL
		"<p>x is {if(defined(x),x,100)}</p>" NL
	);
	string out = (
		"<p>7</p>" NL
		"<p>123.5</p>" NL
		"<p>12 apples</p>" NL
		"<p>5</p>" NL
		"<p>4.5</p>" NL
		"<p>1</p>" NL
		"<p>three</p>" NL
		"<p>world!</p>" NL
		"<p>WORLD!</p>" NL
		"<p>1</p>" NL
		"<p>12 green apples</p>" NL
		"<p>x is 100</p>" NL
	);
	string err = "";
	return run({in}, out, err, 0);
}


// ------------------------------------------------------------------------------------------ //