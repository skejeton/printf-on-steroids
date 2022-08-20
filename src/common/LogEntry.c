#include "Common.h"

static void ConvLogEntry(PacketStream *ps, LogEntry *e) {
  PS_WRITEVAL(ps, &e->line);
  PS_WRITESTR(ps, &e->file);
  PS_WRITESTR(ps, &e->data);
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
// LogEntryDeinit(entry)
// Deinitializes data within LogEntry,
// This assumes the log entry was allocated by PacketStream.
void LogEntryDeinit(LogEntry *entry) {
  PacketStream ps = PS_BeginFree();
  ConvLogEntry(&ps, entry);
}