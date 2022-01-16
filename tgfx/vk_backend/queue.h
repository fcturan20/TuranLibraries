#include "predefinitions_vk.h"
#include <vector>



struct queuesys_vk {
	std::vector<VkDeviceQueueCreateInfo> analize_queues(gpu_public* vkgpu);
};