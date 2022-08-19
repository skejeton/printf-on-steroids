#include <common/Common.h>
#include <common/LogEntry.h>
#include <enet/enet.h>
#include <signal.h>
#include <string.h>

char *CopyTextMalloc(const char *text) {
  const size_t SIZE = strlen(text) + 1;
  char *data = (char*)malloc(SIZE);
  memcpy(data, text, SIZE);
  return data;
}

struct Category {
  const char *name;
  size_t *handles;
  size_t handles_len;
};

struct CategorizedList {
  Category *categories;
  size_t categories_len;

  void Append(const char *category, size_t handle);
  void Deinit();
};

void CategorizedList::Append(const char *category, size_t handle) {
  // Find category.
  Category *cat = NULL;
  for (int i = 0; i < this->categories_len; ++i) {
    if (strcmp(this->categories[i].name, category) == 0) {
      cat = &this->categories[i];
    }
  }

  // If not found, create.
  if (cat == NULL) {
    if (this->categories_len % 32 == 0) {
      this->categories = (Category*)realloc(this->categories, sizeof(Category) * (this->categories_len + 32));
    }
    cat = &this->categories[this->categories_len++];
    *cat = {}; 
    cat->name = category;
  }

  // Add entry to the category.
  if (cat->handles_len % 32 == 0) {
    cat->handles = (size_t*)realloc(cat->handles, sizeof(size_t) * (cat->handles_len + 32));
  }
  cat->handles[cat->handles_len++] = handle;
}

void CategorizedList::Deinit() {
  for (int i = 0; i < this->categories_len; ++i) {
    free(this->categories[i].handles);
  }
  free(this->categories);
}

struct LogList {
  LogEntry *logs;
  size_t logs_len;

  void AppendLog(LogEntry entry);
};

struct Server {
  LogList logs;
  ENetHost *host;
};

void LogList::AppendLog(LogEntry entry) {
  if (this->logs_len % 32 == 0) {
    this->logs = (LogEntry*)realloc(this->logs, sizeof(LogEntry) * (this->logs_len + 32));
  }

  this->logs[this->logs_len++] = entry;
}

/* LogList_ClearAssumingPS(log_list)
// Clears log list assuming the data was allocated by PacketStream.
*/
void LogList_ClearAssumingPS(LogList *list) {
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
      case ENET_EVENT_TYPE_RECEIVE:
        server->logs.AppendLog(LogEntryDecode(event.packet->data));
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

void DisplayLogEntry(LogEntry *entry, int i, bool show_file) {
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  if (show_file) {
    ImGui::Text("%s:%zu", entry->file, entry->line);
  } else {
    ImGui::Text("%zu", entry->line);
  }
  ImGui::TableNextColumn();
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered, 0x22FFFFFF);
  ImGui::TextWrapped("%s", entry->data);
  ImGui::PopStyleColor(1);
} 

void DisplayLogList(LogList *list, ImGuiTextFilter *filter) {
  // NOTE(skejeton): I wouldn't need this, but BeginTable then assigns 0 for the fit.
  if (list->logs_len == 0) {
    return;
  }

  if (group_by == 1) {
    // Categorized
    CategorizedList categorized_list = {};
    for (int i = 0; i < list->logs_len; ++i) {
      if (PassLogEntryFilter(filter, &list->logs[i])) {
        categorized_list.Append(list->logs[i].file, i);
      }
    }

    for (int i = 0; i < categorized_list.categories_len; ++i) {
      Category *category = &categorized_list.categories[i];
      if (ImGui::TreeNode(category->name)) {
        ImGui::Indent();
        if (ImGui::BeginTable("Table", 2, ImGuiTableFlags_Resizable|ImGuiTableFlags_RowBg)) {
          ImGui::TableSetupColumn("##Line", ImGuiTableColumnFlags_WidthFixed, 0.0f);
          for (int j = 0; j < category->handles_len; ++j) {
            DisplayLogEntry(&list->logs[category->handles[j]], j, false);
          }
          ImGui::EndTable();
        }
        ImGui::Unindent();
        ImGui::TreePop();
      }
    }

    categorized_list.Deinit();
  } else {
    // Uncategorized

    if (ImGui::BeginTable("Table", 2, ImGuiTableFlags_Resizable|ImGuiTableFlags_RowBg)) {
      ImGui::TableSetupColumn("##Line", ImGuiTableColumnFlags_WidthFixed, 0.0f);
      
      for (int i = 0; i < list->logs_len; ++i) {
        if (PassLogEntryFilter(filter, &list->logs[i])) {
          DisplayLogEntry(&list->logs[i], i, true);
        }
      }
      ImGui::EndTable();
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
