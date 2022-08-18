#include <common/LogEntry.h>
#include <ctype.h>

void DumpLogEntry(LogEntry *e) {
  LOG_INFO("%s:%zu\t%s", e->file, e->line, e->data);
}

int main() {
  LogEntry entry = {
    .data = "This is the data.",
    .file = "file_name.c",
    .line = 42
  };

  DumpLogEntry(&entry);

  void *origin = LogEntryEncode(entry);

  size_t size = PS_PacketSize(origin);
  LOG_INFO("Packet size is %zu.", size);
  DumpBinary(origin, size);

  LOG_INFO("Unpacking the data.");
  LogEntry e = LogEntryDecode(origin);

  LOG_INFO("Unpacked entry is:");
  DumpLogEntry(&entry);
}
