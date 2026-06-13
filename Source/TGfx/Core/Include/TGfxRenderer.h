#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef int textureUsageMask_tgfxflag;
typedef struct tgfx_renderer {
  // Command Buffer Functions
  ////////////////////////////

  // Command buffers are one-time only buffers
  //  so when you submit for execution, they'll be freed after their execution
  // Extensions: Storage command buffers
  struct tgfx_commandBuffer* (*beginCommandBuffer)(struct tgfx_gpuQueue* queue, unsigned int extCount,
                                              struct tgfx_extension* const* exts);
  void (*endCommandBuffer)(struct tgfx_commandBuffer* commandBuffer);
  // In a Rendersubpass: All bundles should be created with the rendersubpass' handle
  // Outside: All bundles should be created with rendersubpass as NULL.
  // All bundles should be from the compatible queue with the cmdBuffer's queue
  void (*executeBundles)(struct tgfx_commandBuffer* commandBuffer, unsigned int bundleCount,
                         struct tgfx_commandBundle* const* bundles, unsigned int extCount,
                         struct tgfx_extension* const* exts);
  void (*beginRasterpass)(struct tgfx_commandBuffer* commandBuffer, unsigned int colorAttachmentCount,
                          const struct tgfx_rasterpassBeginSlotInfo* colorAttachments,
                          const struct tgfx_rasterpassBeginSlotInfo* depthAttachment, unsigned int extCount,
                          struct tgfx_extension* const* exts);
  void (*nextSubRasterpass)(struct tgfx_commandBuffer* commandBuffer);
  void (*endRasterpass)(struct tgfx_commandBuffer* commandBuffer, unsigned int extCount,
                        struct tgfx_extension* const* exts);

  // Synchronization Functions

  // @param fenceCount: Fence count that will be created
  // @param initValue: Set initial value of all fences
  // @param fenceList: User should create the array of fence_tgfx_handles.
  //    So array isn't created by backend
  void (*createFences)(struct tgfx_gpu* gpu, unsigned int count, unsigned int initValue,
                       struct tgfx_fence** fenceList);
  void (*destroyFence)(struct tgfx_fence* fence);
  // CPU side fence value change
  enum result_tgfx (*setFence)(struct tgfx_fence* fence, unsigned long long value);
  enum result_tgfx (*getFenceValue)(struct tgfx_fence* fence, unsigned long long* value);

  // Queue Functions
  //  These functions're executed sequentially (implicitly synchronized) on GPU queue
  ///////////////////////////

  // All command buffers should be from the same queue
  void (*queueFenceSignalWait)(struct tgfx_gpuQueue* queue, unsigned int waitsCount,
                               struct tgfx_fence* const*      waitFences,
                               const unsigned long long* waitValues, unsigned int signalsCount,
                               struct tgfx_fence* const*      signalFences,
                               const unsigned long long* signalValues);
  void (*queueExecuteCmdBuffers)(struct tgfx_gpuQueue* queue, unsigned int cmdBufferCount,
                                 struct tgfx_commandBuffer* const* cmdBuffers, unsigned int extCount,
                                 struct tgfx_extension* const* exts);
  void (*queuePresent)(struct tgfx_gpuQueue* queue, unsigned int windowCount, struct tgfx_window* const* windowlist);
  // Submit queue operations to GPU.
  // You should call this right before changing Queue Operation type.
  // Queue Operation Types: ExecuteCmdBuffers, Present, BindSparse (optional, in future)
  // FenceSignalWait operation doesn't have a type, it can work with all of these operations.
  // Because operations are sent through PCI-E, it costs a lot. Profile this on your device and
  //  design your renderer according to it.
  void (*queueSubmit)(struct tgfx_gpuQueue* queue);

  // Command Bundle Functions
  ////////////////////////////

  // @param maxCmdCount: Backend allocates a command buffer to store commands
  // Every cmdXXX call's "key" argument should be [0,maxCmdCount-1].
  struct tgfx_commandBundle* (*beginCommandBundle)(struct tgfx_gpu* gpu, unsigned long long maxCmdCount,
                                              struct tgfx_pipeline* defaultPipeline,
                                              unsigned int extCount, struct tgfx_extension* const* exts);
  void (*finishCommandBundle)(struct tgfx_commandBundle* bundle, unsigned int extCount,
                              struct tgfx_extension* const* exts);
  // If you won't execute same bundle later, destroy to allow backend
  //   implementation to optimize memory usage
  void (*destroyCommandBundle)(struct tgfx_commandBundle* hnd);
  void (*cmdBindBindingTables)(struct tgfx_commandBundle* bundle, unsigned long long sortKey,
                               unsigned int firstSetIndx, unsigned int bindingTableCount,
                               struct tgfx_bindingTable* const* bindingTables,
                               enum pipelineType_tgfx           pipeline);
  void (*cmdBindPipeline)(struct tgfx_commandBundle* bundle, unsigned long long sortKey,
                          struct tgfx_pipeline* pipeline);
  // For devices that doesn't allow storage buffers to store vertex buffers,
  //   this function is needed
  void (*cmdBindVertexBuffers)(struct tgfx_commandBundle* bundle, unsigned long long key,
                               unsigned int firstBinding, unsigned int bindingCount,
                               struct tgfx_buffer* const* buffers, const unsigned long long* offsets);
  // @param indexDataTypeSize: Specify the byte size of index data type
  //   (most devices support only 2 and 4)
  void (*cmdBindIndexBuffer)(struct tgfx_commandBundle* bundle, unsigned long long key,
                             struct tgfx_buffer* buffer, unsigned long long offset,
                             unsigned char indexDataTypeSize);
  void (*cmdSetViewport)(struct tgfx_commandBundle* bundle, unsigned long long key,
                         const struct tgfx_viewportInfo* viewport);
  void (*cmdSetScissor)(struct tgfx_commandBundle* bundle, unsigned long long key, const struct tgfx_ivec2* offset,
                        const struct tgfx_uvec2* size);
  void (*cmdSetDepthBounds)(struct tgfx_commandBundle* bundle, unsigned long long key, float min,
                            float max);
  void (*cmdDrawNonIndexedDirect)(struct tgfx_commandBundle* bndl, unsigned long long key,
                                  unsigned int vertCount, unsigned int instanceCount,
                                  unsigned int frstVert, unsigned int frstInstance);
  void (*cmdDrawIndexedDirect)(struct tgfx_commandBundle* bndl, unsigned long long key,
                               unsigned int indxCount, unsigned int instanceCount,
                               unsigned int firstIndex, int vertexOffset,
                               unsigned int firstInstance);
  void (*cmdCopyBufferToTexture)(struct tgfx_commandBundle* bndl, unsigned long long key,
                                 struct tgfx_buffer* srcBuffer, unsigned long long bufferOffset,
                                 struct tgfx_texture* dstTexture, enum image_access_tgfx lastAccess,
                                 unsigned int extCount, struct tgfx_extension* const* exts);
  void (*cmdCopyBufferToBuffer)(struct tgfx_commandBundle* bndl, unsigned long long key,
                                unsigned long long copySize, struct tgfx_buffer* srcBuffer,
                                unsigned long long srcOffset, struct tgfx_buffer* dstBuffer,
                                unsigned long long dstOffset);

  // EXT: TGFX_OperationCountBuffer
  void (*cmdExecuteIndirect)(struct tgfx_commandBundle* bndl, unsigned long long key,
                             unsigned int                      operationCount,
                             const enum indirectOperationType_tgfx* operationTypes,
                             struct tgfx_buffer* dataBffr, unsigned long long drawDataBufferOffset,
                             unsigned int extCount, struct tgfx_extension* const* exts);
  // Extensions: TransferQueueOwnership
  void (*cmdBarrierTexture)(struct tgfx_commandBundle* bndl, unsigned long long key,
                            struct tgfx_texture* texture, enum image_access_tgfx lastAccess,
                            enum image_access_tgfx nextAccess, textureUsageMask_tgfxflag lastUsage,
                            textureUsageMask_tgfxflag nextUsage, unsigned int extCount,
                            struct tgfx_extension* const* exts);

  void (*cmdDispatch)(struct tgfx_commandBundle* bndl, unsigned long long key, const struct tgfx_uvec3* dispatchSize);
  void (*cmdPushConstant)(struct tgfx_commandBundle* bndl, unsigned long long key,
                          unsigned char offset, unsigned char size, const void* d);
};

#ifdef __cplusplus
}
#endif