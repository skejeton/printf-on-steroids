
#ifndef H_LOG
#define H_LOG

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct LogEntry typedef LogEntry;
struct LogEntry {
  uint64_t line;
  char    *file;
  size_t   data_size;
  void    *data;
};

// For now implementation lives here

static inline void *
LogEntryEncode(LogEntry entry, uint64_t *out_size) {
  size_t hdr_size = sizeof(uint64_t) + sizeof(char *) + sizeof(size_t);
  size_t str_size = strlen(entry.file);
  // Allocate log entry
  size_t log_entry_size = hdr_size + entry.data_size + str_size;
  void *log_entry = malloc(log_entry_size);
  size_t data_offs = 0+hdr_size;
  size_t str_offs = data_offs + entry.data_size;
  // Write the data
  ((uint64_t *)log_entry)[0] = entry.line;
  ((uint64_t *)log_entry)[1] = str_offs;
  ((uint64_t *)log_entry)[2] = entry.data_size;
  memcpy((uint8_t *)log_entry+data_offs, entry.data, entry.data_size);
  memcpy((uint8_t *)log_entry+str_offs,  entry.file, str_size);
  *out_size = log_entry_size;
  return log_entry;
}

static inline LogEntry
LogEntryDecode(size_t data_size, void *data) {
  size_t hdr_size = sizeof(uint64_t) + sizeof(char *) + sizeof(size_t);
  LogEntry entry;
  entry.line = ((uint64_t *)data)[0];
  entry.file = (void *)(data + ((uint64_t *)data)[1]);
  entry.data_size = ((uint64_t *)data)[2];
  entry.data = (void *)((uint8_t *)data + hdr_size);
  return entry;
}

#endif // H_LOG
