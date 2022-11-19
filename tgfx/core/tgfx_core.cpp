// Make sure that all tgfx headers are in C
extern "C" {
#define T_INCLUDE_PLATFORM_LIBS
#include "tgfx_core.h"

#include <assert.h>
#include <ecs_tapi.h>
#include <predefinitions_tapi.h>

#include "tgfx_gpucontentmanager.h"
#include "tgfx_helper.h"
#include "tgfx_imgui.h"
#include "tgfx_renderer.h"
#include "tgfx_structs.h"
}

#include <string>

static core_tgfx_type* core_type_ptr;
static ecs_tapi*       core_regsys;

void defaultPrintCallback(result_tgfx result, const char* text) {
  printf("TGFX Result: %u, Message: %s\n", result, text);
}

result_tgfx load_backend(core_tgfx* parent, backends_tgfx backend,
                         tgfx_PrintLogCallback printcallback) {
  const char* path = nullptr;
  switch (backend) {
    case backends_tgfx_VULKAN: path = "TGFXVulkan.dll"; break;
    case backends_tgfx_OPENGL4: path = "TGFXOpenGL4.dll"; break;
    case backends_tgfx_D3D12: path = "TGFXD3D12.dll"; break;
    default: assert(0 && "This backend can't be loaded!"); return result_tgfx_FAIL;
  };

  if (!printcallback) {
    printcallback = defaultPrintCallback;
  }

  auto backend_dll = DLIB_LOAD_TAPI(path);
  if (!backend_dll) {
    printcallback(result_tgfx_FAIL, ("There is no " + std::string(path) + " file!").c_str());
    return result_tgfx_FAIL;
  }
  backend_load_func backendloader =
    ( backend_load_func )DLIB_FUNC_LOAD_TAPI(backend_dll, "BACKEND_LOAD");
  if (!backendloader) {
    printcallback(result_tgfx_FAIL,
                  "TGFX Backend doesn't have any backend_load() function, which it should. You are "
                  "probably using a newer version that uses different load scheme!");
    return result_tgfx_FAIL;
  }
  return backendloader(core_regsys, core_type_ptr, printcallback);
}

ECSPLUGIN_ENTRY(ecsSys, reloadFlag) {
  core_tgfx_type* core = ( core_tgfx_type* )malloc(sizeof(core_tgfx_type) + sizeof(core_tgfx));
  if (core == NULL) {
    printf("TGFX core creation failed because malloc failed");
    exit(-1);
  }
  core->api                 = ( core_tgfx* )(core + 1);
  core_type_ptr             = core;
  core_regsys               = ecsSys;
  core->api->contentmanager = new gpudatamanager_tgfx;
  core->api->helpers        = new helper_tgfx;
  core->api->imgui          = new dearimgui_tgfx;
  core->api->renderer       = new renderer_tgfx;

  core->api->load_backend = &load_backend;

  ecsSys->addSystem(TGFX_PLUGIN_NAME, TGFX_PLUGIN_VERSION, core);
}
ECSPLUGIN_EXIT(ecsSys, reloadFlag) {}