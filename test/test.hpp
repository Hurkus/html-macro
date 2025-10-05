#pragma once
#include <string>
#include <vector>
#include <filesystem>

#include "ANSI.h"

using filepath = std::filesystem::path;


// ----------------------------------- [ Variables ] ---------------------------------------- //


struct TestList;

extern filepath html_macro;
extern TestList* tests;


// ----------------------------------- [ Structures ] --------------------------------------- //


using test_func = bool(*)();

struct TestList {
	TestList* next = nullptr;
	const char* name;
	test_func func = nullptr;
	const char* file = nullptr;
	long line = 0;
};


#define REGISTER(test_name, f)          \
	bool f();                           \
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


// ----------------------------------- [ Structures ] --------------------------------------- //


struct TmpFile {
	static filepath dir;
	filepath path;
	
	TmpFile(std::string_view name, std::string_view content);
	TmpFile(std::string_view content) : TmpFile("file", content) {}
	~TmpFile();
	
	operator std::string() const {
		return path.string();
	}
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool slurp(int fd, std::string& buff);
bool slurp(const filepath& path, std::string& buff);
std::string slurp(const filepath& path);

int exe(const std::vector<std::string>& args, std::string& out, std::string& err);

bool run(const std::vector<std::string>& args, std::string_view out, std::string_view err, int status = 0);


// ------------------------------------------------------------------------------------------ //