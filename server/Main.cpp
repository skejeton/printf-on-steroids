#include "src/Common.h"
#include <enet/enet.h>
#include <signal.h>

struct {
  ENetHost *host;
}
typedef Server;

void HandleEvents(Server *server) {
  INFO("Listening to events.");
  ENetEvent event;
  while (enet_host_service(server->host, &event, 1000) > 0) {
    INFO("Got event!");
    switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT:
        INFO("Client connected.");
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        INFO("Client disconnected.");
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        INFO("Recieved data: %s.", event.packet->data);
        enet_packet_destroy(event.packet);
        break;
      case ENET_EVENT_TYPE_NONE:
        INFO("None event.");
        break;
    }
  }
}

Server HostServer(int port) {
  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = port;

  enet_address_set_host(&address, "127.0.0.1");
  INFO("Hosting server.");
  ENetHost *server = enet_host_create(&address, DEFAULT_CLIENT_CAP, DEFAULT_CHAN_CAP, 0, 0);
  if (server == NULL) {
    ERROR("Failed to start server.");
  }

  return (Server){.host = server};
}

void StopServer(Server *server) {
  INFO("Destroying server.");
  enet_host_destroy(server->host);
}

//////////////////////////////////////////

#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "lib/sokol/sokol_app.h"
#include "lib/sokol/sokol_gfx.h"
#include "lib/sokol/sokol_glue.h"
#include "lib/imgui/imgui.h"
#define SOKOL_IMGUI_IMPL
#include "lib/sokol/util/sokol_imgui.h"

static bool show_test_window = true;
static bool show_another_window = false;

static sg_pass_action pass_action;

Server GLOBAL_SERVER;

// Termination via signal.
void HandleSignal(int signal) {
  void HandleCleanUp();

  HandleCleanUp();
}

void HandleInit(void) {
  // setup sokol-gfx, sokol-time and sokol-imgui
  sg_desc desc = { };
  desc.context = sapp_sgcontext();
  sg_setup(&desc);

  // use sokol-imgui with all default-options (we're not doing
  // multi-sampled rendering or using non-default pixel formats)
  simgui_desc_t simgui_desc = { };
  simgui_setup(&simgui_desc);

  // initial clear color
  pass_action.colors[0].action = SG_ACTION_CLEAR;
  pass_action.colors[0].value = { 0.0f, 0.5f, 0.7f, 1.0f };

  // Initialize enet.
  if (enet_initialize() != 0) {
    ERROR("Failed to initialize enet.");
  }

  INFO("Enet initialized.");

  GLOBAL_SERVER = HostServer(DEFAULT_PORT);
  signal(SIGINT, HandleSignal);
}

void HandleFrame(void) {
  const int width = sapp_width();
  const int height = sapp_height();
  simgui_new_frame({ width, height, sapp_frame_duration(), sapp_dpi_scale() });

  // 1. Show a simple window
  // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
  static float f = 0.0f;
  ImGui::Text("Hello, world!");
  ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
  ImGui::ColorEdit3("clear color", &pass_action.colors[0].value.r);
  if (ImGui::Button("Test Window")) show_test_window ^= 1;
  if (ImGui::Button("Another Window")) show_another_window ^= 1;
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::Text("w: %d, h: %d, dpi_scale: %.1f", sapp_width(), sapp_height(), sapp_dpi_scale());
  if (ImGui::Button(sapp_is_fullscreen() ? "Switch to windowed" : "Switch to fullscreen")) {
      sapp_toggle_fullscreen();
  }

  // 2. Show another simple window, this time using an explicit Begin/End pair
  if (show_another_window) {
    ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiCond_FirstUseEver);
    ImGui::Begin("Another Window", &show_another_window);
    ImGui::Text("Hello");
    ImGui::End();
  }

  // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowDemoWindow()
  if (show_test_window) {
    ImGui::SetNextWindowPos(ImVec2(460, 20), ImGuiCond_FirstUseEver);
    ImGui::ShowDemoWindow();
  }

  // the sokol_gfx draw pass
  sg_begin_default_pass(&pass_action, width, height);
  simgui_render();
  sg_end_pass();
  sg_commit();
}

void HandleCleanUp(void) {
  StopServer(&GLOBAL_SERVER);

  INFO("Deinitializing enet.");
  enet_deinitialize();

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
