#pragma once
#include <TGfxDeclarations.h>
TCORE_BEGIN_C_LINKAGE

typedef int textureUsageMask_tgfxflag;
typedef struct TGfxRenderer
{
	// Command Buffer Functions
	////////////////////////////

	// Command buffers are one-time only buffers
	//  so when you submit for execution, they'll be freed after their execution
	// Extensions: Storage command buffers
	TGfxCommandBufferHnd (*BeginCommandBuffer)(TGfxQueueHnd queue,
											   unsigned int extCount,
											   struct tgfx_extension* const* exts);
	void (*EndCommandBuffer)(TGfxCommandBufferHnd commandBuffer);
	// In a Rendersubpass: All bundles should be created with the rendersubpass' handle
	// Outside: All bundles should be created with rendersubpass as NULL.
	// All bundles should be from the compatible queue with the cmdBuffer's queue
	void (*ExecuteBundles)(TGfxCommandBufferHnd commandBuffer,
						   unsigned int bundleCount,
						   struct tgfx_commandBundle* const* bundles,
						   unsigned int extCount,
						   struct tgfx_extension* const* exts);
	void (*BeginRasterpass)(TGfxCommandBufferHnd commandBuffer,
							unsigned int colorAttachmentCount,
							const struct tgfx_rasterpassBeginSlotInfo* colorAttachments,
							const struct tgfx_rasterpassBeginSlotInfo* depthAttachment,
							unsigned int extCount,
							struct tgfx_extension* const* exts);
	void (*NextSubRasterpass)(TGfxCommandBufferHnd commandBuffer);
	void (*EndRasterpass)(TGfxCommandBufferHnd commandBuffer,
						  unsigned int extCount,
						  struct tgfx_extension* const* exts);

	// Synchronization Functions

	// @param fenceCount: Fence count that will be created
	// @param initValue: Set initial value of all fences
	// @param fenceList: User should create the array of fence_tgfx_handles.
	//    So array isn't created by backend
	void (*CreateFences)(TGfxGpuHnd gpu, unsigned int count, unsigned int initValue, TGfxFenceHnd* fenceList);
	void (*DestroyFence)(TGfxFenceHnd fence);
	// CPU side fence value change
	enum result_tgfx (*SetFence)(TGfxFenceHnd fence, unsigned long long value);
	enum result_tgfx (*GetFenceValue)(TGfxFenceHnd fence, unsigned long long* value);

	// Queue Functions
	//  These functions're executed sequentially (implicitly synchronized) on GPU queue
	///////////////////////////

	// All command buffers should be from the same queue
	void (*QueueFenceSignalWait)(TGfxQueueHnd queue,
								 unsigned int waitsCount,
								 TGfxFenceHnd const* waitFences,
								 const unsigned long long* waitValues,
								 unsigned int signalsCount,
								 TGfxFenceHnd const* signalFences,
								 const unsigned long long* signalValues);
	void (*QueueExecuteCmdBuffers)(TGfxQueueHnd queue,
								   unsigned int cmdBufferCount,
								   TGfxCommandBufferHnd const* cmdBuffers,
								   unsigned int extCount,
								   struct tgfx_extension* const* exts);
	void (*QueuePresent)(TGfxQueueHnd queue, unsigned int windowCount, struct tgfx_window* const* windowlist);
	// Submit queue operations to GPU.
	// You should call this right before changing Queue Operation type.
	// Queue Operation Types: ExecuteCmdBuffers, Present, BindSparse (optional, in future)
	// FenceSignalWait operation doesn't have a type, it can work with all of these operations.
	// Because operations are sent through PCI-E, it costs a lot. Profile this on your device and
	//  design your renderer according to it.
	void (*QueueSubmit)(TGfxQueueHnd queue);

	// Command Bundle Functions
	////////////////////////////

	// @param maxCmdCount: Backend allocates a command buffer to store commands
	// Every cmdXXX call's "key" argument should be [0,maxCmdCount-1].
	struct tgfx_commandBundle* (*BeginCommandBundle)(TGfxGpuHnd gpu,
													 unsigned long long maxCmdCount,
													 struct tgfx_pipeline* defaultPipeline,
													 unsigned int extCount,
													 struct tgfx_extension* const* exts);
	void (*FinishCommandBundle)(struct tgfx_commandBundle* bundle,
								unsigned int extCount,
								struct tgfx_extension* const* exts);
	// If you won't execute same bundle later, destroy to allow backend
	//   implementation to optimize memory usage
	void (*DestroyCommandBundle)(struct tgfx_commandBundle* hnd);
	void (*CmdBindBindingTables)(struct tgfx_commandBundle* bundle,
								 unsigned long long sortKey,
								 unsigned int firstSetIndx,
								 unsigned int bindingTableCount,
								 struct tgfx_bindingTable* const* bindingTables,
								 enum pipelineType_tgfx pipeline);
	void (*CmdBindPipeline)(struct tgfx_commandBundle* bundle,
							unsigned long long sortKey,
							struct tgfx_pipeline* pipeline);
	// For devices that doesn't allow storage buffers to store vertex buffers,
	//   this function is needed
	void (*CmdBindVertexBuffers)(struct tgfx_commandBundle* bundle,
								 unsigned long long key,
								 unsigned int firstBinding,
								 unsigned int bindingCount,
								 struct tgfx_buffer* const* buffers,
								 const unsigned long long* offsets);
	// @param indexDataTypeSize: Specify the byte size of index data type
	//   (most devices support only 2 and 4)
	void (*CmdBindIndexBuffer)(struct tgfx_commandBundle* bundle,
							   unsigned long long key,
							   struct tgfx_buffer* buffer,
							   unsigned long long offset,
							   unsigned char indexDataTypeSize);
	void (*CmdSetViewport)(struct tgfx_commandBundle* bundle,
						   unsigned long long key,
						   const struct tgfx_viewportInfo* viewport);
	void (*CmdSetScissor)(struct tgfx_commandBundle* bundle,
						  unsigned long long key,
						  const struct tgfx_ivec2* offset,
						  const struct tgfx_uvec2* size);
	void (*CmdSetDepthBounds)(struct tgfx_commandBundle* bundle, unsigned long long key, float min, float max);
	void (*CmdDrawNonIndexedDirect)(struct tgfx_commandBundle* bndl,
									unsigned long long key,
									unsigned int vertCount,
									unsigned int instanceCount,
									unsigned int frstVert,
									unsigned int frstInstance);
	void (*CmdDrawIndexedDirect)(struct tgfx_commandBundle* bndl,
								 unsigned long long key,
								 unsigned int indxCount,
								 unsigned int instanceCount,
								 unsigned int firstIndex,
								 int vertexOffset,
								 unsigned int firstInstance);
	void (*CmdCopyBufferToTexture)(struct tgfx_commandBundle* bndl,
								   unsigned long long key,
								   struct tgfx_buffer* srcBuffer,
								   unsigned long long bufferOffset,
								   struct tgfx_texture* dstTexture,
								   enum image_access_tgfx lastAccess,
								   unsigned int extCount,
								   struct tgfx_extension* const* exts);
	void (*CmdCopyBufferToBuffer)(struct tgfx_commandBundle* bndl,
								  unsigned long long key,
								  unsigned long long copySize,
								  struct tgfx_buffer* srcBuffer,
								  unsigned long long srcOffset,
								  struct tgfx_buffer* dstBuffer,
								  unsigned long long dstOffset);

	// EXT: TGFX_OperationCountBuffer
	void (*CmdExecuteIndirect)(struct tgfx_commandBundle* bndl,
							   unsigned long long key,
							   unsigned int operationCount,
							   const enum indirectOperationType_tgfx* operationTypes,
							   struct tgfx_buffer* dataBffr,
							   unsigned long long drawDataBufferOffset,
							   unsigned int extCount,
							   struct tgfx_extension* const* exts);
	// Extensions: TransferQueueOwnership
	void (*CmdBarrierTexture)(struct tgfx_commandBundle* bndl,
							  unsigned long long key,
							  struct tgfx_texture* texture,
							  enum image_access_tgfx lastAccess,
							  enum image_access_tgfx nextAccess,
							  textureUsageMask_tgfxflag lastUsage,
							  textureUsageMask_tgfxflag nextUsage,
							  unsigned int extCount,
							  struct tgfx_extension* const* exts);

	void (*CmdDispatch)(struct tgfx_commandBundle* bndl, unsigned long long key, const struct tgfx_uvec3* dispatchSize);
	void (*CmdPushConstant)(struct tgfx_commandBundle* bndl,
							unsigned long long key,
							unsigned char offset,
							unsigned char size,
							const void* d);
} TGfxRenderer;

TCORE_END_C_LINKAGE