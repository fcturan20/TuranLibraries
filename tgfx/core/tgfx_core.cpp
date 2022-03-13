#include "tgfx_core.h"
#include <predefinitions_tapi.h>
#include <registrysys_tapi.h>
#include <assert.h>
#include "tgfx_gpucontentmanager.h"
#include "tgfx_helper.h"
#include "tgfx_imgui.h"
#include "tgfx_renderer.h"
#include <string>


static core_tgfx_type* core_type_ptr;
static registrysys_tapi* core_regsys;

result_tgfx load_backend(backends_tgfx backend, tgfx_PrintLogCallback printcallback){
    const char* path = nullptr;
	switch (backend)
	{
	case backends_tgfx_VULKAN:
		path = "TGFXVulkan.dll";
		break;
	case backends_tgfx_OPENGL4:
		path = "TGFXOpenGL4.dll";
		break;
	default:
		assert(0 && "This backend can't be loaded!");
		return result_tgfx_FAIL;
	};

    auto backend_dll = DLIB_LOAD_TAPI(path);
	if (!backend_dll) { printcallback(result_tgfx_FAIL, ("There is no " + std::string(path) + "% s file!").c_str()); return result_tgfx_FAIL; }
    backend_load_func backendloader = (backend_load_func)DLIB_FUNC_LOAD_TAPI(backend_dll, "backend_load");
	if(!backendloader){ printcallback(result_tgfx_FAIL, "TGFX Backend doesn't have any backend_load() function, which it should. You are probably using a newer version that uses different load scheme!"); return result_tgfx_FAIL; }
    return backendloader(core_regsys, core_type_ptr, printcallback);
}

extern "C" FUNC_DLIB_EXPORT void* load_plugin(registrysys_tapi* regsys, unsigned char reload){
    core_tgfx_type* core = (core_tgfx_type*)malloc(sizeof(core_tgfx_type));
    core->api = (core_tgfx*)malloc(sizeof(core_tgfx));
    core_type_ptr = core;
    core_regsys = regsys;
	core->api->contentmanager = new gpudatamanager_tgfx;
	core->api->helpers = new helper_tgfx;
	core->api->imgui = new dearimgui_tgfx;
	core->api->renderer = new renderer_tgfx;

    core->api->load_backend = &load_backend;

	regsys->add(TGFX_PLUGIN_NAME, TGFX_PLUGIN_VERSION, core);

    return core;
}
extern "C" FUNC_DLIB_EXPORT void unload_plugin(registrysys_tapi* regsys, unsigned char reload){

}