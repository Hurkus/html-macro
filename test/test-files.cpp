#include "test.hpp"
using namespace std;


// ----------------------------------- [ Functions ] ---------------------------------------- //


REGISTER("file_1", test_file_1);
bool test_file_1(){
	filepath in = "test/test-1.in.html";
	string out = slurp("test/test-1.out.html");
	string err = "";
	return run({in}, out, err);
}


REGISTER("file_2", test_file_2);
bool test_file_2(){
	filepath in = "test/test-2.in.html";
	string out = slurp("test/test-2.out.html");
	string err = "";
	return run({in}, out, err);
}


REGISTER("file_3", test_file_3);
bool test_file_3(){
	filepath in = "test/test-3.in.html";
	string out = slurp("test/test-3.out.html");
	string err = "";
	return run({in}, out, err);
}


REGISTER("file_4", test_file_4);
bool test_file_4(){
	filepath in = "test/test-4.in.html";
	string out = slurp("test/test-4.out.html");
	string err = "";
	return run({in, "definedVariable=hello defined world"}, out, err);
}


REGISTER("file_5", test_file_5);
bool test_file_5(){
	filepath in = "test/test-5.in.html";
	string out = slurp("test/test-5.out.html");
	string err = "";
	return run({in}, out, err);
}


REGISTER("file_6", test_file_6);
bool test_file_6(){
	filepath in = "test/test-6.in.html";
	string out = slurp("test/test-6.out.html");
	string err = "";
	return run({in}, out, err);
}


REGISTER("file_7", test_file_7);
bool test_file_7(){
	filepath in = "test/test-7.in.html";
	string out = slurp("test/test-7.out.html");
	string err = "";
	return run({in}, out, err);
}


REGISTER("file_8", test_file_8);
bool test_file_8(){
	filepath in = "test/test-8.in.html";
	string out = slurp("test/test-8.out.html");
	string err = "";
	return run({in}, out, err);
}


// ------------------------------------------------------------------------------------------ //
