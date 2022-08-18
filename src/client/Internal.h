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
#include <common/LogEntry.h>

struct Mutex typedef Mutex;
struct Thread typedef Thread;

static int MutexInit(Mutex *mutex);
static int MutexLock(Mutex *mutex);
static int MutexUnlock(Mutex *mutex);
static int MutexDestroy(Mutex *mutex);

static int ThreadCreate(Thread *thread, void *(*func)(void*));
static int ThreadJoin(Thread *thread);