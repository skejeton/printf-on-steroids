#include "Internal.h"
#include "enet/enet.h"

struct {
  ENetPeer *peer;
  ENetHost *host;
  char data[1 << 14];
  size_t data_len;
}
typedef Client;

static int StartClient(Client *client) {
  ENetAddress address = {0};
  address.host = ENET_HOST_ANY;
  address.port = DEFAULT_PORT;
  enet_address_set_host(&address, "127.0.0.1");

  ENetHost *host = enet_host_create(NULL, 1, DEFAULT_CHAN_CAP, 0, 0);
  if (host == NULL) {
    return 1;
  }

  ENetPeer *peer = enet_host_connect(host, &address, 2, 0);
  if (peer == NULL) {
    return 1;
  }

  ENetEvent event;
  if (enet_host_service (host, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
    LOG_INFO("Connected to server.");
  } else {
    enet_peer_reset(peer);
    enet_host_destroy(host);
    return 1;
  }

  *client = (Client){.peer = peer, .host = host};
  return 0;
}

static void StopClient(Client *client) {
  LOG_INFO("Stopping client.");
  enet_host_flush(client->host);
  enet_peer_disconnect(client->peer, 0);
}

static Client GLOBAL_CLIENT;
static Mutex MUTEX;
static _Atomic(int) CORE_THREAD_RUNNING = 0;
static Thread CORE_THREAD_ID;

// TODO: Reliably send packets by recieving acknowledgments.
static void* CoreThread(void *param) {
  bool running = true;

  while (running) {
    running = CORE_THREAD_RUNNING;

    // Push packets from commands.
    MutexLock(&MUTEX);
    LOG_INFO("Data len is %zu.", GLOBAL_CLIENT.data_len);
    running = running || GLOBAL_CLIENT.data_len > 0;
    int npackets = 0;

    if (GLOBAL_CLIENT.data_len > 0) {
      size_t i = 0;
      while (i < GLOBAL_CLIENT.data_len) {
        void *datum = GLOBAL_CLIENT.data+i;
        size_t datum_size = *(uint32_t*)datum;

        ENetPacket *packet = enet_packet_create(datum, datum_size, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(GLOBAL_CLIENT.peer, 0, packet);
        npackets++;

        i += datum_size;
      }

      GLOBAL_CLIENT.data_len = 0;
    }
    LOG_INFO("Sending %d packets.", npackets);

    MutexUnlock(&MUTEX);

    ENetEvent event;
    LOG_INFO("Servicing events.");
    while (enet_host_service(GLOBAL_CLIENT.host, &event, 5) > 0) {
      switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: 
          LOG_INFO("Server connected again WTF?.");
          break;
        case ENET_EVENT_TYPE_DISCONNECT:
          LOG_INFO("Server disconnected.");
          goto end;
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

end:
  Core_Deinit();

  return NULL;
}

static void SignalHandler(int signal) {
  // TODO: Don't handle deinit after shutdown (unlikely to happen, but still).
  void Core_Deinit();
  Core_Deinit();
  exit(0);
}

int Core_Init() {
  if (CORE_THREAD_RUNNING) {
    return 2;
  }
  if (enet_initialize() != 0) {
    return 3;
  }

  LOG_INFO("Enet initialized.");

  if (StartClient(&GLOBAL_CLIENT)) {
    return 1;
  }

  MutexInit(&MUTEX);
  signal(SIGINT, SignalHandler);
  LOG_INFO("Creating thread.");
  CORE_THREAD_RUNNING = 1;
  ThreadCreate(&CORE_THREAD_ID, CoreThread);
  return 0;
}

void Core_Deinit()
{
  LOG_INFO("Stopping.");
  if (CORE_THREAD_RUNNING) {
    CORE_THREAD_RUNNING = 0;
    ThreadJoin(&CORE_THREAD_ID);
    LOG_INFO("Thread finished.");
  }
  MutexDestroy(&MUTEX);

  ENetEvent event;
  StopClient(&GLOBAL_CLIENT);

  // I need it to ensure all the logs will sync up.
  // This might take some time but this is an ok estimate. 
  // This is a hacky way of doing it, I should fix it. (FIXME)
  while (enet_host_service(GLOBAL_CLIENT.host, &event, 500) > 0) {}

  LOG_INFO("Deinitializing enet.");
  enet_deinitialize();
}

void Core_OutputLog(LogEntry entry) {
  if (CORE_THREAD_RUNNING) {
    MutexLock(&MUTEX);
      const size_t LIMIT = (sizeof GLOBAL_CLIENT.data - GLOBAL_CLIENT.data_len);
      void *origin = LogEntryEncode(entry);
      const size_t OSIZE = PS_PacketSize(origin);
      if (OSIZE >= LIMIT) {
        LOG_ERROR("Push data failed. Size %zu is over limit %zu at length %zu", OSIZE, LIMIT, GLOBAL_CLIENT.data_len);
      }
    

      memcpy(GLOBAL_CLIENT.data+GLOBAL_CLIENT.data_len, origin, OSIZE);
      GLOBAL_CLIENT.data_len += OSIZE;
      free(origin);
    MutexUnlock(&MUTEX);
  }
}

