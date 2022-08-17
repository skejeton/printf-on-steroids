
#ifndef H_LOG
#define H_LOG

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>
#include "Common.h"

struct LogEntry typedef LogEntry;
struct LogEntry {
  uint64_t line;
  char    *file;
  char    *data;
};

#define PS_BYTECAP 512
struct PacketStream typedef PacketStream;
struct PacketStream {
  bool read;
  void *origin;
  void *data;
};

static PacketStream PS_BeginWrite() {
  PacketStream result = {0};
  result.read = false;
  result.origin = malloc(PS_BYTECAP);
  // Skip packet size. Will be written to during finalization.
  result.data = result.origin + sizeof(uint32_t); 
  return result;
}

static PacketStream PS_BeginRead(void *source) {
  PacketStream result = {0};
  result.read = true;
  result.origin = source;
  // Skip packet size. Will be written to during finalization.
  result.data = result.origin + sizeof(uint32_t); 
  return result;
}

/* PS_FinalizeWrite()
// Stops writing data to the stream and returns the raw data owned.
// The data is allocated with malloc.
// Don't call this when you are doing a read stream.
*/
static void* PS_FinalizeWrite(PacketStream *ps) {
  // Write packet size.
  *(uint32_t*)ps->origin = (uint8_t*)ps->data-(uint8_t*)ps->origin;
  return ps->origin;
}

static void PS_WriteBytes(PacketStream *ps, void *data, size_t nbytes) {
  const size_t NEW_SIZE = ((uint8_t*)ps->data-(uint8_t*)ps->origin)+nbytes;
  if (NEW_SIZE >= PS_BYTECAP) {
    LOG_ERROR("Packet stream is overflowed by %zu bytes. That means I need to dyanmically resize it FFS!", NEW_SIZE-PS_BYTECAP);
  }

  if (ps->read) {
    memcpy(data, ps->data, nbytes);
  } else {
    memcpy(ps->data, data, nbytes);
  }
  ps->data += nbytes;
}

static void PS_WritePointer(PacketStream *ps, void **data, size_t nbytes) {
  const size_t NEW_SIZE = ((uint8_t*)ps->data-(uint8_t*)ps->origin)+nbytes;
  if (NEW_SIZE >= PS_BYTECAP) {
    LOG_ERROR("Packet stream is overflowed by %zu bytes. That means I need to dyanmically resize it FFS!", NEW_SIZE-PS_BYTECAP);
  }

  if (ps->read) {
    *data = malloc(nbytes);
    memcpy(*data, ps->data, nbytes);
  } else {
    memcpy(ps->data, *data, nbytes);
  }
  ps->data += nbytes;
}

static void PS_WriteString(PacketStream *ps, char **str) {
  if (ps->read) {
    PS_WritePointer(ps, (void**)str, strlen(ps->data)+1);
  } else {
    PS_WritePointer(ps, (void**)str, strlen(*str)+1);
  }
}

static uint32_t PS_PacketSize(void *origin) {
  return *(uint32_t*)origin;
}

#define PS_CANFIT(ps, size) 
#define PS_WRITEVAL(ps, data) PS_WriteBytes((ps), (data), sizeof(*(data)))
#define PS_WRITESTR(ps, str) PS_WriteString((ps), (str))

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
  return PS_FinalizeWrite(&ps);
}

static inline LogEntry
LogEntryDecode(void *origin) {
  PacketStream ps = PS_BeginRead(origin);
  LogEntry result = {0};
  ConvLogEntry(&ps, &result);
  return result;
}

/*
static inline LogEntry
LogEntryDecode(size_t data_size, void *data) {
  size_t hdr_size = sizeof(uint64_t) + sizeof(char *) + sizeof(size_t);
  LogEntry entry;
  entry.line = ((uint64_t *)data)[0];
  entry.file = (char*)data + ((uint64_t *)data)[1];
  entry.data_size = ((uint64_t *)data)[2];
  entry.data = (void *)((uint8_t *)data + hdr_size);
  return entry;
}
*/

#endif // H_LOG
