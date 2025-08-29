#pragma once
#include <iostream>
#include "ANSI.h"


// ---------------------------------- [ Definitions ] --------------------------------------- //


#ifdef NDEBUG
	#undef DEBUG
#else
	#define DEBUG
#endif


// ---------------------------------- [ Definitions ] --------------------------------------- //


#define PRINT_HERE(...)	\
	std::fprintf(stderr, ANSI_BOLD "[%s:%ld]" ANSI_RESET "\n ", __FILE__, size_t(__LINE__))


#ifdef DEBUG
	#define ERROR(...)	{\
		PRINT_HERE(); \
		::error(__VA_ARGS__); \
	}
	#define WARN(...)	{\
		PRINT_HERE(); \
		::warn(__VA_ARGS__); \
	}
	#define INFO(...)	{\
		PRINT_HERE(); \
		::info(__VA_ARGS__); \
	}
#else
	#define ERROR(...)	::error(__VA_ARGS__)
	#define WARN(...)	::warn(__VA_ARGS__)
	#define INFO(...)	::info(__VA_ARGS__)
#endif


// ----------------------------------- [ Functions ] ---------------------------------------- //


template <typename ...T>
static void error(const char* fmt, T... arg){
	std::fprintf(stderr, ANSI_RED "error: " ANSI_RESET);
	std::fprintf(stderr, fmt, arg...);
	std::fprintf(stderr, "\n");
}


template <typename ...T>
static void warn(const char* fmt, T... arg){
	std::fprintf(stderr, ANSI_YELLOW ANSI_BOLD "warn: " ANSI_RESET);
	std::fprintf(stderr, fmt, arg...);
	std::fprintf(stderr, "\n");
}


template <typename ...T>
static void info(const char* fmt, T... arg){
	std::fprintf(stderr, fmt, arg...);
	std::fprintf(stderr, "\n");
}


// ------------------------------------------------------------------------------------------ //
