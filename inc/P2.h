
#ifndef H_P2
#define H_P2

#include <stdarg.h>

void P2_Init();
void P2_Terminate();

void P2_Print_(int line, const char *file, const char *fmt, ...);
#define P2_Print(fmt, ...) P2_Print_(__LINE__, __FILE__, fmt, __VA_ARGS__)

void P2_Printv_(int line, const char *file, const char *fmt, va_list va);
#define P2_Printv(fmt, ...) P2_Printv_(__LINE__, __FILE__, fmt, __VA_ARGS__)

#endif
