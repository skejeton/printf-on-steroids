#include <include/Common.h>
#include <enet/enet.h>
#include <signal.h>
#include <string.h>

char *CopyTextMalloc(const char *text) {
  const size_t SIZE = strlen(text) + 1;
  char *data = (char*)malloc(SIZE);
  memcpy(data, text, SIZE);
  return data;
}

struct LogList {
  char **logs;
  size_t logs_len;

  void AppendLog(const char *text);
};

struct Server {
  LogList logs;
  ENetHost *host;
};

void LogList::AppendLog(const char *text) {
  if (this->logs_len % 32 == 0) {
    this->logs = (char**)realloc(this->logs, sizeof *this->logs * (this->logs_len + 32));
  }

  this->logs[this->logs_len++] = CopyTextMalloc(text);
}

void HandleEvents(Server *server) {
  ENetEvent event;
  while (enet_host_service(server->host, &event, 0) > 0) {
    LOG_INFO("Got event!");
    switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT:
        LOG_INFO("Client connected.");
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        LOG_INFO("Client disconnected.");
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        LOG_INFO("Recieved data: %s.", event.packet->data);
        server->logs.AppendLog((const char*)event.packet->data);
        enet_packet_destroy(event.packet);
        break;
      case ENET_EVENT_TYPE_NONE:
        LOG_INFO("None event.");
        break;
    }
  }
}

Server HostServer(int port) {
  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = port;

  enet_address_set_host(&address, "127.0.0.1");
  LOG_INFO("Hosting server.");
  ENetHost *server = enet_host_create(&address, DEFAULT_CLIENT_CAP, DEFAULT_CHAN_CAP, 0, 0);
  if (server == NULL) {
    LOG_ERROR("Failed to start server.");
  }

  return (Server){.host = server};
}

void StopServer(Server *server) {
  LOG_INFO("Destroying server.");
  enet_host_destroy(server->host);
}

//////////////////////////////////////////

#include "lib/sokol/sokol_app.h"
#include "lib/sokol/sokol_gfx.h"
#include "lib/sokol/sokol_glue.h"
#include "lib/imgui/imgui.h"
#include "lib/sokol/util/sokol_imgui.h"

static sg_pass_action pass_action;

ImGuiTextFilter TEXT_FILTER;
Server GLOBAL_SERVER;
ImFont *MAIN_FONT;

void CloseServer() {
  StopServer(&GLOBAL_SERVER);

  LOG_INFO("Deinitializing enet.");
  enet_deinitialize();
}

// Termination via signal.
void HandleSignal(int signal) {
  CloseServer();
  exit(0);
}

void HandleInit(void) {
  // setup sokol-gfx, sokol-time and sokol-imgui
  sg_desc desc = { };
  desc.context = sapp_sgcontext();
  sg_setup(&desc);

  // Iniitialize ImGui.
  simgui_desc_t simgui_desc = { };
  simgui_setup(&simgui_desc);
  auto &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  MAIN_FONT = io.Fonts->AddFontFromFileTTF("data/Roboto.ttf", 16);

  unsigned char* font_pixels;
  int font_width, font_height;
  io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);
  sg_image_desc img_desc = { };
  img_desc.width = font_width;
  img_desc.height = font_height;
  img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
  img_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
  img_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
  img_desc.min_filter = SG_FILTER_LINEAR;
  img_desc.mag_filter = SG_FILTER_LINEAR;
  img_desc.data.subimage[0][0].ptr = font_pixels;
  img_desc.data.subimage[0][0].size = font_width * font_height * 4;
  io.Fonts->TexID = (ImTextureID)(uintptr_t) sg_make_image(&img_desc).id;

  // initial clear color
  pass_action.colors[0].action = SG_ACTION_CLEAR;
  pass_action.colors[0].value = { 0.0f, 0.5f, 0.7f, 1.0f };

  // Initialize enet.
  if (enet_initialize() != 0) {
    LOG_ERROR("Failed to initialize enet.");
  }


  LOG_INFO("Enet initialized.");

  GLOBAL_SERVER = HostServer(DEFAULT_PORT);
  signal(SIGINT, HandleSignal);
}

void DisplayLogList(LogList *list, ImGuiTextFilter *filter) {
  for (int i = 0; i < list->logs_len; ++i) {
    if (filter->PassFilter(list->logs[i])) {
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, 0x22FFFFFF);
      ImGui::PushID(i);
      ImGui::Selectable(list->logs[i]);
      ImGui::PopID();
      ImGui::PopStyleColor(1);
    }
  }
}

void HandleFrame(void) {
  HandleEvents(&GLOBAL_SERVER);

  const int width = sapp_width();
  const int height = sapp_height();
  simgui_new_frame({ width, height, sapp_frame_duration(), sapp_dpi_scale() });

  ImGui::PushFont(MAIN_FONT);
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Root window", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
      TEXT_FILTER.Draw();
      ImGui::Separator();
      ImGui::BeginChild("Scroller", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        DisplayLogList(&GLOBAL_SERVER.logs, &TEXT_FILTER);
      ImGui::EndChild();
    ImGui::End();
  ImGui::PopFont();

  // the sokol_gfx draw pass
  sg_begin_default_pass(&pass_action, width, height);
  simgui_render();
  sg_end_pass();
  sg_commit();
}

void HandleCleanUp(void) {
  CloseServer();
  simgui_shutdown();
  sg_shutdown();
}

void HandleInput(const sapp_event* event) {
  simgui_handle_event(event);
}

sapp_desc sokol_main(int argc, char* argv[]) {
  (void)argc; (void)argv;
  sapp_desc desc = { };
  desc.init_cb = HandleInit;
  desc.frame_cb = HandleFrame;
  desc.cleanup_cb = HandleCleanUp;
  desc.event_cb = HandleInput;
  desc.gl_force_gles2 = true;
  desc.window_title = "Dear ImGui (sokol-app)";
  desc.ios_keyboard_resizes_canvas = false;
  desc.icon.sokol_default = true;
  desc.enable_clipboard = true;

  desc.width = 320*3;
  desc.height= 200*3;
  return desc;
}
