#include <stdio.h>
#include "imgui.h"
#include "core.h"


result_tgfx IMGUI_VK::Initialize(subdrawpass_tgfx_handle SubPass) {
	//Create a special Descriptor Pool for IMGUI
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
	};

	VkDescriptorPoolCreateInfo descpool_ci;
	descpool_ci.flags = 0;
	descpool_ci.maxSets = 1;
	descpool_ci.pNext = nullptr;
	descpool_ci.poolSizeCount = 1;
	descpool_ci.pPoolSizes = pool_sizes;
	descpool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	if (vkCreateDescriptorPool(rendergpu->LOGICALDEVICE(), &descpool_ci, nullptr, &descpool) != VK_SUCCESS) {
		printer(result_tgfx_FAIL, "Creating a descriptor pool for dear IMGUI has failed!");
		return result_tgfx_FAIL;
	}


	return result_tgfx_SUCCESS;
}

extern void Create_IMGUI() {
	printf("Create imgui!");
}