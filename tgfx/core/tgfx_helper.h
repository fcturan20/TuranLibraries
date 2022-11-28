#pragma once
#include "tgfx_forwarddeclarations.h"

typedef struct tgfx_helper {
  // Hardware Capability Helpers
  void (*getGPUInfo_General)(gpu_tgfxhnd GPUhnd, gpuDescription_tgfx* desc);
  void (*getGPUInfo_Queues)(gpu_tgfxhnd GPUhnd, unsigned int queueFamIndx,
                            gpuQueue_tgfxlsthnd* queueList);
  unsigned char (*getTextureTypeLimits)(texture_dimensions_tgfx dims, textureOrder_tgfx dataorder,
                                        textureChannels_tgfx      channeltype,
                                        textureUsageMask_tgfxflag usageflag, gpu_tgfxhnd GPUHandle,
                                        unsigned int* MAXWIDTH, unsigned int* MAXHEIGHT,
                                        unsigned int* MAXDEPTH, unsigned int* MAXMIPLEVEL);
  result_tgfx (*getWindow_GPUSupport)(window_tgfxhnd window, gpu_tgfxhnd gpu,
                                      windowGPUsupport_tgfx* info);

  void (*getTextureSupportedMemTypes)(texture_tgfxhnd texture,
                                      unsigned int*   SupportedMemoryTypesBitset);
  void (*getMonitorInfo)(monitor_tgfxhnd MonitorHandle, unsigned int* WIDTH, unsigned int* HEIGHT,
                         unsigned int* ColorBites, unsigned int* RefreshRate);

  // TO BE USED IN TGFX_SUBPASS
  RTSlotDescription_tgfxhnd (*CreateRTSlotDescription_Color)(
    texture_tgfxhnd text0, texture_tgfxhnd text1, operationtype_tgfx OPTYPE,
    rasterpassLoad_tgfx LOADTYPE, unsigned char useLater, unsigned char slotIndx,
    typelessColor_tgfx CLEARVALUE);
  RTSlotDescription_tgfxhnd (*CreateRTSlotDescription_DepthStencil)(
    texture_tgfxhnd text0, texture_tgfxhnd text1, operationtype_tgfx depthOp,
    rasterpassLoad_tgfx depthLoad, operationtype_tgfx stencilOp, rasterpassLoad_tgfx stencilLoad,
    double depthClearValue, unsigned char stencilClearValue);
  rtslotusage_tgfx_handle (*CreateRTSlotUsage_Color)(RTSlotDescription_tgfxhnd base,
                                                     operationtype_tgfx        opType,
                                                     rasterpassLoad_tgfx       LOADTYPE);
  rtslotusage_tgfx_handle (*CreateRTSlotUsage_Depth)(RTSlotDescription_tgfxhnd base,
                                                     operationtype_tgfx        depthOp,
                                                     rasterpassLoad_tgfx       depthLoad,
                                                     operationtype_tgfx        stencilOp,
                                                     rasterpassLoad_tgfx       stencilLoad);
  // EXTENSION HELPERS

  void (*destroyExtensionData)(extension_tgfx_handle ExtensionToDestroy);
  void (*EXT_IsVariableDescCount_andNonUniformIndexing_Supported)(unsigned char* isSupported);
} helper_tgfx;
