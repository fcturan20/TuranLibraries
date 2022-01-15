#pragma once
#include "tgfx_forwarddeclarations.h"
#define TGFX_PLUGIN_NAME "tgfx_core"
#define TGFX_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0,0,0)
#define TGFX_PLUGIN_LOAD_TYPE core_tgfx_type*

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
	result_tgfx (*load_backend)(backends_tgfx backend);
	void (*initialize_secondstage)(initializationsecondstageinfo_tgfx_handle info);
	//SwapchainTextureHandles should point to an array of 2 elements! Triple Buffering is not supported for now.
	void (*create_window)(unsigned int WIDTH, unsigned int HEIGHT, monitor_tgfx_handle monitor, 
		windowmode_tgfx Mode, const char* NAME, textureusageflag_tgfx_handle SwapchainUsage, tgfx_windowResizeCallback ResizeCB, 
		void* UserPointer, texture_tgfx_handle* SwapchainTextureHandles, window_tgfx_handle* window);
	void (*change_window_resolution)(window_tgfx_handle WindowHandle, unsigned int width, unsigned int height);
	void (*getmonitorlist)(monitor_tgfx_listhandle* MonitorList);
	void (*getGPUlist)(gpu_tgfx_listhandle* GpuList);

	//Debug callbacks are user defined callbacks, you should assign the function pointer if you want to use them
	//As default, all backends set them as empty no-work functions
	void (*debugcallback)(result_tgfx result, const char* Text);
	//You can set this if TGFX is started with threaded call.
	void (*debugcallback_threaded)(unsigned char ThreadIndex, result_tgfx Result, const char* Text);




	void (*take_inputs)();
	//Destroy GFX API systems to create again (with a different Graphics API maybe?) or close application
	//This will close all of the systems with "GFX" prefix and you shouldn't call them either
	void (*destroy_tgfx_resources)();
} core_tgfx;

//This is for backend to implement
typedef struct core_tgfx_d core_tgfx_d;
typedef struct core_tgfx_type {
	core_tgfx_d* data;
	core_tgfx* api;
} core_tgfx_type;

//While coding GFX, we don't know the name of the handles, so we need to convert them according to the namespace preprocessor
#define TGFXLISTCOUNT(gfxcoreptr, listobject, countername) unsigned int countername = 0;  while (listobject[countername] != gfxcoreptr->INVALIDHANDLE) { countername++; }
typedef struct registrysys_tapi registrysys_tapi;
//This function should be exported by the backend dll
typedef result_tgfx (*backend_load_func)(registrysys_tapi* regsys, core_tgfx_type* core);