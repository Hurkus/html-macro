#include "test.hpp"
using namespace std;

#define NL "\n"


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER("error_unclosed_end_tag", test_error_unclosed_end_tag);
bool test_error_unclosed_end_tag(){
	TmpFile in = TmpFile("<p></p");
	string err =
		"/tmp/html-macro-test/file.html:1:5: error: Invalid end tag doesn't close any opened tag." NL
		"    1 | <p></p" NL
		"      |     ^~" NL;
	return run({in}, "", err, 2);
}


REGISTER("error_invalid_end_tag", test_invalid_end_tag);
bool test_invalid_end_tag(){
	TmpFile in = TmpFile("<div></p></div>");
	string err =
		"/tmp/html-macro-test/file.html:1:7: error: Invalid end tag doesn't close any opened tag." NL
		"    1 | <div></p></div>" NL
		"      |       ^~" NL;
	return run({in}, "", err, 2);
}


// ------------------------------------------------------------------------------------------ //