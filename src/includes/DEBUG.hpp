#pragma once
#include <cstdio>
#include <cassert>
#include "ANSI.h"


// ---------------------------------- [ Definitions ] --------------------------------------- //


#ifdef NDEBUG
	#undef DEBUG
#else
	#define DEBUG
#endif


#ifdef DEBUG
	#ifndef DEBUG_L0
		#define DEBUG_L0 1
	#endif
	#ifndef DEBUG_L1
		#define DEBUG_L1 1
	#endif
	#ifndef DEBUG_L2
		#define DEBUG_L2 1
	#endif
	#ifndef DEBUG_THROW_WARNING
		#define DEBUG_THROW_WARNING 0
	#endif
	#ifndef DEBUG_THROW_ERROR
		#define DEBUG_THROW_ERROR 1
	#endif
	#ifndef DEBUG_TIME
		#define DEBUG_TIME 0
	#endif
#else
	#ifndef DEBUG_L0
		#define DEBUG_L0 1
	#endif
	#ifndef DEBUG_L1
		#define DEBUG_L1 1
	#endif
	#ifndef DEBUG_L2
		#define DEBUG_L2 0
	#endif
	#ifndef DEBUG_THROW_WARNING
		#define DEBUG_THROW_WARNING 0
	#endif
	#ifndef DEBUG_THROW_ERROR
		#define DEBUG_THROW_ERROR 0
	#endif
	#ifndef DEBUG_TIME
		#define DEBUG_TIME 0
	#endif
#endif


// ---------------------------------- [ Definitions ] --------------------------------------- //


#ifdef DEBUG
	#define ERROR(...)		::errorf(__FILE__, size_t(__LINE__), __VA_ARGS__)
	#define INFO(...)		::infof(__FILE__, size_t(__LINE__), __VA_ARGS__)
	#define WARNING(...)	::warnf(__FILE__, size_t(__LINE__), __VA_ARGS__)
#else
	#define INFO(...)		::info(__VA_ARGS__)
	#define ERROR(...)		::error(__VA_ARGS__)
	#define WARNING(...)	::warn(__VA_ARGS__)
#endif


#if DEBUG_L0 == 1
	#define ERROR_L0(...)		ERROR(__VA_ARGS__)
	#define WARNING_L0(...)		WARNING(__VA_ARGS__)
	#define INFO_L0(...)		INFO(__VA_ARGS__)
#else
	#define ERROR_L0(...)
	#define WARNING_L0(...)
	#define INFO_L0(...)
#endif


#if DEBUG_L1 == 1
	#define ERROR_L1(...)		ERROR(__VA_ARGS__)
	#define WARNING_L1(...)		WARNING(__VA_ARGS__)
	#define INFO_L1(...)		INFO(__VA_ARGS__)
#else
	#define ERROR_L1(...)
	#define WARNING_L1(...)
	#define INFO_L1(...)
#endif


#if DEBUG_L2 == 1
	#define ERROR_L2(...)		ERROR(__VA_ARGS__)
	#define WARNING_L2(...)		WARNING(__VA_ARGS__)
	#define INFO_L2(...)		INFO(__VA_ARGS__)
#else
	#define ERROR_L2(...)
	#define WARNING_L2(...)
	#define INFO_L2(...)
#endif


// ---------------------------------- [ Definitions ] --------------------------------------- //


#if DEBUG_TIME == 1
	#include <chrono>
	#define TIME_NOW()			(std::chrono::high_resolution_clock::now())
	#define TIME_START(t)		auto timer_##t = TIME_NOW()
	#define TIME_RESTART(t)		(timer_##t = TIME_NOW())
	#define TIME_SECONDS(t)		(std::chrono::duration<float>(TIME_NOW() - timer_##t).count())
	#define TIME_REPORT(t)		INFO(#t ": %.1f ms", TIME_SECONDS(t) * 1000)
#else
	#define TIME_NOW()
	#define TIME_START(t)
	#define TIME_RESTART(t)
	#define TIME_SECONDS(t)
	#define TIME_REPORT(t)
#endif


// ----------------------------------- [ Functions ] ---------------------------------------- //


template <typename ...T>
static void error(const char* fmt, T... arg){
	std::fprintf(stderr, ANSI_RED "error: " ANSI_RESET);
	std::fprintf(stderr, fmt, arg...);
	std::fprintf(stderr, "\n");
	
	#if DEBUG_THROW_ERROR == 1
		exit(1);
	#endif
}

template <typename ...T>
static void errorf(const char* file, const char* fmt, T... arg){
	std::fprintf(stderr, ANSI_BOLD "%s: ", file);
	std::fprintf(stderr, ANSI_RED "error: " ANSI_RESET);
	std::fprintf(stderr, fmt, arg...);
	std::fprintf(stderr, "\n");
	
	#if DEBUG_THROW_ERROR == 1
		exit(1);
	#endif
}

template <typename ...T>
static void errorf(const char* file, size_t line, const char* fmt, T... arg){
	std::fprintf(stderr, ANSI_BOLD "%s:%ld: ", file, line);
	std::fprintf(stderr, ANSI_RED "error: " ANSI_RESET);
	std::fprintf(stderr, fmt, arg...);
	std::fprintf(stderr, "\n");
	
	#if DEBUG_THROW_ERROR == 1
		exit(1);
	#endif
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


template <typename ...T>
static void warn(const char* fmt, T... arg){
	std::fprintf(stderr, ANSI_YELLOW ANSI_BOLD "warn: " ANSI_RESET);
	std::fprintf(stderr, fmt, arg...);
	std::fprintf(stderr, "\n");
	
	#if DEBUG_THROW_WARNING == 1
		exit(1);
	#endif
}

template <typename ...T>
static void warnf(const char* file, size_t line, const char* fmt, T... arg){
	std::fprintf(stderr, ANSI_BOLD "%s:%ld: ", file, line);
	std::fprintf(stderr, ANSI_YELLOW ANSI_BOLD "warn: " ANSI_RESET);
	std::fprintf(stderr, fmt, arg...);
	std::fprintf(stderr, "\n");
	
	#if DEBUG_THROW_WARNING == 1
		exit(1);
	#endif
}


template <typename ...T>
static void info(const char* fmt, T... arg){
	std::fprintf(stdout, fmt, arg...);
	std::fprintf(stdout, "\n");
}

template <typename ...T>
static void infof(const char* file, size_t line, const char* fmt, T... arg){
	std::fprintf(stdout, ANSI_BOLD "%s:%ld: " ANSI_RESET, file, line);
	std::fprintf(stdout, fmt, arg...);
	std::fprintf(stdout, "\n");
}


// ------------------------------------------------------------------------------------------ //
