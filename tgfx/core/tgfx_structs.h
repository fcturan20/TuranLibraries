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

typedef struct tgfx_boxRegion {
  unsigned int XOffset, YOffset, WIDTH, HEIGHT;
} boxRegion_tgfx;

typedef struct tgfx_cubeRegion {
  unsigned int XOffset, YOffset, ZOffset, WIDTH, HEIGHT, DEPTH;
} cubeRegion_tgfx;

typedef struct tgfx_memory_description {
  unsigned char             memorytype_id;
  memoryallocationtype_tgfx allocationtype;
  unsigned long             max_allocationsize;
} memoryDescription_tgfx;

typedef struct tgfx_gpu_description {
  const char*   name;
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
  windowmode_tgfx           Mode;
  const char*               NAME;
  tgfx_windowResizeCallback ResizeCB;
} windowDescription_tgfx;

typedef enum windowpresentation_tgfx windowpresentation_tgfx;
typedef struct tgfx_swapchain_description {
  window_tgfxhnd           window;
  windowpresentation_tgfx  presentationMode;
  windowcomposition_tgfx   composition;
  colorspace_tgfx          colorSpace;
  textureChannels_tgfx     channels;
  textureUsageFlag_tgfxhnd swapchainUsage;
  gpuQueue_tgfxlsthnd      permittedQueues;
  unsigned int             imageCount;
} swapchainDescription_tgfx;

#define TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT 24
#define TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT 64
#define TGFX_WINDOWGPUSUPPORT_MAXPRESENTATIONMODE 6
typedef struct tgfx_window_gpu_support {
  unsigned int             maxImageCount;
  uvec2_tgfx               minExtent, maxExtent;
  textureUsageFlag_tgfxhnd usageFlag;
  windowpresentation_tgfx  presentationModes[TGFX_WINDOWGPUSUPPORT_MAXPRESENTATIONMODE];
  colorspace_tgfx          colorSpace[TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT];
  textureChannels_tgfx     channels[TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT];
  gpuQueue_tgfxhnd         queues[TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT];
} windowGPUsupport_tgfx;

typedef struct tgfx_sampler_description {
  unsigned int              MinMipLevel, MaxMipLevel;
  texture_mipmapfilter_tgfx minFilter, magFilter;
  texture_wrapping_tgfx     wrapWidth, wrapHeight;
  texture_wrapping_tgfx     wrapDepth;
  uvec4_tgfx                bordercolor;
} samplerDescription_tgfx;

typedef struct tgfx_texture_description {
  texture_dimensions_tgfx  dimension;
  unsigned int             width, height;
  textureChannels_tgfx     channelType;
  unsigned char            mipCount;
  textureUsageFlag_tgfxhnd usage;
  textureOrder_tgfx        dataOrder;
  gpuQueue_tgfxlsthnd      permittedQueues;
} textureDescription_tgfx;

typedef struct tgfx_buffer_description {
  unsigned int            dataSize;
  bufferUsageFlag_tgfxhnd usageFlag;
  gpuQueue_tgfxlsthnd     permittedQueues;
  extension_tgfxlsthnd    exts;
} bufferDescription_tgfx;

typedef struct tgfx_binding_table_description {
  shaderdescriptortype_tgfx DescriptorType;
  unsigned int              ElementCount;
  shaderStageFlag_tgfxhnd   VisibleStages;
  sampler_tgfxlsthnd        SttcSmplrs;
} bindingTableDescription_tgfx;

typedef struct tgfx_raster_state_description {
  cullmode_tgfx           culling;
  polygonmode_tgfx        polygonmode;
  depthsettings_tgfxhnd   depthtest;
  stencilcnfg_tgfxnd      StencilFrontFaced, StencilBackFaced;
  blendingInfo_tgfxlsthnd BLENDINGs;
  vertexlisttypes_tgfx    topology;
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

#define TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT 8
typedef struct tgfx_raster_pipeline_description {
  shaderSource_tgfxlsthnd       shaderSourceList;
  bindingTableType_tgfxlsthnd   typeTables;
  vertexAttributeLayout_tgfxhnd attribLayout;
  viewportInfo_tgfx                   viewportList;
  const rasterStateDescription_tgfx*  mainStates;
  textureChannels_tgfx                colorTextureFormats[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT];
  textureChannels_tgfx                depthStencilTextureFormat;
} rasterPipelineDescription_tgfx;

typedef struct tgfx_rasterpass_begin_slot_info {
  texture_tgfxhnd      texture;
  image_access_tgfx    imageAccess;
  rasterpassLoad_tgfx  loadOp, loadStencilOp;
  rasterpassStore_tgfx storeOp, storeStencilOp;
  typelessColor_tgfx   clearValue;
} rasterpassBeginSlotInfo_tgfx;
