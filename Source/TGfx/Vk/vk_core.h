#pragma once
#include <atomic>
#include <string>
#include <vector>

#include "TGfxStructs.h"
#include <TGfxCore.h>
#include "vk_predefinitions.h"
#include "vulkan/vulkan.h"

struct GPU_VKOBJ
{
public:
	bool isALIVE = 1;
	VkConstHndType HANDLETYPE = VKHANDLETYPEs::GPU;
	typedef TGfxGpu TGFXHNDTYPE;
	static uint16_t GET_EXTRAFLAGS(GPU_VKOBJ* obj) { return obj->m_gpuIndx; }

	TGfxGpuInfo desc;

	VkPhysicalDevice vk_physical = {};
	VkDevice vk_logical = {};
	VkPhysicalDeviceProperties2 vk_propsDev = {};
	VkPhysicalDeviceFeatures2 vk_featuresDev = {};
	VkPhysicalDeviceMemoryProperties2 vk_propsMemory = {};
	TGfxMemoryInfo m_memoryDescTGFX[32] = {};
	struct tgfx_texture *m_invalidStorageTexture = {}, *m_invalidShaderReadTexture = {};
	struct tgfx_sampler* m_invalidSampler = {};
	TGfxBuffer m_invalidBuffer = {};

	VkQueueFamilyProperties2 vk_propsQueue[kMaxQueueFamilyCountPerGpu] = {};
	uint32_t m_queueFamPtrs[kMaxQueueFamilyCountPerGpu] = {};
	TGfxQueue m_internalQueue = {};

private:
	extManager_vkDevice* m_extensions;
	uint8_t m_gpuIndx = 255;

public:
	const extManager_vkDevice* ext() const { return m_extensions; }
	extManager_vkDevice*& ext() { return m_extensions; }
	uint8_t gpuIndx() const { return m_gpuIndx; }
	void setGPUINDX(uint8_t v) { m_gpuIndx = v; }
};

struct VkSwapchainObj
{
	bool isALIVE = false;
	VkConstHndType HANDLETYPE = VKHANDLETYPEs::SWAPCHAIN;
	static uint16_t GET_EXTRAFLAGS(VkSwapchainObj* obj) { return 0; }

	VkSwapchainObj() = default;
	TGfxUVec2 m_lastSize;
	GPU_VKOBJ* m_gpu = nullptr;
	TGfxTexture m_swapchainTextures[kMaxSwapchainTextureCountPerSwapchain] = {};
	unsigned char m_swapchainCurrentTextureIndx = 0;
	// Presentation Fences should only be used for CPU to wait
	TGfxFence m_presentationFences[kMaxSwapchainTextureCountPerSwapchain];
	struct PresentationModes
	{
		unsigned char immediate : 1;
		unsigned char mailbox : 1;
		unsigned char fifo : 1;
		unsigned char fifo_relaxed : 1;
		PresentationModes() : immediate(0), mailbox(0), fifo(0), fifo_relaxed(0) {}
	};
	PresentationModes m_presentModes;

	VkSurfaceKHR Surface = {};
	VkSwapchainKHR Swapchain = {};
	VkCommandBuffer vk_generalToPresent[kMaxQueueFamilyCountPerGpu][kMaxSwapchainTextureCountPerSwapchain] = {},
					vk_presentToGeneral[kMaxQueueFamilyCountPerGpu][kMaxSwapchainTextureCountPerSwapchain] = {};
	VkImageUsageFlags TextureUsage = {};
	VkSemaphore AcquireSemaphore = {};
};