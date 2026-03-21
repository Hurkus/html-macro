#include "test.hpp"
using namespace std;

#define NL "\n"
#define REGISTER2(name)	REGISTER(#name, test_##name)


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_macro_MACRO_1);
Result test_doc_macro_MACRO_1(){
	TmpFile in = TmpFile("doc_macro_MACRO_1.html",
		R"|(
			<ul>
				<FOR i='0' TRUE='i<3' i='i+1'>
					<CALL NAME="func"/>
				</FOR>
			</ul>
			
			<MACRO NAME="func">
				<SET col='if(i == 1, "red", "green")' />
				<li style="color:{col};">Line #{i}</li>
			</MACRO>
		)|"
	);
	string_view out = (
		NL
		"<ul>" NL
		"	<li style=\"color:green;\">Line #0</li>" NL
		"	<li style=\"color:red;\">Line #1</li>" NL
		"	<li style=\"color:green;\">Line #2</li>" NL
		"</ul>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_MACRO_2);
Result test_doc_macro_MACRO_2(){
	TmpFile in = TmpFile("doc_macro_MACRO_2.html",
		R"(
			<MACRO NAME="func" x y='0'>
				<SET x='x+2' y='y+2' z='z+2'/>
				<p>A: (x,y,z) is ({x}, {y}, {z})</p>
			</MACRO>
			
			<SET x='10' y='20' z='30'/>
			<CALL NAME="func"/>
			<p>B: (x,y,z) is ({x}, {y}, {z})</p>
		)"
	);
	string_view out = (
		NL
		"<p>A: (x,y,z) is (12, 2, 32)</p>" NL
		"<p>B: (x,y,z) is (10, 20, 32)</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_MACRO_3);
Result test_doc_macro_MACRO_3(){
	TmpFile in = TmpFile("doc_macro_MACRO_3.html",
		R"(
			<MACRO NAME="F1">
				<h1>Book named {book}</h1>
			</MACRO>
			<MACRO NAME="F2">
				<SET-ATTR href="#{VALUE}"/>
				{VALUE}
			</MACRO>
			
			<F1 book="Unnamed"/>
			<p>
				Go to chapter
				<a F2="main" class="box"/>
			</p>
		)"
	);
	string_view out = (
		NL
		"<h1>Book named Unnamed</h1>" NL
		"<p>" NL
		"	Go to chapter" NL
		"	<a href=\"#main\" class=\"box\">" NL
		"		main" NL
		"	</a>" NL
		"</p>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_macro_INCLUDE);
Result test_doc_macro_INCLUDE(){
	TmpFile in = TmpFile("doc_macro_INCLUDE.html",
		R"(
			<SET name="cookbook"/>
			<body>
				<INCLUDE SRC="doc_macro_INCLUDE-header.html"/>
				<p>Amazing content!</p>
				<INCLUDE SRC="doc_macro_INCLUDE-footer.html"/>
			</body>
		)"
	);
	
	TmpFile header = TmpFile("doc_macro_INCLUDE-header.html",
		R"(
			<style>
				#header {
					color: red;
				}
			</style>
			<div id="header">
				<h1>Yet another {name}</h1>
			</div>
		)"
	);
	
	TmpFile footer = TmpFile("doc_macro_INCLUDE-footer.html",
		R"(
			<div id="footer">
				<p>Contacts: none</p>
				<p>Last edited on: 14/10/2025</p>
			</div>
		)"
	);
	
	string_view out = (
		NL
		"<body>" NL
		"	<style>" NL
		"		#header {" NL
		"			color: red;" NL
		"		}" NL
		"	</style>" NL
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


REGISTER2(doc_attr_macro_INCLUDE);
Result test_doc_attr_macro_INCLUDE(){
	TmpFile in = TmpFile("doc_attr_macro_INCLUDE.html",
		R"(
			<SET lang="en"/>
			<body lang="{lang}">
				<ol>
					<FOR i='1' TRUE='i<=3' i='i+1'>
						<li INCLUDE="doc_attr_macro_INCLUDE-content.html"/>
					</FOR>
				</ol>
			</body>
		)"
	);
	
	TmpFile content = TmpFile("doc_attr_macro_INCLUDE-content.html",
		R"(
			<IF lang="de">
				<p IF='i==1'>Ein</p>
				<p ELIF='i==2'>Zwei</p>
				<p ELIF='i==3'>Drei</p>
			</IF>
			<ELSE>
				<p IF='i==1'>One</p>
				<p ELIF='i==2'>Two</p>
				<p ELIF='i==3'>Three</p>
			</ELSE>
		)"
	);
	
	string_view out = (
		NL
		"<body lang=\"en\">" NL
		"	<ol>" NL
		"		<li>" NL
		"			<p>One</p>" NL
		"		</li>" NL
		"		<li>" NL
		"			<p>Two</p>" NL
		"		</li>" NL
		"		<li>" NL
		"			<p>Three</p>" NL
		"		</li>" NL
		"	</ol>" NL
		"</body>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_macro_CALL_1);
Result test_doc_macro_CALL_1(){
	TmpFile in = TmpFile("doc_macro_CALL_1.html",
		R"(
			<MACRO NAME="f">
				<SET x='x+2'/>
				<p>x1 is {x}</p>
			</MACRO>
			
			<SET x='10'/>
			<CALL NAME="f" x/>
			<p>x2 is {x}</p>
		)"
	);
	string_view out = (
		"" NL
		"<p>x1 is 12</p>" NL
		"<p>x2 is 10</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_CALL_2);
Result test_doc_macro_CALL_2(){
	TmpFile in = TmpFile("doc_macro_CALL_2.html",
		R"(
			<MACRO NAME="f" y='200'>
				<SET y='y+2'/>
				<p>y1 is {y}</p>
			</MACRO>
			
			<SET y='20'/>
			<CALL NAME="f"/>
			<p>y2 is {y}</p>
		)"
	);
	string_view out = (
		NL
		"<p>y1 is 202</p>" NL
		"<p>y2 is 20</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_CALL_3);
Result test_doc_macro_CALL_3(){
	TmpFile in = TmpFile("doc_macro_CALL_3.html",
		R"(
			<MACRO NAME="f">
				<SET z='z+2'/>
				<p>z1 is {z}</p>
			</MACRO>
			
			<SET z='30'/>
			<CALL NAME="f" z='300'/>
			<p>z2 is {z}</p>
		)"
	);
	string_view out = (
		NL
		"<p>z1 is 302</p>" NL
		"<p>z2 is 30</p>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_macro_SET);
Result test_doc_macro_SET(){
	TmpFile in = TmpFile("doc_macro_SET.html",
		R"(
			<SET x='4' y='6.5' s="apples"/>
			<p>I have {int(x*y)} bushels of {s}</p> 
		)"
	);
	string_view out = (
		NL
		"<p>I have 26 bushels of apples</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_IF);
Result test_doc_macro_IF(){
	TmpFile in1 = TmpFile("doc_macro_IF-1.html",
		R"(
			<IF TRUE='6>3'>
				<p>6 is greater than 3</p>
			</IF>
		)"
	);
	string_view out1 = (
		NL
		"<p>6 is greater than 3</p>" NL
	);
	
	Result r = run({in1}, out1, "", 0);
	if (!r){
		return r;
	}
	
	TmpFile in2 = TmpFile("doc_macro_IF-2.html",
		R"(
			<SET lang="en"/>
			<IF lang="en">
				<p>English</p>
			</IF>
		)"
	);
	string_view out2 = (
		NL
		"<p>English</p>" NL
	);
	
	return run({in2}, out2, "", 0);
}


REGISTER2(doc_macro_ELIF);
Result test_doc_macro_ELIF(){
	TmpFile in = TmpFile("doc_macro_ELIF.html",
		R"(
			<SET lang="en"/>
			
			<IF lang="de">
				<p>Deutsch</p>
			</IF>
			<ELIF lang="en">
				<p>English</p>
			</ELIF>
		)"
	);
	string_view out = (
		NL
		"<p>English</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_ELSE);
Result test_doc_macro_ELSE(){
	TmpFile in = TmpFile("doc_macro_ELSE.html",
		R"(
			<SET lang="en"/>
			
			<IF lang="de">
				<p>Deutsch</p>
			</IF>
			<ELSE>
				<p>English</p>
			</ELSE>
		)"
	);
	string_view out = (
		NL
		"<p>English</p>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_macro_FOR);
Result test_doc_macro_FOR(){
	TmpFile in = TmpFile("doc_macro_FOR.html",
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
Result test_doc_macro_WHILE(){
	TmpFile in = TmpFile("doc_macro_WHILE.html",
		R"(
			<SET i='0'/>
			<WHILE TRUE='i < 3'>
				<p>{i}</p>
				<SET i='i+1'/>
			</WHILE>
		)"
	);
	string_view out = (
		NL
		"<p>0</p>" NL
		"<p>1</p>" NL
		"<p>2</p>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_macro_SET_TAG);
Result test_doc_macro_SET_TAG(){
	TmpFile in = TmpFile("doc_macro_SET_TAG.html",
		"<div>" NL
		"	<SET-TAG p/>" NL
		"	text" NL
		"</div>" NL
	);
	string_view out = (
		"<p>" NL
		"	text" NL
		"</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_GET_TAG);
Result test_doc_macro_GET_TAG(){
	TmpFile in = TmpFile("doc_macro_GET_TAG.html",
		"<div>" NL
		"	<GET-TAG tag/>" NL
		"	This text is within a {tag} element." NL
		"</div>" NL
	);
	string_view out = (
		"<div>" NL
		"	This text is within a div element." NL
		"</div>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_SET_ATTR);
Result test_doc_macro_SET_ATTR(){
	TmpFile in = TmpFile("doc_macro_SET_ATTR.html",
		"<div>" NL
		"	<SET-ATTR title=\"text\"/>" NL
		"	text" NL
		"</div>" NL
	);
	string_view out = (
		"<div title=\"text\">" NL
		"	text" NL
		"</div>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_macro_GET_ATTR);
Result test_doc_macro_GET_ATTR(){
	TmpFile in = TmpFile("doc_macro_GET_ATTR.html",
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
Result test_doc_macro_DEL_ATTR(){
	TmpFile in = TmpFile("doc_macro_DEL_ATTR.html",
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
Result test_doc_macro_SHELL(){
	TmpFile in = TmpFile("doc_macro_SHELL.html",
		R"(
			<SET t='0' fmt="%d/%m/%Y" />
			<p>
				The world was created on:
				<SHELL VARS="t,fmt">date -d "@$t" +"$fmt"</SHELL>
			</p>
		)"
	);
	string_view out = (
		NL
		"<p>" NL
		"	The world was created on:" NL
		"	01/01/1970" NL
		"</p>" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_attr_macro_IF);
Result test_doc_attr_macro_IF(){
	TmpFile in = TmpFile("doc_attr_macro_IF.html",
		R"(
			<SET n='1'/>
			<p IF='n==1'>one</p>
			<p ELIF='n==2'>two</p>
			<p ELSE>none</p>
		)"
	);
	string_view out = (
		NL
		"<p>one</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_attr_macro_ELIF);
Result test_doc_attr_macro_ELIF(){
	TmpFile in = TmpFile("doc_attr_macro_ELIF.html",
		R"(
			<SET n='2'/>
			<p IF='n==1'>one</p>
			<p ELIF='n==2'>two</p>
			<p ELSE>none</p>
		)"
	);
	string_view out = (
		NL
		"<p>two</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_attr_macro_ELSE);
Result test_doc_attr_macro_ELSE(){
	TmpFile in = TmpFile("doc_attr_macro_ELSE.html",
		R"(
			<SET n='3'/>
			<p IF='n==1'>one</p>
			<p ELIF='n==2'>two</p>
			<p ELSE>none</p>
		)"
	);
	string_view out = (
		NL
		"<p>none</p>" NL
	);
	return run({in}, out, "", 0);
}


// ------------------------------------------------------------------------------------------ //