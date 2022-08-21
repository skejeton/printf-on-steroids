#include "client/Printf.c"
#include "common/Common.c"
#include "common/LogEntry.c"

int FormatLikeFn(const char *fmt, ...) {
  LogItem items[64];
  uint32_t items_len;

  va_list va;
  va_start(va, fmt);
  FormatItems(items, 64, &items_len, fmt, &va);
  va_end(va);

  for (size_t i = 0; i < items_len; ++i) {
    LogItemDump(items[i]);
  }
  return 0;
}

int main() {
  FormatLikeFn("Test %d and %c and %s and %% %x", 123, 'x', "hello");
}
