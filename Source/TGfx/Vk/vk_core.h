#pragma once
#include <atomic>
#include <string>
#include <vector>

#include "TGfxStructs.h"
#include <TGfxCore.h>
#include "vk_predefinitions.h"
#include "vulkan/vulkan.h"

namespace TGFX
{
namespace Vulkan
{
struct GPU : public VkObjectBase<GPU, TGfxGpu, VkObjTypes::GPU>
{
public:
	GPU(uint8_t idx);
	uint16_t GetExtraFlags() { return m_gpuIdx; }

	TGfxGpuInfo desc;

	VkPhysicalDeviceHnd vk_physical;
	VkDeviceHnd vk_logical;
	VkPhysicalDeviceProperties2 vk_propsDev = {};
	VkPhysicalDeviceFeatures2 vk_featuresDev = {};
	VkPhysicalDeviceMemoryProperties2 vk_propsMemory = {};
	TGfxMemoryInfo m_memoryDescTGFX[32] = {};
	TGfxTexture m_invalidStorageTexture = {}, m_invalidShaderReadTexture = {};
	TGfxSampler m_invalidSampler = {};
	TGfxBuffer m_invalidBuffer = {};

	VkQueueFamilyProperties2 vk_propsQueue[kMaxQueueFamilyCountPerGpu] = {};
	TGfxQueue m_internalQueue = {};
	VkReferenceManager ReferenceManager;

private:
	uint8_t m_gpuIdx = 255;

public:
	uint8_t GetGpuIdx() const { return m_gpuIdx; }
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(GPU, Vk)

// Adds simple GPU reference to access owner GPU
struct GpuObject
{
protected:
	uint8_t GpuIdx;
	GpuObject(GPU* gpu) { GpuIdx = gpu->GetGpuIdx(); }

public:
	GPU* GetGpu();
};

struct Swapchain : public VkObjectBase<Swapchain, TGfxSwapchain, VkObjTypes::SWAPCHAIN>, GpuObject
{
	Swapchain(GPU* gpu)
		: GpuObject(gpu), Surface(gpu->ReferenceManager), Swpchn(gpu->ReferenceManager),
		  AcquireSemaphore(gpu->ReferenceManager)
	{
	}
	uint16_t GetExtraFlags() { return 0; }

	TGfxUVec2 m_lastSize;
	TGfxTexture m_swapchainTextures[kMaxSwapchainTextureCountPerSwapchain] = {};
	TU1 m_swapchainCurrentTextureIdx = 0;
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

	VkSurfaceKHRHnd Surface;
	VkSwapchainKHRHnd Swpchn;
	VkSemaphoreHnd AcquireSemaphore;
	TGfxGpuSwapchainSupportInfo Info{};
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(Swapchain, Vk)

template <typename T, TU8 MaxCapacity>
class VkGpuObjectArray : public TCore::Vector<T, true>
{
public:
	VkGpuObjectArray() : TCore::Vector<T, true>(TCore::GSuperMemoryBlock, 0, MaxCapacity) {}

	T* CreateObject(GPU* gpu)
	{
		auto& obj = this->EmplaceBack(gpu);
		return &obj;
	}

	void DestroyObject(T* obj) { obj->~T(); }
};

extern class VkContext* GContext = nullptr;
class VkContext
{
public:
	// These are VK_VECTORs, instead of VK_LINEAROBJARRAYs, because they won't change at run-time so
	// frequently
	VK_ARRAY<GPU, TGfxGpu> GPUs;
	TCore::PairedList<void*, VkSurfaceKHR> WindowToVkSurfaceMap;

	VkConstU4 VKGLOBAL_MAX_INSTANCE_EXT_COUNT = 256;
	VkExtensionProperties SupportedInstanceExtensions[VKGLOBAL_MAX_INSTANCE_EXT_COUNT] = {};

	TCResult Initialize();

	VkContext() { GContext = this; }

	~VkContext() { GContext = nullptr; }

	VkSurfaceKHR FindOrCreateSurface(void* windowOsHnd);

	// Functions for TGFXX API

	static TCResult CreateSwapchain(const TGfxSwapchainDescription* desc, TGfxSwapchain* swapchain);
	static TCResult GetCurrentSwapchainTextureIndex(TGfxSwapchain swapchain, TU4* index);
	static TCResult ChangeSwapchainResolution(TGfxSwapchain swapchain, TGfxUVec2 newSize);
	static TCResult QuerySwapchainSupportOfGpu(TGfxGpu gpu, void* windowOsHnd, TGfxGpuSwapchainSupportInfo* oInfo);
	static void DestroySwapchain(TGfxSwapchain swapchain) {}
	static TCResult Hook(ITGfx* gfx);

	VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;
	// VkDevice, VkSurfaceKHR
	VkReferenceManager InstanceObjectReferences;
};

GPU::GPU(uint8_t idx)
	: vk_physical(GContext->InstanceObjectReferences), vk_logical(GContext->InstanceObjectReferences), m_gpuIdx(idx)
{
}

// TODO: Remove this and use GpuObject's GetGpu
inline GPU* GetGpuFromIndex(uint8_t idx)
{
	TCORE_SOFT_CHECK(GContext->GPUs.size() > idx);
	return &GContext->GPUs[idx];
}

GPU* GpuObject::GetGpu()
{
	return GetGpuFromIndex(GpuIdx);
}

} // namespace Vulkan
} // namespace TGFX