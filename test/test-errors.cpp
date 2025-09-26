#include "test.hpp"
using namespace std;

#define NL "\n"


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER("error_unclosed_tag", test_error_unclosed_tag);
bool test_error_unclosed_tag(){
	TmpFile in = TmpFile("<p></p");
	string out = "";
	string err =
	"/tmp/html-macro-test/file.html:1:2: error: Invalid end tag doesn't close any opened tag." NL
	"    1 | <p></p" NL
	"      |  ^" NL;
	return run({in}, out, err, 2);
}


// ------------------------------------------------------------------------------------------ //