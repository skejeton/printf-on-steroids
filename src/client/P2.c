#include "Internal.h"
#include "common/Common.h"

void P2_Print_(int line, const char *filename, const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  char pre_formatted_data[4096];
  vsnprintf(pre_formatted_data, 64, fmt, va);
  va_end(va);
  LogItem item_buffer[64];

  LogEntry entry = {.pre_formatted_data=pre_formatted_data, .line=line,.file=(char*)filename,.data=(char*)fmt};
  entry.items = item_buffer;

  va_start(va, fmt);
  FormatItems(entry.items, 64, &entry.items_len, fmt, &va);
  va_end(va);

  IFDEBUG(LogEntryDump(&entry));
  Core_OutputLog(entry);
}

int P2_Init() {
  return Core_Init();
}

void P2_Deinit() {
  Core_Deinit();
}