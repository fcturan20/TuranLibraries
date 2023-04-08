#include <assert.h>
#include <stdio.h>

#include <glm/common.hpp>
#include <glm/gtx/common.hpp>
#include <random>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "allocator_tapi.h"
#include "bitset_tapi.h"
#include "ecs_tapi.h"
#include "filesys_tapi.h"
#include "logger_tapi.h"
#include "main.h"
#include "pecfManager/pecfManager.h"
#include "profiler_tapi.h"
#include "string_tapi.h"
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
FILESYS_TAPI_PLUGIN_LOAD_TYPE  fileSys        = {};
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
  fileSys = ( FILESYS_TAPI_PLUGIN_LOAD_TYPE )editorECS->getSystem(FILESYS_TAPI_PLUGIN_NAME);

  pluginHnd_ecstapi loggerPlugin = editorECS->loadPlugin("tapi_logger.dll");
  auto loggerSys = ( LOGGER_TAPI_PLUGIN_LOAD_TYPE )editorECS->getSystem(LOGGER_TAPI_PLUGIN_NAME);

  pluginHnd_ecstapi bitsetPlugin = editorECS->loadPlugin("tapi_bitset.dll");
  auto bitsetSys = ( BITSET_TAPI_PLUGIN_LOAD_TYPE )editorECS->getSystem(BITSET_TAPI_PLUGIN_NAME);

  allocatorSys =
    ( ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE )editorECS->getSystem(ALLOCATOR_TAPI_PLUGIN_NAME);
  unsigned long long        Resizes[1000];
  static constexpr uint64_t vectorMaxSize = uint64_t(1) << uint64_t(36);
  for (unsigned int i = 0; i < 1000; i++) {
    Resizes[i] = rand() % vectorMaxSize;
  }

  supermemoryblock_tapi* superMemBlock =
    allocatorSys->createSuperMemoryBlock(1ull << 30, "Vector Perf Test");
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
    STOP_PROFILE_TAPI(profilerSys->funcs);
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

gpu_tgfxhnd               gpu;
tgfx_gpu_description      gpuDesc;
textureUsageMask_tgfxflag textureAllUsages = textureUsageMask_tgfx_COPYFROM |
                                             textureUsageMask_tgfx_COPYTO |
                                             textureUsageMask_tgfx_RANDOMACCESS |
                                             textureUsageMask_tgfx_RASTERSAMPLE |
                                             textureUsageMask_tgfx_RENDERATTACHMENT,
                          storageImageUsage =
                            textureUsageMask_tgfx_COPYTO | textureUsageMask_tgfx_RANDOMACCESS;
gpuQueue_tgfxhnd           allQueues[TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT] = {};
window_tgfxhnd             mainWindowRT;
tgfx_swapchain_description swpchn_desc;
static constexpr uint32_t  swapchainTextureCount = 2;
static constexpr uint32_t  INIT_GPUINDEX         = 0;

void createGPU() {
  tgfx->load_backend(nullptr, backends_tgfx_VULKAN, nullptr);

  gpu_tgfxhnd  gpus[4];
  unsigned int gpuCount;
  tgfx->getGPUlist(&gpuCount, gpus);
  tgfx->getGPUlist(&gpuCount, gpus);
  gpu = gpus[INIT_GPUINDEX];
  tgfx->helpers->getGPUInfo_General(gpu, &gpuDesc);
  printf("\n\nGPU Name: %s\n  Queue Fam Count: %u\n", gpuDesc.name, gpuDesc.queueFamilyCount);
  tgfx->initGPU(gpu);
}

static tgfx_window_gpu_support swapchainSupport                      = {};
static textureChannels_tgfx    depthRTFormat                         = texture_channels_tgfx_D24S8;
texture_tgfxhnd                swpchnTextures[swapchainTextureCount] = {};

void keyCB(window_tgfxhnd windowHnd, void* userPointer, key_tgfx key, int scanCode,
           key_action_tgfx action, keyMod_tgfx mode) {
  if (key == key_tgfx_A) {
    exit(-1);
  }
};
void createFirstWindow() {
  // Create window and the swapchain
  monitor_tgfxhnd monitors[4];
  {
    uint32_t monitorCount = 0;
    // Get monitor list
    tgfx->getMonitorList(&monitorCount, monitors);
    tgfx->getMonitorList(&monitorCount, monitors);
    printf("Monitor Count: %u\n", monitorCount);
  }

  // Create window (OS operation) on first monitor
  {
    tgfx_window_description windowDesc = {};
    windowDesc.size                    = {1280, 720};
    windowDesc.mode                    = windowmode_tgfx_WINDOWED;
    windowDesc.monitor                 = monitors[0];
    windowDesc.name                    = gpuDesc.name;
    windowDesc.resizeCb                = nullptr;
    windowDesc.keyCb                   = keyCB;
    tgfx->createWindow(&windowDesc, nullptr, &mainWindowRT);
  }

  // Create swapchain (GPU operation) on the window
  {
    tgfx->helpers->getWindow_GPUSupport(mainWindowRT, gpu, &swapchainSupport);

    swpchn_desc.channels       = swapchainSupport.channels[0];
    swpchn_desc.colorSpace     = colorspace_tgfx_sRGB_NONLINEAR;
    swpchn_desc.composition    = windowcomposition_tgfx_OPAQUE;
    swpchn_desc.imageCount     = swapchainTextureCount;
    swpchn_desc.swapchainUsage = textureAllUsages;

    swpchn_desc.presentationMode = windowpresentation_tgfx_IMMEDIATE;
    swpchn_desc.window           = mainWindowRT;
    // Get all supported queues of the first GPU
    for (uint32_t i = 0; i < TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT; i++) {
      if (!swapchainSupport.queues[i]) {
        break;
      }
      allQueues[i] = swapchainSupport.queues[i];
      swpchn_desc.permittedQueueCount++;
    }
    swpchn_desc.permittedQueues = allQueues;
    // Create swapchain
    tgfx->createSwapchain(gpu, &swpchn_desc, swpchnTextures);
  }
#ifdef NDEBUG
  printf("createGPUandFirstWindow() finished!\n");
#endif
}

// Create device local resources
texture_tgfxhnd customDepthRT = {}, reiChikitaTexture = {};
buffer_tgfxhnd  firstBuffer  = {};
void*           mappedRegion = nullptr;
struct vertex {
  vec2_tgfx position;
  vec2_tgfx textCoord;
};
struct firstUboStruct {
  vertex vertices[6];
  // uint16_t  indices[6];
  vec4_tgfx translate[2];
};

void createDeviceLocalResources() {
  static constexpr uint32_t heapSize           = 1 << 27;
  uint32_t                  deviceLocalMemType = UINT32_MAX, hostVisibleMemType = UINT32_MAX;
  for (uint32_t memTypeIndx = 0; memTypeIndx < gpuDesc.memRegionsCount; memTypeIndx++) {
    const memoryDescription_tgfx& memDesc = gpuDesc.memRegions[memTypeIndx];
    if (memDesc.allocationtype == memoryallocationtype_HOSTVISIBLE ||
        memDesc.allocationtype == memoryallocationtype_FASTHOSTVISIBLE) {
      // If there 2 different memory types with same allocation type, select the bigger one!
      if (hostVisibleMemType != UINT32_MAX &&
          gpuDesc.memRegions[hostVisibleMemType].max_allocationsize > memDesc.max_allocationsize) {
        continue;
      }
      hostVisibleMemType = memTypeIndx;
    }
    if (memDesc.allocationtype == memoryallocationtype_DEVICELOCAL) {
      // If there 2 different memory types with same allocation type, select the bigger one!
      if (deviceLocalMemType != UINT32_MAX &&
          gpuDesc.memRegions[deviceLocalMemType].max_allocationsize > memDesc.max_allocationsize) {
        continue;
      }
      deviceLocalMemType = memTypeIndx;
    }
  }
  heap_tgfxhnd hostVisibleHeap = {}, deviceLocalHeap = {};
  assert(hostVisibleMemType != UINT32_MAX && deviceLocalMemType != UINT32_MAX &&
         "An appropriate memory region isn't found!");
  contentManager->createHeap(gpu, gpuDesc.memRegions[hostVisibleMemType].memorytype_id, heapSize, 0,
                             nullptr, &hostVisibleHeap);
  contentManager->mapHeap(hostVisibleHeap, 0, heapSize, 0, nullptr, &mappedRegion);

  contentManager->createHeap(gpu, gpuDesc.memRegions[deviceLocalMemType].memorytype_id, heapSize, 0,
                             nullptr, &deviceLocalHeap);

  textureDescription_tgfx textureDesc = {};
  textureDesc.channelType             = depthRTFormat;
  textureDesc.dataOrder               = textureOrder_tgfx_SWIZZLE;
  textureDesc.dimension               = texture_dimensions_tgfx_2D;
  textureDesc.height                  = 720;
  textureDesc.width                   = 1280;
  textureDesc.mipCount                = 1;
  textureDesc.permittedQueues         = allQueues;
  textureDesc.usage = textureUsageMask_tgfx_RENDERATTACHMENT | textureUsageMask_tgfx_COPYFROM |
                      textureUsageMask_tgfx_COPYTO;
  uvec4_tgfx maxSize = {};
  if (tgfx->helpers->getTextureTypeLimits(textureDesc.dimension, textureDesc.dataOrder,
                                          textureDesc.channelType, textureDesc.usage, gpu,
                                          &maxSize.x, &maxSize.y, &maxSize.z, &maxSize.w)) {
    contentManager->createTexture(gpu, &textureDesc, &customDepthRT);
  }

  textureDesc.channelType = texture_channels_tgfx_RGBA8UB;
  textureDesc.width       = 3000;
  textureDesc.height      = 3000;
  textureDesc.usage       = textureAllUsages;
  if (tgfx->helpers->getTextureTypeLimits(textureDesc.dimension, textureDesc.dataOrder,
                                          textureDesc.channelType, textureDesc.usage, gpu,
                                          &maxSize.x, &maxSize.y, &maxSize.z, &maxSize.w)) {
    contentManager->createTexture(gpu, &textureDesc, &reiChikitaTexture);
  }

  heapRequirementsInfo_tgfx reiChikitaTextureReqs = {}, depthTextureReqs = {};
  contentManager->getHeapRequirement_Texture(reiChikitaTexture, 0, nullptr, &reiChikitaTextureReqs);
  contentManager->getHeapRequirement_Texture(customDepthRT, 0, nullptr, &depthTextureReqs);

  bufferDescription_tgfx bufferDesc = {};
  bufferDesc.dataSize               = 2048 + reiChikitaTextureReqs.size;
  bufferDesc.exts                   = nullptr;
  bufferDesc.permittedQueues        = allQueues;
  bufferDesc.usageFlag              = bufferUsageMask_tgfx_COPYFROM | bufferUsageMask_tgfx_COPYTO |
                         bufferUsageMask_tgfx_STORAGEBUFFER | bufferUsageMask_tgfx_VERTEXBUFFER |
                         bufferUsageMask_tgfx_INDEXBUFFER | bufferUsageMask_tgfx_INDIRECTBUFFER;
  contentManager->createBuffer(gpu, &bufferDesc, &firstBuffer);

  heapRequirementsInfo_tgfx bufferHeapReqs = {};
  contentManager->getHeapRequirement_Buffer(firstBuffer, 0, nullptr, &bufferHeapReqs);

  // Host Visible Heap
  contentManager->bindToHeap_Buffer(hostVisibleHeap, 0, firstBuffer, 0, nullptr);
  ivec3_tgfx reiChikitaWHC  = {};
  void*      reiChikitaData = stbi_load(SOURCE_DIR "/shaders/reiChikita.jpg", &reiChikitaWHC.x,
                                        &reiChikitaWHC.y, &reiChikitaWHC.z, 4);
  memcpy((( char* )mappedRegion) + 2048, reiChikitaData, 3000 * 3000ull * 4ull);
  STBI_FREE(reiChikitaData);

  uint32_t reiChikitaSupportedMemTypes = {};
  tgfx->helpers->getTextureSupportedMemTypes(reiChikitaTexture, &reiChikitaSupportedMemTypes);

  // DevLocal heap
  uint32_t lastMemPoint        = 0;
  auto&    calculateHeapOffset = [&lastMemPoint](const heapRequirementsInfo_tgfx& heapReq) -> void {
    lastMemPoint = ((lastMemPoint / heapReq.offsetAlignment) +
                    ((lastMemPoint % heapReq.offsetAlignment) ? 1 : 0)) *
                   heapReq.offsetAlignment;
  };
  contentManager->bindToHeap_Texture(deviceLocalHeap, lastMemPoint, reiChikitaTexture, 0, nullptr);
  lastMemPoint += reiChikitaTextureReqs.size;
  calculateHeapOffset(depthTextureReqs);
  contentManager->bindToHeap_Texture(deviceLocalHeap, lastMemPoint, customDepthRT, 0, nullptr);

#ifdef NDEBUG
  printf("createDeviceLocalResources() finished!\n");
#endif
}

bindingTableDescription_tgfx bufferBindingType = {}, textureBindingType = {},
                             samplerBindingType = {};
pipeline_tgfxhnd     firstComputePipeline       = {};
pipeline_tgfxhnd     firstRasterPipeline        = {};
bindingTable_tgfxhnd bufferBindingTable = {}, textureBindingTable = {}, samplerBindingTable = {};
sampler_tgfxhnd      firstSampler = {};

void compileShadersandPipelines() {
  // Compile compute shader, create binding table type & compute pipeline
  {
    const char* shaderText = ( const char* )fileSys->funcs->read_textfile(
      string_type_tapi_UTF8, SOURCE_DIR "/shaders/firstComputeShader.comp", string_type_tapi_UTF8);
    shaderSource_tgfxhnd firstComputeShader = nullptr;
    contentManager->compileShaderSource(gpu, shaderlanguages_tgfx_GLSL,
                                        shaderStage_tgfx_COMPUTESHADER, ( void* )shaderText,
                                        strlen(shaderText), &firstComputeShader);

    {
      samplerDescription_tgfx samplerDesc = {};
      samplerDesc.MinMipLevel             = 0;
      samplerDesc.MaxMipLevel             = 0;
      samplerDesc.magFilter               = texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP;
      samplerDesc.minFilter               = texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP;
      samplerDesc.wrapWidth               = texture_wrapping_tgfx_REPEAT;
      samplerDesc.wrapHeight              = texture_wrapping_tgfx_REPEAT;
      samplerDesc.wrapDepth               = texture_wrapping_tgfx_REPEAT;
      contentManager->createSampler(gpu, &samplerDesc, &firstSampler);
    }

    // Create binding table types
    {
      tgfx_binding_table_description desc = {};
      desc.DescriptorType                 = shaderdescriptortype_tgfx_BUFFER;
      desc.ElementCount                   = 1;
      desc.staticSamplerCount             = 0;
      desc.visibleStagesMask = shaderStage_tgfx_COMPUTESHADER | shaderStage_tgfx_VERTEXSHADER |
                               shaderStage_tgfx_FRAGMENTSHADER;
      bufferBindingType = desc;

      desc.DescriptorType = shaderdescriptortype_tgfx_SAMPLEDTEXTURE;
      textureBindingType  = desc;

      desc.DescriptorType     = shaderdescriptortype_tgfx_SAMPLER;
      desc.staticSamplerCount = 1;
      desc.staticSamplers     = &firstSampler;
      desc.ElementCount       = 1;
      samplerBindingType      = desc;
    }

    contentManager->createComputePipeline(firstComputeShader, 1, &bufferBindingType, false,
                                          &firstComputePipeline);
  }

  // Compile first vertex-fragment shader & raster pipeline
  {
    shaderSource_tgfxhnd shaderSources[2] = {};
    const char*          vertShaderText   = ( const char* )fileSys->funcs->read_textfile(
      string_type_tapi_UTF8, SOURCE_DIR "/shaders/firstShader.vert", string_type_tapi_UTF8);
    shaderSource_tgfxhnd& firstVertShader = shaderSources[0];
    contentManager->compileShaderSource(gpu, shaderlanguages_tgfx_GLSL,
                                        shaderStage_tgfx_VERTEXSHADER, ( void* )vertShaderText,
                                        strlen(vertShaderText),
                                        // vertShaderBin, vertDataSize,
                                        &firstVertShader);

    const char* fragShaderText = ( const char* )fileSys->funcs->read_textfile(
      string_type_tapi_UTF8, SOURCE_DIR "/shaders/firstShader.frag", string_type_tapi_UTF8);
    shaderSource_tgfxhnd& firstFragShader = shaderSources[1];
    contentManager->compileShaderSource(gpu, shaderlanguages_tgfx_GLSL,
                                        shaderStage_tgfx_FRAGMENTSHADER, ( void* )fragShaderText,
                                        strlen(fragShaderText),
                                        // fragShaderBin, fragDataSize,
                                        &firstFragShader);

    vertexAttributeDescription_tgfx attribs[3];
    vertexBindingDescription_tgfx   bindings[2];
    {
      attribs[0].attributeIndx = 0;
      attribs[0].bindingIndx   = 0;
      attribs[0].dataType      = datatype_tgfx_VAR_VEC2;
      attribs[0].offset        = 0;

      attribs[1].attributeIndx = 1;
      attribs[1].bindingIndx   = 0;
      attribs[1].dataType      = datatype_tgfx_VAR_VEC2;
      attribs[1].offset        = 8;

      attribs[2].attributeIndx = 2;
      attribs[2].bindingIndx   = 1;
      attribs[2].dataType      = datatype_tgfx_VAR_VEC4;
      attribs[2].offset        = 0;

      bindings[0].bindingIndx = 0;
      bindings[0].inputRate   = vertexBindingInputRate_tgfx_VERTEX;
      bindings[0].stride      = 16;

      bindings[1].bindingIndx = 1;
      bindings[1].inputRate   = vertexBindingInputRate_tgfx_INSTANCE;
      bindings[1].stride      = 16;
    }

    rasterStateDescription_tgfx stateDesc         = {};
    stateDesc.culling                             = cullmode_tgfx_OFF;
    stateDesc.polygonmode                         = polygonmode_tgfx_FILL;
    stateDesc.topology                            = vertexlisttypes_tgfx_TRIANGLELIST;
    stateDesc.depthStencilState.depthTestEnabled  = true;
    stateDesc.depthStencilState.depthWriteEnabled = true;
    stateDesc.depthStencilState.depthCompare      = compare_tgfx_ALWAYS;
    stateDesc.blendStates[0].blendEnabled         = false;
    stateDesc.blendStates[0].blendComponents      = textureComponentMask_tgfx_ALL;
    stateDesc.blendStates[0].alphaMode            = blendmode_tgfx_MAX;
    stateDesc.blendStates[0].colorMode            = blendmode_tgfx_ADDITIVE;
    stateDesc.blendStates[0].dstAlphaFactor       = blendfactor_tgfx_DST_ALPHA;
    stateDesc.blendStates[0].srcAlphaFactor       = blendfactor_tgfx_SRC_ALPHA;
    stateDesc.blendStates[0].dstColorFactor       = blendfactor_tgfx_DST_COLOR;
    stateDesc.blendStates[0].srcColorFactor       = blendfactor_tgfx_SRC_COLOR;
    rasterPipelineDescription_tgfx pipelineDesc   = {};
    pipelineDesc.colorTextureFormats[0]           = swpchn_desc.channels;
    pipelineDesc.depthStencilTextureFormat        = depthRTFormat;
    pipelineDesc.mainStates                       = &stateDesc;
    pipelineDesc.shaderCount                      = 2;
    pipelineDesc.shaders                          = shaderSources;
    pipelineDesc.attribLayout.attribCount         = 3;
    pipelineDesc.attribLayout.bindingCount        = 2;
    pipelineDesc.attribLayout.i_attributes        = attribs;
    pipelineDesc.attribLayout.i_bindings          = bindings;
    bindingTableDescription_tgfx bindingTypes[3]  = {bufferBindingType, textureBindingType,
                                                     samplerBindingType};
    pipelineDesc.tableCount                       = 3;
    pipelineDesc.tables                           = bindingTypes;

    contentManager->createRasterPipeline(&pipelineDesc, &firstRasterPipeline);
    contentManager->destroyShaderSource(shaderSources[0]);
    contentManager->destroyShaderSource(shaderSources[1]);
  }

  // Instantiate binding tables
  {
    contentManager->createBindingTable(gpu, &bufferBindingType, &bufferBindingTable);
    uint32_t bindingIndex = 0;
    contentManager->setBindingTable_Buffer(bufferBindingTable, 1, &bindingIndex, &firstBuffer,
                                           nullptr, nullptr, 0, nullptr);

    contentManager->createBindingTable(gpu, &textureBindingType, &textureBindingTable);
    contentManager->setBindingTable_Texture(textureBindingTable, 1, &bindingIndex,
                                            &reiChikitaTexture);

    contentManager->createBindingTable(gpu, &samplerBindingType, &samplerBindingTable);
    /*contentManager->setBindingTable_Sampler(samplerBindingTable, 1, &bindingIndex,
                                            &firstSampler);*/
  }
}

commandBundle_tgfxhnd standardDrawBundle, initBundle;

void recordCommandBundles() {
  initBundle = renderer->beginCommandBundle(gpu, 3, nullptr, 0, nullptr);
  renderer->cmdBarrierTexture(initBundle, 0, reiChikitaTexture, image_access_tgfx_NO_ACCESS,
                              image_access_tgfx_TRANSFER_DIST, textureAllUsages, textureAllUsages,
                              0, nullptr);
  renderer->cmdCopyBufferToTexture(initBundle, 1, firstBuffer, 2048, reiChikitaTexture,
                                   image_access_tgfx_TRANSFER_DIST, 0, nullptr);
  renderer->cmdBarrierTexture(initBundle, 2, reiChikitaTexture, image_access_tgfx_TRANSFER_DIST,
                              image_access_tgfx_SHADER_SAMPLEONLY, textureAllUsages,
                              textureAllUsages, 0, nullptr);
  renderer->finishCommandBundle(initBundle, 0, nullptr);
  static constexpr uint32_t cmdCount = 9;
  // Record command bundle
  standardDrawBundle = renderer->beginCommandBundle(gpu, cmdCount, firstRasterPipeline, 0, nullptr);
  {
    bindingTable_tgfxhnd bindingTables[3] = {bufferBindingTable, textureBindingTable,
                                             samplerBindingTable};
    uint32_t             cmdKey           = 0;

    renderer->cmdSetDepthBounds(standardDrawBundle, cmdKey++, 0.0f, 1.0f);
    renderer->cmdSetViewport(standardDrawBundle, cmdKey++, {0, 0, 1280, 720, 0.0f, 1.0f});
    renderer->cmdSetScissor(standardDrawBundle, cmdKey++, {0, 0}, {1280, 720});
    renderer->cmdBindPipeline(standardDrawBundle, cmdKey++, firstRasterPipeline);
    uint64_t       offsets[2] = {0, offsetof(firstUboStruct, translate)};
    buffer_tgfxhnd buffers[2] = {firstBuffer, firstBuffer};
    renderer->cmdBindVertexBuffers(standardDrawBundle, cmdKey++, 0, 2, buffers, offsets);
    // renderer->cmdBindIndexBuffer(standardDrawBundle, cmdKey++, firstBuffer,
    // offsetof(firstUboStruct, indices), 2);
    renderer->cmdBindBindingTables(standardDrawBundle, cmdKey++, 0, 3, bindingTables,
                                   pipelineType_tgfx_RASTER);
    indirectOperationType_tgfx firstOp[5] = {
      indirectOperationType_tgfx_DRAWNONINDEXED, indirectOperationType_tgfx_DRAWNONINDEXED,
      indirectOperationType_tgfx_DRAWNONINDEXED, indirectOperationType_tgfx_DRAWNONINDEXED,
      indirectOperationType_tgfx_DRAWNONINDEXED};
    renderer->cmdExecuteIndirect(standardDrawBundle, cmdKey++, 5, firstOp, firstBuffer,
                                 sizeof(firstUboStruct), 0, nullptr);

    assert(cmdKey <= cmdCount && "Cmd count doesn't match!");
  }
  renderer->finishCommandBundle(standardDrawBundle, 0, nullptr);
}

commandBundle_tgfxhnd perSwpchnCmdBundles[swapchainTextureCount];

void recordSwpchnCmdBundles() {
  for (uint32_t i = 0; i < swapchainTextureCount; i++) {
    static constexpr uint32_t cmdCount = 5;
    // Record command bundle
    perSwpchnCmdBundles[i] =
      renderer->beginCommandBundle(gpu, cmdCount, firstRasterPipeline, 0, nullptr);
    {
      uint32_t cmdKey = 0;

      renderer->cmdBindPipeline(perSwpchnCmdBundles[i], cmdKey++, firstRasterPipeline);
      renderer->cmdBindBindingTables(perSwpchnCmdBundles[i], cmdKey++, 0, 1, &bufferBindingTable,
                                     pipelineType_tgfx_COMPUTE);
      renderer->cmdSetViewport(perSwpchnCmdBundles[i], cmdKey++, {0, 0, 1280, 720, 0.0f, 1.0f});
      renderer->cmdSetScissor(perSwpchnCmdBundles[i], cmdKey++, {0, 0}, {1280, 720});
      renderer->cmdDrawNonIndexedDirect(perSwpchnCmdBundles[i], cmdKey++, 3, 1, 0, 0);

      assert(cmdKey <= cmdCount && "Cmd count doesn't match!");
    }
    renderer->finishCommandBundle(perSwpchnCmdBundles[i], 0, nullptr);
  }
}

void load_systems() {
  load_plugins();

  createGPU();

  createFirstWindow();

  createDeviceLocalResources();

  compileShadersandPipelines();

  recordCommandBundles();

  firstUboStruct& ubo = *( firstUboStruct* )mappedRegion;
  ubo.vertices[0]     = {{-0.5, -0.5}, {1.0, 1.0}};
  ubo.vertices[1]     = {{-0.5, 0.5}, {0.0, 1.0}};
  ubo.vertices[2]     = {{0.5, -0.5}, {1.0, 0.0}};
  ubo.vertices[3]     = {{0.5, 0.5}, {0.0, 0.0}};
  ubo.vertices[4]     = {{0.5, -0.5}, {1.0, 0.0}};
  ubo.vertices[5]     = {{-0.5, 0.5}, {0.0, 1.0}};

  /*
  ubo.indices[0] = 0;
  ubo.indices[1] = 1;
  ubo.indices[2] = 2;
  ubo.indices[3] = 3;
  ubo.indices[4] = 2;
  ubo.indices[5] = 1;
  */

  drawNonIndexedIndirectArgument_tgfx& firstArgument =
    *( drawNonIndexedIndirectArgument_tgfx* )(( firstUboStruct* )mappedRegion + 1);
  firstArgument.firstVertex            = 0;
  firstArgument.firstInstance          = 0;
  firstArgument.vertexCountPerInstance = 6;
  firstArgument.instanceCount          = 2;

  fence_tgfxhnd fence;
  renderer->createFences(gpu, 1, 0u, &fence);
  gpuQueue_tgfxhnd queuesPerFam[TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT] = {};
  uint32_t         queueCount                                        = 0;
  tgfx->helpers->getGPUInfo_Queues(gpu, 0, &queueCount, queuesPerFam);
  tgfx->helpers->getGPUInfo_Queues(gpu, 0, &queueCount, queuesPerFam);

  gpuQueue_tgfxhnd queue    = queuesPerFam[0];
  uint64_t         duration = 0;
  TURAN_PROFILE_SCOPE_MCS(profilerSys->funcs, "queueSignal", &duration);
  static uint64_t waitValue = 0, signalValue = 1;
  renderer->queueFenceSignalWait(queue, 1, &fence, &waitValue, 1, &fence, &signalValue);

  // Color Attachment Info for Begin Render Pass
  rasterpassBeginSlotInfo_tgfx colorAttachmentInfo = {}, depthAttachmentInfo = {};
  {
    colorAttachmentInfo.imageAccess = image_access_tgfx_SHADER_SAMPLEWRITE;
    colorAttachmentInfo.loadOp      = rasterpassLoad_tgfx_CLEAR;
    colorAttachmentInfo.storeOp     = rasterpassStore_tgfx_STORE;
    float cleardata[]               = {0.5, 0.5, 0.5, 1.0};
    memcpy(colorAttachmentInfo.clearValue.data, cleardata, sizeof(cleardata));

    depthAttachmentInfo.imageAccess    = image_access_tgfx_DEPTHREADWRITE_STENCILWRITE;
    depthAttachmentInfo.loadOp         = rasterpassLoad_tgfx_CLEAR;
    depthAttachmentInfo.loadStencilOp  = rasterpassLoad_tgfx_CLEAR;
    depthAttachmentInfo.storeOp        = rasterpassStore_tgfx_STORE;
    depthAttachmentInfo.storeStencilOp = rasterpassStore_tgfx_STORE;
    depthAttachmentInfo.texture        = customDepthRT;
  }

  // Initialization Command Buffer Recording
  commandBuffer_tgfxhnd initCmdBuffer = {};
  {
    initCmdBuffer = renderer->beginCommandBuffer(queue, 0, nullptr);
    renderer->executeBundles(initCmdBuffer, 1, &initBundle, 0, nullptr);
    renderer->endCommandBuffer(initCmdBuffer);
  }

  // Submit initialization operations to GPU!
  {
    uint32_t swpchnIndx = UINT32_MAX;
    tgfx->getCurrentSwapchainTextureIndex(mainWindowRT, &swpchnIndx);
    if (initCmdBuffer) {
      renderer->queueExecuteCmdBuffers(queue, 1, &initCmdBuffer, 0, nullptr);
      renderer->queueSubmit(queue);
    }
    renderer->queuePresent(queue, 1, &mainWindowRT);
    renderer->queueSubmit(queue);
  }

  STOP_PROFILE_TAPI(profilerSys->funcs);
  waitValue++;
  signalValue++;

  int i = 0;
  while (++i && i < 10000) {
    TURAN_PROFILE_SCOPE_MCS(profilerSys->funcs, "presentation", &duration);
    uint64_t currentFenceValue = 0;
    while (currentFenceValue < signalValue - 2) {
      renderer->getFenceValue(fence, &currentFenceValue);
      printf("Waiting for fence value %u, currentFenceValue %u!\n", signalValue - 2,
             currentFenceValue);
    }
    uint32_t swpchnIndx = 0;
    tgfx->getCurrentSwapchainTextureIndex(mainWindowRT, &swpchnIndx);

    // Record frame's command buffer
    {
      commandBuffer_tgfxhnd frameCmdBuffer = renderer->beginCommandBuffer(queue, 0, nullptr);
      colorAttachmentInfo.texture          = swpchnTextures[i % 2];
      renderer->beginRasterpass(frameCmdBuffer, 1, &colorAttachmentInfo, depthAttachmentInfo, 0,
                                nullptr);
      renderer->executeBundles(frameCmdBuffer, 1, &standardDrawBundle, 0, nullptr);
      renderer->endRasterpass(frameCmdBuffer, 0, nullptr);
      renderer->endCommandBuffer(frameCmdBuffer);

      ubo.translate[0] = {sinf(i / 360.0), cosf(i / 360.0)};

      renderer->queueExecuteCmdBuffers(queue, 1, &frameCmdBuffer, 0, nullptr);
      renderer->queueFenceSignalWait(queue, 1, &fence, &waitValue, 1, &fence, &signalValue);
      renderer->queueSubmit(queue);

      waitValue++;
      signalValue++;
      renderer->queuePresent(queue, 1, &mainWindowRT);
      renderer->queueSubmit(queue);
      STOP_PROFILE_TAPI(profilerSys->funcs);
      printf("Finished and index: %u\n", swpchnIndx);
      tgfx->takeInputs();
    }
  }
  uint64_t fenceVal = 0;
  while (fenceVal != waitValue) {
    renderer->getFenceValue(fence, &fenceVal);
  }
  contentManager->destroyTexture(customDepthRT);
  contentManager->destroyTexture(reiChikitaTexture);
  contentManager->destroyPipeline(firstRasterPipeline);
  contentManager->destroyPipeline(firstComputePipeline);
  contentManager->destroySampler(firstSampler);
  contentManager->destroyBuffer(firstBuffer);
  contentManager->destroyBindingTable(bufferBindingTable);
  contentManager->destroyBindingTable(textureBindingTable);
  // renderer->destroyCommandBundle(perSwpchnCmdBundles[0]);
  // renderer->destroyCommandBundle(perSwpchnCmdBundles[1]);
  // renderer->destroyCommandBundle(standardDrawBundle);
  // renderer->destroyCommandBundle(initBundle);
  renderer->destroyFence(fence);
}