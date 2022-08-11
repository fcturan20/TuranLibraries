#pragma once
#include "tgfx_forwarddeclarations.h"

typedef struct tgfx_helper {
  //Hardware Capability Helpers
  void (*GetGPUInfo_General)(gpu_tgfxhnd GPUhnd, gpuDescription_tgfx* desc);
  unsigned char (*GetTextureTypeLimits)
  (
    texture_dimensions_tgfx dims, textureOrder_tgfx dataorder,
    textureChannels_tgfx channeltype, textureUsageFlag_tgfxhnd usageflag,
    gpu_tgfxhnd GPUHandle, unsigned int* MAXWIDTH, unsigned int* MAXHEIGHT,
    unsigned int* MAXDEPTH, unsigned int* MAXMIPLEVEL
  );
  textureUsageFlag_tgfxhnd(*CreateTextureUsageFlag)
  (
    unsigned char copyFrom, unsigned char copyTo, unsigned char renderTo,
    unsigned char sampledReadOnly, unsigned char randomWrittenTo
  );
  void (*GetSupportedAllocations_ofTexture)
  (unsigned int GPUIndex, unsigned int* SupportedMemoryTypesBitset);
  void (*GetMonitor_Resolution_ColorBites_RefreshRate)
  (
    monitor_tgfxhnd MonitorHandle, unsigned int* WIDTH,
    unsigned int* HEIGHT, unsigned int* ColorBites, unsigned int* RefreshRate
  );



  // You can't create a memory type, only set allocation size of it
  //  (Memory Type is given by the GFX initialization process)
  result_tgfx(*SetMemoryTypeInfo)
  (unsigned int ID, unsigned long long allocSize, extension_tgfxlsthnd exts);

  initSecondStageInfo_tgfxhnd (*createInitSecondStageInfo)
  (initSecondStageInfo_tgfx* info, extension_tgfxlsthnd EXTList);


  shaderStageFlag_tgfxhnd(*CreateShaderStageFlag)(unsigned char count, ...);
  RTSlotDescription_tgfxhnd(*CreateRTSlotDescription_Color)
  (
    texture_tgfxhnd text0, texture_tgfxhnd text1, operationtype_tgfx OPTYPE,
    drawpassload_tgfx LOADTYPE, unsigned char useLater, unsigned char slotIndx,
    vec4_tgfx CLEARVALUE
  );
  RTSlotDescription_tgfxhnd(*CreateRTSlotDescription_DepthStencil)
  (
    texture_tgfxhnd text0, texture_tgfxhnd text1, operationtype_tgfx depthOp,
    drawpassload_tgfx depthLoad, operationtype_tgfx stencilOp,
    drawpassload_tgfx stencilLoad, float depthClearValue,
    unsigned char stencilClearValue
  );
  rtslotusage_tgfx_handle(*CreateRTSlotUsage_Color)
  (
    RTSlotDescription_tgfxhnd base, operationtype_tgfx opType,
    drawpassload_tgfx LOADTYPE
  );
  rtslotusage_tgfx_handle(*CreateRTSlotUsage_Depth)
  (
    RTSlotDescription_tgfxhnd base, operationtype_tgfx depthOp,
    drawpassload_tgfx depthLoad, operationtype_tgfx stencilOp,
    drawpassload_tgfx stencilLoad
  );
  //Depth bounds extension
  depthsettings_tgfxhnd(*CreateDepthConfiguration)
    (unsigned char write, depthtest_tgfx compareOp, extension_tgfxlsthnd exts);
  stencilcnfg_tgfxnd(*CreateStencilConfiguration)
  (
    unsigned char ref, unsigned char writeMask, unsigned char compareMask,
    stencilcompare_tgfx compareOp, stencilop_tgfx depthFailOp,
    stencilop_tgfx stencilFailOp, stencilop_tgfx allSuccessOp
  );
  blendinginfo_tgfx_handle(*CreateBlendingConfiguration)
  (
    unsigned char colorSlotIndx, vec4_tgfx value, blendfactor_tgfx srcFctr_Clr,
    blendfactor_tgfx srcFctr_Alpha, blendfactor_tgfx dstFctr_Color,
    blendfactor_tgfx dstFctr_Alpha, blendmode_tgfx blendOp_Color,
    blendmode_tgfx blendOp_Alpha, colorcomponents_tgfx writeChannels
  );

  //EXTENSION HELPERS

  void (*Destroy_ExtensionData)(extension_tgfx_handle ExtensionToDestroy);
  void (*EXT_IsVariableDescCount_andNonUniformIndexing_Supported)
    (unsigned char* isSupported);
  extension_tgfx_handle(*EXT_DepthBoundsInfo)(float bndMin, float bndMax);
} helper_tgfx;
