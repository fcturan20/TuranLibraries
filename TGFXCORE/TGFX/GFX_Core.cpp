#define GFX_BACKENDBUILD
#include <iostream>
#include "GFX_Core.h"
#include <assert.h>


#if _WIN32 || _WIN64
#include <Windows.h>
#include <libloaderapi.h>

tgfx_core* Create_GFXAPI(tgfx_backends backend) {
	HMODULE Lib;
	switch (backend)
	{
	case tgfx_backends_VULKAN:
		Lib = LoadLibrary(TEXT("TGFXVulkan.dll"));
		break;
	case tgfx_backends_OPENGL4:
		Lib = LoadLibrary(TEXT("TGFXOpenGL4.dll"));
		break;
	default:
		assert(0 && "This backend can't be loaded!");
		return NULL;
	};
	if (Lib == NULL) {
		assert(0 && "Backend file isn't found! ");
		return NULL;
	}
	TGFXBackendLoader backend_loader = (TGFXBackendLoader)GetProcAddress(Lib, "Load_TGFXBackend");
	if (backend_loader == NULL) {
		assert(0 && "Backend's load function isn't found! ");
		return NULL;
	}

	tgfx_core* core_ptr = backend_loader(0);
	if (core_ptr == NULL) {
		assert(0 && "Backend didn't return a valid TGFX_CORE object, so TGFX creation process has failed!");
		return NULL;
	}



	return core_ptr;
}

#elif defined(__linux__)
#include <stdlib.h>
#include <dlfcn.h>
tgfx_core* Create_GFXAPI(tgfx_backends backend){
	void* handle = nullptr;

	switch (backend)
	{
	case tgfx_backends_VULKAN:
  		handle = dlopen ("TGFXVulkan.so", RTLD_LAZY);
		break;
	case tgfx_backends_OPENGL4:

		break;
	default:
		assert(0 && "Specified backend isn't supported by Linux loader!");
		break;
	}
   	if (!handle) {
	  	fprintf (stderr, "%s\n", dlerror());
    	exit(1);
    }
    dlerror();
	TGFXBackendLoader backend_loader = (TGFXBackendLoader)dlsym(handle, "Load_TGFXBackend");
	if(!backend_loader){
		assert(0 && "Backend didn't return a valid TGFX_CORE object, so TGFX creation process has failed!");
		return NULL;
	}
	tgfx_core* core_ptr = backend_loader(0);
	if (core_ptr == NULL) {
		assert(0 && "Backend didn't return a valid TGFX_CORE object, so TGFX creation process has failed!");
		return NULL;
	}

	return core_ptr;
}
#else
#error "Platform isn't supported!"
#endif





//HELPER FUNCTIONS
GFXAPI unsigned int tgfx_Get_UNIFORMTYPEs_SIZEinbytes(tgfx_datatype uniform) {
	switch (uniform)
	{
	case tgfx_datatype_VAR_BYTE8:
	case tgfx_datatype_VAR_UBYTE8:
		return 1;
	case tgfx_datatype_VAR_FLOAT32:
	case tgfx_datatype_VAR_INT32:
	case tgfx_datatype_VAR_UINT32:
		return 4;
	case tgfx_datatype_VAR_VEC2:
		return 8;
	case tgfx_datatype_VAR_VEC3:
		return 12;
	case tgfx_datatype_VAR_VEC4:
		return 16;
	case tgfx_datatype_VAR_MAT4x4:
		return 64;
	default:
		assert(0 && "Uniform's size in bytes isn't set in GFX_ENUMs.cpp!");
		return 0;
	}
}
GFXAPI const char* tgfx_Find_UNIFORM_VARTYPE_Name(tgfx_datatype uniform_var_type) {
	switch (uniform_var_type) {
	case tgfx_datatype_VAR_UINT32:
		return "Unsigned Integer 32-bit";
	case tgfx_datatype_VAR_INT32:
		return "Signed Integer 32-bit";
	case tgfx_datatype_VAR_FLOAT32:
		return "Float 32-bit";
	case tgfx_datatype_VAR_VEC2:
		return "Vec2 (2 float";
	case tgfx_datatype_VAR_VEC3:
		return "Vec3 (3 float";
	case tgfx_datatype_VAR_VEC4:
		return "Vec4 (4 float";
	case tgfx_datatype_VAR_MAT4x4:
		return "Matrix 4x4";
	case tgfx_datatype_VAR_UBYTE8:
		return "Unsigned Byte 8-bit";
	case tgfx_datatype_VAR_BYTE8:
		return "Signed Byte 8-bit";
	default:
		return "Error, Uniform_Var_Type isn't supported by Find_UNIFORM_VARTYPE_Name!\n";
	}
}
GFXAPI const char* tgfx_GetNameOf_TextureWRAPPING(tgfx_texture_wrapping WRAPPING) {
	switch (WRAPPING) {
	case tgfx_texture_wrapping_REPEAT:
		return "Repeat";
	case tgfx_texture_wrapping_MIRRORED_REPEAT:
		return "Mirrored Repeat";
	case tgfx_texture_wrapping_CLAMP_TO_EDGE:
		return "Clamp to Edge";
	default:
		assert(0 && "GetNameOf_TextureWRAPPING doesn't support this wrapping type!");
		return "ERROR";
	}
}
GFXAPI unsigned char tgfx_GetByteSizeOf_TextureChannels(tgfx_texture_channels channeltype) {
	switch (channeltype)
	{
	case tgfx_texture_channels_R8B:
	case tgfx_texture_channels_R8UB:
		return 1;
	case tgfx_texture_channels_RGB8B:
	case tgfx_texture_channels_RGB8UB:
		return 3;
	case tgfx_texture_channels_D24S8:
	case tgfx_texture_channels_D32:
	case tgfx_texture_channels_RGBA8B:
	case tgfx_texture_channels_RGBA8UB:
	case tgfx_texture_channels_BGRA8UB:
	case tgfx_texture_channels_BGRA8UNORM:
	case tgfx_texture_channels_R32F:
	case tgfx_texture_channels_R32I:
	case tgfx_texture_channels_R32UI:
		return 4;
	case tgfx_texture_channels_RA32F:
	case tgfx_texture_channels_RA32I:
	case tgfx_texture_channels_RA32UI:
		return 8;
	case tgfx_texture_channels_RGB32F:
	case tgfx_texture_channels_RGB32I:
	case tgfx_texture_channels_RGB32UI:
		return 12;
	case tgfx_texture_channels_RGBA32F:
	case tgfx_texture_channels_RGBA32I:
	case tgfx_texture_channels_RGBA32UI:
		return 16;
	default:
		assert(0 && "GetSizeOf_TextureChannels() doesn't support this type!");
		break;
	}
}
