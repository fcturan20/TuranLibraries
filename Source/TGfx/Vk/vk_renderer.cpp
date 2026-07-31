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
#include "vk_queue.h"
#include "vk_resource.h"

namespace TGFX
{
namespace Vulkan
{

VkConstU4 VKCONST_MAXFENCECOUNT_PERSUBMIT = 8, VKCONST_MAXCMDBUFFER_PRIMARY_COUNT = 32;

#define getCmdBufferfromHnd(cmdBufferHnd)                                                                              \
	CommandBuffer* cmdBuffer = GetVkObject(cmdBufferHnd);                                                              \
	GPU* gpu = cmdBuffer->GetGpu();

#define checkCmdBufferHnd()                                                                                            \
	if (gpu == nullptr)                                                                                                \
	{                                                                                                                  \
		vkPrint(11);                                                                                                   \
		return;                                                                                                        \
	}

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
	void cmd_destroy() { virmem::free_page(VK_POINTER_TO_MEMOFFSET(opStates)); }
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

struct cmd
{
	CommandType cmd_type = CommandType::error_2;

	// From https://stackoverflow.com/a/46408751
	template <typename... T>
	static constexpr size_t max_sizeof()
	{
		return std::max({sizeof(T)...});
	}

#define vkCmdStructsLists                                                                                              \
	CmdBarrierTexture, CmdBindBindingTables, CmdBindPipeline, CmdDispatch, CmdBindVertexBuffers, CmdExecuteIndirect,   \
		CmdCopyBufferToTexture, CmdPushConstant
	static constexpr uint32_t maxCmdStructSize = max_sizeof<vkCmdStructsLists>();
	uint8_t cmd_data[maxCmdStructSize] = {};
	cmd() : cmd_type(CommandType::error) {}
};

template <typename T>
T* createCmdStruct(cmd* cmd)
{
	static_assert(T::Type != CommandType::error,
				  "You forgot to specify command type as \"cmd_type\" variable in command struct");
	cmd->cmd_type = T::Type;
	static_assert(cmd::maxCmdStructSize >= sizeof(T), "You forgot to specify the struct in cmd::maxCmdStructSize!");
	*(T*)cmd->cmd_data = T();
	return (T*)cmd->cmd_data;
}

void destroyCmd(cmd& cmd)
{
	switch (cmd.cmd_type)
	{
	case CommandType::ExecuteIndirect: ((CmdExecuteIndirect*)cmd.cmd_data)->cmd_destroy(); break;
	}
}

void CommandBundle::createCmdBuffer(uint64_t cmdCount)
{
	uint32_t allocSize = sizeof(cmd) * cmdCount;
	m_cmds = (cmd*)TCAllocator->Malloc(TCore::GSuperMemoryBlock, allocSize, "Command Bundle buffer");
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

	VkSemaphoreCreateInfo sCi{};
	sCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Timeline semaphores are not allowed on exported semaphores
	VkSemaphoreTypeCreateInfo timelineCreateInfo;
	if (!isShared)
	{
		timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		timelineCreateInfo.pNext = NULL;
		timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		timelineCreateInfo.initialValue = initValue;
		Append_pNext(&sCi, &timelineCreateInfo);
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
		Append_pNext(&sCi, &exportSCi);
	}
	for (uint32_t i = 0; i < count; i++)
	{
		auto fenceObj = GContentManagerContext->Fences.CreateObject(gpu);
		VkFence vkFence{};
		TCORE_SOFT_CHECK(vkCreateFence(gpu->vk_logical, &fCi, nullptr, &vkFence) == VK_SUCCESS,
						 "Failed to create vkFence");
		fenceObj->FenceHnd.Set(vkFence);

		VkSemaphore vkSemaphore{};
		TCORE_SOFT_CHECK(vkCreateSemaphore(gpu->vk_logical, &sCi, nullptr, &vkSemaphore) == VK_SUCCESS,
						 "Failed to create vkSemaphore");
		fenceObj->SemaphoreHnd.Set(vkSemaphore);
		fenceList[i] = GetOpaqueHandle(fenceObj);
	}
}

void DestroyFence(TGfxFence fence)
{
	auto vkFence = GetVkObject(fence);
	GPU* gpu = vkFence->GetGpu();
	vkDestroySemaphore(gpu->vk_logical, vkFence->SemaphoreHnd, nullptr);
	vkFence->SemaphoreHnd.SetAsDead();
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
		{
			rInfo.colorAttachmentCount++;
		}
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
			executeCmd(cb, bundle, bundle->m_cmds[cmdIdx]);
		if (vkEndCommandBuffer(cb) != VK_SUCCESS)
			vkPrint(16, "at vkEndCommandBuffer()");
	}
}
void DestroyCommandBundle(TGfxCommandBundle hnd)
{
	CommandBundle* bundle = GetVkObject(hnd);

	for (uint32_t i = 0; i < bundle->m_cmdCount; i++)
		destroyCmd(bundle->m_cmds[i]);

	virmem::free_page(VK_POINTER_TO_MEMOFFSET(bundle->m_cmds));
	static constexpr TU8 cmdBufferCount =
		sizeof(bundle->SecondaryCommandBuffers) / sizeof(bundle->SecondaryCommandBuffers[0]);
	for (uint32_t i = 0; i < cmdBufferCount; i++)
	{
		VkCommandBuffer cmdBuffer = bundle->SecondaryCommandBuffers[i];
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
	cmd->opStates = (CmdExecuteIndirect::IndirectOperationState*)VK_MEMOFFSET_TO_POINTER(
		virmem::allocatePage(allocSize, &allocSize));
	vm->commit(cmd->opStates, allocSize);
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
	renderer->QueueSubmit = queueSubmit;
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