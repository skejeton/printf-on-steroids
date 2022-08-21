#include "Common.h"

static void ConvLogEntry(PacketStream *ps, LogEntry *e) {
  PS_WRITEVAL(ps, &e->line);
  PS_WRITESTR(ps, &e->file);
  PS_WRITESTR(ps, &e->data);
  PS_WRITESTR(ps, &e->pre_formatted_data);
  PS_LIST(i, ps, &e->items, &e->items_len) {
    LogItemType type = *(LogItemType*)PS_WRITEVAL(ps, &e->items[i].type);
    PS_WRITEVAL(ps, &e->items[i].start);
    PS_WRITEVAL(ps, &e->items[i].size);
    PS_WRITEVAL(ps, &e->items[i].flags);
    PS_WRITEVAL(ps, &e->items[i].precision);
    PS_WRITEVAL(ps, &e->items[i].width);

    switch (type) {
      case LIT_INT: case LIT_CHR: 
        PS_WRITEVAL(ps, &e->items[i].int_);
        break;
      case LIT_UINT: case LIT_UOCT: case LIT_HEX: case LIT_UPCHEX: case LIT_PTR:
        PS_WRITEVAL(ps, &e->items[i].uint_);
        break;
      case LIT_FLT: case LIT_UPCFLT: case LIT_SCIFLT: case LIT_SCIUPCFLT: case LIT_SHRFLT: case LIT_SHRUPCFLT: case LIT_HEXFLT: case LIT_UPCHEXFLT:
        PS_WRITEVAL(ps, &e->items[i].flt_);
        break;
      case LIT_STR:
        PS_WRITESTR(ps, &e->items[i].str_);
        break;
      case LIT_NIL:
        break;
    }
  }
}

void * LogEntryEncode(LogEntry entry) {
  PacketStream ps = PS_BeginWrite();
  ConvLogEntry(&ps, &entry);
  void *origin = PS_FinalizeWrite(&ps);
  

  LOG_INFO("Encoded package:");
  IFDEBUG(DumpBinary(origin, PS_PacketSize(origin)));


  return origin;
}

LogEntry LogEntryDecode(void *origin) {
  LOG_INFO("Decoding package:");
  IFDEBUG(DumpBinary(origin, PS_PacketSize(origin)));

  PacketStream ps = PS_BeginRead(origin);
  LogEntry result = {0};
  ConvLogEntry(&ps, &result);
  return result;
}

/* LogEntryDeinit(entry)
// Deinitializes data within LogEntry,
// This assumes the log entry was allocated by PacketStream.
*/
void LogEntryDeinit(LogEntry *entry) {
  PacketStream ps = PS_BeginFree();
  ConvLogEntry(&ps, entry);
}

/*
// The difference between GENPRINT1 and GENPRINT2 is that GENPRINT1 ignores precision parameter.
// This is because for some specifiers lie %c using precision parameter is UB.
*/
#define GENPRINT1(cas, suffix, val)\
  case cas:\
  if (item->width == 0)\
    snprintf(dest, max, "%" suffix, val);\
  else\
    snprintf(dest, max, "%*" suffix, item->width, val);\
  break

#define GENPRINT2(cas, suffix, val)\
  case cas:\
  if (item->precision == 0) {\
    if (item->width == 0)\
      snprintf(dest, max, "%" suffix, val);\
    else\
      snprintf(dest, max, "%*" suffix, item->width, val);\
  } else {\
    if (item->width == 0)\
      snprintf(dest, max, "%.*" suffix, item->precision, val);\
    else\
      snprintf(dest, max, "%*.*" suffix, item->width, item->precision, val);\
  }\
  break

void LogItemToString(char *dest, size_t max, LogItem *item) {
  switch (item->type) {
    GENPRINT2(LIT_INT,       "ld",  item->int_);
    GENPRINT2(LIT_UINT,      "lu",  item->int_);
    GENPRINT2(LIT_UOCT,      "lo",  item->uint_);
    GENPRINT2(LIT_HEX,       "lx",  item->uint_);
    GENPRINT2(LIT_UPCHEX,    "luX", item->uint_);
    GENPRINT2(LIT_FLT,       "f",   item->flt_);
    GENPRINT2(LIT_UPCFLT,    "F",   item->flt_);
    GENPRINT2(LIT_SCIFLT,    "e",   item->flt_);
    GENPRINT2(LIT_SCIUPCFLT, "E",   item->flt_);
    GENPRINT2(LIT_SHRFLT,    "g",   item->flt_);
    GENPRINT2(LIT_SHRUPCFLT, "G",   item->flt_);
    GENPRINT2(LIT_HEXFLT,    "a",   item->flt_);
    GENPRINT2(LIT_UPCHEXFLT, "A",   item->flt_);
    GENPRINT1(LIT_CHR,       "lc",  (unsigned int)item->uint_);
    GENPRINT2(LIT_STR,       "s",   item->str_);
    GENPRINT1(LIT_PTR,       "p",   (void*)item->uint_);
    case      LIT_NIL: *dest = 0; break;
  }
}

static const char *LOG_ITEM_TYPE_NAMES[] = {
  "NIL", "INT", "UINT", "UOCT", "HEX", "UPCHEX",
  "FLT", "UPCFLT", "SCIFLT", "SCIUPCFLT", "SHRFLT",
  "SHRUPCFLT", "HEXFLT", "UPCHEXFLT", "CHR", "STR", "PTR"
};

#undef GENPRINT1
#undef GENPRINT2

void LogItemDump(LogItem item) {
  char data[4096];
  LogItemToString(data, 4096, &item);

  LOG_INFO("%s %u.%u %u:%u %s", LOG_ITEM_TYPE_NAMES[item.type], item.width, item.precision, item.start, item.size, data);
}

void LogEntryDump(LogEntry *e) {
  LOG_INFO("%s:%zu\t%s", e->file, e->line, e->data);

  for (size_t i = 0; i < e->items_len; ++i) {
    LogItemDump(e->items[i]);
  }
}