#pragma once
#include "tgfx_forwarddeclarations.h"

typedef struct tgfx_uvec4 {
  unsigned int x, y, z, w;
} uvec4_tgfx;

typedef struct tgfx_uvec3 {
  unsigned int x, y, z;
} uvec3_tgfx;

typedef struct tgfx_uvec2 {
  unsigned int x, y;
} uvec2_tgfx;

typedef struct tgfx_vec2 {
  float x, y;
} vec2_tgfx;

typedef struct tgfx_vec3 {
  float x, y, z;
} vec3_tgfx;

typedef struct tgfx_vec4 {
  float x, y, z, w;
} vec4_tgfx;

typedef struct tgfx_ivec2 {
  int x, y;
} ivec2_tgfx;

typedef struct tgfx_ivec3 {
  int x, y, z;
} ivec3_tgfx;

typedef struct tgfx_boxRegion {
  unsigned int XOffset, YOffset, WIDTH, HEIGHT;
} boxRegion_tgfx;

typedef struct tgfx_cubeRegion {
  unsigned int XOffset, YOffset, ZOffset, WIDTH, HEIGHT, DEPTH;
} cubeRegion_tgfx;

typedef struct tgfx_memory_description {
  unsigned char             memoryTypeId;
  memoryallocationtype_tgfx allocationType;
  unsigned long             maxAllocationSize;
} memoryDescription_tgfx;

typedef struct tgfx_gpu_description {
  const wchar_t*   name;
  unsigned int  gfxApiVersion, driverVersion;
  gpu_type_tgfx type;
  unsigned char operationSupport_raster, operationSupport_compute, operationSupport_transfer,
    queueFamilyCount;
  const memoryDescription_tgfx* memRegions;
  unsigned char                 memRegionsCount;
} gpuDescription_tgfx;

typedef struct tgfx_window_description {
  tgfx_uvec2                size;
  monitor_tgfxhnd           monitor;
  windowmode_tgfx           mode;
  const wchar_t*               name;
  tgfx_windowResizeCallback resizeCb;
  tgfx_windowKeyCallback    keyCb;
} windowDescription_tgfx;

typedef enum windowpresentation_tgfx windowpresentation_tgfx;
typedef struct tgfx_swapchain_description {
  window_tgfxhnd                window;
  windowpresentation_tgfx       presentationMode;
  windowcomposition_tgfx        composition;
  colorspace_tgfx               colorSpace;
  textureChannels_tgfx          channels;
  textureUsageMask_tgfxflag     swapchainUsage;
  unsigned int                  permittedQueueCount;
  const gpuQueue_tgfxhnd* permittedQueues;
  unsigned int                  imageCount;
} swapchainDescription_tgfx;

#define TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT 24
#define TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT 64
#define TGFX_WINDOWGPUSUPPORT_MAXPRESENTATIONMODE 6
typedef struct tgfx_window_gpu_support {
  unsigned int              maxImageCount;
  uvec2_tgfx                minExtent, maxExtent;
  textureUsageMask_tgfxflag usageFlag;
  windowpresentation_tgfx   presentationModes[TGFX_WINDOWGPUSUPPORT_MAXPRESENTATIONMODE];
  colorspace_tgfx           colorSpace[TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT];
  textureChannels_tgfx      channels[TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT];
  gpuQueue_tgfxhnd          queues[TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT];
} windowGPUsupport_tgfx;

typedef struct tgfx_sampler_description {
  unsigned int              MinMipLevel, MaxMipLevel;
  texture_mipmapfilter_tgfx minFilter, magFilter;
  texture_wrapping_tgfx     wrapWidth, wrapHeight;
  texture_wrapping_tgfx     wrapDepth;
  uvec4_tgfx                bordercolor;
} samplerDescription_tgfx;

typedef struct tgfx_texture_description {
  texture_dimensions_tgfx       dimension;
  tgfx_uvec2                    resolution;
  textureChannels_tgfx          channelType;
  unsigned char                 mipCount;
  textureUsageMask_tgfxflag     usage;
  textureOrder_tgfx             dataOrder;
  unsigned int                  permittedQueueCount;
  const gpuQueue_tgfxhnd* permittedQueues;
} textureDescription_tgfx;

typedef struct tgfx_buffer_description {
  unsigned int                  dataSize;
  bufferUsageMask_tgfxflag      usageFlag;
  unsigned int                  permittedQueueCount;
  const gpuQueue_tgfxhnd*        permittedQueues;
  unsigned int                         extCount;
  const extension_tgfxhnd* exts;
} bufferDescription_tgfx;

typedef struct tgfx_binding_table_description {
  shaderdescriptortype_tgfx    DescriptorType;
  unsigned int                 ElementCount;
  shaderStage_tgfxflag         visibleStagesMask;
  unsigned int                 staticSamplerCount;
  const sampler_tgfxhnd* staticSamplers;
  unsigned char                isDynamic;
} bindingTableDescription_tgfx;

#define TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT 8
typedef struct tgfx_stencil_state {
  stencilop_tgfx stencilFail, pass, depthFail;
  compare_tgfx   compareOp;
  unsigned int   compareMask, writeMask, reference;
} stencilState_tgfx;

typedef struct tgfx_depth_stencil_state {
  unsigned char      depthTestEnabled, depthWriteEnabled, stencilTestEnabled;
  compare_tgfx       depthCompare;
  tgfx_stencil_state front, back;
} depthStencilState_tgfx;

typedef struct tgfx_blend_state {
  unsigned char             blendEnabled;
  blendfactor_tgfx          srcColorFactor, dstColorFactor, srcAlphaFactor, dstAlphaFactor;
  blendmode_tgfx            colorMode, alphaMode;
  textureComponentMask_tgfx blendComponents;
} blendState_tgfx;

typedef struct tgfx_raster_state_description {
  cullmode_tgfx          culling;
  polygonmode_tgfx       polygonmode;
  depthStencilState_tgfx depthStencilState;
  blendState_tgfx        blendStates[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT];
  vertexlisttypes_tgfx   topology;
} rasterStateDescription_tgfx;

typedef struct tgfx_heap_requirements_info {
  // Single GPU can have max 32 regions
  // Sentinel = 255
  // GPU should be same with the one used in CreateTexture()
  unsigned char      memoryRegionIDs[32];
  unsigned int       offsetAlignment;
  unsigned long long size;
} heapRequirementsInfo_tgfx;

// 32 byte data to store extreme colors (RGBA64)
typedef struct tgfx_typelessColor {
  char data[32];
} typelessColor_tgfx;

typedef struct tgfx_vertex_attribute_description {
  unsigned int  attributeIndx, bindingIndx, offset;
  datatype_tgfx dataType;
} vertexAttributeDescription_tgfx;

typedef struct tgfx_vertex_binding_description {
  unsigned int                bindingIndx, stride;
  vertexBindingInputRate_tgfx inputRate;
} vertexBindingDescription_tgfx;

// TGFX_SUBPASS_EXTENSION
typedef struct tgfx_subpass_slot_description {
  rasterpassStore_tgfx storeType;
  rasterpassLoad_tgfx  loadType;
  typelessColor_tgfx   clearValue;
  textureChannels_tgfx format;
  image_access_tgfx    layout;
} subpassSlotDescription_tgfx;

typedef struct tgfx_viewport_info {
  vec2_tgfx topLeftCorner, size, depthMinMax;
} viewportInfo_tgfx;

typedef struct tgfx_raster_input_assembler_description {
  unsigned int                           attribCount, bindingCount;
  const vertexAttributeDescription_tgfx* i_attributes;
  const vertexBindingDescription_tgfx*   i_bindings;
} rasterInputAssemblerDescription_tgfx;

typedef struct tgfx_raster_pipeline_description {
  unsigned int                         shaderCount;
  const shaderSource_tgfxhnd*    shaders;
  rasterInputAssemblerDescription_tgfx attribLayout;
  viewportInfo_tgfx                    viewportList;
  const rasterStateDescription_tgfx*   mainStates;
  textureChannels_tgfx                 colorTextureFormats[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT];
  const bindingTableDescription_tgfx* tables;
  unsigned int                              tableCount;
  textureChannels_tgfx                      depthStencilTextureFormat;
  unsigned int                              extCount;
  const extension_tgfxhnd*      exts;
} rasterPipelineDescription_tgfx;

typedef struct tgfx_rasterpass_begin_slot_info {
  texture_tgfxhnd      texture;
  image_access_tgfx    imageAccess;
  rasterpassLoad_tgfx  loadOp, loadStencilOp;
  rasterpassStore_tgfx storeOp, storeStencilOp;
  typelessColor_tgfx   clearValue;
} rasterpassBeginSlotInfo_tgfx;

typedef struct tgfx_indirect_argument_draw_indexed {
  unsigned int indexCountPerInstance;
  unsigned int instanceCount;
  unsigned int firstIndex;
  int          vertexOffset;
  unsigned int firstInstance;
} drawIndexedIndirectArgument_tgfx;

typedef struct tgfx_indirect_argument_draw_nonindexed {
  unsigned int vertexCountPerInstance;
  unsigned int instanceCount;
  unsigned int firstVertex;
  unsigned int firstInstance;
} drawNonIndexedIndirectArgument_tgfx;

typedef struct tgfx_indirect_argument_dispatch {
  uvec3_tgfx threadGroupCount;
} dispatchIndirectArgument_tgfx;