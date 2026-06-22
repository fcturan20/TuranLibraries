#pragma once
#include <TCore.h>

TCORE_BEGIN_C_LINKAGE

// Functions a TGFX backend plugin must implement. This is used for TGFX to call the backend functions without
typedef struct TGfxBackendFunctions {
    TGfxGpuHandle (*CreateGpu)(const char* backendName);
    
} TGfxBackendFunctions;

typedef struct TGfxBackendManager {
    TCResult (*RegisterBackend)(const TGfxBackendFunctions* backendFunctions);
} ITGfxBackend;

TCORE_END_C_LINKAGE