#include "test.hpp"
using namespace std;

#define NL "\n"
#define REGISTER2(name)	REGISTER(#name, test_##name)


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(error_unclosed_end_tag);
bool test_error_unclosed_end_tag(){
	TmpFile in = TmpFile("<p></p");
	string err = (
		"/tmp/html-macro-test/file.html:1:4: error: Invalid end tag doesn't close any opened tag." NL
		"    1 | <p></p" NL
		"      |    ^~~" NL
	);
	return run({in}, "", err, 2);
}


REGISTER2(invalid_end_tag);
bool test_invalid_end_tag(){
	TmpFile in = TmpFile("<div></p></div>");
	string err = (
		"/tmp/html-macro-test/file.html:1:6: error: Invalid end tag doesn't close any opened tag." NL
		"    1 | <div></p></div>" NL
		"      |      ^~~" NL
	);
	return run({in}, "", err, 2);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(output_compress_css);
bool test_output_compress_css(){
	TmpFile in = TmpFile(R"(
		<style>
		@font-face {
			font-family: "Noto Sans";
			src:url("NotoSans.woff2") format("woff2"),
				url("NotoSans.otf") format("opentype");
		}
		</style>
	)");
	string out = (
		NL
		R"(<style>@font-face {font-family:"Noto Sans";src:url("NotoSans.woff2")format("woff2"),url("NotoSans.otf")format("opentype");}</style>)"
		NL
	);
	return run({in, "--compress=css"}, out, "", 0);
}


// ------------------------------------------------------------------------------------------ //