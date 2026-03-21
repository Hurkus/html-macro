#pragma once
#include <string>
#include <vector>

#include "../src/fs.hpp"
#include "../src/includes/fd.hpp"
#include "../src/Debug.hpp"


// ----------------------------------- [ Variables ] ---------------------------------------- //


struct TestList;

extern filepath html_macro;
extern TestList* tests;


// ----------------------------------- [ Structures ] --------------------------------------- //


struct TmpFile {
	filepath path;
	
	TmpFile(std::string_view filename, std::string_view content);
	~TmpFile();
	
	operator std::string() const {
		return path.string();
	}
};


struct Result {
	int recievedStatus = 0;
	int expectedStatus = 0;
	std::string expectedStdout;
	std::string recievedStdout;
	std::string expectedStderr;
	std::string recievedStderr;
	
	operator bool() const {
		return (recievedStatus == expectedStatus) && (expectedStdout == recievedStdout) && (expectedStderr == recievedStderr);
	}
	
};


// ----------------------------------- [ Structures ] --------------------------------------- //


using test_func = Result(*)();

struct TestList {
	TestList* next = nullptr;
	const char* name;
	test_func func = nullptr;
	const char* file = nullptr;
	long line = 0;
};


#define REGISTER(test_name, f)          \
	Result f();                         \
	struct _test_register_t_##f {       \
		_test_register_t_##f(){         \
			tests = new TestList {      \
				.next = tests,          \
				.name = test_name,      \
				.func = f,              \
				.file = __FILE__,       \
				.line = long(__LINE__), \
			};                          \
		}                               \
	} _test_register_##f;               \


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool slurp(int fd, std::string& buff);
bool slurp(const filepath& path, std::string& buff);
std::string slurp(const filepath& path);

int exe(const std::vector<std::string>& args, std::string& out, std::string& err);
Result run(const std::vector<std::string>& args, std::string_view out, std::string_view err, int status = 0);


// ------------------------------------------------------------------------------------------ //