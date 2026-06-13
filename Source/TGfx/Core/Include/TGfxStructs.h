#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef int textureUsageMask_tgfxflag;
typedef int bufferUsageMask_tgfxflag;
typedef int shaderStage_tgfxflag;
struct tgfx_uvec4 {
  unsigned int x, y, z, w;
};

struct tgfx_uvec3 {
  unsigned int x, y, z;
};

struct tgfx_uvec2 {
  unsigned int x, y;
};

struct tgfx_vec2 {
  float x, y;
};

struct tgfx_vec3 {
  float x, y, z;
};

struct tgfx_vec4 {
  float x, y, z, w;
};

struct tgfx_ivec2 {
  int x, y;
};

struct tgfx_ivec3 {
  int x, y, z;
};

struct tgfx_boxRegion {
  unsigned int XOffset, YOffset, WIDTH, HEIGHT;
};

struct tgfx_cubeRegion {
  unsigned int XOffset, YOffset, ZOffset, WIDTH, HEIGHT, DEPTH;
};

struct tgfx_memoryDescription {
  unsigned char                  memoryTypeId;
  enum memoryallocationtype_tgfx allocationType;
  unsigned long                  maxAllocationSize;
};

struct tgfx_gpuDescription {
  const wchar_t*     name;
  unsigned int       gfxApiVersion, driverVersion;
  enum gpu_type_tgfx type;
  unsigned char      operationSupport_raster, operationSupport_compute, operationSupport_transfer,
    queueFamilyCount;
  const struct tgfx_memoryDescription* memRegions;
  unsigned char                        memRegionsCount;
};

typedef void (*tgfx_windowResizeCallback)(struct tgfx_window* windowHnd, void* userPtr,
                                          tgfx_uvec2            resolution,
                                          struct tgfx_texture** swapchainTextures);
// @param scanCode: System-specific scan code
typedef void (*tgfx_windowKeyCallback)(struct tgfx_window* windowHnd, void* userPointer,
                                       enum key_tgfx key, int scanCode, enum keyAction_tgfx action,
                                       enum keyMod_tgfx mode);
typedef void (*tgfx_windowCloseCallback)(struct tgfx_window* windowHnd, void* userPtr);

struct tgfx_windowDescription {
  struct tgfx_uvec2         size;
  struct tgfx_monitor*      monitor;
  enum windowmode_tgfx      mode;
  const wchar_t*            name;
  tgfx_windowResizeCallback resizeCb;
  tgfx_windowKeyCallback    keyCb;
  tgfx_windowCloseCallback  closeCb;
};

struct tgfx_swapchainDescription {
  struct tgfx_window*          window;
  enum windowpresentation_tgfx presentationMode;
  enum windowcomposition_tgfx  composition;
  enum colorspace_tgfx         colorSpace;
  enum textureChannels_tgfx    channels;
  textureUsageMask_tgfxflag    swapchainUsage;
  unsigned int                 permittedQueueCount;
  struct tgfx_gpuQueue* const* permittedQueues;
  unsigned int                 imageCount;
};

#define TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT 24
#define TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT 64
#define TGFX_WINDOWGPUSUPPORT_MAXPRESENTATIONMODE 6
struct tgfx_windowGPUsupport {
  unsigned int                 maxImageCount;
  struct tgfx_uvec2            minExtent, maxExtent;
  textureUsageMask_tgfxflag    usageFlag;
  enum windowpresentation_tgfx presentationModes[TGFX_WINDOWGPUSUPPORT_MAXPRESENTATIONMODE];
  enum colorspace_tgfx         colorSpace[TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT];
  enum textureChannels_tgfx    channels[TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT];
  struct tgfx_gpuQueue*        queues[TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT];
};

struct tgfx_samplerDescription {
  unsigned int                   minMipLevel, maxMipLevel;
  enum texture_mipmapfilter_tgfx minFilter, magFilter;
  enum texture_wrapping_tgfx     wrapWidth, wrapHeight, wrapDepth;
  struct tgfx_uvec4              bordercolor;
};

struct tgfx_textureDescription {
  enum texture_dimensions_tgfx dimension;
  struct tgfx_uvec2            resolution;
  enum textureChannels_tgfx    channelType;
  unsigned char                mipCount;
  textureUsageMask_tgfxflag    usage;
  enum textureOrder_tgfx       dataOrder;
  unsigned int                 permittedQueueCount;
  struct tgfx_gpuQueue* const* permittedQueues;
};

struct tgfx_bufferDescription {
  unsigned int                  dataSize;
  bufferUsageMask_tgfxflag      usageFlag;
  unsigned int                  permittedQueueCount;
  struct tgfx_gpuQueue* const*  permittedQueues;
  unsigned int                  extCount;
  struct tgfx_extension* const* exts;
};

struct tgfx_bindingTableDescription {
  enum shaderdescriptortype_tgfx descriptorType;
  unsigned int                   elementCount;
  shaderStage_tgfxflag           visibleStagesMask;
  unsigned int                   staticSamplerCount;
  struct tgfx_sampler* const*    staticSamplers;
  unsigned char                  isDynamic;
};

#define TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT 8
struct tgfx_stencilState {
  enum stencilop_tgfx stencilFail, pass, depthFail;
  enum compare_tgfx   compareOp;
  unsigned int        compareMask, writeMask, reference;
};

struct tgfx_depthStencilState {
  unsigned char            depthTestEnabled, depthWriteEnabled, stencilTestEnabled;
  enum compare_tgfx        depthCompare;
  struct tgfx_stencilState front, back;
};

struct tgfx_blendState {
  unsigned char                  blendEnabled;
  enum blendfactor_tgfx          srcColorFactor, dstColorFactor, srcAlphaFactor, dstAlphaFactor;
  enum blendmode_tgfx            colorMode, alphaMode;
  enum textureComponentMask_tgfx blendComponents;
};

struct tgfx_rasterStateDescription {
  enum cullmode_tgfx            culling;
  enum polygonmode_tgfx         polygonmode;
  struct tgfx_depthStencilState depthStencilState;
  struct tgfx_blendState        blendStates[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT];
  enum vertexlisttypes_tgfx     topology;
};

struct tgfx_heapRequirementsInfo {
  // Single GPU can have max 32 regions
  // GPU should be same with the one used in CreateTexture()
  unsigned char      memoryRegionIDs[32];
  unsigned int       offsetAlignment;
  unsigned long long size;
};

// 32 byte data to store extreme colors (RGBA64)
struct tgfx_typelessColor {
  char data[32];
};

struct tgfx_vertexAttributeDescription {
  unsigned int       attributeIndx, bindingIndx, offset;
  enum datatype_tgfx dataType;
};

struct tgfx_vertexBindingDescription {
  unsigned int                     bindingIndx, stride;
  enum vertexBindingInputRate_tgfx inputRate;
};

// TGFX_SUBPASS_EXTENSION
struct tgfx_subpassSlotDescription {
  enum rasterpassStore_tgfx storeType;
  enum rasterpassLoad_tgfx  loadType;
  struct tgfx_typelessColor clearValue;
  enum textureChannels_tgfx format;
  enum image_access_tgfx    layout;
};

struct tgfx_viewportInfo {
  struct tgfx_vec2 topLeftCorner, size, depthMinMax;
};

struct tgfx_rasterInputAssemblerDescription {
  unsigned int                                  attribCount, bindingCount;
  const struct tgfx_vertexAttributeDescription* i_attributes;
  const struct tgfx_vertexBindingDescription*   i_bindings;
};

struct tgfx_rasterPipelineDescription {
  unsigned int                                shaderCount;
  struct tgfx_shaderSource* const*            shaders;
  struct tgfx_rasterInputAssemblerDescription attribLayout;
  const tgfx_rasterStateDescription*          mainStates;
  enum textureChannels_tgfx colorTextureFormats[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT];
  const struct tgfx_bindingTableDescription* tables;
  unsigned int                               tableCount;
  enum textureChannels_tgfx                  depthStencilTextureFormat;
  unsigned int                               extCount;
  struct tgfx_extension* const*              exts;
  unsigned char                              pushConstantOffset, pushConstantSize;
};

struct tgfx_rasterpassBeginSlotInfo {
  struct tgfx_texture*      texture;
  enum image_access_tgfx    imageAccess;
  enum rasterpassLoad_tgfx  loadOp, loadStencilOp;
  enum rasterpassStore_tgfx storeOp, storeStencilOp;
  struct tgfx_typelessColor clearValue;
};

struct tgfx_drawIndexedIndirectArgument {
  unsigned int indexCountPerInstance;
  unsigned int instanceCount;
  unsigned int firstIndex;
  int          vertexOffset;
  unsigned int firstInstance;
};

struct tgfx_drawNonIndexedIndirectArgument {
  unsigned int vertexCountPerInstance;
  unsigned int instanceCount;
  unsigned int firstVertex;
  unsigned int firstInstance;
};

struct tgfx_dispatchIndirectArgument {
  struct tgfx_uvec3 threadGroupCount;
};

#ifdef __cplusplus
}
#endif