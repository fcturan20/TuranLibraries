#pragma once
#include "tgfx_forwarddeclarations.h"
#include "tgfx_structs.h"

typedef struct tgfx_gpudatamanager {
  void (*destroyAllResources)();

  // If sampler is used as constant;
  //	DX12 limits bordercolor to be vec4(0), vec4(0,0,0,1) and vec4(1)
  result_tgfx (*createSampler)(gpu_tgfxhnd gpu, const samplerDescription_tgfx* desc,
                               sampler_tgfxhnd* hnd);

  // Attributes list should end with UNDEFINED (one extra element at the end)
  result_tgfx (*createVertexAttribLayout)(const datatype_tgfx*           attributes,
                                          vertexAttributeLayout_tgfxhnd* hnd);
  void (*destroyVertexAttribLayout)(vertexAttributeLayout_tgfxhnd hnd);

  // Extension can be UNIFORMBUFFER
  result_tgfx (*createBuffer)(gpu_tgfxhnd gpu, const bufferDescription_tgfx* desc,
                              buffer_tgfxhnd* handle);
  void (*destroyBuffer)(buffer_tgfxhnd BUFFER_ID);
  //	If your GPU supports buffer_GPUaddress_pointers, you can use this.
  //		Otherwise this function pointer is NULL
  //	You can pass pointers to buffers (call buffer data or classic GPU buffers)
  //		and use complex data structures and access strategies in shaders
  result_tgfx (*getBufferPointer_GPU)(buffer_tgfxhnd h, unsigned long long* ptr);

  result_tgfx (*createTexture)(gpu_tgfxhnd gpu, const textureDescription_tgfx* desc,
                               texture_tgfxhnd* textureHnd);
  void (*deleteTexture)(texture_tgfxhnd TEXTUREHANDLE);

  // BINDING TABLES
  ////////////////////////////////////

  // If descType is sampler, SttcSmplrs can be used at binding index 0
  // If your GPU supports VariableDescCount, you set ElementCount to UINT32_MAX
  result_tgfx (*createBindingTableType)(gpu_tgfxhnd gpu, const bindingTableDescription_tgfx* desc,
                                        bindingTableType_tgfxhnd* bindingTableHandle);
  result_tgfx (*instantiateBindingTable)(bindingTableType_tgfxhnd type,
                                         bindingTable_tgfxhnd*    table);
  result_tgfx (*setBindingTable_Texture)(bindingTable_tgfxhnd table, unsigned int BINDINDEX,
                                         texture_tgfxhnd texture);
  result_tgfx (*setBindingTable_Buffer)(bindingTable_tgfxhnd table, unsigned int BINDINDEX,
                                        buffer_tgfxhnd buffer, unsigned int offset,
                                        unsigned int size, extension_tgfxlsthnd exts);
  result_tgfx (*setBindingTable_Sampler)(bindingTable_tgfxhnd table, unsigned int BINDINDEX,
                                         sampler_tgfxhnd sampler);

  // SHADER & PIPELINE COMPILATION
  /////////////////////////////////////

  result_tgfx (*compileShaderSource)(gpu_tgfxhnd gpu, shaderlanguages_tgfx language,
                                     shaderstage_tgfx shaderstage, void* DATA,
                                     unsigned int          DATA_SIZE,
                                     shaderSource_tgfxhnd* ShaderSourceHandle);
  void (*deleteShaderSource)(shaderSource_tgfxhnd ShaderSourceHandle);
  // AttribLayout can be NULL if GPU supports dynamic vertex buffer layout
  // All objects should be created from the same GPU
  // Extensions: CallBufferInfo
  result_tgfx (*createRasterPipeline)(const shaderSource_tgfxlsthnd       ShaderSourcsLst,
                                      const bindingTableType_tgfxlsthnd   typeTables,
                                      const vertexAttributeLayout_tgfxhnd AttribLayout,
                                      const viewport_tgfxlsthnd           ViewportList,
                                      const renderSubPass_tgfxhnd         subpass,
                                      const rasterStateDescription_tgfx*  mainStates,
                                      rasterPipeline_tgfxhnd*             MaterialHandle);
  // Extensions: Dynamic States, CallBufferInfo, Specialization Constants
  result_tgfx (*copyRasterPipeline)(rasterPipeline_tgfxhnd basePipeline, extension_tgfxlsthnd exts,
                                    rasterPipeline_tgfxhnd* derivedPipeline);
  void (*destroyRasterPipeline)(rasterPipeline_tgfxhnd hnd);

  result_tgfx (*createComputePipeline)(shaderSource_tgfxhnd        Source,
                                       bindingTableType_tgfxlsthnd TypeBindingTables,
                                       unsigned char               isCallBufferSupported,
                                       computePipeline_tgfxhnd*    hnd);
  // Extensions: CallBufferInfo, Specialization Constants
  result_tgfx (*copyComputePipeline)(computePipeline_tgfxhnd src, extension_tgfxlsthnd exts,
                                     computePipeline_tgfxhnd* dst);
  void (*destroyComputePipeline)(computePipeline_tgfxhnd hnd);

  //////////////////////////////
  // RT SLOTSET
  //////////////////////////////

  result_tgfx (*createRTSlotset)(RTSlotDescription_tgfxlsthnd descriptions,
                                 RTSlotset_tgfxhnd*           slotsetHnd);
  void (*Delete_RTSlotSet)(RTSlotset_tgfxhnd RTSlotSetHandle);
  // Changes on RTSlots only happens at the frame slot is gonna be used
  result_tgfx (*changeRTSlot_Texture)(RTSlotset_tgfxhnd SlotSetHnd, unsigned char isColor,
                                      unsigned char SlotIndx, unsigned char FrameIndex,
                                      texture_tgfxhnd TextureHandle);
  result_tgfx (*inheriteRTSlotset)(RTSlotUsage_tgfxlsthnd descs, RTSlotset_tgfxhnd RTSlotSetHandle,
                                   inheritedRTSlotset_tgfxhnd* InheritedSlotSetHandle);
  void (*deleteInheritedRTSlotset)(inheritedRTSlotset_tgfxhnd hnd);

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