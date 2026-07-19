#include "vk_predefinitions.h"

#include "vk_resource.h"

VkInstance GVkInstance = VK_NULL_HANDLE;
VkApplicationInfo GVkAppInfo{};
unsigned char VKGLOBAL_FRAMEINDEX = 0;

void Append_pNext(void* targetStruct, void* attachStruct)
{
	// pNext is always second member of a struct
	void** target_pNext = (void**)(((VkStructureType*)targetStruct) + 2);
	// Iterate until you find last pNext filled
	while (*target_pNext)
	{
		target_pNext = (void**)(((VkStructureType*)*target_pNext) + 2);
	}
	*target_pNext = attachStruct;
}