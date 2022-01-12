#include "tgfx_forwarddeclarations.h"

typedef struct core_tgfx{
    renderer_tgfx* renderer;
    dearimgui_tgfx* imgui;
    gpudatamanager_tgfx* contentmanager;
    helper_tgfx* helpers;

	//Invalid handle is used to terminate array of i/o. NULL is used to say i/o is a failed handle and ignored if possible.
	const void* INVALIDHANDLE;

    /*
    If isMultiThreaded is 0 :
    * 1) You shouldn't call TGFX functions concurrently. So your calls to TGFX functions should be externally synchronized.
    * 2) TGFX never uses multiple threads. So GPU command creation process may take longer on large workloads
    * If isMultiThreaded is 1 :
    * 1) You shouldn't call TGFX functions concurrently. So your calls to TGFX functions should be externally synchronized.
    * 2) TGFX uses multiple threads to create GPU commands faster.
    * If isMultiThreaded is 2 :
    * 1) You can call TGFX functions concurrently. You can call all TGFX functions asynchronously unless otherwise is mentioned.
    * 2) TGFX uses multiple threads to create GPU commands faster.
    */
	result_tgfx (*load_backend)(backends_tgfx backend, unsigned char is_Multithreaded);
	void (*Initialize_SecondStage)(initializationsecondstageinfo_tgfx_handle info);
	//SwapchainTextureHandles should point to an array of 2 elements! Triple Buffering is not supported for now.
	void (*CreateWindow)(unsigned int WIDTH, unsigned int HEIGHT, monitor_tgfx_handle monitor, 
		windowmode_tgfx Mode, const char* NAME, textureusageflag_tgfx_handle SwapchainUsage, tgfx_windowResizeCallback ResizeCB, 
		void* UserPointer, texture_tgfx_handle* SwapchainTextureHandles, window_tgfx_handle* window);
	void (*Change_Window_Resolution)(window_tgfx_handle WindowHandle, unsigned int width, unsigned int height);
	void (*GetMonitorList)(monitor_tgfx_listhandle* MonitorList);
	void (*GetGPUList)(gpu_tgfx_listhandle* GpuList);

	//Debug callbacks are user defined callbacks, you should assign the function pointer if you want to use them
	//As default, all backends set them as empty no-work functions
	void (*DebugCallback)(result_tgfx result, const char* Text);
	//You can set this if TGFX is started with threaded call.
	void (*DebugCallback_Threaded)(unsigned char ThreadIndex, result_tgfx Result, const char* Text);




	void (*Take_Inputs)();
	//Destroy GFX API systems to create again (with a different Graphics API maybe?) or close application
	//This will close all of the systems with "GFX" prefix and you shouldn't call them either
	void (*Destroy_GFX_Resources)();
} core_tgfx;

typedef struct core_tgfx_d core_tgfx_d;
typedef struct core_tgfx_type {
	core_tgfx_d* data;
	core_tgfx* api;
} tgfx_core;

//While coding GFX, we don't know the name of the handles, so we need to convert them according to the namespace preprocessor
#define TGFXLISTCOUNT(gfxcoreptr, listobject, countername) unsigned int countername = 0;  while (listobject[countername] != gfxcoreptr->INVALIDHANDLE) { countername++; }
//This function should be exported by the backend dll
typedef result_tgfx (*backend_load_func)(core_tgfx_type* core);