/*
  This file is responsible for managing TGFX Command Bundle.
  TGFX Command Bundle = Secondary Command Buffers.
*/
#include "vk_renderer.h"

#include <algorithm>
#include <numeric>
#include <utility>

#include "TGfxRenderer.h"
#include "vk_contentmanager.h"
#include "vk_core.h"
#include "vk_predefinitions.h"
#include "vk_queue.h"
#include "vk_resource.h"
#include "vkext_timelineSemaphore.h"

namespace TGFX
{
namespace Vulkan
{

struct cmd;
struct CMDBUNDLE_VKOBJ
{
	VkConstHndType HANDLETYPE = VKHANDLETYPEs::CMDBUNDLE;
	static uint16_t GET_EXTRAFLAGS(CMDBUNDLE_VKOBJ* obj) { return 0; }

	// Command Buffer States
	VkPipeline activePipeline = {};
	// To check pipeline compatibility
	VkPipelineLayout activePipelineLayout = {};
	VkDescriptorSetLayout activeDescSets[kMaxDescSetPerList] = {};
	uint8_t callBuffer[128] = {}; // CallBuffer = Push Constants = 128 byte

	GPU_VKOBJ* m_gpu = {};
	cmd* m_cmds = {};
	uint64_t m_cmdCount = 0;
	TGfxPipeline m_defaultPipeline = {};
	VkPipelineBindPoint bindPoint = {};
	VkCommandBuffer cmdBuffers[kMaxQueueFamilyCountPerGpu] = {};
	cmdPool_vk* cmdPools[kMaxQueueFamilyCountPerGpu] = {};

	void createCmdBuffer(uint64_t cmdCount);
};

#define vkEnumType_cmdType() uint32_t
enum class cmdType : vkEnumType_cmdType(){
	error = VK_PRIM_MIN(vkEnumType_cmdType()), // cmdType neither should be 0 nor 255
	bindBindingTables,
	bindVertexBuffers,
	bindIndexBuffer,
	setDepthBounds,
	setViewport,
	setScissor,
	drawNonIndexedDirect,
	drawIndexedDirect,
	executeIndirect,
	barrierTexture,
	barrierBuffer,
	bindPipeline,
	dispatch,
	copyBufferToTexture,
	copyBufferToBuffer,
	pushConstant,
	error_2 = VK_PRIM_MAX(vkEnumType_cmdType())};
// Template struct for new cmd structs
// Then specify the struct in vkCmdStructsLists
struct vkCmdStruct_example
{
	// Necessary template variables & functions should have "cmd_" prefix
	static constexpr cmdType type = cmdType::error;
	vkCmdStruct_example() = default;
	void cmd_execute(CMDBUNDLE_VKOBJ* cmdBundle) {}
};

struct vkCmdStruct_barrierTexture
{
	static constexpr cmdType cmd_type = cmdType::barrierTexture;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle)
	{
		vkCmdPipelineBarrier(cb,
							 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
							 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
							 VK_DEPENDENCY_BY_REGION_BIT,
							 0,
							 nullptr,
							 0,
							 nullptr,
							 1,
							 &m_imBar);
	}

	// Command specific variables should have "m_" prefix
	VkImageMemoryBarrier m_imBar = {};
};

struct vkCmdStruct_bindBindingTables
{
	static constexpr cmdType cmd_type = cmdType::bindBindingTables;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle)
	{
		VkDescriptorSet sets[kMaxDescSetPerList] = {};
		for (uint32_t i = 0; i < m_setCount; i++)
		{
			BINDINGTABLEINST_VKOBJ* table = getOBJ<BINDINGTABLEINST_VKOBJ>(tables[i]);
			sets[i] = table->set;
			cmdBundle->activeDescSets[i] = table->layout;
		}
		vkCmdBindDescriptorSets(
			cb, bindPoint, cmdBundle->activePipelineLayout, m_firstSetIndx, m_setCount, sets, 0, nullptr);
	}
	VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
	TGfxBindingTable tables[kMaxDescSetPerList] = {};
	uint32_t m_setCount = 0, m_firstSetIndx = 0;
};

struct vkCmdStruct_bindPipeline
{
	static constexpr cmdType cmd_type = cmdType::bindPipeline;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle)
	{
		vkCmdBindPipeline(cb, bindPoint, pipeline);
		cmdBundle->activePipeline = pipeline;
		cmdBundle->activePipelineLayout = pipelineLayout;
	}
	VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
	VkPipeline pipeline = {};
	VkPipelineLayout pipelineLayout = {};
};

struct vkCmdStruct_dispatch
{
	static constexpr cmdType cmd_type = cmdType::dispatch;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle)
	{
		vkCmdDispatch(cb, m_dispatchSize.x, m_dispatchSize.y, m_dispatchSize.z);
	};
	TGfxUVec3 m_dispatchSize;
};

struct vkCmdStruct_setViewport
{
	static constexpr cmdType cmd_type = cmdType::setViewport;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) { vkCmdSetViewport(cb, 0, 1, &viewport); };

	VkViewport viewport = {};
};

struct vkCmdStruct_setScissor
{
	static constexpr cmdType cmd_type = cmdType::setScissor;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) { vkCmdSetScissor(cb, 0, 1, &rect); };
	VkRect2D rect = {};
};

struct vkCmdStruct_drawNonIndexedDirect
{
	static constexpr cmdType cmd_type = cmdType::drawNonIndexedDirect;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle)
	{
		vkCmdDraw(cb, vertexCount, instanceCount, firstVertex, firstInstance);
	};
	uint32_t vertexCount = {}, instanceCount = {}, firstVertex = {}, firstInstance = {};
};

struct vkCmdStruct_drawIndexedDirect
{
	static constexpr cmdType cmd_type = cmdType::drawIndexedDirect;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle)
	{
		vkCmdDrawIndexed(cb, indxCount, instanceCount, firstIndx, vertexOffset, firstInstance);
	};
	uint32_t indxCount = {}, instanceCount = {}, firstIndx = {}, firstInstance = {};
	int32_t vertexOffset = {};
};

struct vkCmdStruct_bindIndexBuffer
{
	static constexpr cmdType cmd_type = cmdType::bindIndexBuffer;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle)
	{
		vkCmdBindIndexBuffer(cb, buffer, offset, indexType);
	};
	VkBuffer buffer;
	VkDeviceSize offset;
	VkIndexType indexType;
};

struct vkCmdStruct_setDepthBounds
{
	static constexpr cmdType cmd_type = cmdType::setDepthBounds;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) { vkCmdSetDepthBounds(cb, min, max); };
	float min = 0.0f, max = 1.0f;
};

struct vkCmdStruct_copyBufferToTexture
{
	static constexpr cmdType cmd_type = cmdType::copyBufferToTexture;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle)
	{
		vkCmdCopyBufferToImage(cb, src, dst, dstImageLayout, 1, &copy);
	};
	VkBuffer src;
	VkImage dst;
	VkImageLayout dstImageLayout;
	VkBufferImageCopy copy;
};

struct vkCmdStruct_bindVertexBuffers
{
	static constexpr cmdType cmd_type = cmdType::bindVertexBuffers;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle)
	{
		vkCmdBindVertexBuffers(cb, firstBinding, bindingCount, buffers, bufferOffsets);
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
struct vkCmdStruct_executeIndirect
{
	static constexpr cmdType cmd_type = cmdType::executeIndirect;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle)
	{
		VkDeviceSize activeOffset = bufferOffset;
		for (uint32_t stateIndx = 0; stateIndx < opStateCount; stateIndx++)
		{
			uint64_t loopCount = 1, drawCount = opStates[stateIndx].opCount;
			auto opType = opStates[stateIndx].opType;
			uint64_t indirectArgumentDataSize = findIndirectOperationDataSize(opStates[stateIndx].opType);
			// If GPU doesn't support multiDrawIndirect or execute type is compute, call same VkCmd*
			// multiple times with incrementing offsets
			if (!cmdBundle->m_gpu->vk_featuresDev.features.multiDrawIndirect ||
				opType == TGFX_INDIRECTOPERATIONTYPE_DISPATCH)
			{
				loopCount = opStates[stateIndx].opCount;
				drawCount = 1;
			}
			for (uint32_t loopIndx = 0; loopIndx < loopCount; loopIndx++)
			{
				switch (opType)
				{
				case TGFX_INDIRECTOPERATIONTYPE_DRAWNONINDEXED:
					vkCmdDrawIndirect(cb, buffer, activeOffset, drawCount, indirectArgumentDataSize);
					break;
				case TGFX_INDIRECTOPERATIONTYPE_DRAWINDEXED:
					vkCmdDrawIndexedIndirect(cb, buffer, activeOffset, drawCount, indirectArgumentDataSize);
					break;
				case TGFX_INDIRECTOPERATIONTYPE_DISPATCH: vkCmdDispatchIndirect(cb, buffer, activeOffset); break;
				default: vkPrint(59); return;
				}
				activeOffset += indirectArgumentDataSize * drawCount;
			}
		}
	};
	void cmd_destroy() { virmem::free_page(VK_POINTER_TO_MEMOFFSET(opStates)); }
	struct indirectOperationState
	{
		uint32_t opCount;
		TGfxIndirectOperationType opType;
	};
	indirectOperationState* opStates = {};
	uint32_t opStateCount = 0;
	VkBuffer buffer;
	VkDeviceSize bufferOffset;
};

struct vkCmdStruct_copyBufferToBuffer
{
	static constexpr cmdType cmd_type = cmdType::copyBufferToBuffer;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle)
	{
		vkCmdCopyBuffer(cb, srcBuffer, dstBuffer, 1, &bufCopy);
	};

	VkBuffer srcBuffer = {}, dstBuffer = {};
	VkBufferCopy bufCopy = {};
};

struct vkCmdStruct_pushConstant
{
	static constexpr cmdType cmd_type = cmdType::pushConstant;

	void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle)
	{
		vkCmdPushConstants(cb, cmdBundle->activePipelineLayout, VK_SHADER_STAGE_ALL, offset, size, data);
	};

	unsigned char offset, size, data[128];
};

struct cmd
{
	cmdType cmd_type = cmdType::error_2;

	// From https://stackoverflow.com/a/46408751
	template <typename... T>
	static constexpr size_t max_sizeof()
	{
		return std::max({sizeof(T)...});
	}

#define vkCmdStructsLists                                                                                              \
	vkCmdStruct_example, vkCmdStruct_barrierTexture, vkCmdStruct_bindBindingTables, vkCmdStruct_bindPipeline,          \
		vkCmdStruct_dispatch, vkCmdStruct_bindVertexBuffers, vkCmdStruct_executeIndirect,                              \
		vkCmdStruct_copyBufferToTexture, vkCmdStruct_pushConstant
	static constexpr uint32_t maxCmdStructSize = max_sizeof<vkCmdStructsLists>();
	uint8_t cmd_data[maxCmdStructSize] = {};
	cmd() : cmd_type(cmdType::error) {}
};

template <typename T>
T* createCmdStruct(cmd* cmd)
{
	static_assert(T::cmd_type != cmdType::error,
				  "You forgot to specify command type as \"cmd_type\" variable in command struct");
	cmd->cmd_type = T::cmd_type;
	static_assert(cmd::maxCmdStructSize >= sizeof(T), "You forgot to specify the struct in cmd::maxCmdStructSize!");
	*(T*)cmd->cmd_data = T();
	return (T*)cmd->cmd_data;
}

void executeCmd(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* bundle, const cmd& cmd)
{
	switch (cmd.cmd_type)
	{
	case cmdType::barrierTexture: ((vkCmdStruct_barrierTexture*)cmd.cmd_data)->cmd_execute(cb, bundle); break;
	case cmdType::bindBindingTables: ((vkCmdStruct_bindBindingTables*)cmd.cmd_data)->cmd_execute(cb, bundle); break;
	case cmdType::bindPipeline: ((vkCmdStruct_bindPipeline*)cmd.cmd_data)->cmd_execute(cb, bundle); break;
	case cmdType::dispatch: ((vkCmdStruct_dispatch*)cmd.cmd_data)->cmd_execute(cb, bundle); break;
	case cmdType::setScissor: ((vkCmdStruct_setScissor*)cmd.cmd_data)->cmd_execute(cb, bundle); break;
	case cmdType::setViewport: ((vkCmdStruct_setViewport*)cmd.cmd_data)->cmd_execute(cb, bundle); break;
	case cmdType::drawNonIndexedDirect:
		((vkCmdStruct_drawNonIndexedDirect*)cmd.cmd_data)->cmd_execute(cb, bundle);
		break;
	case cmdType::setDepthBounds: ((vkCmdStruct_setDepthBounds*)cmd.cmd_data)->cmd_execute(cb, bundle); break;
	case cmdType::copyBufferToTexture: ((vkCmdStruct_copyBufferToTexture*)cmd.cmd_data)->cmd_execute(cb, bundle); break;
	case cmdType::bindVertexBuffers: ((vkCmdStruct_bindVertexBuffers*)cmd.cmd_data)->cmd_execute(cb, bundle); break;
	case cmdType::bindIndexBuffer: ((vkCmdStruct_bindIndexBuffer*)cmd.cmd_data)->cmd_execute(cb, bundle); break;
	case cmdType::drawIndexedDirect: ((vkCmdStruct_drawIndexedDirect*)cmd.cmd_data)->cmd_execute(cb, bundle); break;
	case cmdType::executeIndirect: ((vkCmdStruct_executeIndirect*)cmd.cmd_data)->cmd_execute(cb, bundle); break;
	case cmdType::copyBufferToBuffer: ((vkCmdStruct_copyBufferToBuffer*)cmd.cmd_data)->cmd_execute(cb, bundle); break;
	case cmdType::pushConstant: ((vkCmdStruct_pushConstant*)cmd.cmd_data)->cmd_execute(cb, bundle); break;
	case cmdType::error:
	case cmdType::error_2: vkPrint(60, "one of the commands in the buffer is not used"); break;
	default: vkPrint(16, "invalid command type in executeCmd()");
	}
}
void destroyCmd(cmd& cmd)
{
	switch (cmd.cmd_type)
	{
	case cmdType::executeIndirect: ((vkCmdStruct_executeIndirect*)cmd.cmd_data)->cmd_destroy(); break;
	}
}

void CMDBUNDLE_VKOBJ::createCmdBuffer(uint64_t cmdCount)
{
	uint32_t allocSize = sizeof(cmd) * cmdCount;
	m_cmds = (cmd*)VK_MEMOFFSET_TO_POINTER(virmem::allocatePage(allocSize));
	vm->commit(m_cmds, allocSize);
	m_cmdCount = cmdCount;
	for (uint32_t i = 0; i < cmdCount; i++)
	{
		m_cmds[i] = {};
	}
}

struct renderer_private
{
	VK_LINEAR_OBJARRAY<CMDBUNDLE_VKOBJ, TGfxCommandBundle, 1 << 20> m_cmdBundles;
};
renderer_private* hiddenRenderer = nullptr;

// Synchronization Functions

void CreateFences(TGfxGpu gpu, TU4 count, TU8 initValue, TGfxFence* fenceList)
{
	GPU_VKOBJ* GPU = getOBJ<GPU_VKOBJ>(gpu);
	for (uint32_t i = 0; i < count; i++)
	{
		fenceList[i] = createTGFXFence(GPU, initValue);
	}
}
void DestroyFence(TGfxFence fence)
{
	FENCE_VKOBJ* vkFence = getOBJ<FENCE_VKOBJ>(fence);
	GPU_VKOBJ* gpu = core_vk->getGPU(vkFence->m_gpuIndx);
	vkDestroySemaphore(gpu->vk_logical, vkFence->timelineSemaphore, nullptr);
	vkext_timelineSemaphore* ext = (vkext_timelineSemaphore*)gpu->ext()->m_exts[IVkExt::TimelineSemaphoresExtension];
	ext->fences.destroyObj(ext->fences.getINDEXbyOBJ(vkFence));
}

// Command Bundle Functions
////////////////////////////

TGfxCommandBundle BeginCommandBundle(TGfxGpu gpu,
									 unsigned long long maxCmdCount,
									 TGfxPipeline defaultPipeline,
									 unsigned int extCount,
									 TGfxExtension* exts)
{
	VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
	cmdPool_vk* cmdPool;
	GPU_VKOBJ* GPU = getOBJ<GPU_VKOBJ>(gpu);
	if (!GPU)
	{
		return nullptr;
	}
	CMDBUNDLE_VKOBJ* cmdBundle = hiddenRenderer->m_cmdBundles.create_OBJ();
	for (uint32_t i = 0; i < kMaxQueueFamilyCountPerGpu; i++)
	{
		cmdBundle->cmdBuffers[i] = {};
	}
	cmdBundle->m_gpu = GPU;
	cmdBundle->createCmdBuffer(maxCmdCount);
	cmdBundle->m_defaultPipeline = defaultPipeline;
	if (defaultPipeline)
	{
		PIPELINE_VKOBJ* pipe = getOBJ<PIPELINE_VKOBJ>(defaultPipeline);
		cmdBundle->bindPoint = pipe->vk_type;
	}
	else
	{
		cmdBundle->bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	}

	return getHANDLE<TGfxCommandBundle>(cmdBundle);
}
void FinishCommandBundle(TGfxCommandBundle bndl, TU4 extCount, TGfxExtension* exts)
{
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);

	// MOVE THIS RECORDING STAGE BECAUSE FRAMEBUFFER WILL RECORD ALREADY!
}
void destroyCommandBundle(TGfxCommandBundle hnd)
{
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(hnd);

	for (uint32_t i = 0; i < bundle->m_cmdCount; i++)
	{
		destroyCmd(bundle->m_cmds[i]);
	}
	virmem::free_page(VK_POINTER_TO_MEMOFFSET(bundle->m_cmds));
	const uint32_t cmdBufferCount = sizeof(bundle->cmdBuffers) / sizeof(bundle->cmdBuffers[0]);
	for (uint32_t i = 0; i < cmdBufferCount; i++)
	{
		VkCommandBuffer cmdBuffer = bundle->cmdBuffers[i];
		if (cmdBuffer != VK_NULL_HANDLE)
		{
			freeCmdBuffer(bundle->cmdPools[i], cmdBuffer);
		}
	}
}

void cmdBindBindingTables(TGfxCommandBundle bndl,
						  unsigned long long sortKey,
						  unsigned int firstSetIndx,
						  unsigned int bindingTableCount,
						  TGfxBindingTable const* bindingTables,
						  TGfxPipelineType pipelineType)
{
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmd = createCmdStruct<vkCmdStruct_bindBindingTables>(&bundle->m_cmds[sortKey]);

	{
		uint32_t descSetLimit =
			glm::min(kMaxDescSetPerList, bundle->m_gpu->vk_propsDev.properties.limits.maxBoundDescriptorSets);
		if (bindingTableCount > descSetLimit)
		{
			vkPrint(22, "Max binding table count is exceeded!");
			return;
		}
		for (uint32_t i = 0; i < bindingTableCount; i++)
		{
			BINDINGTABLEINST_VKOBJ* bindingTable = getOBJ<BINDINGTABLEINST_VKOBJ>(bindingTables[i]);
			assert(bindingTable && "Binding table isn't found!");
			cmd->tables[cmd->m_setCount++] = bindingTables[i];
		}
	}

	cmd->bindPoint = findPipelineBindPoint(pipelineType);
	cmd->m_firstSetIndx = firstSetIndx;
}
void cmdBindPipeline(TGfxCommandBundle bndl, unsigned long long sortKey, TGfxPipeline pipeline)
{
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmd = createCmdStruct<vkCmdStruct_bindPipeline>(&bundle->m_cmds[sortKey]);
	PIPELINE_VKOBJ* pipe = getOBJ<PIPELINE_VKOBJ>(pipeline);

	if (pipe->vk_type != bundle->bindPoint)
	{
		vkPrint(61);
	}
	cmd->bindPoint = pipe->vk_type;
	cmd->pipeline = pipe->vk_object;
	cmd->pipelineLayout = pipe->vk_layout;
}
void cmdSetViewport(TGfxCommandBundle bndl, unsigned long long sortKey, const TGfxViewportInfo* viewport)
{
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmd = createCmdStruct<vkCmdStruct_setViewport>(&bundle->m_cmds[sortKey]);

	cmd->viewport.x = viewport->TopLeftCorner.x;
	cmd->viewport.y = viewport->TopLeftCorner.y;
	cmd->viewport.width = viewport->Size.x;
	cmd->viewport.height = viewport->Size.y;
	cmd->viewport.minDepth = viewport->DepthMinMax.x;
	cmd->viewport.maxDepth = viewport->DepthMinMax.y;
}
void cmdSetScissor(TGfxCommandBundle bndl, TU8 sortKey, TGfxIVec2 offset, TGfxUVec2 size)
{
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmd = createCmdStruct<vkCmdStruct_setScissor>(&bundle->m_cmds[sortKey]);

	cmd->rect.offset.x = offset->x;
	cmd->rect.offset.y = offset->x;
	cmd->rect.extent.width = size->x;
	cmd->rect.extent.height = size->y;
}
void cmdSetDepthBounds(TGfxCommandBundle bndl, unsigned long long sortKey, float min, float max)
{
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmd = createCmdStruct<vkCmdStruct_setDepthBounds>(&bundle->m_cmds[sortKey]);

	cmd->min = min;
	cmd->max = max;
};
void cmdBindVertexBuffers(TGfxCommandBundle bndl,
						  unsigned long long sortKey,
						  unsigned int firstBinding,
						  unsigned int bindingCount,
						  TGfxBuffer const* buffers,
						  const unsigned long long* offsets)
{
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmd = createCmdStruct<vkCmdStruct_bindVertexBuffers>(&bundle->m_cmds[sortKey]);

	cmd->firstBinding = firstBinding;
	cmd->bindingCount = bindingCount;
	for (uint32_t i = 0; i < bindingCount; i++)
	{
		cmd->bufferOffsets[i] = offsets[i];
		cmd->buffers[i] = getOBJ<BUFFER_VKOBJ>(buffers[i])->vk_buffer;
	}
}
void cmdBindIndexBuffer(TGfxCommandBundle bndl,
						unsigned long long sortKey,
						TGfxBuffer buffer,
						unsigned long long offset,
						unsigned char IndexTypeSize)
{
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmd = createCmdStruct<vkCmdStruct_bindIndexBuffer>(&bundle->m_cmds[sortKey]);

	cmd->buffer = getOBJ<BUFFER_VKOBJ>(buffer)->vk_buffer;
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
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmd = createCmdStruct<vkCmdStruct_drawNonIndexedDirect>(&bundle->m_cmds[sortKey]);

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
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmd = createCmdStruct<vkCmdStruct_drawIndexedDirect>(&bundle->m_cmds[sortKey]);

	cmd->firstIndx = firstIndex;
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
						unsigned int extCount,
						TGfxExtension* exts)
{
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmd = createCmdStruct<vkCmdStruct_executeIndirect>(&bundle->m_cmds[sortKey]);

	// Find operation state count
	for (uint32_t i = 0; i < operationCount;)
	{
		auto opType = operationTypes[i];
		for (; i < operationCount && opType == operationTypes[i]; i++)
		{
		}
		cmd->opStateCount++;
	}
	uint32_t allocSize = sizeof(vkCmdStruct_executeIndirect::indirectOperationState) * cmd->opStateCount;
	cmd->opStates = (vkCmdStruct_executeIndirect::indirectOperationState*)VK_MEMOFFSET_TO_POINTER(
		virmem::allocatePage(allocSize, &allocSize));
	vm->commit(cmd->opStates, allocSize);
	for (uint32_t i = 0, stateIndx = 0; i < operationCount;)
	{
		auto opType = operationTypes[i];
		cmd->opStates[stateIndx].opType = opType;
		cmd->opStates[stateIndx].opCount = 0;
		for (; i < operationCount && opType == operationTypes[i]; i++, cmd->opStates[stateIndx].opCount++)
		{
		}
		stateIndx++;
	}
	cmd->buffer = getOBJ<BUFFER_VKOBJ>(dataBffr)->vk_buffer;
	cmd->bufferOffset = indirectBufferOffset;
}
void cmdBarrierTexture(TGfxCommandBundle bndl,
					   TU8 key,
					   TGfxTexture i_texture,
					   TGfxImageAccess lastAccess,
					   TGfxImageAccess nextAccess,
					   TU4 extCount,
					   TGfxExtension* exts)
{
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmdBar = createCmdStruct<vkCmdStruct_barrierTexture>(&bundle->m_cmds[key]);
	cmdBar->m_imBar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	cmdBar->m_imBar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	cmdBar->m_imBar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	cmdBar->m_imBar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	cmdBar->m_imBar.subresourceRange.baseArrayLayer = 0;
	cmdBar->m_imBar.subresourceRange.baseMipLevel = 0;
	cmdBar->m_imBar.subresourceRange.layerCount = 1;
	cmdBar->m_imBar.subresourceRange.levelCount = 1;
	TEXTURE_VKOBJ* texture = getOBJ<TEXTURE_VKOBJ>(i_texture);
	cmdBar->m_imBar.image = texture->image;
	findImageAccessPattern(lastAccess, cmdBar->m_imBar.srcAccessMask, cmdBar->m_imBar.oldLayout);
	findImageAccessPattern(nextAccess, cmdBar->m_imBar.dstAccessMask, cmdBar->m_imBar.newLayout);
	cmdBar->m_imBar.pNext = nullptr;
}
void cmdDispatch(TGfxCommandBundle bndl, unsigned long long key, const TGfxUVec3 dispatchSize)
{
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmd = createCmdStruct<vkCmdStruct_dispatch>(&bundle->m_cmds[key]);

	cmd->m_dispatchSize = dispatchSize;
}
void cmdCopyBufferToTexture(TGfxCommandBundle bndl,
							unsigned long long key,
							TGfxBuffer srcBuffer,
							unsigned long long bufferOffset,
							TGfxTexture dstTexture,
							TGfxImageAccess lastAccess,
							unsigned int extCount,
							TGfxExtension* exts)
{
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmd = createCmdStruct<vkCmdStruct_copyBufferToTexture>(&bundle->m_cmds[key]);

	BUFFER_VKOBJ* buffer = getOBJ<BUFFER_VKOBJ>(srcBuffer);
	TEXTURE_VKOBJ* texture = getOBJ<TEXTURE_VKOBJ>(dstTexture);

	cmd->src = buffer->vk_buffer;
	cmd->dst = texture->vk_image;
	VkAccessFlags flag;
	findImageAccessPattern(lastAccess, flag, cmd->dstImageLayout);
	cmd->copy.imageOffset = {};
	cmd->copy.imageExtent.width = texture->m_width;
	cmd->copy.imageExtent.height = texture->m_height;
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
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmd = createCmdStruct<vkCmdStruct_copyBufferToBuffer>(&bundle->m_cmds[key]);

	cmd->bufCopy.dstOffset = dstOffset;
	cmd->bufCopy.size = size;
	cmd->bufCopy.srcOffset = srcOffset;
	cmd->dstBuffer = getOBJ<BUFFER_VKOBJ>(dstBuffer)->vk_buffer;
	cmd->srcBuffer = getOBJ<BUFFER_VKOBJ>(srcBuffer)->vk_buffer;
}
void cmdPushConstant(
	TGfxCommandBundle bndl, unsigned long long key, unsigned char offset, unsigned char size, const void* d)
{
	CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
	auto* cmd = createCmdStruct<vkCmdStruct_pushConstant>(&bundle->m_cmds[key]);
	size = std::min(128u, uint32_t(size));
	cmd->size = size;
	cmd->offset = offset;
	memcpy(cmd->data, d, size);
}

void getSecondaryCmdBuffers(unsigned int cmdBundleCount,
							TGfxCommandBundle const* cmdBundles,
							uint32_t queueFamIndx,
							VkCommandBuffer* secondaryCmdBuffers)
{
	uint32_t bundleCount = 0;
	for (uint32_t bundleListIndx = 0; bundleListIndx < cmdBundleCount; bundleListIndx++)
	{
		if (!cmdBundles[bundleListIndx])
		{
			continue;
		}
		const TGfxCommandBundle bundleHnd = cmdBundles[bundleListIndx];
		CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bundleHnd);

		if (!bundle || bundleCount >= VKCONST_MAXCMDBUNDLE_PERCALL)
		{
			continue;
		}

		VkCommandBuffer& vkCmdBuffer = bundle->cmdBuffers[queueFamIndx];

		// Command bundle isn't used in this queue fam, so use it
		if (vkCmdBuffer == VK_NULL_HANDLE)
		{
			allocateCmdBuffer(getQueueFam(bundle->m_gpu, queueFamIndx),
							  VK_COMMAND_BUFFER_LEVEL_SECONDARY,
							  bundle->cmdPools[queueFamIndx],
							  &vkCmdBuffer,
							  1);

			VkCommandBufferInheritanceRenderingInfo rInfo = {};
			VkCommandBufferBeginInfo bi = {};
			bi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			VkCommandBufferInheritanceInfo secInfo = {};
			secInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			if (bundle->bindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS)
			{
				rInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
				rInfo.viewMask = 0;
				PIPELINE_VKOBJ* defaultPipe = getOBJ<PIPELINE_VKOBJ>(bundle->m_defaultPipeline);
				rInfo.pColorAttachmentFormats = defaultPipe->vk_colorAttachmentFormats;
				while (rInfo.colorAttachmentCount < TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT &&
					   rInfo.pColorAttachmentFormats[rInfo.colorAttachmentCount] != VK_FORMAT_UNDEFINED)
				{
					rInfo.colorAttachmentCount++;
				}
				rInfo.depthAttachmentFormat = defaultPipe->vk_depthAttachmentFormat;
				rInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
				secInfo.pNext = &rInfo;
				bi.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
			}

			bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			bi.pInheritanceInfo = &secInfo;
			if (vkBeginCommandBuffer(vkCmdBuffer, &bi) != VK_SUCCESS)
			{
				vkPrint(16, "at vkBeginCommandBuffer()");
				return;
			}
			if (bundle->m_defaultPipeline)
			{
				PIPELINE_VKOBJ* pipe = getOBJ<PIPELINE_VKOBJ>(bundle->m_defaultPipeline);
				vkCmdBindPipeline(vkCmdBuffer, pipe->vk_type, pipe->vk_object);
				bundle->activePipeline = pipe->vk_object;
				bundle->activePipelineLayout = pipe->vk_layout;
			}
			for (uint64_t cmdIndx = 0; cmdIndx < bundle->m_cmdCount; cmdIndx++)
			{
				executeCmd(vkCmdBuffer, bundle, bundle->m_cmds[cmdIndx]);
			}
			if (vkEndCommandBuffer(vkCmdBuffer) != VK_SUCCESS)
			{
				vkPrint(16, "at vkEndCommandBuffer()");
			}
		}

		secondaryCmdBuffers[bundleCount++] = vkCmdBuffer;
	}
}

void BindFunctions(ITGfxRenderer* renderer)
{
	renderer->BeginCommandBuffer = BeginCommandBundle;
	renderer->FinishCommandBundle = FinishCommandBundle;
	renderer->CreateFences = CreateFences;
	renderer->DestroyCommandBundle = destroyCommandBundle;
	renderer->GetFenceValue = GetFenceValue;
	renderer->SetFence = SetFence;
	renderer->DestroyFence = DestroyFence;

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

void initRenderer()
{
	set_VkRenderer_funcPtrs();
	VKGLOBAL_VIRMEM_RENDERER = virmem::allocate_dynamicmem(sizeof(renderer_private));
	hiddenRenderer = new (VKGLOBAL_VIRMEM_RENDERER) renderer_private;
}

} // namespace Vulkan
} // namespace TGFX