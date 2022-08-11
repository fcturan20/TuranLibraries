#pragma once
#include "predefinitions_dx.h"
#include "tgfx_structs.h"
#include <atomic>

struct core_public {
	const std::vector<window_dx*>& GET_WINDOWs();
};

struct gpu_private;
struct GPU_VKOBJ {
private:
	friend struct core_functions_dx;
	friend struct allocatorsys_dx;
	friend struct queuesys_dx;
	gpu_private* hidden = nullptr;
	tgfx_gpu_description desc;
	unsigned int TRANSFERs_supportedqueuecount = 0, COMPUTE_supportedqueuecount = 0, GRAPHICS_supportedqueuecount = 0;


	ComPtr<ID3D12Device2> Logical_Device;
public:
	inline const char* DEVICENAME() { return desc.NAME; }
	inline const unsigned int APIVERSION() { return desc.API_VERSION; }
	inline const unsigned int DRIVERSION() { return desc.DRIVER_VERSION; }
	inline const gpu_type_tgfx DEVICETYPE() { return desc.GPU_TYPE; }
	inline const bool GRAPHICSSUPPORTED() { return GRAPHICS_supportedqueuecount; }
	inline const bool COMPUTESUPPORTED() { return COMPUTE_supportedqueuecount; }
	inline const bool TRANSFERSUPPORTED() { return TRANSFERs_supportedqueuecount; }
	inline ComPtr<ID3D12Device2> DEVICE() { return Logical_Device; }
};
struct monitor_dx {
	unsigned int width, height, color_bites, refresh_rate, physical_width, physical_height;
	const char* name;
	GLFWmonitor* monitorobj;
};

struct window_dx {
	unsigned int LASTWIDTH, LASTHEIGHT, NEWWIDTH, NEWHEIGHT;
	windowmode_tgfx DISPLAYMODE;
	monitor_dx* MONITOR;
	std::string NAME;
	tgfx_windowResizeCallback resize_cb = nullptr;
	void* UserPTR;
	unsigned char CurrentFrameSWPCHNIndex = 0;
	//To avoid calling resize or swapbuffer twice in a frame!
	std::atomic<bool> isResized = false, isSwapped = false;
	//Don't make these 3 again! There should 3 textures to be able to use 3 semaphores, otherwise 2 different semaphores for the same texture doesn't work as you expect!
	//Presentation Semaphores should only be used for rendergraph submission as wait
	semaphore_idtype_dx PresentationSemaphores[2];
	texture_dx* Swapchain_Textures[2];
	//Presentation Fences should only be used for CPU to wait
	fence_idtype_dx PresentationFences[2];


	// WindowDX handle.
	HWND Window_Handle;
	// Window rectangle (used to toggle fullscreen state).
	RECT PreviousWindowRect;
	// Swapchain object
	ComPtr<IDXGISwapChain4> Swapchain_Handle;
	GLFWwindow* GLFWHANDLE;
};