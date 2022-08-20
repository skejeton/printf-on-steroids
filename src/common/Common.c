#include "Common.h"
#include <stdlib.h>

void
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

PacketStream PS_BeginFree() {
  PacketStream result = {0};
  result.mode = PS_MODE_FREE;
  // Skip packet size. Will be written to during finalization if write.
  return result;

}
PacketStream PS_BeginWrite() {
  PacketStream result = {0};
  result.mode = PS_MODE_WRITE;
  result.origin = (uint8_t*)malloc(PS_BYTECAP);
  // Skip packet size. Will be written to during finalization if write.
  result.data = (uint8_t*)result.origin + sizeof(uint32_t); 
  return result;
}
PacketStream PS_BeginRead(void *origin) {
  PacketStream result = {0};
  result.mode = PS_MODE_READ;
  result.origin = (uint8_t*)origin;
  // Skip packet size. Will be written to during finalization if write.
  result.data = (uint8_t*)result.origin + sizeof(uint32_t); 
  return result;
}

uint32_t PS_PacketSize(void *origin) {
  return *(uint32_t*)origin;
}

/* PS_FinalizeWrite()
// Stops writing data to the stream and returns the raw data owned.
// The data is allocated with malloc.
// Don't call this when you are doing a read stream.
*/
void* PS_FinalizeWrite(PacketStream *ps) {
  // Write packet size.
  *(uint32_t*)ps->origin = (uint32_t)(ps->data-ps->origin);
  return ps->origin;
}

void* PS_WriteBytes(PacketStream *ps, void *data, size_t nbytes) {
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
  return data;
}

void PS_WritePointer(PacketStream *ps, void **data, size_t nbytes) {
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

uint32_t PS_WriteLen(PacketStream *ps, uint32_t *len, void **data, size_t datum_size, uint32_t **data_out) {
  uint32_t length = *(uint32_t*)PS_WriteBytes(ps, len, sizeof(uint32_t));
  if (ps->mode == PS_MODE_READ) { 
    *data = malloc(datum_size * length);
  }

  *data_out = (uint32_t*)(ps->mode == PS_MODE_FREE ? *data : NULL);
  return length;
}

void PS_WriteString(PacketStream *ps, char **str) {
  return PS_WritePointer(ps, (void**)str, ps->mode == PS_MODE_READ ? 0 : strlen(*str)+1);
}
