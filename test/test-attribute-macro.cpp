#include "test.hpp"
using namespace std;

#define NL "\n"
#define REGISTER2(name)	REGISTER(#name, test_##name)


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(attr_macro_CALL_1);
Result test_attr_macro_CALL_1(){
	TmpFile in = TmpFile("attr_macro_CALL_1.html",
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
		"/tmp/html-macro-test/attr_macro_CALL_1.html:4:4: error: Attribute `CALL` missing value." NL
		"    4 | <p CALL></p>" NL
		"      |    ^~~~" NL
		"/tmp/html-macro-test/attr_macro_CALL_1.html:5:10: error: Attribute `CALL` missing value." NL
		"    5 | <p CALL=\"\"></p>" NL
		"      |         ^~" NL
	);
	return run({in}, out, err, 0);
}


REGISTER2(attr_macro_CALL_2);
Result test_attr_macro_CALL_2(){
	TmpFile in = TmpFile("attr_macro_CALL_2.html",
		R"(
			<MACRO NAME="f">
				<SET x='x+2'/>
				<p>x1 is {x}</p>
			</MACRO>
			<SET x='10'/>
			<div CALL="f">
				<p>x2 is {x}</p>
			</div>
		)"
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
Result test_attr_macro_CALL_3(){
	TmpFile in = TmpFile("attr_macro_CALL_3.html",
		R"(
			<MACRO NAME="f" x>
				<SET x='x+2'/>
				<p>x1 is {x}</p>
			</MACRO>
			<SET x='10'/>
			<div CALL="f">
				<p>x2 is {x}</p>
			</div>
		)"
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
Result test_attr_macro_CALL_4(){
	TmpFile in = TmpFile("attr_macro_CALL_4.html",
		R"(
			<MACRO NAME="f" x='100'>
				<SET x='x+2'/>
				<p>x1 is {x}</p>
			</MACRO>
			<SET x='10'/>
			<div CALL="f">
				<p>x2 is {x}</p>
			</div>
		)"
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