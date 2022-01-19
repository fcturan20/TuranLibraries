#include "memory.h"
#include <atomic>
#include <vector>
#include <tgfx_core.h>
#include <tgfx_structs.h>
#include "core.h"
#include "includes.h"


struct suballocation_vk {
	VkDeviceSize Size = 0, Offset = 0;
	std::atomic<bool> isEmpty;
	suballocation_vk();
	suballocation_vk(const suballocation_vk& copyblock);
};
struct memoryHeap_vk {

};

//Each memory type has its own buffers
struct memorytype_vk {
	unsigned int memorytype_ID = UINT32_MAX;		//AllocatorSys' index
	VkDeviceMemory Allocated_Memory;
	VkBuffer Buffer;
	std::atomic<uint32_t> UnusedSize = 0;
	uint32_t MemoryTypeIndex;	//Vulkan's index
	unsigned long long MaxSize = 0, ALLOCATIONSIZE = 0;
	void* MappedMemory;
	memoryallocationtype_tgfx TYPE;
	memorytype_vk() = default;
	memorytype_vk(const memorytype_vk& copy) {
		Allocated_Memory = copy.Allocated_Memory;
		Buffer = copy.Buffer;
		UnusedSize.store(UnusedSize.load());
		MemoryTypeIndex = copy.MemoryTypeIndex;
		MaxSize = copy.MaxSize;
		ALLOCATIONSIZE = copy.ALLOCATIONSIZE;
		MappedMemory = copy.MappedMemory;
		memorytype_ID = copy.memorytype_ID;
	}
};
struct allocatorsys_data {
	std::vector<memorytype_vk> memorytypes;
};


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

				data->memorytypes.push_back(memorytype_vk());
				memorytype_vk& memtype = data->memorytypes[data->memorytypes.size() - 1];
				memtype.MemoryTypeIndex = MemoryTypeIndex;
				memtype.TYPE = memoryallocationtype_FASTHOSTVISIBLE;
				printer(result_tgfx_SUCCESS, ("Found FAST HOST VISIBLE BIT! Size: " + std::to_string(memtype_desc.allocationtype)).c_str());
			}
			else {
				memory_description_tgfx memtype_desc;
				memtype_desc.allocationtype = memoryallocationtype_DEVICELOCAL;
				memtype_desc.memorytype_id = data->memorytypes.size();
				memtype_desc.max_allocationsize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				mem_descs.push_back(memtype_desc);

				data->memorytypes.push_back(memorytype_vk());
				memorytype_vk& memtype = data->memorytypes[data->memorytypes.size() - 1];
				memtype.MemoryTypeIndex = MemoryTypeIndex;
				memtype.TYPE = memoryallocationtype_DEVICELOCAL;
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

				data->memorytypes.push_back(memorytype_vk());
				memorytype_vk& memtype = data->memorytypes[data->memorytypes.size() - 1];
				memtype.MemoryTypeIndex = MemoryTypeIndex;
				memtype.TYPE = memoryallocationtype_READBACK;
				printer(result_tgfx_SUCCESS, ("Found READBACK BIT! Size: " + std::to_string(memtype_desc.allocationtype)).c_str());
			}
			else {
				memory_description_tgfx memtype_desc;
				memtype_desc.allocationtype = memoryallocationtype_HOSTVISIBLE;
				memtype_desc.memorytype_id = data->memorytypes.size();
				memtype_desc.max_allocationsize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				mem_descs.push_back(memtype_desc);

				data->memorytypes.push_back(memorytype_vk());
				memorytype_vk& memtype = data->memorytypes[data->memorytypes.size() - 1];
				memtype.MemoryTypeIndex = MemoryTypeIndex;
				memtype.TYPE = memoryallocationtype_HOSTVISIBLE;
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
	/*
	//Do memory allocations
	for (unsigned int allocindex = 0; allocindex < rendergpu->ALLOCS().size(); allocindex++) {
		memoryallocation_vk& ALLOC = rendergpu->ALLOCS()[allocindex];
		if (!ALLOC.ALLOCATIONSIZE) {
			continue;
		}

		VkMemoryRequirements memrequirements;
		VkBufferUsageFlags USAGEFLAGs = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		uint64_t AllocSize = ALLOC.ALLOCATIONSIZE;
		VkBuffer GPULOCAL_buf = Create_VkBuffer(AllocSize, USAGEFLAGs);
		vkGetBufferMemoryRequirements(rendergpu->LOGICALDEVICE(), GPULOCAL_buf, &memrequirements);
		if (!(memrequirements.memoryTypeBits & (1 << ALLOC.MemoryTypeIndex))) {
			printer(result_tgfx_FAIL, "GPU Local Memory Allocation doesn't support the MemoryType!");
			return;
		}
		VkMemoryAllocateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		ci.allocationSize = memrequirements.size;
		ci.memoryTypeIndex = ALLOC.MemoryTypeIndex;
		VkDeviceMemory allocated_memory;
		if (vkAllocateMemory(rendergpu->LOGICALDEVICE(), &ci, nullptr, &allocated_memory) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "vk_gpudatamanager initialization has failed because vkAllocateMemory GPULocalBuffer has failed!");
		}
		ALLOC.Allocated_Memory = allocated_memory;
		ALLOC.UnusedSize.DirectStore(AllocSize);
		ALLOC.MappedMemory = nullptr;
		ALLOC.Buffer = GPULOCAL_buf;
		if (vkBindBufferMemory(rendergpu->LOGICALDEVICE(), GPULOCAL_buf, allocated_memory, 0) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "Binding buffer to the allocated memory has failed!");
		}

		//If allocation is device local, it is not mappable. So continue.
		if (ALLOC.TYPE == memoryallocationtype_DEVICELOCAL) {
			continue;
		}

		if (vkMapMemory(rendergpu->LOGICALDEVICE(), allocated_memory, 0, memrequirements.size, 0, &ALLOC.MappedMemory) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "Mapping the HOSTVISIBLE memory has failed!");
			return;
		}
	}
	*/
}

VkDeviceSize allocatorsys_vk::suballocate_memoryblock(memorytype_vk* memory, VkDevice RequiredSize, VkDeviceSize AlignmentOffset, VkDeviceSize RequiredAlignment) {
	return 0;
}
void allocatorsys_vk::free_memoryblock(memorytype_vk* memory, VkDeviceSize offset) {

}
extern void Create_AllocatorSys() {
	allocatorsys = new allocatorsys_vk;
	allocatorsys->data = new allocatorsys_data;
}