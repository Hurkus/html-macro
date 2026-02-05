#pragma once
#include "config.hpp"

// https://en.wikipedia.org/wiki/ANSI_escape_code


#ifndef ANSI
	#define ANSI 1
#endif


#if ANSI
	#define ANSI_RESET		"\e[0m"
	#define ANSI_BOLD		"\e[1m"
	#define ANSI_WEAK		"\e[2m"
	#define ANSI_ITALIC		"\e[3m"
	#define ANSI_UNDERLINE	"\e[4m"
	#define ANSI_OVERLINE	"\e[53m"
	#define ANSI_STRIKE		"\e[9m"
	#define ANSI_BLINK		"\e[5m"

	#define ANSI_GRAY		"\e[90m"
	#define ANSI_RED		"\e[91m"
	#define ANSI_GREEN		"\e[92m"
	#define ANSI_YELLOW		"\e[93m"
	#define ANSI_BLUE		"\e[94m"
	#define ANSI_PURPLE		"\e[95m"
	#define ANSI_CYAN		"\e[96m"
	#define ANSI_WHITE		"\e[97m"
#else
	#define ANSI_RESET
	#define ANSI_BOLD
	#define ANSI_WEAK
	#define ANSI_ITALIC
	#define ANSI_UNDERLINE
	#define ANSI_OVERLINE
	#define ANSI_STRIKE
	#define ANSI_BLINK

	#define ANSI_GRAY
	#define ANSI_RED
	#define ANSI_GREEN
	#define ANSI_YELLOW
	#define ANSI_BLUE
	#define ANSI_PURPLE
	#define ANSI_CYAN
	#define ANSI_WHITE
#endif


#define BOLD(s) 	ANSI_BOLD s ANSI_RESET
#define RED(s)		ANSI_RED s ANSI_RESET
#define GREEN(s)	ANSI_GREEN s ANSI_RESET
#define CYAN(s)		ANSI_CYAN s ANSI_RESET
#define YELLOW(s)	ANSI_YELLOW s ANSI_RESET
#define PURPLE(s)	ANSI_PURPLE s ANSI_RESET

