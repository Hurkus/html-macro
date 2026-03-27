#include "test.hpp"
#include <array>

using namespace std;

#define NL "\n"
#define REGISTER2(name)	REGISTER(#name, test_##name)


// ----------------------------------- [ Functions ] ---------------------------------------- //


struct TestSet {
	string_view fileName;
	string_view in;
	string_view out;
	
	Result run() const {
		TmpFile file = TmpFile(fileName, in);
		return ::run({file}, out, "", 0);
	}
	
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(doc_html_whitespace_1);
Result test_doc_html_whitespace_1(){
	TmpFile in = TmpFile("doc_html_whitespace_1.html",
		R"(
			<p>Here<u> is some </u><span>text</span>.</p>
			
			<p>
				Long text,
				long text
			</p>
			
			<div>
			<p>Weird input
			</p>
			</div>
		)"
	);
	string_view out = (
		NL
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
Result test_doc_html_whitespace_2(){
	TmpFile in = TmpFile("doc_html_whitespace_2.html",
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
Result test_doc_element_macros(){
	TmpFile in = TmpFile("doc_element_macros.html",
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
Result test_doc_expression_context(){
	TmpFile in = TmpFile("doc_expression_context.html",
		"<p attr=\"text {1 + 2}\"></p>" NL
		"<p attr='2 * 2'></p>" NL
		"<p>There are {9.0 / 2} apples.</p>" NL
		"<p>Escaped expression \\\\\\{1+1}</p>" NL
	);
	string_view out = (
		"<p attr=\"text 3\"></p>" NL
		"<p attr=\"4\"></p>" NL
		"<p>There are 4.5 apples.</p>" NL
		"<p>Escaped expression \\{1+1}</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_expression_values);
Result test_doc_expression_values(){
	TmpFile in = TmpFile("doc_expression_values.html",
		R"(
			<SET a='[0,10,20]'></SET>
			<SET m='["x": 30, "y": 40]'></SET>
			<SET am='[0, 10, 20, "x": 30, "y": 40]'></SET>
			<p>{a[1]}</p>
			<p>{m["y"]}</p>
			<p>{am[2] + am["x"]}</p>
		)"
	);
	string_view out = (
		NL
		"<p>10</p>" NL
		"<p>40</p>" NL
		"<p>50</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_expression_variables);
Result test_doc_expression_variables(){
	TmpFile in = TmpFile("doc_expression_variables.html",
		R"(
			<SET s="header"/>
			<p class='s'>My title</p>
			
			<SET x='2' y='3'/>
			<p>My text number {x * (y + 1)}</p>
		)"
	);
	string_view out = (
		NL
		"<p class=\"header\">My title</p>" NL
		"<p>My text number 8</p>" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_expression_operators);
Result test_doc_expression_operators(){
	TmpFile in = TmpFile("doc_expression_operators.html",
		R"(
			{3.6 % 1.25}
			{"x is " + 3}
			{"a" * 3}
			{"abc" - 2}
			
			{[123] + 456}
			{[0,2,2,3] - 2}
			{[0] * [1, "x": 2]}
			
			{[10,20,30] / [1,0]}
			{["x": 1, "y": 2] / ["y"]}
			{["x": 1, "y": 2] / ["y": 123]}
			
			{[10,20,30] % [1,0]}
			{["x": 1, "y": 2] % ["y"]}
			{["x": 1, "y": 2] % ["y": 123]}
			
			{[10,20,30] % 2}
			{["x": 1, "y": 2] % "y"}
			
			{!10}
			{"a" || ""}
			{![1,2]}
		)"
	);
	string_view out = (
		NL
		"1.1" NL
		"x is 3" NL
		"aaa" NL
		"a" NL
		NL
		"[123,456]" NL
		"[0,3]" NL
		"[0,1,\"x\":2]" NL
		NL
		"[30]" NL
		"[\"x\":1]" NL
		"[\"x\":1]" NL
		NL
		"[20,10]" NL
		"[2]" NL
		"[\"y\":2]" NL
		NL
		"30" NL
		"2" NL
		NL
		"0" NL
		"1" NL
		"0" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(doc_expressions_functions);
Result test_doc_expressions_functions(){
	array<TestSet,4> t;
	
	t[0] = TestSet {
		.fileName = "doc_expressions_functions-1.html",
		.in = R"(
			{bool(4)}
			{bool("true")}
			{bool("FALSE")}
			{bool("hello")}
			{bool("")}
			
			{int('7.234')}
			{float('122.5') + 1}
			{str(12) + ' apples'}
		)",
		.out = (
			NL
			"1" NL
			"1" NL
			"0" NL
			"1" NL
			"0" NL
			"" NL
			"7" NL
			"123.5" NL
			"12 apples" NL
		)
	};
	
	t[1] = TestSet {
		.fileName = "doc_expressions_functions-2.html",
		.in = R"(
			{len('hello')}
			{abs(-4.5)}
			{min(1, 2.5, 'three')}
			{max(1, 2.5, 'three')}
			{sin(pi/6)}
			{cos(pi/3)}
		)",
		.out = (
			NL
			"5" NL
			"4.5" NL
			"1" NL
			"three" NL
			"0.5" NL
			"0.5" NL
		)
	};
	
	t[2] = TestSet {
		.fileName = "doc_expressions_functions-3.html",
		.in = R"(
			{lower('World!')}
			{upper('World!')}
			{slice("green", 1)}
			{slice("green", 1, 3)}
			{slice("green", 1, -2)}
			{slice("green", -3, 2)}
			{match('01:20:13', '\d?\d:\d\d:\d\d')}
			{replace('12 apples', '(\d+)\s+(\w+)', '$1 green $2')}
		)",
		.out = (
			NL
			"world!" NL
			"WORLD!" NL
			"reen" NL
			"ree" NL
			"ree" NL
			"ee" NL
			"1" NL
			"12 green apples" NL
		)
	};
	
	t[3] = TestSet {
		.fileName = "doc_expressions_functions-4.html",
		.in = R"(
			{if(3 > 2, "yes", "no")}
			{if(defined(x),x,100)}
			{coalesce(x,100)}
		)",
		.out = (
			NL
			"yes" NL
			"100" NL
			"100" NL
		)
	};
	
	Result r;
	for (TestSet s : t){
		r = s.run();
		if (!r) break;
	}
	
	return r;
}


// ------------------------------------------------------------------------------------------ //