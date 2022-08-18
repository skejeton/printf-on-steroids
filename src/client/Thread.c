#include "Internal.h"

#ifdef _WIN32

static int MutexInit(Mutex *mutex)
{
  HANDLE mutex_handle = CreateMutexA(NULL, FALSE, NULL);
  if(mutex_handle != NULL) {
    mutex->handle = mutex_handle;
    return 1;
  }
  return 0;
}

static int MutexLock(Mutex *mutex)
{
  if(WaitForSingleObject(mutex->handle, INFINITE) == WAIT_FAILED) {
    return 0;
  }
  return 1;
}

static int MutexUnlock(Mutex *mutex)
{
  return ReleaseMutex(mutex->handle);
}

static int MutexDestroy(Mutex *mutex)
{
  return CloseHandle(mutex->handle);
}

static int ThreadCreate(Thread *thread, void *(*func)(void *))
{
  HANDLE id = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, NULL, 0, NULL);
  if(id != NULL) {
    thread->id = id;
    return 1;
  }
  return 0;
}

static int ThreadJoin(Thread *thread)
{
  if(WaitForSingleObject(thread->id, INFINITE) != WAIT_FAILED) {
    return 1;
  }
  return 0;
}

#else // POSIX

static int MutexInit(Mutex *mutex)
{
  return !pthread_mutex_init(&mutex->handle, NULL);
}

static int MutexLock(Mutex *mutex)
{
  return !pthread_mutex_lock(&mutex->handle);
}

static int MutexUnlock(Mutex *mutex)
{
  return !pthread_mutex_unlock(&mutex->handle);
}

static int MutexDestroy(Mutex *mutex)
{
  return !pthread_mutex_destroy(&mutex->handle);
}

static int ThreadCreate(Thread *thread, void *(*func)(void *))
{
  return !pthread_create(&thread->id, NULL, func, NULL);
}

static int ThreadJoin(Thread *thread)
{
  return !pthread_join(thread->id, NULL);
}


#endif
