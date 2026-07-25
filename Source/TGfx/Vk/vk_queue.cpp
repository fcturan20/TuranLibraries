/*
  This file is responsible for managing TGFX Command Buffer sending queue operations to GPU and.
  TGFX Command Buffer = Primary Command Buffer.
  TGFX GPU QUEUE = VkQueue

*/

#include "vk_queue.h"

#include <mutex>

#include "TGfxRenderer.h"
#include "vk_contentmanager.h"
#include "vk_core.h"
#include "vk_renderer.h"

namespace TGFX
{
namespace Vulkan
{
manager_vk* manager = nullptr;
VkConstU4 VKCONST_MAXFENCECOUNT_PERSUBMIT = 8, VKCONST_MAXCMDBUFFER_PRIMARY_COUNT = 32;

struct submission_vk
{
	VkFence fence = nullptr;
	GPU* m_gpu = nullptr;
	void* m_userData = nullptr;
	vk_submissionCallback m_callback = nullptr;
	uint32_t queueIndx = UINT32_MAX;
};

enum queueOpType : uint8_t
{
	ERROR_QUEUEOPTYPE = 0,
	CMDBUFFER = 1,
	PRESENT = 2,
	SPARSE = 3
};
struct submit_vk
{
	queueOpType type = CMDBUFFER;
	union {
		VkSubmitInfo submit;
		VkPresentInfoKHR present;
	};

	uint32_t signalSemaphoreCount = 0, waitSemaphoreCount = 0, cmdBufferCount = 0, windowCount = 0;
	TGfxCommandBuffer* cmdBuffers = {};

	VkSemaphore *signalSemaphores = {}, *waitSemaphores = {};

	uint64_t *signalSemaphoreValues = {}, *waitSemaphoreValues = {};
	VkTimelineSemaphoreSubmitInfo semaphoreInfo;

	Swapchain** m_windows = {};
	// Allocates enough memory and sets pointers to valid arrays (waiting to be filled)
	static submit_vk* allocateSubmit(uint64_t i_signalSemaphoreCount,
									 uint64_t i_waitSemaphoreCount,
									 uint64_t i_cmdBufferCount,
									 uint32_t i_windowCount)
	{
		uint32_t allocSize =
			sizeof(submit_vk) +
			(sizeof(void*) * (i_signalSemaphoreCount + i_waitSemaphoreCount + i_cmdBufferCount + i_windowCount + 4)) +
			(sizeof(uint64_t) * (i_signalSemaphoreCount + i_waitSemaphoreCount));
		submit_vk* submit = (submit_vk*)VK_MEMOFFSET_TO_POINTER(virmem::allocatePage(allocSize));
		vm->commit(submit, allocSize);
		submit->signalSemaphoreCount = i_signalSemaphoreCount;
		submit->waitSemaphoreCount = i_waitSemaphoreCount;
		submit->cmdBufferCount = i_cmdBufferCount;
		submit->windowCount = i_windowCount;

		uintptr_t lastPos = uintptr_t(submit + 1);
		// Allocate cmd buffer object list
		{
			submit->cmdBuffers = (TGfxCommandBuffer*)lastPos;
			lastPos += sizeof(CommandBuffer**) * (submit->cmdBufferCount + 1ull);
		}
		// Allocate signal semaphore list
		{
			submit->signalSemaphores = (VkSemaphore*)lastPos;
			lastPos += sizeof(VkSemaphore) * (submit->signalSemaphoreCount + 1ull);
			submit->signalSemaphoreValues = (uint64_t*)lastPos;
			lastPos += sizeof(uint64_t) * (submit->signalSemaphoreCount + 1ull);
		}
		// Allocate wait semaphore list
		{
			submit->waitSemaphores = (VkSemaphore*)lastPos;
			lastPos += sizeof(VkSemaphore) * (submit->waitSemaphoreCount + 1ull);
			submit->waitSemaphoreValues = (uint64_t*)lastPos;
			lastPos += sizeof(uint64_t) * (submit->waitSemaphoreCount + 1ull);
		}
		// Allocate window list
		{
			submit->m_windows = (Swapchain**)lastPos;
			lastPos += sizeof(Swapchain*) * (submit->windowCount + 1ull);
		}
		return submit;
	}
};
VkConstU4 VKCONST_MAXUNSENTSUBMITCOUNT = 32;
struct Queue : public VkObjectBase<Queue, TGfxQueue, VkObjTypes::GPUQUEUE>
{
	uint16_t GetExtraFlags() { return (m_gpu->GetGpuIndx() << 8) | queueFamIndex; }
	static GPU* getGPUfromHnd(TGfxQueue hnd);
	static QUEUEFAM_VK* getFAMfromHnd(TGfxQueue hnd);

	uint32_t queueFamIndex = 0, m_queueIndx = 0;
	VkQueue queue;

	queueOpType m_activeQueueOp = ERROR_QUEUEOPTYPE, m_prevQueueOp = ERROR_QUEUEOPTYPE;
	GPU* m_gpu = nullptr;
	// This is a binary semaphore to sync sequential executeCmdBufferList calls in the same queue
	// (DX12 way)
	VkSemaphore callSynchronizer = {};
	submit_vk* m_unsentSubmits[VKCONST_MAXUNSENTSUBMITCOUNT];
	VK_LINEAR_OBJARRAY<submission_vk, void*, 1 << 10> m_submissions;
	uint32_t sizeUnsetSubmits();

	// Check previously sent submissions
	void checkSubmissions();
	void createSubmission(VkFence fence, void* data, submissionCallback callback);
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(Queue, Vk)

struct CommandBuffer : public VkObjectBase<CommandBuffer, TGfxCommandBuffer, VkObjTypes::CMDBUFFER>
{
	uint16_t GetExtraFlags(CommandBuffer* obj) { return uint16_t(obj->m_gpuIndx) << 8; }
	static GPU* getGPUfromHnd(TGfxCommandBuffer hnd)
	{
		auto handle = *(TCHandleLayout<VkObjTypes>*)&hnd;
		uint32_t gpuIndx = handle.ExtraFlags >> 8;
	}
	VkCommandBuffer cb = {};
	VkCommandPool cp{};
	uint8_t m_gpuIndx = 0, m_queueFamIndx = 0;
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(CommandBuffer, Vk)

struct QUEUEFAM_VK
{
	uint16_t GetExtraFlags() { return (m_gpu->GetGpuIndx() << 8) | m_supportFlag; }

	queueflag_vk m_supportFlag = {};
	uint32_t queueFamIndex = 0, m_queueListStartIndx = 0;
	GPU* m_gpu = nullptr;
	cmdPool_vk* m_pools = nullptr;
};
queueflag_vk::operator uint8_t() const
{
	if constexpr (sizeof(queueflag_vk) == 1)
		return *((uint8_t*)this);
	else
	{
		vkPrint(16, "Backend: queueflag_vk shouldn't be more than 1 byte");
	}
	return 0;
};
inline void getGPUInfoQueues(TGfxGpu GPUhnd, unsigned int queueFamIndx, unsigned int* queueCount, TGfxQueue* queueList)
{
	GPU* VKGPU = GetVkObject(GPUhnd);
	QUEUEFAM_VK* fam = getQueueFam(VKGPU, queueFamIndx);
	if (*queueCount == VKGPU->propsQueue[queueFamIndx].queueFamilyProperties.queueCount)
	{
		for (uint32_t i = 0; i < *queueCount; i++)
		{
			queueList[i] = GetOpaqueHandle(getQueue(fam, i));
		}
	}
	*queueCount = VKGPU->propsQueue[queueFamIndx].queueFamilyProperties.queueCount;
}
void getWindowSupportedQueues(GPU* GPU, Swapchain* window, TGfxGpuSwapchainSupportInfo* info)
{
	uint32_t supportedQueueCount = 0;
	for (uint32_t queueFamIndx = 0; queueFamIndx < GPU->desc.queueFamilyCount; queueFamIndx++)
	{
		VkBool32 isSupported = false;
		if (vkGetPhysicalDeviceSurfaceSupportKHR(GPU->vk_physical, queueFamIndx, window->Surface, &isSupported) !=
			VK_SUCCESS)
		{
			vkPrint(50, "at vkGetPhysicalDeviceSurfaceSupportKHR");
		}
		QUEUEFAM_VK* queueFam = getQueueFam(GPU, queueFamIndx);
		const uint32_t queueCount = GPU->propsQueue[queueFamIndx].queueFamilyProperties.queueCount;
		if (isSupported)
			for (uint32_t queueIndx = 0;
				 queueIndx < queueCount && supportedQueueCount < TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT;
				 queueIndx++)
				info->queues[supportedQueueCount++] = GetOpaqueHandle(getQueue(queueFam, queueIndx));
	}
	if (supportedQueueCount < TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT)
		info->queues[supportedQueueCount] = nullptr;
}

void createManager()
{
	manager = new manager_vk;

	uint32_t totalQueueFamCount = 0, totalQueueCount = 0;
	for (uint32_t gpuIndx = 0; gpuIndx < core_vk->gpuCount(); gpuIndx++)
	{
		totalQueueFamCount;
		GPU* gpu = GetGpuFromIndex(gpuIndx);
		uint32_t queueFamiliesCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties2(gpu->vk_physical, &queueFamiliesCount, nullptr);
#ifdef VULKAN_DEBUGGING
		if (kMaxQueueFamilyCountPerGpu < queueFamiliesCount)
		{
			vkPrint(16, "VKCONST_MAXQUEUEFAMCOUNT_PERGPU is exceeded, please report this!");
		}
#endif
		for (uint32_t i = 0; i < kMaxQueueFamilyCountPerGpu; i++)
		{
			gpu->propsQueue[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
			gpu->propsQueue[i].pNext = nullptr;
		}
		vkGetPhysicalDeviceQueueFamilyProperties2(gpu->vk_physical, &queueFamiliesCount, gpu->propsQueue);

		gpu->desc.queueFamilyCount = queueFamiliesCount;
		for (uint32_t queueFamIndx = 0; queueFamIndx < kMaxQueueFamilyCountPerGpu; queueFamIndx++)
		{
			totalQueueCount += gpu->propsQueue[queueFamIndx].queueFamilyProperties.queueCount;
			gpu->m_queueFamPtrs[queueFamIndx] = totalQueueFamCount + queueFamIndx;
		}
		totalQueueFamCount += queueFamiliesCount;
	}
	mngrPriv->m_queueFams.init(new (VKGLOBAL_VIRMEM_MANAGER) QUEUEFAM_VK[totalQueueFamCount], totalQueueFamCount);
	mngrPriv->m_queues.init(new (VKGLOBAL_VIRMEM_MANAGER) Queue[totalQueueCount], totalQueueCount);

	totalQueueFamCount = 0, totalQueueCount = 0;
	for (uint32_t gpuIndx = 0; gpuIndx < core_vk->gpuCount(); gpuIndx++)
	{
		GPU* gpu = GetGpuFromIndex(gpuIndx);
		QUEUEFAM_VK* bestQueueFam = nullptr;
		for (uint32_t queueFamIndx = 0; queueFamIndx < gpu->desc.queueFamilyCount; queueFamIndx++)
		{
			VkQueueFamilyProperties2* props = &gpu->propsQueue[queueFamIndx];
			QUEUEFAM_VK* queueFam = mngrPriv->m_queueFams[totalQueueFamCount++];

			queueFam->queueFamIndex = queueFamIndx;
			if (props->queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				gpu->desc.OperationSupport_Raster = true;
				queueFam->m_supportFlag.IsSupportedGraphics = true;
			}
			if (props->queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				gpu->desc.OperationSupport_Compute = true;
				queueFam->m_supportFlag.IsSupportedCompute = true;
			}
			if (props->queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				gpu->desc.OperationSupport_Transfer = true;
				queueFam->m_supportFlag.IsSupportedTransfer = true;
			}
			queueFam->m_gpu = gpu;

			// FIND BEST QUEUE FAM
			// Select internal queue, then create other queue backend objects

			if (!bestQueueFam)
			{
				bestQueueFam = queueFam;
				continue;
			}
			const VkQueueFamilyProperties& bestQueueProps =
				gpu->propsQueue[bestQueueFam->queueFamIndex].queueFamilyProperties;

			if (props->queueFamilyProperties.queueCount > bestQueueProps.queueCount)
			{
				bestQueueFam = queueFam;
				continue;
			}

			// Bigger values in VkQueueFlags means more useful the queue gets
			// For internal operations, we need a queue as useless as possible.
			// For example: 4 is tranfer, 8 is sparse, 16 is protected bits which are supported by all
			//  queues if device supports. Main difference between is generally in Graphics and Compute
			//  bits. So Graphics & Compute bits are main usefulness detector.
			if (props->queueFamilyProperties.queueFlags < bestQueueProps.queueFlags &&
				props->queueFamilyProperties.queueCount > 1)
			{
				bestQueueFam = queueFam;
				continue;
			}
		}
		if (!gpu->desc.OperationSupport_Raster || !gpu->desc.OperationSupport_Transfer ||
			!gpu->desc.OperationSupport_Compute)
		{
			vkPrint(51, gpu->desc.Name);
			continue;
		}

		if (gpu->propsQueue[bestQueueFam->queueFamIndex].queueFamilyProperties.queueCount > 1)
		{
			gpu->propsQueue[bestQueueFam->queueFamIndex].queueFamilyProperties.queueCount--;

			// Create internal queue
			{
				Queue* internalQueue = new (VKGLOBAL_VIRMEM_MANAGER) Queue;
				gpu->m_internalQueue = GetOpaqueHandle(internalQueue);
				internalQueue->m_queueIndx = 0;
				internalQueue->m_gpu = gpu;
				internalQueue->queueFamIndex = bestQueueFam->queueFamIndex;
				internalQueue->queue = VK_NULL_HANDLE;
				internalQueue->IsAlive = true;
			}
		}
		else
		{
			// First queue will be internal queue
			gpu->m_internalQueue = GetOpaqueHandle(mngrPriv->m_queues[totalQueueCount]);
		}

		// Create user queues
		for (uint32_t queueFamIndx = 0; queueFamIndx < gpu->desc.queueFamilyCount; queueFamIndx++)
		{
			const VkQueueFamilyProperties& props = gpu->propsQueue[queueFamIndx].queueFamilyProperties;
			QUEUEFAM_VK* queueFam = getQueueFam(gpu, queueFamIndx);
			queueFam->m_queueListStartIndx = totalQueueCount;

			for (uint32_t i = 0; i < props.queueCount; i++)
			{
				Queue* queue = mngrPriv->m_queues[totalQueueCount++];
				queue->m_gpu = gpu;
				queue->m_queueIndx = i;
				queue->queueFamIndex = queueFamIndx;
				queue->queue = VK_NULL_HANDLE;
			}
		}
	}
}

struct cmdPool_vk
{
	cmdPool_vk() = default;
	VkCommandPool secondaryCP = VK_NULL_HANDLE;
	QUEUEFAM_VK* m_queueFam = nullptr;
	std::mutex m_secondarySync;
};

VkConstU4 queueCountMax = 256;
static float VKCONST_DEFAULT_QUEUE_PRIORITIES[queueCountMax] = {};
// While creating VK Logical Device, we need which queues to create. Get that info from here.
manager_vk::queueCreateInfoList manager_vk::get_queue_cis(GPU* gpu) const
{
	for (uint32_t i = 0; i < queueCountMax; i++)
	{
		VKCONST_DEFAULT_QUEUE_PRIORITIES[i] = std::min(float(i) / queueCountMax, 1.0f);
	}
	queueCreateInfoList QueueCreationInfos;
	// Queue Creation Processes
	for (unsigned int queueFamIndx = 0; queueFamIndx < gpu->desc.queueFamilyCount; queueFamIndx++)
	{
		const QUEUEFAM_VK* queuefam = getQueueFam(gpu, queueFamIndx);
		VkDeviceQueueCreateInfo QueueInfo = {};
		QueueInfo.flags = 0;
		QueueInfo.pNext = nullptr;
		QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueInfo.queueFamilyIndex = queuefam->queueFamIndex;
		QueueInfo.pQueuePriorities = VKCONST_DEFAULT_QUEUE_PRIORITIES;
		QueueInfo.queueCount = gpu->propsQueue[queueFamIndx].queueFamilyProperties.queueCount;
		QueueCreationInfos.list[queueFamIndx] = QueueInfo;
	}
	return QueueCreationInfos;
}

void allocateCmdBuffer(
	QUEUEFAM_VK* queueFam, VkCommandBufferLevel level, cmdPool_vk*& cmdPool, VkCommandBuffer* cbs, uint32_t count)
{
	bool isFound = false;
	std::mutex* synchronizer = nullptr;
	uint32_t cmdPoolIndx = 0;
	while (!isFound && cmdPoolIndx < threadcount)
	{
		cmdPool = &queueFam->m_pools[cmdPoolIndx];
		synchronizer = &cmdPool->m_secondarySync;
		if (synchronizer->try_lock())
		{
			break;
		}
		// Loop again and again till find an unlocked one
		cmdPoolIndx = (cmdPoolIndx + 1) % threadcount;
	}

	VkCommandBufferAllocateInfo cb_ai = {};
	cb_ai.pNext = nullptr;
	cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cb_ai.commandBufferCount = count;
	cb_ai.level = level;
	cb_ai.commandPool = queueFam->m_pools[cmdPoolIndx].secondaryCP;
	if (vkAllocateCommandBuffers(queueFam->m_gpu->vk_logical, &cb_ai, cbs) != VK_SUCCESS)
	{
		vkPrint(16, "at vkAllocateCommandBuffers()");
	}
	synchronizer->unlock();
}
void freeCmdBuffer(cmdPool_vk* cmdPool, VkCommandBuffer cb)
{
	vkFreeCommandBuffers(cmdPool->m_queueFam->m_gpu->vk_logical, cmdPool->secondaryCP, 1, &cb);
}
void manager_vk::get_queue_objects(GPU* gpu)
{
	for (uint32_t queueIndx = 0; queueIndx < mngrPriv->m_queues.size(); queueIndx++)
	{
		Queue* queue = mngrPriv->m_queues[queueIndx];
		if (queue->m_gpu != gpu)
		{
			continue;
		}

		vkGetDeviceQueue(gpu->vk_logical, queue->queueFamIndex, queue->m_queueIndx, &queue->queue);

		// Create synchronizer binary semaphore
		{
			VkSemaphoreCreateInfo sem_ci = {};
			sem_ci.flags = 0;
			sem_ci.pNext = nullptr;
			sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			if (vkCreateSemaphore(gpu->vk_logical, &sem_ci, nullptr, &queue->callSynchronizer) != VK_SUCCESS)
			{
				vkPrint(16, "Queue Synchronizer Binary Semaphore Creation failed!");
			}
		}
	}
	Queue* internalQueue = GetVkObject(gpu->m_internalQueue);
	if (!internalQueue->queue)
	{
		vkGetDeviceQueue(gpu->vk_logical,
						 internalQueue->queueFamIndex,
						 gpu->propsQueue[internalQueue->queueFamIndex].queueFamilyProperties.queueCount - 1,
						 &internalQueue->queue);

		// Create synchronizer binary semaphore
		{
			VkSemaphoreCreateInfo sem_ci = {};
			sem_ci.flags = 0;
			sem_ci.pNext = nullptr;
			sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			if (vkCreateSemaphore(gpu->vk_logical, &sem_ci, nullptr, &internalQueue->callSynchronizer) != VK_SUCCESS)
			{
				vkPrint(16, "Queue Synchronizer Binary Semaphore Creation failed!");
			}
		}
	}

	// Create cmdPool for each queue family and CPU thread
	for (unsigned int queueFamIndx = 0; queueFamIndx < gpu->desc.queueFamilyCount; queueFamIndx++)
	{
		QUEUEFAM_VK* queueFam = getQueueFam(gpu, queueFamIndx);
		if (!(queueFam->m_supportFlag.IsSupportedCompute || queueFam->m_supportFlag.IsSupportedGraphics ||
			  queueFam->m_supportFlag.IsSupportedTransfer))
		{
			continue;
		}
		queueFam->m_pools = new (VKGLOBAL_VIRMEM_MANAGER) cmdPool_vk[threadcount];
		for (unsigned char threadIndx = 0; threadIndx < threadcount; threadIndx++)
		{
			cmdPool_vk& CP = queueFam->m_pools[threadIndx];
			CP.m_queueFam = queueFam;
			VkCommandPoolCreateInfo cp_ci_g = {};
			cp_ci_g.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cp_ci_g.queueFamilyIndex = queueFamIndx;
			cp_ci_g.pNext = nullptr;

			{
				cp_ci_g.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				if (vkCreateCommandPool(gpu->vk_logical, &cp_ci_g, nullptr, &CP.secondaryCP) != VK_SUCCESS)
				{
					vkPrint(16, "Secondary command pool creation for a queue has failed!");
				}
			}
		}
	}
}
uint32_t manager_vk::get_queuefam_index(Queue* fam)
{
	return fam->queueFamIndex;
}

static VkPipelineStageFlags VKCONST_PRESENTWAITSTAGEs[kMaxSemaphoreCountPerSubmit] = {
	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

struct submitList
{
	unsigned int submitCount = 0, binarySemCount = 0;
	submit_vk** submits = nullptr;
	VkSemaphore* binarySems = nullptr;
	Queue* queue = nullptr;
};
void destroyCBsubmission(GPU* gpu, VkFence fence, void* data)
{
	submitList* submission = (submitList*)data;
	for (uint32_t submitIndx = 0; submitIndx < submission->submitCount; submitIndx++)
	{
		submit_vk* submit = submission->submits[submitIndx];

		for (uint32_t cbIndx = 0; cbIndx < submit->cmdBufferCount; cbIndx++)
		{
			CommandBuffer* cmdBuffer = GetVkObject(submit->cmdBuffers[cbIndx]);
			vkDestroyCommandPool(gpu->vk_logical, cmdBuffer->cp, nullptr);
			mngrPriv->m_cmdBuffers.destroyObj(mngrPriv->m_cmdBuffers.getINDEXbyOBJ(cmdBuffer));
		}

		virmem::free_page(VK_POINTER_TO_MEMOFFSET(submit));
	}
	for (uint32_t binarySemaphoreIndx = 0; binarySemaphoreIndx < submission->binarySemCount; binarySemaphoreIndx++)
	{
		vkDestroySemaphore(gpu->vk_logical, submission->binarySems[binarySemaphoreIndx], nullptr);
	}

	vkDestroyFence(gpu->vk_logical, fence, nullptr);
	virmem::free_page(VK_POINTER_TO_MEMOFFSET(submission));
}
void Queue::checkSubmissions()
{
	for (int32_t i = 0; i < m_submissions.size(); i++)
	{
		submission_vk* submission = m_submissions[i];
		if (!submission)
		{
			continue;
		}
		if (!submission->m_gpu)
		{
			continue;
		}
		if (vkGetFenceStatus(m_gpu->vk_logical, submission->fence) == VK_SUCCESS)
		{
			submission->m_callback(submission->m_gpu, submission->fence, submission->m_userData);
			m_submissions.destroyObj(i);
		}
	}
}
void Queue::createSubmission(VkFence fence, void* data, submissionCallback callback)
{
	submission_vk* sm = m_submissions.create_OBJ();
	sm->m_gpu = m_gpu;
	sm->m_callback = callback;
	sm->m_userData = data;
	sm->fence = fence;
	sm->queueIndx = m_queueIndx;
}
void createQueueSubmitSubmission(VkFence submitFence,
								 Queue* queue,
								 uint32_t binarySemCount,
								 const VkSemaphore* binarySems)
{
	uint32_t submitCount = 0;
	for (; queue->m_unsentSubmits[submitCount]; submitCount++)
	{
	}
	uint32_t allocSize = sizeof(submitList) + (sizeof(void*) * (submitCount)) + (sizeof(VkSemaphore) * binarySemCount);
	submitList* list = (submitList*)VK_MEMOFFSET_TO_POINTER(virmem::allocatePage(allocSize));
	vm->commit(list, allocSize);
	list->submitCount = submitCount;
	list->binarySemCount = binarySemCount;
	list->submits = (submit_vk**)(list + 1);
	for (uint32_t submitIndx = 0; submitIndx < submitCount; submitIndx++)
	{
		list->submits[submitIndx] = queue->m_unsentSubmits[submitIndx];
	}
	list->binarySems = (VkSemaphore*)(list->submits + list->submitCount);
	for (uint32_t semIndx = 0; semIndx < binarySemCount; semIndx++)
	{
		list->binarySems[semIndx] = binarySems[semIndx];
	}
	list->queue = queue;
	queue->createSubmission(submitFence, list, destroyCBsubmission);
}
uint32_t Queue::sizeUnsetSubmits()
{
	uint32_t submitCount = 0;
	for (; m_unsentSubmits[submitCount] && submitCount < VKCONST_MAXUNSENTSUBMITCOUNT; submitCount++)
	{
	}
	return submitCount;
}

void vkQueueSubmit_CmdBuffers(Queue* queue, VkFence submitFence)
{
	VkSubmitInfo infos[VKCONST_MAXUNSENTSUBMITCOUNT] = {};
	VkCommandBuffer cmdBuffers[VKCONST_MAXUNSENTSUBMITCOUNT * 16] = {};
	uint32_t lastCmdBufferIndx = 0;
	const uint32_t submitCount = queue->sizeUnsetSubmits();
	for (uint32_t submitIndx = 0; submitIndx < submitCount; submitIndx++)
	{
		submit_vk* submit = queue->m_unsentSubmits[submitIndx];
		submit->submit.pWaitDstStageMask = VKCONST_PRESENTWAITSTAGEs;
		// Add queue call synchronizer semaphore as wait to sync sequential executeCmdLists calls
		if (submitIndx == 0 && queue->m_prevQueueOp == CMDBUFFER)
		{
			submit->waitSemaphores[submit->submit.waitSemaphoreCount++] = queue->callSynchronizer;
			submit->waitSemaphoreValues[submit->semaphoreInfo.waitSemaphoreValueCount++] = 0;
		}
		// Add queue call synchronizer semaphore as signal to the last submit
		if (submitIndx == submitCount - 1)
		{
			submit->signalSemaphores[submit->submit.signalSemaphoreCount++] = queue->callSynchronizer;
			submit->signalSemaphoreValues[submit->semaphoreInfo.signalSemaphoreValueCount++] = 1;
		}
		submit->submit.pCommandBuffers = &cmdBuffers[lastCmdBufferIndx];
		for (uint32_t cmdBufferIndx = 0; cmdBufferIndx < submit->submit.commandBufferCount; cmdBufferIndx++)
		{
			CommandBuffer* cmdBffr = GetVkObject(submit->cmdBuffers[cmdBufferIndx]);
			cmdBuffers[lastCmdBufferIndx++] = cmdBffr->cb;
			if (cmdBffr->cb == nullptr)
			{
				vkPrint(16, "vkCommandBuffer is nullptr");
			}
		}
		infos[submitIndx] = submit->submit;
	}
	if (vkQueueSubmit(queue->queue, submitCount, infos, submitFence) != VK_SUCCESS)
	{
		vkPrint(16, "at vkQueueSubmit()");
	}

	// Add this submit call to submission tracker
	createQueueSubmitSubmission(submitFence, queue, 0, nullptr);
}
void vkQueueSubmit_Present(Queue* queue, VkFence submitFence)
{
	VkSwapchainKHR swpchns[kMaxSwapchainCountPerSubmit] = {};
	uint32_t swpchnIndices[kMaxSwapchainCountPerSubmit] = {}, swpchnCount = 0;
	Swapchain* windows[kMaxSwapchainCountPerSubmit] = {};
	// Timeline Semaphores
	VkSemaphore waitSemaphores[kMaxSemaphoreCountPerSubmit] = {}, signalSemaphores[kMaxSemaphoreCountPerSubmit] = {};
	uint64_t waitValues[kMaxSemaphoreCountPerSubmit] = {}, signalValues[kMaxSemaphoreCountPerSubmit] = {};
	uint32_t signalSemCount = 0, waitSemCount = 0;
	// Layout transition command buffers
	VkCommandBuffer generalToPresent[kMaxSwapchainCountPerSubmit] = {},
					presentToGeneral[kMaxSwapchainCountPerSubmit] = {};

	for (uint32_t submitIndx = 0; queue->m_unsentSubmits[submitIndx]; submitIndx++)
	{
		submit_vk* submit = queue->m_unsentSubmits[submitIndx];

		// If queue call is present
		if (submit->present.sType == VK_STRUCTURE_TYPE_PRESENT_INFO_KHR)
		{
			if (swpchnCount + submit->present.swapchainCount > kMaxSwapchainCountPerSubmit)
			{
				vkPrint(16, "Max swapchain count per submit is exceeded!");
				break;
			}
			for (uint32_t swpchnIndx = 0;
				 swpchnIndx < submit->present.swapchainCount && swpchnCount < kMaxSwapchainCountPerSubmit;
				 swpchnIndx++)
			{
				swpchnIndices[swpchnCount] = submit->m_windows[swpchnIndx]->m_swapchainCurrentTextureIndx;
				swpchns[swpchnCount] = submit->m_windows[swpchnIndx]->Swapchain;
				windows[swpchnCount] = submit->m_windows[swpchnIndx];
				generalToPresent[swpchnCount] =
					submit->m_windows[swpchnIndx]
						->vk_generalToPresent[queue->queueFamIndex][swpchnIndices[swpchnCount]];
				presentToGeneral[swpchnCount] =
					submit->m_windows[swpchnIndx]->vk_presentToGeneral[GetVkObject(queue->m_gpu->m_internalQueue)
																		   ->queueFamIndex][swpchnIndices[swpchnCount]];

				swpchnCount++;
			}
		} // If queue call is wait/signal
		else if (submit->submit.sType == VK_STRUCTURE_TYPE_SUBMIT_INFO)
		{
			for (uint32_t i = 0; i < submit->submit.waitSemaphoreCount; i++)
			{
				waitSemaphores[i + waitSemCount] = submit->submit.pWaitSemaphores[i];
				waitValues[i + waitSemCount] = submit->waitSemaphoreValues[i];
			}
			waitSemCount += submit->submit.waitSemaphoreCount;

			for (uint32_t i = 0; i < submit->submit.signalSemaphoreCount; i++)
			{
				signalSemaphores[i + signalSemCount] = submit->submit.pSignalSemaphores[i];
				signalValues[i + signalSemCount] = submit->signalSemaphoreValues[i];
			}
			signalSemCount += submit->submit.signalSemaphoreCount;
		} // Queue call invalid
		else
		{
			vkPrint(16, "One of the submits is invalid!");
		}
	}

	VkSemaphore binarySignalSemaphores[kMaxSemaphoreCountPerSubmit] = {};
	// Submit for timeline -> binary conversion
	{
		for (uint32_t i = 0; i < waitSemCount; i++)
		{
			VkSemaphoreCreateInfo ci = {};
			ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			if (vkCreateSemaphore(queue->m_gpu->vk_logical, &ci, nullptr, &binarySignalSemaphores[i]) != VK_SUCCESS)
			{
				vkPrint(16, "Present's Timeline->Binary converter failed at binary semaphore creation!");
			}
		}
		VkTimelineSemaphoreSubmitInfo timInfo = {};
		timInfo.pNext = nullptr;
		timInfo.pSignalSemaphoreValues = nullptr;
		timInfo.signalSemaphoreValueCount = 0;
		timInfo.pWaitSemaphoreValues = waitValues;
		timInfo.waitSemaphoreValueCount = waitSemCount;
		timInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		VkSubmitInfo si = {};
		si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		si.pNext = &timInfo;
		si.commandBufferCount = swpchnCount;
		si.pCommandBuffers = generalToPresent;
		si.waitSemaphoreCount = waitSemCount;
		si.pWaitDstStageMask = VKCONST_PRESENTWAITSTAGEs;
		si.pWaitSemaphores = waitSemaphores;
		if (queue->m_prevQueueOp == CMDBUFFER)
		{
			si.waitSemaphoreCount = waitSemCount + 1;
			timInfo.waitSemaphoreValueCount++;
			waitValues[waitSemCount] = 0;
			waitSemaphores[waitSemCount] = queue->callSynchronizer;
		}
		si.signalSemaphoreCount = signalSemCount;
		si.pSignalSemaphores = binarySignalSemaphores;
		if (vkQueueSubmit(queue->queue, 1, &si, VK_NULL_HANDLE) != VK_SUCCESS)
		{
			vkPrint(16, "at vkQueueSubmit() of Present's Timeline->Binary converter");
		}
	}

	VkPresentInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.pNext = nullptr;
	info.pSwapchains = swpchns;
	info.swapchainCount = swpchnCount;
	info.pImageIndices = swpchnIndices;
	// For now, all calls are synchronized after each other
	// Because we don't have timeline semaphore emulation in binary semaphores
	info.waitSemaphoreCount = waitSemCount;
	info.pWaitSemaphores = binarySignalSemaphores;
	info.pResults = nullptr;
	VkResult result = vkQueuePresentKHR(queue->queue, &info);
	if (result != VK_SUCCESS)
	{
		vkPrint(16, "at vkQueuePresentKHR()");
	}

	// Send a submit to signal timeline semaphore when all binary semaphore are signaled
	{
		VkSemaphore binAcquireSemaphores[kMaxSwapchainCountPerSubmit] = {};
		for (uint32_t i = 0; i < swpchnCount; i++)
		{
			binAcquireSemaphores[i] = windows[i]->acquireSemaphore;
		}

		VkTimelineSemaphoreSubmitInfo temSignalSemaphoresInfo = {};
		temSignalSemaphoresInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		temSignalSemaphoresInfo.pWaitSemaphoreValues = nullptr;
		temSignalSemaphoresInfo.waitSemaphoreValueCount = 0;
		temSignalSemaphoresInfo.pNext = nullptr;
		temSignalSemaphoresInfo.signalSemaphoreValueCount = signalSemCount;
		temSignalSemaphoresInfo.pSignalSemaphoreValues = signalValues;
		VkSubmitInfo acquireSubmit = {};
		acquireSubmit.waitSemaphoreCount = swpchnCount;
		acquireSubmit.pWaitSemaphores = binAcquireSemaphores;
		acquireSubmit.signalSemaphoreCount = signalSemCount;
		acquireSubmit.pSignalSemaphores = signalSemaphores;
		acquireSubmit.commandBufferCount = swpchnCount;
		acquireSubmit.pCommandBuffers = presentToGeneral;
		acquireSubmit.pNext = nullptr;
		acquireSubmit.pWaitDstStageMask = VKCONST_PRESENTWAITSTAGEs;
		acquireSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		if (vkQueueSubmit(GetVkObject(queue->m_gpu->m_internalQueue)->queue, 1, &acquireSubmit, submitFence))
		{
			vkPrint(16, "at vkQueueSubmit() for binary -> timeline semaphore conversion");
		}
	}

	// Add this submit call to tracker
	createQueueSubmitSubmission(submitFence, queue, waitSemCount, binarySignalSemaphores);
}
void manager_vk::queueSubmit(Queue* queue)
{
	GPU* gpu = queue->m_gpu;
	// Check previously sent submission to detect if they're still executing
	queue->checkSubmissions();

	VkFence submitFence = {};
	{
		VkFenceCreateInfo f_ci = {};
		f_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		if (vkCreateFence(gpu->vk_logical, &f_ci, nullptr, &submitFence) != VK_SUCCESS)
		{
			vkPrint(52, "at Submission tracker fence creation");
		}
	}
	switch (queue->m_activeQueueOp)
	{
	case ERROR_QUEUEOPTYPE: queue->m_activeQueueOp = CMDBUFFER;
	case CMDBUFFER: vkQueueSubmit_CmdBuffers(queue, submitFence); break;
	case PRESENT: vkQueueSubmit_Present(queue, submitFence); break;
	default: vkPrint(52, "Active queue operation type isn't supported by backend!"); break;
	}

	queue->m_prevQueueOp = queue->m_activeQueueOp;
	queue->m_activeQueueOp = ERROR_QUEUEOPTYPE;
	for (uint32_t submitIndx = 0; submitIndx < VKCONST_MAXUNSENTSUBMITCOUNT; submitIndx++)
	{
		queue->m_unsentSubmits[submitIndx] = nullptr;
	}
}
bool manager_vk::does_queuefamily_support(Queue* family, const queueflag_vk& flag)
{
	vkPrint(16, "does_queuefamily_support() isn't coded");
	return false;
}

// Helper functions

void VK_getQueueAndSharingInfos(unsigned int queueList,
								TGfxQueue const* i_queueList,
								unsigned int extCount,
								struct tgfx_extension* const* i_exts,
								uint32_t* o_famList,
								uint32_t* o_famListSize,
								VkSharingMode* o_sharingMode)
{
	uint32_t validQueueFamCount = 0;
	GPU* theGPU = nullptr;
	for (uint32_t listIndx = 0; listIndx < queueList; listIndx++)
	{
		Queue* queue = GetVkObject(i_queueList[listIndx]);
		if (!queue)
		{
			continue;
		}
		GPU* gpu = queue->m_gpu;
		if (!theGPU)
		{
			theGPU = gpu;
		}
		if (theGPU != gpu)
		{
			vkPrint(37);
			return;
		}
		bool isPreviouslyAdded = false;
		for (uint32_t validQueueIndx = 0; validQueueIndx < validQueueFamCount; validQueueIndx++)
		{
			if (o_famList[validQueueIndx] == queue->queueFamIndex)
			{
				isPreviouslyAdded = true;
				break;
			}
		}
		if (!isPreviouslyAdded)
		{
			o_famList[validQueueFamCount++] = queue->queueFamIndex;
		}
	}
	if (validQueueFamCount > 1)
	{
		*o_sharingMode = VK_SHARING_MODE_CONCURRENT;
	}
	else
	{
		*o_sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	*o_famListSize = validQueueFamCount;
	for (uint32_t i = *o_famListSize; i < kMaxQueueFamilyCountPerGpu; i++)
	{
		o_famList[i] = UINT32_MAX;
	}
}

// Renderer Command Buffer Functionality

TGfxCommandBuffer beginCommandBuffer(TGfxQueue i_queue, unsigned int extCount, TGfxExtension* exts)
{
	getGPUfromQueueHnd(i_queue);
	QUEUEFAM_VK* queueFam = getQueueFam(gpu, queue->queueFamIndex);

	VkCommandPoolCreateInfo cp_ci = {};
	cp_ci.flags = 0;
	cp_ci.pNext = nullptr;
	cp_ci.queueFamilyIndex = queue->queueFamIndex;
	cp_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	VkCommandPool cp = {};
	if (vkCreateCommandPool(gpu->vk_logical, &cp_ci, nullptr, &cp) != VK_SUCCESS)
	{
		vkPrint(53, "at vkCreateCommandPool()");
		return nullptr;
	}
	VkCommandBuffer cb = {};
	VkCommandBufferAllocateInfo cb_ai = {};
	cb_ai.commandBufferCount = 1;
	cb_ai.commandPool = cp;
	cb_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cb_ai.pNext = nullptr;
	if (vkAllocateCommandBuffers(gpu->vk_logical, &cb_ai, &cb) != VK_SUCCESS)
	{
		vkPrint(53, "at vkAllocatteCommandBuffers()");
		return nullptr;
	}
	VkCommandBufferBeginInfo cb_bi = {};
	cb_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cb_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(cb, &cb_bi) != VK_SUCCESS)
	{
		vkPrint(53, "at vkBeginCommandBuffer() failed!");
		vkDestroyCommandPool(gpu->vk_logical, cp, nullptr);
		return nullptr;
	}
	CommandBuffer* cmdBuffer = mngrPriv->m_cmdBuffers.create_OBJ();
	cmdBuffer->cp = cp;
	cmdBuffer->cb = cb;
	cmdBuffer->m_gpuIndx = gpu->gpuIndx();
	cmdBuffer->m_queueFamIndx = queueFam->queueFamIndex;
	return GetOpaqueHandle(cmdBuffer);
}

#define getCmdBufferfromHnd(cmdBufferHnd)                                                                              \
	CommandBuffer* cmdBuffer = GetVkObject(cmdBufferHnd);                                                              \
	GPU* gpu = GetGpuFromIndex(cmdBuffer->m_gpuIndx);                                                                  \
	QUEUEFAM_VK* queueFam = getQueueFam(gpu, cmdBuffer->m_queueFamIndx);

#define checkCmdBufferHnd()                                                                                            \
	if (gpu == nullptr || queueFam == nullptr)                                                                         \
	{                                                                                                                  \
		vkPrint(11);                                                                                                   \
		return;                                                                                                        \
	}

void endCommandBuffer(TGfxCommandBuffer cb)
{
	getCmdBufferfromHnd(cb);
#ifdef VULKAN_DEBUGGING
	checkCmdBufferHnd();
#endif
	if (vkEndCommandBuffer(cmdBuffer->cb) != VK_SUCCESS)
	{
		vkPrint(53, "at vkEndCommandBuffer()");
	}
}
void executeBundles(TGfxCommandBuffer cb,
					unsigned int bundleCount,
					TGfxCommandBundle const* bundles,
					unsigned int extCount,
					TGfxExtension* exts)
{
	getCmdBufferfromHnd(cb);
#ifdef VULKAN_DEBUGGING
	checkCmdBufferHnd();
#endif

	if (bundleCount > VKCONST_MAXCMDBUNDLE_PERCALL)
	{
		vkPrint(16, "Bundle list size exceeded -VKCONST_MAXCMDBUNDLE_PERCALL-, please report this");
		return;
	}
	VkCommandBuffer secCmdBuffers[VKCONST_MAXCMDBUNDLE_PERCALL] = {};
	if (!bundleCount)
	{
		return;
	}
	getSecondaryCmdBuffers(bundleCount, bundles, queueFam->queueFamIndex, secCmdBuffers);
	vkCmdExecuteCommands(cmdBuffer->cb, bundleCount, secCmdBuffers);
}

void beginRasterpass(TGfxCommandBuffer commandBuffer,
					 unsigned int colorAttachmentCount,
					 const TGfxRasterPassBeginSlotInfo* colorAttachments,
					 const TGfxRasterPassBeginSlotInfo* depthAttachment,
					 unsigned int extCount,
					 TGfxExtension* exts)
{
	getCmdBufferfromHnd(commandBuffer);
#ifdef VULKAN_DEBUGGING
	checkCmdBufferHnd();
#endif

	DynamicRenderingExtension* dynRenderingExt =
		(DynamicRenderingExtension*)gpu->ext()->m_exts[IVkExt::DynamicRenderingExtension];
	dynRenderingExt->beginRenderpass(
		cmdBuffer->cb, colorAttachmentCount, colorAttachments, *depthAttachment, extCount, exts);
}
void nextRendersubpass(TGfxCommandBuffer commandBuffer)
{
	getCmdBufferfromHnd(commandBuffer);
#ifdef VULKAN_DEBUGGING
	checkCmdBufferHnd();
#endif

	vkCmdNextSubpass(cmdBuffer->cb, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}
void endRasterpass(TGfxCommandBuffer commandBuffer, unsigned int extCount, TGfxExtension* exts)
{
	getCmdBufferfromHnd(commandBuffer);
#ifdef VULKAN_DEBUGGING
	checkCmdBufferHnd();
#endif

	DynamicRenderingExtension* dynRenderingExt =
		(DynamicRenderingExtension*)gpu->ext()->m_exts[IVkExt::DynamicRenderingExtension];
	dynRenderingExt->endRenderpass(cmdBuffer->cb, extCount, exts);
}

void addSubmitToUnsentList(Queue* queue, submit_vk* submit)
{
	for (uint32_t submitIndx = 0; submitIndx < VKCONST_MAXUNSENTSUBMITCOUNT; submitIndx++)
	{
		if (!queue->m_unsentSubmits[submitIndx])
		{
			queue->m_unsentSubmits[submitIndx] = submit;
			return;
		}
	}
	vkPrint(16, "Unsent submit count limit is exceeded!");
}
static constexpr VkPipelineStageFlags waitDstStageMask[kMaxSemaphoreCountPerSubmit * 2] = {
	VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
void queueExecuteCmdBuffers(TGfxQueue i_queue,
							unsigned int cmdBufferCount,
							TGfxCommandBuffer const* i_cmdBuffers,
							unsigned int extCount,
							TGfxExtension* exts)
{
	getGPUfromQueueHnd(i_queue);
	getTimelineSemaphoreEXT(gpu, semSys);

	if (!cmdBufferCount)
	{
		return;
	}
	if (queue->m_activeQueueOp != ERROR_QUEUEOPTYPE && queue->m_activeQueueOp != CMDBUFFER)
	{
		vkPrint(54);
		return;
	}
	queue->m_activeQueueOp = CMDBUFFER;

	{
		for (uint32_t i = 0; i < cmdBufferCount; i++)
		{
			getCmdBufferfromHnd(i_cmdBuffers[i]);
			checkCmdBufferHnd();
		}
	}

	submit_vk* submit = submit_vk::allocateSubmit(0, 0, cmdBufferCount, 0);
	memcpy(submit->cmdBuffers, i_cmdBuffers, sizeof(TGfxCommandBuffer) * cmdBufferCount);
	submit->submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit->submit.pNext = nullptr;
	submit->submit.pSignalSemaphores = submit->signalSemaphores;
	submit->submit.pWaitSemaphores = submit->waitSemaphores;
	submit->submit.pWaitDstStageMask = waitDstStageMask;
	submit->submit.commandBufferCount = cmdBufferCount;
	addSubmitToUnsentList(queue, submit);
}
void queueFenceWaitSignal(TGfxQueue i_queue,
						  unsigned int waitsCount,
						  TGfxFence const* waitFences,
						  const unsigned long long* waitValues,
						  unsigned int signalsCount,
						  TGfxFence const* signalFences,
						  const unsigned long long* signalValues)
{
	getGPUfromQueueHnd(i_queue);
	getTimelineSemaphoreEXT(gpu, semSys);
	const FENCE_VKOBJ* waits[kMaxSemaphoreCountPerSubmit] = {};
	{
		if (waitsCount > kMaxSemaphoreCountPerSubmit)
		{
			vkPrint(52, "Max semaphore count per submit is exceeded!");
			return;
		}
		for (uint32_t i = 0; i < waitsCount; i++)
		{
			FENCE_VKOBJ* wait = getOBJ<FENCE_VKOBJ>(waitFences[i]);
			assert(wait);
			waits[i] = wait;
			wait->m_curValue.store(waitValues[i]);
		}
	}
	const FENCE_VKOBJ* signals[kMaxSemaphoreCountPerSubmit] = {};
	{
		if (signalsCount > kMaxSemaphoreCountPerSubmit)
		{
			vkPrint(52, "Max semaphore count per submit is exceeded!");
			return;
		}
		for (uint32_t i = 0; i < signalsCount; i++)
		{
			FENCE_VKOBJ* signal = getOBJ<FENCE_VKOBJ>(signalFences[i]);
			assert(signal);
			signals[i] = signal;
			signal->m_nextValue.store(signalValues[i]);
		}
	}

	// TIMELINE SEMAPHORE

	// Create and fill submit struct
	submit_vk* submit = submit_vk::allocateSubmit(signalsCount, waitsCount, 0, 0);
	{
		uint32_t& waitSemaphoreCount = submit->submit.waitSemaphoreCount;
		for (waitSemaphoreCount = 0; waitSemaphoreCount < kMaxSemaphoreCountPerSubmit; waitSemaphoreCount++)
		{
			const FENCE_VKOBJ* fence = waits[waitSemaphoreCount];
			if (!fence)
			{
				break;
			}
			submit->waitSemaphoreValues[waitSemaphoreCount] = fence->m_curValue;
			submit->waitSemaphores[waitSemaphoreCount] = fence->timelineSemaphore;
		}
		uint32_t& signalSemaphoreCount = submit->submit.signalSemaphoreCount;
		for (signalSemaphoreCount = 0; signalSemaphoreCount < kMaxSemaphoreCountPerSubmit; signalSemaphoreCount++)
		{
			const FENCE_VKOBJ* fence = signals[signalSemaphoreCount];
			if (!fence)
			{
				break;
			}
			submit->signalSemaphoreValues[signalSemaphoreCount] = fence->m_nextValue;
			submit->signalSemaphores[signalSemaphoreCount] = fence->timelineSemaphore;
		}

		submit->semaphoreInfo.pNext = nullptr;
		submit->semaphoreInfo.pSignalSemaphoreValues = submit->signalSemaphoreValues;
		submit->semaphoreInfo.pWaitSemaphoreValues = submit->waitSemaphoreValues;
		submit->semaphoreInfo.signalSemaphoreValueCount = signalSemaphoreCount;
		submit->semaphoreInfo.waitSemaphoreValueCount = waitSemaphoreCount;
		submit->semaphoreInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		submit->submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit->submit.pNext = &submit->semaphoreInfo;
		submit->submit.pCommandBuffers = nullptr;
		submit->submit.commandBufferCount = 0;
		submit->submit.pSignalSemaphores = submit->signalSemaphores;
		submit->submit.pWaitSemaphores = submit->waitSemaphores;
		submit->submit.pWaitDstStageMask = waitDstStageMask;
	}
	addSubmitToUnsentList(queue, submit);
}
void queueSubmit(TGfxQueue i_queue)
{
	getGPUfromQueueHnd(i_queue);
	manager->queueSubmit(queue);
}
void queuePresent(TGfxQueue i_queue, unsigned int windowCount, TGfxSwapchain const* windowlist)
{
	getGPUfromQueueHnd(i_queue);

	if (queue->m_activeQueueOp != ERROR_QUEUEOPTYPE && queue->m_activeQueueOp != PRESENT)
	{
		vkPrint(54);
		return;
	}

	queue->m_activeQueueOp = PRESENT;
	submit_vk* submit = submit_vk::allocateSubmit(0, 0, 0, windowCount);

	for (uint32_t i = 0; i < windowCount; i++)
	{
		submit->m_windows[i] = GetVkObject(windowlist[i]);
	}
	submit->type = PRESENT;
	submit->present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	submit->present.pNext = nullptr;
	submit->present.pWaitSemaphores = nullptr;
	submit->present.waitSemaphoreCount = 0;
	submit->present.pResults = nullptr;
	submit->present.pImageIndices = nullptr;
	submit->present.pSwapchains = nullptr;
	submit->present.swapchainCount = windowCount;
	addSubmitToUnsentList(queue, submit);
}

VkCommandPool getSecondaryCmdPool(QUEUEFAM_VK* queueFam, uint32_t cmdPoolIndx)
{
	cmdPool_vk& cmdPool = queueFam->m_pools[cmdPoolIndx];
	if (!cmdPool.secondaryCP)
	{
		vkPrint(16, "at getSecondaryCmdPool()");
		return nullptr;
	}
	return cmdPool.secondaryCP;
}
void setQueueFncPtrs()
{
	core_tgfx_main->renderer->beginCommandBuffer = beginCommandBuffer;
	core_tgfx_main->renderer->endCommandBuffer = endCommandBuffer;
	core_tgfx_main->renderer->executeBundles = executeBundles;
	core_tgfx_main->renderer->beginRasterpass = beginRasterpass;
	core_tgfx_main->renderer->nextSubRasterpass = nextRendersubpass;
	core_tgfx_main->renderer->endRasterpass = endRasterpass;
	core_tgfx_main->renderer->queueExecuteCmdBuffers = queueExecuteCmdBuffers;
	core_tgfx_main->renderer->queueFenceSignalWait = queueFenceWaitSignal;
	core_tgfx_main->renderer->queueSubmit = queueSubmit;
	core_tgfx_main->renderer->queuePresent = queuePresent;
	core_tgfx_main->helpers->getGPUInfo_Queues = &getGPUInfoQueues;

	for (uint32_t i = 0; i < kMaxSemaphoreCountPerSubmit; i++)
	{
		VKCONST_PRESENTWAITSTAGEs[i] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
}

} // namespace Vulkan
} // namespace TGFX