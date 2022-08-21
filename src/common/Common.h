#ifndef H_COMMON
#define H_COMMON

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>
#include <ctype.h>

#ifndef DISABLE_LOGS
#define LOG_ERROR(...) (printf("\x1b[31mLOG_ERROR\x1b[0m: " __VA_ARGS__), printf("\n"), exit(-1))
#define LOG_INFO(...) (printf("\x1b[34mLOG_INFO\x1b[0m: " __VA_ARGS__), printf("\n"))
#endif

#define DEFAULT_PORT       4891
#define DEFAULT_CLIENT_CAP 32
#define DEFAULT_CHAN_CAP   2

void DumpBinary(void *binary, size_t size);

enum {
  PS_MODE_WRITE,
  PS_MODE_READ,
  PS_MODE_FREE
}
typedef PacketStreamMode;

#define PS_BYTECAP 512
struct PacketStream typedef PacketStream;
struct PacketStream {
  uint8_t *origin;
  uint8_t *data;
  PacketStreamMode mode;
};

PacketStream PS_BeginFree();
PacketStream PS_BeginWrite();
PacketStream PS_BeginRead(void *origin);
uint32_t PS_PacketSize(void *origin);

/* PS_FinalizeWrite()
// Stops writing data to the stream and returns the raw data owned.
// The data is allocated with malloc.
// Don't call this when you are doing a read stream.
*/
void* PS_FinalizeWrite(PacketStream *ps);
void* PS_WriteBytes(PacketStream *ps, void *data, size_t nbytes);
void PS_WritePointer(PacketStream *ps, void **data, size_t nbytes);
uint32_t PS_WriteLen(PacketStream *ps, uint32_t *len, void **data, size_t datum_size, uint32_t **data_out);
void PS_WriteString(PacketStream *ps, char **str);

#define PS_WRITEVAL(ps, data) PS_WriteBytes((ps), (data), sizeof(*(data)))
#define PS_LIST(i, ps, data, length) for (uint32_t i = 0, *x, length_ = PS_WriteLen((ps), (length), ((void**)data), sizeof **(data), &x); i < length_ ? 1 : (free(x), 0); ++i) 
#define PS_WRITEPTR(ps, ptr, size) PS_WritePointer((ps), ((void**)ptr), (size))
#define PS_WRITESTR(ps, str) PS_WriteString((ps), (str))

////////////////////////////////////////////
// LogEntry.c

#include "Common.h"

enum LogItemType {
  LIT_NIL,
  LIT_STR,
  LIT_CHR,
  LIT_INT,
}
typedef LogItemType;

struct LogItem typedef LogItem;
struct LogItem {
  LogItemType type;
  uint32_t start, size;
  union {
    char chr_;
    char *str_;
    int int_;
  };
};

struct LogEntry typedef LogEntry;
struct LogEntry {
  uint64_t line;
  char    *file;
  char    *data;
  char    *pre_formatted_data;
  uint32_t items_len;
  LogItem *items;
};

void * LogEntryEncode(LogEntry entry);

LogEntry LogEntryDecode(void *origin);

// LogEntryDeinit(entry)
// Deinitializes data within LogEntry,
// This assumes the log entry was allocated by PacketStream.
void LogEntryDeinit(LogEntry *entry);
void LogItemDump(LogItem item);
void LogEntryDump(LogEntry *e);

#endif