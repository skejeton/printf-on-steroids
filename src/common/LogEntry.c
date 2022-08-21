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

    switch (type) {
      case LIT_INT:
        PS_WRITEVAL(ps, &e->items[i].int_);
        break;
      case LIT_CHR:
        PS_WRITEVAL(ps, &e->items[i].chr_);
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
  return origin;
}

LogEntry LogEntryDecode(void *origin) {
  LOG_INFO("Decoding package:");

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

void LogItemDump(LogItem item) {
  switch (item.type) {
    case LIT_CHR:
      LOG_INFO("CHR %u:%u %c", item.start, item.size, item.chr_);
      break;
    case LIT_STR:
      LOG_INFO("STR %u:%u %s", item.start, item.size, item.str_);
      break;    
    case LIT_INT:
      LOG_INFO("INT %u:%u %d", item.start, item.size, item.int_);
      break;     
    case LIT_NIL:
      LOG_INFO("NIL %u:%u", item.start, item.size);
      break;     
  }
}

void LogEntryDump(LogEntry *e) {
  LOG_INFO("%s:%zu\t%s", e->file, e->line, e->data);

  for (size_t i = 0; i < e->items_len; ++i) {
    LogItemDump(e->items[i]);
  }
}