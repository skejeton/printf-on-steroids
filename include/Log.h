
#ifndef H_LOG
#define H_LOG

#include <stdint.h>
#include <stdlib.h>

struct LogEntry typedef LogEntry;
struct LogEntry {
  uint64_t src_line;
  uint64_t src_file_offs;
  size_t   data_size;
};

// For now implementation lives here

static inline LogEntry *
LogEntryCreate_(uint64_t src_line, char *src_file, size_t data_size, void *data)
{
  // Calculate offsets
  size_t src_file_len = strlen(src_file);
  size_t data_offs = sizeof(LogEntry);
  size_t src_file_offs = data_offs + data_size;
  size_t packet_size = sizeof(LogEntry) + data_size + (src_file_len+1);
  // Create the header
  LogEntry *entry = malloc(packet_size);
  entry->src_line = src_line;
  entry->src_file_offs = src_file_offs;
  entry->data_size = data_size;
  // Write the data for the data and the filename
  void *header_data = (uint8_t *)entry + data_offs;
  memcpy(header_data, data, data_size);
  void *src_file_data = (uint8_t *)entry + src_file_offs;
  memcpy(src_file_data, src_file, src_file_len+1);
  return entry;
}

static inline void *
LogEntryData(LogEntry *entry, size_t *out_data_size) {
  void *header_data = (uint8_t *entry) + sizeof(LogEntry);
  *out_data_size = entry->data_size;
}

static inline char *
LogEntryLocation(LogEntry *entry, uint64_t *out_line) {
  void *src_file = (uint8_t *)entry + entry->src_file_offs;
  *out_line = entry->src_line;
  return src_file;
}

#endif // H_LOG
