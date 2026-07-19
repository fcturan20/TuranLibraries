#pragma once
#include <TCore.h>
#include <TGfxDeclarations.h>
TCORE_BEGIN_C_LINKAGE

typedef struct ITGfxResourceManager
{
	void (*DestroyAllResources)();

	// If sampler is used as constant;
	//	DX12 limits bordercolor to be vec4(0), vec4(0,0,0,1) and vec4(1)
	TCResult (*CreateSampler)(TGfxGpu gpu, const TGfxSamplerDescription* desc, TGfxSampler* hnd);
	void (*DestroySampler)(TGfxSampler sampler);

	// Extension can be UNIFORMBUFFER
	TCResult (*CreateBuffer)(TGfxGpu gpu, const TGfxBufferDescription* desc, TGfxBuffer* handle);
	void (*DestroyBuffer)(TGfxBuffer buffer);

	//	If your GPU supports buffer_GPUaddress_pointers, you can use this.
	//		Otherwise this function pointer is NULL
	//	You can pass pointers to buffers (call buffer data or classic GPU buffers)
	//		and use complex data structures and access strategies in shaders
	TCResult (*GetBufferPointer_GPU)(TGfxBuffer h, TSize* ptr);

	TCResult (*CreateTexture)(TGfxGpu gpu, const TGfxTextureDescription* desc, TGfxTexture* textureHnd);
	void (*DestroyTexture)(TGfxTexture textureHnd);

	// BINDING TABLES
	////////////////////////////////////

	// If descType is sampler, SttcSmplrs can be used at binding index 0
	TCResult (*CreateBindingTable)(TGfxGpu gpu, const TGfxBindingTableDescription* desc, TGfxBindingTable* table);
	void (*DestroyBindingTable)(TGfxBindingTable bindingTable);
	TCResult (*SetBindingTable_Texture)(TGfxBindingTable table,
										TU4 bindingCount,
										const TU4* bindingIndices,
										TGfxTexture const* textures);
	// If offsets is nullptr, then all offsets are 0
	// If sizes is nullptr, then all sizes are whole buffer
	TCResult (*SetBindingTable_Buffer)(TGfxBindingTable table,
									   TU4 bindingCount,
									   const TU4* bindingIndices,
									   TGfxBuffer const* buffers,
									   const TU8* offsets,
									   const TU8* sizes,
									   TGfxExtension* exts);
	TCResult (*SetBindingTable_Sampler)(TGfxBindingTable table,
										TU4 bindingCount,
										const TU4* bindingIndices,
										TGfxSampler const* samplers);

	// SHADER & PIPELINE COMPILATION
	/////////////////////////////////////

	TCResult (*CompileShaderSource)(TGfxGpu gpu,
									TGfxShaderLanguage language,
									TGfxShaderStage shaderstage,
									const void* data,
									TU4 size,
									TGfxShaderSource* oShaderSource);
	void (*DestroyShaderSource)(TGfxShaderSource shader);
	// Extensions: CallBufferInfo, Subpass, StaticRasterState
	TCResult (*CreateRasterPipeline)(const TGfxRasterPipelineDescription* desc, TGfxPipeline* oPipeline);
	// Extensions: Dynamic States, CallBufferInfo, Specialization Constants
	TCResult (*CopyRasterPipeline)(TGfxPipeline basePipeline, TGfxExtension* exts, TGfxPipeline* derivedPipeline);

	TCResult (*CreateComputePipeline)(TGfxShaderSource Source,
									  TU4 bindingTableCount,
									  const TGfxBindingTableDescription* bindingTableDescs,
									  TU1 pushConstantOffset,
									  TU1 pushConstantSize,
									  TGfxPipeline* hnd);
	// Extensions: CallBufferInfo, Specialization Constants
	TCResult (*CopyComputePipeline)(TGfxPipeline src, TGfxPipeline dst, TGfxExtension* exts);
	void (*DestroyPipeline)(TGfxPipeline pipeline);

	//////////////////////////////
	// MEMORY
	//////////////////////////////

	// Extensions: Dedicated Memory Allocation
	TCResult (*CreateHeap)(TGfxGpu gpu, TU1 memoryRegionID, TSize heapSize, TGfxExtension* exts, TGfxHeap* heap);
	TCResult (*GetHeapRequirement_Texture)(TGfxTexture texture, TGfxExtension* exts, TGfxHeapRequirementsInfo* reqs);
	TCResult (*GetHeapRequirement_Buffer)(TGfxBuffer buffer, TGfxExtension* exts, TGfxHeapRequirementsInfo* reqs);
	// @return FAIL if this feature isn't supported
	TCResult (*GetRemainingMemory)(TGfxGpu gpu, TU1 memoryRegionID, TGfxExtension* exts, TSize* size);
	TCResult (*BindToHeap_Buffer)(TGfxHeap heap, TSize offset, TGfxBuffer buffer, TGfxExtension* exts);
	TCResult (*BindToHeap_Texture)(TGfxHeap heap, TSize offset, TGfxTexture texture, TGfxExtension* exts);
	// You can only map one part of a heap at a time
	// Unmap the heap if you want to map another part of it
	// @param size: UINT64_MAX if you want to map the whole heap
	TCResult (*MapHeap)(TGfxHeap heap, TSize offset, TSize size, TGfxExtension* exts, void** mappedRegion);
	TCResult (*UnmapHeap)(TGfxHeap heap);
} ITGfxResourceManager;

TCORE_END_C_LINKAGE