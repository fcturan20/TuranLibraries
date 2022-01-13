#pragma once
#include "Vulkan_Resource.h"
#include "VK_GPUContentManager.h"
#include "Vulkan_Renderer_Core.h"
#include "IMGUI_VK.h"





class Vulkan_Core {
public:
	//Window Operations
	std::vector<vkMONITOR*> MONITORs;
	std::vector<WINDOW*> WINDOWs;

	static bool Create_WindowSwapchain(WINDOW* Vulkan_Window, unsigned int WIDTH, unsigned int HEIGHT, VkSwapchainKHR* SwapchainOBJ, VK_Texture** SwapchainTextureHandles);

	//TGFX functions to point
	static void ChangeWindowResolution(tgfx_window WindowHandle, unsigned int Width, unsigned int Height);
	static void Initialize_SecondStage(tgfx_initializationsecondstageinfo info);
	//SwapchainTextureHandles should point to an array of 2 elements! Triple Buffering is not supported for now.
	static void CreateWindow(unsigned int WIDTH, unsigned int HEIGHT, tgfx_monitor monitor,
		tgfx_windowmode Mode, const char* NAME, tgfx_textureusageflag SwapchainUsage, tgfx_windowResizeCallback ResizeCB, void* UserPointer,
		tgfx_texture* SwapchainTextureHandles, tgfx_window* window);
	//Debug callbacks are user defined callbacks, you should assign the function pointer if you want to use them
	//As default, all backends set them as empty no-work functions
	static void TGFXDebugCallback(tgfx_result Result, const char* Text);
	static void TGFXDebugCallback_Threaded(unsigned char ThreadIndex, tgfx_result Result, const char* Text);
	static void Take_Inputs();
	//Destroy GFX API systems to create again (with a different Graphics API maybe?) or close application
	//This will close all of the systems with "GFX" prefix and you shouldn't call them either
	static void Destroy_GFX_Resources();
	static void GetMonitorList(tgfx_monitor_list* List);
	static void GetGPUList(tgfx_gpu_list* List);

	Vulkan_Core(tapi_threadingsystem JobSystem);
	~Vulkan_Core();
};
