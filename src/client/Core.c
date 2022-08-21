#include "Internal.h"
#include "enet/enet.h"

struct {
  uint8_t *buffer;
  size_t size;
} 
typedef PacketQueue;

void PacketQueue_Clear(PacketQueue *queue) {
  queue->size = 0;
}

int PacketQueue_HasData(PacketQueue *queue) {
  return queue->size > 0;
}

void PacketQueue_Insert(PacketQueue *queue, void *data, size_t size) {
  queue->buffer = (uint8_t*)realloc(queue->buffer, queue->size + size);
  memcpy(queue->buffer+queue->size, data, size);
  queue->size += size;
}

void PacketQueue_Deinit(PacketQueue *queue) {
  free(queue->buffer);
}

struct {
  // Not protected under Mutex.
  // Access after thread is done.
  ENetPeer *peer;
  ENetHost *host;
  
  // Protected under Mutex.
  PacketQueue queue;
  bool is_running;
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
    enet_host_destroy(host);
    return 1;
  }

  bool connected = false;

  ENetEvent event;
  for (int i = 0; !connected && i < 5; ++i) {
    if (enet_host_service(host, &event, 1000) > 0) {
      if (event.type == ENET_EVENT_TYPE_CONNECT) {
        LOG_INFO("Connected to server.");
        connected = true;
      } else {
        break;
      }
    }
  }

  if (!connected) {
    enet_peer_reset(peer);
    enet_host_destroy(host);
    return 1;
  }


  *client = (Client){.peer = peer, .host = host, .is_running = true};
  return 0;
}

static void StopClient(Client *client) {
  LOG_INFO("Stopping client.");
  PacketQueue_Deinit(&client->queue);
  enet_host_flush(client->host);
  enet_peer_disconnect(client->peer, 0);
  enet_host_destroy(client->host);
}

static Client GLOBAL_CLIENT;
static Mutex MUTEX;
static _Atomic(int) CORE_THREAD_RUNNING = 0;
static Thread CORE_THREAD_ID;

// TODO: Reliably send packets by recieving acknowledgments.
static void* CoreThread(void *param) {
  bool running = true;

  while (running) {
    MutexLock(&MUTEX);

    running = GLOBAL_CLIENT.is_running;

    LOG_INFO("Data len is %zu.", GLOBAL_CLIENT.queue.size);
    running = running || PacketQueue_HasData(&GLOBAL_CLIENT.queue);
    int npackets = 0;

    // Push packets from commands.
    if (PacketQueue_HasData(&GLOBAL_CLIENT.queue)) {
      size_t i = 0;
      while (i < GLOBAL_CLIENT.queue.size) {
        void *datum = GLOBAL_CLIENT.queue.buffer+i;
        size_t datum_size = *(uint32_t*)datum;

        ENetPacket *packet = enet_packet_create(datum, datum_size, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(GLOBAL_CLIENT.peer, 0, packet);
        npackets++;

        i += datum_size;
      }

      PacketQueue_Clear(&GLOBAL_CLIENT.queue);
    }
    LOG_INFO("Sending %d packets.", npackets);

    MutexUnlock(&MUTEX);

    ENetEvent event;
    LOG_INFO("Servicing events.");
    while (enet_host_service(GLOBAL_CLIENT.host, &event, 16) > 0) {
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
  LOG_INFO("Thread finished.");
  MutexLock(&MUTEX);
  GLOBAL_CLIENT.is_running = false;

  ENetEvent event;

  // I need it to ensure all the logs will sync up.
  // This might take some time but this is an ok estimate. 
  // This is a hacky way of doing it, I should fix it. (FIXME)
  while (enet_host_service(GLOBAL_CLIENT.host, &event, 500) > 0) {}
  StopClient(&GLOBAL_CLIENT);

  LOG_INFO("Deinitializing enet.");
  enet_deinitialize();

  MutexUnlock(&MUTEX);
  CORE_THREAD_RUNNING = 0;
  return NULL;
}

int Core_Init() {
  if (CORE_THREAD_RUNNING) {
    return 2;
  }

  GLOBAL_CLIENT = (Client){0};

  if (enet_initialize() != 0) {
    return 3;
  }

  LOG_INFO("Enet initialized.");

  if (StartClient(&GLOBAL_CLIENT)) {
    enet_deinitialize();
    return 1;
  }

  MutexInit(&MUTEX);
  LOG_INFO("Creating thread.");
  CORE_THREAD_RUNNING = 1;
  ThreadCreate(&CORE_THREAD_ID, CoreThread);
  return 0;
}

void Core_Deinit() 
{
  LOG_INFO("Stopping.");
  if (CORE_THREAD_RUNNING) {
    MutexLock(&MUTEX);
    GLOBAL_CLIENT.is_running = false;
    MutexUnlock(&MUTEX);
    ThreadJoin(&CORE_THREAD_ID);
    MutexDestroy(&MUTEX);
  }
}

void Core_OutputLog(LogEntry entry)
{
  if (CORE_THREAD_RUNNING) {
    MutexLock(&MUTEX);
    if (CORE_THREAD_RUNNING) {
      void *origin = LogEntryEncode(entry);
      PacketQueue_Insert(&GLOBAL_CLIENT.queue, origin, PS_PacketSize(origin));
      free(origin);
    }
    MutexUnlock(&MUTEX);
  }
}

int Core_Status() {
  return CORE_THREAD_RUNNING;
}
