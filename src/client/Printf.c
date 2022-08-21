// Bare bones implementation of printf
#include <stdarg.h>
#include "common/Common.h"
#include "Internal.h"

static void HandleFormatArgument(LogItem *destination, const char **format, va_list *va) {
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
      destination->str_ = (char*)va_arg(*va, const char *);
      fmt++;
      break;
    case 'd': case 'i': 
      destination->type = LIT_INT;
      destination->int_ = va_arg(*va, int);
      fmt++;
      break;
    case 'c':
      destination->type = LIT_CHR;
      destination->chr_ = va_arg(*va, int);
      fmt++;
      break;
    case '%':
      destination->type = LIT_CHR;
      destination->chr_ = '%';
      fmt++;
      break;
    default:
      destination->type = LIT_NIL;
      LOG_INFO("Invalid format character '%c'.", *fmt);
  }
  *format = fmt;
}

// Returns negative value on error. 
int FormatItems(LogItem *items, size_t items_max, uint32_t *out_nwritten, const char *fmt, va_list *va) {
  int nprocessed = 0;
  int status = 0;
  const char *format_start = fmt;

  while (*fmt) {
    switch (*fmt) {
      case '%': {
        if (nprocessed == items_max) {
          status = -1;
          goto end;
        } 

        const char *argument_start = fmt;
        LOG_INFO("Encountered format.");

        HandleFormatArgument(items, &fmt, va);
        items->start = argument_start-format_start;
        items->size = fmt-argument_start;

        nprocessed++;
        items++;
      } break;
      default:
        fmt++;
        break;
    }
  }

  LOG_INFO("Processed items: %d.", nprocessed);
end:
  *out_nwritten = nprocessed;
  return status;
}
