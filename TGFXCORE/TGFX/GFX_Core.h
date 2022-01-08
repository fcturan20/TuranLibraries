#pragma once
//User may define preprocessors wrong. Before including anything else, give the error

#ifndef STATIC_GFXERROR
#include "GFX_Includes.h"
#include "GFX_Renderer.h"
#include "GPU_ContentManager.h"
#include "GFX_Helper.h"
#include "IMGUI_Core.h"




#if defined(__cplusplus)
extern "C" {
#endif

	typedef struct tgfx_core {
		tgfx_renderer Renderer;
		tgfx_imguicore IMGUI;
		tgfx_gpudatamanager ContentManager;
		tgfx_helper Helpers;
		//Invalid handle is used to terminate array of i/o. NULL is used to say i/o is a failed handle and ignored if possible.
		const void* INVALIDHANDLE;

		void (*Initialize_SecondStage)(tgfx_initializationsecondstageinfo info);
		//SwapchainTextureHandles should point to an array of 2 elements! Triple Buffering is not supported for now.
		void (*CreateWindow)(unsigned int WIDTH, unsigned int HEIGHT, tgfx_monitor monitor, 
			tgfx_windowmode Mode, const char* NAME, tgfx_textureusageflag SwapchainUsage, tgfx_windowResizeCallback ResizeCB, 
			void* UserPointer, tgfx_texture* SwapchainTextureHandles, tgfx_window* window);
		void (*Change_Window_Resolution)(tgfx_window WindowHandle, unsigned int width, unsigned int height);
		void (*GetMonitorList)(tgfx_monitor_list* MonitorList);
		void (*GetGPUList)(tgfx_gpu_list* GpuList);

		//Debug callbacks are user defined callbacks, you should assign the function pointer if you want to use them
		//As default, all backends set them as empty no-work functions
		void (*DebugCallback)(tgfx_result result, const char* Text);
		//You can set this if TGFX is started with threaded call.
		void (*DebugCallback_Threaded)(unsigned char ThreadIndex, tgfx_result Result, const char* Text);




		void (*Take_Inputs)();
		//Destroy GFX API systems to create again (with a different Graphics API maybe?) or close application
		//This will close all of the systems with "GFX" prefix and you shouldn't call them either
		void (*Destroy_GFX_Resources)();
	} tgfx_core;

#if defined(__cplusplus)
}
#endif


//This preprocessor is used by the GFX backends, so don't use it!
#ifdef GFX_BACKENDBUILD
/*If isMultiThreaded is 0 :
* 1) You shouldn't call TGFX functions concurrently. So your calls to TGFX functions should be externally synchronized.
* 2) TGFX never uses multiple threads. So GPU command creation process may take longer on large workloads
* If isMultiThreaded is 1 :
* 1) You shouldn't call TGFX functions concurrently. So your calls to TGFX functions should be externally synchronized.
* 2) TGFX uses multiple threads to create GPU commands faster.
* If isMultiThreaded is 2 :
* 1) You can call TGFX functions concurrently. You can call all TGFX functions asynchronously unless otherwise is mentioned.
* 2) TGFX uses multiple threads to create GPU commands faster.
*/
typedef tgfx_core* (*TGFXBackendLoader)(unsigned char isMultiThreaded);
#endif
extern "C" tgfx_core* Create_GFXAPI(tgfx_backends StartBackend);
extern "C" void Destroy_GFXAPI(tgfx_core core);


#endif	//STATIC_GFXERROR