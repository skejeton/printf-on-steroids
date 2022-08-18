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

static void
DumpBinary(void *binary, size_t size) {
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

static PacketStream PS_BeginFree() {
  PacketStream result = {0};
  result.mode = PS_MODE_FREE;
  // Skip packet size. Will be written to during finalization if write.
  return result;

}
static PacketStream PS_BeginWrite() {
  PacketStream result = {0};
  result.mode = PS_MODE_WRITE;
  result.origin = (uint8_t*)malloc(PS_BYTECAP);
  // Skip packet size. Will be written to during finalization if write.
  result.data = (uint8_t*)result.origin + sizeof(uint32_t); 
  return result;
}
static PacketStream PS_BeginRead(void *origin) {
  PacketStream result = {0};
  result.mode = PS_MODE_READ;
  result.origin = (uint8_t*)origin;
  // Skip packet size. Will be written to during finalization if write.
  result.data = (uint8_t*)result.origin + sizeof(uint32_t); 
  return result;
}

static uint32_t PS_PacketSize(void *origin) {
  return *(uint32_t*)origin;
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
  
  switch (ps->mode) {
    case PS_MODE_WRITE:
      memcpy(ps->data, data, nbytes);
      break;
    case PS_MODE_READ:
      memcpy(data, ps->data, nbytes);
      break;
    default:
      break;
  }

  ps->data = (uint8_t*)ps->data+nbytes;
}

static void PS_WritePointer(PacketStream *ps, void **data, size_t nbytes) {
  const size_t NEW_SIZE = ((uint8_t*)ps->data-(uint8_t*)ps->origin)+nbytes+sizeof(uint32_t);
  if (NEW_SIZE >= PS_BYTECAP) {
    LOG_ERROR("Packet stream is overflowed by %zu bytes. That means I need to dyanmically resize it FFS!", NEW_SIZE-PS_BYTECAP);
  }
  
  uint32_t *size_ptr = (uint32_t*)ps->data;
  ps->data += sizeof *size_ptr;

  switch (ps->mode) {
    case PS_MODE_WRITE:
      *size_ptr = nbytes;
      memcpy(ps->data, *data, nbytes);
      ps->data += nbytes;
      break;
    case PS_MODE_READ:
      *data = malloc(*size_ptr);
      memcpy(*data, ps->data, *size_ptr);
      ps->data += *size_ptr;
      break;
    case PS_MODE_FREE:
      free(*data);
      ps->data += nbytes;
      break;
  }
}

static void PS_WriteString(PacketStream *ps, char **str) {
  return PS_WritePointer(ps, (void**)str, ps->mode == PS_MODE_READ ? 0 : strlen(*str)+1);
}

#define PS_CANFIT(ps, size) 
#define PS_WRITEVAL(ps, data) PS_WriteBytes((ps), (data), sizeof(*(data)))
#define PS_WRITESTR(ps, str) PS_WriteString((ps), (str))

#endif
