#include "tgfx_core.h"
#include <turanapi/predefinitions.h>
#include <turanapi/registrysys_tapi.h>
#include <assert.h>


typedef struct core_tgfx_d{
    core_tgfx_type* type;
} core_tgfx_d;
static core_tgfx_d* core_data;

result_tgfx load_backend(backends_tgfx backend, unsigned char is_Multithreaded){
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
    backend_load_func backendloader = (backend_load_func)DLIB_FUNC_LOAD_TAPI(backend_dll, "backend_load");
    return backendloader(core_data->type);
}

extern "C" FUNC_DLIB_EXPORT void* load_plugin(registrysys_tapi* regsys, unsigned char reload){
    core_tgfx_type* core = (core_tgfx_type*)malloc(sizeof(core_tgfx_type));
    core->data = (core_tgfx_d*)malloc(sizeof(core_tgfx_d));
    core->api = (core_tgfx*)malloc(sizeof(core_tgfx));
    core_data = core->data;
    
    core->data->type = core;

    core->api->load_backend = &load_backend;

    return core;
}
extern "C" FUNC_DLIB_EXPORT void unload_plugin(registrysys_tapi* regsys, unsigned char reload){

}