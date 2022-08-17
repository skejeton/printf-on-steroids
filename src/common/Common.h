#ifndef H_COMMON
#define H_COMMON

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>

#ifndef DISABLE_LOGS
#define LOG_ERROR(...) (printf("\x1b[31mLOG_ERROR\x1b[0m: " __VA_ARGS__), printf("\n"), exit(-1))
#define LOG_INFO(...) (printf("\x1b[34mLOG_INFO\x1b[0m: " __VA_ARGS__), printf("\n"))
#endif

#define DEFAULT_PORT       4891
#define DEFAULT_CLIENT_CAP 32
#define DEFAULT_CHAN_CAP   2

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
  uint8_t *origin;
  uint8_t *data;
};

static PacketStream PS_BeginWrite() {
  PacketStream result = {0};
  result.read = false;
  result.origin = (uint8_t*)malloc(PS_BYTECAP);
  // Skip packet size. Will be written to during finalization.
  result.data = (uint8_t*)result.origin + sizeof(uint32_t); 
  return result;
}

static PacketStream PS_BeginRead(void *source) {
  PacketStream result = {0};
  result.read = true;
  result.origin = (uint8_t*)source;
  // Skip packet size. 
  result.data = (uint8_t*)result.origin + sizeof(uint32_t); 

  return result;
}

/* PS_FinalizeWrite()
// Stops writing data to the stream and returns the raw data owned.
// The data is allocated with malloc.
// Don't call this when you are doing a read stream.
*/
static void* PS_FinalizeWrite(PacketStream *ps) {
  // Write packet size.
  *(uint32_t*)ps->origin = (uint32_t)(ps->data-ps->origin);
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

  ps->data = (uint8_t*)ps->data+nbytes;
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

  ps->data = (uint8_t*)ps->data+nbytes;
}

static void PS_WriteString(PacketStream *ps, char **str) {
  if (ps->read) {
    PS_WritePointer(ps, (void**)str, strlen((char*)ps->data)+1);
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


#endif
