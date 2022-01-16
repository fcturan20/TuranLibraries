#include "memory.h"
#include <atomic>
#include <vector>
#include "tgfx_structs.h"
#include "core.h"

struct allocatorsys_privatefuncs {
	static inline void Analize_PhysicalDeviceMemoryProperties(gpu_public* VKGPU, gpudescription_tgfx& GPUdesc);
};
struct suballocation_vk {
	VkDeviceSize Size = 0, Offset = 0;
	std::atomic<bool> isEmpty;
	suballocation_vk();
	suballocation_vk(const suballocation_vk& copyblock);
};
struct memoryallocation_vk {
	std::atomic<uint32_t> UnusedSize = 0;
	uint32_t MemoryTypeIndex;
	unsigned long long MaxSize = 0, ALLOCATIONSIZE = 0;
	void* MappedMemory;
	memoryallocationtype_tgfx TYPE;

	VkDeviceMemory Allocated_Memory;
	VkBuffer Buffer;
	//threadlocal_vector<suballocation_vk> Allocated_Blocks;
	memoryallocation_vk();
	memoryallocation_vk(const memoryallocation_vk& copyALLOC);
	//Use this function in Suballocate_Buffer and Suballocate_Image after you're sure that you have enough memory in the allocation
	VkDeviceSize FindAvailableOffset(VkDeviceSize RequiredSize, VkDeviceSize AlignmentOffset, VkDeviceSize RequiredAlignment);
	//Free a block
	void FreeBlock(VkDeviceSize Offset);
};

void allocatorsys_vk::analize_gpumemory(gpu_public* vkgpu) {

}

void allocatorsys_privatefuncs::Analize_PhysicalDeviceMemoryProperties(gpu_public* VKGPU, gpudescription_tgfx& GPUdesc) {
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

		if (GPUdesc.GPU_TYPE != gpu_type_tgfx_DISCRETE_GPU) {
			continue;
		}
		if (!isDeviceLocal && !isHostVisible && !isHostCoherent && !isHostCached) {
			continue;
		}
		if (isDeviceLocal) {
			if (isHostVisible && isHostCoherent) {
				memory_description_tgfx memtype_desc;
				memtype_desc.allocationtype = memoryallocationtype_FASTHOSTVISIBLE;
				memtype_desc.memorytype_id = mem_descs.size();
				memtype_desc.max_allocationsize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				mem_descs.push_back(memtype_desc);

				/*				
				memoryallocation_vk alloc;
				alloc.MemoryTypeIndex = MemoryTypeIndex;
				alloc.TYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::FASTHOSTVISIBLE;
				VKGPU->ALLOCs.push_back(alloc);
				LOG_STATUS_TAPI("Found FAST HOST VISIBLE BIT! Size: " + to_string(GPUdesc.FASTHOSTVISIBLE_MaxMemorySize));
			}
			else {
				GFX_API::MemoryType MEMTYPE;
				MEMTYPE.HEAPTYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL;
				MEMTYPE.MemoryTypeIndex = GPUdesc.MEMTYPEs.size();
				GPUdesc.MEMTYPEs.push_back(MEMTYPE);
				VK_MemoryAllocation alloc;
				alloc.TYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL;
				alloc.MemoryTypeIndex = MemoryTypeIndex;
				VKGPU->ALLOCs.push_back(alloc);
				GPUdesc.DEVICELOCAL_MaxMemorySize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				LOG_STATUS_TAPI("Found DEVICE LOCAL BIT! Size: " + to_string(GPUdesc.DEVICELOCAL_MaxMemorySize));
			}
		}
		else if (isHostVisible && isHostCoherent) {
			if (isHostCached) {
				GFX_API::MemoryType MEMTYPE;
				MEMTYPE.HEAPTYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::READBACK;
				MEMTYPE.MemoryTypeIndex = GPUdesc.MEMTYPEs.size();
				GPUdesc.MEMTYPEs.push_back(MEMTYPE);
				VK_MemoryAllocation alloc;
				alloc.MemoryTypeIndex = MemoryTypeIndex;
				alloc.TYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::READBACK;
				VKGPU->ALLOCs.push_back(alloc);
				GPUdesc.READBACK_MaxMemorySize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				LOG_STATUS_TAPI("Found READBACK BIT! Size: " + to_string(GPUdesc.READBACK_MaxMemorySize));
			}
			else {
				GFX_API::MemoryType MEMTYPE;
				MEMTYPE.HEAPTYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE;
				MEMTYPE.MemoryTypeIndex = GPUdesc.MEMTYPEs.size();
				GPUdesc.MEMTYPEs.push_back(MEMTYPE);
				VK_MemoryAllocation alloc;
				alloc.MemoryTypeIndex = MemoryTypeIndex;
				alloc.TYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE;
				VKGPU->ALLOCs.push_back(alloc);
				GPUdesc.HOSTVISIBLE_MaxMemorySize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				LOG_STATUS_TAPI("Found HOST VISIBLE BIT! Size: " + to_string(GPUdesc.HOSTVISIBLE_MaxMemorySize));
				*/
			}
		}
	}
}
extern void Create_AllocatorSys() {
	allocatorsys = new allocatorsys_vk;

}