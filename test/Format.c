#include "client/Printf.c"

int FormatLikeFn(const char *fmt, ...) {
  LogItem items[64];
  size_t items_len;

  va_list va;
  va_start(va, fmt);
  FormatItems(items, 64, &items_len, fmt, va);
  va_end(va);

  for (size_t i = 0; i < items_len; ++i) {
    switch (items[i].type) {
      case LIT_CHR:
        LOG_INFO("CHR %c", items[i].chr_);
        break;
      case LIT_STR:
        LOG_INFO("STR %s", items[i].str_);
        break;    
      case LIT_INT:
        LOG_INFO("INT %d", items[i].int_);
        break;     
      case LIT_NIL:
        LOG_INFO("NIL");
        break;     
    }
  }
  return 0;
}

int main() {
  FormatLikeFn("Test %d and %c and %s and %%", 123, 'x', "hello");
}
