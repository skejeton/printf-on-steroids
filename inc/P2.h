
#ifndef H_P2
#define H_P2

#include <stdarg.h>

#if !(defined(__clang__) || defined(__GNUC__))
#define __attribute__(x) 
#endif

int P2_Init();
void P2_Deinit();

__attribute__((format(printf, 3, 4)))
void P2_Print_(int line, const char *file, const char *fmt, ...);
#define P2_Print(...) P2_Print_(__LINE__, __FILE__, __VA_ARGS__)

void P2_Printv_(int line, const char *file, const char *fmt, va_list va);
#endif
