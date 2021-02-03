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
		TAPIResult				Suballocate_Buffer(VkBuffer BUFFER, GFX_API::SUBALLOCATEBUFFERTYPEs GPUregion, VkDeviceSize& MemoryOffset);
		//Suballocate and bind memory to VkImage object
		TAPIResult				Suballocate_Image(VK_Texture& Texture);

		VkDescriptorPool		GlobalBuffers_DescPool;
		VkDescriptorSet			GlobalBuffers_DescSet;
		VkDescriptorSetLayout	GlobalBuffers_DescSetLayout;


		vector<VK_DescPool> MaterialData_DescPools;
		//Creates same set by SetCount times! Sets should be a pointer to an array that's at SetCount size!
		bool Create_DescSets(const VK_DescSetLayout& Layout, VkDescriptorSet* Sets, unsigned int SetCount);

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


		virtual TAPIResult Create_GlobalBuffer(const char* BUFFER_NAME, unsigned int DATA_SIZE, 
			GFX_API::SUBALLOCATEBUFFERTYPEs MemoryType, GFX_API::GFXHandle& GlobalBufferHandle) override;
		virtual TAPIResult Upload_GlobalBuffer(GFX_API::GFXHandle BufferHandle, const void* InputData,
			unsigned int DataSize, unsigned int TargetOffset) override;
		virtual void Unload_GlobalBuffer(GFX_API::GFXHandle BUFFER_ID) override;


		virtual TAPIResult Compile_ShaderSource(const GFX_API::ShaderSource_Resource* SHADER, GFX_API::GFXHandle& ShaderSourceHandle) override;
		virtual void Delete_ShaderSource(GFX_API::GFXHandle ASSET_ID) override;
		virtual TAPIResult Compile_ComputeShader(GFX_API::ComputeShader_Resource* SHADER, GFX_API::GFXHandle* Handles, unsigned int Count) override;
		virtual void Delete_ComputeShader(GFX_API::GFXHandle ASSET_ID) override;
		virtual TAPIResult Link_MaterialType(const GFX_API::Material_Type& MATTYPE_ASSET, GFX_API::GFXHandle& MaterialHandle) override;
		virtual void Delete_MaterialType(GFX_API::GFXHandle Asset_ID) override;
		virtual TAPIResult Create_MaterialInst(const GFX_API::Material_Instance& MATINST_ASSET, GFX_API::GFXHandle& MaterialInstHandle) override;
		virtual void Delete_MaterialInst(GFX_API::GFXHandle Asset_ID) override;

		virtual TAPIResult Create_RTSlotset(const vector<GFX_API::RTSLOT_Description>& Descriptions, GFX_API::GFXHandle& RTSlotSetHandle) override;
		virtual TAPIResult Inherite_RTSlotSet(const vector<GFX_API::RTSLOTUSAGE_Description>& Descriptions, GFX_API::GFXHandle RTSlotSetHandle, GFX_API::GFXHandle& InheritedSlotSetHandle) override;
	};
}