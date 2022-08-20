#include "common/LogEntry.c"
#include "common/Common.c"

static LogItem MkLogItemInt(int val) { 
  return (LogItem){.int_=val,.type=LIT_INT, .start=1, .size=3};
}

static LogItem MkLogItemStr(char *val) { 
  return (LogItem){.str_=val,.type=LIT_STR, .start=8, .size=1};
}

static LogItem MkLogItemChr(char val) { 
  return (LogItem){.chr_=val,.type=LIT_CHR, .start=10, .size=4};
}

static LogItem MkLogItemNil() { 
  return (LogItem){.type=LIT_NIL, .start=0, .size=1};
}

int main() {
  LogEntry entry = {
    .data = "This is the data.",
    .file = "file_name.c",
    .line = 42,
    .items_len = 5,
    .items = (LogItem[]){MkLogItemStr("Test"), MkLogItemInt(123), MkLogItemStr("Another test"), MkLogItemChr('x'), MkLogItemNil()}
  };

  LogEntryDump(&entry);

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
