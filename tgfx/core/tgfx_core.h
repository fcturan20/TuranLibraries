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

  result_tgfx (*load_backend)(tgfx_core* parent, backends_tgfx backend, tgfx_logCallback printFnc);
  // Don't use the GPU to create object/resources before init
  result_tgfx (*initGPU)(gpu_tgfxhnd gpu);
  void (*getGPUlist)(unsigned int* gpuCount, gpu_tgfxhnd* gpuList);

  ////////////// DISPLAY/WINDOWING FUNCTIONALITY

  // Also create a supported swapchain with create_swapchain() to use the window
  // Create a new swapchain in the callback too
  void (*createWindow)(const windowDescription_tgfx* desc, void* user, window_tgfxhnd* window);
  // @param textures: Should point to an array of swapchainImageCount elements!
  result_tgfx (*createSwapchain)(gpu_tgfxhnd gpu, const tgfx_swapchain_description* desc,
                                 texture_tgfxhnd* textures);
  result_tgfx (*getCurrentSwapchainTextureIndex)(window_tgfxhnd window, unsigned int* index);
  // If count is zero, list isn't touched (count is used to return value).
  // If count is equal to returned value, list is filled.
  void (*getMonitorList)(unsigned int* monitorCount, monitor_tgfxhnd* monitorList);
  void (*changeWindowResolution)(window_tgfxhnd WindowHandle, unsigned int width,
                                 unsigned int height);
  void (*takeInputs)();
  tgfx_vec2 (*getCursorPos)(window_tgfxhnd windowHnd);
  void (*setInputMode)(window_tgfxhnd windowHnd, cursorMode_tgfx cursorMode, unsigned char stickyKeys,
                       unsigned char stickyMouseButtons, unsigned char lockKeyMods);

  /////////////

  // Destroy all resources created by GFX API systems
  void (*destroy)();
} core_tgfx;

// This is for backend to implement
typedef struct tgfx_core_d core_tgfx_d;
typedef struct tgfx_core_type {
  core_tgfx_d* data;
  core_tgfx*   api;
} core_tgfx_type;

typedef struct tapi_ecs ecs_tapi;
// This function should be exported by the backend dll
typedef result_tgfx (*backend_load_func)(ecs_tapi* regsys, core_tgfx_type* core,
                                         tgfx_logCallback printcallback);
#define TGFX_BACKEND_ENTRY()                                                        \
  FUNC_DLIB_EXPORT result_tgfx BACKEND_LOAD(ecs_tapi* ecsSys, core_tgfx_type* core, \
                                            tgfx_logCallback printCallback)