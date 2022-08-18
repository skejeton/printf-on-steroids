#include "Internal.h"
#include "Thread.c"
#include "Core.c"

void P2_Printv_(int line, const char *filename, const char *fmt, va_list va)
{
  Core_OutputLog((LogEntry){.line=__LINE__,.file=__FILE__,.data=(fmt)});
}

void P2_Print_(int line, const char *filename, const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  P2_Printv_(line, filename, fmt, va);
  va_end(va);
}

void P2_Init()
{
  if (enet_initialize() != 0) {
    LOG_ERROR("Failed to initialize enet.");
  }

  LOG_INFO("Enet initialized.");

  GLOBAL_CLIENT = StartClient();

  MutexInit(&MUTEX);
  signal(SIGINT, SignalHandler);
  LOG_INFO("Creating thread.");
  ThreadCreate(&CORE_THREAD_ID, CoreThread);
}

void P2_Terminate()
{
  LOG_INFO("Stopping.");
  CORE_THREAD_RUNNING = 0;
  ThreadJoin(&CORE_THREAD_ID);
  LOG_INFO("Thread finished.");
  MutexDestroy(&MUTEX);
}
