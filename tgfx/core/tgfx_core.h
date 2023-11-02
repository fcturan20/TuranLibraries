#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define TGFX_PLUGIN_NAME "tgfx_core"
#define TGFX_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)
#define TGFX_PLUGIN_LOAD_TYPE struct tgfx_core_type*

struct tgfx_core {
  struct tgfx_renderer*       renderer;
  struct tgfx_dearImgui*      imgui;
  struct tgfx_gpuDataManager* contentmanager;
  struct tgfx_helper*         helpers;

  // logCode is an index to look up from all tgfx logs. You should use core->getLogMessage() for
  // text extraInfo is the text from the backend, probably contains specific info about your system
  enum result_tgfx (*load_backend)(struct tgfx_core* parent, enum backends_tgfx backend,
                                   void (*printFnc)(unsigned int   logCode,
                                                    const wchar_t* extraInfo));
  // Don't use the GPU to create object/resources before init
  enum result_tgfx (*initGPU)(struct tgfx_gpu* gpu);
  void (*getGPUlist)(unsigned int* gpuCount, struct tgfx_gpu** gpuList);
  enum result_tgfx (*getLogMessage)(unsigned int logCode, const wchar_t** logMessage);

  ////////////// DISPLAY/WINDOWING FUNCTIONALITY

  // Also create a supported swapchain with create_swapchain() to use the window
  // Create a new swapchain in the callback too
  void (*createWindow)(const struct tgfx_windowDescription* desc, void* user,
                       struct tgfx_window** window);
  // @param textures: Should point to an array of swapchainImageCount elements!
  enum result_tgfx (*createSwapchain)(struct tgfx_gpu*                        gpu,
                                      const struct tgfx_swapchainDescription* desc,
                                      struct tgfx_texture**                   textures);
  enum result_tgfx (*getCurrentSwapchainTextureIndex)(struct tgfx_window* window,
                                                      unsigned int*       index);
  // If count is zero, list isn't touched (count is used to return value).
  // If count is equal to returned value, list is filled.
  void (*getMonitorList)(unsigned int* monitorCount, struct tgfx_monitor** monitorList);
  void (*changeWindowResolution)(struct tgfx_window* WindowHandle, unsigned int width,
                                 unsigned int height);
  void (*takeInputs)();
  void (*getCursorPos)(struct tgfx_window* windowHnd, struct tgfx_vec2* cursorPos);
  void (*setInputMode)(struct tgfx_window* windowHnd, enum cursorMode_tgfx cursorMode,
                       unsigned char stickyKeys, unsigned char stickyMouseButtons,
                       unsigned char lockKeyMods);

  /////////////

  // Destroy all resources created by GFX API systems
  void (*destroy)();
};

// This is for backend to implement
struct tgfx_core_type {
  struct tgfx_core_d* data;
  struct tgfx_core*   api;
};

#ifdef TGFX_BACKEND
// This function should be exported by the backend dll
#define TGFX_BACKEND_ENTRY()                                    \
  FUNC_DLIB_EXPORT result_tgfx BACKEND_LOAD(                    \
    const struct tlECS* ecsSys, struct tgfx_core_type* core, \
    void (*printFnc)(unsigned int logCode, const wchar_t* extraInfo))
#endif
#ifdef __cplusplus
}
#endif