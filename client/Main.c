#include "src/Common.h"
#include <enet/enet.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

struct {
  ENetPeer *peer;
  ENetHost *host;
  bool connected;
}
typedef Client;

Client StartClient() {
  ENetAddress address = {0};
  address.host = ENET_HOST_ANY;
  address.port = DEFAULT_PORT;
  enet_address_set_host(&address, "127.0.0.1");

  ENetHost *host = enet_host_create(NULL, 1, DEFAULT_CHAN_CAP, 0, 0);
  if (host == NULL) {
    LOG_ERROR("Can not create host.");
  }

  ENetPeer *peer = enet_host_connect(host, &address, 2, 0);

  if (peer == NULL) {
    LOG_ERROR("Failed to create peer.");
  }

  return (Client){.peer = peer, .host = host};
}

void StopClient(Client *client) {
  LOG_INFO("Stopping client.");
  enet_peer_disconnect_now(client->peer, 0);
  enet_host_destroy(client->host);
}

Client GLOBAL_CLIENT;

void SignalHandler(int signal) {
  // TODO: Don't handle deinit after shutdown (unlikely to happen, but still)
  void P2_Deinit();
  P2_Deinit();
  exit(0);
}

void P2_HandleEvents(int time) {
  ENetEvent event;
  while (enet_host_service(GLOBAL_CLIENT.host, &event, time) > 0) {
    switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT: {
        LOG_INFO("Server connected.");
        GLOBAL_CLIENT.connected = true;
      } break;
      case ENET_EVENT_TYPE_DISCONNECT:
        LOG_INFO("Server disconnected.");
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        LOG_INFO("Recieved data.");
        break;
      case ENET_EVENT_TYPE_NONE:
        LOG_INFO("None event.");
        break;
    }
  }
}

void P2_Init() {
  if (enet_initialize() != 0) {
    LOG_ERROR("Failed to initialize enet.");
  }

  LOG_INFO("Enet initialized.");

  GLOBAL_CLIENT = StartClient();
  signal(SIGINT, SignalHandler);
  P2_HandleEvents(3000);
}

void P2_OutputLog(const char *text) {
  ENetPacket *packet = enet_packet_create(text, strlen(text)+1, 0);
  enet_peer_send(GLOBAL_CLIENT.peer, 0, packet);

  P2_HandleEvents(100);
}

void P2_Deinit() {
  StopClient(&GLOBAL_CLIENT);

  LOG_INFO("Deinitializing enet.");
  enet_deinitialize();
}

void P2_Printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char log[512];
  vsnprintf(log, 512, fmt, ap);
  va_end(ap);
  P2_OutputLog(log);
}

int main() {
  P2_Init();

#if 0
  P2_Printf("Oh and also, the logs preserve across runs.");
#else




  P2_Printf("Printf2 (Printf on steroids).");
  P2_Printf("This is a library/GUI package that enchances printf debugging.");
  P2_Printf("The main drawback of printf is that the data is output into the console.");
  P2_Printf("This library allows you to sort the logs.");
  P2_Printf("It's planned for this library to also visualize things printf can't do, like bitmaps.");
  P2_Printf("$For now in sorting properties you can choose to either sort by inclusion.");
  P2_Printf("Or_by_exclusion");
  P2_Printf("@Or_by_both");
  P2_Printf("That's all I have for now! Cheers :P");
#endif

  P2_Deinit();
  return 0;
}