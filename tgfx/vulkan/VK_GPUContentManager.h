#pragma once
#include "Vulkan_Includes.h"
#include "Vulkan_Resource.h"


class vk_gpudatamanager {
public:
	tapi_threadlocal_vector<VK_VertexBuffer*> MESHBUFFERs;
	tapi_threadlocal_vector<VK_IndexBuffer*> INDEXBUFFERs;
	tapi_threadlocal_vector<VK_Texture*> TEXTUREs;
	tapi_threadlocal_vector<VK_GlobalBuffer*> GLOBALBUFFERs;
	tapi_threadlocal_vector<VK_ShaderSource*> SHADERSOURCEs;
	tapi_threadlocal_vector<VK_GraphicsPipeline*> SHADERPROGRAMs;
	tapi_threadlocal_vector<VK_PipelineInstance*> SHADERPINSTANCEs;
	tapi_threadlocal_vector<VK_ComputePipeline*> COMPUTETYPEs;
	tapi_threadlocal_vector<VK_ComputeInstance*> COMPUTEINSTANCEs;
	tapi_threadlocal_vector<VK_Sampler*> SAMPLERs;
	tapi_threadlocal_vector<VK_VertexAttribLayout*> VERTEXATTRIBLAYOUTs;
	tapi_threadlocal_vector<VK_RTSLOTSET*> RT_SLOTSETs;
	tapi_threadlocal_vector<VK_IRTSLOTSET*> IRT_SLOTSETs;
	tapi_threadlocal_vector<MemoryBlock*> STAGINGBUFFERs;

	//Suballocate and bind memory to VkBuffer object
	tgfx_result				Suballocate_Buffer(VkBuffer BUFFER, VkBufferUsageFlags UsageFlags, MemoryBlock& Block);
	//Suballocate and bind memory to VkImage object
	tgfx_result				Suballocate_Image(VK_Texture& Texture);


	//DESCRIPTOR RELATED DATAs

	//This vector contains descriptor sets that are for material types/instances that are created this frame
	tapi_threadlocal_vector<VK_DescSet*> DescSets_toCreate;
	//This vector contains descriptor sets that needs update
	//These desc sets are created before and is used in last 2 frames (So new descriptor set creation and updating it is necessary)
	tapi_threadlocal_vector<VK_DescSetUpdateCall> DescSets_toCreateUpdate;
	//These desc sets are not used recently in draw calls. So don't go to create-update-delete process, just update.
	tapi_threadlocal_vector<VK_DescSetUpdateCall> DescSets_toJustUpdate;
	//These are the desc sets that should be destroyed next frame!
	std::vector<VkDescriptorSet> UnboundMaterialDescSetList;
	unsigned int UnboundDescSetImageCount, UnboundDescSetSamplerCount, UnboundDescSetUBufferCount, UnboundDescSetSBufferCount;
	VK_DescPool MaterialRelated_DescPool;

	VkDescriptorPool GlobalShaderInputs_DescPool;
	VkDescriptorSet UnboundGlobalBufferDescSet, UnboundGlobalTextureDescSet; //These won't be deleted, just as a back buffer
	VK_DescSet GlobalBuffers_DescSet, GlobalTextures_DescSet;
	bool Create_DescSet(VK_DescSet* Set);
	//Create Global descriptor sets
	void Resource_Finalizations();

	//These are the textures that will be deleted after waiting for 2 frames ago's command buffer
	tapi_threadlocal_vector<VK_Texture*> DeleteTextureList;
	//These are the texture that will be added to the list above after clearing the above list
	tapi_threadlocal_vector<VK_Texture*> NextFrameDeleteTextureCalls;

	//Apply changes in Descriptor Sets, RTSlotSets
	void Apply_ResourceChanges();


	//INHERITANCE

	static tgfx_result Create_SamplingType(unsigned int MinimumMipLevel, unsigned int MaximumMipLevel,
		tgfx_texture_mipmapfilter MINFILTER, tgfx_texture_mipmapfilter MAGFILTER, tgfx_texture_wrapping WRAPPING_WIDTH,
		tgfx_texture_wrapping WRAPPING_HEIGHT, tgfx_texture_wrapping WRAPPING_DEPTH, tgfx_samplingtype* SamplingTypeHandle);

	/*Attributes are ordered as the same order of input vector
	* For example: Same attribute ID may have different location/order in another attribute layout
	* So you should gather your vertex buffer data according to that
	*/
	static tgfx_result Create_VertexAttributeLayout(const tgfx_datatype* Attributes, tgfx_vertexlisttypes listtype, 
		tgfx_vertexattributelayout* VertexAttributeLayoutHandle);
	static void Delete_VertexAttributeLayout(tgfx_vertexattributelayout VertexAttributeLayoutHandle);

	static tgfx_result Upload_toBuffer(tgfx_buffer Handle, tgfx_buffertype Type, const void* DATA, unsigned int DATA_SIZE, unsigned int OFFSET);

	static tgfx_result Create_StagingBuffer(unsigned int DATASIZE, unsigned int MemoryTypeIndex, tgfx_buffer* Handle);
	static void Delete_StagingBuffer(tgfx_buffer StagingBufferHandle);
	/*
	* You should sort your vertex data according to attribute layout, don't forget that
	* VertexCount shouldn't be 0
	*/
	static tgfx_result Create_VertexBuffer(tgfx_vertexattributelayout VertexAttributeLayoutHandle, unsigned int VertexCount,
		unsigned int MemoryTypeIndex, tgfx_buffer* VertexBufferHandle);
	static void Unload_VertexBuffer(tgfx_buffer BufferHandle);

	static tgfx_result Create_IndexBuffer(tgfx_datatype DataType, unsigned int IndexCount, unsigned int MemoryTypeIndex, tgfx_buffer* IndexBufferHandle);
	static void Unload_IndexBuffer(tgfx_buffer BufferHandle);

	static tgfx_result Create_Texture(tgfx_texture_dimensions DIMENSION, unsigned int WIDTH, unsigned int HEIGHT, tgfx_texture_channels CHANNEL_TYPE,
		unsigned char MIPCOUNT, tgfx_textureusageflag USAGE, tgfx_texture_order DATAORDER, unsigned int MemoryTypeIndex, tgfx_texture* TextureHandle);
	//TARGET OFFSET is the offset in the texture's buffer to copy to
	static tgfx_result Upload_Texture(tgfx_texture TextureHandle, const void* InputData, unsigned int DataSize, unsigned int TargetOffset);
	static void Delete_Texture(tgfx_texture TEXTUREHANDLE, unsigned char isUsedLastFrame);

	static tgfx_result Create_GlobalBuffer(const char* BUFFER_NAME, unsigned int DATA_SIZE, unsigned char isUniform, unsigned int MemoryTypeIndex, 
		tgfx_buffer* GlobalBufferHandle);
	static void Unload_GlobalBuffer(tgfx_buffer BUFFER_ID);
	static tgfx_result SetGlobalShaderInput_Buffer(unsigned char isUniformBuffer, unsigned int ElementIndex, unsigned char isUsedLastFrame,
		tgfx_buffer BufferHandle, unsigned int BufferOffset, unsigned int BoundDataSize);
	static tgfx_result SetGlobalShaderInput_Texture(unsigned char isSampledTexture, unsigned int ElementIndex, unsigned char isUsedLastFrame,
		tgfx_texture TextureHandle, tgfx_samplingtype sampler, tgfx_imageaccess access);


	static tgfx_result Compile_ShaderSource(tgfx_shaderlanguages language, tgfx_shaderstage shaderstage, void* DATA, unsigned int DATA_SIZE,
		tgfx_shadersource* ShaderSourceHandle);
	static void Delete_ShaderSource(tgfx_shadersource ShaderSourceHandle);
	static tgfx_result Link_MaterialType(tgfx_shadersource_list ShaderSources, tgfx_shaderinputdescription_list ShaderInputDescs,
		tgfx_vertexattributelayout AttributeLayout, tgfx_subdrawpass Subdrawpass, tgfx_cullmode culling,
		tgfx_polygonmode polygonmode, tgfx_depthsettings depthtest, tgfx_stencilsettings StencilFrontFaced,
		tgfx_stencilsettings StencilBackFaced, tgfx_blendinginfo_list BLENDINGINFOs, tgfx_rasterpipelinetype* MaterialHandle);
	static void Delete_MaterialType(tgfx_rasterpipelinetype ID);
	static tgfx_result Create_MaterialInst(tgfx_rasterpipelinetype MaterialType, tgfx_rasterpipelineinstance* MaterialInstHandle);
	static void Delete_MaterialInst(tgfx_rasterpipelineinstance ID);

	static tgfx_result Create_ComputeType(tgfx_shadersource Source, tgfx_shaderinputdescription_list ShaderInputDescs, tgfx_computeshadertype* ComputeTypeHandle);
	static tgfx_result Create_ComputeInstance(tgfx_computeshadertype ComputeType, tgfx_computeshaderinstance* ComputeShaderInstanceHandle);
	static void Delete_ComputeShaderType(tgfx_computeshadertype ID);
	static void Delete_ComputeShaderInstance(tgfx_computeshaderinstance ID);

	//IsUsedRecently means is the material type/instance used in last frame. This is necessary for Vulkan synchronization process.
	static tgfx_result SetMaterialType_Buffer(tgfx_rasterpipelinetype MaterialType, unsigned char isUsedRecently, unsigned int BINDINDEX,
		tgfx_buffer BufferHandle, tgfx_buffertype BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize);
	static tgfx_result SetMaterialType_Texture(tgfx_rasterpipelinetype MaterialType, unsigned char isUsedRecently, unsigned int BINDINDEX,
		tgfx_texture TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, tgfx_samplingtype Sampler, tgfx_imageaccess usage);

	static tgfx_result SetMaterialInst_Buffer(tgfx_rasterpipelineinstance MaterialInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
		tgfx_buffer BufferHandle, tgfx_buffertype BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize);
	static tgfx_result SetMaterialInst_Texture(tgfx_rasterpipelineinstance MaterialInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
		tgfx_texture TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, tgfx_samplingtype Sampler, tgfx_imageaccess usage);
	//IsUsedRecently means is the material type/instance used in last frame. This is necessary for Vulkan synchronization process.
	static tgfx_result SetComputeType_Buffer(tgfx_computeshadertype ComputeType, unsigned char isUsedRecently, unsigned int BINDINDEX,
		tgfx_buffer TargetBufferHandle, tgfx_buffertype BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize);
	static tgfx_result SetComputeType_Texture(tgfx_computeshadertype ComputeType, unsigned char isComputeType, unsigned char isUsedRecently, unsigned int BINDINDEX,
		tgfx_texture TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, tgfx_samplingtype Sampler, tgfx_imageaccess usage);

	static tgfx_result SetComputeInst_Buffer(tgfx_computeshaderinstance ComputeInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
		tgfx_buffer TargetBufferHandle, tgfx_buffertype BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize);
	static tgfx_result SetComputeInst_Texture(tgfx_computeshaderinstance ComputeInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
		tgfx_texture TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, tgfx_samplingtype Sampler, tgfx_imageaccess usage);


	static tgfx_result Create_RTSlotset(tgfx_rtslotdescription_list Descriptions, tgfx_rtslotset* RTSlotSetHandle);
	static void Delete_RTSlotSet(tgfx_rtslotset RTSlotSetHandle);
	//Changes on RTSlots only happens at the frame slot is gonna be used
	//For example; if you change next frame's slot, necessary API calls are gonna be called next frame
	//For example; if you change slot but related slotset isn't used by drawpass, it doesn't happen until it is used
	static tgfx_result Change_RTSlotTexture(tgfx_rtslotset RTSlotHandle, unsigned char isColorRT, unsigned char SlotIndex, unsigned char FrameIndex, tgfx_texture TextureHandle);
	static tgfx_result Inherite_RTSlotSet(tgfx_rtslotusage_list Descriptions, tgfx_rtslotset RTSlotSetHandle, tgfx_inheritedrtslotset* InheritedSlotSetHandle);
	static void Delete_InheritedRTSlotSet(tgfx_inheritedrtslotset InheritedRTSlotSetHandle);


	vk_gpudatamanager(InitializationSecondStageInfo& info);
	~vk_gpudatamanager();
	void Destroy_RenderGraphRelatedResources();
	static void Destroy_AllResources();
};