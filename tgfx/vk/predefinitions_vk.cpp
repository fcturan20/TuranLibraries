#include "predefinitions_vk.h"


core_public* core = nullptr;
renderer_public* renderer = nullptr;
gpudatamanager_public* contentmanager = nullptr;
IMGUI_VK* imgui = nullptr;
gpu_vk* rendergpu = nullptr;
threadingsys_tapi* threadingsys = nullptr;
semaphoresys_vk* semaphoresys = nullptr;
fencesys_vk* fencesys = nullptr;

VkInstance Vulkan_Instance = VK_NULL_HANDLE;
VkApplicationInfo Application_Info;


print_log printer = nullptr;