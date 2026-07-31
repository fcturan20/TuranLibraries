/* There are 5 different branches for Renderer:
1) RG Description: Describes render passes then checks if RG is valid.
	If valid, creates Vulkan objects for the passes (RenderPass, Framebuffer etc.)
	Implemented in RG_Description.cpp
2) RG Static Linking: Creates optimized data structures for dynamic framegraph optimizations.
	Implemented in RG_StaticLinkage.cpp
3) RG Commander: Implements render commands API (Draws, Dispatches, Transfer calls etc.)
	Implemented in RG_Commander.cpp
4) RG Dynamic Linking: Optimizes RG for minimal queue and sync operations and submits recorded CBs.
	Implemented in RG_DynamicLinkage.cpp
5) RG CommandBuffer Recorder: Records command buffers according to the commands from
RG_DynamicLinkage.cpp Implemented in RG_CBRecording.cpp
//For the sake of simplicity; RG_CBRecording, RG_StaticLinkage and RG_DynamicLinkage is implemented
in RG_primitive.cpp for now
//But all of these branches are communicating through RG.h header, so don't include the header in
other systems
*/

#pragma once
#include <TGfxDeclarations.h>
#include <TGfxStructs.h>

#include <atomic>
#include <vector>

#include "vk_includes.h"
#include "vk_core.h"

namespace TGFX
{
namespace Vulkan
{

VkConstU4 kMaxBundleCountPerCall = 1024;
struct Submission
{
};

VkConstU4 VKCONST_MAXUNSENTSUBMITCOUNT = 32;
struct Queue : public VkObjectBase<Queue, TGfxQueue, VkObjTypes::GPUQUEUE>, GpuObject
{
	Queue(GPU* gpu) : GpuObject(gpu), CallSynchronizer(gpu->ReferenceManager), queue(gpu->ReferenceManager) {}
	uint16_t GetExtraFlags() { return (GpuIdx << 8) | QueueFamIdx; }
	static GPU* getGPUfromHnd(TGfxQueue hnd)
	{
		auto handle = *(TCHandleLayout<VkObjTypes>*)&hnd;
		uint32_t gpuIdx = handle.ExtraFlags >> 8;
		return GetGpuFromIndex(gpuIdx);
	}
	static TU4 getFAMfromHnd(TGfxQueue hnd)
	{
		auto handle = *(TCHandleLayout<VkObjTypes>*)&hnd;
		return handle.ExtraFlags & (255 << 8);
	}

	uint32_t QueueFamIdx = 0, QueueIdx = 0;
	VkQueueHnd queue;

	// This is a binary semaphore to sync sequential executeCmdBufferList calls in the same queue
	// (DX12 way)
	VkSemaphoreHnd CallSynchronizer;
	Submission* m_unsentSubmits[VKCONST_MAXUNSENTSUBMITCOUNT];
	TCore::Vector<Submission> m_submissions;
	uint32_t sizeUnsetSubmits();

	// Check previously sent submissions
	void checkSubmissions();
	void createSubmission(VkFence fence, void* data, submissionCallback callback);
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(Queue, Vk)

struct CommandBuffer : public VkObjectBase<CommandBuffer, TGfxCommandBuffer, VkObjTypes::CMDBUFFER>, GpuObject
{
	CommandBuffer(GPU* gpu) : GpuObject(gpu), Buffer(gpu->ReferenceManager), Pool(gpu->ReferenceManager) {}
	uint16_t GetExtraFlags() { return uint16_t(GpuIdx) << 8; }
	static GPU* getGPUfromHnd(TGfxCommandBuffer hnd)
	{
		auto handle = *(TCHandleLayout<VkObjTypes>*)&hnd;
		uint32_t gpuIdx = handle.ExtraFlags >> 8;
		return GetGpuFromIndex(gpuIdx);
	}
	TU4 QueueFamIdx;
	VkCommandBufferHnd Buffer;
	VkCommandPoolHnd Pool;

	TCore::Vector<VkCommandBufferHnd> ReferencedSecondaryCommandBuffers;
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(CommandBuffer, Vk)

struct CommandPool : GpuObject
{
	CommandPool(GPU* gpu, bool permanent, TU4 queueFamIdx)
		: GpuObject(gpu), Pool(gpu->ReferenceManager), IsPermanent(permanent), QueueFamilyIdx(queueFamIdx)
	{
		VkCommandPoolCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		ci.queueFamilyIndex = QueueFamilyIdx;
		if (IsPermanent)
			ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		else
			ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		VkCommandPool p{};
		TCORE_SOFT_CHECK(vkCreateCommandPool(gpu->vk_logical, &ci, nullptr, &p), "Failed to create vkCommandPool");
		Pool.Set(p);
	}
	VkCommandPoolHnd Pool;
	TU4 QueueFamilyIdx = 0;
	bool GetThreadOwnership()
	{
		auto currentThread = std::hash<std::thread::id>()(std::this_thread::get_id());
		// We expect prevThread to be no one
		size_t prevThread = 0;
		// If it is, set owner thread
		if (OwnerThreadId.compare_exchange_weak(prevThread, currentThread))
			return true;
		// If not, check if owner is already this thread
		if (prevThread == currentThread)
			return true;
		return false;
	}
	bool LeaveThreadOwnership()
	{
		auto currentThread = std::hash<std::thread::id>()(std::this_thread::get_id());
		// We expect prevThread to be current thread
		size_t prevThread = currentThread;
		// If it is, set owner thread to no one
		if (OwnerThreadId.compare_exchange_weak(prevThread, 0))
			return true;
		// It was either no one's or some other thread's, so this is an issue
		return false;
	}
	VkCommandBufferHnd AllocateCommandBuffer()
	{
		if (!GetThreadOwnership())
			return VkCommandBufferHnd();

		VkCommandBufferAllocateInfo ai{};
		ai.commandBufferCount = 1;
		ai.commandPool = Pool;
		ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

		if (IsPermanent)
			ai.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		else
		{
			PrimaryCommandBufferCounter++;
			ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		}
		VkCommandBuffer c;
		TCORE_SOFT_CHECK(vkAllocateCommandBuffers(GetGpu()->vk_logical, &ai, &c), "Failed to allocate vkCommandBuffer");
		VkCommandBufferHnd cb(GetGpu()->ReferenceManager);
		cb.Set(c);
		return cb;
	}
	void FreeCommandBuffer(CommandBuffer cb)
	{
		if (IsPermanent)
		{
			VkCommandBuffer c = cb.Buffer;
			vkFreeCommandBuffers(GetGpu()->vk_logical, Pool, 1, &c);
		}
		else
		{
			PrimaryCommandBufferCounter--;
			if (!PrimaryCommandBufferCounter)
			{
				vkDestroyCommandPool(GetGpu()->vk_logical, Pool, nullptr);
				Pool.SetAsDead();
				Pool.Set(nullptr);
			}
		}
	}

private:
	// IsPermanent = true -> Secondary Command Buffers (Command Bundles), false -> Primary Command Buffers
	// Secondary CBs should be freed individually, Primary CB are one time only submits
	bool IsPermanent = false;

	// When a thread uses a command buffer from a pool, locks this
	// Ideally; each thread should already own a pool, so there is no need for that
	// But it is here to ensure safety for now.
	// TODO: Either remove this completely or just add for unit tests
	std::atomic_uint64_t OwnerThreadId;
	// Used only for Primary Command Pools
	// Primary command buffers are not freed seperately
	// Instead they all destroyed at once by destroying their command pool
	TU8 PrimaryCommandBufferCounter = 0;
};

typedef TU4 VkCmdTypeEnumType;
enum class CommandType : VkCmdTypeEnumType
{
	error = VK_PRIM_MIN(VkCmdTypeEnumType), // cmdType neither should be 0 nor 255
	BindBindingTables,
	BindVertexBuffers,
	BindIndexBuffer,
	SetDepthBounds,
	SetViewport,
	SetScissor,
	DrawNonIndexedIndirect,
	DrawIndexedDirect,
	ExecuteIndirect,
	BarrierTexture,
	BarrierBuffer,
	BindPipeline,
	Dispatch,
	CopyBufferToTexture,
	CopyBufferToBuffer,
	PushConstant,
	error_2 = VK_PRIM_MAX(VkCmdTypeEnumType)
};

// Secondary command buffers
// These are to use across different command buffers and frames
struct CommandBundle : public VkObjectBase<CommandBundle, TGfxCommandBundle, VkObjTypes::CMDBUNDLE>, public GpuObject
{
	CommandBundle(GPU* gpu)
		: GpuObject(gpu), ActivePipeline(gpu->ReferenceManager), ActivePipelineLayout(gpu->ReferenceManager),
		  ActiveCb(gpu->ReferenceManager)
	{
		for (TU8 i = 0; i < kMaxQueueFamilyCountPerGpu; i++)
			SecondaryCommandBuffers[i].SetManager(gpu->ReferenceManager);
		for (TU8 i = 0; i < kMaxDescSetPerList; i++)
			ActiveDescSets[i].SetManager(gpu->ReferenceManager);
	}
	uint16_t GetExtraFlags() { return 0; }

	VkCommandBufferHnd SecondaryCommandBuffers[kMaxQueueFamilyCountPerGpu];
	VkCommandBufferHnd ActiveCb;
	// Command Buffer States
	VkPipelineHnd ActivePipeline;
	// To check pipeline compatibility
	VkPipelineLayoutHnd ActivePipelineLayout;
	VkDescriptorSetLayoutHnd ActiveDescSets[kMaxDescSetPerList];
	VkPipelineBindPoint BindPoint = {};
	TGfxPipeline m_defaultPipeline = {};

	Command* m_cmds = {};
	uint64_t m_cmdCount = 0;

	void createCmdBuffer(uint64_t cmdCount);
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(CommandBundle, Vk)

template <typename T, CommandType type>
struct Command
{
	template <typename T>
	constexpr bool HasCmdExecute = requires(T t) { t.CmdExecute(std::declval<CommandBundle*>()); };

	static constexpr CommandType Type = type;

	void Execute(CommandBundle* cmdBundle)
	{
		if constexpr (HasCmdExecute<T>)
		{
			((T*)this)->CmdExecute();
		}
	}
};

extern class RendererContext* GRendererContext = nullptr;
class RendererContext
{
public:
	VkGpuObjectArray<CommandBundle, 1 << 24> CommandBundles;
	static void HookRenderer(ITGfxRenderer* renderer);
	static void Initialize();
};

} // namespace Vulkan
} // namespace TGFX