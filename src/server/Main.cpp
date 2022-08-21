#include "common/Common.h"
#include <enet/enet.h>
#include <signal.h>
#include <string.h>
#include <cstdlib>

typedef size_t Handle;
typedef Handle BiHandle; 

#define SIZET_LAST_BIT ((size_t)1 << (sizeof(size_t)*(size_t)8-(size_t)1))

#define HANDLE_INVALID (SIZET_LAST_BIT|(SIZET_LAST_BIT-(size_t)1))
// Sets the binary handle value to "Right".
#define BH_SET(h) ((h)|SIZET_LAST_BIT)
// Returns nonzero if the binary handle is "Right".
#define BH_RIGHT(h) ((h)&SIZET_LAST_BIT)
// Returns the value of binary handle without the Right/Left markings.
#define BH_GET(h) ((h) & (SIZET_LAST_BIT-1))

char *CopyTextMalloc(const char *text) {
  const size_t SIZE = strlen(text) + 1;
  char *data = (char*)malloc(SIZE);
  memcpy(data, text, SIZE);
  return data;
}

struct LogGroup {
  bool used;
  const char *key;
 
  // Handles 
  size_t handles_len;
  BiHandle *handles;
};

struct LogMeta {
  // Previous/Next generation of the log.
  Handle previous, next;
};

void AppendHandle(LogGroup *group, size_t handle) {
  if (group->handles_len % 32 == 0) {
    group->handles = (size_t*)realloc(group->handles, sizeof(size_t) * (group->handles_len + 32));
  }

  group->handles[group->handles_len++] = handle;
}

struct LogList {
  LogGroup root;
  LogGroup *groups;
  LogEntry *logs;
  LogMeta *meta;
  size_t logs_len;

  static LogList Init();
  void AppendLog(LogEntry entry);
};

#define LOGLIST_GROUPMAX 1024

Handle AllocateLogGroup(LogList *list) {
  for (int i = 0; i < LOGLIST_GROUPMAX; ++i) {
    if (!list->groups[i].used) {
      // Zero it out, just in case.
      list->groups[i] = {};
      list->groups[i].used = true;
      return i;
    }
  }

  LOG_ERROR("Too many log groups. What can I say...");
  return HANDLE_INVALID;
}

static LogGroup *AcquireGroup(LogList *list, LogGroup *group, const char *key) {
  for (size_t i = 0; i < group->handles_len; ++i) {
    // NOTE(skejeton): Here we are assuming all nodes have Left prefix.
    LogGroup *subgroup = &list->groups[group->handles[i]];
    if (strcmp(key, subgroup->key) == 0) {
      return subgroup;
    }
  }

  Handle handle = AllocateLogGroup(list);
  AppendHandle(group, handle);
  list->groups[handle].key = key;
  return &list->groups[handle];
}

LogList LogList::Init() {
  LogList l = {};
  l.groups = (LogGroup*)calloc(LOGLIST_GROUPMAX, sizeof(LogGroup));
  LOG_INFO("Allocating log list: %p", l.groups);
  return l;
}

void PrintMetadataChain(LogList *list, Handle genesis) {
  while (genesis != HANDLE_INVALID) {
    printf("%zu->", genesis);
    if (genesis == list->meta[genesis].next) {
      printf("\n");
      LOG_ERROR("Cyclic reference. %zu->%zu", genesis, list->meta[genesis].next);
    }
    genesis = list->meta[genesis].next;
  }
  printf("!\n");
}

void LogList::AppendLog(LogEntry entry) {
  if (this->logs_len % 32 == 0) {
    this->logs = (LogEntry*)realloc(this->logs, sizeof(LogEntry) * (this->logs_len + 32));
    this->meta = (LogMeta*)realloc(this->meta, sizeof(LogMeta) * (this->logs_len + 32));
  }

  LogGroup *group = AcquireGroup(this, &this->root, entry.file);

  this->logs[this->logs_len] = entry;
  this->meta[this->logs_len] = {HANDLE_INVALID, HANDLE_INVALID};

  // Link logs with the same ID (file:line)
  for (size_t i = 0; i < group->handles_len; ++i) {
    // Check if the child is a leaf (LogEntry)
    if (BH_RIGHT(group->handles[i])) {
      Handle log_handle = BH_GET(group->handles[i]);
      // Here we only have to check the line because we are getting the group already.
      if (this->logs[log_handle].line == entry.line) {
        // Find the latest log in the chain.
        while (this->meta[log_handle].next != HANDLE_INVALID) {
          log_handle = this->meta[log_handle].next;
        }

        if (this->logs_len == log_handle) {
          LOG_ERROR("Attempted to create cyclic reference.");
        }

        // Link related logs together.
        this->meta[this->logs_len].previous = log_handle;
        this->meta[log_handle].next = this->logs_len;
        break;
      }
    }
  }

  // Populate groups
  AppendHandle(group, BH_SET(this->logs_len));

  this->logs_len++;
}

struct Server {
  LogList logs;

  size_t peers_len;
  ENetPeer *peers[32];
  ENetHost *host;
};

void ConnectPeer(Server *serv, ENetPeer *peer) {
  if (serv->peers_len >= 32) {
    LOG_ERROR("Too many peers connected.");
  }

  serv->peers[serv->peers_len++] = peer;
}

/* LogList_ClearAssumingPS(log_list)
// Clears log list assuming the data was allocated by PacketStream.
*/
void LogList_ClearAssumingPS(LogList *list) {
  free(list->root.handles);
  list->root = {};
  for (size_t i = 0; i < LOGLIST_GROUPMAX; ++i) {
    if (list->groups[i].used) {
      free(list->groups[i].handles);
    }
    list->groups[i] = {};
  }
  for (size_t i = 0; i < list->logs_len; ++i) {
    LogEntryDeinit(&list->logs[i]);
  }
  list->logs_len = 0;
}

void HandleEvents(Server *server) {
  ENetEvent event;
  while (enet_host_service(server->host, &event, 0) > 0) {
    LOG_INFO("Got event!");
    switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT:
        ConnectPeer(server, event.peer);
        LOG_INFO("Client connected.");
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        LOG_INFO("Client disconnected.");
        break;
      case ENET_EVENT_TYPE_RECEIVE: {
        LogEntry entry = LogEntryDecode(event.packet->data);
        IFDEBUG(LogEntryDump(&entry));
        server->logs.AppendLog(entry);
        LOG_INFO("New log.");
        enet_packet_destroy(event.packet);
      } break;
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

  Server serv = {};
  serv.host = server;
  serv.logs = LogList::Init();

  return serv;
}

void StopServer(Server *server) {
  for (int i = 0; i < server->peers_len; ++i) {
    enet_peer_disconnect(server->peers[i], 0);
  }
  LOG_INFO("Destroying server.");
  enet_host_flush(server->host);
  enet_host_destroy(server->host);
}

//////////////////////////////////////////

#include <sokol/sokol_app.h>
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_glue.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <sokol/util/sokol_imgui.h>

static sg_pass_action pass_action;

ImGuiTextFilter TEXT_FILTER;
Server GLOBAL_SERVER;
ImFont *MAIN_FONT;
int group_by;
bool is_monitor;
const char *WAYS_TO_GROUP[] = {"None", "File"};

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

void SetImGuiRounding(float rounding) {
  ImGui::GetStyle().TabRounding = rounding;
  ImGui::GetStyle().ChildRounding = rounding;
  ImGui::GetStyle().FrameRounding = rounding;
  ImGui::GetStyle().GrabRounding = rounding-1;
  ImGui::GetStyle().ScrollbarRounding = rounding;
  ImGui::GetStyle().PopupRounding = rounding;
}


void SetUpImguiTheme() {
  // Theme by @MomoDeve on GitHub.

  auto ColorFromBytes = [](uint8_t r, uint8_t g, uint8_t b) {
    return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f);
  };

  auto& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;

  const ImVec4 bgColor           = ColorFromBytes(37, 37, 38);
  const ImVec4 lightBgColor      = ColorFromBytes(82, 82, 85);
  const ImVec4 veryLightBgColor  = ColorFromBytes(90, 90, 95);

  const ImVec4 panelColor        = ColorFromBytes(51, 51, 55);
  const ImVec4 panelHoverColor   = ColorFromBytes(26, 71, 102);
  const ImVec4 panelActiveColor  = ColorFromBytes(0, 119, 200);

  const ImVec4 textColor         = ColorFromBytes(255, 255, 255);
  const ImVec4 textDisabledColor = ColorFromBytes(151, 151, 151);
  const ImVec4 borderColor       = ColorFromBytes(78, 78, 78);

  colors[ImGuiCol_Text]                 = textColor;
  colors[ImGuiCol_TextDisabled]         = textDisabledColor;
  colors[ImGuiCol_TextSelectedBg]       = panelActiveColor;
  colors[ImGuiCol_WindowBg]             = bgColor;
  colors[ImGuiCol_ChildBg]              = bgColor;
  colors[ImGuiCol_PopupBg]              = bgColor;
  colors[ImGuiCol_Border]               = borderColor;
  colors[ImGuiCol_BorderShadow]         = borderColor;
  colors[ImGuiCol_FrameBg]              = panelColor;
  colors[ImGuiCol_FrameBgHovered]       = panelHoverColor;
  colors[ImGuiCol_FrameBgActive]        = panelActiveColor;
  colors[ImGuiCol_TitleBg]              = bgColor;
  colors[ImGuiCol_TitleBgActive]        = bgColor;
  colors[ImGuiCol_TitleBgCollapsed]     = bgColor;
  colors[ImGuiCol_MenuBarBg]            = panelColor;
  colors[ImGuiCol_ScrollbarBg]          = panelColor;
  colors[ImGuiCol_ScrollbarGrab]        = lightBgColor;
  colors[ImGuiCol_ScrollbarGrabHovered] = veryLightBgColor;
  colors[ImGuiCol_ScrollbarGrabActive]  = veryLightBgColor;
  colors[ImGuiCol_CheckMark]            = panelActiveColor;
  colors[ImGuiCol_SliderGrab]           = panelHoverColor;
  colors[ImGuiCol_SliderGrabActive]     = panelActiveColor;
  colors[ImGuiCol_Button]               = panelColor;
  colors[ImGuiCol_ButtonHovered]        = panelHoverColor;
  colors[ImGuiCol_ButtonActive]         = panelHoverColor;
  colors[ImGuiCol_Header]               = panelColor;
  colors[ImGuiCol_HeaderHovered]        = panelHoverColor;
  colors[ImGuiCol_HeaderActive]         = panelActiveColor;
  colors[ImGuiCol_Separator]            = borderColor;
  colors[ImGuiCol_SeparatorHovered]     = borderColor;
  colors[ImGuiCol_SeparatorActive]      = borderColor;
  colors[ImGuiCol_ResizeGrip]           = bgColor;
  colors[ImGuiCol_ResizeGripHovered]    = panelColor;
  colors[ImGuiCol_ResizeGripActive]     = lightBgColor;
  colors[ImGuiCol_PlotLines]            = panelActiveColor;
  colors[ImGuiCol_PlotLinesHovered]     = panelHoverColor;
  colors[ImGuiCol_PlotHistogram]        = panelActiveColor;
  colors[ImGuiCol_PlotHistogramHovered] = panelHoverColor;
  colors[ImGuiCol_DragDropTarget]       = bgColor;
  colors[ImGuiCol_NavHighlight]         = bgColor;
  colors[ImGuiCol_DockingPreview]       = panelActiveColor;
  colors[ImGuiCol_Tab]                  = bgColor;
  colors[ImGuiCol_TabActive]            = panelActiveColor;
  colors[ImGuiCol_TabUnfocused]         = bgColor;
  colors[ImGuiCol_TabUnfocusedActive]   = panelActiveColor;
  colors[ImGuiCol_TabHovered]           = panelHoverColor;

  style.WindowRounding    = 0.0f;
  style.ChildRounding     = 0.0f;
  style.FrameRounding     = 0.0f;
  style.GrabRounding      = 0.0f;
  style.PopupRounding     = 0.0f;
  style.ScrollbarRounding = 0.0f;
  style.TabRounding       = 0.0f;
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
  MAIN_FONT = io.Fonts->AddFontFromFileTTF("data/Roboto.ttf", 18);
  SetUpImguiTheme();
  SetImGuiRounding(2);

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

bool PassLogEntryFilter(ImGuiTextFilter *filter, LogEntry *entry) {
  bool pass = filter->PassFilter(entry->pre_formatted_data) || filter->PassFilter(entry->file);
  if (!pass) {
    char line_str[64];
    snprintf(line_str, 64, "%lu", entry->line);
    pass = filter->PassFilter(line_str);
  }

  return pass;
}

/* DisplayLogEntryHypertext()
// Displays the hypertext content of a LogEntry into ImGui, with formatted values like ints.
*/     
static void DisplayLogEntryHypertext(LogEntry *entry) {
  size_t string_offset = 0;
  size_t data_length = strlen(entry->data);

  // Disable item spacing between format elements.
  ImVec2 previous_item_spacing = ImGui::GetStyle().ItemSpacing;
  ImGui::GetStyle().ItemSpacing = {0, 0};

  // Iterates over each format parameter, renders all the text and values in between. 
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 10));
  for (int i = 0; i < entry->items_len; ++i) {
    LogItem *item = &entry->items[i];

    // Disable SameLine call on first element so they are interleaved within.
    ImGui::Text("%.*s", (int)(item->start-string_offset), entry->data+string_offset);
    ImGui::SameLine();
    ImU32 color = 0xFFFFFFFF;

    switch (item->type) {
      case LIT_UINT:
      case LIT_UOCT:
      case LIT_HEX:
      case LIT_UPCHEX:
      case LIT_FLT:
      case LIT_UPCFLT:
      case LIT_SCIFLT:
      case LIT_SCIUPCFLT:
      case LIT_SHRFLT:
      case LIT_SHRUPCFLT:
      case LIT_HEXFLT:
      case LIT_UPCHEXFLT:
      case LIT_INT: color = 0xFFEEA999; break;
      case LIT_CHR: color = 0xFF99A9EE; break;
      case LIT_PTR:
      case LIT_STR: color = 0xFFA9EEEE; break;
      case LIT_NIL:
        LOG_INFO("Rendering LIT_NIL. WTF?");
        break;
    }

    char buffer[4096];
    LogItemToString(buffer, 4096, item);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
      ImGui::Text("%s", buffer);
    ImGui::PopStyleColor();


    ImGui::SameLine();
    string_offset = item->start + item->size;
  }
  ImGui::PopStyleVar(1);
  ImGui::Text("%s", entry->data+string_offset);
  ImGui::GetStyle().ItemSpacing = previous_item_spacing;
}

static void DisplayLogEntry(LogEntry *entry, int i, bool show_file) {
  ImGui::PushStyleColor(ImGuiCol_Text, 0x99FFFFFF);
  if (show_file) {
    ImGui::Text("%s:%zu", entry->file, entry->line);
  } else {
    ImGui::Text("%zu", entry->line);
  }
  ImGui::PopStyleColor(1);
  ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered, 0x22FFFFFF);
    DisplayLogEntryHypertext(entry);
  ImGui::PopStyleColor(1);
} 

void DisplayLogListRecursive(LogList *list, LogGroup *group, ImGuiTextFilter *filter, int depth) {
  bool should_have_tree = group_by == 1 && group->key != NULL;

  if (should_have_tree && !ImGui::TreeNode(group->key)) {
    return;
  }

  for (size_t i = 0; i < group->handles_len; ++i) {
    BiHandle handle = group->handles[i];
    if (BH_RIGHT(handle)) {
      LogEntry *log_entry = &list->logs[BH_GET(handle)];
      LogMeta log_meta = list->meta[BH_GET(handle)];

      if (is_monitor) {
        // We found the first log in the related log chain.
        if (log_meta.previous == HANDLE_INVALID) {
          Handle last_handle = handle;
          // Scroll down for the last log in the chain.
          while (log_meta.next != HANDLE_INVALID) {
            if (PassLogEntryFilter(filter, &list->logs[log_meta.next])) {
              last_handle = log_meta.next;
            }
            log_meta = list->meta[log_meta.next];
          }
          log_entry = &list->logs[last_handle];
        } else {
          log_entry = NULL;
        }
      }

      if (log_entry) {
        if (PassLogEntryFilter(filter, log_entry)) {
          DisplayLogEntry(log_entry, i, !should_have_tree);
        }
      }
    } else {
      DisplayLogListRecursive(list, &list->groups[BH_GET(handle)], filter, depth+1);
    }
  }

  if (should_have_tree)
    ImGui::TreePop();
}

void DisplayLogList(LogList *list, ImGuiTextFilter *filter) {
  DisplayLogListRecursive(list, &list->root, filter, 0);
}

void HandleFrame(void) {
  HandleEvents(&GLOBAL_SERVER);

  const int width = sapp_width();
  const int height = sapp_height();
  simgui_new_frame({ width, height, sapp_frame_duration(), sapp_dpi_scale() });

  ImGui::PushFont(MAIN_FONT);
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Root window", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

      if (ImGui::BeginTable("##Split", 4, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("##Filter", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableNextColumn();
        TEXT_FILTER.Draw("Filter");
        ImGui::TableNextColumn();
        ImGui::Combo("Group by", &group_by, WAYS_TO_GROUP, 2);
        ImGui::TableNextColumn();
        ImGui::Checkbox("Monitor", &is_monitor);
        if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 1.0) {
          ImGui::SetTooltip("Shows latest logs from their group.");
        }
        ImGui::TableNextColumn();
        if (ImGui::Button("Clear")) {
          LogList_ClearAssumingPS(&GLOBAL_SERVER.logs);
        }
        ImGui::EndTable();
      }

      ImGui::BeginChild("Scroller", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()-40.0)
          ImGui::SetScrollY(ImGui::GetScrollMaxY());
     

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

  desc.width = 640;
  desc.height= 480;
  return desc;
}
