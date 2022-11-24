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

tgfx_core*                     tgfx           = {};
tgfx_renderer*                 renderer       = {};
tgfx_gpudatamanager*           contentManager = {};
FILESYS_TAPI_PLUGIN_LOAD_TYPE  filesys        = {};
PROFILER_TAPI_PLUGIN_LOAD_TYPE profilerSys    = {};

void load_plugins() {
  pluginHnd_ecstapi threadingPlugin = editorECS->loadPlugin("tapi_threadedjobsys.dll");
  auto              threadingSys =
    ( THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE )editorECS->getSystem(THREADINGSYS_TAPI_PLUGIN_NAME);
  printf("Thread Count: %u\n", threadingSys->funcs->thread_count());

  pluginHnd_ecstapi arrayOfStringsPlugin = editorECS->loadPlugin("tapi_array_of_strings_sys.dll");
  auto              AoSsys =
    ( ARRAY_OF_STRINGS_TAPI_LOAD_TYPE )editorECS->getSystem(ARRAY_OF_STRINGS_TAPI_PLUGIN_NAME);

  pluginHnd_ecstapi profilerPlugin = editorECS->loadPlugin("tapi_profiler.dll");
  profilerSys = ( PROFILER_TAPI_PLUGIN_LOAD_TYPE )editorECS->getSystem(PROFILER_TAPI_PLUGIN_NAME);

  pluginHnd_ecstapi filesysPlugin = editorECS->loadPlugin("tapi_filesys.dll");
  filesys = ( FILESYS_TAPI_PLUGIN_LOAD_TYPE )editorECS->getSystem(FILESYS_TAPI_PLUGIN_NAME);

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

  {
    pluginHnd_ecstapi tgfxPlugin = editorECS->loadPlugin("tgfx_core.dll");
    auto              tgfxSys    = ( TGFX_PLUGIN_LOAD_TYPE )editorECS->getSystem(TGFX_PLUGIN_NAME);
    if (tgfxSys) {
      tgfx           = tgfxSys->api;
      renderer       = tgfx->renderer;
      contentManager = tgfx->contentmanager;
    }
  }
}

gpu_tgfxhnd                gpu;
tgfx_gpu_description       gpuDesc;
textureUsageFlag_tgfxhnd   textureAllUsages;
gpuQueue_tgfxhnd           allQueues[TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT] = {};
window_tgfxhnd             window;
tgfx_swapchain_description swpchn_desc;
static constexpr uint32_t  swapchainTextureCount = 2;
static constexpr uint32_t  INIT_GPUINDEX         = 1;

void createGPU() {
  tgfx->load_backend(nullptr, backends_tgfx_VULKAN, nullptr);

  gpu_tgfxlsthnd gpus;
  tgfx->getGPUlist(&gpus);
  TGFXLISTCOUNT(tgfx, gpus, gpuCount);
  gpu = gpus[INIT_GPUINDEX];
  tgfx->helpers->getGPUInfo_General(gpu, &gpuDesc);
  printf("\n\nGPU Name: %s\n  Queue Fam Count: %u\n", gpuDesc.name, gpuDesc.queueFamilyCount);
  tgfx->initGPU(gpu);
}

static tgfx_window_gpu_support swapchainSupport = {};
static textureChannels_tgfx    rtFormat                              = texture_channels_tgfx_UNDEF;
texture_tgfxhnd                swpchnTextures[swapchainTextureCount] = {};

void createFirstWindow() {
  // Create window and the swapchain
  monitor_tgfxlsthnd monitors;
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
  textureAllUsages = tgfx->helpers->createUsageFlag_Texture(false, false, true, false, false);
  textureUsageFlag_tgfxhnd storageImageUsage =
    tgfx->helpers->createUsageFlag_Texture(true, true, false, false, true);
  {
    tgfx->helpers->getWindow_GPUSupport(window, gpu, &swapchainSupport);

    swpchn_desc.channels       = swapchainSupport.channels[0];
    swpchn_desc.colorSpace     = colorspace_tgfx_sRGB_NONLINEAR;
    swpchn_desc.composition    = windowcomposition_tgfx_OPAQUE;
    swpchn_desc.imageCount     = swapchainTextureCount;
    swpchn_desc.swapchainUsage = textureAllUsages;

    swpchn_desc.presentationMode = windowpresentation_tgfx_IMMEDIATE;
    swpchn_desc.window           = window;
    // Get all supported queues of the first GPU
    for (uint32_t i = 0; i < TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT; i++) {
      allQueues[i] = swapchainSupport.queues[i];
    }
    swpchn_desc.permittedQueues = allQueues;
    // Create swapchain
    tgfx->createSwapchain(gpu, &swpchn_desc, swpchnTextures);
  }
  rtFormat = swapchainSupport.channels[0];
  #ifdef NDEBUG
  printf("createGPUandFirstWindow() finished!\n");
  #endif
}

// Create device local resources
texture_tgfxhnd             customRTs[2] = {};
buffer_tgfxhnd              firstBuffer  = {};

void createDeviceLocalResources() {
  static constexpr uint32_t heapSize        = 1 << 27;
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
  textureDesc.channelType             = rtFormat;
  textureDesc.dataOrder               = textureOrder_tgfx_SWIZZLE;
  textureDesc.dimension               = texture_dimensions_tgfx_2D;
  textureDesc.height                  = 256;
  textureDesc.width                   = 256;
  textureDesc.mipCount                = 1;
  textureDesc.permittedQueues         = allQueues;
  textureDesc.usage                   = textureAllUsages;
  //contentManager->createTexture(gpu, &textureDesc, &customRTs[0]);
  //contentManager->createTexture(gpu, &textureDesc, &customRTs[1]);

  bufferDescription_tgfx bufferDesc = {};
  bufferDesc.dataSize               = 1650;
  bufferDesc.exts                   = nullptr;
  bufferDesc.permittedQueues        = allQueues;
  bufferDesc.usageFlag =
    tgfx->helpers->createUsageFlag_Buffer(true, true, false, true, false, false, false, false);
  contentManager->createBuffer(gpu, &bufferDesc, &firstBuffer);

  heapRequirementsInfo_tgfx firstTextureReqs = {};
  //contentManager->getHeapRequirement_Texture(customRTs[0], nullptr, &firstTextureReqs);
  heapRequirementsInfo_tgfx bufferHeapReqs = {};
  //contentManager->getHeapRequirement_Buffer(firstBuffer, nullptr, &bufferHeapReqs);

  uint32_t lastMemPoint        = 0;
  auto&    calculateHeapOffset = [&lastMemPoint](const heapRequirementsInfo_tgfx& heapReq) -> void {
    lastMemPoint = ((lastMemPoint / heapReq.offsetAlignment) +
                    ((lastMemPoint % heapReq.offsetAlignment) ? 1 : 0)) *
                   heapReq.offsetAlignment;
  };
  /*contentManager->bindToHeap_Texture(firstHeap, lastMemPoint, customRTs[0], nullptr);
  lastMemPoint += firstTextureReqs.size;
  calculateHeapOffset(firstTextureReqs);
  contentManager->bindToHeap_Texture(firstHeap, lastMemPoint, customRTs[1], nullptr);
  lastMemPoint += firstTextureReqs.size;
  calculateHeapOffset(bufferHeapReqs);*/
  contentManager->bindToHeap_Buffer(firstHeap, lastMemPoint, firstBuffer, nullptr);
  lastMemPoint += firstTextureReqs.size;

#ifdef NDEBUG
  printf("createDeviceLocalResources() finished!\n");
#endif
}

bindingTableType_tgfxhnd bindingType          = {};
pipeline_tgfxhnd         firstComputePipeline = {};
pipeline_tgfxhnd         firstRasterPipeline  = {};
bindingTable_tgfxhnd     bindingTable         = {};

void compileShadersandPipelines() {
  // Compile compute shader, create binding table type & compute pipeline
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
      desc.VisibleStages = tgfx->helpers->createShaderStageFlag(1, shaderstage_tgfx_COMPUTESHADER);

      contentManager->createBindingTableType(gpu, &desc, &bindingType);
    }

    bindingTableType_tgfxhnd bindingTypes[2] = {bindingType,
                                                ( bindingTableType_tgfxhnd )tgfx->INVALIDHANDLE};
    contentManager->createComputePipeline(firstComputeShader, bindingTypes, false,
                                          &firstComputePipeline);
  }

  // Compile first vertex-fragment shader & raster pipeline
  {
    shaderSource_tgfxhnd shaderSources[2] = {};
    const char*          vertShaderText   = filesys->funcs->read_textfile("../firstShader.vert");
    // unsigned long vertDataSize;
    // void* vertShaderBin = filesys->funcs->read_binaryfile("../firstVert.spv", &vertDataSize);
    shaderSource_tgfxhnd& firstVertShader = shaderSources[0];
    contentManager->compileShaderSource(gpu, shaderlanguages_tgfx_GLSL,
                                        shaderstage_tgfx_VERTEXSHADER, ( void* )vertShaderText,
                                        strlen(vertShaderText),
                                        // vertShaderBin, vertDataSize,
                                        &firstVertShader);

    const char* fragShaderText = filesys->funcs->read_textfile("../firstShader.frag");
    // unsigned long fragDataSize;
    // void* fragShaderBin = filesys->funcs->read_binaryfile("../firstFrag.spv", &fragDataSize);
    shaderSource_tgfxhnd& firstFragShader = shaderSources[1];
    contentManager->compileShaderSource(gpu, shaderlanguages_tgfx_GLSL,
                                        shaderstage_tgfx_FRAGMENTSHADER, ( void* )fragShaderText,
                                        strlen(fragShaderText),
                                        // fragShaderBin, fragDataSize,
                                        &firstFragShader);

    rasterStateDescription_tgfx stateDesc       = {};
    stateDesc.culling                           = cullmode_tgfx_OFF;
    stateDesc.polygonmode                       = polygonmode_tgfx_FILL;
    stateDesc.topology                          = vertexlisttypes_tgfx_TRIANGLELIST;
    rasterPipelineDescription_tgfx pipelineDesc = {};
    pipelineDesc.colorTextureFormats[0]         = rtFormat;
    pipelineDesc.mainStates                     = &stateDesc;
    pipelineDesc.shaderSourceList               = shaderSources;
    bindingTableType_tgfxhnd bindingTypes[2]    = {bindingType,
                                                   ( bindingTableType_tgfxhnd )tgfx->INVALIDHANDLE};
    pipelineDesc.typeTables                     = bindingTypes;

    contentManager->createRasterPipeline(&pipelineDesc, nullptr, &firstRasterPipeline);
  }

  
  // Instantiate binding table
  {
    contentManager->instantiateBindingTable(bindingType, true, &bindingTable);
    uint32_t bindingIndex = 0;
    contentManager->setBindingTable_Buffer(bindingTable, 1, &bindingIndex, &firstBuffer, nullptr,
                                           nullptr, nullptr);
  }
}

commandBundle_tgfxhnd standardDrawBundle, initBundle;

void recordCommandBundles() {
  /*initBundle = renderer->beginCommandBundle(gpu, 1, nullptr, nullptr);
  renderer->cmdBarrierTexture(initBundle, 0, customRTs[0], image_access_tgfx_NO_ACCESS,
                              image_access_tgfx_RTCOLOR_READWRITE, textureAllUsages,
                              textureAllUsages, nullptr);
  renderer->cmdBarrierTexture(initBundle, 0, customRTs[1], image_access_tgfx_NO_ACCESS,
                              image_access_tgfx_RTCOLOR_READWRITE, textureAllUsages,
                              textureAllUsages, nullptr);
  renderer->finishCommandBundle(initBundle, nullptr);*/

  static constexpr uint32_t cmdCount = 5;
  // Record command bundle
  standardDrawBundle = renderer->beginCommandBundle(gpu, cmdCount, firstRasterPipeline, nullptr);
  {
    bindingTable_tgfxhnd bindingTables[2] = {bindingTable,
                                             ( bindingTable_tgfxhnd )tgfx->INVALIDHANDLE};
    uint32_t             cmdKey           = 0;

    renderer->cmdBindPipeline(standardDrawBundle, cmdKey++, firstRasterPipeline);
    renderer->cmdBindBindingTables(standardDrawBundle, cmdKey++, bindingTables, 0,
                                   pipelineType_tgfx_COMPUTE);
    renderer->cmdSetViewport(standardDrawBundle, cmdKey++, {0, 0, 256, 256, 0.0f, 1.0f});
    renderer->cmdSetScissor(standardDrawBundle, cmdKey++, {0, 0}, {256, 256});
    renderer->cmdDrawNonIndexedDirect(standardDrawBundle, cmdKey++, 3, 1, 0, 0);

    assert(cmdKey <= cmdCount && "Cmd count doesn't match!");
  }
  renderer->finishCommandBundle(standardDrawBundle, nullptr);
}

commandBundle_tgfxhnd perSwpchnCmdBundles[swapchainTextureCount];

void recordSwpchnCmdBundles() {
  for (uint32_t i = 0; i < swapchainTextureCount; i++) {
    static constexpr uint32_t cmdCount = 5;
    // Record command bundle
    perSwpchnCmdBundles[i] =
      renderer->beginCommandBundle(gpu, cmdCount, firstRasterPipeline, nullptr);
    {
      bindingTable_tgfxhnd bindingTables[2] = {bindingTable,
                                               ( bindingTable_tgfxhnd )tgfx->INVALIDHANDLE};
      uint32_t             cmdKey           = 0;

      renderer->cmdBindPipeline(perSwpchnCmdBundles[i], cmdKey++, firstRasterPipeline);
      renderer->cmdBindBindingTables(perSwpchnCmdBundles[i], cmdKey++, bindingTables, 0,
                                     pipelineType_tgfx_COMPUTE);
      renderer->cmdSetViewport(perSwpchnCmdBundles[i], cmdKey++, {0, 0, 256, 256, 0.0f, 1.0f});
      renderer->cmdSetScissor(perSwpchnCmdBundles[i], cmdKey++, {0, 0}, {256, 256});
      renderer->cmdDrawNonIndexedDirect(perSwpchnCmdBundles[i], cmdKey++, 3, 1, 0, 0);

      assert(cmdKey <= cmdCount && "Cmd count doesn't match!");
    }
    renderer->finishCommandBundle(perSwpchnCmdBundles[i], nullptr);
  }
}

void load_systems() {
  load_plugins();

  createGPU();

  createFirstWindow();

  createDeviceLocalResources();

  compileShadersandPipelines();

  recordCommandBundles();

  fence_tgfxhnd fence;
  renderer->createFences(gpu, 1, 15u, &fence);
  gpuQueue_tgfxlsthnd queuesPerFam;
  tgfx->helpers->getGPUInfo_Queues(gpu, 0, &queuesPerFam);

  gpuQueue_tgfxhnd queue    = queuesPerFam[0];
  uint64_t         duration = 0;
  TURAN_PROFILE_SCOPE_MCS(profilerSys->funcs, "queueSignal", &duration);
  static uint64_t waitValue = 15, signalValue = 25;
  fence_tgfxhnd   waitFences[2] = {fence, ( fence_tgfxhnd )tgfx->INVALIDHANDLE};
  renderer->queueFenceSignalWait(queue, {}, &waitValue, waitFences, &signalValue);
  renderer->queueSubmit(queue);
  
  // Initialization Command Buffer Recording
  commandBuffer_tgfxhnd        initCmdBuffer          = {};
  rasterpassBeginSlotInfo_tgfx colorAttachmentInfo    = {};
  commandBundle_tgfxhnd        standardDrawBundles[2] = {standardDrawBundle,
                                                         ( commandBundle_tgfxhnd )tgfx->INVALIDHANDLE};
  {
    colorAttachmentInfo.imageAccess = image_access_tgfx_SHADER_SAMPLEWRITE;
    colorAttachmentInfo.loadOp      = rasterpassLoad_tgfx_CLEAR;
    colorAttachmentInfo.storeOp     = rasterpassStore_tgfx_STORE;
    float cleardata[]               = {0.5, 0.5, 0.5, 1.0};
    memcpy(colorAttachmentInfo.clearValue.data, cleardata, sizeof(cleardata));
  }
  /* {
    initCmdBuffer = renderer->beginCommandBuffer(queue, nullptr);
    colorAttachmentInfo.texture          = customRTs[0];
    commandBundle_tgfxhnd initBundles[2] = {initBundle,
                                            ( commandBundle_tgfxhnd )tgfx->INVALIDHANDLE};
    renderer->executeBundles(initCmdBuffer, initBundles, nullptr);

    renderer->beginRasterpass(initCmdBuffer, 1, &colorAttachmentInfo, {}, {});
    renderer->executeBundles(initCmdBuffer, standardDrawBundles, nullptr);
    renderer->endRasterpass(initCmdBuffer, {});

    renderer->endCommandBuffer(initCmdBuffer);
  }
  */
  // Submit initialization operations to GPU!
  window_tgfxhnd windowlst[2] = {window, ( window_tgfxhnd )tgfx->INVALIDHANDLE};
  {
    uint32_t swpchnIndx = UINT32_MAX;
    tgfx->getCurrentSwapchainTextureIndex(window, &swpchnIndx);
    if (initCmdBuffer) {
      commandBuffer_tgfxhnd initCmdBuffers[2] = {initCmdBuffer,
                                                 ( commandBuffer_tgfxhnd )tgfx->INVALIDHANDLE};
      renderer->queueExecuteCmdBuffers(queue, initCmdBuffers, nullptr);
      renderer->queueSubmit(queue);
    }
    renderer->queuePresent(queue, windowlst);
    renderer->queueSubmit(queue);
  }

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
    uint32_t swpchnIndx = 0;
    tgfx->getCurrentSwapchainTextureIndex(window, &swpchnIndx);

    // Record frame's command buffer
    commandBuffer_tgfxhnd frameCmdBuffers[2] = {nullptr,
                                                ( commandBuffer_tgfxhnd )tgfx->INVALIDHANDLE};
    commandBundle_tgfxhnd frameCmdBundles[2] = {perSwpchnCmdBundles[i % swapchainTextureCount],
                                                ( commandBundle_tgfxhnd )tgfx->INVALIDHANDLE};
    {
      commandBuffer_tgfxhnd& frameCmdBuffer = frameCmdBuffers[0];
      frameCmdBuffer                        = renderer->beginCommandBuffer(queue, nullptr);
      colorAttachmentInfo.texture           = swpchnTextures[i % 2];
      renderer->beginRasterpass(frameCmdBuffer, 1, &colorAttachmentInfo, {}, {});
      renderer->executeBundles(frameCmdBuffer, standardDrawBundles, nullptr);
      renderer->endRasterpass(frameCmdBuffer, {});
      renderer->endCommandBuffer(frameCmdBuffer);
    }

    renderer->queueExecuteCmdBuffers(queue, frameCmdBuffers, nullptr);
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