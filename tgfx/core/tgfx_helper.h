#pragma once
#include "tgfx_forwarddeclarations.h"

typedef struct tgfx_helper {
  // Hardware Capability Helpers
  void (*getGPUInfo_General)(gpu_tgfxhnd GPUhnd, gpuDescription_tgfx* desc);
  // If queueCount is 0, queueList isn't touched. Only queueCount is changed.
  // If queueCount is a previously returned value, queueList is filled.
  // queueCount has to be a valid pointer
  void (*getGPUInfo_Queues)(gpu_tgfxhnd GPUhnd, unsigned int queueFamIndx, unsigned int* queueCount,
                            gpuQueue_tgfxhnd* queueList);
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

  // EXTENSION HELPERS

  void (*destroyExtensionData)(extension_tgfxhnd ext);
  void (*EXT_IsVariableDescCount_andNonUniformIndexing_Supported)(unsigned char* isSupported);
} helper_tgfx;
