#pragma once
#include "tgfx_forwarddeclarations.h"
#define TGFX_PLUGIN_NAME "tgfx_core"
#define TGFX_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)
#define TGFX_PLUGIN_LOAD_TYPE core_tgfx_type*

typedef struct tgfx_core {
  renderer_tgfx*       renderer;
  dearimgui_tgfx*      imgui;
  gpudatamanager_tgfx* contentmanager;
  helper_tgfx*         helpers;

  // Invalid handle is used to terminate array of i/o.
  //	NULL is used to say i/o is a failed handle and ignored if possible.
  void* INVALIDHANDLE;

  result_tgfx (*load_backend)(tgfx_core* parent, backends_tgfx backend,
                              tgfx_PrintLogCallback printFnc);
  // Don't use the GPU to create object/resources before init
  result_tgfx (*initGPU)(gpu_tgfxhnd gpu);
  // Also create a supported swapchain with create_swapchain() to use the window
  // Create a new swapchain in the callback too
  void (*createWindow)(const windowDescription_tgfx* desc, void* user, window_tgfxhnd* window);
  // @param textures: Should point to an array of swapchainImageCount elements!
  result_tgfx (*createSwapchain)(gpu_tgfxhnd gpu, const tgfx_swapchain_description* desc,
                                 texture_tgfxhnd* textures);
  result_tgfx (*getCurrentSwapchainTextureIndex)(window_tgfxhnd window, uint32_t* index);
  void (*change_window_resolution)(window_tgfxhnd WindowHandle, unsigned int width,
                                   unsigned int height);
  void (*getmonitorlist)(monitor_tgfxlsthnd* MonitorList);
  void (*getGPUlist)(gpu_tgfxlsthnd* GpuList);

  // Debug callbacks are user defined callbacks (optional)
  // As default, all backends set them as empty no-work functions
  void (*debugcallback)(result_tgfx result, const char* Text);
  // You can set this if TGFX is started with threaded call
  void (*debugcallback_threaded)(unsigned char ThreadIndex, result_tgfx Result, const char* Text);

  // Destroy all resources created by GFX API systems
  void (*destroy_tgfx_resources)();
} core_tgfx;

// This is for backend to implement
typedef struct tgfx_core_d core_tgfx_d;
typedef struct tgfx_core_type {
  core_tgfx_d* data;
  core_tgfx*   api;
} core_tgfx_type;

#define TGFXLISTCOUNT(gfxcoreptr, listobj, countername)                  \
  unsigned int countername = 0;                                          \
  while (listobj && listobj[countername] != gfxcoreptr->INVALIDHANDLE) { \
    countername++;                                                       \
  }

typedef struct tapi_ecs ecs_tapi;
// This function should be exported by the backend dll
typedef result_tgfx (*backend_load_func)(ecs_tapi* regsys, core_tgfx_type* core,
                                         tgfx_PrintLogCallback printcallback);
#define TGFX_BACKEND_ENTRY()                                                        \
  FUNC_DLIB_EXPORT result_tgfx BACKEND_LOAD(ecs_tapi* ecsSys, core_tgfx_type* core, \
                                            tgfx_PrintLogCallback printCallback)