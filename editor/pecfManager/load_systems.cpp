#include <stdio.h>

#include <random>
#include <string>

#include "allocator_tapi.h"
#include "array_of_strings_tapi.h"
#include "ecs_tapi.h"
#include "filesys_tapi.h"
#include "logger_tapi.h"
#include "main.h"
#include "pecfManager/pecfManager.h"
#include "profiler_tapi.h"
#include "tgfx_core.h"
#include "tgfx_helper.h"
#include "tgfx_renderer.h"
#include "tgfx_structs.h"
#include "threadingsys_tapi.h"

allocator_sys_tapi* allocatorSys     = nullptr;
uint64_t            destructionCount = 0;
struct pluginElement {
  pluginHnd_ecstapi pluginPTR;
  const char*       pluginMainSysName;
  unsigned char     copyCount;
  /*
  pluginElement() : pluginPTR(nullptr), pluginMainSysName(nullptr), copyCount(0){}
  ~pluginElement() {
    destructionCount++;
  }
  pluginElement::pluginElement(const pluginElement& src) {
    pluginPTR = src.pluginPTR;
    pluginMainSysName = src.pluginMainSysName;
    copyCount = src.copyCount;
  }
  static void defaultInitializePluginStruct(void* ptr) {
    pluginElement* plgn = (pluginElement*)ptr;
    plgn->pluginMainSysName = nullptr;
    plgn->pluginPTR = nullptr;
    plgn->copyCount = 0;
  }
  static void defaultCopyFuncPluginStruct(const void* src, void* dst) {
    const pluginElement* srcPlgn = (pluginElement*)src;
    pluginElement* dstPlgn = (pluginElement*)dst;
    dstPlgn->copyCount = srcPlgn->copyCount + 1;
    dstPlgn->pluginMainSysName = srcPlgn->pluginMainSysName;
    dstPlgn->pluginPTR = srcPlgn->pluginPTR;
  }
  static void defaultDestructorPluginStruct(void* ptr) {
    destructionCount++;
  }*/
};

void load_systems() {
  pluginHnd_ecstapi threadingPlugin = editorECS->loadPlugin("tapi_threadedjobsys.dll");
  auto              threadingSys =
    ( THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE )editorECS->getSystem(THREADINGSYS_TAPI_PLUGIN_NAME);
  printf("Thread Count: %u\n", threadingSys->funcs->thread_count());

  pluginHnd_ecstapi arrayOfStringsPlugin = editorECS->loadPlugin("tapi_array_of_strings_sys.dll");
  auto              AoSsys =
    ( ARRAY_OF_STRINGS_TAPI_LOAD_TYPE )editorECS->getSystem(ARRAY_OF_STRINGS_TAPI_PLUGIN_NAME);

  pluginHnd_ecstapi profilerPlugin = editorECS->loadPlugin("tapi_profiler.dll");
  auto              profilerSys =
    ( PROFILER_TAPI_PLUGIN_LOAD_TYPE )editorECS->getSystem(PROFILER_TAPI_PLUGIN_NAME);

  pluginHnd_ecstapi filesysPlugin = editorECS->loadPlugin("tapi_filesys.dll");
  auto filesys = ( FILESYS_TAPI_PLUGIN_LOAD_TYPE )editorECS->getSystem(FILESYS_TAPI_PLUGIN_NAME);

  pluginHnd_ecstapi loggerPlugin = editorECS->loadPlugin("tapi_logger.dll");
  auto loggerSys = ( LOGGER_TAPI_PLUGIN_LOAD_TYPE )editorECS->getSystem(LOGGER_TAPI_PLUGIN_NAME);

  allocatorSys =
    ( ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE )editorECS->getSystem(ALLOCATOR_TAPI_PLUGIN_NAME);
  unsigned long long        Resizes[1000];
  static constexpr uint64_t vectorMaxSize = uint64_t(1) << uint64_t(36);
  for (unsigned int i = 0; i < 1000; i++) {
    Resizes[i] = rand() % vectorMaxSize;
  }

  supermemoryblock_tapi* superMemBlock =
    allocatorSys->createSuperMemoryBlock(1 << 30, "Vector Perf Test");
  pluginElement* v_pluginElements = nullptr;
  {
    unsigned long long duration = 0;
    TURAN_PROFILE_SCOPE_MCS(profilerSys->funcs, "Vector Custom", &duration);
    v_pluginElements = ( pluginElement* )allocatorSys->vector_manager->create_vector(
      sizeof(pluginElement), superMemBlock, 10, 1000, 0
      //(vector_flagbits_tapi)(vector_flagbit_constructor_tapi | vector_flagbit_copy_tapi |
      // vector_flagbit_destructor_tapi), pluginElement::defaultInitializePluginStruct,
      // pluginElement::defaultCopyFuncPluginStruct, pluginElement::defaultDestructorPluginStruct
    );
    for (uint32_t i = 0; i < 1000; i++) {
      allocatorSys->vector_manager->resize(v_pluginElements, Resizes[i]);
    }
    STOP_PROFILE_PRINTLESS_TAPI(profilerSys->funcs);
    loggerSys->funcs->log_status(("Loading systems took: " + std::to_string(duration)).c_str());
  }

  tgfx_core*     tgfx     = nullptr;
  tgfx_renderer* renderer = nullptr;
  {
    pluginHnd_ecstapi tgfxPlugin = editorECS->loadPlugin("tgfx_core.dll");
    auto              tgfxSys    = ( TGFX_PLUGIN_LOAD_TYPE )editorECS->getSystem(TGFX_PLUGIN_NAME);
    if (tgfxSys) {
      tgfx     = tgfxSys->api;
      renderer = tgfx->renderer;
    }
  }
  gpu_tgfxlsthnd gpus;
  tgfx->load_backend(nullptr, backends_tgfx_VULKAN, nullptr);
  tgfx->getGPUlist(&gpus);
  TGFXLISTCOUNT(tgfx, gpus, gpuCount);
  for (uint32_t gpuIndx = 0; gpuIndx < 1; gpuIndx++) {
    gpu_tgfxhnd          gpu = gpus[gpuIndx];
    tgfx_gpu_description gpuDesc;
    tgfx->helpers->getGPUInfo_General(gpu, &gpuDesc);
    printf("\n\nGPU Name: %s\n Queue Fam Count: %u\n", gpuDesc.NAME, gpuDesc.queueFamilyCount);
    tgfx->initGPU(gpu);

    monitor_tgfxlsthnd monitors;
    tgfx->getmonitorlist(&monitors);
    TGFXLISTCOUNT(tgfx, monitors, monitorCount);
    printf("Monitor Count: %u\n", monitorCount);
    tgfx_window_description windowDesc = {};
    windowDesc.size                    = {1280, 720};
    windowDesc.Mode                    = windowmode_tgfx_WINDOWED;
    windowDesc.monitor                 = monitors[0];
    windowDesc.NAME                    = gpuDesc.NAME;
    windowDesc.ResizeCB                = nullptr;
    window_tgfxhnd window;
    tgfx->createWindow(&windowDesc, nullptr, &window);
    tgfx_swapchain_description swpchn_desc;
    swpchn_desc.channels    = texture_channels_tgfx_BGRA8UNORM;
    swpchn_desc.colorSpace  = colorspace_tgfx_sRGB_NONLINEAR;
    swpchn_desc.composition = windowcomposition_tgfx_OPAQUE;
    swpchn_desc.imageCount  = 2;
    swpchn_desc.swapchainUsage =
      tgfx->helpers->createUsageFlag_Texture(true, true, true, true, true);
    swpchn_desc.presentationMode = windowpresentation_tgfx_FIFO;
    swpchn_desc.window           = window;
    tgfx_window_gpu_support swapchainSupport;
    tgfx->helpers->getWindow_GPUSupport(window, gpu, &swapchainSupport);
    tgfx->createSwapchain(gpu, &swpchn_desc, nullptr);
    fence_tgfxhnd fence;
    renderer->createFences(gpu, 1, 15u, &fence);
    for (uint32_t queueFamIndx = 0; queueFamIndx < gpuDesc.queueFamilyCount; queueFamIndx++) {
      gpuQueue_tgfxlsthnd queuesPerFam;
      tgfx->helpers->getGPUInfo_Queues(gpu, queueFamIndx, &queuesPerFam);
      TGFXLISTCOUNT(tgfx, queuesPerFam, queueCount_perFam);
      for (uint32_t queueIndx = 0; queueIndx < queueCount_perFam; queueIndx++) {
        gpuQueue_tgfxhnd queue    = queuesPerFam[queueIndx];
        uint64_t         duration = 0;
        TURAN_PROFILE_SCOPE_MCS(profilerSys->funcs, "queueSignal", &duration);
        static uint64_t waitValue = 15, signalValue = 25;
        fence_tgfxhnd   waitFences[2] = {fence, ( fence_tgfxhnd )tgfx->INVALIDHANDLE};
        renderer->queueFenceSignalWait(queue, waitFences, &waitValue, waitFences, &signalValue);
        renderer->queueSubmit(queue);
        window_tgfxhnd windowlst[2]     = {window, ( window_tgfxhnd )tgfx->INVALIDHANDLE};
        uint32_t       swpchnIndices[2] = {0};
        // renderer->queuePresent(queue, windowlst, swpchnIndices);
        // renderer->queueSubmit(queue);

        for (uint32_t i = 0; i < 3; i++) {
          _sleep(1 << 4);
          uint64_t value = 0;
          renderer->getFenceValue(fence, &value);
          printf("Fence Values: %u\n", value);
        }

        STOP_PROFILE_PRINTFUL_TAPI(profilerSys->funcs);
        waitValue++;
        signalValue++;
      }
      printf("Queue Count: %u\n\n", queueCount_perFam);
    }
  }
}