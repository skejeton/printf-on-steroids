// Bare bones implementation of printf
#include <stdarg.h>
#include "common/Common.h"
#include "Internal.h"

static int HandleIntegerParameter(const char **format, va_list *va) {
  const char *fmt = *format;
  int result = 0;
  if (*fmt == '*') {
    result = va_arg(*va, int);
    fmt++;
  } else if (*fmt >= '0' && *fmt <= '9') {
    while (*fmt && *fmt >= '0' && *fmt <= '9') {
      result = result * 10 + (*fmt-'0');
      fmt++;
    }
  }
  *format = fmt;
  return result;
}

static void HandleFormatArgument(LogItem *destination, const char **format, va_list *va) {
  const char *fmt = *format;
  if (*fmt != '%') {
    LOG_ERROR("Format argument doesn't start with a percent sign.");
  }
  fmt++;
  switch(*fmt) {
    case '-':
      destination->flags |= LIF_PAD_LEFT; fmt++;
      break;
    case '+':
      destination->flags |= LIF_FORCE_SIGN; fmt++;
      break;
    case ' ':
      destination->flags |= LIF_PAD_SIGN; fmt++;
      break;
    case '#':
      destination->flags |= LIF_NUMBER_DECOR; fmt++;
      break;
    case '0':
      destination->flags |= LIF_PAD_ZERO; fmt++;
      break;
  }
  destination->width = HandleIntegerParameter(&fmt, va);
  if (*fmt == '.') {
    fmt++;
    destination->precision = HandleIntegerParameter(&fmt, va);
  }

  #define VAENTRY1(enumtype, field, vtype, cast) destination->type = enumtype; destination->field##_ = (cast)va_arg(*va, vtype); fmt++; break;
  #define VAENTRY2(enumtype, field, vtype) VAENTRY1(enumtype, field, vtype, vtype)

  switch (*fmt) {
    case 'i':
    case 'd': VAENTRY2(LIT_INT, int, int);

    case 'u': VAENTRY2(LIT_UINT, uint, unsigned int);
    case 'o': VAENTRY2(LIT_UOCT, uint, unsigned int);
    case 'x': VAENTRY2(LIT_HEX, uint, unsigned int);
    case 'X': VAENTRY2(LIT_UPCHEX, uint, unsigned int);
    case 'f': VAENTRY2(LIT_FLT, flt, double); 
    case 'F': VAENTRY2(LIT_UPCFLT, flt, double);
    case 'e': VAENTRY2(LIT_SCIFLT, flt, double);
    case 'E': VAENTRY2(LIT_SCIUPCFLT, flt, double);
    case 'g': VAENTRY2(LIT_SHRFLT, flt, double); 
    case 'G': VAENTRY2(LIT_SHRUPCFLT, flt, double); 
    case 'a': VAENTRY2(LIT_HEXFLT, flt, double); 
    case 'A': VAENTRY2(LIT_UPCHEXFLT, flt, double); 
    case 'c': VAENTRY2(LIT_CHR, int, int);
    case 's': VAENTRY1(LIT_STR, str, const char *, char*);
    case 'p': VAENTRY1(LIT_PTR, uint, void *, uint64_t);
    case 'n': *va_arg(*va, signed int*) = 0; fmt++; break; // Write zero as a stub.
    case '%': destination->type = LIT_CHR; destination->int_ = '%'; fmt++; break;
    default:  LOG_ERROR("Invalid format specified '%c'", *fmt);
  }

  #undef VAENTRY1
  #undef VAENTRY2
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
