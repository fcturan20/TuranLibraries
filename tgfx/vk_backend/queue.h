#include "predefinitions_vk.h"
#include <vector>


struct queuesys_data;
struct queuesys_vk {
	//Analize queues to fill gpu description
	void analize_queues(gpu_public* vkgpu);
	//While creating VK Logical Device, we need which queues to create. Get that info from here.
	std::vector<VkDeviceQueueCreateInfo> get_queue_cis(gpu_public* vkgpu);
	//Get VkQueue objects from logical device
	void get_queue_objects(gpu_public* vkgpu);
	uint32_t get_queuefam_index(gpu_public* vkgpu, queuefam_vk* fam);
	bool check_windowsupport(gpu_public* vkgpu, VkSurfaceKHR surface);
	queuesys_data* data;
};