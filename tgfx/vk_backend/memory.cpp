#include "memory.h"
#include <atomic>
#include <vector>
#include <tgfx_core.h>
#include <tgfx_structs.h>
#include "core.h"
#include "includes.h"
#include "resource.h"
#include <map>
#include <numeric>

extern VkBuffer Create_VkBuffer(unsigned int size, VkBufferUsageFlags usage);
struct suballocation_vk {
	VkDeviceSize Size = 0, Offset = 0;
	std::atomic<bool> isEmpty;
	suballocation_vk() : isEmpty(true){}
	suballocation_vk(const suballocation_vk& copyblock) : isEmpty(copyblock.isEmpty.load()), Size(copyblock.Size), Offset(copyblock.Offset) {}
	suballocation_vk operator=(const suballocation_vk& copyblock) {
		Size = copyblock.Size;
		Offset = copyblock.Offset;
		isEmpty.store(copyblock.isEmpty.load());
		return *this;
	}
};
struct memoryHeap_vk {

};

//Each memory type has its own buffers
struct memorytype_vk {
	VkDeviceMemory Allocated_Memory;
	VkBuffer Buffer;
	std::atomic<uint32_t> UnusedSize = 0;
	uint32_t MemoryTypeIndex;	//Vulkan's index
	VkDeviceSize MaxSize = 0, ALLOCATIONSIZE = 0;
	void* MappedMemory = nullptr;
	memoryallocationtype_tgfx TYPE;
	threadlocal_vector<suballocation_vk> Allocated_Blocks;
	memorytype_vk() : Allocated_Blocks(1024) {

	}
	memorytype_vk(const memorytype_vk& copy) : Allocated_Blocks(copy.Allocated_Blocks) {
		Allocated_Memory = copy.Allocated_Memory;
		Buffer = copy.Buffer;
		UnusedSize.store(UnusedSize.load());
		MemoryTypeIndex = copy.MemoryTypeIndex;
		MaxSize = copy.MaxSize;
		ALLOCATIONSIZE = copy.ALLOCATIONSIZE;
		MappedMemory = copy.MappedMemory;
	}
	inline VkDeviceSize FindAvailableOffset(VkDeviceSize RequiredSize, VkDeviceSize AlignmentOffset, VkDeviceSize RequiredAlignment) {
		VkDeviceSize FurthestOffset = 0;
		if (AlignmentOffset && !RequiredAlignment) {
			RequiredAlignment = AlignmentOffset;
		}
		else if (!AlignmentOffset && RequiredAlignment) {
			AlignmentOffset = RequiredAlignment;
		}
		else if (!AlignmentOffset && !RequiredAlignment) {
			AlignmentOffset = 1;
			RequiredAlignment = 1;
		}
		for (unsigned int ThreadID = 0; ThreadID < threadcount; ThreadID++) {
			for (unsigned int i = 0; i < Allocated_Blocks.size(ThreadID); i++) {
				suballocation_vk& Block = Allocated_Blocks.get(ThreadID, i);
				if (FurthestOffset <= Block.Offset) {
					FurthestOffset = Block.Offset + Block.Size;
				}
				if (!Block.isEmpty.load()) {
					continue;
				}

				VkDeviceSize Offset = CalculateOffset(Block.Offset, AlignmentOffset, RequiredAlignment);

				if (Offset + RequiredSize - Block.Offset > Block.Size ||
					Offset + RequiredSize - Block.Offset < (Block.Size / 5) * 3) {
					continue;
				}
				bool x = true, y = false;
				//Try to get the block first (Concurrent usages are prevented that way)
				if (!Block.isEmpty.compare_exchange_strong(x, y)) {
					continue;
				}
				//Don't change the block's own offset, because that'd probably cause shifting offset the memory block after free-reuse-free-reuse sequences
				return Offset;
			}
		}

		VkDeviceSize finaloffset = CalculateOffset(FurthestOffset, AlignmentOffset, RequiredAlignment);
		if (finaloffset + RequiredSize > ALLOCATIONSIZE) {
			printer(result_tgfx_FAIL, "Suballocation failed because memory type has not enough space for the data!");
			return UINT64_MAX;
		}
		//None of the current blocks is suitable, so create a new block in this thread's local memoryblocks list
		suballocation_vk newblock;
		newblock.isEmpty.store(false);
		newblock.Offset = finaloffset;
		newblock.Size = RequiredSize + newblock.Offset - FurthestOffset;
		Allocated_Blocks.push_back(newblock);
		return newblock.Offset;
	}
private:
	//Don't use this functions outside of the FindAvailableOffset
	inline VkDeviceSize CalculateOffset(VkDeviceSize baseoffset, VkDeviceSize AlignmentOffset, VkDeviceSize ReqAlignment) {
		VkDeviceSize FinalOffset = 0;
		VkDeviceSize LCM = std::lcm(AlignmentOffset, ReqAlignment);
		FinalOffset = (baseoffset % LCM) ? (((baseoffset / LCM) + 1) * LCM) : baseoffset;
		return FinalOffset;
	}
};
struct allocatorsys_data {
	std::map<unsigned int, memorytype_vk> memorytypes;
	unsigned int get_nextemptyid() { return next_emptyid++; }
	memorytype_vk& get_memtype_byid(unsigned int MemTypeID) {
		return memorytypes[MemTypeID];
	}
private:
	unsigned int next_emptyid = 0;
};

//Returns the given offset
//RequiredSize: You should use vkGetBuffer/ImageRequirements' output size here
//AligmentOffset: You should use GPU's own aligment offset limitation for the specified type of data
//RequiredAlignment: You should use vkGetBuffer/ImageRequirements' output alignment here
inline result_tgfx suballocate_memoryblock(unsigned int memoryid, VkDeviceSize RequiredSize, VkDeviceSize AlignmentOffset, VkDeviceSize RequiredAlignment, VkDeviceSize* ResultOffset) {
	memorytype_vk& memtype = allocatorsys->data->get_memtype_byid(memoryid);
	*ResultOffset = memtype.FindAvailableOffset(RequiredSize, AlignmentOffset, RequiredAlignment);
	if (*ResultOffset == UINT64_MAX) {
		printer(result_tgfx_NOTCODED, "There is not enough space in the memory allocation, Vulkan backend should support multiple memory allocations");
		return result_tgfx_NOTCODED;
	}
	return result_tgfx_SUCCESS;
}

void allocatorsys_vk::analize_gpumemory(gpu_public* VKGPU) {
	std::vector<memory_description_tgfx> mem_descs;
	for (uint32_t MemoryTypeIndex = 0; MemoryTypeIndex < VKGPU->MemoryProperties.memoryTypeCount; MemoryTypeIndex++) {
		VkMemoryType& MemoryType = VKGPU->MemoryProperties.memoryTypes[MemoryTypeIndex];
		bool isDeviceLocal = false;
		bool isHostVisible = false;
		bool isHostCoherent = false;
		bool isHostCached = false;

		if ((MemoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
			isDeviceLocal = true;
		}
		if ((MemoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
			isHostVisible = true;
		}
		if ((MemoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
			isHostCoherent = true;
		}
		if ((MemoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
			isHostCached = true;
		}


		if (!isDeviceLocal && !isHostVisible && !isHostCoherent && !isHostCached) {
			continue;
		}
		if (isDeviceLocal) {
			if (isHostVisible && isHostCoherent) {
				memory_description_tgfx memtype_desc;
				memtype_desc.allocationtype = memoryallocationtype_FASTHOSTVISIBLE;
				memtype_desc.memorytype_id = data->memorytypes.size();
				memtype_desc.max_allocationsize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				mem_descs.push_back(memtype_desc);

				unsigned int memtypeid = data->get_nextemptyid();
				data->memorytypes.insert(std::make_pair(memtypeid, memorytype_vk()));
				memorytype_vk& memtype = data->memorytypes[data->memorytypes.size() - 1];
				memtype.MemoryTypeIndex = MemoryTypeIndex;
				memtype.TYPE = memoryallocationtype_FASTHOSTVISIBLE;
				memtype.MaxSize = memtype_desc.max_allocationsize;
				VKGPU->memtype_ids.push_back(memtypeid);
				printer(result_tgfx_SUCCESS, ("Found FAST HOST VISIBLE BIT! Size: " + std::to_string(memtype_desc.allocationtype)).c_str());
			}
			else {
				memory_description_tgfx memtype_desc;
				memtype_desc.allocationtype = memoryallocationtype_DEVICELOCAL;
				memtype_desc.memorytype_id = data->memorytypes.size();
				memtype_desc.max_allocationsize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				mem_descs.push_back(memtype_desc);

				unsigned int memtypeid = data->get_nextemptyid();
				data->memorytypes.insert(std::make_pair(memtypeid, memorytype_vk()));
				memorytype_vk& memtype = data->memorytypes[data->memorytypes.size() - 1];
				memtype.MemoryTypeIndex = MemoryTypeIndex;
				memtype.TYPE = memoryallocationtype_DEVICELOCAL;
				memtype.MaxSize = memtype_desc.max_allocationsize;
				VKGPU->memtype_ids.push_back(memtypeid);
				printer(result_tgfx_SUCCESS, ("Found DEVICE LOCAL BIT! Size: " + std::to_string(memtype_desc.allocationtype)).c_str());
			}
		}
		else if (isHostVisible && isHostCoherent) {
			if (isHostCached) {
				memory_description_tgfx memtype_desc;
				memtype_desc.allocationtype = memoryallocationtype_READBACK;
				memtype_desc.memorytype_id = data->memorytypes.size();
				memtype_desc.max_allocationsize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				mem_descs.push_back(memtype_desc);

				unsigned int memtypeid = data->get_nextemptyid();
				data->memorytypes.insert(std::make_pair(memtypeid, memorytype_vk()));
				memorytype_vk& memtype = data->memorytypes[data->memorytypes.size() - 1];
				memtype.MemoryTypeIndex = MemoryTypeIndex;
				memtype.TYPE = memoryallocationtype_READBACK;
				memtype.MaxSize = memtype_desc.max_allocationsize;
				VKGPU->memtype_ids.push_back(memtypeid);
				printer(result_tgfx_SUCCESS, ("Found READBACK BIT! Size: " + std::to_string(memtype_desc.allocationtype)).c_str());
			}
			else {
				memory_description_tgfx memtype_desc;
				memtype_desc.allocationtype = memoryallocationtype_HOSTVISIBLE;
				memtype_desc.memorytype_id = data->memorytypes.size();
				memtype_desc.max_allocationsize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				mem_descs.push_back(memtype_desc);

				unsigned int memtypeid = data->get_nextemptyid();
				data->memorytypes.insert(std::make_pair(memtypeid, memorytype_vk()));
				memorytype_vk& memtype = data->memorytypes[data->memorytypes.size() - 1];
				memtype.MemoryTypeIndex = MemoryTypeIndex;
				memtype.TYPE = memoryallocationtype_HOSTVISIBLE;
				memtype.MaxSize = memtype_desc.max_allocationsize;
				VKGPU->memtype_ids.push_back(memtypeid);
				printer(result_tgfx_SUCCESS, ("Found HOST VISIBLE BIT! Size: " + std::to_string(memtype_desc.allocationtype)).c_str());
			}
		}
	}
	VKGPU->desc.MEMTYPEsCOUNT = mem_descs.size();
	memory_description_tgfx* memdescs_final = new memory_description_tgfx[mem_descs.size()];

	for (uint32_t i = 0; i < mem_descs.size(); i++) {
		memdescs_final[i] = mem_descs[i];
	}

	VKGPU->desc.MEMTYPEs = memdescs_final;
}
void allocatorsys_vk::do_allocations(gpu_public* gpu) {
	for (unsigned int allocindex = 0; allocindex < gpu->memtype_ids.size(); allocindex++) {
		memorytype_vk& memtype = data->get_memtype_byid(gpu->memtype_ids[allocindex]);
		if (!memtype.ALLOCATIONSIZE) {
			continue;
		}

		VkMemoryRequirements memrequirements;
		VkBufferUsageFlags USAGEFLAGs = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		uint64_t AllocSize = memtype.ALLOCATIONSIZE;
		VkBuffer GPULOCAL_buf = Create_VkBuffer(AllocSize, USAGEFLAGs);
		vkGetBufferMemoryRequirements(rendergpu->LOGICALDEVICE(), GPULOCAL_buf, &memrequirements);
		if (!(memrequirements.memoryTypeBits & (1 << memtype.MemoryTypeIndex))) {
			printer(result_tgfx_FAIL, "GPU Local Memory Allocation doesn't support the MemoryType!");
			continue;
		}
		VkMemoryAllocateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		ci.allocationSize = memrequirements.size;
		ci.memoryTypeIndex = memtype.MemoryTypeIndex;
		VkDeviceMemory allocated_memory;
		auto allocatememorycode = vkAllocateMemory(rendergpu->LOGICALDEVICE(), &ci, nullptr, &allocated_memory);
		if (allocatememorycode != VK_SUCCESS) {
			printer(result_tgfx_FAIL, ("vk_gpudatamanager initialization has failed because vkAllocateMemory has failed with code: " + std::to_string(allocatememorycode) + "!").c_str());
			continue;
		}
		memtype.Allocated_Memory = allocated_memory;
		memtype.UnusedSize.store(AllocSize);
		memtype.MappedMemory = nullptr;
		memtype.Buffer = GPULOCAL_buf;
		if (vkBindBufferMemory(rendergpu->LOGICALDEVICE(), GPULOCAL_buf, allocated_memory, 0) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "Binding buffer to the allocated memory has failed!");
		}

		//If allocation is device local, it is not mappable. So continue.
		if (memtype.TYPE == memoryallocationtype_DEVICELOCAL) {
			continue;
		}

		if (vkMapMemory(rendergpu->LOGICALDEVICE(), allocated_memory, 0, memrequirements.size, 0, &memtype.MappedMemory) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "Mapping the HOSTVISIBLE memory has failed!");
			continue;
		}
	}
}
unsigned int allocatorsys_vk::get_memorytypeindex_byID(gpu_public* gpu, unsigned int memorytype_id) {
	return data->get_memtype_byid(memorytype_id).MemoryTypeIndex;
}
memoryallocationtype_tgfx allocatorsys_vk::get_memorytype_byID(gpu_public* gpu, unsigned int memorytype_id) {
	return data->get_memtype_byid(memorytype_id).TYPE;
}
VkBuffer allocatorsys_vk::get_memorybufferhandle_byID(gpu_public* gpu, unsigned int memorytype_id) {
	return data->get_memtype_byid(memorytype_id).Buffer;
}
result_tgfx allocatorsys_vk::copy_to_mappedmemory(gpu_public* gpu, unsigned int memorytype_id, const void* dataptr, unsigned long data_size, unsigned long offset) {
	void* MappedMemory = data->get_memtype_byid(memorytype_id).MappedMemory;
	if (!MappedMemory) {
		printer(result_tgfx_FAIL, "Memory is not mapped, so you are either trying to upload to an GPU Local buffer or MemoryTypeIndex is not allocated memory type's index!");
		return result_tgfx_FAIL;
	}
	memcpy(((char*)MappedMemory) + offset, dataptr, data_size);
	return result_tgfx_SUCCESS;
}

result_tgfx allocatorsys_vk::suballocate_image(texture_vk* texture) {
	VkMemoryRequirements req;
	vkGetImageMemoryRequirements(rendergpu->LOGICALDEVICE(), texture->Image, &req);
	VkDeviceSize size = req.size;

	VkDeviceSize Offset = 0;
	unsigned int memorytypeindex = allocatorsys->get_memorytypeindex_byID(rendergpu, texture->Block.MemAllocIndex);
	if (!(req.memoryTypeBits & (1u << memorytypeindex))) {
		printer(result_tgfx_FAIL, "Intended texture doesn't support to be stored in the specified memory region!");
		return result_tgfx_FAIL;
	}
	if (suballocate_memoryblock(texture->Block.MemAllocIndex, size, 0, req.alignment, &Offset) != result_tgfx_SUCCESS) {
		printer(result_tgfx_FAIL, "Suballocation from memory block has failed for the texture!");
		return result_tgfx_FAIL;
	}

	if (vkBindImageMemory(rendergpu->LOGICALDEVICE(), texture->Image, allocatorsys->data->get_memtype_byid(texture->Block.MemAllocIndex).Allocated_Memory, Offset) != VK_SUCCESS) {
		printer(result_tgfx_FAIL, "VKContentManager->Suballocate_Image() has failed at VkBindImageMemory()!");
		return result_tgfx_FAIL;
	}
	texture->Block.Offset = Offset;
	return result_tgfx_SUCCESS;
}
result_tgfx allocatorsys_vk::suballocate_buffer(VkBuffer BUFFER, VkBufferUsageFlags UsageFlags, memoryblock_vk& Block) {
	VkMemoryRequirements bufferreq;
	vkGetBufferMemoryRequirements(rendergpu->LOGICALDEVICE(), BUFFER, &bufferreq);
	VkDeviceSize size = bufferreq.size;

	VkDeviceSize Offset = 0;
	unsigned int memorytypeindex = allocatorsys->get_memorytypeindex_byID(rendergpu, Block.MemAllocIndex);

	if (!(bufferreq.memoryTypeBits & (1u << memorytypeindex))) {
		printer(result_tgfx_FAIL, "Intended buffer doesn't support to be stored in specified memory region!");
		return result_tgfx_FAIL;
	}

	VkDeviceSize AlignmentOffset_ofGPU = 0;
	if (UsageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
		AlignmentOffset_ofGPU = rendergpu->DEVICEPROPERTIES().limits.minStorageBufferOffsetAlignment;
	}
	if (UsageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
		AlignmentOffset_ofGPU = rendergpu->DEVICEPROPERTIES().limits.minUniformBufferOffsetAlignment;
	}
	if (suballocate_memoryblock(Block.MemAllocIndex, size, AlignmentOffset_ofGPU, bufferreq.alignment, &Offset) != result_tgfx_SUCCESS) {
		printer(result_tgfx_FAIL, "Suballocation from memory block has failed for the buffer!");
		return result_tgfx_FAIL;
	}
	Block.Offset = Offset;

	return result_tgfx_SUCCESS;
}
void allocatorsys_vk::free_memoryblock(unsigned int memoryid, VkDeviceSize offset) {

}
extern void Create_AllocatorSys() {
	allocatorsys = new allocatorsys_vk;
	allocatorsys->data = new allocatorsys_data;
}
extern result_tgfx SetMemoryTypeInfo(unsigned int MemoryType_id, unsigned long long AllocationSize, extension_tgfx_listhandle Extensions) {
	memorytype_vk& ALLOC = allocatorsys->data->get_memtype_byid(MemoryType_id);
	if (AllocationSize > ALLOC.MaxSize) {
		printer(result_tgfx_INVALIDARGUMENT, "SetMemoryTypeInfo() has failed because allocation size can't be larger than maximum size!");
		return result_tgfx_INVALIDARGUMENT;
	}
	ALLOC.ALLOCATIONSIZE = AllocationSize;
	return result_tgfx_SUCCESS;
}