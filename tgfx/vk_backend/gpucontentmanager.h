#pragma once
#include "predefinitions_vk.h"


struct gpudatamanager_private;
struct gpudatamanager_public {
	gpudatamanager_private* hidden;
	//Create Global descriptor sets
	void Resource_Finalizations();
	//Apply changes in Descriptor Sets, RTSlotSets
	void Apply_ResourceChanges();
	VkDescriptorSet get_GlobalBuffersDescSet();
	VkDescriptorSet get_GlobalTexturesDescsSet();
};