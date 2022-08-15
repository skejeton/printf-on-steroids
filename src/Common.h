#ifndef H_COMMON
#define H_COMMON

#include <stdio.h>

#define ERROR(...) (printf("\x1b[31merror\x1b[0m: " __VA_ARGS__), printf("\n"), exit(-1))
#define INFO(...) (printf("\x1b[34minfo\x1b[0m: " __VA_ARGS__), printf("\n"))

#endif
