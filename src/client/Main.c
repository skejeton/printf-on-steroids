
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

#include <enet/enet.h>
#include "common/Common.h"
#include "Thread.c"
#include "Core.c"

void P2_Printf(int line, const char *file, const char *fmt, ...) {
  va_list ap;
  char buf[4096];
  va_start(ap, fmt);
  vsnprintf(buf, 4096, fmt, ap);
  va_end(ap);
  Core_OutputLog((LogEntry){.line=line,.file=(char*)file,.data=buf});
}

#define P2_Print(s, x) Core_OutputLog((LogEntry){.line=__LINE__,.file=x,.data=(s)})

int main() {
  Core_Init();

#if 0
  Core_OutputLog("Oh and also, the logs preserve across runs.");
#else

  P2_Print("Printf2 (Printf on steroids) **Now with faster output**.", "test.c");
  P2_Print("This is a library/GUI package that enchances printf debugging.", "test.c");
  P2_Print("The main drawback of printf is that the data is output into the console.", "file2.c");
  P2_Print("This library allows you to sort the logs.", "file3.c");
  P2_Print("It's planned for this library to also visualize things printf can't do, like bitmaps.", "destroy.c");
  P2_Print("$For now in sorting properties you can choose to either filter by inclusion.", "destroy.c");
  P2_Print("Or_by_exclusion", "destrdaasdoy.c");
  P2_Print("@Or_by_both", "test.c");
  P2_Print("That's all I have for now! Cheers :P", "file2.c");
#endif

  Core_Deinit();
  return 0;
}