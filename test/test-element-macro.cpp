#include "test.hpp"
using namespace std;

#define NL "\n"
#define REGISTER2(name)	REGISTER(#name, test_##name)


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(macro_MACRO_name);
bool test_macro_MACRO_name(){
	TmpFile in = TmpFile(
		"<MACRO/>" NL
		"<MACRO NAME/>" NL
		"<MACRO NAME=''/>" NL
		"<MACRO NAME='hello'/>" NL
		"<MACRO NAME=\"\"/>" NL
		"<MACRO NAME=\"hello\"/>" NL
	);
	string_view err = (
		"/tmp/html-macro-test/file.html:1:2: warn: Tag <MACRO> missing attribute `NAME`." NL
		"    1 | <MACRO/>" NL
		"      |  ^~~~~" NL
		"/tmp/html-macro-test/file.html:2:8: error: Attribute `NAME` missing value." NL
		"    2 | <MACRO NAME/>" NL
		"      |        ^~~~" NL
		"/tmp/html-macro-test/file.html:3:14: error: Attribute `NAME` missing value." NL
		"    3 | <MACRO NAME=''/>" NL
		"      |             ^~" NL
		"/tmp/html-macro-test/file.html:4:14: warn: Attribute `NAME` expected double quotes (\"\"). Value is always interpreted as a string." NL
		"    4 | <MACRO NAME='hello'/>" NL
		"      |             ^~~~~~~" NL
		"/tmp/html-macro-test/file.html:5:14: error: Attribute `NAME` missing value." NL
		"    5 | <MACRO NAME=\"\"/>" NL
		"      |             ^~" NL
	);
	return run({in}, "", err, 0);
}


REGISTER2(macro_MACRO_customElement);
bool test_macro_MACRO_customElement(){
	TmpFile in = TmpFile(
		"<MACRO NAME=\"FUNC\" x>" NL
		"	<SET x='x+2' y='y+2'/>" NL
		"	<p>A: (x,y) is ({x}, {y})</p>" NL
		"</MACRO>" NL
		"" NL
		"<SET x='10'/>" NL
		"<FUNC y='20'/>" NL
	);
	string_view out = (
		NL
		"<p>A: (x,y) is (12, 22)</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(macro_MACRO_customAttribute);
bool test_macro_MACRO_customAttribute(){
	TmpFile in = TmpFile(
		"<MACRO NAME=\"FUNC\">" NL
		"	<SET-ATTR href='VALUE'/>" NL
		"	<SET-ATTR class=\"link\"/>" NL
		"</MACRO>" NL
		"" NL
		"<a FUNC=\"https://github.com/Hurkus/html-macro/\">repo</a>" NL
	);
	string_view out = (
		NL
		"<a href=\"https://github.com/Hurkus/html-macro/\" class=\"link\">repo</a>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(macro_CALL_name);
bool test_macro_CALL_name(){
	TmpFile in = TmpFile(
		"<MACRO NAME=\"g\"/>" NL
		"<CALL/>" NL
		"<CALL NAME/>" NL
		"<CALL NAME=\"\"/>" NL
		"<CALL NAME=\"f\"/>" NL
		"<CALL NAME=\"g\"/>" NL
	);
	string_view err = string_view(
		"/tmp/html-macro-test/file.html:2:2: error: Tag <CALL> missing attribute `NAME`." NL
		"    2 | <CALL/>" NL
		"      |  ^~~~" NL
		"/tmp/html-macro-test/file.html:3:7: error: Attribute `NAME` missing value." NL
		"    3 | <CALL NAME/>" NL
		"      |       ^~~~" NL
		"/tmp/html-macro-test/file.html:4:13: error: Attribute `NAME` missing value." NL
		"    4 | <CALL NAME=\"\"/>" NL
		"      |            ^~" NL
		"/tmp/html-macro-test/file.html:5:13: error: Macro `f` not found." NL
		"    5 | <CALL NAME=\"f\"/>" NL
		"      |             ^" NL
		""
	);
	return run({in}, "", err, 0);
}


REGISTER2(macro_CALL_name_expression);
bool test_macro_CALL_name_expression(){
	TmpFile in = TmpFile(
		"<SET s=\"f\"/>" NL
		"<MACRO NAME=\"func1\">a</MACRO>" NL
		"<MACRO NAME=\"func2\">b</MACRO>" NL
		"<CALL NAME='s + \"unc\" + 1'/>" NL
		"<CALL NAME='s + \"unc\" + 2'/>" NL
	);
	string_view out = (
		"ab"
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(macro_parameters_1);
bool test_macro_parameters_1(){
	TmpFile in = TmpFile(
		"<MACRO NAME=\"f\">{x}</MACRO>" NL
		"<SET x='10'/>" NL
		"<CALL NAME=\"f\"/>" NL
		"{x}" NL
	);
	string_view out = (
		"10" NL
		"10" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(macro_parameters_2);
bool test_macro_parameters_2(){
	TmpFile in = TmpFile(
		"<MACRO NAME=\"f\">{x}</MACRO>" NL
		"<SET x='10'/>" NL
		"<CALL NAME=\"f\" x='20'/>" NL
		"{x}" NL
	);
	string_view out = (
		"20" NL
		"10" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(macro_parameters_3);
bool test_macro_parameters_3(){
	TmpFile in = TmpFile(
		"<MACRO NAME=\"f\">" NL
		"	<p>{x}</p>" NL
		"	<SET x='20'/>" NL
		"</MACRO>" NL
		"" NL
		"<SET x='10'/>" NL
		"<CALL NAME=\"f\"/>" NL
		"<p>{x}</p>" NL
	);
	string_view out = (
		NL
		"<p>10</p>" NL
		"<p>20</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(macro_parameters_4);
bool test_macro_parameters_4(){
	TmpFile in = TmpFile(
		"<MACRO NAME=\"f\">" NL
		"	<p>{x}</p>" NL
		"	<SET x='20'/>" NL
		"</MACRO>" NL
		"" NL
		"<SET x='10'/>" NL
		"<CALL NAME=\"f\" x='30'/>" NL
		"<p>{x}</p>" NL
	);
	string_view out = (
		NL
		"<p>30</p>" NL
		"<p>10</p>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(macro_INCLUDE_wrap);
bool test_macro_INCLUDE_wrap(){
	TmpFile txt = TmpFile("text.txt",
		"Lorem ipsum, etc."
	);
	TmpFile css = TmpFile("style.css",
		"div { color: red; }"
	);
	TmpFile js = TmpFile("script.js",
		"function f(){ return 10; }"
	);
	TmpFile in = TmpFile(
		"<INCLUDE SRC=\"text.txt\"/>" NL
		"<INCLUDE SRC=\"style.css\"/>" NL
		"<INCLUDE SRC=\"script.js\"/>" NL
	);
	string_view out = (
		"Lorem ipsum, etc." NL
		"<style>div { color: red; }</style>" NL
		"<script>function f(){ return 10; }</script>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(macro_INCLUDE_nowrap);
bool test_macro_INCLUDE_nowrap(){
	TmpFile txt = TmpFile("text.txt",
		"Lorem ipsum, etc." NL
	);
	TmpFile css = TmpFile("style.css",
		"div { color: red; }"
	);
	TmpFile js = TmpFile("script.js",
		"function f(){ return 10; }" NL
	);
	TmpFile in = TmpFile(
		"<INCLUDE NO-WRAP SRC=\"text.txt\"/>" NL
		"<INCLUDE NO-WRAP SRC=\"style.css\"/>" NL
		"<INCLUDE NO-WRAP SRC=\"script.js\"/>" NL
	);
	string_view out = (
		"Lorem ipsum, etc." NL
		"div { color: red; }" NL
		"function f(){ return 10; }" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(macro_INCLUDE_header);
bool test_macro_INCLUDE_header(){
	TmpFile txt = TmpFile("other.html",
		"<MACRO NAME=\"title\">" NL
		"	<h1>Title</h1>" NL
		"</MACRO>" NL
		"" NL
		"<p>Lorem ipsum, etc.</p>" NL
	);
	TmpFile in = TmpFile(
		"<body>" NL
		"	<INCLUDE HEADER SRC=\"other.html\"/>" NL
		"	<CALL NAME=\"title\"/>" NL
		"	<INCLUDE SRC=\"other.html\"/>" NL
		"</body>" NL
	);
	string_view out = (
		"<body>" NL
		"	<h1>Title</h1>" NL
		"	<p>Lorem ipsum, etc.</p>" NL
		"</body>" NL
	);
	return run({in}, out, "", 0);
}


// ------------------------------------------------------------------------------------------ //