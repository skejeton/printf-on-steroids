#include "Internal.h"
#include "common/Common.h"


void P2_Printv_(int line, const char *filename, const char *fmt, va_list va) {
  va_list va_format, va_data;

  va_copy(va_format, va);
  va_copy(va_data, va);

  char pre_formatted_data[4096];
  vsnprintf(pre_formatted_data, 64, fmt, va_format);
  LogItem item_buffer[64];

  LogEntry entry = {.pre_formatted_data=pre_formatted_data, .line=line,.file=(char*)filename,.data=(char*)fmt};
  entry.items = item_buffer;

  FormatItems(entry.items, 64, &entry.items_len, fmt, &va_data);

  IFDEBUG(LogEntryDump(&entry));
  Core_OutputLog(entry);
}

void P2_Print_(int line, const char *filename, const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  P2_Printv_(line, filename, fmt, va);
  va_end(va);
}

int P2_GetStatus() {
  return Core_Status();
}

int P2_Init() {
  return Core_Init();
}

void P2_Deinit() {
  Core_Deinit();
}