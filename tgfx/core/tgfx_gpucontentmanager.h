#pragma once
#include "tgfx_forwarddeclarations.h"
#include "tgfx_structs.h"

typedef struct tgfx_gpudatamanager {
  void (*destroyAllResources)();

  // If sampler is used as constant;
  //	DX12 limits bordercolor to be vec4(0), vec4(0,0,0,1) and vec4(1)
  result_tgfx (*createSampler)(gpu_tgfxhnd gpu, const samplerDescription_tgfx* desc,
                               sampler_tgfxhnd* hnd);
  void (*destroySampler)(sampler_tgfxhnd sampler);

  // Extension can be UNIFORMBUFFER
  result_tgfx (*createBuffer)(gpu_tgfxhnd gpu, const bufferDescription_tgfx* desc,
                              buffer_tgfxhnd* handle);
  void (*destroyBuffer)(buffer_tgfxhnd bufferHnd);

  //	If your GPU supports buffer_GPUaddress_pointers, you can use this.
  //		Otherwise this function pointer is NULL
  //	You can pass pointers to buffers (call buffer data or classic GPU buffers)
  //		and use complex data structures and access strategies in shaders
  result_tgfx (*getBufferPointer_GPU)(buffer_tgfxhnd h, unsigned long long* ptr);

  result_tgfx (*createTexture)(gpu_tgfxhnd gpu, const textureDescription_tgfx* desc,
                               texture_tgfxhnd* textureHnd);
  void (*destroyTexture)(texture_tgfxhnd textureHnd);

  // BINDING TABLES
  ////////////////////////////////////

  // If descType is sampler, SttcSmplrs can be used at binding index 0
  result_tgfx (*createBindingTable)(gpu_tgfxhnd gpu,
                                    const bindingTableDescription_tgfx const* const desc,
                                    bindingTable_tgfxhnd* table);
  void (*destroyBindingTable)(bindingTable_tgfxhnd bindingTable);
  result_tgfx (*setBindingTable_Texture)(bindingTable_tgfxhnd table, unsigned int bindingCount,
                                         const unsigned int*      bindingIndices,
                                         const texture_tgfxlsthnd textures);
  // If offsets is nullptr, then all offsets are 0
  // If sizes is nullptr, then all sizes are whole buffer
  result_tgfx (*setBindingTable_Buffer)(bindingTable_tgfxhnd table, unsigned int bindingCount,
                                        const unsigned int*     bindingIndices,
                                        const buffer_tgfxlsthnd buffers,
                                        const unsigned int* offsets, const unsigned int* sizes,
                                        extension_tgfxlsthnd exts);
  result_tgfx (*setBindingTable_Sampler)(bindingTable_tgfxhnd table, unsigned int bindingCount,
                                         const unsigned int*      bindingIndices,
                                         const sampler_tgfxlsthnd samplers);

  // SHADER & PIPELINE COMPILATION
  /////////////////////////////////////

  result_tgfx (*compileShaderSource)(gpu_tgfxhnd gpu, shaderlanguages_tgfx language,
                                     shaderStage_tgfx shaderstage, const void* DATA,
                                     unsigned int          DATA_SIZE,
                                     shaderSource_tgfxhnd* ShaderSourceHandle);
  void (*destroyShaderSource)(shaderSource_tgfxhnd ShaderSourceHandle);
  // Extensions: CallBufferInfo, Subpass, StaticRasterState
  result_tgfx (*createRasterPipeline)(const rasterPipelineDescription_tgfx const* desc,
                                      extension_tgfxlsthnd exts, pipeline_tgfxhnd* hnd);
  // Extensions: Dynamic States, CallBufferInfo, Specialization Constants
  result_tgfx (*copyRasterPipeline)(pipeline_tgfxhnd basePipeline, extension_tgfxlsthnd exts,
                                    pipeline_tgfxhnd* derivedPipeline);

  result_tgfx (*createComputePipeline)(shaderSource_tgfxhnd Source, unsigned int bindingTableCount,
                                       const bindingTableDescription_tgfx const* const bindingTableDescs,
                                       unsigned char isCallBufferSupported, pipeline_tgfxhnd* hnd);
  // Extensions: CallBufferInfo, Specialization Constants
  result_tgfx (*copyComputePipeline)(pipeline_tgfxhnd src, extension_tgfxlsthnd exts,
                                     pipeline_tgfxhnd* dst);
  void (*destroyPipeline)(pipeline_tgfxhnd pipeline);

  //////////////////////////////
  // MEMORY
  //////////////////////////////

  // Extensions: Dedicated Memory Allocation
  result_tgfx (*createHeap)(gpu_tgfxhnd gpu, unsigned char memoryRegionID,
                            unsigned long long heapSize, extension_tgfxlsthnd exts,
                            heap_tgfxhnd* heap);
  result_tgfx (*getHeapRequirement_Texture)(texture_tgfxhnd texture, extension_tgfxlsthnd exts,
                                            heapRequirementsInfo_tgfx* reqs);
  result_tgfx (*getHeapRequirement_Buffer)(buffer_tgfxhnd buffer, extension_tgfxlsthnd exts,
                                           heapRequirementsInfo_tgfx* reqs);
  // @return FAIL if this feature isn't supported
  result_tgfx (*getRemainingMemory)(gpu_tgfxhnd GPU, unsigned char memoryRegionID,
                                    extension_tgfxlsthnd exts, unsigned long long* size);
  result_tgfx (*bindToHeap_Buffer)(heap_tgfxhnd heap, unsigned long long offset,
                                   buffer_tgfxhnd buffer, extension_tgfxlsthnd exts);
  result_tgfx (*bindToHeap_Texture)(heap_tgfxhnd heap, unsigned long long offset,
                                    texture_tgfxhnd texture, extension_tgfxlsthnd exts);
  // You can only map one part of a heap at a time
  // Unmap the heap if you want to map another part of it
  // @param size: UINT64_MAX if you want to map the whole heap
  result_tgfx (*mapHeap)(heap_tgfxhnd heap, unsigned long long offset, unsigned long long size,
                         extension_tgfxlsthnd exts, void** mappedRegion);
  result_tgfx (*unmapHeap)(heap_tgfxhnd heap);

} gpudatamanager_tgfx;