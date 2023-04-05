#pragma once
#include "tgfx_forwarddeclarations.h"
#include "tgfx_structs.h"

typedef struct tgfx_renderer {
  // Command Buffer Functions
  ////////////////////////////

  // Command buffers are one-time only buffers
  //  so when you submit for execution, they'll be freed after their execution
  // Extensions: Storage command buffers
  commandBuffer_tgfxhnd (*beginCommandBuffer)(gpuQueue_tgfxhnd queue, unsigned int extCount,
                                              const extension_tgfxhnd* exts);
  void (*endCommandBuffer)(commandBuffer_tgfxhnd commandBuffer);
  // In a Rendersubpass: All bundles should be created with the rendersubpass' handle
  // Outside: All bundles should be created with rendersubpass as NULL.
  // All bundles should be from the compatible queue with the cmdBuffer's queue
  void (*executeBundles)(commandBuffer_tgfxhnd commandBuffer, unsigned int bundleCount,
                         const commandBundle_tgfxhnd* bundles, unsigned int extCount,
                         const extension_tgfxhnd* exts);
  void (*beginRasterpass)(commandBuffer_tgfxhnd commandBuffer, unsigned int colorAttachmentCount,
                          const rasterpassBeginSlotInfo_tgfx* colorAttachments,
                          rasterpassBeginSlotInfo_tgfx depthAttachment, unsigned int extCount,
                          const extension_tgfxhnd* exts);
  void (*nextSubRasterpass)(commandBuffer_tgfxhnd commandBuffer);
  void (*endRasterpass)(commandBuffer_tgfxhnd commandBuffer, unsigned int extCount,
                        const extension_tgfxhnd* exts);

  // Synchronization Functions

  // @param fenceCount: Fence count that will be created
  // @param initValue: Set initial value of all fences
  // @param fenceList: User should create the array of fence_tgfx_handles.
  //    So array isn't created by backend
  void (*createFences)(gpu_tgfxhnd gpu, unsigned int count, unsigned int initValue,
                       fence_tgfxhnd* fenceList);
  void (*destroyFence)(fence_tgfxhnd fence);
  // CPU side fence value change
  result_tgfx (*setFence)(fence_tgfxhnd fence, unsigned long long value);
  result_tgfx (*getFenceValue)(fence_tgfxhnd fence, unsigned long long* value);

  // Queue Functions
  //  These functions're executed sequentially (implicitly synchronized) on GPU queue
  ///////////////////////////

  // All command buffers should be from the same queue
  void (*queueFenceSignalWait)(gpuQueue_tgfxhnd queue, unsigned int waitsCount,
                               const fence_tgfxhnd*      waitFences,
                               const unsigned long long* waitValues, unsigned int signalsCount,
                               const fence_tgfxhnd*      signalFences,
                               const unsigned long long* signalValues);
  void (*queueExecuteCmdBuffers)(gpuQueue_tgfxhnd queue, unsigned int cmdBufferCount,
                                 const commandBuffer_tgfxhnd* cmdBuffers, unsigned int extCount,
                                 const extension_tgfxhnd* exts);
  void (*queuePresent)(gpuQueue_tgfxhnd queue, unsigned int windowCount, const window_tgfxhnd* windowlist);
  // Submit queue operations to GPU.
  // You should call this right before changing Queue Operation type.
  // Queue Operation Types: ExecuteCmdBuffers, Present, BindSparse (optional, in future)
  // FenceSignalWait operation doesn't have a type, it can work with all of these operations.
  // Because operations are sent through PCI-E, it costs a lot. Profile this on your device and
  //  design your renderer according to it.
  void (*queueSubmit)(gpuQueue_tgfxhnd queue);

  // Command Bundle Functions
  ////////////////////////////

  // @param maxCmdCount: Backend allocates a command buffer to store commands
  // @param subpassHandle: If command bundle is gonna run raster operations, this should be a valid
  // raster pipeline. Compute bundles doesn't need this to be set.
  // Every cmdXXX call's "key" argument should be [0,maxCmdCount-1].
  commandBundle_tgfxhnd (*beginCommandBundle)(gpu_tgfxhnd gpu, unsigned long long maxCmdCount,
                                              pipeline_tgfxhnd defaultPipeline,
                                              unsigned int extCount, const extension_tgfxhnd* exts);
  void (*finishCommandBundle)(commandBundle_tgfxhnd bundle, unsigned int extCount,
                              const extension_tgfxhnd* exts);
  // If you won't execute same bundle later, destroy to allow backend
  //   implementation to optimize memory usage
  void (*destroyCommandBundle)(commandBundle_tgfxhnd hnd);
  void (*cmdBindBindingTables)(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                               unsigned int firstSetIndx, unsigned int bindingTableCount,
                               const bindingTable_tgfxhnd* bindingTables,
                               pipelineType_tgfx           pipelineType);
  void (*cmdBindPipeline)(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                          pipeline_tgfxhnd pipeline);
  // For devices that doesn't allow storage buffers to store vertex buffers,
  //   this function is needed
  void (*cmdBindVertexBuffers)(commandBundle_tgfxhnd bundle, unsigned long long key,
                               unsigned int firstBinding, unsigned int bindingCount,
                               const buffer_tgfxhnd* buffers, const unsigned long long* offsets);
  // @param indexDataTypeSize: Specify the byte size of index data type
  //   (most devices support only 2 and 4)
  void (*cmdBindIndexBuffer)(commandBundle_tgfxhnd bundle, unsigned long long key,
                             buffer_tgfxhnd buffer, unsigned long long offset,
                             unsigned char indexDataTypeSize);
  void (*cmdSetViewport)(commandBundle_tgfxhnd bundle, unsigned long long key,
                         viewportInfo_tgfx viewport);
  void (*cmdSetScissor)(commandBundle_tgfxhnd bundle, unsigned long long key, ivec2_tgfx offset,
                        uvec2_tgfx size);
  void (*cmdSetDepthBounds)(commandBundle_tgfxhnd bundle, unsigned long long key, float min,
                            float max);
  void (*cmdDrawNonIndexedDirect)(commandBundle_tgfxhnd bndl, unsigned long long key,
                                  unsigned int vertCount, unsigned int instanceCount,
                                  unsigned int frstVert, unsigned int frstInstance);
  void (*cmdDrawIndexedDirect)(commandBundle_tgfxhnd bndl, unsigned long long key,
                               unsigned int indxCount, unsigned int instanceCount,
                               unsigned int firstIndex, int vertexOffset,
                               unsigned int firstInstance);
  void (*cmdCopyBufferToTexture)(commandBundle_tgfxhnd bndl, unsigned long long key,
                                 buffer_tgfxhnd srcBuffer, unsigned long long bufferOffset,
                                 texture_tgfxhnd dstTexture, image_access_tgfx lastAccess,
                                 unsigned int extCount, const extension_tgfxhnd* exts);
  void (*cmdCopyBufferToBuffer)(commandBundle_tgfxhnd bndl, unsigned long long key,
                                unsigned long long copySize, buffer_tgfxhnd srcBuffer,
                                unsigned long long srcOffset, buffer_tgfxhnd dstBuffer,
                                unsigned long long dstOffset);

  // EXT: TGFX_OperationCountBuffer
  void (*cmdExecuteIndirect)(commandBundle_tgfxhnd bndl, unsigned long long key,
                             unsigned int                      operationCount,
                             const indirectOperationType_tgfx* operationTypes,
                             buffer_tgfxhnd dataBffr, unsigned long long drawDataBufferOffset,
                             unsigned int extCount, const extension_tgfxhnd* exts);
  // Extensions: TransferQueueOwnership
  void (*cmdBarrierTexture)(commandBundle_tgfxhnd bndl, unsigned long long key,
                            texture_tgfxhnd texture, image_access_tgfx lastAccess,
                            image_access_tgfx nextAccess, textureUsageMask_tgfxflag lastUsage,
                            textureUsageMask_tgfxflag nextUsage, unsigned int extCount,
                            const extension_tgfxhnd* exts);

  void (*cmdDispatch)(commandBundle_tgfxhnd bndl, unsigned long long key, uvec3_tgfx dispatchSize);
} renderer_tgfx;
