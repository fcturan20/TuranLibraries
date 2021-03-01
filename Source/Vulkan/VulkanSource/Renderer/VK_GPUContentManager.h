#pragma once
#include "Vulkan/VulkanSource/Vulkan_Includes.h"
#include "Vulkan_Resource.h"
#include "TuranAPI/ThreadedJobCore.h"


namespace Vulkan {

	class VK_API GPU_ContentManager : public GFX_API::GPU_ContentManager {
	public:
		TuranAPI::Threading::TLVector<VK_VertexBuffer*> MESHBUFFERs;
		TuranAPI::Threading::TLVector<VK_IndexBuffer*> INDEXBUFFERs;
		TuranAPI::Threading::TLVector<VK_Texture*> TEXTUREs;
		TuranAPI::Threading::TLVector<VK_GlobalBuffer*> GLOBALBUFFERs;
		TuranAPI::Threading::TLVector<VK_ShaderSource*> SHADERSOURCEs;
		TuranAPI::Threading::TLVector<VK_GraphicsPipeline*> SHADERPROGRAMs;
		TuranAPI::Threading::TLVector<VK_PipelineInstance*> SHADERPINSTANCEs;
		TuranAPI::Threading::TLVector<VK_Sampler*> SAMPLERs;
		TuranAPI::Threading::TLVector<VK_VertexAttribLayout*> VERTEXATTRIBLAYOUTs;
		TuranAPI::Threading::TLVector<VK_RTSLOTSET*> RT_SLOTSETs;
		TuranAPI::Threading::TLVector<VK_IRTSLOTSET*> IRT_SLOTSETs;
		TuranAPI::Threading::TLVector<MemoryBlock*> STAGINGBUFFERs;

		//Suballocate and bind memory to VkBuffer object
		TAPIResult				Suballocate_Buffer(VkBuffer BUFFER, VkBufferUsageFlags UsageFlags, MemoryBlock& Block);
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
		vector<VkDescriptorSet> UnboundDescSetList;
		unsigned int UnboundDescSetImageCount, UnboundDescSetSamplerCount, UnboundDescSetUBufferCount, UnboundDescSetSBufferCount;

		VK_DescPool MaterialRelated_DescPool;
		VkDescriptorPool GlobalBuffers_DescPool;
		VkDescriptorSet GlobalBuffers_DescSet;
		VkDescriptorSetLayout	GlobalBuffers_DescSetLayout;
		bool Create_DescSet(VK_DescSet* Set);

		//Create Global descriptor sets
		void Resource_Finalizations();


		//Apply changes in Descriptor Sets, RTSlotSets
		void Apply_ResourceChanges();





		//INHERITANCE

		GPU_ContentManager();
		virtual ~GPU_ContentManager();
		void Destroy_RenderGraphRelatedResources();
		virtual void Destroy_AllResources() override;


		virtual TAPIResult Create_SamplingType(GFX_API::TEXTURE_DIMENSIONs dimension, unsigned int MinimumMipLevel, unsigned int MaximumMipLevel, 
			GFX_API::TEXTURE_MIPMAPFILTER MINFILTER, GFX_API::TEXTURE_MIPMAPFILTER MAGFILTER, GFX_API::TEXTURE_WRAPPING WRAPPING_WIDTH, 
			GFX_API::TEXTURE_WRAPPING WRAPPING_HEIGHT, GFX_API::TEXTURE_WRAPPING WRAPPING_DEPTH, GFX_API::GFXHandle& SamplingTypeHandle) override;

		unsigned int Calculate_sizeofVertexLayout(const GFX_API::DATA_TYPE* ATTRIBUTEs, unsigned int Count);
		virtual TAPIResult Create_VertexAttributeLayout(const vector<GFX_API::DATA_TYPE>& Attributes, GFX_API::VERTEXLIST_TYPEs listtype, GFX_API::GFXHandle& Handle) override;
		virtual void Delete_VertexAttributeLayout(GFX_API::GFXHandle Layout_ID) override;

		virtual TAPIResult Upload_toBuffer(GFX_API::GFXHandle BufferHandle, GFX_API::BUFFER_TYPE BufferType, const void* DATA, unsigned int DATA_SIZE,
			unsigned int OFFSET) override;

		virtual TAPIResult Create_StagingBuffer(unsigned int DATASIZE, unsigned int MemoryTypeIndex, GFX_API::GFXHandle& Handle) override;
		virtual void Delete_StagingBuffer(GFX_API::GFXHandle StagingBufferHandle) override;

		virtual TAPIResult Create_VertexBuffer(GFX_API::GFXHandle AttributeLayout, unsigned int VertexCount, 
			unsigned int MemoryTypeIndex, GFX_API::GFXHandle& VertexBufferHandle) override;
		virtual void Unload_VertexBuffer(GFX_API::GFXHandle BufferHandle) override;


		virtual TAPIResult Create_IndexBuffer(GFX_API::DATA_TYPE DataType, unsigned int IndexCount, unsigned int MemoryTypeIndex, GFX_API::GFXHandle& IndexBufferHandle) override;
		virtual void Unload_IndexBuffer(GFX_API::GFXHandle BufferHandle) override;


		virtual TAPIResult Create_Texture(const GFX_API::Texture_Description& TEXTURE_ASSET, unsigned int MemoryTypeIndex, GFX_API::GFXHandle& TextureHandle) override;
		virtual TAPIResult Upload_Texture(GFX_API::GFXHandle BufferHandle, const void* InputData,
			unsigned int DataSize, unsigned int TargetOffset) override;
		virtual void Unload_Texture(GFX_API::GFXHandle TEXTUREHANDLE) override;


		virtual TAPIResult Create_GlobalBuffer(const char* BUFFER_NAME, unsigned int DATA_SIZE, unsigned int BINDINDEX, bool isUniform,
			GFX_API::SHADERSTAGEs_FLAG AccessableStages, unsigned int MemoryTypeIndex, GFX_API::GFXHandle& GlobalBufferHandle) override;
		virtual void Unload_GlobalBuffer(GFX_API::GFXHandle BUFFER_ID) override;


		virtual TAPIResult Compile_ShaderSource(const GFX_API::ShaderSource_Resource* SHADER, GFX_API::GFXHandle& ShaderSourceHandle) override;
		virtual void Delete_ShaderSource(GFX_API::GFXHandle ASSET_ID) override;
		virtual TAPIResult Compile_ComputeShader(GFX_API::ComputeShader_Resource* SHADER, GFX_API::GFXHandle* Handles, unsigned int Count) override;
		virtual void Delete_ComputeShader(GFX_API::GFXHandle ASSET_ID) override;
		virtual TAPIResult Link_MaterialType(const GFX_API::Material_Type& MATTYPE_ASSET, GFX_API::GFXHandle& MaterialHandle) override;
		virtual void Delete_MaterialType(GFX_API::GFXHandle Asset_ID) override;
		virtual TAPIResult Create_MaterialInst(GFX_API::GFXHandle MaterialType, GFX_API::GFXHandle& MaterialInstHandle) override;
		virtual void Delete_MaterialInst(GFX_API::GFXHandle Asset_ID) override;
		virtual TAPIResult SetMaterial_UniformBuffer(GFX_API::GFXHandle MaterialType_orInstance, bool isMaterialType, bool isUsedRecently, unsigned int BINDINDEX, 
			GFX_API::GFXHandle TargetBufferHandle, unsigned int ELEMENTINDEX, GFX_API::BUFFER_TYPE BufferType, unsigned int TargetOffset, unsigned int BoundDataSize) override;
		virtual TAPIResult SetMaterial_SampledTexture(GFX_API::GFXHandle MaterialType_orInstance, bool isMaterialType, bool isUsedRecently, unsigned int BINDINDEX,
			unsigned int ELEMENTINDEX, GFX_API::GFXHandle TextureHandle, GFX_API::GFXHandle SamplingType, GFX_API::IMAGE_ACCESS usage) override;
		virtual TAPIResult SetMaterial_ImageTexture(GFX_API::GFXHandle MaterialType_orInstance, bool isMaterialType, bool isUsedRecently, unsigned int BINDINDEX,
			unsigned int ELEMENTINDEX, GFX_API::GFXHandle TextureHandle, GFX_API::GFXHandle SamplingType, GFX_API::IMAGE_ACCESS usage) override;

		virtual TAPIResult Create_RTSlotset(const vector<GFX_API::RTSLOT_Description>& Descriptions, GFX_API::GFXHandle& RTSlotSetHandle) override;
		virtual TAPIResult Change_RTSlotTexture(GFX_API::GFXHandle RTSlotHandle, bool isColorRT, unsigned char SlotIndex, unsigned char FrameIndex, GFX_API::GFXHandle TextureHandle) override;
		virtual TAPIResult Inherite_RTSlotSet(const vector<GFX_API::RTSLOTUSAGE_Description>& Descriptions, GFX_API::GFXHandle RTSlotSetHandle, GFX_API::GFXHandle& InheritedSlotSetHandle) override;
	};
}