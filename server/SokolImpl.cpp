// This file contains sokol implementation for incremental builds. 
// It's not changes often but sokol drags the compile times down quite a lot.

#define SOKOL_IMPL

#if _WIN32
    #define SOKOL_D3D11
#else
    #define SOKOL_GLCORE33
#endif

#include "lib/sokol/sokol_app.h"
#include "lib/sokol/sokol_gfx.h"
#include "lib/sokol/sokol_glue.h"
#include "lib/imgui/imgui.h"
#define SOKOL_IMGUI_IMPL
#include "lib/sokol/util/sokol_imgui.h"
