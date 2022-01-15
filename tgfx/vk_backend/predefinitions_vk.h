#pragma once
//Vulkan Libs
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <tgfx_forwarddeclarations.h>

struct core_tgfx;
extern core_tgfx* core_tgfx_main;
struct core_public;
extern core_public* core_vk;
struct renderer_public;
extern renderer_public* renderer;
struct gpudatamanager_public;
extern gpudatamanager_public* contentmanager;
struct IMGUI_VK;
extern IMGUI_VK* imgui;
struct gpu_public;
extern gpu_public* rendergpu;
typedef struct threadingsys_tapi threadingsys_tapi;
extern threadingsys_tapi* threadingsys;


struct semaphoresys_vk;
extern semaphoresys_vk* semaphoresys;
struct fencesys_vk;
extern fencesys_vk* fencesys;
struct texture_vk;
#ifdef VULKAN_DEBUGGING
typedef unsigned int semaphore_idtype_vk;
static constexpr semaphore_idtype_vk invalid_semaphoreid = UINT32_MAX;
typedef unsigned int fence_idtype_vk;
static constexpr fence_idtype_vk invalid_fenceid = UINT32_MAX;
#else
struct semaphore_vk;
typedef semaphore_vk* semaphore_idtype_vk;
static constexpr semaphore_idtype_vk invalid_semaphoreid = nullptr;
struct fence_vk;
typedef fence_vk* fence_idtype_vk;
static constexpr fence_idtype_vk invalid_fenceid = nullptr;
#endif


typedef void (*print_log)(result_tgfx result, const char* text);
extern print_log printer;

extern VkInstance Vulkan_Instance;
extern VkApplicationInfo Application_Info;