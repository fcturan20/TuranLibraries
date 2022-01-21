#pragma once
#include "predefinitions_vk.h"


struct allocatorsys_data;
struct allocatorsys_vk {
	//Use this to create gpu memory description, before creating logical device
	void analize_gpumemory(gpu_public* gpu);
	void do_allocations(gpu_public* gpu);
	//RequiredSize: You should use vkGetBuffer/ImageRequirements' output size here
	//AligmentOffset: You should use GPU's own aligment offset limitation for the specified type of data
	//RequiredAlignment: You should use vkGetBuffer/ImageRequirements' output alignment here
	VkDeviceSize suballocate_memoryblock(unsigned int memoryid, VkDevice RequiredSize, VkDeviceSize AlignmentOffset, VkDeviceSize RequiredAlignment);
	void free_memoryblock(unsigned int memoryid, VkDeviceSize offset);
	allocatorsys_data* data;
};