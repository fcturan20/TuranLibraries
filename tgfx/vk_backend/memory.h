#pragma once
#include "predefinitions_vk.h"

struct memoryblock_vk;
struct allocatorsys_data;
struct allocatorsys_vk {
	//Use this to create gpu memory description, before creating logical device
	void analize_gpumemory(gpu_public* gpu);
	void do_allocations(gpu_public* gpu);
	unsigned int get_memorytypeindex_byID(gpu_public* gpu, unsigned int memorytype_id);
	memoryallocationtype_tgfx get_memorytype_byID(gpu_public* gpu, unsigned int memorytype_id);
	VkBuffer get_memorybufferhandle_byID(gpu_public* gpu, unsigned int memorytype_id);
	result_tgfx copy_to_mappedmemory(gpu_public* gpu, unsigned int memorytype_id, const void* data, unsigned long data_size, unsigned long offset);

	result_tgfx suballocate_image(texture_vk* texture);
	result_tgfx suballocate_buffer(VkBuffer BUFFER, VkBufferUsageFlags UsageFlags, memoryblock_vk& Block);
	void free_memoryblock(unsigned int memoryid, VkDeviceSize offset);
	allocatorsys_data* data;
};