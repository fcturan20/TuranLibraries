#pragma once
//Vulkan Libs
#include <vulkan\vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <tgfx_forwarddeclarations.h>


//Systems 

struct core_tgfx;
extern core_tgfx* core_tgfx_main;
struct core_public;
extern core_public* core_vk;
struct renderer_public;
extern renderer_public* renderer;
struct gpudatamanager_public;
extern gpudatamanager_public* contentmanager;
struct imgui_vk;
extern imgui_vk* imgui;
struct gpu_public;
extern gpu_public* rendergpu;
typedef struct threadingsys_tapi threadingsys_tapi;
extern threadingsys_tapi* threadingsys;
extern unsigned int threadcount;
struct allocatorsys_vk;
extern allocatorsys_vk* allocatorsys;
struct queuesys_vk;
extern queuesys_vk* queuesys;

extern tgfx_PrintLogCallback printer;


//Synchronization systems and objects

struct semaphoresys_vk;
extern semaphoresys_vk* semaphoresys;
struct fencesys_vk;
extern fencesys_vk* fencesys;
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


//Structs

struct texture_vk;
struct memorytype_vk;
struct queuefam_vk;
struct extension_manager;	//Stores activated extensions and set function pointers according to that
struct pass_vk;
struct drawpass_vk;
struct transferpass_vk;
struct windowpass_vk;
struct computepass_vk;
struct subdrawpass_vk;
struct indexeddrawcall_vk;
struct nonindexeddrawcall_vk;
struct irtslotset_vk;
struct rtslotset_vk;
struct window_vk;

//Enums
enum class desctype_vk : unsigned char {
	SAMPLER = 0,
	IMAGE = 1,
	SBUFFER = 2,
	UBUFFER = 3
};

//Resources
struct texture_vk;


extern VkInstance Vulkan_Instance;
extern VkApplicationInfo Application_Info;