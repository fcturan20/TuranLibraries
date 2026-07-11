#pragma once
#include <TCoreMacros.h>
TCORE_BEGIN_C_LINKAGE

TCORE_DEFINE_HANDLE(TGfxGpu);
TCORE_DEFINE_HANDLE(TGfxSwapchain);
TCORE_DEFINE_HANDLE(TGfxTexture);
TCORE_DEFINE_HANDLE(TGfxMonitor);
TCORE_DEFINE_HANDLE(TGfxBuffer);
TCORE_DEFINE_HANDLE(TGfxSampler);
TCORE_DEFINE_HANDLE(TGfxShaderSource);
TCORE_DEFINE_HANDLE(TGfxBindingTable);
TCORE_DEFINE_HANDLE(TGfxCommandBuffer);
TCORE_DEFINE_HANDLE(TGfxCommandBundle);
TCORE_DEFINE_HANDLE(TGfxQueue);
TCORE_DEFINE_HANDLE(TGfxFence);
TCORE_DEFINE_HANDLE(TGfxHeap);
TCORE_DEFINE_HANDLE(TGfxPipeline);

typedef enum TGfxDataType TGfxDataType;
typedef enum TGfxCubeFace TGfxCubeFace;
typedef enum TGfxOperationType TGfxOperationType;
typedef enum TGfxRasterPassLoad TGfxRasterPassLoad;
typedef enum TGfxRasterPassStore TGfxRasterPassStore;
typedef enum TGfxCompare TGfxCompare;
typedef enum TGfxDepthMode TGfxDepthMode;
typedef enum TGfxCullMode TGfxCullMode;
typedef enum TGfxStencilOp TGfxStencilOp;
typedef enum TGfxBlendFactor TGfxBlendFactor;
typedef enum TGfxBlendMode TGfxBlendMode;
typedef enum TGfxPolygonMode TGfxPolygonMode;
typedef enum TGfxVertexListTypes TGfxVertexListTypes;
typedef enum TGfxTextureDimensions TGfxTextureDimensions;
typedef enum TGfxTextureMipmapFilter TGfxTextureMipmapFilter;
typedef enum TGfxTextureOrder TGfxTextureOrder;
typedef enum TGfxTextureWrapping TGfxTextureWrapping;
typedef enum TGfxTextureChannels TGfxTextureChannels;
typedef enum TGfxTextureAccess TGfxTextureAccess;
typedef enum TGfxGpuType TGfxGpuType;
typedef enum TGfxVsync TGfxVsync;
typedef enum TGfxWindowMode TGfxWindowMode;
typedef enum TGfxBackends TGfxBackends;
typedef enum TGfxShaderLanguages TGfxShaderLanguages;
typedef enum TGfxShaderStage TGfxShaderStage;
typedef enum TGfxPipelineType TGfxPipelineType;
typedef enum TGfxBarrierPlace TGfxBarrierPlace;
typedef enum TGfxTransferPassType TGfxTransferPassType;
typedef enum TGfxImageAccess TGfxImageAccess;
typedef enum TGfxSubDrawPassAccess TGfxSubDrawPassAccess;
typedef enum TGfxShaderDescriptorType TGfxShaderDescriptorType;
typedef enum TGfxMemoryAllocationType TGfxMemoryAllocationType;
typedef enum TGfxConstantSamplerColor TGfxConstantSamplerColor;
typedef enum TGfxColorSpace TGfxColorSpace;
typedef enum TGfxWindowComposition TGfxWindowComposition;
typedef enum TGfxWindowPresentation TGfxWindowPresentation;
typedef enum TGfxTextureComponentMask TGfxTextureComponentMask;
typedef enum TGfxTextureUsageMask TGfxTextureUsageMask;
typedef enum TGfxBufferUsageMask TGfxBufferUsageMask;
typedef enum TGfxVertexBindingInputRate TGfxVertexBindingInputRate;
typedef enum TGfxIndirectOperationType TGfxIndirectOperationType;

typedef struct TGfxRenderer TGfxRenderer;
typedef struct TGfxResourceManager TGfxResourceManager;

TCORE_END_C_LINKAGE