#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef int textureUsageMask_tgfxflag;
typedef int bufferUsageMask_tgfxflag;
typedef struct tgfx_helper {
  // Hardware Capability Helpers
  void (*getGPUInfo_General)(struct tgfx_gpu* GPUhnd, struct tgfx_gpuDescription* desc);
  // If queueCount is 0, queueList isn't touched. Only queueCount is changed.
  // If queueCount is a previously returned value, queueList is filled.
  // queueCount has to be a valid pointer
  void (*getGPUInfo_Queues)(struct tgfx_gpu* GPUhnd, unsigned int queueFamIndx, unsigned int* queueCount,
                            struct tgfx_gpuQueue** queueList);
  unsigned char (*getTextureTypeLimits)(enum texture_dimensions_tgfx dims, enum textureOrder_tgfx dataorder,
                                        enum textureChannels_tgfx      channeltype,
                                        textureUsageMask_tgfxflag usageflag, struct tgfx_gpu* GPUHandle,
                                        unsigned int* MAXWIDTH, unsigned int* MAXHEIGHT,
                                        unsigned int* MAXDEPTH, unsigned int* MAXMIPLEVEL);
  result_tgfx (*getWindow_GPUSupport)(struct tgfx_window* window, struct tgfx_gpu* gpu,
                                      struct tgfx_windowGPUsupport* info);

  void (*getTextureSupportedMemTypes)(struct tgfx_texture* texture,
                                      unsigned int*   SupportedMemoryTypesBitset);
  void (*getMonitorInfo)(struct tgfx_monitor* MonitorHandle, unsigned int* WIDTH, unsigned int* HEIGHT,
                         unsigned int* ColorBites, unsigned int* RefreshRate);

  // EXTENSION HELPERS

  void (*destroyExtensionData)(struct tgfx_extension* ext);
  void (*EXT_IsVariableDescCount_andNonUniformIndexing_Supported)(unsigned char* isSupported);
} tgfx_helper;

#ifdef __cplusplus
}
#endif