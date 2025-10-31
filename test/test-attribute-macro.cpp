#include "test.hpp"
using namespace std;

#define NL "\n"
#define REGISTER2(name)	REGISTER(#name, test_##name)


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(attr_macro_CALL_1);
bool test_attr_macro_CALL_1(){
	TmpFile in = TmpFile(
		"<SET s=\"f\"/>" NL
		"<MACRO NAME=\"func1\">a</MACRO>" NL
		"<MACRO NAME=\"func2\">b</MACRO>" NL
		"<p CALL></p>" NL
		"<p CALL=\"\"></p>" NL
		"<p CALL='s + \"unc\" + 1'></p>" NL
		"<p CALL='s + \"unc\" + 2'></p>" NL
	);
	string_view out = (
		NL
		"<p></p>" NL
		"<p></p>" NL
		"<p>a</p>" NL
		"<p>b</p>" NL
	);
	string_view err = (
		"/tmp/html-macro-test/file.html:4:4: error: Attribute `CALL` missing value." NL
		"    4 | <p CALL></p>" NL
		"      |    ^~~~" NL
		"/tmp/html-macro-test/file.html:5:10: error: Attribute `CALL` missing value." NL
		"    5 | <p CALL=\"\"></p>" NL
		"      |         ^~" NL
	);
	return run({in}, out, err, 0);
}


REGISTER2(attr_macro_CALL_2);
bool test_attr_macro_CALL_2(){
	TmpFile in = TmpFile(
		"<MACRO NAME=\"f\">" NL
		"	<SET x='x+2'/>" NL
		"	<p>x1 is {x}</p>" NL
		"</MACRO>" NL
		"<SET x='10'/>" NL
		"<div CALL=\"f\">" NL
		"	<p>x2 is {x}</p>" NL
		"</div>" NL
	);
	string_view out = (
		NL
		"<div>" NL
		"	<p>x1 is 12</p>" NL
		"	<p>x2 is 12</p>" NL
		"</div>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(attr_macro_CALL_3);
bool test_attr_macro_CALL_3(){
	TmpFile in = TmpFile(
		"<MACRO NAME=\"f\" x>" NL
		"	<SET x='x+2'/>" NL
		"	<p>x1 is {x}</p>" NL
		"</MACRO>" NL
		"<SET x='10'/>" NL
		"<div CALL=\"f\">" NL
		"	<p>x2 is {x}</p>" NL
		"</div>" NL
	);
	string_view out = (
		NL
		"<div>" NL
		"	<p>x1 is 12</p>" NL
		"	<p>x2 is 10</p>" NL
		"</div>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(attr_macro_CALL_4);
bool test_attr_macro_CALL_4(){
	TmpFile in = TmpFile(
		"<MACRO NAME=\"f\" x='100'>" NL
		"	<SET x='x+2'/>" NL
		"	<p>x1 is {x}</p>" NL
		"</MACRO>" NL
		"<SET x='10'/>" NL
		"<div CALL=\"f\">" NL
		"	<p>x2 is {x}</p>" NL
		"</div>" NL
	);
	string_view out = (
		NL
		"<div>" NL
		"	<p>x1 is 102</p>" NL
		"	<p>x2 is 10</p>" NL
		"</div>" NL
	);
	return run({in}, out, "", 0);
}


// ------------------------------------------------------------------------------------------ //