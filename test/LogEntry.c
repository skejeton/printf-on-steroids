#include "include/Log.h"
#include <ctype.h>

void DumpBinary(void *binary, size_t size) {
  const int CELLS_PER_ROW = 80/4;

  for (size_t i = 0; i < size; i += CELLS_PER_ROW) {
    const size_t CELLS = (size - i) > CELLS_PER_ROW ? CELLS_PER_ROW : (size - i);

    printf("%04zx ", i);
    for (size_t j = 0; j < CELLS; ++j) {
      uint8_t datum = ((uint8_t*)binary)[i+j];
      if (isprint(datum)) {
        printf("%c  ", datum);
      } else {
        printf(".  ");
      }
    }
    printf("\n%04zx ", i);
    for (size_t j = 0; j < CELLS; ++j) {
      printf("%02x ", ((uint8_t*)binary)[i+j]);
    }
    printf("\n");
  }
}

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