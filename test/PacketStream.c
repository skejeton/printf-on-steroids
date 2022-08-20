#include <common/Common.c>

struct Test typedef Test;
struct Test {
  uint32_t len;
  char **strings;
};

void ConvTest(PacketStream *ps, Test *test) {
  PS_LIST(i, ps, &test->strings, &test->len) {
    PS_WRITESTR(ps, &test->strings[i]);
  }
}

char *STRINGS[] = {
  "Hello", "World", "Programmed", "To", "Work", "And", "Not", "To", "Feel."
};

void DumpTest(Test *test) {
  printf("Test: ");
  for (int i = 0; i < test->len; ++i) {
    printf("%s ", test->strings[i]);
  }
  printf("\n");
}

int main() {
  void *origin;
  Test test_out, test_in = {
    .len = sizeof STRINGS/sizeof STRINGS[0],
    .strings = STRINGS
  };
  DumpTest(&test_in);

  // Write
  {
    PacketStream ps = PS_BeginWrite();
    ConvTest(&ps, &test_in);
    origin = PS_FinalizeWrite(&ps);
    DumpBinary(origin, PS_PacketSize(origin));
  }

  // Read
  {
    PacketStream ps = PS_BeginRead(origin);
    ConvTest(&ps, &test_out);
    DumpTest(&test_out);
  }

  // Free
  {
    PacketStream ps = PS_BeginFree();
    ConvTest(&ps, &test_out);
  }

  return 0;
}