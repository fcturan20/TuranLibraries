#pragma once
#define TCORE_USE_CPP_WRAPPER
#include "CppGenerics.h"
#include <TGfxCore.h>
#include <Allocator.h>
#include <Logger.h>

#ifndef NDEBUG
#define VK_VALIDATION_LAYER
#endif

#include <volk.h>

namespace TGFX
{
namespace Vulkan
{

// <-------------------------------------------------------------------------------------->
//		GLOBALS
// <-------------------------------------------------------------------------------------->

extern VkInstance GVkInstance;
extern VkApplicationInfo GVkAppInfo;

// <------------------------------------------------------------------------------------>
//		CONSTANTS
// <------------------------------------------------------------------------------------>

#define VkConstU4 static constexpr uint32_t
#define VkConstF4 static constexpr float
#define VkConstU1 static constexpr uint8_t
#define VkConstStr static constexpr const char*
// User can pass lots of descriptor sets in a list, this defines the max size
VkConstU4 kMaxDescSetPerList = 12;
VkConstU4 kMaxViewportCount = 16;
VkConstU4 kMaxGpuCount = 4;
VkConstU4 kMaxRtSlotCount = 16;
VkConstU4 kMaxQueueFamilyCountPerGpu = 5;
VkConstU4 kMaxSemaphoreCountPerSubmit = 16;
VkConstU4 kMaxSwapchainCountPerSubmit = 8; // Max count of swapchain count per submit
VkConstU1 kMaxSwapchainTextureCountPerSwapchain = 4;
VkConstStr kRequiredDeviceExtensionNames[] = {VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
											  VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
											  VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
											  VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
											  VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME};

// <-------------------------------------------------------------------------------------->
//      Object system
// <-------------------------------------------------------------------------------------->

// There can be only 65536 types of handles that users can access (which is enough)
enum class VkObjTypes : unsigned short
{
	UNDEFINED = 0,
	SAMPLER,
	BINDINGTABLEINST, // Returned as binding table
	VERTEXATTRIB,
	SHADERSOURCE,
	BUFFER,
	RTSLOTSET,
	IRTSLOTSET,
	TEXTURE,
	GPU,
	VIEWPORT,
	HEAP,
	GPUQUEUE,
	FENCE,
	CMDBUFFER,
	CMDBUNDLE,
	PIPELINE,
	SUBRASTERPASS,
	SWAPCHAIN
};

TCORE_DEFINE_OPAQUE_HANDLE_SYSTEM(Vk, VkObjTypes)

uint32_t GetMemOffset(void* object) {}

void* GetPointer(uint32_t memOffset) {}

TCResult vkPrint(TU4 returnCode, const char* extraDetails = nullptr)
{
	const char* message{};
	auto state = TGfx->GetResultStateByReturnCode(returnCode, &message);
	if (extraDetails)
		TCore::tl(TC_LOG_LEVEL_STATUS) << message << TCore::td() << extraDetails;
	else
		TCore::tl(TC_LOG_LEVEL_STATUS) << message;
}

void Append_pNext(void* targetStruct, void* attachStruct);

#define VK_PRIM_MIN(primType) std::numeric_limits<primType>::min()
#define VK_PRIM_MAX(primType) std::numeric_limits<primType>::max()

enum class VkOpaqueHandleType : TU4
{
	VkSurfaceKHR_HND,
	VkSwapchainKHR_HND,
	VkSemaphore_HND,
	VkImage_HND,
	VkBuffer_HND,
	VkImageView_HND,
	VkCommandPool_HND,
	VkCommandBuffer_HND,
	VkDevice_HND,
	VkPhysicalDevice_HND,
	VkRenderPass_HND,
	VkSampler_HND,
	VkPipeline_HND,
	VkPipelineLayout_HND,
	VkShaderModule_HND,
	VkDeviceMemory_HND,
	VkFramebuffer_HND,
	VkDescriptorSetLayout_HND,
	VkDescriptorSet_HND,
	VkDescriptorPool_HND,
	VkFence_HND
};

class VkReferenceManager
{
public:
	struct ReferenceCountHandle
	{
		TU8 Id = 0;
		TU8 ReferenceCount = 0;
		void* Handle;
		VkOpaqueHandleType Type;
		TBool IsAlive = TTRUE;
	};
	// TCore::UnorderedMap<void*, ReferenceCountHandle> HandleList;
	void AddRef(void* opaqueHandle, VkOpaqueHandleType type)
	{
		if (auto obj = GetObj())
		{
			TCORE_SOFT_CHECK(
				obj.IsAlive == TTRUE,
				"A dead object referenced again. Either a different object uses same handle before all references are "
				"removed (VkAllocator problem) or vulkan backend references a dead object.");
			return;
		}
		// Add to list
	}
	void RemoveRef(void* opaqueHandle) {}
	void SetAsDead(void* opaqueHandle)
	{
		auto obj = GetObj();
		TCORE_SOFT_CHECK(obj.ReferenceCount <= 1, "This vulkan object still has references somewhere!");
		obj.IsAlive = TFALSE;
	}
};

// Reference counted
template <typename T, VkOpaqueHandleType typeEnum>
class VkHandle
{
public:
	VkHandle(VkReferenceManager& refManager) : ReferenceManager(refManager) {}
	VkHandle(VkReferenceManager& refManager, T handle) : ReferenceManager(refManager) { Set(handle); }
	VkHandle(const VkHandle<T, typeEnum>& r) : ReferenceManager(r.ReferenceManager)
	{
		Handle = r.Handle;
		ReferenceManager.AddRef(Handle, typeEnum);
	}
	void* Handle;
	VkReferenceManager& ReferenceManager;
	~VkHandle()
	{
		TCORE_SOFT_CHECK(Handle);
		ReferenceManager.RemoveRef(Handle);
	}
	void Set(T hnd)
	{
		TCORE_SOFT_CHECK(!Handle);
		Handle = hnd;
		ReferenceManager.AddRef(Handle, typeEnum);
	}
	operator T() const { return (T)Handle; }
	VkHandle<T, typeEnum>& operator=(const VkHandle<T, typeEnum>& rhs)
	{
		if (Handle)
			ReferenceManager.RemoveRef(Handle);
		ReferenceManager = rhs.ReferenceManager;
		Handle = rhs.Handle;
		ReferenceManager.AddRef(Handle, typeEnum);
	}
	void SetAsDead() { ReferenceManager.SetAsDead(Handle); }
};

#define VK_TYPE_TO_HND_SPECIALIZATION(type) using type##Hnd = VkHandle<##type, VkOpaqueHandleType::##type##_HND>
VK_TYPE_TO_HND_SPECIALIZATION(VkSurfaceKHR);
VK_TYPE_TO_HND_SPECIALIZATION(VkSwapchainKHR);
VK_TYPE_TO_HND_SPECIALIZATION(VkSemaphore);
VK_TYPE_TO_HND_SPECIALIZATION(VkImage);
VK_TYPE_TO_HND_SPECIALIZATION(VkBuffer);
VK_TYPE_TO_HND_SPECIALIZATION(VkImageView);
VK_TYPE_TO_HND_SPECIALIZATION(VkCommandPool);
VK_TYPE_TO_HND_SPECIALIZATION(VkCommandBuffer);
VK_TYPE_TO_HND_SPECIALIZATION(VkDevice);
VK_TYPE_TO_HND_SPECIALIZATION(VkPhysicalDevice);
VK_TYPE_TO_HND_SPECIALIZATION(VkRenderPass);
VK_TYPE_TO_HND_SPECIALIZATION(VkSampler);
VK_TYPE_TO_HND_SPECIALIZATION(VkPipeline);
VK_TYPE_TO_HND_SPECIALIZATION(VkPipelineLayout);
VK_TYPE_TO_HND_SPECIALIZATION(VkShaderModule);
VK_TYPE_TO_HND_SPECIALIZATION(VkDeviceMemory);
VK_TYPE_TO_HND_SPECIALIZATION(VkFramebuffer);
VK_TYPE_TO_HND_SPECIALIZATION(VkDescriptorSet);
VK_TYPE_TO_HND_SPECIALIZATION(VkDescriptorSetLayout);
VK_TYPE_TO_HND_SPECIALIZATION(VkDescriptorPool);
VK_TYPE_TO_HND_SPECIALIZATION(VkFence);

#if defined(T_ENVWINDOWS)
static constexpr VkExternalFenceHandleTypeFlags kSharedFenceHandleType = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
static constexpr VkExternalFenceHandleTypeFlags kSharedFenceHandleType = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

} // namespace Vulkan
} // namespace TGFX