#include "predefinitions_dx.h"

core_tgfx*             core_tgfx_main = nullptr;
core_public*           core_dx        = nullptr;
renderer_public*       renderer       = nullptr;
gpudatamanager_public* contentmanager = nullptr;
imgui_dx*              imgui          = nullptr;
GPU_VKOBJ*             rendergpu      = nullptr;
threadingsys_tapi*     threadingsys   = nullptr;
unsigned int           threadcount    = 1;
semaphoresys_dx*       semaphoresys   = nullptr;
fencesys_dx*           fencesys       = nullptr;
allocatorsys_dx*       gpu_allocator  = nullptr;
queuesys_dx*           queuesys       = nullptr;

tgfx_PrintLogCallback printer = nullptr;

HINSTANCE hInstance           = NULL;
UINT      g_RTVDescriptorSize = UINT32_MAX;