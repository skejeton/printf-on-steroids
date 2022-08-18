#ifndef H_LOGENTRY
#define H_LOGENTRY

#include "Common.h"

struct LogEntry typedef LogEntry;
struct LogEntry {
  uint64_t line;
  char    *file;
  char    *data;
};

static void ConvLogEntry(PacketStream *ps, LogEntry *e) {
  PS_WRITEVAL(ps, &e->line);
  PS_WRITESTR(ps, &e->file);
  PS_WRITESTR(ps, &e->data);
}

// For now implementation lives here

static inline void *
LogEntryEncode(LogEntry entry) {
  PacketStream ps = PS_BeginWrite();
  ConvLogEntry(&ps, &entry);
  void *origin = PS_FinalizeWrite(&ps);
  LOG_INFO("Encoded package:");
  return origin;
}

static inline LogEntry
LogEntryDecode(void *origin) {
  LOG_INFO("Decoding package:");

  PacketStream ps = PS_BeginRead(origin);
  LogEntry result = {0};
  ConvLogEntry(&ps, &result);
  return result;
}
// LogEntryDeinit(entry)
// Deinitializes data within LogEntry,
// This assumes the log entry was allocated by PacketStream.
static inline void
LogEntryDeinit(LogEntry *entry) {
  PacketStream ps = PS_BeginFree();
  ConvLogEntry(&ps, entry);
}
#endif