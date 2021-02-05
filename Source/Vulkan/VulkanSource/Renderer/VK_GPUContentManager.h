#pragma once
#include "Vulkan/VulkanSource/Vulkan_Includes.h"
#include "Vulkan_Resource.h"
#include "TuranAPI/ThreadedJobCore.h"


namespace Vulkan {

	class VK_API GPU_ContentManager : public GFX_API::GPU_ContentManager {
	public:
		TuranAPI::Threading::TLVector<VK_VertexBuffer*> MESHBUFFERs;
		TuranAPI::Threading::TLVector<VK_Texture*> TEXTUREs;
		TuranAPI::Threading::TLVector<VK_GlobalBuffer*> GLOBALBUFFERs;
		TuranAPI::Threading::TLVector<VK_ShaderSource*> SHADERSOURCEs;
		TuranAPI::Threading::TLVector<VK_GraphicsPipeline*> SHADERPROGRAMs;
		TuranAPI::Threading::TLVector<VK_PipelineInstance*> SHADERPINSTANCEs;
		TuranAPI::Threading::TLVector<VK_VertexAttribute*> VERTEXATTRIBUTEs;
		TuranAPI::Threading::TLVector<VK_VertexAttribLayout*> VERTEXATTRIBLAYOUTs;
		TuranAPI::Threading::TLVector<VK_RTSLOTSET*> RT_SLOTSETs;

		//Suballocate and bind memory to VkBuffer object
		TAPIResult				Suballocate_Buffer(VkBuffer BUFFER, GFX_API::SUBALLOCATEBUFFERTYPEs GPUregion, VkDeviceSize& MemoryOffset, VkDeviceSize& RequiredSize, bool ShouldBind);
		//Suballocate and bind memory to VkImage object
		TAPIResult				Suballocate_Image(VK_Texture& Texture);

		//This vector contains descriptor sets that are for material types/instances that are created this frame
		TuranAPI::Threading::TLVector<VK_DescSet*> DescSets_toCreate;
		//This vector contains descriptor sets that needs update
		//These desc sets are created before and is used in last 2 frames (So new descriptor set creation and updating it is necessary)
		TuranAPI::Threading::TLVector<VK_DescSetUpdateCall> DescSets_toCreateUpdate;
		//These desc sets are not used recently in draw calls. So don't go to create-update-delete process, just update.
		TuranAPI::Threading::TLVector<VK_DescSetUpdateCall> DescSets_toJustUpdate;
		//These are the desc sets that should be destroyed 2 frames later!
		vector<VkDescriptorSet> UnboundDescSetList[2];
		unsigned int UnboundDescSetImageCount[2], UnboundDescSetSamplerCount[2], UnboundDescSetUBufferCount[2], UnboundDescSetSBufferCount[2];
		VK_DescPool MaterialRelated_DescPool;
		VkDescriptorPool GlobalBuffers_DescPool;
		VkDescriptorSet GlobalBuffers_DescSet;
		VkDescriptorSetLayout	GlobalBuffers_DescSetLayout;
		bool Create_DescSet(VK_DescSet* Set);

		//Create Global descriptor sets
		void Resource_Finalizations();

		VkSampler DefaultSampler;






		//INHERITANCE

		GPU_ContentManager();
		virtual ~GPU_ContentManager();
		virtual void Unload_AllResources() override;

		virtual TAPIResult Create_VertexAttribute(const GFX_API::DATA_TYPE& TYPE, const bool& is_perVertex, GFX_API::GFXHandle& Handle) override;
		virtual bool Delete_VertexAttribute(GFX_API::GFXHandle Attribute_ID) override;

		unsigned int Calculate_sizeofVertexLayout(const VK_VertexAttribute* const* ATTRIBUTEs, unsigned int Count);
		virtual TAPIResult Create_VertexAttributeLayout(const vector<GFX_API::GFXHandle>& Attributes, GFX_API::GFXHandle& Handle) override;
		virtual void Delete_VertexAttributeLayout(GFX_API::GFXHandle Layout_ID) override;

		virtual TAPIResult Create_StagingBuffer(unsigned int DATASIZE, const GFX_API::SUBALLOCATEBUFFERTYPEs& MemoryRegion, GFX_API::GFXHandle& Handle) override;
		virtual TAPIResult Uploadto_StagingBuffer(GFX_API::GFXHandle StagingBufferHandle, const void* DATA, unsigned int DATA_SIZE, unsigned int OFFSET) override;
		virtual void Delete_StagingBuffer(GFX_API::GFXHandle StagingBufferHandle) override;

		virtual TAPIResult Create_VertexBuffer(GFX_API::GFXHandle AttributeLayout, unsigned int VertexCount, 
			GFX_API::SUBALLOCATEBUFFERTYPEs MemoryType, GFX_API::GFXHandle& VertexBufferHandle) override;
		virtual TAPIResult Upload_VertexBuffer(GFX_API::GFXHandle BufferHandle, const void* InputData, 
			unsigned int DataSize, unsigned int TargetOffset) override;
		virtual void Unload_VertexBuffer(GFX_API::GFXHandle BufferHandle) override;


		virtual TAPIResult Create_IndexBuffer(unsigned int DataSize, GFX_API::SUBALLOCATEBUFFERTYPEs MemoryType, GFX_API::GFXHandle& IndexBufferHandle) override;
		virtual TAPIResult Upload_IndexBuffer(GFX_API::GFXHandle BufferHandle, const void* InputData,
			unsigned int DataSize, unsigned int TargetOffset) override;
		virtual void Unload_IndexBuffer(GFX_API::GFXHandle BufferHandle) override;


		virtual TAPIResult Create_Texture(const GFX_API::Texture_Resource& TEXTURE_ASSET, GFX_API::SUBALLOCATEBUFFERTYPEs MemoryType, GFX_API::GFXHandle& TextureHandle) override;
		virtual TAPIResult Upload_Texture(GFX_API::GFXHandle BufferHandle, const void* InputData,
			unsigned int DataSize, unsigned int TargetOffset) override;
		virtual void Unload_Texture(GFX_API::GFXHandle TEXTUREHANDLE) override;


		virtual TAPIResult Create_GlobalBuffer(const char* BUFFER_NAME, unsigned int DATA_SIZE, unsigned int BINDINDEX, bool isUniform,
			GFX_API::SHADERSTAGEs_FLAG AccessableStages, GFX_API::SUBALLOCATEBUFFERTYPEs MemoryType, GFX_API::GFXHandle& GlobalBufferHandle) override;
		virtual TAPIResult Upload_GlobalBuffer(GFX_API::GFXHandle BufferHandle, const void* InputData,
			unsigned int DataSize, unsigned int TargetOffset) override;
		virtual void Unload_GlobalBuffer(GFX_API::GFXHandle BUFFER_ID) override;


		virtual TAPIResult Compile_ShaderSource(const GFX_API::ShaderSource_Resource* SHADER, GFX_API::GFXHandle& ShaderSourceHandle) override;
		virtual void Delete_ShaderSource(GFX_API::GFXHandle ASSET_ID) override;
		virtual TAPIResult Compile_ComputeShader(GFX_API::ComputeShader_Resource* SHADER, GFX_API::GFXHandle* Handles, unsigned int Count) override;
		virtual void Delete_ComputeShader(GFX_API::GFXHandle ASSET_ID) override;
		virtual TAPIResult Link_MaterialType(const GFX_API::Material_Type& MATTYPE_ASSET, GFX_API::GFXHandle& MaterialHandle) override;
		virtual void Delete_MaterialType(GFX_API::GFXHandle Asset_ID) override;
		virtual TAPIResult Create_MaterialInst(GFX_API::GFXHandle MaterialType, GFX_API::GFXHandle& MaterialInstHandle) override;
		virtual void Delete_MaterialInst(GFX_API::GFXHandle Asset_ID) override;
		virtual void SetMaterial_UniformBuffer(GFX_API::GFXHandle MaterialType_orInstance, bool isMaterialType, bool isUsedRecently, unsigned int BINDINDEX, GFX_API::GFXHandle TargetBufferHandle,
			GFX_API::BUFFER_TYPE BufferType, unsigned int TargetOffset) override;

		virtual TAPIResult Create_RTSlotset(const vector<GFX_API::RTSLOT_Description>& Descriptions, GFX_API::GFXHandle& RTSlotSetHandle) override;
		virtual TAPIResult Inherite_RTSlotSet(const vector<GFX_API::RTSLOTUSAGE_Description>& Descriptions, GFX_API::GFXHandle RTSlotSetHandle, GFX_API::GFXHandle& InheritedSlotSetHandle) override;
	};
}