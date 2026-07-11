#pragma once
#include <TCore.h>
#include <TGfxMath.h>
#include "TGfxDeclarations.h"
TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(TGfx, "TGfx", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0));

typedef struct TGfxSwapchainDescription
{
	TGfxUI2 Size;
	TGfxWindowMode Mode;
	const char* Name;
	void* WindowNativeHnd; // Platform specific window handle
} TGfxSwapchainDescription;

typedef struct ITGfx
{
	// Checks for a backend in plugins and validates it
	TCResult (*RegisterBackend)(const char* backendName);

	TGfxRenderer* Renderer;
	TGfxResourceManager* ContentManager;

	void (*CreateSwapchain)(const TGfxSwapchainDescription* desc, TGfxSwapchainHnd* swapchain);
	enum result_tgfx (*GetCurrentSwapchainTextureIndex)(TGfxSwapchainHnd swapchain, TUint* index);
	void (*ChangeSwapchainResolution)(TGfxSwapchainHnd swapchain, TUint width, TUint height);
} ITGfx;

// Backend must register to TCore with this struct as API
// Otherwise it can't hook to TGfxCore
typedef struct TGfxBackendFunctions
{
	TCResult (*Hook)(ITGfx* gfx);
} TGfxBackendFunctions;

TCORE_END_C_LINKAGE