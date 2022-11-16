#pragma once
#include "tgfx_forwarddeclarations.h"

typedef struct tgfx_helper {
  // Hardware Capability Helpers
  void (*getGPUInfo_General)(gpu_tgfxhnd GPUhnd, gpuDescription_tgfx* desc);
  void (*getGPUInfo_Queues)(gpu_tgfxhnd GPUhnd, unsigned int queueFamIndx,
                            gpuQueue_tgfxlsthnd* queueList);
  unsigned char (*GetTextureTypeLimits)(texture_dimensions_tgfx dims, textureOrder_tgfx dataorder,
                                        textureChannels_tgfx     channeltype,
                                        textureUsageFlag_tgfxhnd usageflag, gpu_tgfxhnd GPUHandle,
                                        unsigned int* MAXWIDTH, unsigned int* MAXHEIGHT,
                                        unsigned int* MAXDEPTH, unsigned int* MAXMIPLEVEL);
  result_tgfx (*getWindow_GPUSupport)(window_tgfxhnd window, gpu_tgfxhnd gpu,
                                      windowGPUsupport_tgfx* info);

  textureUsageFlag_tgfxhnd (*createUsageFlag_Texture)(unsigned char copyFrom, unsigned char copyTo,
                                                      unsigned char renderTo,
                                                      unsigned char sampledReadOnly,
                                                      unsigned char randomWrittenTo);
  bufferUsageFlag_tgfxhnd (*createUsageFlag_Buffer)(
    unsigned char copyFrom, unsigned char copyTo, unsigned char uniformBuffer,
    unsigned char storageBuffer, unsigned char vertexBuffer, unsigned char indexBuffer,
    unsigned char indirectBuffer, unsigned char accessByPointerInShader);
  void (*getTextureRecommendedAllocationInfo)(unsigned int  GPUIndex,
                                            unsigned int* SupportedMemoryTypesBitset);
  void (*getMonitorInfo)(monitor_tgfxhnd MonitorHandle,
                                                       unsigned int* WIDTH, unsigned int* HEIGHT,
                                                       unsigned int* ColorBites,
                                                       unsigned int* RefreshRate);

  // You can't create a memory type, only set allocation size of it
  //  (Memory Type is given by the GFX initialization process)
  result_tgfx (*SetMemoryTypeInfo)(unsigned int ID, unsigned long long allocSize,
                                   extension_tgfxlsthnd exts);

  shaderStageFlag_tgfxhnd (*createShaderStageFlag)(unsigned char count, ...);
  RTSlotDescription_tgfxhnd (*CreateRTSlotDescription_Color)(
    texture_tgfxhnd text0, texture_tgfxhnd text1, operationtype_tgfx OPTYPE,
    drawpassload_tgfx LOADTYPE, unsigned char useLater, unsigned char slotIndx,
    vec4_tgfx CLEARVALUE);
  RTSlotDescription_tgfxhnd (*CreateRTSlotDescription_DepthStencil)(
    texture_tgfxhnd text0, texture_tgfxhnd text1, operationtype_tgfx depthOp,
    drawpassload_tgfx depthLoad, operationtype_tgfx stencilOp, drawpassload_tgfx stencilLoad,
    float depthClearValue, unsigned char stencilClearValue);
  rtslotusage_tgfx_handle (*CreateRTSlotUsage_Color)(RTSlotDescription_tgfxhnd base,
                                                     operationtype_tgfx        opType,
                                                     drawpassload_tgfx         LOADTYPE);
  rtslotusage_tgfx_handle (*CreateRTSlotUsage_Depth)(RTSlotDescription_tgfxhnd base,
                                                     operationtype_tgfx        depthOp,
                                                     drawpassload_tgfx         depthLoad,
                                                     operationtype_tgfx        stencilOp,
                                                     drawpassload_tgfx         stencilLoad);
  // Depth bounds extension
  depthsettings_tgfxhnd (*CreateDepthConfiguration)(unsigned char write, depthtest_tgfx compareOp,
                                                    extension_tgfxlsthnd exts);
  stencilcnfg_tgfxnd (*CreateStencilConfiguration)(unsigned char ref, unsigned char writeMask,
                                                   unsigned char       compareMask,
                                                   stencilcompare_tgfx compareOp,
                                                   stencilop_tgfx      depthFailOp,
                                                   stencilop_tgfx      stencilFailOp,
                                                   stencilop_tgfx      allSuccessOp);
  blendinginfo_tgfx_handle (*CreateBlendingConfiguration)(
    unsigned char colorSlotIndx, vec4_tgfx value, blendfactor_tgfx srcFctr_Clr,
    blendfactor_tgfx srcFctr_Alpha, blendfactor_tgfx dstFctr_Color, blendfactor_tgfx dstFctr_Alpha,
    blendmode_tgfx blendOp_Color, blendmode_tgfx blendOp_Alpha, colorcomponents_tgfx writeChannels);

  // EXTENSION HELPERS

  void (*destroyExtensionData)(extension_tgfx_handle ExtensionToDestroy);
  void (*EXT_IsVariableDescCount_andNonUniformIndexing_Supported)(unsigned char* isSupported);
  extension_tgfx_handle (*EXT_DepthBoundsInfo)(float bndMin, float bndMax);
} helper_tgfx;
