#pragma once
#include <TCore.h>
#include <TGfxMath.h>
#include "TGfxDeclarations.h"
TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(TGfx, "TGfx", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0));

typedef enum TGfxWindowComposition
{
	TGFX_WINDOWCOMPOSITION_OPAQUE
} TGfxWindowComposition;

typedef struct TGfxSwapchainDescription
{
	TGfxGpu Gpu;
	TGfxWindowPresentation Presentation;
	TGfxWindowComposition Composition;
	TGfxColorSpace ColorSpace;
	TGfxTextureChannels Channels;
	TU4 ImageCount;
	TGfxUVec2 ImageExtent;
	// If TCoreWindowing is used, this should be a TCoreWindowHnd
	// Otherwise, this should be a platform-specific window handle (e.g., HWND on Windows)
	void* WindowHnd;
} TGfxSwapchainDescription;

typedef enum TGfxGpuType
{
	TGFX_GPUTYPE_DISCRETE,
	TGFX_GPUTYPE_INTEGRATED
} TGfxGpuType;

typedef struct TGfxMemoryInfo
{
	TU1 MemoryTypeId;
	TGfxMemoryAllocationType AllocationType;
	TU8 MaxAllocationSize;
} TGfxMemoryInfo;

typedef struct TGfxGpuInfo
{
	const char* Name;
	TU4 GfxApiVersion, DriverVersion;
	TGfxGpuType Type;
	TBool OperationSupport_Raster, OperationSupport_Compute, OperationSupport_Transfer;
	TU4 QueueFamilyCount;
	TU4 MemRegionsCount;
	const TGfxMemoryInfo* MemRegions;
} TGfxGpuInfo;

#define TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT 24
#define TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT 64
#define TGFX_WINDOWGPUSUPPORT_MAXPRESENTATIONMODE 6
typedef struct TGfxGpuSwapchainSupportInfo
{
	TU4 MaxImageCount;
	TGfxUVec2 MinExtent, MaxExtent;
	TGfxWindowPresentation PresentationModes[TGFX_WINDOWGPUSUPPORT_MAXPRESENTATIONMODE];
	TGfxColorSpace ColorSpace[TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT];
	TGfxTextureChannels Channels[TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT];
	TGfxQueue Queues[TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT];
} TGfxGpuSwapchainSupportInfo;

typedef struct ITGfx
{
	// Checks for a backend in plugins and validates it
	TCResult (*RegisterBackend)(const char* backendName);
	// Mostly used by backends
	TCResultState (*GetResultStateByReturnCode)(TU4 returnCode, const char** message);

	ITGfxRenderer* Renderer;
	ITGfxResourceManager* ResourceManager;

	TCResult (*CreateSwapchain)(const TGfxSwapchainDescription* desc, TGfxSwapchain* swapchain);
	TCResult (*GetCurrentSwapchainTextureIndex)(TGfxSwapchain swapchain, TU4* index);
	TCResult (*ChangeSwapchainResolution)(TGfxSwapchain swapchain, TGfxUVec2 newSize);
	void (*DestroySwapchain)(TGfxSwapchain swapchain);
	TCResult (*QuerySwapchainSupportOfGpu)(TGfxGpu gpu, void* windowOsHnd, TGfxGpuSwapchainSupportInfo* info);
} ITGfx;

// Backend must register to TCore with this struct as API
// Otherwise it can't hook to TGfxCore
typedef struct TGfxBackendFunctions
{
	TCResult (*Hook)(ITGfx* gfx);
} TGfxBackendFunctions;

TCORE_END_C_LINKAGE