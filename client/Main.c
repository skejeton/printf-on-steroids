#include "src/Common.h"
#include <enet/enet.h>

int main() {
  if (enet_initialize() != 0) {
    ERROR("Failed to initialize enet.");
  }

  INFO("Enet initialized.");

  ENetAddress address = {0};
  address.host = ENET_HOST_ANY;
  address.port = DEFAULT_PORT;
  enet_address_set_host(&address, "127.0.0.1");

  ENetHost *host = enet_host_create(NULL, 1, DEFAULT_CHAN_CAP, 0, 0);
  if (host == NULL) {
    ERROR("Can not create host.");
  }

  ENetPeer *peer = enet_host_connect(host, &address, 2, 0);

  if (peer == NULL) {
    ERROR("Failed to create peer.");
  }

  ENetEvent event;

  char text[] = "amogus";

  while (enet_host_service(host, &event, 5000) > 0) {
    switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT: {
        INFO("Server connected.");
        ENetPacket *packet = enet_packet_create(text, sizeof text, 0);
        enet_peer_send(peer, 0, packet);
      } break;
      case ENET_EVENT_TYPE_DISCONNECT:
        INFO("Server disconnected.");
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        INFO("Recieved data.");
        break;
      case ENET_EVENT_TYPE_NONE:
        INFO("None event.");
        break;
    }
  }

  enet_peer_reset(peer);
  enet_host_destroy(host);

  INFO("Deinitializing enet.");
  enet_deinitialize();
  return 0;
}