#include "predefinitions_dx.h"

core_tgfx* gCoreTgfxMain = nullptr;
core_public* gCoreDx = nullptr;
renderer_public* gRenderer = nullptr;
gpudatamanager_public* gContentManager = nullptr;
imgui_dx* gImgui = nullptr;
GPU_VKOBJ* gRenderGpu = nullptr;
threadingsys_tapi* gThreadingSystem = nullptr;
unsigned int gThreadCount = 1;
semaphoresys_dx* gSemaphoreSystem = nullptr;
fencesys_dx* gFenceSystem = nullptr;
allocatorsys_dx* gGpuAllocator = nullptr;
queuesys_dx* gQueueSystem = nullptr;

tgfx_PrintLogCallback printer = nullptr;

HINSTANCE gHInstance = NULL;
UINT gRtvDescriptorSize = UINT32_MAX;