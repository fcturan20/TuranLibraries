#undef USE_TGFXNAMESPACE
#include <TGFX/GFX_Core.h>
#include "Main.h"

#if _WIN32 || _WIN64
#define VK_DLLEXPORTER __declspec(dllexport)

#elif defined(__linux__)
#define VK_DLLEXPORTER __attribute__((visibility("default")))
#endif

#include <stdlib.h>
#include <stdio.h>


extern "C" {
	VK_DLLEXPORTER tgfx_core* Load_TGFXBackend() {
		printf("Hi from Load_TGFXBackend()!");
		return (tgfx_core*)StartBackend_returnTGFXCore();
	}
}