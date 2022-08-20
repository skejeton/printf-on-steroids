#include "Internal.h"
#include "common/Common.h"

void P2_Print_(int line, const char *filename, const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  char data[4096];
  vsnprintf(data, 4096, fmt, va);
  va_end(va);

  Core_OutputLog((LogEntry){.line=line,.file=(char*)filename,.data=data});
}

void P2_Init() {
  Core_Init();
}

void P2_Deinit() {
  Core_Deinit();
}