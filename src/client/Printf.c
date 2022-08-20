// Bare bones implementation of printf
#include <stdarg.h>
#include "common/Common.h"
#include "Internal.h"

static void HandleFormatArgument(LogItem *destination, const char **format, va_list va) {
  const char *fmt = *format;
  if (*fmt != '%') {
    LOG_ERROR("Format argument doesn't start with a percent sign.");
  }
  fmt++;

  switch (*fmt) {
    case 's':
      destination->type = LIT_STR;
      // Removing the const.
      // This assumes the user knows what they're doing and the data isn't stored after the call.
      destination->str_ = (char*)va_arg(va, const char *);
      break;
    case 'd': case 'i': 
      destination->type = LIT_INT;
      destination->int_ = va_arg(va, int);
      break;
    case 'c':
      destination->type = LIT_CHR;
      destination->chr_ = va_arg(va, int);
      break;
    case '%':
      destination->type = LIT_CHR;
      destination->chr_ = '%';
      break;
    default:
      LOG_INFO("Invalid format character '%c'.", *fmt);
  }
  fmt++;
  *format = fmt;
}

// Returns negative value on error. 
int FormatItems(LogItem *items, size_t items_max, size_t *out_nwritten, const char *fmt, va_list va) {
  int nprocessed = 0;
  int status = 0;

  while (*fmt) {
    switch (*fmt) {
      case '%':
        LOG_INFO("Encountered format.");
        if (nprocessed == items_max) {
          status = -1;
          goto end;
        } 
        nprocessed += 1;
        HandleFormatArgument(items++, &fmt, va);
        break;
      default:
        fmt++;
        break;
    }
  }

end:
  *out_nwritten = nprocessed;
  return status;
}
