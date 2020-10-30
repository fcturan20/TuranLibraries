#pragma once
#include "Vulkan/VulkanSource/Vulkan_Includes.h"
#include "Vulkan_Resource.h"


namespace Vulkan {
	class VK_API GPU_ContentManager : public GFX_API::GPU_ContentManager {
	public:
		vector<VK_Mesh*> MESHBUFFERs;
		vector<VK_Texture*> TEXTUREs;
		vector<VK_GlobalBuffer*> GLOBALBUFFERs;
		vector<VK_ShaderSource*> SHADERSOURCEs;
		vector<VK_GraphicsPipeline*> SHADERPROGRAMs;
		vector<VK_PipelineInstance*> SHADERPINSTANCEs;
		vector<VK_VertexAttribute*> VERTEXATTRIBUTEs;
		vector<VK_VertexAttribLayout*> VERTEXATTRIBLAYOUTs;
		vector<VK_RTSLOTSET*> RT_SLOTSETs;


		VK_MemoryAllocation		STAGINGBUFFERALLOC;
		VK_MemoryAllocation		GPULOCAL_BUFFERALLOC;
		VK_MemoryAllocation		GPULOCAL_TEXTUREALLOC;
		//Suballocate and bind memory to VkBuffer object
		void					Suballocate_Buffer(VkBuffer BUFFER, SUBALLOCATEBUFFERTYPEs GPUregion, VkDeviceSize& MemoryOffset);
		//Suballocate and bind memory to VkImage object
		void					Suballocate_Image(VkImage IMAGE, SUBALLOCATEBUFFERTYPEs GPUregion, VkDeviceSize& MemoryOffset);
		//Suballocate a memory block to safely copy data
		VkDeviceSize			Get_StagingBufferOffset(unsigned int datasize);
		void					CopyData_toStagingMemory(const void* data, unsigned int data_size, VkDeviceSize offset);
		void					Clear_StagingMemory();

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

		virtual GFX_API::GFXHandle Create_VertexAttribute(GFX_API::DATA_TYPE TYPE, bool is_perVertex) override;
		virtual bool Delete_VertexAttribute(GFX_API::GFXHandle Attribute_ID) override;

		unsigned int Calculate_sizeofVertexLayout(const vector<VK_VertexAttribute*>& ATTRIBUTEs);
		virtual GFX_API::GFXHandle Create_VertexAttributeLayout(const vector<GFX_API::GFXHandle>& Attributes) override;
		virtual void Delete_VertexAttributeLayout(GFX_API::GFXHandle Layout_ID) override;

		virtual GFX_API::GFXHandle Create_MeshBuffer(GFX_API::GFXHandle attributelayout, const void* vertex_data, unsigned int vertex_count,
			const unsigned int* index_data, unsigned int index_count, GFX_API::GFXHandle TransferPassHandle) override;
		virtual void Upload_MeshBuffer(GFX_API::GFXHandle MeshBufferID, const void* vertex_data, const void* index_data) override;
		//When you call this function, Draw Calls that uses this ID may draw another Mesh or crash
		//Also if you have any Point Buffer that uses first vertex attribute of that Mesh Buffer, it may crash or draw any other buffer
		virtual void Unload_MeshBuffer(GFX_API::GFXHandle MeshBuffer_ID) override;

		virtual GFX_API::GFXHandle Create_Texture(const GFX_API::Texture_Description& TEXTURE_ASSET, GFX_API::GFXHandle TransferPassID) override;
		virtual void Upload_Texture(GFX_API::GFXHandle TEXTUREHANDLE, void* DATA, unsigned int DATA_SIZE) override;
		virtual void Unload_Texture(GFX_API::GFXHandle TEXTUREHANDLE) override;


		virtual GFX_API::GFXHandle Create_GlobalBuffer(const char* BUFFER_NAME, void* DATA, unsigned int DATA_SIZE, GFX_API::BUFFER_VISIBILITY USAGE) override;
		virtual void Upload_GlobalBuffer(GFX_API::GFXHandle BUFFER_ID, void* DATA = nullptr, unsigned int DATA_SIZE = 0) override;
		virtual void Unload_GlobalBuffer(GFX_API::GFXHandle BUFFER_ID) override;


		virtual GFX_API::GFXHandle Compile_ShaderSource(GFX_API::ShaderSource_Resource* SHADER) override;
		virtual void Delete_ShaderSource(GFX_API::GFXHandle ASSET_ID) override;
		virtual GFX_API::GFXHandle Compile_ComputeShader(GFX_API::ComputeShader_Resource* SHADER) override;
		virtual void Delete_ComputeShader(GFX_API::GFXHandle ASSET_ID) override;
		virtual GFX_API::GFXHandle Link_MaterialType(GFX_API::Material_Type* MATTYPE_ASSET) override;
		virtual void Delete_MaterialType(GFX_API::GFXHandle Asset_ID) override;
		virtual GFX_API::GFXHandle Create_MaterialInst(GFX_API::Material_Instance* MATINST_ASSET) override;
		virtual void Delete_MaterialInst(GFX_API::GFXHandle Asset_ID) override;

		virtual GFX_API::GFXHandle Create_RTSlotSet(const vector<GFX_API::RTSLOT_Description>& Description) override;
		virtual GFX_API::GFXHandle Inherite_RTSlotSet(const GFX_API::GFXHandle SLOTSETHandle, const vector<GFX_API::IRTSLOT_Description>& InheritanceInfo) override;
	};
}