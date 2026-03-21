#include "test.hpp"
using namespace std;

#define NL "\n"
#define REGISTER2(name)	REGISTER(#name, test_##name)


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(element_macro_MACRO_name);
Result test_element_macro_MACRO_name(){
	TmpFile in = TmpFile("element_macro_MACRO_name.html",
		"<MACRO/>" NL
		"<MACRO NAME/>" NL
		"<MACRO NAME=''/>" NL
		"<MACRO NAME='hello'/>" NL
		"<MACRO NAME=\"\"/>" NL
		"<MACRO NAME=\"hello\"/>" NL
	);
	string_view err = (
		"/tmp/html-macro-test/element_macro_MACRO_name.html:1:2: warn: Element <MACRO> missing attribute `NAME`." NL
		"    1 | <MACRO/>" NL
		"      |  ^~~~~" NL
		"/tmp/html-macro-test/element_macro_MACRO_name.html:2:8: error: Attribute `NAME` missing value." NL
		"    2 | <MACRO NAME/>" NL
		"      |        ^~~~" NL
		"/tmp/html-macro-test/element_macro_MACRO_name.html:3:14: error: Attribute `NAME` missing value." NL
		"    3 | <MACRO NAME=''/>" NL
		"      |             ^~" NL
		"/tmp/html-macro-test/element_macro_MACRO_name.html:4:14: warn: Attribute `NAME` expected double quotes (\"\"). Value is always interpreted as a string." NL
		"    4 | <MACRO NAME='hello'/>" NL
		"      |             ^~~~~~~" NL
		"/tmp/html-macro-test/element_macro_MACRO_name.html:5:14: error: Attribute `NAME` missing value." NL
		"    5 | <MACRO NAME=\"\"/>" NL
		"      |             ^~" NL
	);
	return run({in}, "", err, 0);
}


REGISTER2(element_macro_MACRO_customElement);
Result test_element_macro_MACRO_customElement(){
	TmpFile in = TmpFile("element_macro_MACRO_customElement.html",
		R"(
			<MACRO NAME="FUNC" x>
				<SET x='x+2' y='y+2'/>
				<p>A: (x,y) is ({x}, {y})</p>
			</MACRO>
			
			<SET x='10'/>
			<FUNC y='20'/>
		)"
	);
	string_view out = (
		NL
		"<p>A: (x,y) is (12, 22)</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(element_macro_MACRO_customAttribute);
Result test_element_macro_MACRO_customAttribute(){
	TmpFile in = TmpFile("element_macro_MACRO_customAttribute.html",
		R"(
			<MACRO NAME="FUNC">
				<SET-ATTR href='VALUE'/>
				<SET-ATTR class="link"/>
			</MACRO>
			
			<a FUNC="https://github.com/Hurkus/html-macro/">repo</a>
		)"
	);
	string_view out = (
		NL
		"<a href=\"https://github.com/Hurkus/html-macro/\" class=\"link\">repo</a>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(element_macro_CALL_name);
Result test_element_macro_CALL_name(){
	TmpFile in = TmpFile("element_macro_CALL_name.html",
		"<MACRO NAME=\"g\"/>" NL
		"<CALL/>" NL
		"<CALL NAME/>" NL
		"<CALL NAME=\"\"/>" NL
		"<CALL NAME=\"f\"/>" NL
		"<CALL NAME=\"g\"/>" NL
	);
	string_view err = string_view(
		"/tmp/html-macro-test/element_macro_CALL_name.html:2:2: error: Element <CALL> missing attribute `NAME`." NL
		"    2 | <CALL/>" NL
		"      |  ^~~~" NL
		"/tmp/html-macro-test/element_macro_CALL_name.html:3:7: error: Attribute `NAME` missing value." NL
		"    3 | <CALL NAME/>" NL
		"      |       ^~~~" NL
		"/tmp/html-macro-test/element_macro_CALL_name.html:4:13: error: Attribute `NAME` missing value." NL
		"    4 | <CALL NAME=\"\"/>" NL
		"      |            ^~" NL
		"/tmp/html-macro-test/element_macro_CALL_name.html:5:13: error: Macro `f` not found." NL
		"    5 | <CALL NAME=\"f\"/>" NL
		"      |             ^" NL
		""
	);
	return run({in}, "", err, 0);
}


REGISTER2(element_macro_CALL_name_expr);
Result test_element_macro_CALL_name_expr(){
	TmpFile in = TmpFile("element_macro_CALL_name_expr.html",
		R"(
			<SET s="f"/>
			<MACRO NAME="func1">a</MACRO>
			<MACRO NAME="func2">b</MACRO>
			<CALL NAME='s + "unc" + 1'/>
			<CALL NAME='s + "unc" + 2'/>
		)"
	);
	string_view out = (
		"ab"
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(element_macro_parameters_1);
Result test_element_macro_parameters_1(){
	TmpFile in = TmpFile("element_macro_parameters-1.html",
		R"(
			<MACRO NAME="f">{x}</MACRO>
			<SET x='10'/>
			<CALL NAME="f"/>
			{x}
		)"
	);
	string_view out = (
		"10" NL
		"10" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(element_macro_parameters_2);
Result test_element_macro_parameters_2(){
	TmpFile in = TmpFile("element_macro_parameters-2.html",
		R"(
			<MACRO NAME="f">{x}</MACRO>
			<SET x='10'/>
			<CALL NAME="f" x='20'/>
			{x}
		)"
	);
	string_view out = (
		"20" NL
		"10" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(element_macro_parameters_3);
Result test_element_macro_parameters_3(){
	TmpFile in = TmpFile("element_macro_parameters-3.html",
		R"(
			<MACRO NAME="f">
				<p>{x}</p>
				<SET x='20'/>
			</MACRO>
			
			<SET x='10'/>
			<CALL NAME="f"/>
			<p>{x}</p>
		)"
	);
	string_view out = (
		NL
		"<p>10</p>" NL
		"<p>20</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(element_macro_parameters_4);
Result test_element_macro_parameters_4(){
	TmpFile in = TmpFile("element_macro_parameters-4.html",
		R"(
			<MACRO NAME="f">
				<p>{x}</p>
				<SET x='20'/>
			</MACRO>
			
			<SET x='10'/>
			<CALL NAME="f" x='30'/>
			<p>{x}</p>
		)"
	);
	string_view out = (
		NL
		"<p>30</p>" NL
		"<p>10</p>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(element_macro_INCLUDE_wrap);
Result test_element_macro_INCLUDE_wrap(){
	TmpFile txt = TmpFile("element_macro_INCLUDE_wrap-text.txt",
		"Lorem ipsum, etc."
	);
	TmpFile css = TmpFile("element_macro_INCLUDE_wrap-style.css",
		"div { color: red; }"
	);
	TmpFile js = TmpFile("element_macro_INCLUDE_wrap-script.js",
		"function f(){ return 10; }"
	);
	TmpFile in = TmpFile("element_macro_INCLUDE_wrap.html",
		"<INCLUDE SRC=\"element_macro_INCLUDE_wrap-text.txt\"/>" NL
		"<INCLUDE SRC=\"element_macro_INCLUDE_wrap-style.css\"/>" NL
		"<INCLUDE SRC=\"element_macro_INCLUDE_wrap-script.js\"/>" NL
	);
	string_view out = (
		"Lorem ipsum, etc." NL
		"<style>div { color: red; }</style>" NL
		"<script>function f(){ return 10; }</script>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(element_macro_INCLUDE_nowrap);
Result test_element_macro_INCLUDE_nowrap(){
	TmpFile txt = TmpFile("element_macro_INCLUDE_nowrap-text.txt",
		"Lorem ipsum, etc." NL
	);
	TmpFile css = TmpFile("element_macro_INCLUDE_nowrap-style.css",
		"div { color: red; }"
	);
	TmpFile js = TmpFile("element_macro_INCLUDE_nowrap-script.js",
		"function f(){ return 10; }" NL
	);
	TmpFile in = TmpFile("element_macro_INCLUDE_nowrap.html",
		"<INCLUDE NO-WRAP SRC=\"element_macro_INCLUDE_nowrap-text.txt\"/>" NL
		"<INCLUDE NO-WRAP SRC=\"element_macro_INCLUDE_nowrap-style.css\"/>" NL
		"<INCLUDE NO-WRAP SRC=\"element_macro_INCLUDE_nowrap-script.js\"/>" NL
	);
	string_view out = (
		"Lorem ipsum, etc." NL
		"div { color: red; }" NL
		"function f(){ return 10; }" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(element_macro_INCLUDE_header);
Result test_element_macro_INCLUDE_header(){
	TmpFile txt = TmpFile("element_macro_INCLUDE_header-othr.html",
		"<MACRO NAME=\"title\">" NL
		"	<h1>Title</h1>" NL
		"</MACRO>" NL
		"" NL
		"<p>Lorem ipsum, etc.</p>" NL
	);
	TmpFile in = TmpFile("element_macro_INCLUDE_header.html",
		"<body>" NL
		"	<INCLUDE HEADER SRC=\"element_macro_INCLUDE_header-othr.html\"/>" NL
		"	<CALL NAME=\"title\"/>" NL
		"	<INCLUDE SRC=\"element_macro_INCLUDE_header-othr.html\"/>" NL
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


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(element_macro_IF);
Result test_element_macro_IF(){
	TmpFile in = TmpFile("element_macro_IF.html",
		R"(
			<SET lang="en"/>
			<IF lang="en">
				<p>ENGLISH</p>
			</IF>
			<ELIF lang="de">
				<p>GERMAN</p>
			</ELIF>
			<ELSE>
				<p>NOTHING</p>
			</ELSE>
		)"
	);
	string_view out = (
		NL
		"<p>ENGLISH</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(element_macro_ELIF);
Result test_element_macro_ELIF(){
	TmpFile in = TmpFile("element_macro_ELIF.html",
		R"(
			<SET lang="de"/>
			<IF lang="en">
				<p>ENGLISH</p>
			</IF>
			<ELIF lang="de">
				<p>GERMAN</p>
			</ELIF>
			<ELSE>
				<p>NOTHING</p>
			</ELSE>
		)"
	);
	string_view out = (
		NL
		"<p>GERMAN</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(element_macro_ELSE);
Result test_element_macro_ELSE(){
	TmpFile in = TmpFile("element_macro_ELSE.html",
		R"(
			<SET lang="fr"/>
			<IF lang="en">
				<p>ENGLISH</p>
			</IF>
			<ELIF lang="de">
				<p>GERMAN</p>
			</ELIF>
			<ELSE>
				<p>NOTHING</p>
			</ELSE>
		)"
	);
	string_view out = (
		NL
		"<p>NOTHING</p>" NL
	);
	return run({in}, out, "", 0);
}


// ------------------------------------------------------------------------------------------ //