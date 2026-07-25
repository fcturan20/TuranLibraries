#pragma once
#include <TGfxDeclarations.h>
#include <TGfxStructs.h>
TCORE_BEGIN_C_LINKAGE

typedef struct ITGfxRenderer
{
	// Command Buffer Functions
	////////////////////////////

	// Command buffers are one-time only buffers
	//  so when you submit for execution, they'll be freed after their execution
	// Extensions: Storage command buffers
	TGfxCommandBuffer (*BeginCommandBuffer)(TGfxQueue queue,

											TGfxExtension* exts);
	void (*EndCommandBuffer)(TGfxCommandBuffer commandBuffer);
	// In a Rendersubpass: All bundles should be created with the rendersubpass' handle
	// Outside: All bundles should be created with rendersubpass as NULL.
	// All bundles should be from the compatible queue with the cmdBuffer's queue
	void (*ExecuteBundles)(TGfxCommandBuffer commandBuffer,
						   TU4 bundleCount,
						   TGfxCommandBundle const* bundles,
						   TGfxExtension* exts);
	void (*BeginRasterpass)(TGfxCommandBuffer commandBuffer,
							TU4 colorAttachmentCount,
							const TGfxRasterPassBeginSlotInfo* colorAttachments,
							const TGfxRasterPassBeginSlotInfo* depthAttachment,
							TGfxExtension* exts);
	void (*NextSubRasterpass)(TGfxCommandBuffer commandBuffer);
	void (*EndRasterpass)(TGfxCommandBuffer commandBuffer, TGfxExtension* exts);

	// Synchronization Functions

	// @param fenceCount: Fence count that will be created
	// @param initValue: Set initial value of all fences
	// @param isShared: Set to use across processes
	void (*CreateFences)(TGfxGpu gpu, TU4 count, TU8 initValue, TBool isShared, TGfxFence* fenceList);
	void (*DestroyFence)(TGfxFence fence);
	// CPU side fence value change
	TCResult (*SetFence)(TGfxFence fence, TU8 value);
	TCResult (*GetFenceValue)(TGfxFence fence, TU8* value);

	// Queue Functions
	//  These functions're executed sequentially (implicitly synchronized) on GPU queue
	///////////////////////////

	// All command buffers should be from the same queue
	void (*QueueFenceSignalWait)(TGfxQueue queue,
								 TU4 waitsCount,
								 TGfxFence const* waitFences,
								 const unsigned long long* waitValues,
								 TU4 signalsCount,
								 TGfxFence const* signalFences,
								 const unsigned long long* signalValues);
	void (*QueueExecuteCmdBuffers)(TGfxQueue queue,
								   TU4 cmdBufferCount,
								   TGfxCommandBuffer const* cmdBuffers,
								   TGfxExtension* exts);
	void (*QueuePresent)(TGfxQueue queue, TU4 windowCount, TGfxSwapchain const* windowlist);
	// Submit queue operations to GPU.
	// You should call this right before changing Queue Operation type.
	// Queue Operation Types: ExecuteCmdBuffers, Present, BindSparse (optional, in future)
	// FenceSignalWait operation doesn't have a type, it can work with all of these operations.
	// Because operations are sent through PCI-E, it costs a lot. Profile this on your device and
	//  design your renderer according to it.
	void (*QueueSubmit)(TGfxQueue queue);

	// Command Bundle Functions
	////////////////////////////

	// @param maxCmdCount: Backend allocates a command buffer to store commands
	// Every cmdXXX call's "key" argument should be [0,maxCmdCount-1].
	TGfxCommandBundle (*BeginCommandBundle)(TGfxGpu gpu,
											TSize maxCmdCount,
											TGfxPipeline defaultPipeline,
											TGfxExtension* exts);
	void (*FinishCommandBundle)(TGfxCommandBundle bundle, TGfxExtension* exts);
	// If you won't execute same bundle later, destroy to allow backend
	//   implementation to optimize memory usage
	void (*DestroyCommandBundle)(TGfxCommandBundle hnd);
	void (*CmdBindBindingTables)(TGfxCommandBundle bundle,
								 TSize sortKey,
								 TU4 firstSetIndx,
								 TU4 bindingTableCount,
								 TGfxBindingTable const* bindingTables,
								 TGfxPipelineType pipeline);
	void (*CmdBindPipeline)(TGfxCommandBundle bundle, TSize sortKey, TGfxPipeline pipeline);
	// For devices that doesn't allow storage buffers to store vertex buffers,
	//   this function is needed
	void (*CmdBindVertexBuffers)(TGfxCommandBundle bundle,
								 TU8 key,
								 TU4 firstBinding,
								 TU4 bindingCount,
								 TGfxBuffer const* buffers,
								 const unsigned long long* offsets);
	// @param indexDataTypeSize: Specify the byte size of index data type
	//   (most devices support only 2 and 4)
	void (*CmdBindIndexBuffer)(
		TGfxCommandBundle bundle, TU8 key, TGfxBuffer buffer, TSize offset, unsigned char indexDataTypeSize);
	void (*CmdSetViewport)(TGfxCommandBundle bundle, TU8 key, const TGfxViewportInfo* viewport);
	void (*CmdSetScissor)(TGfxCommandBundle bundle, TU8 key, TGfxIVec2 offset, TGfxUVec2 size);
	void (*CmdSetDepthBounds)(TGfxCommandBundle bundle, TU8 key, float min, float max);
	void (*CmdDrawNonIndexedDirect)(
		TGfxCommandBundle bndl, TU8 key, TU4 vertCount, TU4 instanceCount, TU4 frstVert, TU4 frstInstance);
	void (*CmdDrawIndexedDirect)(TGfxCommandBundle bndl,
								 TU8 key,
								 TU4 indxCount,
								 TU4 instanceCount,
								 TU4 firstIndex,
								 int vertexOffset,
								 TU4 firstInstance);
	void (*CmdCopyBufferToTexture)(TGfxCommandBundle bndl,
								   TU8 key,
								   TGfxBuffer srcBuffer,
								   TSize bufferOffset,
								   TGfxTexture dstTexture,
								   TGfxImageAccess lastAccess,
								   TGfxExtension* exts);
	void (*CmdCopyBufferToBuffer)(TGfxCommandBundle bndl,
								  TU8 key,
								  TSize copySize,
								  TGfxBuffer srcBuffer,
								  TSize srcOffset,
								  TGfxBuffer dstBuffer,
								  TSize dstOffset);

	// EXT: TGFX_OperationCountBuffer
	void (*CmdExecuteIndirect)(TGfxCommandBundle bndl,
							   TU8 key,
							   TU4 operationCount,
							   const TGfxIndirectOperationType* operationTypes,
							   TGfxBuffer dataBffr,
							   TSize drawDataBufferOffset,
							   TGfxExtension* exts);
	// Extensions: TransferQueueOwnership
	void (*CmdBarrierTexture)(TGfxCommandBundle bndl,
							  TU8 key,
							  TGfxTexture texture,
							  TGfxImageAccess lastAccess,
							  TGfxImageAccess nextAccess,
							  TGfxExtension* exts);

	void (*CmdDispatch)(TGfxCommandBundle bndl, TU8 key, const TGfxUVec3 dispatchSize);
	void (*CmdPushConstant)(TGfxCommandBundle bndl, TU8 key, unsigned char offset, unsigned char size, const void* d);
} ITGfxRenderer;

TCORE_END_C_LINKAGE