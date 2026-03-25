#include "test.hpp"
using namespace std;

#define NL "\n"
#define REGISTER2(name)	REGISTER(#name, test_##name)


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(expression_array_1);
Result test_expression_array_1(){
	TmpFile in = TmpFile("expression_array-1.html",
		R"(
			<SET x='[1, 2, "three", [len("four")]]' />
			{x[0]}
			{x[2]}
			{x[3][0]}
		)"
	);
	string_view out = (
		NL
		"1" NL
		"three" NL
		"4" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(expression_array_2);
Result test_expression_array_2(){
	TmpFile in = TmpFile("expression_array-2.html",
		R"(
			{[100, 200, 300][2]}
			{[100, 200, 300][-3]}
			{len([1, 2, 3])}
		)"
	);
	string_view out = (
		NL
		"300" NL
		"100" NL
		"3" NL
	);
	return run({in}, out, "", 0);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER2(expression_dict_1);
Result test_expression_dict_1(){
	TmpFile in = TmpFile("expression_dict-1.html",
		R"(
			<SET m='["x": 10, "y": 20]' />
			{m['x']}
			{m['y']}
		)"
	);
	string_view out = (
		NL
		"10" NL
		"20" NL
	);
	return run({in}, out, "", 0);
}


REGISTER2(expression_dict_2);
Result test_expression_dict_2(){
	TmpFile in = TmpFile("expression_dict-2.html",
		R"(
			<!-- Mixed array and dictionary -->
			<SET m='[0, "x": 10, 1, "y": 20, 2]' />
			{m['x']}
			{m['y']}
			{m[0] + "," + m[1] + "," + m[2]}
		)"
	);
	string_view out = (
		NL
		"10" NL
		"20" NL
		"0,1,2" NL
	);
	return run({in}, out, "", 0);
}


// ------------------------------------------------------------------------------------------ //