#include "predefinitions_dx.h"
#include "core.h"

//Load TuranLibraries
#include <registrysys_tapi.h>
#include <virtualmemorysys_tapi.h>
#include <threadingsys_tapi.h>
#include <tgfx_core.h>
#include <tgfx_helper.h>
#include <tgfx_renderer.h>
#include <tgfx_gpucontentmanager.h>
#include <tgfx_imgui.h>
#include <vector>


struct core_private {
public:
	std::vector<gpu_public*> DEVICE_GPUs;
	//Window Operations
	std::vector<monitor_dx*> MONITORs;
	std::vector<window_dx*> WINDOWs;


	bool isAnyWindowResized = false;	//Instead of checking each window each frame, just check this
};
static core_private* hidden = nullptr;

struct core_functions {
public:
	static inline void initialize_secondstage(initializationsecondstageinfo_tgfx_handle info);
	//SwapchainTextureHandles should point to an array of 2 elements! Triple Buffering is not supported for now.
	static inline void createwindow_vk(unsigned int WIDTH, unsigned int HEIGHT, monitor_tgfx_handle monitor,
		windowmode_tgfx Mode, const char* NAME, textureusageflag_tgfx_handle SwapchainUsage, tgfx_windowResizeCallback ResizeCB,
		void* UserPointer, texture_tgfx_handle* SwapchainTextureHandles, window_tgfx_handle* window);
	static inline void change_window_resolution(window_tgfx_handle WindowHandle, unsigned int width, unsigned int height);
	static inline void getmonitorlist(monitor_tgfx_listhandle* MonitorList);
	static inline void getGPUlist(gpu_tgfx_listhandle* GpuList);

	//Debug callbacks are user defined callbacks, you should assign the function pointer if you want to use them
	//As default, all backends set them as empty no-work functions
	static inline void debugcallback(result_tgfx result, const char* Text);
	//You can set this if TGFX is started with threaded call.
	static inline void debugcallback_threaded(unsigned char ThreadIndex, result_tgfx Result, const char* Text);


	static void destroy_tgfx_resources();
	static void take_inputs();

	static inline void set_helper_functions();
	static inline void Save_Monitors();
	static inline void Create_Instance();
	static inline void Setup_Debugging();
	static inline void Check_Computer_Specs();
	static inline void createwindow_vk(unsigned int WIDTH, unsigned int HEIGHT, monitor_tgfx_handle monitor,
		windowmode_tgfx Mode, const char* NAME, textureusageflag_tgfx_handle SwapchainUsage, tgfx_windowResizeCallback ResizeCB, void* UserPointer,
		texture_dx* SwapchainTextureHandles, window_tgfx_handle* window);

};

inline void ThrowIfFailed(HRESULT hr){
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

void printf_log_tgfx(result_tgfx result, const char* text) {
	printf("TGFX %u: %s\n", (unsigned int)result, text);
}
extern "C" FUNC_DLIB_EXPORT result_tgfx backend_load(registrysys_tapi * regsys, core_tgfx_type * core, tgfx_PrintLogCallback printcallback) {
	if (!regsys->get(TGFX_PLUGIN_NAME, TGFX_PLUGIN_VERSION, 0))
		return result_tgfx_FAIL;

	core->api->INVALIDHANDLE = regsys->get(VIRTUALMEMORY_TAPI_PLUGIN_NAME, VIRTUALMEMORY_TAPI_PLUGIN_VERSION, 0);
	//Check if threading system is loaded
	THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE threadsysloaded = (THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE)regsys->get(THREADINGSYS_TAPI_PLUGIN_NAME, THREADINGSYS_TAPI_PLUGIN_VERSION, 0);
	if (threadsysloaded) {
		threadingsys = threadsysloaded->funcs;
	}

	core_dx = new core_public;
	core->data = (core_tgfx_d*)core_dx;
	core_tgfx_main = core->api;
	hidden = new core_private;

	if (printcallback) { printer = printcallback; }
	else { printer = printf_log_tgfx; }
}