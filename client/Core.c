#include <bits/types/stack_t.h>
#include <enet/enet.h>
#include <pthread.h>
#include <stdbool.h>
#include "include/Common.h"
#include <signal.h>
#include <string.h>

struct {
  ENetPeer *peer;
  ENetHost *host;
  bool connected;
  char data[1 << 12];
  size_t data_len;
}
typedef Client;

static Client StartClient() {
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

static void StopClient(Client *client) {
  LOG_INFO("Stopping client.");
  enet_peer_disconnect_now(client->peer, 0);
  enet_host_destroy(client->host);
}

static void SignalHandler(int signal) {
  // TODO: Don't handle deinit after shutdown (unlikely to happen, but still).
  void Core_Deinit();
  Core_Deinit();
  exit(0);
}

static Client GLOBAL_CLIENT;
static pthread_mutex_t MUTEX;
_Atomic(int) CORE_THREAD_RUNNING = 1;
pthread_t CORE_THREAD_ID;
unsigned short start_seq_number = 0;

// TODO: Reliably send packets by recieving acknowledgments.
static void* CoreThread(void *param) {
  bool running = true;

  while (running) {
    running = CORE_THREAD_RUNNING;

    // Push packets from commands.
    pthread_mutex_lock(&MUTEX);
    LOG_INFO("Data len is %zu.", GLOBAL_CLIENT.data_len);
    running = running || GLOBAL_CLIENT.data_len > 0;
    int npackets = 0;
    if (GLOBAL_CLIENT.connected) {
      start_seq_number = GLOBAL_CLIENT.peer->channels[0].outgoingReliableSequenceNumber;

      if (GLOBAL_CLIENT.data_len > 0) {
        size_t i = 0;
        while (i < GLOBAL_CLIENT.data_len) {
          char *datum = GLOBAL_CLIENT.data+i;
          size_t datum_size = strlen(datum)+1;

          ENetPacket *packet = enet_packet_create(datum, strlen(datum)+1, ENET_PACKET_FLAG_RELIABLE);
          enet_peer_send(GLOBAL_CLIENT.peer, 0, packet);
          npackets++;

          i += datum_size;
        }

        GLOBAL_CLIENT.data_len = 0;
      }
      LOG_INFO("Sending %d packets.", npackets);
    }

    pthread_mutex_unlock(&MUTEX);

    ENetEvent event;
    LOG_INFO("Servicing events.");
    while (enet_host_service(GLOBAL_CLIENT.host, &event, 10*(npackets+1)) > 0) {
      switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: 
          LOG_INFO("Server connected.");
          GLOBAL_CLIENT.connected = true;
          break;
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

    LOG_INFO("Done servicing events.");
  }

  StopClient(&GLOBAL_CLIENT);

  LOG_INFO("Deinitializing enet.");
  enet_deinitialize();

  return NULL;
}

void Core_Init() {
  if (enet_initialize() != 0) {
    LOG_ERROR("Failed to initialize enet.");
  }

  LOG_INFO("Enet initialized.");

  GLOBAL_CLIENT = StartClient();

  pthread_mutex_init(&MUTEX, NULL);
  signal(SIGINT, SignalHandler);
  LOG_INFO("Creating thread.");
  pthread_create(&CORE_THREAD_ID, NULL, CoreThread, NULL);
}

void Core_OutputLog(const char *text) {
  pthread_mutex_lock(&MUTEX);
    size_t len = strlen(text) + 1;
    // NOTE: Sizeof relies on static size
    if (len >= (sizeof GLOBAL_CLIENT.data - GLOBAL_CLIENT.data_len)) {
      LOG_ERROR("Command buffer overflow.");
    }
    memcpy(GLOBAL_CLIENT.data+GLOBAL_CLIENT.data_len, text, len);
    LOG_INFO("Putting data of size %zu at %zu.", len, GLOBAL_CLIENT.data_len);
    GLOBAL_CLIENT.data_len += len;
  pthread_mutex_unlock(&MUTEX);
}

void Core_Deinit() {
  LOG_INFO("Stopping.");
  CORE_THREAD_RUNNING = 0;
  pthread_join(CORE_THREAD_ID, NULL);
  LOG_INFO("Thread finished.");
  pthread_mutex_destroy(&MUTEX);
}