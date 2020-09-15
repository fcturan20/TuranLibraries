#pragma once
#include "Vulkan/VulkanSource/Vulkan_Includes.h"
#include "Vulkan_Resource.h"


namespace Vulkan {
	//This system also has a VRAM allocation system
	class VK_API GPU_ContentManager : public GFX_API::GPU_ContentManager {
	public:
		VkCommandPool TransferCommandBuffersPool;
		VkCommandBuffer TransferCommandBuffer;
		void Start_TransferCB();
		void End_TransferCB();

		VK_MemoryAllocation		STAGINGBUFFERALLOC;
		vector<VK_MemoryBlock>	STAGINGBLOCKs;
		//Returns start offset of the block
		VkDeviceSize			Create_StagingMemoryBlock(unsigned int size);
		void					Clear_StagingMemory();

		VK_MemoryAllocation GPULOCAL_BUFFERALLOC;
		vector<VK_MemoryBlock> GPULOCALBLOCKs;
		//Returns start offset of the block
		VkDeviceSize Create_GPULocalMemoryBlock(unsigned int size);


		VkDescriptorPool		GlobalBuffers_DescPool;
		VkDescriptorSet			GlobalBuffers_DescSet;
		VkDescriptorSetLayout	GlobalBuffers_DescSetLayout;

		vector<VK_DescPool> MaterialData_DescPools;
		vector<VkDescriptorPoolSize> DataCount_PerType_PerPool;
		//Creates same set by SetCount times! Set should be a pointer to an array that's at SetCount size!
		//Return true, if it is successful
		bool Create_DescSets(vector<GFX_API::MaterialDataDescriptor>& GFXMatDataDescs, const VkDescriptorSetLayout* DescSetLay, VkDescriptorSet* Set, unsigned int SetCount);

		VkBuffer Create_VkBuffer(unsigned int size, VkBufferUsageFlags usage);

		void Create_RenderGraphResources();
		//INHERITANCE




		GPU_ContentManager();
		virtual ~GPU_ContentManager();
		virtual void Unload_AllResources() override;

		virtual bool Delete_VertexAttribute(unsigned int Attribute_ID) override;

		unsigned int Calculate_sizeofVertexLayout(vector<GFX_API::VertexAttribute*>& ATTRIBUTEs);
		virtual unsigned int Create_VertexAttributeLayout(vector<unsigned int> Attributes) override;
		virtual void Delete_VertexAttributeLayout(unsigned int Layout_ID) override;

		//Return MeshBufferID to use in Draw Calls
		virtual unsigned int Upload_MeshBuffer(unsigned int attributelayout, const void* vertex_data, unsigned int data_size, unsigned int vertex_count,
			const void* index_data, unsigned int index_count) override;
		//When you call this function, Draw Calls that uses this ID may draw another Mesh or crash
		//Also if you have any Point Buffer that uses first vertex attribute of that Mesh Buffer, it may crash or draw any other buffer
		virtual void Unload_MeshBuffer(unsigned int MeshBuffer_ID) override;


		virtual unsigned int Upload_PointBuffer(const void* point_data, unsigned int data_size, unsigned int point_count) override;
		virtual unsigned int CreatePointBuffer_fromMeshBuffer(unsigned int MeshBuffer_ID, unsigned int AttributeIndex_toUseAsPointBuffer) override;
		virtual void Unload_PointBuffer(unsigned int PointBuffer_ID) override;


		virtual void Create_Texture(GFX_API::Texture_Resource* TEXTURE_ASSET, unsigned int Asset_ID, unsigned int TransferPacketID) override;
		virtual void Upload_Texture(unsigned int Asset_ID, void* DATA, unsigned int DATA_SIZE, unsigned int TransferPacketID) override;
		virtual void Unload_Texture(unsigned int ASSET_ID) override;


		virtual unsigned int Create_GlobalBuffer(const char* BUFFER_NAME, void* DATA, unsigned int DATA_SIZE, GFX_API::BUFFER_VISIBILITY USAGE) override;
		virtual void Upload_GlobalBuffer(unsigned int BUFFER_ID, void* DATA = nullptr, unsigned int DATA_SIZE = 0) override;
		virtual void Unload_GlobalBuffer(unsigned int BUFFER_ID) override;


		virtual void Compile_ShaderSource(GFX_API::ShaderSource_Resource* SHADER, unsigned int Asset_ID, string* compilation_status) override;
		virtual void Delete_ShaderSource(unsigned int ASSET_ID) override;
		virtual void Compile_ComputeShader(GFX_API::ComputeShader_Resource* SHADER, unsigned int Asset_ID, string* compilation_status) override;
		virtual void Delete_ComputeShader(unsigned int ASSET_ID) override;
		virtual void Link_MaterialType(GFX_API::Material_Type* MATTYPE_ASSET, unsigned int Asset_ID) override;
		virtual void Delete_MaterialType(unsigned int Asset_ID) override;
		virtual void Create_MaterialInst(GFX_API::Material_Instance* MATINST_ASSET, unsigned int Asset_ID) override;
		virtual void Delete_MaterialInst(unsigned int Asset_ID) override;

		virtual unsigned int Create_RTSlotSet(unsigned int* RTIDs, unsigned int* SlotIndexes, GFX_API::ATTACHMENT_READSTATE* ReadTypes,
			GFX_API::RT_ATTACHMENTs* AttachmentTypes, GFX_API::OPERATION_TYPE* OperationsTypes, unsigned int SlotCount) override;



		GFX_API::GFX_GlobalBuffer* Find_Buffer_byID(unsigned int BufferID, unsigned int* vector_index = nullptr);
		GFX_API::GFX_Texture* Find_GFXTexture_byID(unsigned int Texture_AssetID, unsigned int* vector_index = nullptr);
		GFX_API::GFX_ShaderSource* Find_GFXShaderSource_byID(unsigned int ShaderSource_AssetID, unsigned int* vector_index = nullptr);
		GFX_API::GFX_ComputeShader* Find_GFXComputeShader_byID(unsigned int ComputeShader_AssetID, unsigned int* vector_index = nullptr);
		GFX_API::GFX_ShaderProgram* Find_GFXShaderProgram_byID(unsigned int ShaderProgram_AssetID, unsigned int* vector_index = nullptr);
		GFX_API::VertexAttribute* Find_VertexAttribute_byID(unsigned int VertexAttributeID, unsigned int* vector_index = nullptr);
		GFX_API::VertexAttributeLayout* Find_VertexAttributeLayout_byID(unsigned int LayoutID, unsigned int* vector_index = nullptr);
		GFX_API::RT_SLOTSET* Find_RTSLOTSET_byID(unsigned int SLOTSETID, unsigned int* vector_index = nullptr);
	};
}