#include "common/Common.h"
#include <enet/enet.h>
#include <signal.h>
#include <string.h>

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

enum LogGroupKeyType {
  LGKT_NIL,
  LGTK_INT,
  LGTK_STR
};

struct LogGroupKey {
  LogGroupKeyType type;
  union {
    const char *string;
    int integer;
  };
};

static LogGroupKey KeyInt(int i) {
  LogGroupKey key = {LGTK_INT};
  key.integer = i;
  return key;
}

static LogGroupKey KeyStr(const char *s) {
  LogGroupKey key = {LGTK_STR};
  key.string = s;
  return key;
}

bool KeyEqual(LogGroupKey a, LogGroupKey b) {
  if (a.type != b.type) {
    return false;
  }

  switch (a.type) {
    case LGKT_NIL:
      LOG_INFO("KeyEqual for nil keys, something's wrong.");
      return false;
    case LGTK_INT:
      return a.integer == b.integer;
    case LGTK_STR:
      return strcmp(a.string, b.string) == 0;
  }
  LOG_ERROR("Unreachable.");
}

struct LogGroup {
  LogGroupKey key;
  size_t handles_len;
  BiHandle *handles;
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
  size_t logs_len;

  static LogList Init();
  void AppendLog(LogEntry entry);
};

#define LOGLIST_GROUPMAX 1024

Handle AllocateLogGroup(LogList *list) {
  for (int i = 0; i < LOGLIST_GROUPMAX; ++i) {
    if (list->groups[i].key.type == LGKT_NIL) {
      // Zero it out, just in case.
      list->groups[i] = {};
      return i;
    }
  }

  LOG_ERROR("Too many log groups. What can I say...");
  return HANDLE_INVALID;
}

static LogGroup *AcquireGroup(LogList *list, LogGroup *group, LogGroupKey key) {
  for (size_t i = 0; i < group->handles_len; ++i) {
    // NOTE(skejeton): Here we are assuming all nodes have Left prefix.
    LogGroup *subgroup = &list->groups[group->handles[i]];
    if (KeyEqual(key, subgroup->key)) {
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

void LogList::AppendLog(LogEntry entry) {
  if (this->logs_len % 32 == 0) {
    this->logs = (LogEntry*)realloc(this->logs, sizeof(LogEntry) * (this->logs_len + 32));
  }

  this->logs[this->logs_len++] = entry;

  // Populate groups
  {
    LogGroup *line = AcquireGroup(this, AcquireGroup(this, &this->root, KeyStr(entry.file)), KeyInt(entry.line));
    AppendHandle(line, BH_SET(this->logs_len-1));
  }
}

struct Server {
  LogList logs;
  ENetHost *host;
};

/* LogList_ClearAssumingPS(log_list)
// Clears log list assuming the data was allocated by PacketStream.
*/
void LogList_ClearAssumingPS(LogList *list) {
  free(list->root.handles);
  list->root = {};
  for (size_t i = 0; i < LOGLIST_GROUPMAX; ++i) {
    if (list->groups[i].key.type != LGKT_NIL) {
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
        LOG_INFO("Client connected.");
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        LOG_INFO("Client disconnected.");
        break;
      case ENET_EVENT_TYPE_RECEIVE: {
        LogEntry entry = LogEntryDecode(event.packet->data);
        LogEntryDump(&entry);
        server->logs.AppendLog(entry);
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
  LOG_INFO("Destroying server.");
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
  return filter->PassFilter(entry->data) || filter->PassFilter(entry->file);
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
    switch (item->type) {
      case LIT_INT:
        ImGui::PushStyleColor(ImGuiCol_Text, 0xFFFFAAAA);
          ImGui::Text("%d", item->int_);
        ImGui::PopStyleColor(1);
        break;
      case LIT_CHR:
        ImGui::PushStyleColor(ImGuiCol_Text, 0xFFAAAAFF);
          ImGui::Text("%c", item->chr_);
        ImGui::PopStyleColor(1);
        break;
      case LIT_STR:
        ImGui::PushStyleColor(ImGuiCol_Text, 0xFFAAFFFF);
          ImGui::Text("%s", item->str_);
        ImGui::PopStyleColor(1);
        break;
      case LIT_NIL:
        LOG_INFO("Rendering LIT_NIL. WTF?");
        break;
    }
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
  bool wrap_in_tree_node = group->key.type == LGTK_STR && group_by == 1;

  if (wrap_in_tree_node)
    if (!ImGui::TreeNode(group->key.string)) {
      return;
    }

  BiHandle last_log = 0;

  // NOTE(Skejeton): This is necessary to display logs in chronological order if the monitor mode is disabled.
  if (group->key.type == LGTK_STR && depth == 1 && is_monitor == false) {
    if (group->handles_len > 0) {
      if (BH_RIGHT(group->handles[0])) {
        LOG_ERROR("The first handle of a string node is a leaf. Looks like the temporary hack isn't needed anymore or needs fixing.");
      }
      // NOTE(Skejeton): Group with the line number.
      LogGroup *number_group = &list->groups[group->handles[0]];
      if (number_group->handles_len == 0) {
        LOG_ERROR("The number log has no items. Wack. Looks like the temporary hack isn't needed anymore or needs fixing.");
      }

      Handle log_handle = BH_GET(number_group->handles[0]);
      
      // owo what's this
      for (Handle h = log_handle; h < list->logs_len && strcmp(list->logs[h].file, group->key.string) == 0; h++) {
        DisplayLogEntry(&list->logs[h], h, false);
      }
    } 
  } else {
    for (size_t i = 0; i < group->handles_len; ++i) {
      BiHandle handle = group->handles[i];
      if (BH_RIGHT(handle)) {
        last_log = handle;
        if (PassLogEntryFilter(filter, &list->logs[BH_GET(handle)])) {
          if (!is_monitor) {
            DisplayLogEntry(&list->logs[BH_GET(handle)], i, false);
          }
        }
      } else {
        DisplayLogListRecursive(list, &list->groups[BH_GET(handle)], filter, depth+1);
      }
    }
  }

  if (is_monitor && BH_RIGHT(last_log)) {
    assert(is_monitor);
    if (PassLogEntryFilter(filter, &list->logs[BH_GET(last_log)])) {
      DisplayLogEntry(&list->logs[BH_GET(last_log)], 0, false);
    }
  }

  if (wrap_in_tree_node)
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
