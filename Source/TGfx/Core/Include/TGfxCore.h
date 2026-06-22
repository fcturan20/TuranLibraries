#pragma once
#include <TCore.h>
#include "TGfxDeclarations.h"
TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(TGfx, "TGfx", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0));

typedef struct ITGfx
{
	struct tgfx_renderer* renderer;
	struct tgfx_dearImgui* imgui;
	struct tgfx_gpuDataManager* contentmanager;
	struct tgfx_helper* helpers;

	TGfxGpuHandle CreateGpu(const char* backendName);
	// logCode is an index to look up from all tgfx logs. You should use core->getLogMessage() for
	// text extraInfo is the text from the backend, probably contains specific info about your system
	enum result_tgfx (*load_backend)(enum backends_tgfx backend,
									 void (*printFnc)(unsigned int logCode, const wchar_t* extraInfo));
	// Don't use the GPU to create object/resources before init
	enum result_tgfx (*initGPU)(struct tgfx_gpu* gpu);
	enum result_tgfx (*getLogMessage)(unsigned int logCode, const wchar_t** logMessage);

	////////////// DISPLAY/WINDOWING FUNCTIONALITY

	// Also create a supported swapchain with create_swapchain() to use the window
	// Create a new swapchain in the callback too
	void (*createWindow)(const struct tgfx_windowDescription* desc, void* user, struct tgfx_window** window);
	// @param textures: Should point to an array of swapchainImageCount elements!
	enum result_tgfx (*createSwapchain)(struct tgfx_gpu* gpu,
										const struct tgfx_swapchainDescription* desc,
										struct tgfx_texture** textures);
	enum result_tgfx (*getCurrentSwapchainTextureIndex)(struct tgfx_window* window, unsigned int* index);
	// If count is zero, list isn't touched (count is used to return value).
	// If count is equal to returned value, list is filled.
	void (*getMonitorList)(unsigned int* monitorCount, struct tgfx_monitor** monitorList);
	void (*changeWindowResolution)(struct tgfx_window* WindowHandle, unsigned int width, unsigned int height);
} ITGfx;

TCORE_END_C_LINKAGE