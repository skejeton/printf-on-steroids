#ifndef H_COMMON
#define H_COMMON

#include <stdio.h>

#ifndef DISABLE_LOGS
#define LOG_ERROR(...) (printf("\x1b[31mLOG_ERROR\x1b[0m: " __VA_ARGS__), printf("\n"), exit(-1))
#define LOG_INFO(...) (printf("\x1b[34mLOG_INFO\x1b[0m: " __VA_ARGS__), printf("\n"))
#endif

#define DEFAULT_PORT       4891
#define DEFAULT_CLIENT_CAP 32
#define DEFAULT_CHAN_CAP   2

#endif
