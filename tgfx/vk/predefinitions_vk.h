#pragma once
//Vulkan Libs
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
extern "C" {
	#include <tgfx_forwarddeclarations.h>
}

class core_public;
extern core_public* core;
class renderer_public;
extern renderer_public* renderer;
class gpudatamanager_public;
extern gpudatamanager_public* contentmanager;
class IMGUI_VK;
extern IMGUI_VK* imgui;
struct gpu_vk;
extern gpu_vk* rendergpu;
typedef struct threadingsys_tapi threadingsys_tapi;
extern threadingsys_tapi* threadingsys;
class semaphoresys_vk;
extern semaphoresys_vk* semaphoresys;
class fencesys_vk;
extern fencesys_vk* fencesys;

typedef void (*print_log)(result_tgfx result, const char* text);
extern print_log printer;

extern VkInstance Vulkan_Instance;
extern VkApplicationInfo Application_Info;