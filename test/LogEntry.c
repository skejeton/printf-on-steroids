#include "common/LogEntry.c"
#include "common/Common.c"


int main() {
  LogEntry entry = {
    .data = "This is the data.",
    .file = "file_name.c",
    .pre_formatted_data = "This is the data. Pre-formatted string.",
    .line = 42,
  };

  void *origin = LogEntryEncode(entry);

  size_t size = PS_PacketSize(origin);
  LOG_INFO("Packet size is %zu.", size);
  DumpBinary(origin, size);

  LOG_INFO("Unpacking the data.");
  LogEntry e = LogEntryDecode(origin);

  LOG_INFO("Unpacked entry is:");
  LogEntryDump(&entry);

  free(origin);
  LogEntryDeinit(&e);
}
