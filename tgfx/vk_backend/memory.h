#pragma once
#include "predefinitions_vk.h"


struct allocatorsys_data;
struct allocatorsys_vk {
	//Use this to create gpu memory description, before creating logical device
	void analize_gpumemory(gpu_public* gpu);
	void do_allocations(gpu_public* gpu);
	unsigned int get_memorytypeindex_byID(gpu_public* gpu, unsigned int memorytype_id);

	result_tgfx suballocate_image(texture_vk* texture);
	void free_memoryblock(unsigned int memoryid, VkDeviceSize offset);
	allocatorsys_data* data;
};