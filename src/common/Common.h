#ifndef H_COMMON
#define H_COMMON

#define DISABLE_LOGS

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>
#include <ctype.h>

#define LOG_ERROR(...) (printf("\x1b[31mLOG_ERROR\x1b[0m: " __VA_ARGS__), printf("\n"), exit(-1))

#ifndef DISABLE_LOGS
#define IFDEBUG(...) (__VA_ARGS__)
#define LOG_WARN(...) (printf("\x1b[35mLOG_WARN\x1b[0m: " __VA_ARGS__), printf("\n"), exit(-1))
#define LOG_INFO(...) (printf("\x1b[34mLOG_INFO\x1b[0m: " __VA_ARGS__), printf("\n"))
#else
#define IFDEBUG(...) 
#define LOG_WARN(...) 
#define LOG_INFO(...)
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

// Prefixes:
//   U   :: Unsigned.
//   UPC :: Uppercase. 
//   SCI :: Scientific notation.
//   SHR :: Shortest representation.
//
// The comment after each entry tells the filed to access.
//
// NOTE: We don't include %n, because it's practically impossible.
//       There's no real point in it either, as it's only useful if the library is used as
//       sprintf style fucntion. Which isn't the case here.
enum LogItemType {
  LIT_NIL,        // 
  LIT_INT,        // int_
  LIT_UINT,       // uint_
  LIT_UOCT,       // uint_
  LIT_HEX,        // uint_
  LIT_UPCHEX,     // uint_
  LIT_FLT,        // flt_
  LIT_UPCFLT,     // flt_
  LIT_SCIFLT,     // flt_
  LIT_SCIUPCFLT,  // flt_
  LIT_SHRFLT,     // flt_
  LIT_SHRUPCFLT,  // flt_
  LIT_HEXFLT,     // flt_
  LIT_UPCHEXFLT,  // flt_
  LIT_CHR,        // int_
  LIT_STR,        // str_
  LIT_PTR,        // uint_
}
typedef LogItemType;


// Some of the flags here are mutually exclusive, behaviour of their combination *is not defined*. 
enum {
  // Pad text left instead of right.
  LIF_PAD_LEFT     = 0x1, // -
  // Forces to display sign in numbers.
  LIF_FORCE_SIGN   = 0x2, // +
  // Pads with space if no sign is enabled.
  LIF_PAD_SIGN    = 0x4, //  
  // Number decorations include:
  //   * %x,%X                 :: Prefix with 0x
  //   * %o                    :: Prefix with 0
  //   * %F,%e,%E,%g,%G,%a,%A  :: Always include floating point dot.
  LIF_NUMBER_DECOR = 0x8, // # 
  LIF_PAD_ZERO = 0x10     // 0
};

struct LogItem typedef LogItem;
struct LogItem {
  uint32_t width;
  uint32_t precision;
  uint32_t flags;
  LogItemType type;
  uint32_t start, size;
  union {
    double flt_;
    char *str_;
    int64_t int_;
    uint64_t uint_;
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
void LogItemToString(char *dest, size_t max, LogItem *item);

#endif