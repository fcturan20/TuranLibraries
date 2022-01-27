#include "predefinitions_vk.h"
#include "resource.h"

core_tgfx* core_tgfx_main = nullptr;
core_public* core_vk = nullptr;
renderer_public* renderer = nullptr;
gpudatamanager_public* contentmanager = nullptr;
imgui_vk* imgui = nullptr;
gpu_public* rendergpu = nullptr;
threadingsys_tapi* threadingsys = nullptr;
unsigned int threadcount = 1;
semaphoresys_vk* semaphoresys = nullptr;
fencesys_vk* fencesys = nullptr;
allocatorsys_vk* allocatorsys = nullptr;
queuesys_vk* queuesys = nullptr;

VkInstance Vulkan_Instance = VK_NULL_HANDLE;
VkApplicationInfo Application_Info;

tgfx_PrintLogCallback printer = nullptr;

