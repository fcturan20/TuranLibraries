#include <assert.h>
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
#include "tgfx_gpucontentmanager.h"
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

  tgfx_core*           tgfx           = nullptr;
  tgfx_renderer*       renderer       = nullptr;
  tgfx_gpudatamanager* contentManager = nullptr;
  {
    pluginHnd_ecstapi tgfxPlugin = editorECS->loadPlugin("tgfx_core.dll");
    auto              tgfxSys    = ( TGFX_PLUGIN_LOAD_TYPE )editorECS->getSystem(TGFX_PLUGIN_NAME);
    if (tgfxSys) {
      tgfx           = tgfxSys->api;
      renderer       = tgfx->renderer;
      contentManager = tgfx->contentmanager;
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
    printf("\n\nGPU Name: %s\n Queue Fam Count: %u\n", gpuDesc.name, gpuDesc.queueFamilyCount);
    tgfx->initGPU(gpu);

    // Create window and the swapchain
    window_tgfxhnd             window;
    tgfx_swapchain_description swpchn_desc;
    monitor_tgfxlsthnd         monitors;
    {
      // Get monitor list
      tgfx->getmonitorlist(&monitors);
      TGFXLISTCOUNT(tgfx, monitors, monitorCount);
      printf("Monitor Count: %u\n", monitorCount);
    }

    // Create window (OS operation) on first monitor
    {
      tgfx_window_description windowDesc = {};
      windowDesc.size                    = {1280, 720};
      windowDesc.Mode                    = windowmode_tgfx_WINDOWED;
      windowDesc.monitor                 = monitors[0];
      windowDesc.NAME                    = gpuDesc.name;
      windowDesc.ResizeCB                = nullptr;
      tgfx->createWindow(&windowDesc, nullptr, &window);
    }

    // Create swapchain (GPU operation) on the window
    static constexpr uint32_t swapchainTextureCount                          = 2;
    texture_tgfxhnd           swpchnTextures[swapchainTextureCount]          = {};
    gpuQueue_tgfxhnd          allQueues[TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT] = {};
    textureUsageFlag_tgfxhnd  textureAllUsages =
                               tgfx->helpers->createUsageFlag_Texture(true, true, true, true, true),
                             storageImageUsage = tgfx->helpers->createUsageFlag_Texture(
                               true, true, false, false, true);
    {
      swpchn_desc.channels       = texture_channels_tgfx_BGRA8UNORM;
      swpchn_desc.colorSpace     = colorspace_tgfx_sRGB_NONLINEAR;
      swpchn_desc.composition    = windowcomposition_tgfx_OPAQUE;
      swpchn_desc.imageCount     = swapchainTextureCount;
      swpchn_desc.swapchainUsage = textureAllUsages;

      swpchn_desc.presentationMode = windowpresentation_tgfx_IMMEDIATE;
      swpchn_desc.window           = window;
      // Get all supported queues of the first GPU
      tgfx_window_gpu_support swapchainSupport = {};
      tgfx->helpers->getWindow_GPUSupport(window, gpu, &swapchainSupport);
      for (uint32_t i = 0; i < TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT; i++) {
        allQueues[i] = swapchainSupport.queues[i];
      }
      swpchn_desc.permittedQueues = allQueues;
      // Create swapchain
      tgfx->createSwapchain(gpu, &swpchn_desc, swpchnTextures);
    }

    // Create a device local texture
    texture_tgfxhnd firstTexture      = {};
    texture_tgfxhnd secStorageTexture = {};
    buffer_tgfxhnd  firstBuffer       = {};
    {
      static constexpr uint32_t heapSize        = 1 << 20;
      uint32_t                  devLocalMemType = UINT32_MAX;
      for (uint32_t memTypeIndx = 0; memTypeIndx < gpuDesc.memRegionsCount; memTypeIndx++) {
        const memoryDescription_tgfx& memDesc = gpuDesc.memRegions[memTypeIndx];
        if (memDesc.allocationtype == memoryallocationtype_DEVICELOCAL) {
          // If there 2 different memory types with same allocation type, select the bigger one!
          if (devLocalMemType != UINT32_MAX &&
              gpuDesc.memRegions[devLocalMemType].max_allocationsize > memDesc.max_allocationsize) {
            continue;
          }
          devLocalMemType = memTypeIndx;
        }
      }
      heap_tgfxhnd firstHeap = {};
      contentManager->createHeap(gpu, gpuDesc.memRegions[devLocalMemType].memorytype_id, heapSize,
                                 nullptr, &firstHeap);

      textureDescription_tgfx textureDesc = {};
      textureDesc.channelType             = texture_channels_tgfx_R8B;
      textureDesc.dataOrder               = textureOrder_tgfx_SWIZZLE;
      textureDesc.dimension               = texture_dimensions_tgfx_2D;
      textureDesc.height                  = 1 << 8;
      textureDesc.width                   = 1 << 8;
      textureDesc.mipCount                = 1;
      textureDesc.permittedQueues         = allQueues;
      textureDesc.usage                   = textureAllUsages;
      contentManager->createTexture(gpu, &textureDesc, &firstTexture);

      textureDesc.usage = storageImageUsage;
      contentManager->createTexture(gpu, &textureDesc, &secStorageTexture);

      bufferDescription_tgfx bufferDesc = {};
      bufferDesc.dataSize               = 1650;
      bufferDesc.exts                   = nullptr;
      bufferDesc.permittedQueues        = allQueues;
      bufferDesc.usageFlag              = tgfx->helpers->createUsageFlag_Buffer(true, true, false, true, false, false, false, false);
      contentManager->createBuffer(gpu, &bufferDesc, &firstBuffer);

      heapRequirementsInfo_tgfx firstTextureReqs = {};
      contentManager->getHeapRequirement_Texture(firstTexture, nullptr, &firstTextureReqs);
      heapRequirementsInfo_tgfx secTextureReqs = {};
      contentManager->getHeapRequirement_Texture(secStorageTexture, nullptr, &secTextureReqs);
      heapRequirementsInfo_tgfx bufferHeapReqs = {};
      contentManager->getHeapRequirement_Buffer(firstBuffer, nullptr, &bufferHeapReqs);

      uint32_t lastMemPoint        = 0;
      auto& calculateHeapOffset = [&lastMemPoint](const heapRequirementsInfo_tgfx& heapReq) -> void {
        lastMemPoint = ((lastMemPoint / heapReq.offsetAlignment) +
                ((lastMemPoint % heapReq.offsetAlignment) ? 1 : 0)) *
               heapReq.offsetAlignment;
      };
      contentManager->bindToHeap_Texture(firstHeap, lastMemPoint, firstTexture, nullptr);
      lastMemPoint += firstTextureReqs.size;
      calculateHeapOffset(secTextureReqs);
      contentManager->bindToHeap_Texture(firstHeap, lastMemPoint, secStorageTexture, nullptr);
      lastMemPoint += firstTextureReqs.size;
      calculateHeapOffset(bufferHeapReqs);
      contentManager->bindToHeap_Buffer(firstHeap, lastMemPoint, firstBuffer, nullptr);
      lastMemPoint += firstTextureReqs.size;
    }

    // Compile compute shader, create binding table type & compute pipeline
    bindingTableType_tgfxhnd bindingType          = {};
    pipeline_tgfxhnd         firstComputePipeline = {};
    {
      const char*          shaderText = filesys->funcs->read_textfile("../firstComputeShader.comp");
      shaderSource_tgfxhnd firstComputeShader = nullptr;
      contentManager->compileShaderSource(gpu, shaderlanguages_tgfx_GLSL,
                                          shaderstage_tgfx_COMPUTESHADER, ( void* )shaderText,
                                          strlen(shaderText), &firstComputeShader);

      // Create binding table type
      {
        tgfx_binding_table_description desc = {};
        desc.DescriptorType                 = shaderdescriptortype_tgfx_BUFFER;
        desc.ElementCount                   = 1;
        desc.SttcSmplrs                     = nullptr;
        desc.VisibleStages =
          tgfx->helpers->createShaderStageFlag(1, shaderstage_tgfx_COMPUTESHADER);

        contentManager->createBindingTableType(gpu, &desc, &bindingType);
      }

      bindingTableType_tgfxhnd bindingTypes[2] = {bindingType,
                                                  ( bindingTableType_tgfxhnd )tgfx->INVALIDHANDLE};
      contentManager->createComputePipeline(firstComputeShader, bindingTypes, false,
                                            &firstComputePipeline);
    }

    fence_tgfxhnd fence;
    renderer->createFences(gpu, 1, 15u, &fence);
    for (uint32_t queueFamIndx = 0; queueFamIndx < 1; queueFamIndx++) {
      gpuQueue_tgfxlsthnd queuesPerFam;
      tgfx->helpers->getGPUInfo_Queues(gpu, queueFamIndx, &queuesPerFam);
      TGFXLISTCOUNT(tgfx, queuesPerFam, queueCount_perFam);
      for (uint32_t queueIndx = 0; queueIndx < 1; queueIndx++) {
        gpuQueue_tgfxhnd queue    = queuesPerFam[queueIndx];
        uint64_t         duration = 0;
        TURAN_PROFILE_SCOPE_MCS(profilerSys->funcs, "queueSignal", &duration);
        static uint64_t waitValue = 15, signalValue = 25;
        fence_tgfxhnd   waitFences[2] = {fence, ( fence_tgfxhnd )tgfx->INVALIDHANDLE};
        renderer->queueFenceSignalWait(queue, {}, &waitValue, waitFences, &signalValue);
        renderer->queueSubmit(queue);

        static constexpr uint32_t cmdCount           = 4;
        commandBundle_tgfxhnd     firstCmdBundles[2] = {
              renderer->beginCommandBundle(queue, nullptr, cmdCount, nullptr),
              ( commandBundle_tgfxhnd )tgfx->INVALIDHANDLE};
        bindingTable_tgfxhnd bindingTable = {};
        contentManager->instantiateBindingTable(bindingType, true, &bindingTable);
        uint32_t bindingIndex = 0;
        //contentManager->setBindingTable_Texture(bindingTable, 1, &bindingIndex, &secStorageTexture);
        
        contentManager->setBindingTable_Buffer(bindingTable, 1, &bindingIndex, &firstBuffer,
                                               nullptr, nullptr, nullptr);
        // Record command bundle
        {
          bindingTable_tgfxhnd bindingTables[2] = {bindingTable,
                                                   ( bindingTable_tgfxhnd )tgfx->INVALIDHANDLE};
          uint32_t             cmdKey           = 0;
          /*
          renderer->cmdBarrierTexture(
            firstCmdBundles[0], cmdKey++, secStorageTexture, image_access_tgfx_NO_ACCESS,
            image_access_tgfx_SHADER_WRITEONLY, textureAllUsages, textureAllUsages, nullptr);*/
          renderer->cmdBindPipeline(firstCmdBundles[0], cmdKey++, firstComputePipeline);
          renderer->cmdBindBindingTables(firstCmdBundles[0], cmdKey++, bindingTables, 0,
                                         pipelineType_tgfx_COMPUTE);
          renderer->cmdDispatch(firstCmdBundles[0], cmdKey++, {100, 100, 100});

          assert(cmdKey <= cmdCount && "Cmd count doesn't match!");
        }
        renderer->finishCommandBundle(firstCmdBundles[0], nullptr);
        commandBuffer_tgfxhnd firstCmdBuffers[2] = {renderer->beginCommandBuffer(queue, nullptr),
                                                    ( commandBuffer_tgfxhnd )tgfx->INVALIDHANDLE};
        renderer->executeBundles(firstCmdBuffers[0], firstCmdBundles, nullptr);
        renderer->endCommandBuffer(firstCmdBuffers[0]);
        for (uint32_t i = 0; i < 3; i++) {
          _sleep(1 << 4);
          uint64_t value = 0;
          renderer->getFenceValue(fence, &value);
          printf("Fence Values: %u\n", value);
        }

        window_tgfxhnd windowlst[2] = {window, ( window_tgfxhnd )tgfx->INVALIDHANDLE};
        uint32_t       swpchnIndx   = UINT32_MAX;
        tgfx->getCurrentSwapchainTextureIndex(window, &swpchnIndx);
        renderer->queueExecuteCmdBuffers(queue, firstCmdBuffers, nullptr);
        renderer->queueSubmit(queue);
        renderer->queuePresent(queue, windowlst);
        renderer->queueSubmit(queue);

        STOP_PROFILE_PRINTFUL_TAPI(profilerSys->funcs);
        waitValue++;
        signalValue++;

        int i = 0;
        while (++i) {
          TURAN_PROFILE_SCOPE_MCS(profilerSys->funcs, "presentation", &duration);
          uint64_t currentFenceValue = 0;
          while (currentFenceValue < signalValue - 2) {
            renderer->getFenceValue(fence, &currentFenceValue);
            printf("Waiting for fence value!\n");
          }
          tgfx->getCurrentSwapchainTextureIndex(window, &swpchnIndx);
          
          firstCmdBuffers[0] = renderer->beginCommandBuffer(queue, nullptr);
          renderer->executeBundles(firstCmdBuffers[0], firstCmdBundles, nullptr);
          renderer->endCommandBuffer(firstCmdBuffers[0]);
          renderer->queueExecuteCmdBuffers(queue, firstCmdBuffers, nullptr);
          renderer->queueSubmit(queue);
          

          if (i % 2) {
            renderer->queueFenceSignalWait(queue, {}, &waitValue, waitFences, &signalValue);
            renderer->queueSubmit(queue);
          }
          waitValue++;
          signalValue++;
          renderer->queuePresent(queue, windowlst);
          renderer->queueSubmit(queue);
          STOP_PROFILE_PRINTFUL_TAPI(profilerSys->funcs);
          printf("Finished and index: %u\n", swpchnIndx);
        }
      }
      printf("Queue Count: %u\n\n", queueCount_perFam);
    }
  }
}