#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <iostream>

#include "ANSI.h"

using filepath = std::filesystem::path;


// ----------------------------------- [ Variables ] ---------------------------------------- //


using test_func = bool(*)();


struct TestList {
	TestList* next = nullptr;
	const char* name;
	test_func func = nullptr;
	const char* file = nullptr;
	long line = 0;
};


extern filepath html_macro;
extern TestList* tests;


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


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool slurp(int fd, std::string& buff);
bool slurp(const filepath& path, std::string& buff);
std::string slurp(const filepath& path);

int exe(const std::vector<std::string>& args, std::string& out, std::string& err);

// bool run(const std::vector<std::string>& args, const std::string& in, const std::string& out, const std::string& err);
bool run(const std::vector<std::string>& args, const std::string& out, const std::string& err);


// ------------------------------------------------------------------------------------------ //