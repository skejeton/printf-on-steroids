#include "Internal.h"
#include "common/Common.h"

void P2_Print_(int line, const char *filename, const char *fmt, ...) {
  LogItem item_buffer[64];
  LogEntry entry = {.line=line,.file=(char*)filename,.data=(char*)fmt};
  entry.items = item_buffer;

  va_list va;
  va_start(va, fmt);
  FormatItems(entry.items, 64, &entry.items_len, fmt, &va);
  va_end(va);

  LogEntryDump(&entry);
  Core_OutputLog(entry);
}

void P2_Init() {
  Core_Init();
}

void P2_Deinit() {
  Core_Deinit();
}