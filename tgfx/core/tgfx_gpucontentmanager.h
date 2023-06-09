#pragma once
#ifdef __cplusplus
extern "C" {
#endif

struct tgfx_gpuDataManager {
  void (*destroyAllResources)();

  // If sampler is used as constant;
  //	DX12 limits bordercolor to be vec4(0), vec4(0,0,0,1) and vec4(1)
  result_tgfx (*createSampler)(struct tgfx_gpu* gpu, const struct tgfx_samplerDescription* desc,
                               struct tgfx_sampler** hnd);
  void (*destroySampler)(struct tgfx_sampler* sampler);

  // Extension can be UNIFORMBUFFER
  result_tgfx (*createBuffer)(struct tgfx_gpu* gpu, const struct tgfx_bufferDescription* desc,
                              struct tgfx_buffer** handle);
  void (*destroyBuffer)(struct tgfx_buffer* bufferHnd);

  //	If your GPU supports buffer_GPUaddress_pointers, you can use this.
  //		Otherwise this function pointer is NULL
  //	You can pass pointers to buffers (call buffer data or classic GPU buffers)
  //		and use complex data structures and access strategies in shaders
  result_tgfx (*getBufferPointer_GPU)(struct tgfx_buffer* h, unsigned long long* ptr);

  result_tgfx (*createTexture)(struct tgfx_gpu* gpu, const struct tgfx_textureDescription* desc,
                               struct tgfx_texture** textureHnd);
  void (*destroyTexture)(struct tgfx_texture* textureHnd);

  // BINDING TABLES
  ////////////////////////////////////

  // If descType is sampler, SttcSmplrs can be used at binding index 0
  result_tgfx (*createBindingTable)(struct tgfx_gpu* gpu, const struct tgfx_bindingTableDescription* desc,
                                    struct tgfx_bindingTable** table);
  void (*destroyBindingTable)(struct tgfx_bindingTable* bindingTable);
  result_tgfx (*setBindingTable_Texture)(struct tgfx_bindingTable* table, unsigned int bindingCount,
                                         const unsigned int*         bindingIndices,
                                         struct tgfx_texture* const * textures);
  // If offsets is nullptr, then all offsets are 0
  // If sizes is nullptr, then all sizes are whole buffer
  result_tgfx (*setBindingTable_Buffer)(struct tgfx_bindingTable* table, unsigned int bindingCount,
                                        const unsigned int*        bindingIndices,
                                        struct tgfx_buffer* const* buffers,
                                        const unsigned int* offsets, const unsigned int* sizes,
                                        unsigned int extCount, struct tgfx_extension* const* exts);
  result_tgfx (*setBindingTable_Sampler)(struct tgfx_bindingTable* table, unsigned int bindingCount,
                                         const unsigned int*         bindingIndices,
                                         struct tgfx_sampler* const* samplers);

  // SHADER & PIPELINE COMPILATION
  /////////////////////////////////////

  result_tgfx (*compileShaderSource)(struct tgfx_gpu* gpu, enum shaderlanguages_tgfx language,
                                     enum shaderStage_tgfx shaderstage, const void* DATA,
                                     unsigned int               DATA_SIZE,
                                     struct tgfx_shaderSource** ShaderSourceHandle);
  void (*destroyShaderSource)(struct tgfx_shaderSource* ShaderSourceHandle);
  // Extensions: CallBufferInfo, Subpass, StaticRasterState
  result_tgfx (*createRasterPipeline)(const struct tgfx_rasterPipelineDescription* desc,
                                      struct tgfx_pipeline**                hnd);
  // Extensions: Dynamic States, CallBufferInfo, Specialization Constants
  result_tgfx (*copyRasterPipeline)(struct tgfx_pipeline*         basePipeline,
                                    struct tgfx_extension* const* exts,
                                    struct tgfx_pipeline**        derivedPipeline);

  result_tgfx (*createComputePipeline)(struct tgfx_shaderSource*           Source,
                                       unsigned int                        bindingTableCount,
                                       const tgfx_bindingTableDescription* bindingTableDescs,
                                       unsigned char                       pushConstantOffset,
                                       unsigned char                       pushConstantSize,
                                       struct tgfx_pipeline**              hnd);
  // Extensions: CallBufferInfo, Specialization Constants
  result_tgfx (*copyComputePipeline)(struct tgfx_pipeline* src, struct tgfx_extension* const* exts,
                                     struct tgfx_pipeline** dst);
  void (*destroyPipeline)(struct tgfx_pipeline* pipeline);

  //////////////////////////////
  // MEMORY
  //////////////////////////////

  // Extensions: Dedicated Memory Allocation
  result_tgfx (*createHeap)(struct tgfx_gpu* gpu, unsigned char memoryRegionID,
                            unsigned long long heapSize, unsigned int extCount,
                            struct tgfx_extension* const* exts, struct tgfx_heap** heap);
  result_tgfx (*getHeapRequirement_Texture)(struct tgfx_texture* texture, unsigned int extCount,
                                            struct tgfx_extension* const* exts,
                                            struct tgfx_heapRequirementsInfo*    reqs);
  result_tgfx (*getHeapRequirement_Buffer)(struct tgfx_buffer* buffer, unsigned int extCount,
                                           struct tgfx_extension* const* exts,
                                           struct tgfx_heapRequirementsInfo*    reqs);
  // @return FAIL if this feature isn't supported
  result_tgfx (*getRemainingMemory)(struct tgfx_gpu* GPU, unsigned char memoryRegionID,
                                    unsigned int extCount, struct tgfx_extension* const* exts,
                                    unsigned long long* size);
  result_tgfx (*bindToHeap_Buffer)(struct tgfx_heap* heap, unsigned long long offset,
                                   struct tgfx_buffer* buffer, unsigned int extCount,
                                   struct tgfx_extension* const* exts);
  result_tgfx (*bindToHeap_Texture)(struct tgfx_heap* heap, unsigned long long offset,
                                    struct tgfx_texture* texture, unsigned int extCount,
                                    struct tgfx_extension* const* exts);
  // You can only map one part of a heap at a time
  // Unmap the heap if you want to map another part of it
  // @param size: UINT64_MAX if you want to map the whole heap
  result_tgfx (*mapHeap)(struct tgfx_heap* heap, unsigned long long offset, unsigned long long size,
                         unsigned int extCount, struct tgfx_extension* const* exts,
                         void** mappedRegion);
  result_tgfx (*unmapHeap)(struct tgfx_heap* heap);

};

#ifdef __cplusplus
}
#endif