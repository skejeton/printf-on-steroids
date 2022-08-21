#include "Internal.h"

#ifdef _WIN32

int MutexInit(Mutex *mutex)
{
  HANDLE mutex_handle = CreateMutexA(NULL, FALSE, NULL);
  if(mutex_handle != NULL) {
    mutex->handle = mutex_handle;
    return 1;
  }
  return 0;
}

int MutexLock(Mutex *mutex)
{
  if(WaitForSingleObject(mutex->handle, INFINITE) == WAIT_FAILED) {
    return 0;
  }
  return 1;
}

int MutexUnlock(Mutex *mutex)
{
  return ReleaseMutex(mutex->handle);
}

int MutexDestroy(Mutex *mutex)
{
  return CloseHandle(mutex->handle);
}

int ThreadCreate(Thread *thread, void *(*func)(void *))
{
  HANDLE id = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, NULL, 0, NULL);
  if(id != NULL) {
    thread->id = id;
    return 1;
  }
  return 0;
}

int ThreadJoin(Thread *thread)
{
  if(WaitForSingleObject(thread->id, INFINITE) != WAIT_FAILED) {
    return 1;
  }
  return 0;
}

int ThreadSleep(int seconds) {
  DWORD millis = (DWORD)(seconds * 1000);
  Sleep(millis);
  return 0;
}

#else // POSIX

int MutexInit(Mutex *mutex)
{
  return !pthread_mutex_init(&mutex->handle, NULL);
}

int MutexLock(Mutex *mutex)
{
  return !pthread_mutex_lock(&mutex->handle);
}

int MutexUnlock(Mutex *mutex)
{
  return !pthread_mutex_unlock(&mutex->handle);
}

int MutexDestroy(Mutex *mutex)
{
  return !pthread_mutex_destroy(&mutex->handle);
}

int ThreadCreate(Thread *thread, void *(*func)(void *))
{
  return !pthread_create(&thread->id, NULL, func, NULL);
}

int ThreadJoin(Thread *thread)
{
  return !pthread_join(thread->id, NULL);
}

int ThreadSleep(int seconds) {
  return sleep(seconds);
}

#endif
