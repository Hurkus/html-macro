#include "test.hpp"
using namespace std;

#define NL "\n"
#define REGISTER2(name)	REGISTER(#name, test_##name)


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_html_whitespace_1);
bool test_doc_html_whitespace_1(){
	TmpFile in = TmpFile(
		"<p>Here<u> is some </u><span>text</span>.</p>" NL
		"" NL
		"<p>" NL
		"	Long text," NL
		"	long text" NL
		"</p>" NL
		"" NL
		"<div>" NL
		"<p>Weird input" NL
		"</p>" NL
		"</div>" NL
	);
	string_view out = (
		"<p>Here<u> is some </u><span>text</span>.</p>" NL
		"<p>" NL
		"	Long text," NL
		"	long text" NL
		"</p>" NL
		"<div>" NL
		"	<p>Weird input" NL
		"	</p>" NL
		"</div>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_html_whitespace_2);
bool test_doc_html_whitespace_2(){
	TmpFile in = TmpFile(
		"<head>" NL
		"<style>" NL
		"	body {" NL
		"		color: red;" NL
		"	}" NL
		"</style>" NL
		"</head>" NL
	);
	string_view out = (
		"<head>" NL
		"	<style>" NL
		"		body {" NL
		"			color: red;" NL
		"		}" NL
		"	</style>" NL
		"</head>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_element_macros);
bool test_doc_element_macros(){
	TmpFile in = TmpFile(
		"<div>" NL
		"	<IF lang=\"de\">" NL
		"		<p>Deutsch</p>" NL
		"	</IF>" NL
		"	<ELSE>" NL
		"		<p>English</p>" NL
		"	</ELSE>" NL
		"</div>" NL
	);
	string_view out = (
		"<div>" NL
		"	<p>English</p>" NL
		"</div>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_expression_context);
bool test_doc_expression_context(){
	TmpFile in = TmpFile(
		"<p attr=\"text {1 + 2}\"></p>" NL
		"<p attr='2 * 2'></p>" NL
		"<p>There are {9.0 / 2} apples.</p>" NL
	);
	string_view out = (
		"<p attr=\"text 3\"></p>" NL
		"<p attr=\"4\"></p>" NL
		"<p>There are 4.5 apples.</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_expression_variables);
bool test_doc_expression_variables(){
	TmpFile in = TmpFile(
		"<SET s=\"header\"/>" NL
		"<p class='s'>My title</p>" NL
		"" NL
		"<SET x='2' y='3'/>" NL
		"<p>My text number {x * (y + 1)}</p>" NL
	);
	string_view out = (
		NL
		"<p class=\"header\">My title</p>" NL
		"<p>My text number 8</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_expression_operators);
bool test_doc_expression_operators(){
	TmpFile in = TmpFile(
		"{\"x is \" + 3}" NL
		"{\"a\" * 3}" NL
		"{\"abc\" - 2}" NL
		"{\"a\" || \"\"}" NL
		"{!10}" NL
		"{3.6 % 1.25}" NL
	);
	string_view out = (
		"x is 3" NL
		"aaa" NL
		"a" NL
		"1" NL
		"0" NL
		"1.1" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_expressions_functions);
bool test_doc_expressions_functions(){
	TmpFile in = TmpFile(
		"{int('7.234')}" NL
		"{float('122.5') + 1}" NL
		"{str(12) + ' apples'}" NL
		NL
		"{len('hello')}" NL
		"{abs(-4.5)}" NL
		"{min(1, 2.5, 'three')}" NL
		"{max(1, 2.5, 'three')}" NL
		NL
		"{lower('World!')}" NL
		"{upper('World!')}" NL
		"{substr(\"green\", 1)}" NL
		"{substr(\"green\", 1, 3)}" NL
		"{substr(\"green\", 4, -3)}" NL
		"{substr(\"green\", -1, -3)}" NL
		NL
		"{match('01:20:13', '\\d?\\d:\\d\\d:\\d\\d')}" NL
		"{replace('12 apples', '(\\d+)\\s+(\\w+)', '$1 green $2')}" NL
		NL
		"{if(3 > 2, \"yes\", \"no\")}" NL
		"{if(defined(x),x,100)}" NL
	);
	string_view out = (
		"7" NL
		"123.5" NL
		"12 apples" NL
		NL
		"5" NL
		"4.5" NL
		"1" NL
		"three" NL
		NL
		"world!" NL
		"WORLD!" NL
		"reen" NL
		"ree" NL
		"ree" NL
		"ree" NL
		NL
		"1" NL
		"12 green apples" NL
		NL
		"yes" NL
		"100" NL
	);
	return run({in}, out, "", 0);
}


// ------------------------------------------------------------------------------------------ //