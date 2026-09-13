/*
  This file is responsible for managing TGFX Command Bundle.
  TGFX Command Bundle = Secondary Command Buffers.
*/
#include "vk_renderer.h"

#include <algorithm>
#include <numeric>
#include <utility>
#include <array>

#include "TGfxRenderer.h"
#include "vk_contentmanager.h"
#include "vk_core.h"
#include "vk_predefinitions.h"
#include "vk_resource.h"

namespace TGFX
{
namespace Vulkan
{

VkConstU4 kMaxFenceCountPerSubmit = 8, kMaxPrimaryCmdBufferCount = 32;

#define getCmdBufferfromHnd(cmdBufferHnd)                                                                              \
	CommandBuffer* cmdBuffer = GetVkObject(cmdBufferHnd);                                                              \
	GPU* gpu = cmdBuffer->GetGpu();

#define checkCmdBufferHnd()                                                                                            \
	if (gpu == nullptr)                                                                                                \
	{                                                                                                                  \
		vkPrint(11);                                                                                                   \
		return;                                                                                                        \
	}

#define GetGpuFromQueue(queueHnd)                                                                                      \
	Queue* queue = GetVkObject(queueHnd);                                                                              \
	GPU* gpu = queue->GetGpu();

VkPipelineStageFlags gWaitStagesForPresentOperation[kMaxSemaphoreCountPerSubmit] = {
	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

struct submission_vk
{
	VkFence fence = nullptr;
	GPU* m_gpu = nullptr;
	void* m_userData = nullptr;
	uint32_t queueIdx = UINT32_MAX;
};

struct submit_vk
{
	Queue::OperationType type = Queue::CMDBUFFER;
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
		submit_vk* submit = (submit_vk*)TCore::Malloc(allocSize);
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

		TCore::Free(submit);
	}
	for (uint32_t binarySemaphoreIdx = 0; binarySemaphoreIdx < submission->binarySemCount; binarySemaphoreIdx++)
	{
		vkDestroySemaphore(gpu->vk_logical, submission->binarySems[binarySemaphoreIdx], nullptr);
	}

	vkDestroyFence(gpu->vk_logical, fence, nullptr);
}
void Queue::checkSubmissions()
{
	for (int32_t i = 0; i < m_submissions.size(); i++)
	{
		submission_vk* submission = m_submissions[i];
		if (!submission || !submission->GetGpu())
			continue;
		if (vkGetFenceStatus(GetGpu()->vk_logical, submission->fence) == VK_SUCCESS)
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

	submitList* list = (submitList*)TCore::Malloc(allocSize);
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
		submit->submit.pWaitDstStageMask = gWaitStagesForPresentOperation;
		// Add queue call synchronizer semaphore as wait to sync sequential executeCmdLists calls
		if (submitIdx == 0 && queue->m_prevQueueOp == Queue::CMDBUFFER)
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
			return;
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
				vkPrint(16, "Present's Timeline->Binary converter failed at binary semaphore creation!");
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
		si.commandBufferCount = 0;
		si.pCommandBuffers = nullptr;
		si.waitSemaphoreCount = waitSemCount;
		si.pWaitDstStageMask = gWaitStagesForPresentOperation;
		si.pWaitSemaphores = waitSemaphores;
		if (queue->m_prevQueueOp == Queue::CMDBUFFER)
		{
			si.waitSemaphoreCount = waitSemCount + 1;
			timInfo.waitSemaphoreValueCount++;
			waitValues[waitSemCount] = 0;
			waitSemaphores[waitSemCount] = queue->CallSynchronizer;
		}
		si.signalSemaphoreCount = signalSemCount;
		si.pSignalSemaphores = binarySignalSemaphores;
		if (vkQueueSubmit(queue->queue, 1, &si, VK_NULL_HANDLE) != VK_SUCCESS)
			vkPrint(16, "at vkQueueSubmit() of Present's Timeline->Binary converter");
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
		vkPrint(16, "at vkQueuePresentKHR()");

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
		acquireSubmit.commandBufferCount = 0;
		acquireSubmit.pCommandBuffers = nullptr;
		acquireSubmit.pNext = nullptr;
		acquireSubmit.pWaitDstStageMask = gWaitStagesForPresentOperation;
		acquireSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		if (vkQueueSubmit(GetVkObject(queue->GetGpu()->m_internalQueue)->queue, 1, &acquireSubmit, submitFence))
			vkPrint(16, "at vkQueueSubmit() for binary -> timeline semaphore conversion");
	}

	// Add this submit call to tracker
	createQueueSubmitSubmission(submitFence, queue, waitSemCount, binarySignalSemaphores);
}
void QueueSubmit(TGfxQueue q)
{
	GetGpuFromQueue(q);
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
	switch (queue->ActiveQueueOperation)
	{
	case Queue::ERROR_QUEUEOPTYPE: queue->ActiveQueueOperation = Queue::CMDBUFFER;
	case Queue::CMDBUFFER: vkQueueSubmit_CmdBuffers(queue, submitFence); break;
	case Queue::PRESENT: vkQueueSubmit_Present(queue, submitFence); break;
	default: vkPrint(52, "Active queue operation type isn't supported by Vulkan backend!"); break;
	}

	queue->m_prevQueueOp = queue->m_activeQueueOp;
	queue->ActiveQueueOperation = Queue::ERROR_QUEUEOPTYPE;
	for (uint32_t submitIdx = 0; submitIdx < VKCONST_MAXUNSENTSUBMITCOUNT; submitIdx++)
		queue->m_unsentSubmits[submitIdx] = nullptr;
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

struct CommandContainer;
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

	CommandContainer* m_cmds = {};
	uint64_t m_cmdCount = 0;

	void createCmdBuffer(uint64_t cmdCount);
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(CommandBundle, Vk)

struct CmdBarrierTexture : Command<CmdBarrierTexture, CommandType::BarrierTexture>
{
	void CmdExecute(CommandBundle* bundle)
	{
		vkCmdPipelineBarrier(bundle->ActiveCb,
							 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
							 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
							 VK_DEPENDENCY_BY_REGION_BIT,
							 0,
							 nullptr,
							 0,
							 nullptr,
							 1,
							 &BarrierInfo);
	}

	// Command specific variables should have "m_" prefix
	VkImageMemoryBarrier BarrierInfo = {};
};

struct CmdBarrierBuffer : Command<CmdBarrierBuffer, CommandType::BarrierBuffer>
{
	void CmdExecute(CommandBundle* bundle)
	{
		vkCmdPipelineBarrier(bundle->ActiveCb,
							 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
							 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
							 VK_DEPENDENCY_BY_REGION_BIT,
							 0,
							 nullptr,
							 1,
							 &BarrierInfo,
							 0,
							 nullptr);
	}

	// Command specific variables should have "m_" prefix
	VkBufferMemoryBarrier BarrierInfo = {};
};

struct CmdBindBindingTables : Command<CmdBindBindingTables, CommandType::BindBindingTables>
{
	void CmdExecute(CommandBundle* cmdBundle)
	{
		VkDescriptorSet sets[kMaxDescSetPerList] = {};
		for (uint32_t i = 0; i < m_setCount; i++)
		{
			BindingTableInstance* table = GetVkObject(tables[i]);
			sets[i] = table->vk_set;
			cmdBundle->ActiveDescSets[i] = table->vk_layout;
		}
		vkCmdBindDescriptorSets(cmdBundle->ActiveCb,
								bindPoint,
								cmdBundle->ActivePipelineLayout,
								m_firstSetIdx,
								m_setCount,
								sets,
								0,
								nullptr);
	}
	VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
	TGfxBindingTable tables[kMaxDescSetPerList] = {};
	uint32_t m_setCount = 0, m_firstSetIdx = 0;
};

struct CmdBindPipeline : Command<CmdBindPipeline, CommandType::BindPipeline>
{
	void CmdExecute(CommandBundle* cmdBundle)
	{
		vkCmdBindPipeline(cmdBundle->ActiveCb, bindPoint, pipeline);
		cmdBundle->ActivePipeline = pipeline;
		cmdBundle->ActivePipelineLayout = pipelineLayout;
	}
	VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
	VkPipelineHnd pipeline;
	VkPipelineLayoutHnd pipelineLayout;
};

struct CmdDispatch : Command<CmdDispatch, CommandType::Dispatch>
{
	void CmdExecute(CommandBundle* cmdBundle)
	{
		vkCmdDispatch(cmdBundle->ActiveCb, m_dispatchSize.x, m_dispatchSize.y, m_dispatchSize.z);
	};
	TGfxUVec3 m_dispatchSize;
};

struct CmdSetViewport : Command<CmdSetViewport, CommandType::SetViewport>
{
	void CmdExecute(CommandBundle* cmdBundle) { vkCmdSetViewport(cmdBundle->ActiveCb, 0, 1, &viewport); };

	VkViewport viewport = {};
};

struct CmdSetScissor : Command<CmdSetScissor, CommandType::SetScissor>
{
	void CmdExecute(CommandBundle* cmdBundle) { vkCmdSetScissor(cmdBundle->ActiveCb, 0, 1, &rect); };
	VkRect2D rect = {};
};

struct CmdDrawNonIndexedIndirect : public Command<CmdDrawNonIndexedIndirect, CommandType::DrawNonIndexedIndirect>
{
	void CmdExecute(CommandBundle* cmdBundle)
	{
		vkCmdDraw(cmdBundle->ActiveCb, vertexCount, instanceCount, firstVertex, firstInstance);
	};
	uint32_t vertexCount = {}, instanceCount = {}, firstVertex = {}, firstInstance = {};
};

struct CmdDrawIndexedDirect : public Command<CmdDrawIndexedDirect, CommandType::DrawIndexedDirect>
{
	void CmdExecute(CommandBundle* cmdBundle)
	{
		vkCmdDrawIndexed(cmdBundle->ActiveCb, indxCount, instanceCount, firstIdx, vertexOffset, firstInstance);
	};
	uint32_t indxCount = {}, instanceCount = {}, firstIdx = {}, firstInstance = {};
	int32_t vertexOffset = {};
};

struct CmdBindIndexBuffer : public Command<CmdBindIndexBuffer, CommandType::BindIndexBuffer>
{
	void CmdExecute(CommandBundle* cmdBundle) { vkCmdBindIndexBuffer(cmdBundle->ActiveCb, buffer, offset, indexType); };
	VkBuffer buffer;
	VkDeviceSize offset;
	VkIndexType indexType;
};

struct CmdSetDepthBounds : public Command<CmdSetDepthBounds, CommandType::SetDepthBounds>
{
	void CmdExecute(CommandBundle* cmdBundle) { vkCmdSetDepthBounds(cmdBundle->ActiveCb, min, max); };
	float min = 0.0f, max = 1.0f;
};

struct CmdCopyBufferToTexture : public Command<CmdCopyBufferToTexture, CommandType::CopyBufferToTexture>
{
	void CmdExecute(CommandBundle* cmdBundle)
	{
		vkCmdCopyBufferToImage(cmdBundle->ActiveCb, src, dst, dstImageLayout, 1, &copy);
	};
	VkBuffer src;
	VkImage dst;
	VkImageLayout dstImageLayout;
	VkBufferImageCopy copy;
};

struct CmdBindVertexBuffers : public Command<CmdBindVertexBuffers, CommandType::BindVertexBuffers>
{
	void CmdExecute(CommandBundle* cmdBundle)
	{
		vkCmdBindVertexBuffers(cmdBundle->ActiveCb, firstBinding, bindingCount, buffers, bufferOffsets);
	};
	VkBuffer buffers[VKCONST_MAXVERTEXBINDINGCOUNT];
	VkDeviceSize bufferOffsets[VKCONST_MAXVERTEXBINDINGCOUNT];
	uint32_t firstBinding, bindingCount;
};

VkDeviceSize findIndirectOperationDataSize(TGfxIndirectOperationType opType)
{
	switch (opType)
	{
	case TGFX_INDIRECTOPERATIONTYPE_DRAWNONINDEXED: return sizeof(VkDrawIndirectCommand);
	case TGFX_INDIRECTOPERATIONTYPE_DRAWINDEXED: return sizeof(VkDrawIndexedIndirectCommand);
	case TGFX_INDIRECTOPERATIONTYPE_DISPATCH: return sizeof(VkDispatchIndirectCommand);
	}
	return UINT64_MAX;
}
struct CmdExecuteIndirect : public Command<CmdExecuteIndirect, CommandType::ExecuteIndirect>
{
	void CmdExecute(CommandBundle* cmdBundle)
	{
		VkDeviceSize activeOffset = bufferOffset;
		for (uint32_t stateIdx = 0; stateIdx < opStateCount; stateIdx++)
		{
			uint64_t loopCount = 1, drawCount = opStates[stateIdx].opCount;
			auto opType = opStates[stateIdx].opType;
			uint64_t indirectArgumentDataSize = findIndirectOperationDataSize(opStates[stateIdx].opType);
			// If GPU doesn't support multiDrawIndirect or execute type is compute, call same VkCmd*
			// multiple times with incrementing offsets
			if (!cmdBundle->GetGpu()->vk_featuresDev.features.multiDrawIndirect ||
				opType == TGFX_INDIRECTOPERATIONTYPE_DISPATCH)
			{
				loopCount = opStates[stateIdx].opCount;
				drawCount = 1;
			}
			for (uint32_t loopIdx = 0; loopIdx < loopCount; loopIdx++)
			{
				switch (opType)
				{
				case TGFX_INDIRECTOPERATIONTYPE_DRAWNONINDEXED:
					vkCmdDrawIndirect(cmdBundle->ActiveCb, buffer, activeOffset, drawCount, indirectArgumentDataSize);
					break;
				case TGFX_INDIRECTOPERATIONTYPE_DRAWINDEXED:
					vkCmdDrawIndexedIndirect(
						cmdBundle->ActiveCb, buffer, activeOffset, drawCount, indirectArgumentDataSize);
					break;
				case TGFX_INDIRECTOPERATIONTYPE_DISPATCH:
					vkCmdDispatchIndirect(cmdBundle->ActiveCb, buffer, activeOffset);
					break;
				default: vkPrint(59); return;
				}
				activeOffset += indirectArgumentDataSize * drawCount;
			}
		}
	};
	void cmd_destroy()
	{
		// virmem::free_page(VK_POINTER_TO_MEMOFFSET(opStates));
	}
	struct IndirectOperationState
	{
		uint32_t opCount;
		TGfxIndirectOperationType opType;
	};
	IndirectOperationState* opStates = {};
	uint32_t opStateCount = 0;
	VkBuffer buffer;
	VkDeviceSize bufferOffset;
};

struct CmdCopyBufferToBuffer : public Command<CmdCopyBufferToBuffer, CommandType::CopyBufferToBuffer>
{
	static constexpr CommandType cmd_type = CommandType::CopyBufferToBuffer;

	void CmdExecute(CommandBundle* cmdBundle)
	{
		vkCmdCopyBuffer(cmdBundle->ActiveCb, srcBuffer, dstBuffer, 1, &bufCopy);
	};

	VkBuffer srcBuffer = {}, dstBuffer = {};
	VkBufferCopy bufCopy = {};
};

struct CmdPushConstant : public Command<CmdPushConstant, CommandType::PushConstant>
{
	static constexpr CommandType cmd_type = CommandType::PushConstant;

	void CmdExecute(CommandBundle* cmdBundle)
	{
		vkCmdPushConstants(
			cmdBundle->ActiveCb, cmdBundle->ActivePipelineLayout, VK_SHADER_STAGE_ALL, offset, size, data);
	};

	unsigned char offset, size, data[1];
};

// Maximum size of command struct is calculated at compile time and stored in this struct. This is used to allocate
// memory for command structs in CommandBundle.
struct CommandContainer
{
	CommandType Type = CommandType::error_2;

	// From https://stackoverflow.com/a/46408751
	template <typename... T>
	static constexpr size_t max_sizeof()
	{
		return std::max({sizeof(T)...});
	}

#define vkCmdStructsLists                                                                                              \
	CmdBarrierTexture, CmdBindBindingTables, CmdBindPipeline, CmdDispatch, CmdBindVertexBuffers, CmdExecuteIndirect,   \
		CmdCopyBufferToTexture, CmdPushConstant
	static constexpr uint32_t kMaxCommandStructSize = max_sizeof<vkCmdStructsLists>();
	uint8_t Data[kMaxCommandStructSize] = {};
	CommandContainer() : Type(CommandType::error) {}
};

template <typename T>
T* createCmdStruct(CommandContainer* cmd)
{
	static_assert(T::Type != CommandType::error,
				  "You forgot to specify command type as \"cmd_type\" variable in command struct");
	cmd->Type = T::Type;
	static_assert(CommandContainer::kMaxCommandStructSize >= sizeof(T),
				  "You forgot to specify the struct in CommandContainer::kMaxCommandStructSize!");
	*(T*)cmd->Data = T();
	return (T*)cmd->Data;
}

void destroyCmd(CommandContainer& cmd)
{
	switch (cmd.Type)
	{
	case CommandType::ExecuteIndirect: ((CmdExecuteIndirect*)cmd.Data)->cmd_destroy(); break;
	}
}

void CommandBundle::createCmdBuffer(uint64_t cmdCount)
{
	uint32_t allocSize = sizeof(CommandContainer) * cmdCount;
	m_cmds = (CommandContainer*)TCAllocator->Malloc(TCore::GSuperMemoryBlock, allocSize, "Command Bundle buffer");
	m_cmdCount = cmdCount;
	for (uint32_t i = 0; i < cmdCount; i++)
		m_cmds[i] = {};
}

// Synchronization Functions

void CreateFences(TGfxGpu g, TU4 count, TU8 initValue, TBool isShared, TGfxFence* fenceList)
{
	GPU* gpu = GetVkObject(g);

	VkFenceCreateInfo fCi{};
	fCi.flags = 0;
	fCi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkSemaphoreCreateInfo timelineSemaphoreCi{};
	VkSemaphoreCreateInfo binarySemaphoreCi{};
	timelineSemaphoreCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Timeline semaphores are not allowed on exported semaphores
	VkSemaphoreTypeCreateInfo timelineCreateInfo;
	if (!isShared)
	{
		timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		timelineCreateInfo.pNext = NULL;
		timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		timelineCreateInfo.initialValue = initValue;
		Append_pNext(&timelineSemaphoreCi, &timelineCreateInfo);
	}

	VkExportFenceCreateInfo exportFCi{};
	VkExportSemaphoreCreateInfo exportSCi{};
	if (isShared)
	{
		exportFCi.sType = VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO;
		exportFCi.handleTypes = kSharedFenceHandleType;
		Append_pNext(&fCi, &exportFCi);

		exportSCi.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
		exportSCi.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
		Append_pNext(&timelineSemaphoreCi, &exportSCi);
	}
	for (uint32_t i = 0; i < count; i++)
	{
		auto fenceObj = GContentManagerContext->Fences.CreateObject(gpu);
		VkFence vkFence{};
		TCORE_SOFT_CHECK(vkCreateFence(gpu->vk_logical, &fCi, nullptr, &vkFence) == VK_SUCCESS,
						 "Failed to create vkFence");
		fenceObj->FenceHnd.Set(vkFence);

		VkSemaphore timelineSemaphore{};
		TCORE_SOFT_CHECK(vkCreateSemaphore(gpu->vk_logical, &timelineSemaphoreCi, nullptr, &timelineSemaphore) ==
							 VK_SUCCESS,
						 "Failed to create vkSemaphore");
		fenceObj->TimelineSemaphoreHnd.Set(timelineSemaphore);

		// Remove timeline semaphore create info from pNext of binary semaphore create info
		binarySemaphoreCi = timelineSemaphoreCi;
		if (!isShared)
			binarySemaphoreCi.pNext = nullptr;

		VkSemaphore binarySemaphore{};
		TCORE_SOFT_CHECK(vkCreateSemaphore(gpu->vk_logical, &binarySemaphoreCi, nullptr, &binarySemaphore) ==
							 VK_SUCCESS,
						 "Failed to create vkSemaphore");
		fenceObj->BinarySemaphoreHnd.Set(binarySemaphore);

		fenceList[i] = GetOpaqueHandle(fenceObj);
	}
}

void DestroyFence(TGfxFence fence)
{
	auto vkFence = GetVkObject(fence);
	GPU* gpu = vkFence->GetGpu();
	vkDestroySemaphore(gpu->vk_logical, vkFence->TimelineSemaphoreHnd, nullptr);
	vkFence->TimelineSemaphoreHnd.SetAsDead();
	vkDestroyFence(gpu->vk_logical, vkFence->FenceHnd, nullptr);
	vkFence->FenceHnd.SetAsDead();
}

// Command Bundle Functions
////////////////////////////

TGfxCommandBundle BeginCommandBundle(TGfxGpu gpu, TSize maxCmdCount, TGfxPipeline defaultPipeline, TGfxExtension* exts)
{
	VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
	GPU* GPU = GetVkObject(gpu);
	if (!GPU)
		return nullptr;

	CommandBundle* cmdBundle = GRendererContext->CommandBundles.CreateObject(GPU);
	for (uint32_t i = 0; i < kMaxQueueFamilyCountPerGpu; i++)
		cmdBundle->SecondaryCommandBuffers[i] = {};

	cmdBundle->createCmdBuffer(maxCmdCount);
	cmdBundle->m_defaultPipeline = defaultPipeline;
	if (defaultPipeline)
	{
		Pipeline* pipe = GetVkObject(defaultPipeline);
		cmdBundle->BindPoint = pipe->vk_type;
	}
	else
		cmdBundle->BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

	return GetOpaqueHandle(cmdBundle);
}

#define DEFINE_EXECUTE_COMMAND(CommandName)                                                                            \
	case CommandType::CommandName: ((Cmd##CommandName*)cmd)->CmdExecute(bundle); break;
void ExecuteCommand(CommandContainer* cmd, CommandBundle* bundle)
{
	switch (cmd->Type)
	{
		DEFINE_EXECUTE_COMMAND(BindBindingTables);
		DEFINE_EXECUTE_COMMAND(BindVertexBuffers);
		DEFINE_EXECUTE_COMMAND(BindIndexBuffer);
		DEFINE_EXECUTE_COMMAND(SetDepthBounds);
		DEFINE_EXECUTE_COMMAND(SetViewport);
		DEFINE_EXECUTE_COMMAND(SetScissor);
		DEFINE_EXECUTE_COMMAND(DrawNonIndexedIndirect);
		DEFINE_EXECUTE_COMMAND(DrawIndexedDirect);
		DEFINE_EXECUTE_COMMAND(ExecuteIndirect);
		DEFINE_EXECUTE_COMMAND(BarrierTexture);
		DEFINE_EXECUTE_COMMAND(BarrierBuffer);
		DEFINE_EXECUTE_COMMAND(BindPipeline);
		DEFINE_EXECUTE_COMMAND(Dispatch);
		DEFINE_EXECUTE_COMMAND(CopyBufferToTexture);
		DEFINE_EXECUTE_COMMAND(CopyBufferToBuffer);
		DEFINE_EXECUTE_COMMAND(PushConstant);
	}
}

void FinishCommandBundle(TGfxCommandBundle bndl, TGfxExtension* exts)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto gpu = bundle->GetGpu();

	VkCommandBufferBeginInfo bi = {};
	bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	VkCommandBufferInheritanceInfo secInfo = {};
	secInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	bi.pInheritanceInfo = &secInfo;

	VkCommandBufferInheritanceRenderingInfo rInfo = {};
	if (bundle->BindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS)
	{
		bi.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

		rInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
		rInfo.viewMask = 0;
		Pipeline* defaultPipe = GetVkObject(bundle->m_defaultPipeline);
		rInfo.pColorAttachmentFormats = defaultPipe->vk_colorAttachmentFormats;
		while (rInfo.colorAttachmentCount < TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT &&
			   rInfo.pColorAttachmentFormats[rInfo.colorAttachmentCount] != VK_FORMAT_UNDEFINED)
			rInfo.colorAttachmentCount++;
		rInfo.depthAttachmentFormat = defaultPipe->vk_depthAttachmentFormat;
		rInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		secInfo.pNext = &rInfo;
	}

	for (TU4 queueFamIdx = 0; queueFamIdx < gpu->desc.QueueFamilyCount; queueFamIdx)
	{
		bool suitable = false;
		switch (bundle->BindPoint)
		{
		case VK_PIPELINE_BIND_POINT_GRAPHICS:
			if (gpu->vk_propsQueue[queueFamIdx].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				suitable = true;
			break;
		case VK_PIPELINE_BIND_POINT_COMPUTE:
			if (gpu->vk_propsQueue[queueFamIdx].queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
				suitable = true;
			break;
		}
		if (!suitable)
			false;

		VkCommandBufferHnd cb = GetSecondaryCmdBuffer(gpu, queueFamIdx);

		if (vkBeginCommandBuffer(cb, &bi) != VK_SUCCESS)
		{
			vkPrint(16, "at vkBeginCommandBuffer()");
			return;
		}
		if (bundle->m_defaultPipeline)
		{
			Pipeline* pipe = GetVkObject(bundle->m_defaultPipeline);
			vkCmdBindPipeline(cb, pipe->vk_type, pipe->vk_object);
			bundle->ActivePipeline = pipe->vk_object;
			bundle->ActivePipelineLayout = pipe->vk_layout;
		}
		for (uint64_t cmdIdx = 0; cmdIdx < bundle->m_cmdCount; cmdIdx++)
			ExecuteCommand(&bundle->m_cmds[cmdIdx], bundle);
		if (vkEndCommandBuffer(cb) != VK_SUCCESS)
			vkPrint(16, "at vkEndCommandBuffer()");
	}
}
void DestroyCommandBundle(TGfxCommandBundle hnd)
{
	CommandBundle* bundle = GetVkObject(hnd);

	for (uint32_t i = 0; i < bundle->m_cmdCount; i++)
		destroyCmd(bundle->m_cmds[i]);

	delete[] bundle->m_cmds;

	static constexpr TU8 cmdBufferCount =
		sizeof(bundle->SecondaryCommandBuffers) / sizeof(bundle->SecondaryCommandBuffers[0]);
	for (uint32_t i = 0; i < cmdBufferCount; i++)
	{
		auto& cmdBuffer = bundle->SecondaryCommandBuffers[i];
		if (cmdBuffer != VK_NULL_HANDLE)
			freeCmdBuffer(bundle->cmdPools[i], cmdBuffer);
	}
}

void cmdBindBindingTables(TGfxCommandBundle bndl,
						  unsigned long long sortKey,
						  unsigned int firstSetIdx,
						  unsigned int bindingTableCount,
						  TGfxBindingTable const* bindingTables,
						  TGfxPipelineType pipelineType)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmd = createCmdStruct<CmdBindBindingTables>(&bundle->m_cmds[sortKey]);

	{
		uint32_t descSetLimit =
			std::min(kMaxDescSetPerList, bundle->GetGpu()->vk_propsDev.properties.limits.maxBoundDescriptorSets);
		if (bindingTableCount > descSetLimit)
		{
			vkPrint(22, "Max binding table count is exceeded!");
			return;
		}
		for (uint32_t i = 0; i < bindingTableCount; i++)
		{
			BindingTableInstance* bindingTable = GetVkObject(bindingTables[i]);
			assert(bindingTable && "Binding table isn't found!");
			cmd->tables[cmd->m_setCount++] = bindingTables[i];
		}
	}

	cmd->bindPoint = GetVkEnum(pipelineType);
	cmd->m_firstSetIdx = firstSetIdx;
}
void cmdBindPipeline(TGfxCommandBundle bndl, unsigned long long sortKey, TGfxPipeline pipeline)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmd = createCmdStruct<CmdBindPipeline>(&bundle->m_cmds[sortKey]);
	Pipeline* pipe = GetVkObject(pipeline);

	if (pipe->vk_type != bundle->BindPoint)
	{
		vkPrint(61);
	}
	cmd->bindPoint = pipe->vk_type;
	cmd->pipeline = pipe->vk_object;
	cmd->pipelineLayout = pipe->vk_layout;
}
void cmdSetViewport(TGfxCommandBundle bndl, unsigned long long sortKey, const TGfxViewportInfo* viewport)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmd = createCmdStruct<CmdSetViewport>(&bundle->m_cmds[sortKey]);

	cmd->viewport.x = viewport->TopLeftCorner.x;
	cmd->viewport.y = viewport->TopLeftCorner.y;
	cmd->viewport.width = viewport->Size.x;
	cmd->viewport.height = viewport->Size.y;
	cmd->viewport.minDepth = viewport->DepthMinMax.x;
	cmd->viewport.maxDepth = viewport->DepthMinMax.y;
}
void cmdSetScissor(TGfxCommandBundle bndl, TU8 sortKey, TGfxIVec2 offset, TGfxUVec2 size)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmd = createCmdStruct<CmdSetScissor>(&bundle->m_cmds[sortKey]);

	cmd->rect.offset.x = offset.x;
	cmd->rect.offset.y = offset.x;
	cmd->rect.extent.width = size.x;
	cmd->rect.extent.height = size.y;
}
void cmdSetDepthBounds(TGfxCommandBundle bndl, unsigned long long sortKey, float min, float max)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmd = createCmdStruct<CmdSetDepthBounds>(&bundle->m_cmds[sortKey]);

	cmd->min = min;
	cmd->max = max;
}
void cmdBindVertexBuffers(TGfxCommandBundle bndl,
						  unsigned long long sortKey,
						  unsigned int firstBinding,
						  unsigned int bindingCount,
						  TGfxBuffer const* buffers,
						  const unsigned long long* offsets)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmd = createCmdStruct<CmdBindVertexBuffers>(&bundle->m_cmds[sortKey]);

	cmd->firstBinding = firstBinding;
	cmd->bindingCount = bindingCount;
	for (uint32_t i = 0; i < bindingCount; i++)
	{
		cmd->bufferOffsets[i] = offsets[i];
		cmd->buffers[i] = GetVkObject(buffers[i])->vk_buffer;
	}
}
void cmdBindIndexBuffer(TGfxCommandBundle bndl,
						unsigned long long sortKey,
						TGfxBuffer buffer,
						unsigned long long offset,
						unsigned char IndexTypeSize)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmd = createCmdStruct<CmdBindIndexBuffer>(&bundle->m_cmds[sortKey]);

	cmd->buffer = GetVkObject(buffer)->vk_buffer;
	switch (IndexTypeSize)
	{
	case 1: cmd->indexType = VK_INDEX_TYPE_UINT8_EXT;
	case 2: cmd->indexType = VK_INDEX_TYPE_UINT16; break;
	case 4: cmd->indexType = VK_INDEX_TYPE_UINT32; break;
	}
	cmd->offset = offset;
}
void cmdDrawNonIndexedDirect(TGfxCommandBundle bndl,
							 unsigned long long sortKey,
							 unsigned int vertexCount,
							 unsigned int instanceCount,
							 unsigned int firstVertex,
							 unsigned int firstInstance)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmd = createCmdStruct<CmdDrawNonIndexedIndirect>(&bundle->m_cmds[sortKey]);

	cmd->firstInstance = firstInstance;
	cmd->firstVertex = firstVertex;
	cmd->vertexCount = vertexCount;
	cmd->instanceCount = instanceCount;
}
void cmdDrawIndexedDirect(TGfxCommandBundle bndl,
						  unsigned long long sortKey,
						  unsigned int indexCount,
						  unsigned int instanceCount,
						  unsigned int firstIndex,
						  int vertexOffset,
						  unsigned int firstInstance)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmd = createCmdStruct<CmdDrawIndexedDirect>(&bundle->m_cmds[sortKey]);

	cmd->firstIdx = firstIndex;
	cmd->firstInstance = firstInstance;
	cmd->indxCount = indexCount;
	cmd->instanceCount = instanceCount;
	cmd->vertexOffset = vertexOffset;
}
void cmdExecuteIndirect(TGfxCommandBundle bndl,
						TU8 sortKey,
						TU4 operationCount,
						const TGfxIndirectOperationType* operationTypes,
						TGfxBuffer dataBffr,
						TU8 indirectBufferOffset,
						TGfxExtension* exts)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmd = createCmdStruct<CmdExecuteIndirect>(&bundle->m_cmds[sortKey]);

	// Find operation state count
	for (uint32_t i = 0; i < operationCount;)
	{
		auto opType = operationTypes[i];
		for (; i < operationCount && opType == operationTypes[i]; i++)
		{
		}
		cmd->opStateCount++;
	}
	uint32_t allocSize = sizeof(CmdExecuteIndirect::IndirectOperationState) * cmd->opStateCount;
	cmd->opStates = new CmdExecuteIndirect::IndirectOperationState[cmd->opStateCount];
	for (uint32_t i = 0, stateIdx = 0; i < operationCount;)
	{
		auto opType = operationTypes[i];
		cmd->opStates[stateIdx].opType = opType;
		cmd->opStates[stateIdx].opCount = 0;
		for (; i < operationCount && opType == operationTypes[i]; i++, cmd->opStates[stateIdx].opCount++)
		{
		}
		stateIdx++;
	}
	cmd->buffer = GetVkObject(dataBffr)->vk_buffer;
	cmd->bufferOffset = indirectBufferOffset;
}
void cmdBarrierTexture(TGfxCommandBundle bndl,
					   TU8 key,
					   TGfxTexture i_texture,
					   TGfxImageAccess lastAccess,
					   TGfxImageAccess nextAccess,
					   TGfxExtension* exts)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmdBar = createCmdStruct<CmdBarrierTexture>(&bundle->m_cmds[key]);
	cmdBar->BarrierInfo.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	cmdBar->BarrierInfo.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	cmdBar->BarrierInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	cmdBar->BarrierInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	cmdBar->BarrierInfo.subresourceRange.baseArrayLayer = 0;
	cmdBar->BarrierInfo.subresourceRange.baseMipLevel = 0;
	cmdBar->BarrierInfo.subresourceRange.layerCount = 1;
	cmdBar->BarrierInfo.subresourceRange.levelCount = 1;
	Texture* texture = GetVkObject(i_texture);
	cmdBar->BarrierInfo.image = texture->vk_image;
	findImageAccessPattern(lastAccess, cmdBar->BarrierInfo.srcAccessMask, cmdBar->BarrierInfo.oldLayout);
	findImageAccessPattern(nextAccess, cmdBar->BarrierInfo.dstAccessMask, cmdBar->BarrierInfo.newLayout);
	cmdBar->BarrierInfo.pNext = nullptr;
}
void cmdDispatch(TGfxCommandBundle bndl, unsigned long long key, const TGfxUVec3 dispatchSize)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmd = createCmdStruct<CmdDispatch>(&bundle->m_cmds[key]);

	cmd->m_dispatchSize = dispatchSize;
}
void cmdCopyBufferToTexture(TGfxCommandBundle bndl,
							unsigned long long key,
							TGfxBuffer srcBuffer,
							unsigned long long bufferOffset,
							TGfxTexture dstTexture,
							TGfxImageAccess lastAccess,
							TGfxExtension* exts)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmd = createCmdStruct<CmdCopyBufferToTexture>(&bundle->m_cmds[key]);

	Buffer* buffer = GetVkObject(srcBuffer);
	Texture* texture = GetVkObject(dstTexture);

	cmd->src = buffer->vk_buffer;
	cmd->dst = texture->vk_image;
	VkAccessFlags flag;
	findImageAccessPattern(lastAccess, flag, cmd->dstImageLayout);
	cmd->copy.imageOffset = {};
	cmd->copy.imageExtent.width = texture->Size.x;
	cmd->copy.imageExtent.height = texture->Size.y;
	cmd->copy.imageExtent.depth = 1;
	cmd->copy.bufferImageHeight = 0;
	cmd->copy.bufferOffset = bufferOffset;
	cmd->copy.bufferRowLength = 0;
	if (texture->m_channels == TGFX_TEXTURE_CHANNELS_D32)
	{
		cmd->copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if (texture->m_channels == TGFX_TEXTURE_CHANNELS_D24S8)
	{
		cmd->copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
	{
		cmd->copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	cmd->copy.imageSubresource.baseArrayLayer = 0;
	cmd->copy.imageSubresource.layerCount = 1;
	cmd->copy.imageSubresource.mipLevel = 0;
}

void cmdCopyBufferToBuffer(TGfxCommandBundle bndl,
						   unsigned long long key,
						   unsigned long long size,
						   TGfxBuffer srcBuffer,
						   unsigned long long srcOffset,
						   TGfxBuffer dstBuffer,
						   unsigned long long dstOffset)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmd = createCmdStruct<CmdCopyBufferToBuffer>(&bundle->m_cmds[key]);

	cmd->bufCopy.dstOffset = dstOffset;
	cmd->bufCopy.size = size;
	cmd->bufCopy.srcOffset = srcOffset;
	cmd->dstBuffer = GetVkObject(dstBuffer)->vk_buffer;
	cmd->srcBuffer = GetVkObject(srcBuffer)->vk_buffer;
}
void cmdPushConstant(
	TGfxCommandBundle bndl, unsigned long long key, unsigned char offset, unsigned char size, const void* d)
{
	CommandBundle* bundle = GetVkObject(bndl);
	auto* cmd = createCmdStruct<CmdPushConstant>(&bundle->m_cmds[key]);
	size = std::min(128u, uint32_t(size));
	cmd->size = size;
	cmd->offset = offset;
	memcpy(cmd->data, d, size);
}
void getSecondaryCmdBuffers(unsigned int cmdBundleCount,
							TGfxCommandBundle const* cmdBundles,
							uint32_t queueFamIdx,
							VkCommandBufferHnd* secondaryCmdBuffers)
{
	uint32_t bundleCount = 0;
	for (uint32_t bundleListIdx = 0; bundleListIdx < cmdBundleCount; bundleListIdx++)
	{
		if (!cmdBundles[bundleListIdx])
			continue;

		const TGfxCommandBundle bundleHnd = cmdBundles[bundleListIdx];
		CommandBundle* bundle = GetVkObject(bundleHnd);

		if (!bundle || bundleCount >= kMaxBundleCountPerCall)
			continue;

		VkCommandBufferHnd vkCmdBuffer = bundle->SecondaryCommandBuffers[queueFamIdx];

		// Command bundle isn't used in this queue fam, so use it
		if (vkCmdBuffer == VK_NULL_HANDLE)
		{
		}

		secondaryCmdBuffers[bundleCount++] = vkCmdBuffer;
	}
}

static constexpr VkPipelineStageFlags waitDstStageMask[kMaxSemaphoreCountPerSubmit * 2] = {
	VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
void queueExecuteCmdBuffers(TGfxQueue i_queue,
							unsigned int cmdBufferCount,
							TGfxCommandBuffer const* i_cmdBuffers,
							TGfxExtension* exts)
{
	GetGpuFromQueue(i_queue);
	if (!cmdBufferCount)
		return;

	if (queue->ActiveQueueOperation != Queue::ERROR_QUEUEOPTYPE && queue->ActiveQueueOperation != Queue::CMDBUFFER)
	{
		vkPrint(54);
		return;
	}
	queue->ActiveQueueOperation = Queue::CMDBUFFER;

	// Validate command buffer handles
	for (uint32_t i = 0; i < cmdBufferCount; i++)
	{
		getCmdBufferfromHnd(i_cmdBuffers[i]);
		checkCmdBufferHnd();
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
	GetGpuFromQueue(i_queue);
	const Fence* waits[kMaxSemaphoreCountPerSubmit] = {};
	{
		if (waitsCount > kMaxSemaphoreCountPerSubmit)
		{
			vkPrint(52, "Max semaphore count per submit is exceeded!");
			return;
		}
		for (uint32_t i = 0; i < waitsCount; i++)
		{
			Fence* wait = GetVkObject(waitFences[i]);
			assert(wait);
			waits[i] = wait;
			wait->m_curValue.store(waitValues[i]);
		}
	}
	const Fence* signals[kMaxSemaphoreCountPerSubmit] = {};
	{
		if (signalsCount > kMaxSemaphoreCountPerSubmit)
		{
			vkPrint(52, "Max semaphore count per submit is exceeded!");
			return;
		}
		for (uint32_t i = 0; i < signalsCount; i++)
		{
			Fence* signal = GetVkObject(signalFences[i]);
			assert(signal);
			signals[i] = signal;
			signal->NextValue.store(signalValues[i]);
		}
	}

	// TIMELINE SEMAPHORE

	// Create and fill submit struct
	submit_vk* submit = submit_vk::allocateSubmit(signalsCount, waitsCount, 0, 0);
	{
		uint32_t& waitSemaphoreCount = submit->submit.waitSemaphoreCount;
		for (waitSemaphoreCount = 0; waitSemaphoreCount < kMaxSemaphoreCountPerSubmit; waitSemaphoreCount++)
		{
			const Fence* fence = waits[waitSemaphoreCount];
			if (!fence)
				break;
			submit->waitSemaphoreValues[waitSemaphoreCount] = fence->m_curValue;
			submit->waitSemaphores[waitSemaphoreCount] = fence->timelineSemaphore;
		}
		uint32_t& signalSemaphoreCount = submit->submit.signalSemaphoreCount;
		for (signalSemaphoreCount = 0; signalSemaphoreCount < kMaxSemaphoreCountPerSubmit; signalSemaphoreCount++)
		{
			const Fence* fence = signals[signalSemaphoreCount];
			if (!fence)
				break;

			submit->signalSemaphoreValues[signalSemaphoreCount] = fence->m_nextValue;
			submit->signalSemaphores[signalSemaphoreCount] = fence->TimelineSemaphoreHnd;
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

void queuePresent(TGfxQueue i_queue, unsigned int windowCount, TGfxSwapchain const* windowlist)
{
	GetGpuFromQueue(i_queue);

	if (queue->ActiveQueueOperation != Queue::ERROR_QUEUEOPTYPE && queue->ActiveQueueOperation != Queue::PRESENT)
	{
		vkPrint(54);
		return;
	}

	queue->ActiveQueueOperation = Queue::PRESENT;
	submit_vk* submit = submit_vk::allocateSubmit(0, 0, 0, windowCount);

	for (uint32_t i = 0; i < windowCount; i++)
		submit->m_windows[i] = GetVkObject(windowlist[i]);
	submit->type = Queue::PRESENT;
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

TGfxCommandBuffer beginCommandBuffer(TGfxQueue i_queue, TGfxExtension* exts)
{
	auto queue = GetVkObject(i_queue);
	VkCommandBufferHnd cb = CreatePrimaryCommandBuffer(queue->GetGpu(), queue->QueueFamIdx);

	VkCommandBufferBeginInfo cb_bi = {};
	cb_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cb_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	TCORE_SOFT_CHECK(vkBeginCommandBuffer(cb, &cb_bi) == VK_SUCCESS, "Failed to begin command buffer");

	CommandBuffer* cmdBuffer = mngrPriv->m_cmdBuffers.create_OBJ();
	cmdBuffer->Pool = cp;
	cmdBuffer->Buffer = cb;
	cmdBuffer->QueueFamIdx = queue->QueueFamIdx;
	return GetOpaqueHandle(cmdBuffer);
}

void endCommandBuffer(TGfxCommandBuffer cb)
{
	getCmdBufferfromHnd(cb);
#ifdef VULKAN_DEBUGGING
	checkCmdBufferHnd();
#endif
	if (vkEndCommandBuffer(cmdBuffer->Buffer) != VK_SUCCESS)
		vkPrint(53, "at vkEndCommandBuffer()");
}
void executeBundles(TGfxCommandBuffer cb,
					unsigned int bundleCount,
					TGfxCommandBundle const* bundles,
					TGfxExtension* exts)
{
	getCmdBufferfromHnd(cb);
#ifdef VULKAN_DEBUGGING
	checkCmdBufferHnd();
#endif

	if (!bundleCount)
		return;
	TCORE_SOFT_CHECK(bundleCount <= kMaxBundleCountPerCall,
					 "EndCommandBuffer() failed: kMaxBundleCountPerCall is exceeded");

	VkCommandBufferHnd secCmdBuffers[kMaxBundleCountPerCall] = {};
	VkCommandBuffer natives[kMaxBundleCountPerCall] = {};
	getSecondaryCmdBuffers(bundleCount, bundles, cmdBuffer->QueueFamIdx, secCmdBuffers);
	vkCmdExecuteCommands(cmdBuffer->Buffer, bundleCount, natives);
	for (TU8 i = 0; i < bundleCount; i++)
	{
		cmdBuffer->ReferencedSecondaryCommandBuffers.PushBack(secCmdBuffers[i]);
		natives[i] = secCmdBuffers[i];
	}
}

void beginRasterpass(TGfxCommandBuffer commandBuffer,
					 unsigned int colorAttachmentCount,
					 const TGfxRasterPassBeginSlotInfo* colorAttachments,
					 const TGfxRasterPassBeginSlotInfo* depthAttachment,

					 TGfxExtension* exts)
{
	getCmdBufferfromHnd(commandBuffer);
#ifdef VULKAN_DEBUGGING
	checkCmdBufferHnd();
#endif

	Texture* baseTexture = nullptr;
	if (depthAttachment)
		baseTexture = GetVkObject(depthAttachment->Texture);
	else
		baseTexture = GetVkObject(colorAttachments[0].Texture);

	VkRenderingAttachmentInfo attachmentInfos[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT + 1] = {};
	for (uint32_t colorSlotIndx = 0; colorSlotIndx < colorAttachmentCount; colorSlotIndx++)
	{
		const auto& colorAttachment = colorAttachments[colorSlotIndx];
		auto texture = GetVkObject(colorAttachment.Texture);

		VkFormat format = GetVkEnum(texture->m_channels);
		void* target = nullptr;
		switch (TGfxGetDataTypeFromChannels(texture->m_channels))
		{
		case TGFX_DATATYPE_U32:
			for (uint32_t i = 0; i < 4; i++)
			{
				attachmentInfos[colorSlotIndx].clearValue.color.uint32[i] =
					*(uint32_t*)&colorAttachments[colorSlotIndx].ClearValue.data[i * 4];
			}
			break;
		case TGFX_DATATYPE_I32:
			for (uint32_t i = 0; i < 4; i++)
			{
				attachmentInfos[colorSlotIndx].clearValue.color.int32[i] =
					*(int32_t*)&colorAttachments[colorSlotIndx].ClearValue.data[i * 4];
			}
			break;
		case TGFX_DATATYPE_F32:
			for (uint32_t i = 0; i < 4; i++)
			{
				attachmentInfos[colorSlotIndx].clearValue.color.float32[i] =
					*(float*)&colorAttachments[colorSlotIndx].ClearValue.data[i * 4];
			}
			break;
		}

		attachmentInfos[colorSlotIndx].imageView = texture->vk_imageView;
		attachmentInfos[colorSlotIndx].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		VkAccessFlags unused = {};
		vk_findImageAccessPattern(colorAttachment.imageAccess, unused, attachmentInfos[colorSlotIndx].imageLayout);
		attachmentInfos[colorSlotIndx].loadOp = GetVkEnum(colorAttachment.LoadOp);
		attachmentInfos[colorSlotIndx].storeOp = GetVkEnum(colorAttachment.StoreOp);
	}
	if (depthAttachment)
	{
		attachmentInfos[colorAttachmentCount].imageView = GetVkObject(depthAttachment->Texture)->vk_imageView;
		VkAccessFlags unused = {};
		vk_findImageAccessPattern(
			depthAttachment->ImageAccess, unused, attachmentInfos[colorAttachmentCount].imageLayout);
		attachmentInfos[colorAttachmentCount].loadOp = GetVkEnum(depthAttachment->LoadOp);
		attachmentInfos[colorAttachmentCount].storeOp = GetVkEnum(depthAttachment->StoreOp);
		attachmentInfos[colorAttachmentCount].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		attachmentInfos[colorAttachmentCount].clearValue.depthStencil.depth = *(float*)depthAttachment->ClearValue.data;
		attachmentInfos[colorAttachmentCount].clearValue.depthStencil.stencil =
			*(uint32_t*)&depthAttachment->ClearValue.data[5];
	}

	VkRenderingInfo ri = {};
	ri.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	ri.colorAttachmentCount = colorAttachmentCount;
	ri.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;
	ri.layerCount = 1;
	ri.pColorAttachments = attachmentInfos;
	ri.pDepthAttachment = depthAttachment ? &attachmentInfos[colorAttachmentCount] : nullptr;
	ri.pStencilAttachment = nullptr;
	ri.renderArea.extent.width = baseTexture->Size.x;
	ri.renderArea.extent.height = baseTexture->Size.y;
	ri.renderArea.offset = {};
	vkCmdBeginRendering(cmdBuffer->Buffer, &ri);
}
void nextRendersubpass(TGfxCommandBuffer commandBuffer)
{
	getCmdBufferfromHnd(commandBuffer);
#ifdef VULKAN_DEBUGGING
	checkCmdBufferHnd();
#endif

	vkCmdNextSubpass(cmdBuffer->Buffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}
void endRasterpass(TGfxCommandBuffer commandBuffer, TGfxExtension* exts)
{
	getCmdBufferfromHnd(commandBuffer);
#ifdef VULKAN_DEBUGGING
	checkCmdBufferHnd();
#endif

	vkCmdEndRendering(cmdBuffer->Buffer);
}

void RendererContext::Initialize()
{
	for (uint32_t i = 0; i < kMaxSemaphoreCountPerSubmit; i++)
		gWaitStagesForPresentOperation[i] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
}

void RendererContext::HookRenderer(ITGfxRenderer* renderer)
{
	renderer->BeginCommandBundle = BeginCommandBundle;
	renderer->FinishCommandBundle = FinishCommandBundle;
	renderer->CreateFences = CreateFences;
	renderer->DestroyCommandBundle = DestroyCommandBundle;
	renderer->GetFenceValue = GetFenceValue;
	renderer->SetFence = SetFence;
	renderer->DestroyFence = DestroyFence;
	renderer->BeginCommandBuffer = beginCommandBuffer;
	renderer->EndCommandBuffer = endCommandBuffer;
	renderer->ExecuteBundles = executeBundles;
	renderer->BeginRasterpass = beginRasterpass;
	renderer->NextSubRasterpass = nextRendersubpass;
	renderer->EndRasterpass = endRasterpass;
	renderer->QueueExecuteCmdBuffers = queueExecuteCmdBuffers;
	renderer->QueueFenceSignalWait = queueFenceWaitSignal;
	renderer->QueueSubmit = QueueSubmit;
	renderer->QueuePresent = queuePresent;

	renderer->CmdBindBindingTables = cmdBindBindingTables;
	renderer->CmdBindIndexBuffer = cmdBindIndexBuffer;
	renderer->CmdBindVertexBuffers = cmdBindVertexBuffers;
	renderer->CmdDrawIndexedDirect = cmdDrawIndexedDirect;
	renderer->CmdExecuteIndirect = cmdExecuteIndirect;
	renderer->CmdDrawNonIndexedDirect = cmdDrawNonIndexedDirect;
	renderer->CmdBarrierTexture = cmdBarrierTexture;
	renderer->CmdBindPipeline = cmdBindPipeline;
	renderer->CmdDispatch = cmdDispatch;
	renderer->CmdSetViewport = cmdSetViewport;
	renderer->CmdSetScissor = cmdSetScissor;
	renderer->CmdSetDepthBounds = cmdSetDepthBounds;
	renderer->CmdCopyBufferToTexture = cmdCopyBufferToTexture;
	renderer->CmdCopyBufferToBuffer = cmdCopyBufferToBuffer;
	renderer->CmdPushConstant = cmdPushConstant;
}

} // namespace Vulkan
} // namespace TGFX