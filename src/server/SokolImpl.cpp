// This file contains sokol implementation for incremental builds. 
// It's not changes often but sokol drags the compile times down quite a lot.

#define SOKOL_IMPL

#if _WIN32
    #define SOKOL_D3D11
#else
    #define SOKOL_GLCORE33
#endif

#include <sokol/sokol_app.h>
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_glue.h>
#include <imgui/imgui.h>
#define SOKOL_IMGUI_IMPL
#include <sokol/util/sokol_imgui.h>
