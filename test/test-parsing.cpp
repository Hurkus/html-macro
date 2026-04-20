#include "test.hpp"
using namespace std;

#define NL "\n"
#define REGISTER2(name)	REGISTER(#name, test_##name)


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(parse_unclosed_end_tag);
Result test_parse_unclosed_end_tag(){
	TmpFile in = TmpFile("parse_unclosed_end_tag.html", "<p></p");
	string err = (
		"/tmp/html-macro-test/parse_unclosed_end_tag.html:1:4: error: Invalid end tag doesn't close any opened tag." NL
		"    1 | <p></p" NL
		"      |    ^~~" NL
	);
	return run({in}, "", err, 2);
}


REGISTER2(parse_invalid_end_tag);
Result test_parse_invalid_end_tag(){
	TmpFile in = TmpFile("parse_invalid_end_tag.html", "<div></p></div>");
	string err = (
		"/tmp/html-macro-test/parse_invalid_end_tag.html:1:6: error: Invalid end tag doesn't close any opened tag." NL
		"    1 | <div></p></div>" NL
		"      |      ^~~" NL
	);
	return run({in}, "", err, 2);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(parse_output_compress_css_1);
Result test_parse_output_compress_css_1(){
	TmpFile in = TmpFile("parse_output_compress_css-1.html",
		R"(
			<style>
				@font-face {
					font-family: "Noto Sans";
					src:url("NotoSans.woff2") format("woff2"),
						url("NotoSans.otf") format("opentype");
				}
			</style>
		)"
	);
	string out = (
		NL
		R"(<style>@font-face {font-family:"Noto Sans";src:url("NotoSans.woff2")format("woff2"),url("NotoSans.otf")format("opentype");}</style>)" NL
	);
	return run({in, "--compress=css"}, out, "", 0);
}


REGISTER2(parse_output_compress_css_2);
Result test_parse_output_compress_css_2(){
	TmpFile in = TmpFile("parse_output_compress_css-2.html",
		R"(
			<style>
				body  #paper[data-minify='true']  .slider {
					height: 50%;
				}
			</style>
		)"
	);
	string out = (
		NL
		"<style>body #paper[data-minify='true'] .slider {height:50%;}</style>" NL
	);
	return run({in, "--compress=css"}, out, "", 0);
}


// ------------------------------------------------------------------------------------------ //