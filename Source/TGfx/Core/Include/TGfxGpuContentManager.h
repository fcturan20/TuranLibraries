#pragma once
#include <TCore.h>
#include <TGfxDeclarations.h>
TCORE_BEGIN_C_LINKAGE

struct tgfx_gpuDataManager
{
	void (*DestroyAllResources)();

	// If sampler is used as constant;
	//	DX12 limits bordercolor to be vec4(0), vec4(0,0,0,1) and vec4(1)
	TCResult (*CreateSampler)(TGfxGpuHnd gpu, const struct tgfx_samplerDescription* desc, struct tgfx_sampler** hnd);
	void (*DestroySampler)(struct tgfx_sampler* sampler);

	// Extension can be UNIFORMBUFFER
	TCResult (*CreateBuffer)(TGfxGpuHnd gpu, const struct tgfx_bufferDescription* desc, struct tgfx_buffer** handle);
	void (*DestroyBuffer)(struct tgfx_buffer* bufferHnd);

	//	If your GPU supports buffer_GPUaddress_pointers, you can use this.
	//		Otherwise this function pointer is NULL
	//	You can pass pointers to buffers (call buffer data or classic GPU buffers)
	//		and use complex data structures and access strategies in shaders
	TCResult (*GetBufferPointer_GPU)(struct tgfx_buffer* h, unsigned long long* ptr);

	TCResult (*CreateTexture)(TGfxGpuHnd gpu,
							  const struct tgfx_textureDescription* desc,
							  struct tgfx_texture** textureHnd);
	void (*DestroyTexture)(struct tgfx_texture* textureHnd);

	// BINDING TABLES
	////////////////////////////////////

	// If descType is sampler, SttcSmplrs can be used at binding index 0
	TCResult (*CreateBindingTable)(TGfxGpuHnd gpu,
								   const struct tgfx_bindingTableDescription* desc,
								   struct tgfx_bindingTable** table);
	void (*DestroyBindingTable)(struct tgfx_bindingTable* bindingTable);
	TCResult (*SetBindingTable_Texture)(struct tgfx_bindingTable* table,
										unsigned int bindingCount,
										const unsigned int* bindingIndices,
										struct tgfx_texture* const* textures);
	// If offsets is nullptr, then all offsets are 0
	// If sizes is nullptr, then all sizes are whole buffer
	TCResult (*SetBindingTable_Buffer)(struct tgfx_bindingTable* table,
									   unsigned int bindingCount,
									   const unsigned int* bindingIndices,
									   struct tgfx_buffer* const* buffers,
									   const unsigned int* offsets,
									   const unsigned int* sizes,
									   unsigned int extCount,
									   struct tgfx_extension* const* exts);
	TCResult (*SetBindingTable_Sampler)(struct tgfx_bindingTable* table,
										unsigned int bindingCount,
										const unsigned int* bindingIndices,
										struct tgfx_sampler* const* samplers);

	// SHADER & PIPELINE COMPILATION
	/////////////////////////////////////

	TCResult (*CompileShaderSource)(TGfxGpuHnd gpu,
									enum shaderlanguages_tgfx language,
									enum shaderStage_tgfx shaderstage,
									const void* DATA,
									unsigned int DATA_SIZE,
									struct tgfx_shaderSource** ShaderSourceHnd);
	void (*DestroyShaderSource)(struct tgfx_shaderSource* ShaderSourceHnd);
	// Extensions: CallBufferInfo, Subpass, StaticRasterState
	TCResult (*CreateRasterPipeline)(const struct tgfx_rasterPipelineDescription* desc, struct tgfx_pipeline** hnd);
	// Extensions: Dynamic States, CallBufferInfo, Specialization Constants
	TCResult (*CopyRasterPipeline)(struct tgfx_pipeline* basePipeline,
								   struct tgfx_extension* const* exts,
								   struct tgfx_pipeline** derivedPipeline);

	TCResult (*CreateComputePipeline)(struct tgfx_shaderSource* Source,
									  unsigned int bindingTableCount,
									  const tgfx_bindingTableDescription* bindingTableDescs,
									  unsigned char pushConstantOffset,
									  unsigned char pushConstantSize,
									  struct tgfx_pipeline** hnd);
	// Extensions: CallBufferInfo, Specialization Constants
	TCResult (*CopyComputePipeline)(struct tgfx_pipeline* src,
									struct tgfx_extension* const* exts,
									struct tgfx_pipeline** dst);
	void (*DestroyPipeline)(struct tgfx_pipeline* pipeline);

	//////////////////////////////
	// MEMORY
	//////////////////////////////

	// Extensions: Dedicated Memory Allocation
	TCResult (*CreateHeap)(TGfxGpuHnd gpu,
						   unsigned char memoryRegionID,
						   unsigned long long heapSize,
						   unsigned int extCount,
						   struct tgfx_extension* const* exts,
						   struct tgfx_heap** heap);
	TCResult (*GetHeapRequirement_Texture)(struct tgfx_texture* texture,
										   unsigned int extCount,
										   struct tgfx_extension* const* exts,
										   struct tgfx_heapRequirementsInfo* reqs);
	TCResult (*GetHeapRequirement_Buffer)(TGfxBufferHnd buffer,
										  unsigned int extCount,
										  struct tgfx_extension* const* exts,
										  struct tgfx_heapRequirementsInfo* reqs);
	// @return FAIL if this feature isn't supported
	TCResult (*GetRemainingMemory)(TGfxGpuHnd gpu,
								   unsigned char memoryRegionID,
								   unsigned int extCount,
								   struct tgfx_extension* const* exts,
								   unsigned long long* size);
	TCResult (*BindToHeap_Buffer)(struct tgfx_heap* heap,
								  unsigned long long offset,
								  struct tgfx_buffer* buffer,
								  unsigned int extCount,
								  struct tgfx_extension* const* exts);
	TCResult (*BindToHeap_Texture)(struct tgfx_heap* heap,
								   unsigned long long offset,
								   struct tgfx_texture* texture,
								   unsigned int extCount,
								   struct tgfx_extension* const* exts);
	// You can only map one part of a heap at a time
	// Unmap the heap if you want to map another part of it
	// @param size: UINT64_MAX if you want to map the whole heap
	TCResult (*MapHeap)(struct tgfx_heap* heap,
						unsigned long long offset,
						unsigned long long size,
						unsigned int extCount,
						struct tgfx_extension* const* exts,
						void** mappedRegion);
	TCResult (*UnmapHeap)(struct tgfx_heap* heap);
};

TCORE_END_C_LINKAGE