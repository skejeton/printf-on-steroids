#include "MumboJumbo.c"
#include <enet/enet.h>

int main() {
  if (enet_initialize() != 0) {
    ERROR("Failed to initialize enet.");
  }

  INFO("Enet initialized.");

  return 0;
}