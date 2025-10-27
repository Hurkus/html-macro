#include "test.hpp"
using namespace std;

#define NL "\n"
#define REGISTER2(name)	REGISTER(#name, test_##name)


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_macro_MACRO);
bool test_doc_macro_MACRO(){
	TmpFile in = TmpFile(
		"<ul>" NL
		"	<FOR i='0' TRUE='i<3' i='i+1' >" NL
		"		<CALL NAME=\"func\"/>" NL
		"	</FOR>" NL
		"</ul>" NL
		NL
		"<MACRO NAME=\"func\">" NL
		"<SET col='if(i == 1, \"red\", \"green\")' />" NL
		"<li style=\"color:{col};\">Line #{i}</li>" NL
		"</MACRO>" NL
	);
	string_view out = (
		"<ul>" NL
		"	<li style=\"color:green;\">Line #0</li>" NL
		"	<li style=\"color:red;\">Line #1</li>" NL
		"	<li style=\"color:green;\">Line #2</li>" NL
		"</ul>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_INCLUDE);
bool test_doc_macro_INCLUDE(){
	TmpFile in = TmpFile(
		"<SET name=\"cookbook\"/>" NL
		"<body>" NL
		"	<INCLUDE SRC=\"header.html\"/>" NL
		"	<p>Amazing content!</p>" NL
		"	<INCLUDE SRC=\"footer.html\"/>" NL
		"</body>" NL
	);
	
	TmpFile header = TmpFile("header.html",
		"<style>" NL
		"	#header {" NL
		"		color: red;" NL
		"	}" NL
		"</style>" NL
		"<div id=\"header\">" NL
		"	<h1>Yet another {name}</h1>" NL
		"</div>" NL
	);
	
	TmpFile footer = TmpFile("footer.html",
		"<div id=\"footer\">" NL
		"	<p>Contacts: none</p>" NL
		"	<p>Last edited on: 14/10/2025</p>" NL
		"</div>" NL
	);
	
	string_view out = (
		NL
		"<body><style>" NL
		"	#header {" NL
		"		color: red;" NL
		"	}" NL
		"</style>" NL
		"	<div id=\"header\">" NL
		"		<h1>Yet another cookbook</h1>" NL
		"	</div>" NL
		"	<p>Amazing content!</p>" NL
		"	<div id=\"footer\">" NL
		"		<p>Contacts: none</p>" NL
		"		<p>Last edited on: 14/10/2025</p>" NL
		"	</div>" NL
		"</body>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_macro_SET);
bool test_doc_macro_SET(){
	TmpFile in = TmpFile(
		"<SET x='4' y='6.5' s=\"apples\"/>" NL
		"<p>I have {int(x*y)} bushels of {s}</p> " NL
	);
	string_view out = (
		NL
		"<p>I have 26 bushels of apples</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_IF);
bool test_doc_macro_IF(){
	TmpFile in1 = TmpFile(
		"<IF TRUE='6>3'>" NL
		"	<p>6 is greater than 3</p>" NL
		"</IF>" NL
	);
	string_view out1 = ( NL
		"<p>6 is greater than 3</p>" NL
	);
	if (!run({in1}, out1, "", 0)){
		return false;
	}
	
	TmpFile in2 = TmpFile(
		"<SET lang=\"en\"/>" NL
		"<IF lang=\"en\">" NL
		"	<p>English</p>" NL
		"</IF>" NL
	);
	string_view out2 = ( NL
		"<p>English</p>" NL
	);
	return run({in2}, out2, "", 0);
}


REGISTER2(doc_macro_ELIF);
bool test_doc_macro_ELIF(){
	TmpFile in = TmpFile(
		"<SET lang=\"en\"/>" NL
		"" NL
		"<IF lang=\"de\">" NL
		"	<p>Deutsch</p>" NL
		"</IF>" NL
		"<ELIF lang=\"en\">" NL
		"	<p>English</p>" NL
		"</ELIF>" NL
	);
	string_view out = (
		NL
		"<p>English</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_ELSE);
bool test_doc_macro_ELSE(){
	TmpFile in = TmpFile(
		"<SET lang=\"en\"/>" NL
		"" NL
		"<IF lang=\"de\">" NL
		"	<p>Deutsch</p>" NL
		"</IF>" NL
		"<ELSE>" NL
		"	<p>English</p>" NL
		"</ELSE>" NL
	);
	string_view out = (
		NL
		"<p>English</p>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_macro_FOR);
bool test_doc_macro_FOR(){
	TmpFile in = TmpFile(
		"<FOR i='0' TRUE='i < 3' i='i+1'>" NL
		"	<p>{i}</p>" NL
		"</FOR>" NL
	);
	string_view out = ( NL
		"<p>0</p>" NL
		"<p>1</p>" NL
		"<p>2</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_WHILE);
bool test_doc_macro_WHILE(){
	TmpFile in = TmpFile(
		"<SET i='0'/>" NL
		"<WHILE TRUE='i < 3'>" NL
		"	<p>{i}</p>" NL
		"	<SET i='i+1'/>" NL
		"</WHILE>" NL
	);
	string_view out = ( NL
		"<p>0</p>" NL
		"<p>1</p>" NL
		"<p>2</p>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_macro_SET_TAG);
bool test_doc_macro_SET_TAG(){
	TmpFile in = TmpFile(
		"<div>" NL
		"    <SET-TAG p/>" NL
		"    text" NL
		"</div>" NL
	);
	string_view out = (
		"<p>" NL
		"    text" NL
		"</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_GET_TAG);
bool test_doc_macro_GET_TAG(){
	TmpFile in = TmpFile(
		"<div>" NL
		"    <GET-TAG tag/>" NL
		"    This text is within a {tag} element." NL
		"</div>" NL
	);
	string_view out = (
		"<div>" NL
		"    This text is within a div element." NL
		"</div>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_SET_ATTR);
bool test_doc_macro_SET_ATTR(){
	TmpFile in = TmpFile(
		"<div>" NL
		"    <SET-ATTR title=\"text\"/>" NL
		"    text" NL
		"</div>" NL
	);
	string_view out = (
		"<div title=\"text\">" NL
		"    text" NL
		"</div>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_GET_ATTR);
bool test_doc_macro_GET_ATTR(){
	TmpFile in = TmpFile(
		"<div id=\"box\">" NL
		"    <GET-ATTR x=\"id\"/>" NL
		"    <SET-ATTR class='x'/>" NL
		"</div>" NL
	);
	string_view out = (
		"<div id=\"box\" class=\"box\"></div>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_DEL_ATTR);
bool test_doc_macro_DEL_ATTR(){
	TmpFile in = TmpFile(
		"<div id=\"box\">" NL
		"    <DEL-ATTR id/>" NL
		"</div>" NL
	);
	string_view out = (
		"<div></div>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_macro_SHELL);
bool test_doc_macro_SHELL(){
	TmpFile in = TmpFile(
		"<SET t='0' fmt=\"%d/%m/%Y\" />" NL
		"<p>" NL
		"    The world was created on:" NL
		"    <SHELL VARS=\"t,fmt\">date -d \"@$t\" +\"$fmt\"</SHELL>" NL
		"</p>" NL
	);
	string_view out = (
		NL
		"<p>" NL
		"    The world was created on:" NL
		"    01/01/1970"
		"</p>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_attr_macro_IF);
bool test_doc_attr_macro_IF(){
	TmpFile in = TmpFile(
		"<SET n='1'/>" NL
		"<p IF='n==1'>one</p>" NL
		"<p ELIF='n==2'>two</p>" NL
		"<p ELSE>none</p>" NL
	);
	string_view out = (
		NL
		"<p>one</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_attr_macro_ELIF);
bool test_doc_attr_macro_ELIF(){
	TmpFile in = TmpFile(
		"<SET n='2'/>" NL
		"<p IF='n==1'>one</p>" NL
		"<p ELIF='n==2'>two</p>" NL
		"<p ELSE>none</p>" NL
	);
	string_view out = (
		NL
		"<p>two</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_attr_macro_ELSE);
bool test_doc_attr_macro_ELSE(){
	TmpFile in = TmpFile(
		"<SET n='3'/>" NL
		"<p IF='n==1'>one</p>" NL
		"<p ELIF='n==2'>two</p>" NL
		"<p ELSE>none</p>" NL
	);
	string_view out = (
		NL
		"<p>none</p>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


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
		"\"x is 3\"" NL
		"\"aaa\"" NL
		"\"a\"" NL
		"1" NL
		"0" NL
		"1.1" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_expressions_functions);
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
	string_view out = (
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
	return run({in}, out, "", 0);
}


// ------------------------------------------------------------------------------------------ //