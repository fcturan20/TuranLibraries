#include "VK_GPUContentManager.h"
#include "Vulkan_Renderer_Core.h"
#include "Vulkan/VulkanSource/Vulkan_Core.h"
#define VKRENDERER ((Vulkan::Renderer*)GFXRENDERER)
#define VKGPU (((Vulkan::Vulkan_Core*)GFX)->VK_States.GPU_TO_RENDER)

#define MAXDESCSETCOUNT_PERPOOL 100
#define MAXUBUFFERCOUNT_PERPOOL 100
#define MAXSBUFFERCOUNT_PERPOOL 100
#define MAXSAMPLERCOUNT_PERPOOL 100
#define MAXIMAGECOUNT_PERPOOL	100

namespace Vulkan {
	VkBuffer Create_VkBuffer(unsigned int size, VkBufferUsageFlags usage) {
		VkBuffer buffer;

		VkBufferCreateInfo ci{};
		ci.usage = usage;
		if (VKGPU->QUEUEs.size() > 1) {
			ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
		}
		else {
			ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		ci.queueFamilyIndexCount = VKGPU->QUEUEs.size();
		ci.pQueueFamilyIndices = VKGPU->AllQueueFamilies;
		ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		ci.size = size;

		if (vkCreateBuffer(VKGPU->Logical_Device, &ci, nullptr, &buffer) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Create_VkBuffer has failed!");
		}
		return buffer;
	}
	GPU_ContentManager::GPU_ContentManager() : MESHBUFFERs(*GFX->JobSys), TEXTUREs(*GFX->JobSys), GLOBALBUFFERs(*GFX->JobSys), SHADERSOURCEs(*GFX->JobSys),
		SHADERPROGRAMs(*GFX->JobSys), SHADERPINSTANCEs(*GFX->JobSys), VERTEXATTRIBUTEs(*GFX->JobSys), VERTEXATTRIBLAYOUTs(*GFX->JobSys), RT_SLOTSETs(*GFX->JobSys),
		DescSets_toCreateUpdate(*GFX->JobSys), DescSets_toCreate(*GFX->JobSys), DescSets_toJustUpdate(*GFX->JobSys){

		//GPU Local Memory Allocation
		if (VKGPU->GPULOCAL_ALLOC.FullSize) {
			VkMemoryRequirements memrequirements;
			VkBufferUsageFlags USAGEFLAGs = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			uint64_t AllocSize = VKGPU->GPULOCAL_ALLOC.FullSize;
			VkBuffer GPULOCAL_buf = Create_VkBuffer(AllocSize, USAGEFLAGs);
			vkGetBufferMemoryRequirements(VKGPU->Logical_Device, GPULOCAL_buf, &memrequirements);
			if (!(memrequirements.memoryTypeBits & (1 << VKGPU->GPULOCAL_ALLOC.MemoryTypeIndex))) {
				LOG_CRASHING_TAPI("GPU Local Memory Allocation doesn't support the MemoryType!");
				return;
			}
			VkMemoryAllocateInfo ci{};
			ci.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			ci.allocationSize = memrequirements.size;
			ci.memoryTypeIndex = VKGPU->GPULOCAL_ALLOC.MemoryTypeIndex;
			VkDeviceMemory allocated_memory;
			if (vkAllocateMemory(VKGPU->Logical_Device, &ci, nullptr, &allocated_memory) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("GPU_ContentManager initialization has failed because vkAllocateMemory GPULocalBuffer has failed!");
			}
			VKGPU->GPULOCAL_ALLOC.Allocated_Memory = allocated_memory;
			VKGPU->GPULOCAL_ALLOC.UnusedSize = AllocSize;
			VKGPU->GPULOCAL_ALLOC.MappedMemory = nullptr;
			VKGPU->GPULOCAL_ALLOC.Buffer = GPULOCAL_buf;
			if (vkBindBufferMemory(VKGPU->Logical_Device, GPULOCAL_buf, allocated_memory, 0) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Binding buffer to the allocated memory has failed!");
			}
		}
		//Host Visible Memory Allocation
		if (VKGPU->HOSTVISIBLE_ALLOC.FullSize) {
			VkMemoryRequirements memrequirements;
			VkBufferUsageFlags USAGEFLAGs = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			uint64_t AllocSize = VKGPU->HOSTVISIBLE_ALLOC.FullSize;
			VkBuffer stagingbuffer = Create_VkBuffer(AllocSize, USAGEFLAGs);
			vkGetBufferMemoryRequirements(VKGPU->Logical_Device, stagingbuffer, &memrequirements);
			if (!(memrequirements.memoryTypeBits & (1u << VKGPU->HOSTVISIBLE_ALLOC.MemoryTypeIndex))) {
				LOG_CRASHING_TAPI("Host Visible Memory Allocation doesn't support the related MemoryType!");
				return;
			}

			VkMemoryAllocateInfo ci{};
			ci.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			ci.allocationSize = memrequirements.size;
			ci.memoryTypeIndex = VKGPU->HOSTVISIBLE_ALLOC.MemoryTypeIndex;
			VkDeviceMemory allocated_memory;
			if (vkAllocateMemory(VKGPU->Logical_Device, &ci, nullptr, &allocated_memory) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("GPU_ContentManager has failed because vkAllocateMemory has failed!");
			}
			VKGPU->HOSTVISIBLE_ALLOC.Allocated_Memory = allocated_memory;
			VKGPU->HOSTVISIBLE_ALLOC.UnusedSize = AllocSize;
			VKGPU->HOSTVISIBLE_ALLOC.Buffer = stagingbuffer;
			if (vkBindBufferMemory(VKGPU->Logical_Device, stagingbuffer, allocated_memory, 0) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Binding buffer to the allocated memory has failed!");
			}
			if (vkMapMemory(VKGPU->Logical_Device, allocated_memory, 0, memrequirements.size, 0, &VKGPU->HOSTVISIBLE_ALLOC.MappedMemory) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Mapping the HOSTVISIBLE memory has failed!");
				return;
			}
		}
		//Fast Host Visible Memory Allocation
		if (VKGPU->FASTHOSTVISIBLE_ALLOC.FullSize) {
			VkMemoryRequirements memrequirements;
			VkBufferUsageFlags USAGEFLAGs = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			uint64_t AllocSize = VKGPU->FASTHOSTVISIBLE_ALLOC.FullSize;
			VkBuffer stagingbuffer = Create_VkBuffer(AllocSize, USAGEFLAGs);
			vkGetBufferMemoryRequirements(VKGPU->Logical_Device, stagingbuffer, &memrequirements);
			if (!(memrequirements.memoryTypeBits & (1u << VKGPU->FASTHOSTVISIBLE_ALLOC.MemoryTypeIndex))) {
				LOG_CRASHING_TAPI("Fast Host Visible Memory Allocation doesn't support the related MemoryType!");
				return;
			}

			VkMemoryAllocateInfo ci{};
			ci.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			ci.allocationSize = memrequirements.size;
			ci.memoryTypeIndex = VKGPU->FASTHOSTVISIBLE_ALLOC.MemoryTypeIndex;
			VkDeviceMemory allocated_memory;
			if (vkAllocateMemory(VKGPU->Logical_Device, &ci, nullptr, &allocated_memory) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("GPU_ContentManager has failed because vkAllocateMemory has failed!");
			}
			VKGPU->FASTHOSTVISIBLE_ALLOC.Allocated_Memory = allocated_memory;
			VKGPU->FASTHOSTVISIBLE_ALLOC.UnusedSize = AllocSize;
			VKGPU->FASTHOSTVISIBLE_ALLOC.Buffer = stagingbuffer;
			if (vkBindBufferMemory(VKGPU->Logical_Device, stagingbuffer, allocated_memory, 0) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Binding buffer to the allocated memory has failed!");
			}
			if (vkMapMemory(VKGPU->Logical_Device, allocated_memory, 0, memrequirements.size, 0, &VKGPU->FASTHOSTVISIBLE_ALLOC.MappedMemory) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Mapping the FASTHOSTVISIBLE memory has failed!");
				return;
			}
		}
		//Readback Memory Allocation
		if (VKGPU->READBACK_ALLOC.FullSize) {
			VkMemoryRequirements memrequirements;
			VkBufferUsageFlags USAGEFLAGs = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			uint64_t AllocSize = VKGPU->READBACK_ALLOC.FullSize;
			VkBuffer READBACK_buf = Create_VkBuffer(AllocSize, USAGEFLAGs);
			vkGetBufferMemoryRequirements(VKGPU->Logical_Device, READBACK_buf, &memrequirements);
			if (!(memrequirements.memoryTypeBits & (1u << VKGPU->READBACK_ALLOC.MemoryTypeIndex))) {
				LOG_CRASHING_TAPI("GPU Local Memory Allocation doesn't support the related MemoryType!");
				return;
			}
			VkMemoryAllocateInfo ci{};
			ci.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			ci.allocationSize = memrequirements.size;
			ci.memoryTypeIndex = VKGPU->READBACK_ALLOC.MemoryTypeIndex;
			VkDeviceMemory allocated_memory;
			if (vkAllocateMemory(VKGPU->Logical_Device, &ci, nullptr, &allocated_memory) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("GPU_ContentManager initialization has failed because vkAllocateMemory GPULocalBuffer has failed!");
			}
			VKGPU->READBACK_ALLOC.Allocated_Memory = allocated_memory;
			VKGPU->READBACK_ALLOC.UnusedSize = AllocSize;
			VKGPU->READBACK_ALLOC.Buffer = READBACK_buf;
			if (vkBindBufferMemory(VKGPU->Logical_Device, READBACK_buf, allocated_memory, 0) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Binding buffer to the allocated memory has failed!");
			}
			if (vkMapMemory(VKGPU->Logical_Device, allocated_memory, 0, memrequirements.size, 0, &VKGPU->READBACK_ALLOC.MappedMemory) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Mapping the READBACK memory has failed!");
				return;
			}
		}

		UnboundDescSetImageCount[0] = 0;
		UnboundDescSetImageCount[1] = 0;
		UnboundDescSetSamplerCount[0] = 0;
		UnboundDescSetSamplerCount[1] = 0;
		UnboundDescSetSBufferCount[0] = 0;
		UnboundDescSetSBufferCount[1] = 0;
		UnboundDescSetUBufferCount[0] = 0;
		UnboundDescSetUBufferCount[1] = 0;

		//Material Related Descriptor Pool Creation
		{
			VkDescriptorPoolCreateInfo dp_ci = {};
			dp_ci.flags = 0;
			dp_ci.maxSets = MAXDESCSETCOUNT_PERPOOL;
			dp_ci.pNext = nullptr;
			dp_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			VkDescriptorPoolSize SIZEs[4];
			SIZEs[0].descriptorCount = MAXIMAGECOUNT_PERPOOL;
			SIZEs[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			SIZEs[1].descriptorCount = MAXSAMPLERCOUNT_PERPOOL;
			SIZEs[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
			SIZEs[2].descriptorCount = MAXSBUFFERCOUNT_PERPOOL;
			SIZEs[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			SIZEs[3].descriptorCount = MAXUBUFFERCOUNT_PERPOOL;
			SIZEs[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			dp_ci.poolSizeCount = 4;
			dp_ci.pPoolSizes = SIZEs;
			if (vkCreateDescriptorPool(VKGPU->Logical_Device, &dp_ci, nullptr, &MaterialRelated_DescPool.pool) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Material Related Descriptor Pool Creation has failed! So GFXContentManager system initialization has failed!");
				return;
			}

			MaterialRelated_DescPool.REMAINING_IMAGE.DirectStore(MAXIMAGECOUNT_PERPOOL);
			MaterialRelated_DescPool.REMAINING_SAMPLER.DirectStore(MAXSAMPLERCOUNT_PERPOOL);
			MaterialRelated_DescPool.REMAINING_SBUFFER.DirectStore(MAXSBUFFERCOUNT_PERPOOL);
			MaterialRelated_DescPool.REMAINING_UBUFFER.DirectStore(MAXUBUFFERCOUNT_PERPOOL);
			MaterialRelated_DescPool.REMAINING_SET.DirectStore(MAXDESCSETCOUNT_PERPOOL);
		}

		//Default Sampler Creation
		{
			VkSamplerCreateInfo ci = {};
			ci.addressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
			ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			ci.anisotropyEnable = false;
			ci.borderColor = VkBorderColor::VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
			ci.compareEnable = false;
			ci.compareOp = VkCompareOp::VK_COMPARE_OP_ALWAYS;
			ci.flags = 0;
			ci.magFilter = VkFilter::VK_FILTER_LINEAR;
			ci.minFilter = VkFilter::VK_FILTER_LINEAR;
			ci.maxAnisotropy = 1.0f;
			ci.maxLod = 0.0f;
			ci.minLod = 0.0f;
			ci.mipLodBias = 0.0f;
			ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			ci.pNext = nullptr;
			ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			ci.unnormalizedCoordinates = VK_FALSE;
			if (vkCreateSampler(VKGPU->Logical_Device, &ci, nullptr, &DefaultSampler) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("GPU_ContentManager initialization has failed because vkCreateSampler has failed!");
			}
		}
	}
	GPU_ContentManager::~GPU_ContentManager() {

	}
	void GPU_ContentManager::Unload_AllResources() {

	}
	VK_MemoryAllocation* GetMEMORYALLOCATIONHANDLE(const GFX_API::SUBALLOCATEBUFFERTYPEs& TYPE);

	void GPU_ContentManager::Resource_Finalizations() {
		//Create Global Buffer Descriptors etc
		{
			std::unique_lock<std::mutex> GlobalBufferLocker;
			GLOBALBUFFERs.PauseAllOperations(GlobalBufferLocker);

			//Descriptor Set Layout creation
			unsigned int UNIFORMBUFFER_COUNT = 0, STORAGEBUFFER_COUNT = 0;
			{
				vector<VkDescriptorSetLayoutBinding> bindings;
				for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
					for (unsigned int i = 0; i < GLOBALBUFFERs.size(ThreadID); i++) {
						VK_GlobalBuffer* globbuf = GLOBALBUFFERs.get(ThreadID, i);
						bindings.push_back(VkDescriptorSetLayoutBinding());
						bindings[i].binding = globbuf->BINDINGPOINT;
						bindings[i].descriptorCount = 1;
						VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
						if (globbuf->isUniform) {
							bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						}
						else {
							bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						}
						bindings[i].pImmutableSamplers = VK_NULL_HANDLE;
						bindings[i].stageFlags = Find_VkShaderStages(globbuf->ACCESSED_STAGEs);
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
				}
				
				if (bindings.size()) {
					VkDescriptorSetLayoutCreateInfo DescSetLayout_ci = {};
					DescSetLayout_ci.flags = 0;
					DescSetLayout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					DescSetLayout_ci.pNext = nullptr;
					DescSetLayout_ci.bindingCount = bindings.size();
					DescSetLayout_ci.pBindings = bindings.data();
					if (vkCreateDescriptorSetLayout(VKGPU->Logical_Device, &DescSetLayout_ci, nullptr, &GlobalBuffers_DescSetLayout) != VK_SUCCESS) {
						LOG_CRASHING_TAPI("Create_RenderGraphResources() has failed at vkCreateDescriptorSetLayout()!");
						return;
					}
				}
			}

			//Descriptor Pool and Descriptor Set creation
			vector<VkDescriptorPoolSize> poolsizes;
			if (UNIFORMBUFFER_COUNT > 0) {
				VkDescriptorPoolSize UDescCount;
				UDescCount.descriptorCount = UNIFORMBUFFER_COUNT;
				UDescCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				poolsizes.push_back(UDescCount);
			}
			if (STORAGEBUFFER_COUNT > 0) {
				VkDescriptorPoolSize SDescCount;
				SDescCount.descriptorCount = STORAGEBUFFER_COUNT;
				SDescCount.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				poolsizes.push_back(SDescCount);
			} 
			if (poolsizes.size()) {
				VkDescriptorPoolCreateInfo descpool_ci = {};
				descpool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				descpool_ci.maxSets = 1;
				descpool_ci.pPoolSizes = poolsizes.data();
				descpool_ci.poolSizeCount = poolsizes.size();
				descpool_ci.flags = 0;
				descpool_ci.pNext = nullptr;
				if (poolsizes.size()) {
					if (vkCreateDescriptorPool(VKGPU->Logical_Device, &descpool_ci, nullptr, &GlobalBuffers_DescPool) != VK_SUCCESS) {
						LOG_CRASHING_TAPI("Create_RenderGraphResources() has failed at vkCreateDescriptorPool()!");
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
							LOG_CRASHING_TAPI("Create_RenderGraphResources() has failed at vkAllocateDescriptorSets()!");
						}
					}
				}
			}

			//Update Descriptor Set
			{
				vector<VkWriteDescriptorSet> UpdateInfos;
				vector<VkDescriptorBufferInfo*> DeleteList;
				for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
					for (unsigned int i = 0; i < GLOBALBUFFERs.size(ThreadID); i++) {
						VK_GlobalBuffer* buf = GLOBALBUFFERs.get(ThreadID, i);
						VkWriteDescriptorSet info = {};
						info.descriptorCount = 1;
						if (buf->isUniform) {
							info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						}
						else {
							info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						}
						info.dstArrayElement = 0;
						info.dstBinding = buf->BINDINGPOINT;
						info.dstSet = GlobalBuffers_DescSet;
						VkDescriptorBufferInfo* descbufinfo = new VkDescriptorBufferInfo;
						VkBuffer nonobj; VkDeviceSize nonoffset;
						descbufinfo->buffer = GetMEMORYALLOCATIONHANDLE(buf->Block.Type)->Buffer;
						descbufinfo->offset = buf->Block.Offset;
						descbufinfo->range = static_cast<VkDeviceSize>(buf->DATA_SIZE);
						info.pBufferInfo = descbufinfo;
						info.pNext = nullptr;
						info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						DeleteList.push_back(descbufinfo);
						UpdateInfos.push_back(info);
					}
				}

				vkUpdateDescriptorSets(VKGPU->Logical_Device, UpdateInfos.size(), UpdateInfos.data(), 0, nullptr);

				for (unsigned int DeleteIndex = 0; DeleteIndex < DeleteList.size(); DeleteIndex++) {
					delete DeleteList[DeleteIndex];
				}
			}
		}
	}

	TAPIResult GPU_ContentManager::Suballocate_Buffer(VkBuffer BUFFER, GFX_API::SUBALLOCATEBUFFERTYPEs GPUregion, VkDeviceSize& MemoryOffset, VkDeviceSize& RequiredSize, bool ShouldBind) {
		VkMemoryRequirements bufferreq;
		vkGetBufferMemoryRequirements(VKGPU->Logical_Device, BUFFER, &bufferreq);
		RequiredSize = bufferreq.size;

		VkDeviceSize Offset = 0;
		bool Found_Offset = false;
		VK_MemoryAllocation* MEMALLOC = nullptr;
		switch (GPUregion) {
		case GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL:
			MEMALLOC = &VKGPU->GPULOCAL_ALLOC;
			break;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE:
			MEMALLOC = &VKGPU->HOSTVISIBLE_ALLOC;
			break;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::FASTHOSTVISIBLE:
			MEMALLOC = &VKGPU->FASTHOSTVISIBLE_ALLOC;
			break;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::READBACK:
			MEMALLOC = &VKGPU->READBACK_ALLOC;
			break;
		default:
			LOG_CRASHING_TAPI("Suballocate_Buffer() has failed because SUBALLOCATEBUFFERTYPE isn't supported!");
			return TAPI_NOTCODED;
		}
		if (!(bufferreq.memoryTypeBits & (1u << MEMALLOC->MemoryTypeIndex))) {
			LOG_ERROR_TAPI("Intended buffer doesn't support to be stored in specified memory region!");
			return TAPI_FAIL;
		}
		MEMALLOC->Locker.lock();
		if (MEMALLOC->Allocated_Blocks.size() == 0) {
			VK_MemoryBlock FirstBlock;
			FirstBlock.isEmpty = false;
			FirstBlock.Size = RequiredSize;
			FirstBlock.Offset = 0;
			if (!(MEMALLOC->UnusedSize - RequiredSize)) {
				LOG_CRASHING_TAPI("LimitedSubtract_weak() has failed in Suballocate_Buffer(), returning TAPI_FAIL!");
				return TAPI_FAIL;
			}
			MEMALLOC->Allocated_Blocks.push_back(FirstBlock);
			Offset = FirstBlock.Offset;
			Found_Offset = true;
			MEMALLOC->Locker.unlock();
		}
		for (unsigned int i = 0; i < MEMALLOC->Allocated_Blocks.size() && !Found_Offset; i++) {
			VK_MemoryBlock& Block = MEMALLOC->Allocated_Blocks[i];
			if (Block.isEmpty && RequiredSize < Block.Size && RequiredSize > Block.Size / 5 * 3) {
				Block.isEmpty = false;
				Offset = Block.Offset;
				Found_Offset = true;
				MEMALLOC->Locker.unlock();
				break;
			}
		}
		//There is no empty block, so create new one!
		if (MEMALLOC->UnusedSize > RequiredSize && !Found_Offset) {
			VK_MemoryBlock& LastBlock = MEMALLOC->Allocated_Blocks[MEMALLOC->Allocated_Blocks.size() - 1];
			VK_MemoryBlock NewBlock;
			NewBlock.isEmpty = false;
			NewBlock.Size = RequiredSize;
			NewBlock.Offset = LastBlock.Offset + LastBlock.Size;
			MEMALLOC->UnusedSize -= RequiredSize;
			MEMALLOC->Allocated_Blocks.push_back(NewBlock);
			Offset = NewBlock.Offset;
			Found_Offset = true;
			MEMALLOC->Locker.unlock();
		}

		if (!Found_Offset) {
			LOG_CRASHING_TAPI("VKContentManager->Suballocate_Buffer() has failed because there is no way you can create a buffer at this size!");
			return TAPI_FAIL;
		}
		if (ShouldBind) {
			Offset = (Offset % bufferreq.alignment) ? (Offset) : (((Offset / bufferreq.alignment) + 1) * bufferreq.alignment);

			if (vkBindBufferMemory(VKGPU->Logical_Device, BUFFER, MEMALLOC->Allocated_Memory, Offset) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("VKContentManager->Suballocate_Buffer() has failed at vkBindBufferMemory()!");
				return TAPI_FAIL;
			}
		}

		MemoryOffset = Offset;
		return TAPI_SUCCESS;
	}
	TAPIResult GPU_ContentManager::Suballocate_Image(VK_Texture& Texture) {
		VkMemoryRequirements req;
		vkGetImageMemoryRequirements(VKGPU->Logical_Device, Texture.Image, &req);
		VkDeviceSize size = req.size;

		VkDeviceSize Offset = 0;
		bool Found_Offset = false;
		VK_MemoryAllocation* MEMALLOC = nullptr;
		switch (Texture.Block.Type) {
		case GFX_API::SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE:
			MEMALLOC = &VKGPU->HOSTVISIBLE_ALLOC;
			break;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL:
			MEMALLOC = &VKGPU->GPULOCAL_ALLOC;
			break;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::FASTHOSTVISIBLE:
			MEMALLOC = &VKGPU->FASTHOSTVISIBLE_ALLOC;
			break;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::READBACK:
			MEMALLOC = &VKGPU->READBACK_ALLOC;
			break;
		default:
			LOG_CRASHING_TAPI("VKContentManager->Suballocate_Image() has failed because SUBALLOCATEBUFFERTYPE isn't supported!");
			return TAPI_NOTCODED;
		}
		if (!(req.memoryTypeBits & (1u << MEMALLOC->MemoryTypeIndex))) {
			LOG_ERROR_TAPI("Intended texture doesn't support to be stored in the specified memory region!");
			return TAPI_FAIL;
		}
		MEMALLOC->Locker.lock();
		if (MEMALLOC->Allocated_Blocks.size() == 0) {
			if (MEMALLOC->UnusedSize < req.size) {
				LOG_ERROR_TAPI("Intended texture is bigger than memory allocation itself!");
				return TAPI_FAIL;
			}
			VK_MemoryBlock FirstBlock;
			FirstBlock.isEmpty = false;
			FirstBlock.Size = size;
			FirstBlock.Offset = 0;
			MEMALLOC->UnusedSize -= size;
			MEMALLOC->Allocated_Blocks.push_back(FirstBlock);

			Offset = FirstBlock.Offset;
			Found_Offset = true;
			MEMALLOC->Locker.unlock();
		}
		for (unsigned int i = 0; i < MEMALLOC->Allocated_Blocks.size() && !Found_Offset; i++) {
			VK_MemoryBlock& Block = MEMALLOC->Allocated_Blocks[i];
			if (Block.isEmpty && size < Block.Size && size > Block.Size / 5 * 3) {
				Block.isEmpty = false;
				Offset = Block.Offset;
				Found_Offset = true;
				MEMALLOC->Locker.unlock();
				break;
			}
		}
		//There is no empty block, so create new one!
		if (MEMALLOC->UnusedSize > size && !Found_Offset) {
			VK_MemoryBlock& LastBlock = MEMALLOC->Allocated_Blocks[MEMALLOC->Allocated_Blocks.size() - 1];
			VK_MemoryBlock NewBlock;
			NewBlock.isEmpty = false;
			NewBlock.Size = size;
			NewBlock.Offset = LastBlock.Offset + LastBlock.Size;
			MEMALLOC->UnusedSize -= size;
			MEMALLOC->Allocated_Blocks.push_back(NewBlock);
			Offset = NewBlock.Offset;
			Found_Offset = true;
			MEMALLOC->Locker.unlock();
		}
		Offset = (Offset % req.alignment) ? (((Offset / req.alignment) + 1) * req.alignment) : (Offset);

		if (!Found_Offset) {
			LOG_CRASHING_TAPI("VKContentManager->Suballocate_Image() has failed becuse there is no way you can create a suballocate at this size!");
			return TAPI_INVALIDARGUMENT;
		}

		if (vkBindImageMemory(VKGPU->Logical_Device, Texture.Image, MEMALLOC->Allocated_Memory, Offset) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("VKContentManager->Suballocate_Image() has failed at VkBindImageMemory()!");
			return TAPI_FAIL;
		}
		Texture.Block.Offset = Offset;
		return TAPI_SUCCESS;
	}

	unsigned int GPU_ContentManager::Calculate_sizeofVertexLayout(const VK_VertexAttribute* const* ATTRIBUTEs, unsigned int count) {
		unsigned int size = 0;
		for (unsigned int i = 0; i < count; i++) {
			size += GFX_API::Get_UNIFORMTYPEs_SIZEinbytes(ATTRIBUTEs[i]->DATATYPE);
		}
		return size;
	}
	TAPIResult GPU_ContentManager::Create_VertexAttribute(const GFX_API::DATA_TYPE& TYPE, const bool& is_perVertex, GFX_API::GFXHandle& Handle) {
		unsigned char ThisThreadIndex = GFX->JobSys->GetThisThreadIndex();
		if (!is_perVertex) {
			LOG_ERROR_TAPI("A Vertex Attribute description is not per vertex, so creation fail at it! Descriptions that are after it aren't created!");
			return TAPI_INVALIDARGUMENT;
		}
		VK_VertexAttribute* VA = new VK_VertexAttribute;
		VA->DATATYPE = TYPE;
		VERTEXATTRIBUTEs.push_back(ThisThreadIndex, VA);
		Handle = VA;
		return TAPI_SUCCESS;
	}

	
	bool GPU_ContentManager::Create_DescSet(VK_DescSet* Set) {
		if(!MaterialRelated_DescPool.REMAINING_IMAGE.LimitedSubtract_weak(Set->DescImagesCount, 0) ||
			!MaterialRelated_DescPool.REMAINING_SAMPLER.LimitedSubtract_weak(Set->DescSamplersCount, 0) ||
			!MaterialRelated_DescPool.REMAINING_SBUFFER.LimitedSubtract_weak(Set->DescSBuffersCount, 0) ||
			!MaterialRelated_DescPool.REMAINING_UBUFFER.LimitedSubtract_weak(Set->DescUBuffersCount, 0) ||
			!MaterialRelated_DescPool.REMAINING_SET.LimitedSubtract_weak(1, 0)) {
			LOG_ERROR_TAPI("Create_DescSets() has failed because descriptor pool doesn't have enough space!");
			return false;
		}

		DescSets_toCreate.push_back(GFX->JobSys->GetThisThreadIndex(), Set);
		return true;
	}


	bool GPU_ContentManager::Delete_VertexAttribute(GFX_API::GFXHandle Attribute_ID) {
		VK_VertexAttribute* FOUND_ATTRIB = GFXHandleConverter(VK_VertexAttribute*, Attribute_ID);
		//Check if it's still in use in a Layout
		std::unique_lock<std::mutex> SearchLock;
		VERTEXATTRIBLAYOUTs.PauseAllOperations(SearchLock);
		for (unsigned int ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
			for (unsigned int i = 0; i < VERTEXATTRIBLAYOUTs.size(ThreadID); i++) {
				VK_VertexAttribLayout* VERTEXATTRIBLAYOUT = VERTEXATTRIBLAYOUTs.get(ThreadID, i);
				for (unsigned int j = 0; j < VERTEXATTRIBLAYOUT->Attribute_Number; j++) {
					if (VERTEXATTRIBLAYOUT->Attributes[j] == FOUND_ATTRIB) {
						return false;
					}
				}
			}
		}
		SearchLock.unlock();


		unsigned int vector_index = 0;
		VERTEXATTRIBUTEs.PauseAllOperations(SearchLock);
		for (unsigned int ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
			unsigned int elementindex = 0;
			if (VERTEXATTRIBUTEs.Search(FOUND_ATTRIB, ThreadID, elementindex)) {
				VERTEXATTRIBUTEs.erase(ThreadID, elementindex);
				delete FOUND_ATTRIB;
			}
		}
		return true;
	}
	TAPIResult GPU_ContentManager::Create_VertexAttributeLayout(const vector<GFX_API::GFXHandle>& Attributes, GFX_API::GFXHandle& Handle) {
		VK_VertexAttribLayout* Layout = new VK_VertexAttribLayout;
		Layout->Attribute_Number = Attributes.size();
		Layout->Attributes = new VK_VertexAttribute * [Attributes.size()];
		for (unsigned int i = 0; i < Attributes.size(); i++) {
			VK_VertexAttribute* Attribute = GFXHandleConverter(VK_VertexAttribute*, Attributes[i]);
			if (!Attribute) {
				LOG_ERROR_TAPI("You referenced an uncreated attribute to Create_VertexAttributeLayout!");
				delete Layout;
				return TAPI_INVALIDARGUMENT;
			}
			Layout->Attributes[i] = Attribute;
		}
		unsigned int size_pervertex = Calculate_sizeofVertexLayout(Layout->Attributes, Attributes.size());
		Layout->size_perVertex = size_pervertex;
		Layout->BindingDesc.binding = 0;
		Layout->BindingDesc.stride = size_pervertex;
		Layout->BindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		Layout->AttribDescs = new VkVertexInputAttributeDescription[Attributes.size()];
		Layout->AttribDesc_Count = Attributes.size();
		unsigned int stride_ofcurrentattribute = 0;
		for (unsigned int i = 0; i < Attributes.size(); i++) {
			Layout->AttribDescs[i].binding = 0;
			Layout->AttribDescs[i].location = i;
			Layout->AttribDescs[i].offset = stride_ofcurrentattribute;
			Layout->AttribDescs[i].format = Find_VkFormat_byDataType(Layout->Attributes[i]->DATATYPE);
			stride_ofcurrentattribute += GFX_API::Get_UNIFORMTYPEs_SIZEinbytes(Layout->Attributes[i]->DATATYPE);
		}
		VERTEXATTRIBLAYOUTs.push_back(GFX->JobSys->GetThisThreadIndex(), Layout);
		Handle = Layout;
		return TAPI_SUCCESS;
	}
	void GPU_ContentManager::Delete_VertexAttributeLayout(GFX_API::GFXHandle Layout_ID) {
		LOG_NOTCODED_TAPI("Delete_VertexAttributeLayout() isn't coded yet!", true);
	}


	TAPIResult GPU_ContentManager::Create_StagingBuffer(unsigned int DATASIZE, const GFX_API::SUBALLOCATEBUFFERTYPEs& MemoryRegion, GFX_API::GFXHandle& Handle) {
		if (!DATASIZE) {
			LOG_ERROR_TAPI("Staging Buffer DATASIZE is zero!");
			return TAPI_INVALIDARGUMENT;
		}
		if (MemoryRegion == GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL) {
			LOG_ERROR_TAPI("You can't create a staging buffer in DEVICELOCAL memory!");
			return TAPI_INVALIDARGUMENT;
		}
		VkBufferCreateInfo psuedo_ci = {};
		psuedo_ci.flags = 0;
		psuedo_ci.pNext = nullptr;
		if (VKGPU->QUEUEs.size() > 1) {
			psuedo_ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
		}
		else {
			psuedo_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		psuedo_ci.pQueueFamilyIndices = VKGPU->AllQueueFamilies;
		psuedo_ci.queueFamilyIndexCount = VKGPU->QUEUEs.size();
		psuedo_ci.size = DATASIZE;
		psuedo_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		psuedo_ci.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			| VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		VkBuffer Bufferobj;
		if (vkCreateBuffer(VKGPU->Logical_Device, &psuedo_ci, nullptr, &Bufferobj) != VK_SUCCESS) {
			LOG_ERROR_TAPI("Intended staging buffer's creation failed at vkCreateBuffer()!");
			return TAPI_FAIL;
		}
		
		VkDeviceSize Offset = 0, RequiredSize = 0;
		if (Suballocate_Buffer(Bufferobj, MemoryRegion, Offset, RequiredSize, false) != VK_SUCCESS) {
			LOG_ERROR_TAPI("Suballocation has failed, so staging buffer creation too!");
			return TAPI_FAIL;
		}
		MemoryBlock* StagingBuffer = new MemoryBlock;
		StagingBuffer->Offset = Offset;
		StagingBuffer->Type = MemoryRegion;
		vkDestroyBuffer(VKGPU->Logical_Device, Bufferobj, nullptr);
		Handle = StagingBuffer;
		return TAPI_SUCCESS;
	}
	TAPIResult GPU_ContentManager::Uploadto_StagingBuffer(GFX_API::GFXHandle StagingBufferHandle, const void* DATA, unsigned int DATA_SIZE, unsigned int OFFSET) {
		MemoryBlock* SB = GFXHandleConverter(MemoryBlock*, StagingBufferHandle);
		void* MappedMemory = nullptr;
		switch (SB->Type) {
		case GFX_API::SUBALLOCATEBUFFERTYPEs::FASTHOSTVISIBLE:
			MappedMemory = VKGPU->FASTHOSTVISIBLE_ALLOC.MappedMemory;
			break;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE:
			MappedMemory = VKGPU->HOSTVISIBLE_ALLOC.MappedMemory;
			break;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::READBACK:
			MappedMemory = VKGPU->READBACK_ALLOC.MappedMemory;
			break;
		default:
			LOG_NOTCODED_TAPI("This type of memory block isn't supported for a staging buffer! Uploadto_StagingBuffer() has failed!", true);
			return TAPI_NOTCODED;
		}
		VkDeviceSize UploadOFFSET = static_cast<VkDeviceSize>(OFFSET);
		memcpy(((char*)MappedMemory) + SB->Offset + UploadOFFSET, DATA, DATA_SIZE);
		return TAPI_SUCCESS;
	}
	void GPU_ContentManager::Delete_StagingBuffer(GFX_API::GFXHandle StagingBufferHandle) {
		MemoryBlock* SB = GFXHandleConverter(MemoryBlock*, StagingBufferHandle);
		std::vector<VK_MemoryBlock>* MemoryBlockList = nullptr;
		std::mutex* MutexPTR = nullptr;
		switch (SB->Type) {
		case GFX_API::SUBALLOCATEBUFFERTYPEs::FASTHOSTVISIBLE:
			MutexPTR = &VKGPU->FASTHOSTVISIBLE_ALLOC.Locker;
			MemoryBlockList = &VKGPU->FASTHOSTVISIBLE_ALLOC.Allocated_Blocks;
			break;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE:
			MutexPTR = &VKGPU->HOSTVISIBLE_ALLOC.Locker;
			MemoryBlockList = &VKGPU->HOSTVISIBLE_ALLOC.Allocated_Blocks;
			break;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::READBACK:
			MutexPTR = &VKGPU->READBACK_ALLOC.Locker;
			MemoryBlockList = &VKGPU->READBACK_ALLOC.Allocated_Blocks;
			break;
		default:
			LOG_NOTCODED_TAPI("This type of memory block isn't supported for a staging buffer! Delete_StagingBuffer() has failed!", true);
			return;
		}
		MutexPTR->lock();
		for (unsigned int i = 0; i < MemoryBlockList->size(); i++) {
			if ((*MemoryBlockList)[i].Offset == SB->Offset) {
				(*MemoryBlockList)[i].isEmpty = true;
				MutexPTR->unlock();
				return;
			}
		}
		MutexPTR->unlock();
		LOG_ERROR_TAPI("Delete_StagingBuffer() didn't delete any memory!");
	}

	TAPIResult GPU_ContentManager::Create_VertexBuffer(GFX_API::GFXHandle AttributeLayout, unsigned int VertexCount, 
		GFX_API::SUBALLOCATEBUFFERTYPEs MemoryType, GFX_API::GFXHandle& VertexBufferHandle) {
		if (!VertexCount) {
			LOG_ERROR_TAPI("GFXContentManager->Create_MeshBuffer() has failed because vertex_count is zero!");
			return TAPI_INVALIDARGUMENT;
		}

		VK_VertexAttribLayout* Layout = GFXHandleConverter(VK_VertexAttribLayout*, AttributeLayout);
		if (!Layout) {
			LOG_ERROR_TAPI("GFXContentManager->Create_MeshBuffer() has failed because Attribute Layout ID is invalid!");
			return TAPI_INVALIDARGUMENT;
		}

		unsigned int TOTALDATA_SIZE = Layout->size_perVertex * VertexCount;

		VK_VertexBuffer* VKMesh = new VK_VertexBuffer;
		VkBufferUsageFlags BufferUsageFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		VkBuffer Buffer = Create_VkBuffer(TOTALDATA_SIZE, BufferUsageFlag);
		VkDeviceSize offset = 0, RequiredSize = 0;
		if (Suballocate_Buffer(Buffer, MemoryType, offset, RequiredSize, false) != TAPI_SUCCESS) {
			LOG_ERROR_TAPI("There is no memory left in specified memory region, please try again later!");
			return TAPI_FAIL;
		}
		vkDestroyBuffer(VKGPU->Logical_Device, Buffer, nullptr);

		VKMesh->Block.Offset = offset;
		VKMesh->Block.Type = MemoryType;
		VKMesh->Layout = Layout;
		VKMesh->VERTEX_COUNT = VertexCount;
		MESHBUFFERs.push_back(GFX->JobSys->GetThisThreadIndex(), VKMesh);
		VertexBufferHandle = VKMesh;
		return TAPI_SUCCESS;
	}
	TAPIResult GPU_ContentManager::Upload_VertexBuffer(GFX_API::GFXHandle BufferHandle, const void* InputData, unsigned int DataSize, unsigned int TargetOffset) {
		VK_VertexBuffer* MESH = GFXHandleConverter(VK_VertexBuffer*, BufferHandle);
		if (!(MESH->Block.Type == GFX_API::SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE || MESH->Block.Type == GFX_API::SUBALLOCATEBUFFERTYPEs::FASTHOSTVISIBLE)) {
			LOG_ERROR_TAPI("You can upload a vertex buffer to a buffer that's in either HOSTVISIBLE or FASTHOSTVISIBLE memory region!");
			return TAPI_FAIL;
		}

		LOG_NOTCODED_TAPI("Upload_VertexBuffer() isn't coded yet!", true);
		return TAPI_NOTCODED;
	}
	//When you call this function, Draw Calls that uses this ID may draw another Mesh or crash
	//Also if you have any Point Buffer that uses first vertex attribute of that Mesh Buffer, it may crash or draw any other buffer
	void GPU_ContentManager::Unload_VertexBuffer(GFX_API::GFXHandle MeshBuffer_ID) {
		LOG_NOTCODED_TAPI("VK::Unload_MeshBuffer isn't coded!", true);
	}



	TAPIResult GPU_ContentManager::Create_IndexBuffer(unsigned int DataSize, GFX_API::SUBALLOCATEBUFFERTYPEs MemoryType, GFX_API::GFXHandle& IndexBufferHandle) {
		LOG_NOTCODED_TAPI("Create_IndexBuffer() and nothing IndexBuffer related isn't coded yet!", true);
		return TAPI_NOTCODED;
	}
	TAPIResult GPU_ContentManager::Upload_IndexBuffer(GFX_API::GFXHandle BufferHandle, const void* InputData, unsigned int DataSize, unsigned int TargetOffset) {
		LOG_NOTCODED_TAPI("Upload_IndexBuffer() isn't coded yet!", true);
		return TAPI_NOTCODED;
	}
	void GPU_ContentManager::Unload_IndexBuffer(GFX_API::GFXHandle BufferHandle) {
		LOG_NOTCODED_TAPI("Create_IndexBuffer() and nothing IndexBuffer related isn't coded yet!", true);
	}


	TAPIResult GPU_ContentManager::Create_Texture(const GFX_API::Texture_Resource& TEXTURE_ASSET, GFX_API::SUBALLOCATEBUFFERTYPEs MemoryType, GFX_API::GFXHandle& TextureHandle) {
		LOG_NOTCODED_TAPI("GFXContentManager->Create_Texture() should support mipmaps!", false);
		VK_Texture* TEXTURE = new VK_Texture;
		TEXTURE->CHANNELs = TEXTURE_ASSET.Properties.CHANNEL_TYPE;
		TEXTURE->HEIGHT = TEXTURE_ASSET.HEIGHT;
		TEXTURE->WIDTH = TEXTURE_ASSET.WIDTH;
		TEXTURE->DATA_SIZE = TEXTURE_ASSET.WIDTH * TEXTURE_ASSET.HEIGHT * GFX_API::GetByteSizeOf_TextureChannels(TEXTURE_ASSET.Properties.CHANNEL_TYPE);
		TEXTURE->USAGE = TEXTURE_ASSET.USAGE;
		TEXTURE->Block.Type = MemoryType;

		//Create VkImage
		{
			VkImageCreateInfo im_ci = {};
			im_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			im_ci.arrayLayers = 1;
			im_ci.extent.width = TEXTURE->WIDTH;
			im_ci.extent.height = TEXTURE->HEIGHT;
			im_ci.extent.depth = 1;
			im_ci.flags = 0;
			im_ci.format = Find_VkFormat_byTEXTURECHANNELs(TEXTURE->CHANNELs);
			im_ci.imageType = VK_IMAGE_TYPE_2D;
			im_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			im_ci.mipLevels = 1;
			im_ci.pNext = nullptr;
			if (VKGPU->QUEUEs.size() > 1) {
				im_ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
			}
			else {
				im_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			}
			im_ci.pQueueFamilyIndices = VKGPU->AllQueueFamilies;
			im_ci.queueFamilyIndexCount = VKGPU->QUEUEs.size();
			im_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
			im_ci.usage = Find_VKImageUsage_forVKTexture(*TEXTURE);
			im_ci.samples = VK_SAMPLE_COUNT_1_BIT;

			if (vkCreateImage(VKGPU->Logical_Device, &im_ci, nullptr, &TEXTURE->Image) != VK_SUCCESS) {
				LOG_ERROR_TAPI("GFXContentManager->Create_Texture() has failed in vkCreateImage()!");
				delete TEXTURE;
				return TAPI_FAIL;
			}

			if (Suballocate_Image(*TEXTURE) != TAPI_SUCCESS) {
				LOG_ERROR_TAPI("Suballocation of the texture has failed! Please re-create later.");
				delete TEXTURE;
				return TAPI_FAIL;
			}
		}

		//Create Image View
		{
			VkImageViewCreateInfo ci = {};
			ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ci.flags = 0;
			ci.pNext = nullptr;

			ci.image = TEXTURE->Image;
			ci.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
			ci.format = Find_VkFormat_byTEXTURECHANNELs(TEXTURE->CHANNELs);
			ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ci.subresourceRange.baseArrayLayer = 0;
			ci.subresourceRange.layerCount = 1;
			ci.subresourceRange.baseMipLevel = 0;
			ci.subresourceRange.levelCount = 1;

			if (vkCreateImageView(VKGPU->Logical_Device, &ci, nullptr, &TEXTURE->ImageView) != VK_SUCCESS) {
				LOG_ERROR_TAPI("GFXContentManager->Upload_Texture() has failed in vkCreateImageView()!");
				return TAPI_FAIL;
			}
		}
		
		TEXTUREs.push_back(GFX->JobSys->GetThisThreadIndex(), TEXTURE);
		TextureHandle = TEXTURE;
		return TAPI_SUCCESS;
	}
	TAPIResult GPU_ContentManager::Upload_Texture(GFX_API::GFXHandle TextureHandle, const void* DATA, unsigned int DATA_SIZE, unsigned int TARGETOFFSET) {
		LOG_NOTCODED_TAPI("GFXContentManager->Upload_Texture(): Uploading the data isn't coded yet!", true);
		return TAPI_NOTCODED;
	}
	void GPU_ContentManager::Unload_Texture(GFX_API::GFXHandle ASSET_ID) {
		LOG_NOTCODED_TAPI("GFXContentManager->Unload_Texture() isn't coded!", true);
	}


	TAPIResult GPU_ContentManager::Create_GlobalBuffer(const char* BUFFER_NAME, unsigned int DATA_SIZE, unsigned int BINDINDEX, bool isUniform, 
		GFX_API::SHADERSTAGEs_FLAG AccessableStages, GFX_API::SUBALLOCATEBUFFERTYPEs MemoryType, GFX_API::GFXHandle& GlobalBufferHandle) {
		if (VKRENDERER->Is_ConstructingRenderGraph() || VKRENDERER->FrameGraphs->BranchCount) {
			LOG_ERROR_TAPI("GFX API don't support run-time Global Buffer addition for now because Vulkan needs to recreate PipelineLayouts (so all PSOs)! Please create your global buffers before render graph construction.");
			return TAPI_WRONGTIMING;
		}
		
		VkBuffer obj;
		if (isUniform) {
			obj = Create_VkBuffer(DATA_SIZE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		}
		else {
			obj = Create_VkBuffer(DATA_SIZE, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		}
		
		VkDeviceSize offset = 0, RequiredSize = 0;
		if (Suballocate_Buffer(obj, MemoryType, offset, RequiredSize, false) != TAPI_SUCCESS) {
			LOG_ERROR_TAPI("Create_GlobalBuffer has failed at suballocation!");
			return TAPI_FAIL;
		}
		
		VK_GlobalBuffer* GB = new VK_GlobalBuffer;
		GB->ACCESSED_STAGEs = AccessableStages;
		GB->BINDINGPOINT = BINDINDEX;
		GB->Block.Offset = offset;
		GB->Block.Type = MemoryType;
		GB->DATA_SIZE = RequiredSize;
		GB->isUniform = isUniform;

		GlobalBufferHandle = GB;
		GLOBALBUFFERs.push_back(GFX->JobSys->GetThisThreadIndex(), GB);
		return TAPI_SUCCESS;
	}
	TAPIResult GPU_ContentManager::Upload_GlobalBuffer(GFX_API::GFXHandle BufferHandle, const void* DATA, unsigned int DATA_SIZE, unsigned int TARGETOFFSET) {
		LOG_NOTCODED_TAPI("Upload_GlobalBuffer() isn't coded yet!", true);
		return TAPI_NOTCODED;
	}
	void GPU_ContentManager::Unload_GlobalBuffer(GFX_API::GFXHandle BUFFER_ID) {
		LOG_NOTCODED_TAPI("GFXContentManager->Unload_GlobalBuffer() isn't coded!", true);
	}


	TAPIResult GPU_ContentManager::Compile_ShaderSource(const GFX_API::ShaderSource_Resource* SHADER, GFX_API::GFXHandle& ShaderSourceHandle) {
		//Create Vertex Shader Module
		VkShaderModuleCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ci.flags = 0;
		ci.pNext = nullptr;
		ci.pCode = reinterpret_cast<const uint32_t*>(SHADER->SOURCE_DATA);
		ci.codeSize = static_cast<size_t>(SHADER->DATA_SIZE);

		VkShaderModule Module;
		if (vkCreateShaderModule(((GPU*)VKGPU)->Logical_Device, &ci, 0, &Module) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Shader Source is failed at creation!");
			return TAPI_FAIL;
		}
		
		VK_ShaderSource* SHADERSOURCE = new VK_ShaderSource;
		SHADERSOURCE->Module = Module;
		SHADERSOURCEs.push_back(GFX->JobSys->GetThisThreadIndex(), SHADERSOURCE);
		LOG_STATUS_TAPI("Vertex Shader Module is successfully created!");
		ShaderSourceHandle = SHADERSOURCE;
		return TAPI_SUCCESS;
	}
	void GPU_ContentManager::Delete_ShaderSource(GFX_API::GFXHandle ASSET_ID) {
		LOG_NOTCODED_TAPI("VK::Unload_GlobalBuffer isn't coded!", true);
	}
	TAPIResult GPU_ContentManager::Compile_ComputeShader(GFX_API::ComputeShader_Resource* SHADER, GFX_API::GFXHandle* Handles, unsigned int Count) {
		LOG_NOTCODED_TAPI("VK::Compile_ComputeShader isn't coded!", true);
		return TAPI_NOTCODED;
	}
	void GPU_ContentManager::Delete_ComputeShader(GFX_API::GFXHandle ASSET_ID){
		LOG_NOTCODED_TAPI("VK::Delete_ComputeShader isn't coded!", true);
	}
	TAPIResult GPU_ContentManager::Link_MaterialType(const GFX_API::Material_Type& MATTYPE_ASSET, GFX_API::GFXHandle& MaterialHandle) {
		if (VKRENDERER->Is_ConstructingRenderGraph()) {
			LOG_ERROR_TAPI("You can't link a Material Type while recording RenderGraph!");
			return TAPI_WRONGTIMING;
		}
		VK_VertexAttribLayout* LAYOUT = nullptr;
		LAYOUT = GFXHandleConverter(VK_VertexAttribLayout*, MATTYPE_ASSET.ATTRIBUTELAYOUT_ID);
		if (!LAYOUT) {
			LOG_ERROR_TAPI("Link_MaterialType() has failed because Material Type has invalid Vertex Attribute Layout!");
			return TAPI_INVALIDARGUMENT;
		}
		VK_SubDrawPass* Subpass = GFXHandleConverter(VK_SubDrawPass*, MATTYPE_ASSET.SubDrawPass_ID);
		VK_DrawPass* MainPass = nullptr;
		MainPass = GFXHandleConverter(VK_DrawPass*, Subpass->DrawPass);

		//Subpass attachment should happen here!
		VK_GraphicsPipeline* VKPipeline = new VK_GraphicsPipeline;

		VkPipelineShaderStageCreateInfo Vertex_ShaderStage = {};
		VkPipelineShaderStageCreateInfo Fragment_ShaderStage = {};
		{
			Vertex_ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			Vertex_ShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
			VkShaderModule* VS_Module = &GFXHandleConverter(VK_ShaderSource*, MATTYPE_ASSET.VERTEXSOURCE_ID)->Module;
			Vertex_ShaderStage.module = *VS_Module;
			Vertex_ShaderStage.pName = "main";

			Fragment_ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			Fragment_ShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			VkShaderModule* FS_Module = &GFXHandleConverter(VK_ShaderSource*, MATTYPE_ASSET.FRAGMENTSOURCE_ID)->Module;
			Fragment_ShaderStage.module = *FS_Module;
			Fragment_ShaderStage.pName = "main";
		}
		VkPipelineShaderStageCreateInfo STAGEs[2] = { Vertex_ShaderStage, Fragment_ShaderStage };


		GPU* Vulkan_GPU = (GPU*)VKGPU;

		VkPipelineVertexInputStateCreateInfo VertexInputState_ci = {};
		{
			VertexInputState_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			VertexInputState_ci.pVertexBindingDescriptions = &LAYOUT->BindingDesc;
			VertexInputState_ci.vertexBindingDescriptionCount = 1;
			VertexInputState_ci.pVertexAttributeDescriptions = LAYOUT->AttribDescs;
			VertexInputState_ci.vertexAttributeDescriptionCount = LAYOUT->AttribDesc_Count;
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

		vector<VkPipelineColorBlendAttachmentState> States;
		VkPipelineColorBlendAttachmentState Attachment_ColorBlendState = {};
		VkPipelineColorBlendStateCreateInfo Pipeline_ColorBlendState = {};
		{
			//Non-blend settings
			Attachment_ColorBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			Attachment_ColorBlendState.blendEnable = VK_FALSE;
			Attachment_ColorBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			Attachment_ColorBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
			Attachment_ColorBlendState.colorBlendOp = VK_BLEND_OP_ADD; // Optional
			Attachment_ColorBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			Attachment_ColorBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
			Attachment_ColorBlendState.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

			States.resize(MainPass->SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT, Attachment_ColorBlendState);

			Pipeline_ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			Pipeline_ColorBlendState.logicOpEnable = VK_FALSE;
			Pipeline_ColorBlendState.attachmentCount = States.size();
			Pipeline_ColorBlendState.pAttachments = States.data();
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
			//General DescriptorSet Layout Creation
			{
				vector<VkDescriptorSetLayoutBinding> bindings;
				vector<VK_DescImage> DescImages, DescSamplers;
				vector<VK_DescBuffer> DescUBuffers, DescSBuffers;
				for (unsigned int i = 0; i < MATTYPE_ASSET.MATERIALTYPEDATA.size(); i++) {
					const GFX_API::MaterialDataDescriptor& gfxdesc = MATTYPE_ASSET.MATERIALTYPEDATA[i];
					if (!(gfxdesc.TYPE == GFX_API::MATERIALDATA_TYPE::CONSTIMAGE_G ||
						gfxdesc.TYPE == GFX_API::MATERIALDATA_TYPE::CONSTSAMPLER_G ||
						gfxdesc.TYPE == GFX_API::MATERIALDATA_TYPE::CONSTSBUFFER_G ||
						gfxdesc.TYPE == GFX_API::MATERIALDATA_TYPE::CONSTUBUFFER_G)) {
						continue;
					}

					unsigned int BP = gfxdesc.BINDINGPOINT;
					for (unsigned int bpsearchindex = 0; bpsearchindex < bindings.size(); bpsearchindex++) {
						if (BP == bindings[bpsearchindex].binding) {
							LOG_ERROR_TAPI("Link_MaterialType() has failed because there are colliding binding points!");
							return TAPI_FAIL;
						}
					}

					VkDescriptorSetLayoutBinding bn = {};
					bn.stageFlags = Find_VkShaderStages(gfxdesc.SHADERSTAGEs);
					bn.pImmutableSamplers = VK_NULL_HANDLE;
					bn.descriptorType = Find_VkDescType_byMATDATATYPE(gfxdesc.TYPE);
					bn.descriptorCount = 1;		//I don't support array descriptors for now!
					bn.binding = BP;
					bindings.push_back(bn);

					switch (gfxdesc.TYPE) {
					case GFX_API::MATERIALDATA_TYPE::CONSTIMAGE_G:
					{
						VK_DescImage descimage;
						descimage.BindingIndex = BP;
						descimage.info.imageView = VK_NULL_HANDLE;
						descimage.info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						descimage.info.sampler = VK_NULL_HANDLE;
						DescImages.push_back(descimage);
					}
					break;
					case GFX_API::MATERIALDATA_TYPE::CONSTSAMPLER_G:
					{
						VK_DescImage descimage;
						descimage.BindingIndex = BP;
						descimage.info.imageView = VK_NULL_HANDLE;
						descimage.info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						descimage.info.sampler = VK_NULL_HANDLE;
						DescSamplers.push_back(descimage);
					}
						break;
					case GFX_API::MATERIALDATA_TYPE::CONSTUBUFFER_G:
					{
						VK_DescBuffer descbuffer;
						descbuffer.BindingIndex = BP;
						descbuffer.Info.range = gfxdesc.DATA_SIZE;
						descbuffer.IsUpdated.store(0);
						descbuffer.Info.buffer = VK_NULL_HANDLE;
						descbuffer.Info.offset = 0;
						DescUBuffers.push_back(descbuffer);
					}
						break;
					case GFX_API::MATERIALDATA_TYPE::CONSTSBUFFER_G:
					{
						VK_DescBuffer descbuffer;
						descbuffer.BindingIndex = BP;
						descbuffer.Info.range = gfxdesc.DATA_SIZE;
						descbuffer.IsUpdated.store(0);
						descbuffer.Info.buffer = VK_NULL_HANDLE;
						descbuffer.Info.offset = 0;
						DescSBuffers.push_back(descbuffer);
					}
						break;
					}
				}

				VKPipeline->General_DescSet.DescImagesCount = DescImages.size();
				VKPipeline->General_DescSet.DescSamplersCount = DescSamplers.size();
				VKPipeline->General_DescSet.DescSBuffersCount = DescSBuffers.size();
				VKPipeline->General_DescSet.DescUBuffersCount = DescUBuffers.size();
				if (DescImages.size()) {
					VKPipeline->General_DescSet.DescImages = new VK_DescImage[DescImages.size()];
					memcpy(VKPipeline->General_DescSet.DescImages, DescImages.data(), DescImages.size() * sizeof(VK_DescImage));
				}
				if (DescSamplers.size()) {
					VKPipeline->General_DescSet.DescSamplers = new VK_DescImage[DescSamplers.size()];
					memcpy(VKPipeline->General_DescSet.DescSamplers, DescSamplers.data(), DescSamplers.size() * sizeof(VK_DescImage));
				}
				if (DescUBuffers.size()) {
					VKPipeline->General_DescSet.DescUBuffers = new VK_DescBuffer[DescUBuffers.size()];
					memcpy(VKPipeline->General_DescSet.DescUBuffers, DescUBuffers.data(), DescUBuffers.size() * sizeof(VK_DescBuffer));
				}
				if (DescSBuffers.size()) {
					VKPipeline->General_DescSet.DescSBuffers = new VK_DescBuffer[DescSBuffers.size()];
					memcpy(VKPipeline->General_DescSet.DescSBuffers, DescSBuffers.data(), DescSBuffers.size() * sizeof(VK_DescBuffer));
				}
				VKPipeline->General_DescSet.ShouldRecreate.store(0);

				
				VkDescriptorSetLayoutCreateInfo ci = {};
				ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				ci.pNext = nullptr;
				ci.flags = 0;
				ci.bindingCount = bindings.size();
				ci.pBindings = bindings.data();

				if (vkCreateDescriptorSetLayout(VKGPU->Logical_Device, &ci, nullptr, &VKPipeline->General_DescSet.Layout) != VK_SUCCESS) {
					LOG_ERROR_TAPI("Link_MaterialType() has failed at General DescriptorSetLayout Creation vkCreateDescriptorSetLayout()");
					return TAPI_FAIL;
				}
			}

			//Instance DescriptorSet Layout Creation
			{
				vector<VkDescriptorSetLayoutBinding> bindings;
				vector<VK_DescImage> DescImages, DescSamplers;
				vector<VK_DescBuffer> DescUBuffers, DescSBuffers;
				for (unsigned int i = 0; i < MATTYPE_ASSET.MATERIALTYPEDATA.size(); i++) {
					const GFX_API::MaterialDataDescriptor& gfxdesc = MATTYPE_ASSET.MATERIALTYPEDATA[i];
					if (!gfxdesc.BINDINGPOINT) {
						continue;
					}

					if (gfxdesc.TYPE != GFX_API::MATERIALDATA_TYPE::CONSTIMAGE_PI ||
						gfxdesc.TYPE != GFX_API::MATERIALDATA_TYPE::CONSTSAMPLER_PI ||
						gfxdesc.TYPE != GFX_API::MATERIALDATA_TYPE::CONSTSBUFFER_PI ||
						gfxdesc.TYPE != GFX_API::MATERIALDATA_TYPE::CONSTUBUFFER_PI) {
						continue;
					}
					unsigned int BP = *(unsigned int*)gfxdesc.BINDINGPOINT;
					for (unsigned int bpsearchindex = 0; bpsearchindex < bindings.size(); bpsearchindex++) {
						if (BP == bindings[bpsearchindex].binding) {
							LOG_ERROR_TAPI("Link_MaterialType() has failed because there are colliding binding points!");
							return TAPI_FAIL;
						}
					}
					VkDescriptorSetLayoutBinding bn = {};
					bn.stageFlags = Find_VkShaderStages(gfxdesc.SHADERSTAGEs);
					bn.pImmutableSamplers = VK_NULL_HANDLE;
					bn.descriptorType = Find_VkDescType_byMATDATATYPE(gfxdesc.TYPE);
					bn.descriptorCount = 1;		//I don't support array descriptors for now!
					bn.binding = BP;
					bindings.push_back(bn);
					switch (gfxdesc.TYPE) {
					case GFX_API::MATERIALDATA_TYPE::CONSTIMAGE_PI:
					{
						VK_DescImage descimage;
						descimage.BindingIndex = BP;
						descimage.info.imageView = VK_NULL_HANDLE;
						descimage.info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						descimage.info.sampler = VK_NULL_HANDLE;
						DescImages.push_back(descimage);
					}
						break;
					case GFX_API::MATERIALDATA_TYPE::CONSTSAMPLER_PI:
					{
						VK_DescImage descimage;
						descimage.BindingIndex = BP;
						descimage.info.imageView = VK_NULL_HANDLE;
						descimage.info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						descimage.info.sampler = VK_NULL_HANDLE;
						DescSamplers.push_back(descimage);
					}
						break;
					case GFX_API::MATERIALDATA_TYPE::CONSTUBUFFER_PI:
					{
						VK_DescBuffer descbuffer;
						descbuffer.BindingIndex = BP;
						descbuffer.Info.range = gfxdesc.DATA_SIZE;
						descbuffer.IsUpdated.store(0);
						descbuffer.Info.buffer = VK_NULL_HANDLE;
						descbuffer.Info.offset = 0;
						DescUBuffers.push_back(descbuffer);
					}
						break;
					case GFX_API::MATERIALDATA_TYPE::CONSTSBUFFER_PI:
					{
						VK_DescBuffer descbuffer;
						descbuffer.BindingIndex = BP;
						descbuffer.Info.range = gfxdesc.DATA_SIZE;
						descbuffer.IsUpdated.store(0);
						descbuffer.Info.buffer = VK_NULL_HANDLE;
						descbuffer.Info.offset = 0;
						DescSBuffers.push_back(descbuffer);
					}
						break;
					}
				}


				VKPipeline->Instance_DescSet.DescUBuffersCount = DescUBuffers.size();
				VKPipeline->Instance_DescSet.DescSBuffersCount = DescSBuffers.size();
				VKPipeline->Instance_DescSet.DescImagesCount = DescImages.size();
				VKPipeline->Instance_DescSet.DescSamplersCount = DescSamplers.size();
				if (DescSamplers.size()) {
					VKPipeline->Instance_DescSet.DescSamplers = new VK_DescImage[DescSamplers.size()];
					memcpy(VKPipeline->Instance_DescSet.DescSamplers, DescSamplers.data(), DescSamplers.size() * sizeof(VK_DescImage));
				}
				if (DescImages.size()) {
					VKPipeline->Instance_DescSet.DescImages = new VK_DescImage[DescImages.size()];
					memcpy(VKPipeline->Instance_DescSet.DescImages, DescImages.data(), DescImages.size() * sizeof(VK_DescImage));
				}
				if (DescSBuffers.size()) {
					VKPipeline->Instance_DescSet.DescSBuffers = new VK_DescBuffer[DescSBuffers.size()];
					memcpy(VKPipeline->Instance_DescSet.DescSBuffers, DescSBuffers.data(), DescSBuffers.size() * sizeof(VK_DescBuffer));
				}
				if (DescUBuffers.size()) {
					VKPipeline->Instance_DescSet.DescUBuffers = new VK_DescBuffer[DescUBuffers.size()];
					memcpy(VKPipeline->Instance_DescSet.DescUBuffers, DescUBuffers.data(), DescUBuffers.size() * sizeof(VK_DescBuffer));
				}


				if (bindings.size()) {
					VkDescriptorSetLayoutCreateInfo ci = {};
					ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					ci.pNext = nullptr;
					ci.flags = 0;
					ci.bindingCount = bindings.size();
					ci.pBindings = bindings.data();

					if (vkCreateDescriptorSetLayout(VKGPU->Logical_Device, &ci, nullptr, &VKPipeline->Instance_DescSet.Layout) != VK_SUCCESS) {
						LOG_ERROR_TAPI("Link_MaterialType() has failed at Instance DesciptorSetLayout Creation vkCreateDescriptorSetLayout()");
						return TAPI_FAIL;
					}
				}
			}
			
			//General DescriptorSet Creation
			if (!Create_DescSet(&VKPipeline->General_DescSet)) {
				LOG_ERROR_TAPI("Descriptor pool is full, that means you should expand its size!");
				return TAPI_FAIL;
			}

			//Pipeline Layout Creation
			{
				VkPipelineLayoutCreateInfo pl_ci = {};
				pl_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pl_ci.pNext = nullptr;
				pl_ci.flags = 0;
				VkDescriptorSetLayout descsetlays[3] = { GlobalBuffers_DescSetLayout, VKPipeline->General_DescSet.Layout, VK_NULL_HANDLE };
				if (VKPipeline->Instance_DescSet.Layout != VK_NULL_HANDLE) {
					descsetlays[2] = VKPipeline->Instance_DescSet.Layout;
					pl_ci.setLayoutCount = 3;
				}
				else {
					pl_ci.setLayoutCount = 2;
				}
				pl_ci.pSetLayouts = descsetlays;
				//Don't support for now!
				pl_ci.pushConstantRangeCount = 0;
				pl_ci.pPushConstantRanges = nullptr;

				if (vkCreatePipelineLayout(Vulkan_GPU->Logical_Device, &pl_ci, nullptr, &VKPipeline->PipelineLayout) != VK_SUCCESS) {
					LOG_ERROR_TAPI("Link_MaterialType() failed at vkCreatePipelineLayout()!");
					return TAPI_FAIL;
				}
			}
		}


		VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = {};
		{
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
			GraphicsPipelineCreateInfo.layout = VKPipeline->PipelineLayout;
			GraphicsPipelineCreateInfo.renderPass = MainPass->RenderPassObject;
			GraphicsPipelineCreateInfo.subpass = Subpass->Binding_Index;
			GraphicsPipelineCreateInfo.stageCount = 2;
			GraphicsPipelineCreateInfo.pStages = STAGEs;
			GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;		//Optional
			GraphicsPipelineCreateInfo.basePipelineIndex = -1;					//Optional
			GraphicsPipelineCreateInfo.flags = 0;
			GraphicsPipelineCreateInfo.pNext = nullptr;
			if (vkCreateGraphicsPipelines(Vulkan_GPU->Logical_Device, nullptr, 1, &GraphicsPipelineCreateInfo, nullptr, &VKPipeline->PipelineObject) != VK_SUCCESS) {
				delete VKPipeline;
				delete STAGEs;
				LOG_ERROR_TAPI("vkCreateGraphicsPipelines has failed!");
				return TAPI_FAIL;
			}
		}

		VKPipeline->GFX_Subpass = Subpass;
		SHADERPROGRAMs.push_back(GFX->JobSys->GetThisThreadIndex(), VKPipeline);


		LOG_STATUS_TAPI("Finished creating Graphics Pipeline");
		MaterialHandle = VKPipeline;
		return TAPI_SUCCESS;
	}
	void GPU_ContentManager::Delete_MaterialType(GFX_API::GFXHandle Asset_ID){
		LOG_NOTCODED_TAPI("VK::Unload_GlobalBuffer isn't coded!", true);
	}
	TAPIResult GPU_ContentManager::Create_MaterialInst(GFX_API::GFXHandle MaterialType, GFX_API::GFXHandle& MaterialInstHandle) {
		VK_GraphicsPipeline* VKPSO = GFXHandleConverter(VK_GraphicsPipeline*, MaterialType);
		if (!VKPSO) {
			LOG_ERROR_TAPI("Create_MaterialInst() has failed because Material Type isn't found!");
			return TAPI_INVALIDARGUMENT;
		}

		//Descriptor Set Creation
		VK_PipelineInstance* VKPInstance = new VK_PipelineInstance;

		VKPInstance->DescSet.Layout = VKPSO->Instance_DescSet.Layout;
		VKPInstance->DescSet.ShouldRecreate.store(0);
		VKPInstance->DescSet.DescImagesCount = VKPSO->Instance_DescSet.DescImagesCount;
		VKPInstance->DescSet.DescSamplersCount = VKPSO->Instance_DescSet.DescSamplersCount;
		VKPInstance->DescSet.DescSBuffersCount = VKPSO->Instance_DescSet.DescSBuffersCount;
		VKPInstance->DescSet.DescUBuffersCount = VKPSO->Instance_DescSet.DescUBuffersCount;
		if (VKPInstance->DescSet.DescImagesCount) {
			VKPInstance->DescSet.DescImages = new VK_DescImage[VKPInstance->DescSet.DescImagesCount];
			memcpy(VKPInstance->DescSet.DescUBuffers, VKPSO->Instance_DescSet.DescUBuffers, VKPInstance->DescSet.DescUBuffersCount * sizeof(VK_DescBuffer));
		}
		if (VKPInstance->DescSet.DescSamplersCount) {
			VKPInstance->DescSet.DescSamplers = new VK_DescImage[VKPInstance->DescSet.DescSamplersCount];
			memcpy(VKPInstance->DescSet.DescSBuffers, VKPSO->Instance_DescSet.DescSBuffers, VKPInstance->DescSet.DescSBuffersCount * sizeof(VK_DescBuffer));
		}
		if (VKPInstance->DescSet.DescSBuffersCount) {
			VKPInstance->DescSet.DescSBuffers = new VK_DescBuffer[VKPInstance->DescSet.DescSBuffersCount];
			memcpy(VKPInstance->DescSet.DescSamplers, VKPSO->Instance_DescSet.DescSamplers, VKPInstance->DescSet.DescSamplersCount * sizeof(VK_DescImage));
		}
		if (VKPInstance->DescSet.DescUBuffersCount) {
			VKPInstance->DescSet.DescUBuffers = new VK_DescBuffer[VKPInstance->DescSet.DescUBuffersCount];
			memcpy(VKPInstance->DescSet.DescImages, VKPSO->Instance_DescSet.DescImages, VKPInstance->DescSet.DescImagesCount * sizeof(VK_DescImage));
		}
		if (VKPInstance->DescSet.DescImagesCount ||
			VKPInstance->DescSet.DescSamplersCount ||
			VKPInstance->DescSet.DescSBuffersCount ||
			VKPInstance->DescSet.DescUBuffersCount) {
			Create_DescSet(&VKPInstance->DescSet);
		}


		VKPInstance->PROGRAM = VKPSO;
		SHADERPINSTANCEs.push_back(GFX->JobSys->GetThisThreadIndex(), VKPInstance);
		MaterialInstHandle = VKPInstance;
		return TAPI_SUCCESS;
	}
	void GPU_ContentManager::Delete_MaterialInst(GFX_API::GFXHandle Asset_ID) {
		LOG_CRASHING_TAPI("Delete_MaterialInst() isn't coded yet!");
	}
	void FindBufferOBJ_byBufType(const GFX_API::GFXHandle Handle, GFX_API::BUFFER_TYPE TYPE, VkBuffer& TargetBuffer, VkDeviceSize& TargetOffset);
	void GPU_ContentManager::SetMaterial_UniformBuffer(GFX_API::GFXHandle MaterialType_orInstance, bool isMaterialType, bool isUsedRecently, unsigned int BINDINDEX, GFX_API::GFXHandle TargetBufferHandle,
		GFX_API::BUFFER_TYPE BufferType, unsigned int TargetOffset) {
		VK_DescBuffer* UB = nullptr;
		VK_DescSet* Set = nullptr;
		unsigned int UBIndex = 0;
		if (isMaterialType) {
			VK_GraphicsPipeline* PSO = GFXHandleConverter(VK_GraphicsPipeline*, MaterialType_orInstance);
			for (unsigned int Index = 0; Index < PSO->General_DescSet.DescUBuffersCount; Index++) {
				VK_DescBuffer& UniformBuffer = PSO->General_DescSet.DescUBuffers[Index];
				if (UniformBuffer.BindingIndex != BINDINDEX) {
					continue;
				}
				UB = &PSO->General_DescSet.DescUBuffers[Index];
				Set = &PSO->General_DescSet;
				UBIndex = Index;
				break;
			}
		}
		else {
			VK_PipelineInstance* PSO = GFXHandleConverter(VK_PipelineInstance*, MaterialType_orInstance);
			for (unsigned int Index = 0; Index < PSO->DescSet.DescUBuffersCount; Index++) {
				VK_DescBuffer& UniformBuffer = PSO->DescSet.DescUBuffers[Index];
				if (UniformBuffer.BindingIndex != BINDINDEX) {
					continue;
				}
				UB = &PSO->DescSet.DescUBuffers[Index];
				Set = &PSO->DescSet;
				UBIndex = Index;
				break;
			}
		}
		if (!UB) {
			LOG_ERROR_TAPI("Specified uniform buffer isn't found!");
			return;
		}


		bool x = 0, y = 1;
		if (!UB->IsUpdated.compare_exchange_strong(x, y)) {
			LOG_ERROR_TAPI("You already set the uniform buffer, you can't change it at the same frame!");
			return;
		}
		VkDeviceSize FinalOffset = static_cast<VkDeviceSize>(TargetOffset);
		switch (BufferType) {
		case GFX_API::BUFFER_TYPE::STAGING:
		{
			MemoryBlock* StagingBuf = GFXHandleConverter(MemoryBlock*, TargetBufferHandle);
			FindBufferOBJ_byBufType(TargetBufferHandle, BufferType, UB->Info.buffer, FinalOffset);
		}
		break;
		default:
			LOG_NOTCODED_TAPI("SetMaterial_UniformBuffer() doesn't support this type of target buffers right now!", true);
			return;
		}
		UB->Info.offset = FinalOffset;
		UB->IsUpdated.store(1);
		VK_DescSetUpdateCall call;
		call.Set = Set;
		call.ArrayIndex = UBIndex;
		call.Type = DescType::UBUFFER;
		if (isUsedRecently) {
			call.Set->ShouldRecreate.store(1);
			DescSets_toCreateUpdate.push_back(GFX->JobSys->GetThisThreadIndex(), call);
		}
		else {
			DescSets_toJustUpdate.push_back(GFX->JobSys->GetThisThreadIndex(), call);
		}
	}

	TAPIResult GPU_ContentManager::Create_RTSlotset(const vector<GFX_API::RTSLOT_Description>& Descriptions, GFX_API::GFXHandle& RTSlotSetHandle) {
		if (!VKRENDERER->Is_ConstructingRenderGraph()) {
			LOG_ERROR_TAPI("GFXContentManager->Create_RTSlotSet() has failed because you can't create a RTSlotSet if you aren't constructing a RenderGraph!");
			return TAPI_FAIL;
		}

		for (unsigned int SlotIndex = 0; SlotIndex < Descriptions.size(); SlotIndex++) {
			const GFX_API::RTSLOT_Description& desc = Descriptions[SlotIndex];
			VK_Texture* FirstHandle = GFXHandleConverter(VK_Texture*, desc.TextureHandles[0]);
			VK_Texture* SecondHandle = GFXHandleConverter(VK_Texture*, desc.TextureHandles[1]);
			if ((FirstHandle->CHANNELs != SecondHandle->CHANNELs) ||
				(FirstHandle->WIDTH != SecondHandle->WIDTH) ||
				(FirstHandle->HEIGHT != SecondHandle->HEIGHT)
				) {
				LOG_ERROR_TAPI("GFXContentManager->Create_RTSlotSet() has failed because one of the slots has texture handles that doesn't match channel type, width or height!");
				return TAPI_INVALIDARGUMENT;
			}
			if (!FirstHandle->USAGE.isRenderableTo || !SecondHandle->USAGE.isRenderableTo) {
				LOG_ERROR_TAPI("GFXContentManager->Create_RTSlotSet() has failed because one of the slots has a handle that doesn't use is_RenderableTo in its USAGEFLAG!");
				return TAPI_INVALIDARGUMENT;
			}
		}
		unsigned int DEPTHSLOT_VECTORINDEX = Descriptions.size();
		//Validate the list and find Depth Slot if there is any
		for (unsigned int SlotIndex = 0; SlotIndex < Descriptions.size(); SlotIndex++) {
			const GFX_API::RTSLOT_Description& desc = Descriptions[SlotIndex];
			for (unsigned int RTIndex = 0; RTIndex < 2; RTIndex++) {
				VK_Texture* RT = GFXHandleConverter(VK_Texture*, desc.TextureHandles[RTIndex]);
				if (!RT) {
					LOG_ERROR_TAPI("Create_RTSlotSet() has failed because intended RT isn't found!");
					return TAPI_INVALIDARGUMENT;
				}
				if (desc.OPTYPE == GFX_API::OPERATION_TYPE::UNUSED) {
					LOG_ERROR_TAPI("Create_RTSlotSet() has failed because you can't create a Base RT SlotSet that has unused attachment!");
					return TAPI_INVALIDARGUMENT;
				}
				if (RT->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8 || RT->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32) {
					if (DEPTHSLOT_VECTORINDEX != Descriptions.size()) {
						LOG_ERROR_TAPI("Create_RTSlotSet() has failed because you can't use two depth buffers at the same slotset (If you're trying to use D24S8 or D32 for color textures, that's not allowed!)");
						return TAPI_INVALIDARGUMENT;
					}
					DEPTHSLOT_VECTORINDEX = SlotIndex;
					continue;
				}
				if (desc.SLOTINDEX > Descriptions.size() - 1) {
					LOG_ERROR_TAPI("Create_RTSlotSet() has failed because you gave a overflowing SLOTINDEX to a RTSLOT!");
					return TAPI_INVALIDARGUMENT;
				}
			}
		}
		unsigned char COLORRT_COUNT = (DEPTHSLOT_VECTORINDEX != Descriptions.size()) ? Descriptions.size() - 1 : Descriptions.size();

		unsigned int i = 0;
		unsigned int j = 0;
		for (i = 0; i < COLORRT_COUNT; i++) {
			if (i == DEPTHSLOT_VECTORINDEX) {
				continue;
			}
			for (j = 1; i + j < COLORRT_COUNT; j++) {
				if (i + j == DEPTHSLOT_VECTORINDEX) {
					continue;
				}
				if (Descriptions[i].SLOTINDEX == Descriptions[i + j].SLOTINDEX) {
					LOG_ERROR_TAPI("Create_RTSlotSet() has failed because some SLOTINDEXes are same, but each SLOTINDEX should be unique and lower then COLOR SLOT COUNT!");
					return TAPI_INVALIDARGUMENT;
				}
			}
		}


		VK_RTSLOTSET* VKSLOTSET = new VK_RTSLOTSET;
		for (unsigned int SlotSetIndex = 0; SlotSetIndex < 2; SlotSetIndex++) {
			VK_RTSLOTs& PF_SLOTSET = VKSLOTSET->PERFRAME_SLOTSETs[SlotSetIndex];

			PF_SLOTSET.COLOR_SLOTs = new VK_COLORRTSLOT[COLORRT_COUNT];
			PF_SLOTSET.COLORSLOTs_COUNT = COLORRT_COUNT;
			if (DEPTHSLOT_VECTORINDEX != Descriptions.size()) {
				PF_SLOTSET.DEPTHSTENCIL_SLOT = new VK_DEPTHSTENCILSLOT;
				VK_DEPTHSTENCILSLOT* slot = PF_SLOTSET.DEPTHSTENCIL_SLOT;
				const GFX_API::RTSLOT_Description& DEPTHDESC = Descriptions[DEPTHSLOT_VECTORINDEX];
				slot->CLEAR_COLOR = vec2(DEPTHDESC.CLEAR_VALUE.x, DEPTHDESC.CLEAR_VALUE.y);
				slot->DEPTH_OPTYPE = DEPTHDESC.OPTYPE;
				slot->RT = GFXHandleConverter(VK_Texture*, DEPTHDESC.TextureHandles[SlotSetIndex]);
				slot->STENCIL_OPTYPE = GFX_API::OPERATION_TYPE::UNUSED;
				LOG_NOTCODED_TAPI("Create_RTSlotSet() sets STENCIL_OPTYPE as UNUSED hard-coded for now, fix it in future!", false);
			}
			for (unsigned int i = 0; i < COLORRT_COUNT; i++) {
				const GFX_API::RTSLOT_Description& desc = Descriptions[i];
				VK_Texture* RT = GFXHandleConverter(VK_Texture*, desc.TextureHandles[SlotSetIndex]);
				VK_COLORRTSLOT SLOT;
				SLOT.RT_OPERATIONTYPE = desc.OPTYPE;
				SLOT.LOADSTATE = desc.LOADOP;
				SLOT.RT = RT;
				SLOT.CLEAR_COLOR = desc.CLEAR_VALUE;
				PF_SLOTSET.COLOR_SLOTs[desc.SLOTINDEX] = SLOT;
			}
		}

		RT_SLOTSETs.push_back(GFX->JobSys->GetThisThreadIndex(), VKSLOTSET);
		RTSlotSetHandle = VKSLOTSET;
		return TAPI_SUCCESS;
	}
	TAPIResult GPU_ContentManager::Inherite_RTSlotSet(const vector<GFX_API::RTSLOTUSAGE_Description>& Descriptions, GFX_API::GFXHandle RTSlotSetHandle, GFX_API::GFXHandle& InheritedSlotSetHandle) {
		if (!RTSlotSetHandle) {
			LOG_ERROR_TAPI("Inherite_RTSlotSet() has failed because Handle is invalid!");
			return TAPI_INVALIDARGUMENT;
		}
		VK_RTSLOTSET* BaseSet = GFXHandleConverter(VK_RTSLOTSET*, RTSlotSetHandle);
		VK_IRTSLOTSET* InheritedSet = new VK_IRTSLOTSET;
		InheritedSet->BASESLOTSET = BaseSet;

		//Find Depth/Stencil Slots and count Color Slots
		bool DEPTH_FOUND = false;
		unsigned char COLORSLOT_COUNT = 0, DEPTHDESC_VECINDEX = 0;
		for (unsigned char i = 0; i < Descriptions.size(); i++) {
			const GFX_API::RTSLOTUSAGE_Description& DESC = Descriptions[i];
			if (DESC.IS_DEPTH) {
				if (DEPTH_FOUND) {
					LOG_ERROR_TAPI("Inherite_RTSlotSet() has failed because there are two depth buffers in the description, which is not supported!");
					delete InheritedSet;
					return TAPI_INVALIDARGUMENT;
				}
				DEPTH_FOUND = true;
				DEPTHDESC_VECINDEX = i;
				if (BaseSet->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::READ_ONLY &&
					(DESC.OPTYPE == GFX_API::OPERATION_TYPE::WRITE_ONLY || DESC.OPTYPE == GFX_API::OPERATION_TYPE::READ_AND_WRITE)
					) 
				{
					LOG_ERROR_TAPI("Inherite_RTSlotSet() has failed because you can't use a Read-Only DepthSlot with Write Access in a Inherited Set!");
					delete InheritedSet;
					return TAPI_INVALIDARGUMENT;
				}
				InheritedSet->DEPTH_OPTYPE = DESC.OPTYPE;
				InheritedSet->STENCIL_OPTYPE = GFX_API::OPERATION_TYPE::UNUSED;
			}
			else {
				COLORSLOT_COUNT++;
			}
		}
		if (!DEPTH_FOUND) {
			InheritedSet->DEPTH_OPTYPE = GFX_API::OPERATION_TYPE::UNUSED;
		}
		if (COLORSLOT_COUNT != BaseSet->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT) {
			LOG_ERROR_TAPI("Inherite_RTSlotSet() has failed because BaseSet's Color Slot count doesn't match given Descriptions's one!");
			delete InheritedSet;
			return TAPI_INVALIDARGUMENT;
		}

		//Check SlotIndexes of Color Slots
		for (unsigned int i = 0; i < COLORSLOT_COUNT; i++) {
			if (i == DEPTHDESC_VECINDEX) {
				continue;
			}
			for (unsigned int j = 1; i + j < COLORSLOT_COUNT; j++) {
				if (i + j == DEPTHDESC_VECINDEX) {
					continue;
				}
				if (Descriptions[i].SLOTINDEX >= COLORSLOT_COUNT) {
					LOG_ERROR_TAPI("Inherite_RTSlotSet() has failed because some ColorSlots have indexes that are more than or equal to ColorSlotCount! Each slot's index should be unique and lower than ColorSlotCount!");
					delete InheritedSet;
					return TAPI_INVALIDARGUMENT;
				}
				if (Descriptions[i].SLOTINDEX == Descriptions[i + j].SLOTINDEX) {
					LOG_ERROR_TAPI("Inherite_RTSlotSet() has failed because given Descriptions have some ColorSlots that has same indexes! Each slot's index should be unique and lower than ColorSlotCount!");
					delete InheritedSet;
					return TAPI_INVALIDARGUMENT;
				}
			}
		}

		InheritedSet->COLOR_OPTYPEs = new GFX_API::OPERATION_TYPE[COLORSLOT_COUNT];
		//Set OPTYPEs of inherited slot set
		for (unsigned int i = 0; i < COLORSLOT_COUNT; i++) {
			if (i == DEPTHDESC_VECINDEX) {
				continue;
			}
			const GFX_API::OPERATION_TYPE& BSLOT_OPTYPE = BaseSet->PERFRAME_SLOTSETs[0].COLOR_SLOTs[Descriptions[i].SLOTINDEX].RT_OPERATIONTYPE;

			if (BSLOT_OPTYPE == GFX_API::OPERATION_TYPE::READ_ONLY &&
					(Descriptions[i].OPTYPE == GFX_API::OPERATION_TYPE::WRITE_ONLY || Descriptions[i].OPTYPE == GFX_API::OPERATION_TYPE::READ_AND_WRITE)
				) 
			{
				LOG_ERROR_TAPI("Inherite_RTSlotSet() has failed because you can't use a Read-Only ColorSlot with Write Access in a Inherited Set!");
				delete InheritedSet;
				return TAPI_INVALIDARGUMENT;
			}
			InheritedSet->COLOR_OPTYPEs[Descriptions[i].SLOTINDEX] = Descriptions[i].OPTYPE;
		}

		InheritedSlotSetHandle = InheritedSet;
		return TAPI_SUCCESS;
	}

}