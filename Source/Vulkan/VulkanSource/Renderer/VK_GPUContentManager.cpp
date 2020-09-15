#include "VK_GPUContentManager.h"
#include "Vulkan_Renderer_Core.h"
#define VKRENDERER ((Vulkan::Renderer*)GFXRENDERER)
#define VKGPU ((Vulkan::GPU*)GFX->GPU_TO_RENDER)
#define STAGINGBUFFER_MALLOC 20480
#define BUFFER_MALLOC (1024 * 128)

#define MAXDESCSETCOUNT_PERPOOL 100; 
#define MAXUBUFFERCOUNT_PERPOOL 100; 
#define MAXSBUFFERCOUNT_PERPOOL 100; 
#define MAXSAMPLERCOUNT_PERPOOL 100;
#define MAXIMAGECOUNT_PERPOOL	100;

namespace Vulkan {
	GPU_ContentManager::GPU_ContentManager() {
		//Transfer Operations Command Pool Creation
		{
			VkCommandPoolCreateInfo CommandPool_ci = {};
			CommandPool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			CommandPool_ci.queueFamilyIndex = VKGPU->Transfer_QueueFamily;
			CommandPool_ci.pNext = nullptr;
			CommandPool_ci.flags = 0;

			if (vkCreateCommandPool(VKGPU->Logical_Device, &CommandPool_ci, nullptr, &TransferCommandBuffersPool) != VK_SUCCESS) {
				TuranAPI::LOG_CRASHING("Vulkan Transfer Command Buffer Pool allocation failed!");
			}
		}
		//Transfer Operations Command Buffer Creation
		{
			VkCommandBufferAllocateInfo CommandBuffer_ai = {};
			CommandBuffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			CommandBuffer_ai.commandPool = TransferCommandBuffersPool;
			CommandBuffer_ai.commandBufferCount = 1;
			CommandBuffer_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			CommandBuffer_ai.pNext = nullptr;
			if (vkAllocateCommandBuffers(VKGPU->Logical_Device, &CommandBuffer_ai, &TransferCommandBuffer) != VK_SUCCESS) {
				TuranAPI::LOG_CRASHING("Vulkan Command Buffer allocation failed!");
			}
		}
		//Staging Buffer Memory Allocation
		{
			VkMemoryRequirements memrequirements;
			VkBufferUsageFlags USAGEFLAGs = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			VkBuffer stagingbuffer = Create_VkBuffer(STAGINGBUFFER_MALLOC, USAGEFLAGs);
			vkGetBufferMemoryRequirements(VKGPU->Logical_Device, stagingbuffer, &memrequirements);
			unsigned int memtypeindex = VKGPU->Find_vkMemoryTypeIndex(memrequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VkMemoryAllocateInfo ci{};
			ci.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			ci.allocationSize = memrequirements.size;
			ci.memoryTypeIndex = memtypeindex;
			VkDeviceMemory allocated_memory;
			if (vkAllocateMemory(VKGPU->Logical_Device, &ci, nullptr, &allocated_memory) != VK_SUCCESS) {
				TuranAPI::LOG_CRASHING("GPU_ContentManager has failed because vkAllocateMemory has failed!");
			}
			vkBindBufferMemory(VKGPU->Logical_Device, stagingbuffer, allocated_memory, 0);
			STAGINGBUFFERALLOC.Base_Buffer = stagingbuffer;
			STAGINGBUFFERALLOC.Requirements = memrequirements;
			STAGINGBUFFERALLOC.FullSize = STAGINGBUFFER_MALLOC;
			STAGINGBUFFERALLOC.Usage = USAGEFLAGs;
			STAGINGBUFFERALLOC.Allocated_Memory = allocated_memory;
			STAGINGBUFFERALLOC.UnusedSize = STAGINGBUFFER_MALLOC;
		}
		//GPU Local Memory Allocation
		{
			VkMemoryRequirements memrequirements;
			VkBufferUsageFlags USAGEFLAGs = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			VkBuffer GPULOCAL_buf = Create_VkBuffer(BUFFER_MALLOC, USAGEFLAGs);
			vkGetBufferMemoryRequirements(VKGPU->Logical_Device, GPULOCAL_buf, &memrequirements);
			unsigned int memtypeindex = VKGPU->Find_vkMemoryTypeIndex(memrequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VkMemoryAllocateInfo ci{};
			ci.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			ci.allocationSize = memrequirements.size;
			ci.memoryTypeIndex = memtypeindex;
			VkDeviceMemory allocated_memory;
			if (vkAllocateMemory(VKGPU->Logical_Device, &ci, nullptr, &allocated_memory) != VK_SUCCESS) {
				TuranAPI::LOG_CRASHING("GPU_ContentManager has failed because vkAllocateMemory has failed!");
			}
			//I don't bind the memory to the VkBuffer because I don't intend to use VkBuffer for this allocation type
			vkDestroyBuffer(VKGPU->Logical_Device, GPULOCAL_buf, nullptr);

			GPULOCAL_BUFFERALLOC.Requirements = memrequirements;
			GPULOCAL_BUFFERALLOC.FullSize = BUFFER_MALLOC;
			GPULOCAL_BUFFERALLOC.Usage = USAGEFLAGs;
			GPULOCAL_BUFFERALLOC.Allocated_Memory = allocated_memory;
			GPULOCAL_BUFFERALLOC.UnusedSize = BUFFER_MALLOC;
		}

		//Descriptor Count is fixed per Descriptor Type per Descriptor Pool, so store the fixed way!
		{
			VkDescriptorPoolSize ubuffercount;
			ubuffercount.descriptorCount = MAXUBUFFERCOUNT_PERPOOL;
			DataCount_PerType_PerPool.push_back(ubuffercount);

			VkDescriptorPoolSize sbuffercount;
			ubuffercount.descriptorCount = MAXSBUFFERCOUNT_PERPOOL;
			DataCount_PerType_PerPool.push_back(sbuffercount);

			VkDescriptorPoolSize samplercount;
			samplercount.descriptorCount = MAXSAMPLERCOUNT_PERPOOL;
			DataCount_PerType_PerPool.push_back(samplercount);

			VkDescriptorPoolSize imagecount;
			imagecount.descriptorCount = MAXIMAGECOUNT_PERPOOL;
			DataCount_PerType_PerPool.push_back(imagecount);
		}
	}
	GPU_ContentManager::~GPU_ContentManager() {

	}
	void GPU_ContentManager::Unload_AllResources() {

	}
	void GPU_ContentManager::Start_TransferCB() {
		VkCommandBufferBeginInfo bi = {};
		bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		bi.pInheritanceInfo = nullptr;
		bi.pNext = nullptr;

		if (vkBeginCommandBuffer(TransferCommandBuffer, &bi) != VK_SUCCESS) {
			TuranAPI::LOG_ERROR("Start_TransferCB() has failed!");
		}
	}
	void GPU_ContentManager::End_TransferCB() {
		if (vkEndCommandBuffer(TransferCommandBuffer) != VK_SUCCESS) {
			TuranAPI::LOG_ERROR("End_TransferCB has failed in End Command Buffer!");
		}

		VkSubmitInfo si = {};
		si.commandBufferCount = 1;
		si.pCommandBuffers = &TransferCommandBuffer;
		si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		if (vkQueueSubmit(VKGPU->TransferQueue, 1, &si, VK_NULL_HANDLE) != VK_SUCCESS) {
			TuranAPI::LOG_ERROR("End_TransferCB has failed in Queue Submit!");
		}
		vkQueueWaitIdle(VKGPU->TransferQueue);
	}

	void GPU_ContentManager::Create_RenderGraphResources() {
		//Create Global Buffer Descriptors etc
		{
			//Descriptor Set Layout creation
			unsigned int UNIFORMBUFFER_COUNT = 0, STORAGEBUFFER_COUNT = 0;
			{
				vector<VkDescriptorSetLayoutBinding> bindings;
				for (unsigned int i = 0; i < GLOBALBUFFERs.size(); i++) {
					GFX_API::GFX_GlobalBuffer& globbuf = GLOBALBUFFERs[i];
					bindings.push_back(VkDescriptorSetLayoutBinding());
					bindings[i].binding = globbuf.BINDINGPOINT;
					bindings[i].descriptorCount = 1;
					bindings[i].descriptorType = Find_VkDescType_byVisibility(globbuf.VISIBILITY);
					bindings[i].pImmutableSamplers = VK_NULL_HANDLE;
					bindings[i].stageFlags = Find_VkStages(globbuf.ACCESSED_STAGEs);
					//Buffer Type Counting
					switch (bindings[i].descriptorType) {
					case VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
					case VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
						STORAGEBUFFER_COUNT++;
						break;
					case VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					case VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
						UNIFORMBUFFER_COUNT++;
						break;
					}
				}
				VkDescriptorSetLayoutCreateInfo DescSetLayout_ci = {};
				DescSetLayout_ci.flags = 0;
				DescSetLayout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				DescSetLayout_ci.pNext = nullptr;
				DescSetLayout_ci.bindingCount = bindings.size();
				DescSetLayout_ci.pBindings = bindings.data();
				if (vkCreateDescriptorSetLayout(VKGPU->Logical_Device, &DescSetLayout_ci, nullptr, &GlobalBuffers_DescSetLayout) != VK_SUCCESS) {
					TuranAPI::LOG_CRASHING("Create_RenderGraphResources() has failed at vkCreateDescriptorSetLayout()!");
					return;
				}
			}
			//Descriptor Pool creation
			{
				vector<VkDescriptorPoolSize> poolsizes(2);
				{
					poolsizes[0].descriptorCount = UNIFORMBUFFER_COUNT;
					poolsizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					poolsizes[1].descriptorCount = STORAGEBUFFER_COUNT;
					poolsizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				}
				VkDescriptorPoolCreateInfo descpool_ci = {};
				descpool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				descpool_ci.maxSets = 1;
				descpool_ci.pPoolSizes = poolsizes.data();
				descpool_ci.poolSizeCount = poolsizes.size();
				descpool_ci.flags = 0;
				descpool_ci.pNext = nullptr;
				if (vkCreateDescriptorPool(VKGPU->Logical_Device, &descpool_ci, nullptr, &GlobalBuffers_DescPool) != VK_SUCCESS) {
					TuranAPI::LOG_CRASHING("Create_RenderGraphResources() has failed at vkCreateDescriptorPool()!");
				}
			}
			//Descriptor Set allocation
			{
				VkDescriptorSetAllocateInfo descset_ai = {};
				descset_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				descset_ai.pNext = nullptr;
				descset_ai.descriptorPool = GlobalBuffers_DescPool;
				descset_ai.descriptorSetCount = 1;
				descset_ai.pSetLayouts = &GlobalBuffers_DescSetLayout;
				if (vkAllocateDescriptorSets(VKGPU->Logical_Device, &descset_ai, &GlobalBuffers_DescSet) != VK_SUCCESS) {
					TuranAPI::LOG_CRASHING("Create_RenderGraphResources() has failed at vkAllocateDescriptorSets()!");
				}
			}
		}
	}


	unsigned int GPU_ContentManager::Calculate_sizeofVertexLayout(vector<GFX_API::VertexAttribute*>& ATTRIBUTEs) {
		unsigned int size = 0;
		for (unsigned int i = 0; i < ATTRIBUTEs.size(); i++) {
			size += GFX_API::Get_UNIFORMTYPEs_SIZEinbytes(ATTRIBUTEs[i]->DATATYPE);
		}
		return size;
	}
	unsigned int GPU_ContentManager::Create_VertexAttributeLayout(vector<unsigned int> Attributes) {
		vector<GFX_API::VertexAttribute*> ATTRIBs;
		for (unsigned int i = 0; i < Attributes.size(); i++) {
			GFX_API::VertexAttribute* Attribute = Find_VertexAttribute_byID(Attributes[i]);
			if (!Attribute) {
				TuranAPI::LOG_ERROR("You referenced an uncreated attribute to Create_VertexAttributeLayout!");
				return 0;
			}
			ATTRIBs.push_back(Attribute);
		}
		unsigned int size_pervertex = Calculate_sizeofVertexLayout(ATTRIBs);
		VK_VertexAttribLayout* VK_LAYOUT = new VK_VertexAttribLayout;

		VK_LAYOUT->BindingDesc.binding = 0;
		VK_LAYOUT->BindingDesc.stride = size_pervertex;
		VK_LAYOUT->BindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		
		VK_LAYOUT->AttribDescs = new VkVertexInputAttributeDescription[Attributes.size()];
		VK_LAYOUT->AttribDesc_Count = Attributes.size();
		unsigned int stride_ofcurrentattribute = 0;
		for (unsigned int i = 0; i < ATTRIBs.size(); i++) {
			VK_LAYOUT->AttribDescs[i].binding = 0;
			VK_LAYOUT->AttribDescs[i].location = i;
			VK_LAYOUT->AttribDescs[i].offset = stride_ofcurrentattribute;
			VK_LAYOUT->AttribDescs[i].format = Find_VkFormat_byDataType(ATTRIBs[i]->DATATYPE);
			stride_ofcurrentattribute += GFX_API::Get_UNIFORMTYPEs_SIZEinbytes(ATTRIBs[i]->DATATYPE);
		}

		GFX_API::VertexAttributeLayout Layout;
		Layout.Attributes = new GFX_API::VertexAttribute* [ATTRIBs.size()];
		for (unsigned int i = 0; i < ATTRIBs.size(); i++) {
			Layout.Attributes[i] = ATTRIBs[i];
		}
		Layout.Attribute_Number = ATTRIBs.size();
		Layout.GL_ID = VK_LAYOUT;
		Layout.size_perVertex = size_pervertex;
		Layout.ID = Create_VertexAttribLayoutID();
		VERTEXATTRIBLAYOUTs.push_back(Layout);
	}
	void GPU_ContentManager::Delete_VertexAttributeLayout(unsigned int Layout_ID) {
		TuranAPI::LOG_NOTCODED("Delete_VertexAttributeLayout() isn't coded yet!", true);
	}


	VkDeviceSize GPU_ContentManager::Create_StagingMemoryBlock(unsigned int size) {
		if (STAGINGBLOCKs.size() == 0) {
			VK_MemoryBlock FirstBlock;
			FirstBlock.isEmpty = false;
			FirstBlock.Size = size;
			FirstBlock.Offset = 0;
			STAGINGBUFFERALLOC.UnusedSize -= size;
			STAGINGBLOCKs.push_back(FirstBlock);
			return static_cast<VkDeviceSize>(FirstBlock.Offset);
		}
		for (unsigned int i = 0; i < STAGINGBLOCKs.size(); i++) {
			VK_MemoryBlock& Block = STAGINGBLOCKs[i];
			if (Block.isEmpty && size < Block.Size && size > Block.Size / 5 * 3) {
				Block.isEmpty = false;
				return static_cast<VkDeviceSize>(Block.Offset);
			}
		}
		//There is no empty block, so create new one!
		if (STAGINGBUFFERALLOC.UnusedSize > size) {
			VK_MemoryBlock& LastBlock = STAGINGBLOCKs[STAGINGBLOCKs.size() - 1];
			VK_MemoryBlock NewBlock;
			NewBlock.isEmpty = false;
			NewBlock.Size = size;
			NewBlock.Offset = LastBlock.Offset + LastBlock.Size;
			STAGINGBUFFERALLOC.UnusedSize -= size;
			STAGINGBLOCKs.push_back(NewBlock);
			return static_cast<VkDeviceSize>(NewBlock.Offset);
		}
		TuranAPI::LOG_CRASHING("Create_StagingMemoryBlock() has failed because Vertex Buffer hasn't enough space!");
		return UINT64_MAX;
	}
	void GPU_ContentManager::Clear_StagingMemory() {
		STAGINGBLOCKs.clear();
		STAGINGBUFFERALLOC.UnusedSize = STAGINGBUFFERALLOC.FullSize;
	}
	VkDeviceSize GPU_ContentManager::Create_GPULocalMemoryBlock(unsigned int size) {
		if (GPULOCALBLOCKs.size() == 0) {
			VK_MemoryBlock FirstBlock;
			FirstBlock.isEmpty = false;
			FirstBlock.Size = size;
			FirstBlock.Offset = 0;
			GPULOCAL_BUFFERALLOC.UnusedSize -= size;
			GPULOCALBLOCKs.push_back(FirstBlock);
			return static_cast<VkDeviceSize>(FirstBlock.Offset);
		}
		for (unsigned int i = 0; i < GPULOCALBLOCKs.size(); i++) {
			VK_MemoryBlock& Block = GPULOCALBLOCKs[i];
			if (Block.isEmpty && size < Block.Size && size > Block.Size / 5 * 3) {
				Block.isEmpty = false;
				return static_cast<VkDeviceSize>(Block.Offset);
			}
		}
		//There is no empty block, so create new one!
		if (GPULOCAL_BUFFERALLOC.UnusedSize > size) {
			VK_MemoryBlock& LastBlock = GPULOCALBLOCKs[GPULOCALBLOCKs.size() - 1];
			VK_MemoryBlock NewBlock;
			NewBlock.isEmpty = false;
			NewBlock.Size = size;
			NewBlock.Offset = LastBlock.Offset + LastBlock.Size;
			GPULOCAL_BUFFERALLOC.UnusedSize -= size;
			GPULOCALBLOCKs.push_back(NewBlock);
			return static_cast<VkDeviceSize>(NewBlock.Offset);
		}
		TuranAPI::LOG_CRASHING("Create_GPULocalMemoryBlock() has failed because Vertex Buffer hasn't enough space!");
		return UINT64_MAX;
	}

	void NaiveCreate_DescPool(VkDescriptorPool* Pool, unsigned int DataTypeCount, VkDescriptorPoolSize* DataCounts_PerType) {
		VkDescriptorPoolCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		ci.flags = 0;
		ci.pNext = nullptr;
		ci.maxSets = MAXDESCSETCOUNT_PERPOOL;
		ci.poolSizeCount = DataTypeCount;
		ci.pPoolSizes = DataCounts_PerType;
		if (vkCreateDescriptorPool(VKGPU->Logical_Device, &ci, nullptr, Pool) != VK_SUCCESS) {
			TuranAPI::LOG_ERROR("Create_Descs() has failed at vkCreateDescriptorPool()!");
			return;
		}
	}
	void NaiveCreate_DescSets(VkDescriptorPool Pool, const VkDescriptorSetLayout* layout, unsigned int size, VkDescriptorSet* Set) {
		VkDescriptorSetAllocateInfo ai = {};
		ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		ai.descriptorSetCount = size;
		ai.pSetLayouts = layout;
		ai.descriptorPool = Pool;
		ai.pNext = nullptr;
		if (vkAllocateDescriptorSets(VKGPU->Logical_Device, &ai, Set) != VK_SUCCESS) {
			TuranAPI::LOG_ERROR("Create_DescSet() has failed at vkAllocateDescriptorSets()");
			return;
		}
	}
	bool GPU_ContentManager::Create_DescSets(vector<GFX_API::MaterialDataDescriptor>& GFXMatDataDescs, const VkDescriptorSetLayout* DescSetLay, VkDescriptorSet* DescSet, unsigned int SetCount) {
		unsigned int UBUFFERCOUNT = 0, SBUFFERCOUNT = 0, SAMPLERCOUNT = 0, IMAGECOUNT = 0;
		//Get per data type counts
		for (unsigned int i = 0; i < GFXMatDataDescs.size(); i++) {
			GFX_API::MaterialDataDescriptor& desc = GFXMatDataDescs[i];
			if (desc.BINDINGPOINT) {
				switch (desc.TYPE) {
				case GFX_API::MATERIALDATA_TYPE::CONSTIMAGE_G:
					IMAGECOUNT++;
					break;
				case GFX_API::MATERIALDATA_TYPE::CONSTSAMPLER_G:
					SAMPLERCOUNT++;
					break;
				case GFX_API::MATERIALDATA_TYPE::CONSTSBUFFER_G:
					SBUFFERCOUNT++;
					break;
				case GFX_API::MATERIALDATA_TYPE::CONSTUBUFFER_G:
					UBUFFERCOUNT++;
					break;
				case GFX_API::MATERIALDATA_TYPE::UNDEFINED:
					TuranAPI::LOG_CRASHING("Get_AvailableDescPool() has failed because type is UNDEFINED!");
				default:
					TuranAPI::LOG_CRASHING("Get_AvailableDescPool() has failed because type isn't supported!");
				}
			}
		}
		
		//Allocate sets in available pools
		unsigned int REMAININGALLOCATESETCOUNT = SetCount;
		for (unsigned int i = 0; i < MaterialData_DescPools.size() && SetCount; i++) {
			VK_DescPool VKP = MaterialData_DescPools[i];
			if (VKP.REMAINING_IMAGE > IMAGECOUNT && VKP.REMAINING_SAMPLER > SAMPLERCOUNT && VKP.REMAINING_SBUFFER > SBUFFERCOUNT && VKP.REMAINING_UBUFFER > UBUFFERCOUNT && VKP.REMAINING_SET > 0) {
				unsigned int allocatecount = (VKP.REMAINING_SET > REMAININGALLOCATESETCOUNT) ? (REMAININGALLOCATESETCOUNT) : (VKP.REMAINING_SET);

				NaiveCreate_DescSets(VKP.pool, DescSetLay, allocatecount, DescSet + (SetCount - REMAININGALLOCATESETCOUNT));

				REMAININGALLOCATESETCOUNT -= allocatecount;
				if (!SetCount) {	//Allocated all sets!
					return true;
				}
			}
		}
		
		//Previous pools wasn't big enough, allocate overflowing sets in new pools!
		while (REMAININGALLOCATESETCOUNT) {
			//Create pool first!
			{
				VkDescriptorPool vkpool;
				NaiveCreate_DescPool(&vkpool, DataCount_PerType_PerPool.size(), DataCount_PerType_PerPool.data());
				VK_DescPool NEWPOOL;
				NEWPOOL.pool = vkpool;
				NEWPOOL.REMAINING_IMAGE = MAXIMAGECOUNT_PERPOOL;
				NEWPOOL.REMAINING_SAMPLER = MAXSAMPLERCOUNT_PERPOOL;
				NEWPOOL.REMAINING_SBUFFER = MAXSBUFFERCOUNT_PERPOOL;
				NEWPOOL.REMAINING_UBUFFER = MAXUBUFFERCOUNT_PERPOOL;
				NEWPOOL.REMAINING_SET = MAXDESCSETCOUNT_PERPOOL;
				MaterialData_DescPools.push_back(NEWPOOL);
			}
			VK_DescPool& VKP = MaterialData_DescPools[MaterialData_DescPools.size() - 1];
			{
				unsigned int allocatecount = (VKP.REMAINING_SET > REMAININGALLOCATESETCOUNT) ? (REMAININGALLOCATESETCOUNT) : (VKP.REMAINING_SET);

				NaiveCreate_DescSets(VKP.pool, DescSetLay, allocatecount, DescSet + (SetCount - REMAININGALLOCATESETCOUNT));

				REMAININGALLOCATESETCOUNT -= allocatecount;
				if (!SetCount) {	//Allocated all sets!
					return true;
				}
			}
		}

		TuranAPI::LOG_CRASHING("Create_DescSets() has failed at the end which isn't supposed to happen, there is some crazy shit going on here!");
		return false;
	}

	VkBuffer GPU_ContentManager::Create_VkBuffer(unsigned int size, VkBufferUsageFlags usage) {
		VkBuffer buffer;

		VkBufferCreateInfo ci{};
		ci.usage = usage;
		ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
		ci.queueFamilyIndexCount = 2;
		ci.pQueueFamilyIndices = VKGPU->ResourceSharedQueueFamilies;
		ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		ci.size = size;

		if (vkCreateBuffer(VKGPU->Logical_Device, &ci, nullptr, &buffer) != VK_SUCCESS) {
			TuranAPI::LOG_ERROR("Create_VkBuffer has failed!");
		}
		return buffer;
	}
	//Return MeshBufferID to use in Draw Calls
	unsigned int GPU_ContentManager::Upload_MeshBuffer(unsigned int attributelayout, const void* vertex_data, unsigned int data_size, unsigned int vertex_count,
		const void* index_data, unsigned int index_count) {
		GFX_API::VertexAttributeLayout* Layout = nullptr;
		Layout = Find_VertexAttributeLayout_byID(attributelayout);
		if (!Layout) {
			TuranAPI::LOG_ERROR("Upload_MeshBuffer has failed because Attribute Layout ID is invalid!");
			return 0;
		}
		if (vertex_count * Layout->size_perVertex != data_size) {
			TuranAPI::LOG_ERROR("Attribute Layout * Vertex Count isn't equal to Data Size, this shouldn't happen!");
			return 0;
		}

		VK_Mesh* Mesh = new VK_Mesh;
		Mesh->Vertex_Count = vertex_count;
		Mesh->VertexBufOffset = Create_StagingMemoryBlock(data_size);
		Mesh->Index_Count = 0;
		Mesh->IndexBufOffset = 0;
		Mesh->Layout = Layout;

		void* data;
		vkMapMemory(VKGPU->Logical_Device, STAGINGBUFFERALLOC.Allocated_Memory, Mesh->VertexBufOffset, data_size, 0, &data);
		memcpy(data, vertex_data, data_size);
		vkUnmapMemory(VKGPU->Logical_Device, STAGINGBUFFERALLOC.Allocated_Memory);

		GFX_API::GFX_GlobalBuffer* Buffer = new GFX_API::GFX_GlobalBuffer;
		Buffer->DATA = vertex_data;
		Buffer->DATA_SIZE = data_size;
		Buffer->VISIBILITY = GFX_API::BUFFER_VISIBILITY::CPUEXISTENCE_GPUREADONLY;
		Buffer->TYPE = GFX_API::BUFFER_TYPE::MESH_BUFFER;
		Buffer->ID = Create_GlobalBufferID();
		Buffer->GL_ID = Mesh;
		return Buffer->ID;
	}
	//When you call this function, Draw Calls that uses this ID may draw another Mesh or crash
	//Also if you have any Point Buffer that uses first vertex attribute of that Mesh Buffer, it may crash or draw any other buffer
	void GPU_ContentManager::Unload_MeshBuffer(unsigned int MeshBuffer_ID) {
		TuranAPI::LOG_NOTCODED("VK::Unload_MeshBuffer isn't coded!", true);
	}


	unsigned int GPU_ContentManager::Upload_PointBuffer(const void* point_data, unsigned int data_size, unsigned int point_count) {
		TuranAPI::LOG_NOTCODED("VK::Upload_PointBuffer isn't coded!", true);
		return 0;
	}
	unsigned int GPU_ContentManager::CreatePointBuffer_fromMeshBuffer(unsigned int MeshBuffer_ID, unsigned int AttributeIndex_toUseAsPointBuffer) {
		TuranAPI::LOG_NOTCODED("VK::CreatePointBuffer_fromMeshBuffer isn't coded!", true);
		return 0;
	}
	void GPU_ContentManager::Unload_PointBuffer(unsigned int PointBuffer_ID) {
		TuranAPI::LOG_NOTCODED("VK::Unload_PointBuffer isn't coded!", true);
	}


	void GPU_ContentManager::Create_Texture(GFX_API::Texture_Resource* TEXTURE_ASSET, unsigned int Asset_ID, unsigned int TransferPacketID) {
		VkDeviceSize Offset = Create_StagingMemoryBlock(TEXTURE_ASSET->DATA_SIZE);
		void* stagingbuffer_ptr;
		vkMapMemory(VKGPU->Logical_Device, STAGINGBUFFERALLOC.Allocated_Memory, Offset, 0, TEXTURE_ASSET->DATA_SIZE, &stagingbuffer_ptr);
		memcpy(stagingbuffer_ptr, TEXTURE_ASSET->DATA, TEXTURE_ASSET->DATA_SIZE);
		vkUnmapMemory(VKGPU->Logical_Device, STAGINGBUFFERALLOC.Allocated_Memory);

		//GPU LOCAL IMAGE CREATION!
		VkImage VKIMAGE;
		{
			VkImageCreateInfo im_ci = {};
			im_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			im_ci.arrayLayers = 1;
			im_ci.extent.width = TEXTURE_ASSET->WIDTH;
			im_ci.extent.height = TEXTURE_ASSET->HEIGHT;
			im_ci.extent.depth = 1;
			im_ci.flags = 0;
			im_ci.format = Find_VkFormat_byTEXTURECHANNELs(TEXTURE_ASSET->Properties.CHANNEL_TYPE);
			im_ci.imageType = VK_IMAGE_TYPE_2D;
			im_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			if (vkCreateImage(VKGPU->Logical_Device, &im_ci, nullptr, &VKIMAGE) != VK_SUCCESS) {
				TuranAPI::LOG_ERROR("GFXContentManager->Create_Texture() has failed in vkCreateImage()!");
				return;
			}

			VkDeviceSize Offset = Create_GPULocalMemoryBlock(TEXTURE_ASSET->DATA_SIZE);
			vkBindImageMemory(VKGPU->Logical_Device, VKIMAGE, GPULOCAL_BUFFERALLOC.Allocated_Memory, Offset);
		}

		Start_TransferCB();

		//Transition to TRANSFER_DISTANCEBIT
		{
			VkImageMemoryBarrier im_br = {};
			im_br.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			im_br.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			im_br.srcAccessMask = 0;
			im_br.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			im_br.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			im_br.image = VKIMAGE;
			im_br.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			im_br.subresourceRange.baseArrayLayer = 0;
			im_br.subresourceRange.layerCount = 1;
			im_br.subresourceRange.baseMipLevel = 0;
			im_br.subresourceRange.levelCount = 1;
			vkCmdPipelineBarrier(TransferCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &im_br);
		}

		//Copy Buffer to Image
		{
			VkBufferImageCopy copy = {};
			VkImageSubresourceLayers layers = {};
			layers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			layers.mipLevel = 0;
			layers.baseArrayLayer = 0;
			layers.layerCount = 1;
			copy.bufferImageHeight = 0;
			copy.bufferRowLength = 0;
			copy.bufferOffset = Offset;
			copy.imageExtent.height = TEXTURE_ASSET->HEIGHT;
			copy.imageExtent.width = TEXTURE_ASSET->WIDTH;
			copy.imageExtent.depth = 1;
			copy.imageOffset = { 0,0,0 };
			copy.imageSubresource = layers;
			vkCmdCopyBufferToImage(TransferCommandBuffer, STAGINGBUFFERALLOC.Base_Buffer, VKIMAGE, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
		}

		End_TransferCB();

		Clear_StagingMemory();

		//Create Image View
		VkImageView VKIMAGEVIEW;
		{
			VkImageViewCreateInfo ci = {};
			ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ci.flags = 0;
			ci.pNext = nullptr;

			ci.image = VKIMAGE;
			ci.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
			ci.format = Find_VkFormat_byTEXTURECHANNELs(TEXTURE_ASSET->Properties.CHANNEL_TYPE);
			ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ci.subresourceRange.baseArrayLayer = 0;
			ci.subresourceRange.layerCount = 1;
			ci.subresourceRange.baseMipLevel = 0;
			ci.subresourceRange.levelCount = 1;

			if (vkCreateImageView(VKGPU->Logical_Device, &ci, nullptr, &VKIMAGEVIEW) != VK_SUCCESS) {
				TuranAPI::LOG_ERROR("GFXContentManager->Create_Texture() has failed in vkCreateImageView()!");
				return;
			}
		}

		//Finalizations for registering the texture to GFX pipeline

		VK_Texture* vktexture = new VK_Texture;
		{
			vktexture->Image = VKIMAGE;
			vktexture->ImageView = VKIMAGEVIEW;
		}
		GFX_API::GFX_Texture gfx_texture;
		gfx_texture.ASSET_ID = Asset_ID;
		gfx_texture.CHANNELs = TEXTURE_ASSET->Properties.CHANNEL_TYPE;
		gfx_texture.GL_ID = vktexture;
		gfx_texture.HEIGHT = TEXTURE_ASSET->HEIGHT;
		gfx_texture.WIDTH = TEXTURE_ASSET->WIDTH;
		gfx_texture.TYPE = TEXTURE_ASSET->TYPE;
		gfx_texture.DATA_SIZE = TEXTURE_ASSET->DATA_SIZE;
		gfx_texture.BUF_VIS = TEXTURE_ASSET->BUF_VIS;
		TEXTUREs.push_back(gfx_texture);

		//Barrier Operations
		VkImageMemoryBarrier* VKIMMEMBAR = new VkImageMemoryBarrier{};
		{
			VKIMMEMBAR->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			VKIMMEMBAR->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			VKIMMEMBAR->subresourceRange.baseArrayLayer = 0;
			VKIMMEMBAR->subresourceRange.layerCount = 1;
			VKIMMEMBAR->subresourceRange.baseMipLevel = 0;
			VKIMMEMBAR->subresourceRange.levelCount = 1;
			VKIMMEMBAR->image = VKIMAGE;

			VKIMMEMBAR->oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			VKIMMEMBAR->srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			VKIMMEMBAR->newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			VKIMMEMBAR->dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}
		Add_to_TransferPacket(TransferPacketID, VKIMMEMBAR);
	}
	void GPU_ContentManager::Upload_Texture(unsigned int Asset_ID, void* DATA, unsigned int DATA_SIZE, unsigned int TransferPacketID){
		TuranAPI::LOG_NOTCODED("VK::Upload_Texture isn't coded!", true);
	}
	void GPU_ContentManager::Unload_Texture(unsigned int ASSET_ID) {
		TuranAPI::LOG_NOTCODED("VK::Unload_Texture isn't coded!", true);
	}


	unsigned int GPU_ContentManager::Create_GlobalBuffer(const char* BUFFER_NAME, void* DATA, unsigned int DATA_SIZE, GFX_API::BUFFER_VISIBILITY USAGE) {
		TuranAPI::LOG_NOTCODED("VK::Create_GlobalBuffer isn't coded!", true);
		if (!VKRENDERER->Record_RenderGraphCreation) {
			TuranAPI::LOG_ERROR("I don't support run-time Global Buffer addition for now because I need to recreate PipelineLayouts (so all PSOs)!");
			return 0;
		}
		return 0;
	}
	void GPU_ContentManager::Upload_GlobalBuffer(unsigned int BUFFER_ID, void* DATA, unsigned int DATA_SIZE) {
		TuranAPI::LOG_NOTCODED("VK::Upload_GlobalBuffer isn't coded!", true);
	}
	void GPU_ContentManager::Unload_GlobalBuffer(unsigned int BUFFER_ID) {
		TuranAPI::LOG_NOTCODED("VK::Unload_GlobalBuffer isn't coded!", true);
	}


	void GPU_ContentManager::Compile_ShaderSource(GFX_API::ShaderSource_Resource* SHADER, unsigned int Asset_ID, string* compilation_status) {
		//Create Vertex Shader Module
		VkShaderModuleCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ci.flags = 0;
		ci.pNext = nullptr;
		ci.pCode = reinterpret_cast<const uint32_t*>(SHADER->SOURCE_CODE.c_str());
		ci.codeSize = SHADER->SOURCE_CODE.size();

		VkShaderModule* Module = new VkShaderModule{};
		if (vkCreateShaderModule(((GPU*)VKGPU)->Logical_Device, &ci, 0, Module) != VK_SUCCESS) {
			TuranAPI::LOG_CRASHING("Vertex Shader Module is failed at creation!");
		}
		
		GFX_API::GFX_ShaderSource SHADERSOURCE;
		SHADERSOURCE.ASSET_ID = Asset_ID;
		SHADERSOURCE.GL_ID = Module;
		SHADERSOURCEs.push_back(SHADERSOURCE);
		TuranAPI::LOG_STATUS("Vertex Shader Module is successfully created!");
	}
	void GPU_ContentManager::Delete_ShaderSource(unsigned int ASSET_ID) {
		TuranAPI::LOG_NOTCODED("VK::Unload_GlobalBuffer isn't coded!", true);
	}
	void GPU_ContentManager::Compile_ComputeShader(GFX_API::ComputeShader_Resource* SHADER, unsigned int Asset_ID, string* compilation_status){
		TuranAPI::LOG_NOTCODED("VK::Unload_GlobalBuffer isn't coded!", true);
	}
	void GPU_ContentManager::Delete_ComputeShader(unsigned int ASSET_ID){
		TuranAPI::LOG_NOTCODED("VK::Unload_GlobalBuffer isn't coded!", true);
	}
	void GPU_ContentManager::Link_MaterialType(GFX_API::Material_Type* MATTYPE_ASSET, unsigned int Asset_ID){
		if (VKRENDERER->Record_RenderGraphCreation) {
			TuranAPI::LOG_ERROR("You can't link a Material Type while recording RenderGraph!");
			return;
		}
		GFX_API::VertexAttributeLayout* LAYOUT = nullptr;
		LAYOUT = Find_VertexAttributeLayout_byID(MATTYPE_ASSET->ATTRIBUTELAYOUT_ID);
		if (!LAYOUT) {
			TuranAPI::LOG_ERROR("Link_MaterialType() has failed because Material Type has invalid Vertex Attribute Layout!");
			return;
		}
		GFX_API::SubDrawPass* Subpass = nullptr;
		GFX_API::DrawPass* MainPass = nullptr;
		for (unsigned int i = 0; i < MATTYPE_ASSET->SUBPASSes.size(); i++) {
			unsigned int MainPass_ID = 0;
			unsigned int SUBPASS_ID = MATTYPE_ASSET->SUBPASSes[i];
			Subpass = &VKRENDERER->Find_SubDrawPass_byID(SUBPASS_ID, &MainPass_ID);
			if (Subpass->SLOTSET->ID != MATTYPE_ASSET->RT_SLOTSETID) {
				TuranAPI::LOG_ERROR("Link_MaterialType() has failed because Material Type doesn't match with the SubDrawPass!");
				return;
			}
			MainPass = &VKRENDERER->Find_DrawPass_byID(MainPass_ID);
		}
		
		//Subpass attachment should happen here!
		VK_ShaderProgram* VKShaderProgram = new VK_ShaderProgram;

		VkPipelineShaderStageCreateInfo Vertex_ShaderStage = {};
		VkPipelineShaderStageCreateInfo Fragment_ShaderStage = {};
		VkPipelineShaderStageCreateInfo STAGEs[2] = { Vertex_ShaderStage, Fragment_ShaderStage };
		{
			Vertex_ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			Vertex_ShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
			VkShaderModule* VS_Module = (VkShaderModule*)Find_GFXShaderSource_byID(MATTYPE_ASSET->VERTEXSOURCE_ID)->GL_ID;
			Vertex_ShaderStage.module = *VS_Module;
			Vertex_ShaderStage.pName = "main";

			Fragment_ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			Fragment_ShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			VkShaderModule* FS_Module = (VkShaderModule*)Find_GFXShaderSource_byID(MATTYPE_ASSET->FRAGMENTSOURCE_ID)->GL_ID;
			Fragment_ShaderStage.module = *FS_Module;
			Fragment_ShaderStage.pName = "main";
		}


		GPU* Vulkan_GPU = (GPU*)VKGPU;

		VK_VertexAttribLayout* VKAttribLayout = (VK_VertexAttribLayout*)LAYOUT->GL_ID;
		VkPipelineVertexInputStateCreateInfo VertexInputState_ci = {};
		{
			VertexInputState_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			VertexInputState_ci.pVertexBindingDescriptions = &VKAttribLayout->BindingDesc;
			VertexInputState_ci.vertexBindingDescriptionCount = 1;
			VertexInputState_ci.pVertexAttributeDescriptions = VKAttribLayout->AttribDescs;
			VertexInputState_ci.vertexAttributeDescriptionCount = VKAttribLayout->AttribDesc_Count;
			VertexInputState_ci.flags = 0;
			VertexInputState_ci.pNext = nullptr;
		}
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = {};
		{
			InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			InputAssemblyState.primitiveRestartEnable = false;
			InputAssemblyState.flags = 0;
			InputAssemblyState.pNext = nullptr;
		}
		VkPipelineViewportStateCreateInfo RenderViewportState = {};
		{
			RenderViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			RenderViewportState.scissorCount = 1;
			RenderViewportState.pScissors = nullptr;
			RenderViewportState.viewportCount = 1;
			RenderViewportState.pViewports = nullptr;
			RenderViewportState.pNext = nullptr;
			RenderViewportState.flags = 0;
		}
		VkPipelineRasterizationStateCreateInfo RasterizationState = {};
		{
			RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
			RasterizationState.cullMode = VK_CULL_MODE_NONE;
			RasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
			RasterizationState.lineWidth = 1.0f;
			RasterizationState.depthClampEnable = VK_FALSE;
			RasterizationState.rasterizerDiscardEnable = VK_FALSE;

			RasterizationState.depthBiasEnable = VK_FALSE;
			RasterizationState.depthBiasClamp = 0.0f;
			RasterizationState.depthBiasConstantFactor = 0.0f;
			RasterizationState.depthBiasSlopeFactor = 0.0f;
		}
		VkPipelineMultisampleStateCreateInfo MSAAState = {};
		{
			MSAAState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			MSAAState.sampleShadingEnable = VK_FALSE;
			MSAAState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			MSAAState.minSampleShading = 1.0f;
			MSAAState.pSampleMask = nullptr;
			MSAAState.alphaToCoverageEnable = VK_FALSE;
			MSAAState.alphaToOneEnable = VK_FALSE;
		}

		VkPipelineColorBlendAttachmentState Attachment_ColorBlendState = {};
		VkPipelineColorBlendStateCreateInfo Pipeline_ColorBlendState = {};
		{
			Attachment_ColorBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			Attachment_ColorBlendState.blendEnable = VK_FALSE;
			Attachment_ColorBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			Attachment_ColorBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
			Attachment_ColorBlendState.colorBlendOp = VK_BLEND_OP_ADD; // Optional
			Attachment_ColorBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			Attachment_ColorBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
			Attachment_ColorBlendState.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

			Pipeline_ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			Pipeline_ColorBlendState.logicOpEnable = VK_FALSE;
			Pipeline_ColorBlendState.attachmentCount = 1;
			Pipeline_ColorBlendState.pAttachments = &Attachment_ColorBlendState;
			Pipeline_ColorBlendState.logicOp = VK_LOGIC_OP_COPY; // Optional
			Pipeline_ColorBlendState.blendConstants[0] = 0.0f; // Optional
			Pipeline_ColorBlendState.blendConstants[1] = 0.0f; // Optional
			Pipeline_ColorBlendState.blendConstants[2] = 0.0f; // Optional
			Pipeline_ColorBlendState.blendConstants[3] = 0.0f; // Optional
		}

		VkPipelineDynamicStateCreateInfo Dynamic_States = {};
		vector<VkDynamicState> DynamicStatesList;
		{
			DynamicStatesList.push_back(VK_DYNAMIC_STATE_VIEWPORT);
			DynamicStatesList.push_back(VK_DYNAMIC_STATE_SCISSOR);

			Dynamic_States.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			Dynamic_States.dynamicStateCount = DynamicStatesList.size();
			Dynamic_States.pDynamicStates = DynamicStatesList.data();
		}

		//Material Data related creations
		{
			//DescriptorSet Layout Creation
			{
				vector<VkDescriptorSetLayoutBinding> bindings;
				for (unsigned int i = 0; i < MATTYPE_ASSET->MATERIALTYPEDATA.size(); i++) {
					GFX_API::MaterialDataDescriptor& MATDATAdesc = MATTYPE_ASSET->MATERIALTYPEDATA[i];
					if (MATDATAdesc.BINDINGPOINT) {
						unsigned int BP = *(unsigned int*)MATDATAdesc.BINDINGPOINT;
						for (unsigned int bpsearchindex = 0; bpsearchindex < bindings.size(); bpsearchindex++) {
							if (BP == bindings[bpsearchindex].binding) {
								TuranAPI::LOG_ERROR("Link_MaterialType() has failed because there are colliding binding points!");
								return;
							}
						}
						VkDescriptorSetLayoutBinding bn = {};
						bn.stageFlags = Find_VkStages(MATDATAdesc.SHADERSTAGEs);
						bn.pImmutableSamplers = VK_NULL_HANDLE;
						bn.descriptorType = Find_VkDescType_byMATDATATYPE(MATDATAdesc.TYPE);
						bn.descriptorCount = 1;		//I don't support array descriptors for now!
						bn.binding = BP;
						bindings.push_back(bn);
					}
				}
				VkDescriptorSetLayoutCreateInfo ci = {};
				ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				ci.pNext = nullptr;
				ci.flags = 0;
				ci.bindingCount = bindings.size();
				ci.pBindings = bindings.data();

				if (vkCreateDescriptorSetLayout(VKGPU->Logical_Device, &ci, nullptr, &VKShaderProgram->SpecialBuffersSetLayout) != VK_SUCCESS) {
					TuranAPI::LOG_ERROR("Link_MaterialType() has failed at vkCreateDescriptorSetLayout()");
					return;
				}
			}
			//Pipeline Layout Creation
			{
				VkPipelineLayoutCreateInfo pl_ci = {};
				pl_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pl_ci.pNext = nullptr;
				pl_ci.flags = 0;
				pl_ci.setLayoutCount = 2;
				VkDescriptorSetLayout descsetlays[2] = { GlobalBuffers_DescSetLayout, VKShaderProgram->SpecialBuffersSetLayout };
				pl_ci.pSetLayouts = descsetlays;
				//Don't support for now!
				pl_ci.pushConstantRangeCount = 0;
				pl_ci.pPushConstantRanges = nullptr;

				if (vkCreatePipelineLayout(Vulkan_GPU->Logical_Device, &pl_ci, nullptr, &VKShaderProgram->PipelineLayout) != VK_SUCCESS) {
					TuranAPI::LOG_ERROR("Link_MaterialType() failed at vkCreatePipelineLayout()!");
					return;
				}
			}
		}


		VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = {};
		GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.pColorBlendState = &Pipeline_ColorBlendState;
		//There is no Depth-Stencil, so it will be nullptr
		GraphicsPipelineCreateInfo.pDepthStencilState = nullptr;
		GraphicsPipelineCreateInfo.pDynamicState = &Dynamic_States;
		GraphicsPipelineCreateInfo.pInputAssemblyState = &InputAssemblyState;
		GraphicsPipelineCreateInfo.pMultisampleState = &MSAAState;
		GraphicsPipelineCreateInfo.pRasterizationState = &RasterizationState;
		GraphicsPipelineCreateInfo.pVertexInputState = &VertexInputState_ci;
		GraphicsPipelineCreateInfo.pViewportState = &RenderViewportState;
		GraphicsPipelineCreateInfo.layout = VKShaderProgram->PipelineLayout;
		VK_DrawPass* VKDrawPass = (VK_DrawPass*)MainPass->GL_ID;
		GraphicsPipelineCreateInfo.renderPass = VKDrawPass->RenderPassObject;
		GraphicsPipelineCreateInfo.subpass = Subpass->Binding_Index;
		GraphicsPipelineCreateInfo.stageCount = 2;
		GraphicsPipelineCreateInfo.pStages = STAGEs;
		GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;		//Optional
		GraphicsPipelineCreateInfo.basePipelineIndex = -1;					//Optional
		GraphicsPipelineCreateInfo.flags = 0;
		GraphicsPipelineCreateInfo.pNext = nullptr;
		if (vkCreateGraphicsPipelines(Vulkan_GPU->Logical_Device, nullptr, 1, &GraphicsPipelineCreateInfo, nullptr, &VKShaderProgram->PipelineObject) != VK_SUCCESS) {
			delete VKShaderProgram;
			delete STAGEs;
			TuranAPI::LOG_ERROR("vkCreateGraphicsPipelines has failed!");
			return;
		}
		GFX_API::GFX_ShaderProgram SHADERPROGRAM;
		SHADERPROGRAM.ASSET_ID = Asset_ID;
		SHADERPROGRAM.GL_ID = VKShaderProgram;
		SHADERPROGRAMs.push_back(SHADERPROGRAM);


		delete STAGEs;
		TuranAPI::LOG_STATUS("Finished creating Graphics Pipeline");
	}
	void GPU_ContentManager::Delete_MaterialType(unsigned int Asset_ID){
		TuranAPI::LOG_NOTCODED("VK::Unload_GlobalBuffer isn't coded!", true);
	}
	void GPU_ContentManager::Create_MaterialInst(GFX_API::Material_Instance* MATINST_ASSET, unsigned int Asset_ID) {
		if (!MATINST_ASSET) {
			TuranAPI::LOG_ERROR("Create_MaterialInst() has failed because MATINST_ASSET is nullptr!");
			return;
		}

		GFX_API::GFX_ShaderProgram* MatType = Find_GFXShaderProgram_byID(MATINST_ASSET->Material_Type);
		if (!MatType) {
			TuranAPI::LOG_ERROR("Create_MaterialInst() has failed because Material Type isn't found!");
			return;
		}
		VK_ShaderProgram* VKPROGRAM = (VK_ShaderProgram*)MatType->GL_ID;
		
		//Check MaterialData's incompatiblity!
		{
			if (MatTypeGeneralDataDesc.size() != GENERALDATA.size()) {
				TuranAPI::LOG_CRASHING("Link_MaterialType() has failed because GENERALDATA should contain enough data for Type General Datas!");
				return;
			}
			for (unsigned int i = 0; i < GENERALDATA.size(); i++) {
				GFX_API::MaterialInstanceData& input_data = GENERALDATA[i];
				GFX_API::MaterialDataDescriptor* mattype_data = nullptr;

				//Search for matching data
				for (unsigned int bindingsearchindex = 0; bindingsearchindex < MatTypeGeneralDataDesc.size(); bindingsearchindex++) {
					if (MatTypeGeneralDataDesc[bindingsearchindex].BINDINGPOINT == input_data.BINDINGPOINT) {
						if (mattype_data) {
							TuranAPI::LOG_ERROR("Link_MaterialType() has failed because one of the GENERALDATA matches with multiple type general data descriptor!");
							return;
						}
						mattype_data = &MatTypeGeneralDataDesc[bindingsearchindex];
					}
				}
				if (!mattype_data) {
					TuranAPI::LOG_ERROR("Link_MaterialType() has failed because one of the GENERALDATA doesn't match with any type general data descriptor!");
					return;
				}
			}
		}

		//Descriptor Pool and Set Creation
		Create_DescSets(MATTYPE_ASSET->MATERIALTYPEDATA, &VKShaderProgram->SpecialBuffersSetLayout, &VKShaderProgram->SpecialBuffersSet, 1);

		{
			vector<GFX_API::MaterialDataDescriptor&> MatTypeGeneralDataDesc;
			for (unsigned int i = 0; i < MATTYPE_ASSET->MATERIALTYPEDATA.size(); i++) {
				GFX_API::MaterialDataDescriptor& Data = MATTYPE_ASSET->MATERIALTYPEDATA[i];
				if (Data.TYPE == GFX_API::MATERIALDATA_TYPE::CONSTIMAGE_G ||
					Data.TYPE == GFX_API::MATERIALDATA_TYPE::CONSTSAMPLER_G ||
					Data.TYPE == GFX_API::MATERIALDATA_TYPE::CONSTSBUFFER_G ||
					Data.TYPE == GFX_API::MATERIALDATA_TYPE::CONSTUBUFFER_G) {
					MatTypeGeneralDataDesc.push_back(Data);
				}
			}


			CheckMaterialTypeGeneralDatas();
			AllocateBuffersattheSizes();
			CopyDatatotheBuffer();
			UpdateDescriptorSet();
			Add_to_TransferPacket(); //Adding to the packet because of buffer!
		}
	}
	void GPU_ContentManager::Delete_MaterialInst(unsigned int Asset_ID) {

	}


	bool Is_RTSlotList_Valid(const vector<GFX_API::RT_SLOT*>& slots) {
		for (unsigned int slotset_binding = 0; slotset_binding < slots.size(); slotset_binding++) {
			bool is_found = false;
			for (unsigned int slots_vectorindex = 0; slots_vectorindex < slots.size(); slots_vectorindex++) {
				if (slots[slots_vectorindex]->SLOT_Index == slotset_binding) {
					if (!is_found) {
						is_found = true;
					}
					if (is_found) {
						TuranAPI::LOG_ERROR("SlotSet index: " + to_string(slotset_binding) + " is used by more than one of the Slots!");
						return false;
					}
				}
			}
			if (!is_found) {
				TuranAPI::LOG_ERROR("SlotSet index: " + to_string(slotset_binding) + " isn't used by any slots!");
			}
		}
		return true;
	}
	unsigned int GPU_ContentManager::Create_RTSlotSet(unsigned int* RTIDs, unsigned int* SlotIndexes, GFX_API::ATTACHMENT_READSTATE* ReadTypes,
		GFX_API::RT_ATTACHMENTs* AttachmentTypes, GFX_API::OPERATION_TYPE* OperationsTypes, unsigned int SlotCount) {
		vector<GFX_API::RT_SLOT*> SLOTs;
		for (unsigned int i = 0; i < SlotCount; i++) {
			GFX_API::GFX_Texture* RT = Find_GFXTexture_byID(RTIDs[i]);
			if (!RT) {
				TuranAPI::LOG_ERROR("Create_RTSlotSet() has failed because intended RT isn't found!");
				return 0;
			}
			GFX_API::RT_SLOT* SLOT = new GFX_API::RT_SLOT;
			SLOT->ATTACHMENT_TYPE = AttachmentTypes[i];
			SLOT->RT_OPERATIONTYPE = OperationsTypes[i];
			SLOT->RT_READTYPE = ReadTypes[i];
			SLOT->RT = RT;
			SLOT->SLOT_Index = SlotIndexes[i];
			SLOTs.push_back(SLOT);
		}
		if (!Is_RTSlotList_Valid(SLOTs)) {
			TuranAPI::LOG_ERROR("Create_RTSlotSet() has failed because SlotSet isn't valid!");
			return 0;
		}









		RT_SLOTSETs.push_back(GFX_API::RT_SLOTSET());
		GFX_API::RT_SLOTSET& SLOTSET = RT_SLOTSETs[RT_SLOTSETs.size() - 1];
		SLOTSET.SLOTs = new GFX_API::RT_SLOT* [SLOTs.size()];
		for (unsigned int i = 0; i < SLOTs.size(); i++) {
			SLOTSET.SLOTs[i] = SLOTs[i];
		}
		SLOTSET.SLOT_COUNT = SLOTs.size();
		SLOTSET.ID = Create_RTSLOTSETID();
		SLOTSET.GL_ID = nullptr;
	}




	//HELPER FUNCTIONs
	bool GPU_ContentManager::Delete_VertexAttribute(unsigned int Attribute_ID) {
		GFX_API::VertexAttribute* FOUND_ATTRIB = Find_VertexAttribute_byID(Attribute_ID, nullptr);
		for (unsigned int i = 0; i < VERTEXATTRIBLAYOUTs.size(); i++) {
			GFX_API::VertexAttributeLayout& VERTEXATTRIBLAYOUT = VERTEXATTRIBLAYOUTs[i];
			for (unsigned int j = 0; j < VERTEXATTRIBLAYOUT.Attribute_Number; j++) {
				if (VERTEXATTRIBLAYOUT.Attributes[j] == FOUND_ATTRIB) {
					return false;
				}
			}
		}
		unsigned int vector_index = 0;
		void* PTR = Find_VertexAttribute_byID(Attribute_ID, &vector_index);
		if (PTR) {
			VERTEXATTRIBUTEs.erase(VERTEXATTRIBUTEs.begin() + vector_index);
		}
	}

	GFX_API::GFX_GlobalBuffer* GPU_ContentManager::Find_Buffer_byID(unsigned int GlobalBufferID, unsigned int* vector_index) {
		for (unsigned int i = 0; i < GLOBALBUFFERs.size(); i++) {
			if (GLOBALBUFFERs[i].ID == GlobalBufferID) {
				if (vector_index) {
					*vector_index = i;
				}
				return &GLOBALBUFFERs[i];
			}
		}
		TuranAPI::LOG_WARNING("Intended Global Buffer isn't found in GPU_ContentManager!");
		return nullptr;
	}
	GFX_API::GFX_Texture* GPU_ContentManager::Find_GFXTexture_byID(unsigned int Texture_AssetID, unsigned int* vector_index) {
		for (unsigned int i = 0; i < TEXTUREs.size(); i++) {
			GFX_API::GFX_Texture& TEXTURE = TEXTUREs[i];
			if (TEXTURE.ASSET_ID == Texture_AssetID) {
				if (vector_index) {
					*vector_index = i;
				}
				return &TEXTURE;
			}
		}
		TuranAPI::LOG_WARNING("Intended Texture isn't uploaded to GPU!" + to_string(Texture_AssetID));
		return nullptr;
	}
	GFX_API::GFX_ShaderSource* GPU_ContentManager::Find_GFXShaderSource_byID(unsigned int ShaderSource_AssetID, unsigned int* vector_index) {
		for (unsigned int i = 0; i < SHADERSOURCEs.size(); i++) {
			GFX_API::GFX_ShaderSource& SHADERSOURCE = SHADERSOURCEs[i];
			if (SHADERSOURCE.ASSET_ID == ShaderSource_AssetID) {
				if (vector_index) {
					*vector_index = i;
				}
				return &SHADERSOURCE;
			}
		}
		TuranAPI::LOG_WARNING("Intended ShaderSource isn't uploaded to GPU!");
	}
	GFX_API::GFX_ComputeShader* GPU_ContentManager::Find_GFXComputeShader_byID(unsigned int ComputeShader_AssetID, unsigned int* vector_index) {
		for (unsigned int i = 0; i < COMPUTESHADERs.size(); i++) {
			GFX_API::GFX_ComputeShader& SHADERSOURCE = COMPUTESHADERs[i];
			if (SHADERSOURCE.ASSET_ID == ComputeShader_AssetID) {
				if (vector_index) {
					*vector_index = i;
				}
				return &SHADERSOURCE;
			}
		}
		TuranAPI::LOG_WARNING("Intended ComputeShader isn't uploaded to GPU!");
	}
	GFX_API::GFX_ShaderProgram* GPU_ContentManager::Find_GFXShaderProgram_byID(unsigned int ShaderProgram_AssetID, unsigned int* vector_index) {
		for (unsigned int i = 0; i < SHADERPROGRAMs.size(); i++) {
			GFX_API::GFX_ShaderProgram& SHADERPROGRAM = SHADERPROGRAMs[i];
			if (SHADERPROGRAM.ASSET_ID == ShaderProgram_AssetID) {
				if (vector_index) {
					*vector_index = i;
				}
				return &SHADERPROGRAM;
			}
		}
		TuranAPI::LOG_WARNING("Intended ShaderProgram isn't uploaded to GPU!");
	}
	GFX_API::VertexAttribute* GPU_ContentManager::Find_VertexAttribute_byID(unsigned int AttributeID, unsigned int* vector_index) {
		for (unsigned int i = 0; i < VERTEXATTRIBUTEs.size(); i++) {
			GFX_API::VertexAttribute& ATTRIBUTE = VERTEXATTRIBUTEs[i];
			if (ATTRIBUTE.ID == AttributeID) {
				if (vector_index) {
					*vector_index = i;
				}
				return &ATTRIBUTE;
			}
		}
		TuranAPI::LOG_WARNING("Find_VertexAttribute_byID has failed! ID: " + to_string(AttributeID));
		return nullptr;
	}
	GFX_API::VertexAttributeLayout* GPU_ContentManager::Find_VertexAttributeLayout_byID(unsigned int LayoutID, unsigned int* vector_index) {
		for (unsigned int i = 0; i < VERTEXATTRIBLAYOUTs.size(); i++) {
			GFX_API::VertexAttributeLayout& LAYOUT = VERTEXATTRIBLAYOUTs[i];
			if (LAYOUT.ID == LayoutID) {
				if (vector_index) {
					*vector_index = i;
				}
				return &LAYOUT;
			}
		}
		TuranAPI::LOG_WARNING("Find_VertexAttributeLayout_byID has failed! ID: " + to_string(LayoutID));
		return nullptr;
	}
	GFX_API::RT_SLOTSET* GPU_ContentManager::Find_RTSLOTSET_byID(unsigned int SlotSetID, unsigned int* vector_index) {
		for (unsigned int i = 0; i < RT_SLOTSETs.size(); i++) {
			GFX_API::RT_SLOTSET& SLOTSET = RT_SLOTSETs[i];
			if (SLOTSET.ID == SlotSetID) {
				if (vector_index) {
					*vector_index = i;
				}
				return &SLOTSET;
			}
		}
		TuranAPI::LOG_WARNING("Find_RTSLOTSET_byID has failed! ID: " + to_string(SlotSetID));
		return nullptr;
	}
}