#pragma once
#include "vk_predefinitions.h"

struct memoryblock_vk;
struct allocatorsys_data;
struct gpuallocatorsys_vk {
	//Use this to create gpu memory description, before creating logical device
	void analize_gpumemory(GPU_VKOBJ* gpu);
	void do_allocations(GPU_VKOBJ* gpu);
	unsigned int get_memorytypeindex_byID(GPU_VKOBJ* gpu, unsigned int memorytype_id);
	memoryallocationtype_tgfx get_memorytype_byID(GPU_VKOBJ* gpu, unsigned int memorytype_id);
	VkBuffer get_memorybufferhandle_byID(GPU_VKOBJ* gpu, unsigned int memorytype_id);
	result_tgfx copy_to_mappedmemory(GPU_VKOBJ* gpu, unsigned int memorytype_id, const void* data, unsigned long data_size, unsigned long offset);

	result_tgfx suballocate_image(TEXTURE_VKOBJ* texture);
	result_tgfx suballocate_buffer(VkBuffer BUFFER, VkBufferUsageFlags UsageFlags, memoryblock_vk& Block);
	void free_memoryblock(unsigned int memoryid, VkDeviceSize offset);
};