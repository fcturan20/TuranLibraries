#pragma once
#include "predefinitions_vk.h"


struct allocatorsys_data;
struct allocatorsys_vk {
	//Use this to create gpu memory description, before creating logical device
	void analize_gpumemory(gpu_public* gpu);
	void do_allocations(gpu_public* gpu);
	unsigned int get_memorytypeindex_byID(gpu_public* gpu, unsigned int memorytype_id);
	//Returns the given offset
	//RequiredSize: You should use vkGetBuffer/ImageRequirements' output size here
	//AligmentOffset: You should use GPU's own aligment offset limitation for the specified type of data
	//RequiredAlignment: You should use vkGetBuffer/ImageRequirements' output alignment here
	result_tgfx suballocate_memoryblock(unsigned int memoryid, VkDeviceSize RequiredSize, VkDeviceSize AlignmentOffset, VkDeviceSize RequiredAlignment, VkDeviceSize* ResultOffset);

	result_tgfx suballocate_image(texture_vk* texture);
	void free_memoryblock(unsigned int memoryid, VkDeviceSize offset);
	allocatorsys_data* data;
};