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
struct submission_vk
{
	VkFence fence = nullptr;
	GPU* m_gpu = nullptr;
	void* m_userData = nullptr;
	uint32_t queueIdx = UINT32_MAX;
};

enum queueOpType : TU1
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
	for (uint32_t submitIdx = 0; submitIdx < submission->submitCount; submitIdx++)
	{
		submit_vk* submit = submission->submits[submitIdx];

		for (uint32_t cbIdx = 0; cbIdx < submit->cmdBufferCount; cbIdx++)
		{
			CommandBuffer* cmdBuffer = GetVkObject(submit->cmdBuffers[cbIdx]);
			vkDestroyCommandPool(gpu->vk_logical, cmdBuffer->Pool, nullptr);
			mngrPriv->m_cmdBuffers.destroyObj(mngrPriv->m_cmdBuffers.getINDEXbyOBJ(cmdBuffer));
		}

		virmem::free_page(VK_POINTER_TO_MEMOFFSET(submit));
	}
	for (uint32_t binarySemaphoreIdx = 0; binarySemaphoreIdx < submission->binarySemCount; binarySemaphoreIdx++)
	{
		vkDestroySemaphore(gpu->vk_logical, submission->binarySems[binarySemaphoreIdx], nullptr);
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
		if (!submission->GetGpu())
		{
			continue;
		}
		if (vkGetFenceStatus(m_gpu->vk_logical, submission->fence) == VK_SUCCESS)
		{
			submission->m_callback(submission->GetGpu(), submission->fence, submission->m_userData);
			m_submissions.destroyObj(i);
		}
	}
}
void Queue::createSubmission(VkFence fence, void* data, submissionCallback callback)
{
	submission_vk* sm = m_submissions.create_OBJ();
	sm->GetGpu() = m_gpu;
	sm->m_callback = callback;
	sm->m_userData = data;
	sm->fence = fence;
	sm->queueIdx = QueueIdx;
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
	for (uint32_t submitIdx = 0; submitIdx < submitCount; submitIdx++)
	{
		list->submits[submitIdx] = queue->m_unsentSubmits[submitIdx];
	}
	list->binarySems = (VkSemaphore*)(list->submits + list->submitCount);
	for (uint32_t semIdx = 0; semIdx < binarySemCount; semIdx++)
	{
		list->binarySems[semIdx] = binarySems[semIdx];
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
	uint32_t lastCmdBufferIdx = 0;
	const uint32_t submitCount = queue->sizeUnsetSubmits();
	for (uint32_t submitIdx = 0; submitIdx < submitCount; submitIdx++)
	{
		submit_vk* submit = queue->m_unsentSubmits[submitIdx];
		submit->submit.pWaitDstStageMask = VKCONST_PRESENTWAITSTAGEs;
		// Add queue call synchronizer semaphore as wait to sync sequential executeCmdLists calls
		if (submitIdx == 0 && queue->m_prevQueueOp == CMDBUFFER)
		{
			submit->waitSemaphores[submit->submit.waitSemaphoreCount++] = queue->CallSynchronizer;
			submit->waitSemaphoreValues[submit->semaphoreInfo.waitSemaphoreValueCount++] = 0;
		}
		// Add queue call synchronizer semaphore as signal to the last submit
		if (submitIdx == submitCount - 1)
		{
			submit->signalSemaphores[submit->submit.signalSemaphoreCount++] = queue->CallSynchronizer;
			submit->signalSemaphoreValues[submit->semaphoreInfo.signalSemaphoreValueCount++] = 1;
		}
		submit->submit.pCommandBuffers = &cmdBuffers[lastCmdBufferIdx];
		for (uint32_t cmdBufferIdx = 0; cmdBufferIdx < submit->submit.commandBufferCount; cmdBufferIdx++)
		{
			CommandBuffer* cmdBffr = GetVkObject(submit->cmdBuffers[cmdBufferIdx]);
			cmdBuffers[lastCmdBufferIdx++] = cmdBffr->Buffer;
			if (cmdBffr->Buffer == nullptr)
				vkPrint(16, "vkCommandBuffer is nullptr");
		}
		infos[submitIdx] = submit->submit;
	}
	if (vkQueueSubmit(queue->queue, submitCount, infos, submitFence) != VK_SUCCESS)
		vkPrint(16, "at vkQueueSubmit()");

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

	for (uint32_t submitIdx = 0; queue->m_unsentSubmits[submitIdx]; submitIdx++)
	{
		submit_vk* submit = queue->m_unsentSubmits[submitIdx];

		// If queue call is present
		if (submit->present.sType == VK_STRUCTURE_TYPE_PRESENT_INFO_KHR)
		{
			if (swpchnCount + submit->present.swapchainCount > kMaxSwapchainCountPerSubmit)
			{
				vkPrint(16, "Max swapchain count per submit is exceeded!");
				break;
			}
			for (uint32_t swpchnIdx = 0;
				 swpchnIdx < submit->present.swapchainCount && swpchnCount < kMaxSwapchainCountPerSubmit;
				 swpchnIdx++)
			{
				swpchnIndices[swpchnCount] = submit->m_windows[swpchnIdx]->m_swapchainCurrentTextureIdx;
				swpchns[swpchnCount] = submit->m_windows[swpchnIdx]->Swpchn;
				windows[swpchnCount] = submit->m_windows[swpchnIdx];
				generalToPresent[swpchnCount] =
					submit->m_windows[swpchnIdx]->vk_generalToPresent[queue->QueueFamIdx][swpchnIndices[swpchnCount]];
				presentToGeneral[swpchnCount] =
					submit->m_windows[swpchnIdx]->vk_presentToGeneral[GetVkObject(queue->GetGpu()->m_internalQueue)
																		  ->QueueFamIdx][swpchnIndices[swpchnCount]];

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
			if (vkCreateSemaphore(queue->GetGpu()->vk_logical, &ci, nullptr, &binarySignalSemaphores[i]) != VK_SUCCESS)
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
			waitSemaphores[waitSemCount] = queue->CallSynchronizer;
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
			binAcquireSemaphores[i] = windows[i]->AcquireSemaphore;

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
		if (vkQueueSubmit(GetVkObject(queue->GetGpu()->m_internalQueue)->queue, 1, &acquireSubmit, submitFence))
			vkPrint(16, "at vkQueueSubmit() for binary -> timeline semaphore conversion");
	}

	// Add this submit call to tracker
	createQueueSubmitSubmission(submitFence, queue, waitSemCount, binarySignalSemaphores);
}
void manager_vk::queueSubmit(Queue* queue)
{
	GPU* gpu = queue->GetGpu();
	// Check previously sent submission to detect if they're still executing
	queue->checkSubmissions();

	VkFence submitFence = {};
	{
		VkFenceCreateInfo f_ci = {};
		f_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		TCORE_SOFT_CHECK(vkCreateFence(gpu->vk_logical, &f_ci, nullptr, &submitFence) == VK_SUCCESS,
						 "Vulkan Submission Tracker Fence creation failed");
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
	for (uint32_t submitIdx = 0; submitIdx < VKCONST_MAXUNSENTSUBMITCOUNT; submitIdx++)
		queue->m_unsentSubmits[submitIdx] = nullptr;
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
	for (uint32_t listIdx = 0; listIdx < queueList; listIdx++)
	{
		Queue* queue = GetVkObject(i_queueList[listIdx]);
		if (!queue)
		{
			continue;
		}
		GPU* gpu = queue->GetGpu();
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
		for (uint32_t validQueueIdx = 0; validQueueIdx < validQueueFamCount; validQueueIdx++)
		{
			if (o_famList[validQueueIdx] == queue->QueueFamIdx)
			{
				isPreviouslyAdded = true;
				break;
			}
		}
		if (!isPreviouslyAdded)
		{
			o_famList[validQueueFamCount++] = queue->QueueFamIdx;
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

void addSubmitToUnsentList(Queue* queue, submit_vk* submit)
{
	for (uint32_t submitIdx = 0; submitIdx < VKCONST_MAXUNSENTSUBMITCOUNT; submitIdx++)
	{
		if (!queue->m_unsentSubmits[submitIdx])
		{
			queue->m_unsentSubmits[submitIdx] = submit;
			return;
		}
	}
	vkPrint(16, "Unsent submit count limit is exceeded!");
}
void setQueueFncPtrs()
{

	for (uint32_t i = 0; i < kMaxSemaphoreCountPerSubmit; i++)
	{
		VKCONST_PRESENTWAITSTAGEs[i] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
}

} // namespace Vulkan
} // namespace TGFX