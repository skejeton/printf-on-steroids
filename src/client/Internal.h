#ifndef H_INTERNAL
#define H_INTERNAL
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <Windows.h>
#else
  #include <pthread.h>
#endif

#include <P2.h>
#include <enet/enet.h>
#include <common/Common.h>

#ifdef _WIN32
struct Mutex {
  HANDLE handle;
};

struct Thread {
  HANDLE id;
};
#else
struct Mutex {
  pthread_mutex_t handle;
};

struct Thread {
  pthread_t id;
};
#endif

////////////////////////////////////////////////////////////////////////////////
// Thread.c

struct Mutex typedef Mutex;
struct Thread typedef Thread;

int MutexInit(Mutex *mutex);
int MutexLock(Mutex *mutex);
int MutexUnlock(Mutex *mutex);
int MutexDestroy(Mutex *mutex);

int ThreadCreate(Thread *thread, void *(*func)(void*));
int ThreadJoin(Thread *thread);

////////////////////////////////////////////////////////////////////////////////
// Printf.c
int FormatItems(LogItem *items, size_t items_max, size_t *out_nwritten, const char *fmt, va_list va);

////////////////////////////////////////////////////////////////////////////////
// Core.c

#include "Internal.h"

void Core_Init();
void Core_Deinit();
void Core_OutputLog(LogEntry entry);

#endif