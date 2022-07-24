#include "vk_queue.h"
#include "vk_core.h"
#include <mutex>
#include "vk_synchronization.h"

struct commandbuffer_vk {
	VkCommandBuffer CB;
	bool is_Used = false;
};
struct commandpool_vk {
	commandpool_vk() = default;
	commandpool_vk(const commandpool_vk& RefCP) { CPHandle = RefCP.CPHandle;CBs = RefCP.CBs;}
	void operator= (const commandpool_vk& RefCP) { CPHandle = RefCP.CPHandle;CBs = RefCP.CBs;}
	commandbuffer_vk& CreateCommandBuffer();
	void DestroyCommandBuffer();
	VkCommandPool CPHandle = VK_NULL_HANDLE;
	std::vector<commandbuffer_vk*> CBs;
private:
	std::mutex Sync;
};

struct queue_vk {
	fence_idtype_vk RenderGraphFences[2];
	VkQueue Queue = VK_NULL_HANDLE;
};
struct queuefam_vk {
	uint32_t queueFamIndex = 0;
	unsigned int queuecount = 0, featurescore = 0;
	queueflag_vk supportflag;
	queue_vk* queues = nullptr;
	commandpool_vk CommandPools[2];
};
struct queuesys_data {
	std::vector<queuefam_vk*> queuefams;
};

void queuesys_vk::analize_queues(GPU_VKOBJ* vkgpu) {
	static constexpr unsigned char max_propertiescount = 10;
	bool is_presentationfound = false;
	vkgpu->queuefams = new queuefam_vk * [vkgpu->QUEUEFAMSCOUNT()];
	VkQueueFamilyProperties properties[max_propertiescount]; uint32_t dontuse;
#ifdef VULKAN_DEBUGGING
	if (max_propertiescount < vkgpu->QUEUEFAMSCOUNT()) { printer(result_tgfx_FAIL, "VK backend's queue analization process should support more queue families! Please report this!"); }
#endif
	vkGetPhysicalDeviceQueueFamilyProperties(vkgpu->PHYSICALDEVICE(), &dontuse, properties);
	for (unsigned int queuefamily_index = 0; queuefamily_index < vkgpu->QUEUEFAMSCOUNT(); queuefamily_index++) {
		VkQueueFamilyProperties* QueueFamily = &properties[queuefamily_index];
		queuefam_vk* queuefam = new queuefam_vk;


		queuefam->queueFamIndex = static_cast<uint32_t>(queuefamily_index);
		if (QueueFamily->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			vkgpu->desc.is_GraphicOperations_Supported = true;
			queuefam->supportflag.is_GRAPHICSsupported = true;
			vkgpu->GRAPHICS_supportedqueuecount++;
			queuefam->featurescore++;
			vkgpu->graphicsqueue = queuefam;
		}
		if (QueueFamily->queueFlags & VK_QUEUE_COMPUTE_BIT) {
			vkgpu->desc.is_ComputeOperations_Supported = true;
			queuefam->supportflag.is_COMPUTEsupported = true;
			vkgpu->COMPUTE_supportedqueuecount++;
			queuefam->featurescore++;
		}
		if (QueueFamily->queueFlags & VK_QUEUE_TRANSFER_BIT) {
			vkgpu->desc.is_TransferOperations_Supported = true;
			queuefam->supportflag.is_TRANSFERsupported = true;
			vkgpu->TRANSFERs_supportedqueuecount++;
			queuefam->featurescore++;
		}
		queuefam->queuecount = QueueFamily->queueCount;
		queuefam->queues = new queue_vk[queuefam->queuecount];
		data->queuefams.push_back(queuefam);
		vkgpu->queuefams[queuefamily_index] = queuefam;
	}
	if (!vkgpu->desc.is_GraphicOperations_Supported || !vkgpu->desc.is_TransferOperations_Supported || !vkgpu->desc.is_ComputeOperations_Supported) {
		printer(result_tgfx_FAIL, "The GPU doesn't support one of the following operations, so we can't let you use this GPU: Compute, Transfer, Graphics");
		return;
	}
	//Sort the queue families by their feature score (Example: Element 0 is Transfer Only, Element 1 is Transfer-Compute, Element 2 is Graphics-Transfer-Compute etc)
	//QuickSort Algorithm
	if (vkgpu->QUEUEFAMSCOUNT()) {
		bool should_Sort = true;
		while (should_Sort) {
			should_Sort = false;
			for (unsigned char i = 0; i < vkgpu->QUEUEFAMSCOUNT() - 1; i++) {
				if (vkgpu->queuefams[i + 1]->featurescore < vkgpu->queuefams[i]->featurescore) {
					should_Sort = true;
					queuefam_vk* secondqueuefam = vkgpu->queuefams[i + 1];
					vkgpu->queuefams[i + 1] = vkgpu->queuefams[i];
					vkgpu->queuefams[i] = secondqueuefam;
				}
			}
		}
	}
}


//While creating VK Logical Device, we need which queues to create. Get that info from here.
std::vector<VkDeviceQueueCreateInfo> queuesys_vk::get_queue_cis(GPU_VKOBJ* vkgpu) {
	std::vector<VkDeviceQueueCreateInfo> QueueCreationInfos(0);
	//Queue Creation Processes
	for (unsigned int QueueIndex = 0; QueueIndex < vkgpu->QUEUEFAMSCOUNT(); QueueIndex++) {
		queuefam_vk* queuefam = vkgpu->queuefams[QueueIndex];
		VkDeviceQueueCreateInfo QueueInfo = {};
		QueueInfo.flags = 0;
		QueueInfo.pNext = nullptr;
		QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueInfo.queueFamilyIndex = queuefam->queueFamIndex;
		float QueuePriority = 1.0f;
		float* priorities = new float[queuefam->queuecount];
		QueueInfo.pQueuePriorities = priorities;
		QueueInfo.queueCount = queuefam->queuecount;
		for (unsigned int i = 0; i < queuefam->queuecount; i++) {
			priorities[i] = 1.0f - (float(i) / float(queuefam->queuecount));
		}
		QueueCreationInfos.push_back(QueueInfo);
	}
	return QueueCreationInfos;
}


void queuesys_vk::get_queue_objects(GPU_VKOBJ* vkgpu) {
	rendergpu->AllQueueFamilies = new uint32_t[rendergpu->QUEUEFAMSCOUNT()];
	for (unsigned int i = 0; i < rendergpu->QUEUEFAMSCOUNT(); i++) {
		for (unsigned int queueindex = 0; queueindex < rendergpu->queuefams[i]->queuecount; queueindex++) {
			vkGetDeviceQueue(rendergpu->LOGICALDEVICE(), rendergpu->queuefams[i]->queueFamIndex, queueindex, &(rendergpu->queuefams[i]->queues[queueindex].Queue));
		}
		rendergpu->AllQueueFamilies[i] = rendergpu->queuefams[i]->queueFamIndex;
	}
	//Create Command Pool for each Queue Family
	for (unsigned int queuefamindex = 0; queuefamindex < rendergpu->QUEUEFAMSCOUNT(); queuefamindex++) {
		if (!(rendergpu->QUEUEFAMS()[queuefamindex]->supportflag.is_COMPUTEsupported || rendergpu->QUEUEFAMS()[queuefamindex]->supportflag.is_GRAPHICSsupported ||
			rendergpu->QUEUEFAMS()[queuefamindex]->supportflag.is_TRANSFERsupported)
			) {
			continue;
		}
		for (unsigned char i = 0; i < 2; i++) {
			VkCommandPoolCreateInfo cp_ci_g = {};
			cp_ci_g.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cp_ci_g.queueFamilyIndex = rendergpu->QUEUEFAMS()[queuefamindex]->queueFamIndex;
			cp_ci_g.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			cp_ci_g.pNext = nullptr;

			if (vkCreateCommandPool(rendergpu->LOGICALDEVICE(), &cp_ci_g, nullptr, &rendergpu->QUEUEFAMS()[queuefamindex]->CommandPools[i].CPHandle) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "VulkanRenderer: Command pool creation for a queue has failed at vkCreateCommandPool()!");
			}
		}
	}
}
uint32_t queuesys_vk::get_queuefam_index(GPU_VKOBJ* vkgpu, queuefam_vk* fam) {
#ifdef VULKAN_DEBUGGING
	bool isfound = false;
	for (unsigned int i = 0; i < vkgpu->QUEUEFAMSCOUNT(); i++) {
		if (vkgpu->queuefams[i] == fam) { isfound = true; }
	}
	if (!isfound) { printer(result_tgfx_FAIL, "Queue Family Handle is invalid!"); }
#endif
	return fam->queueFamIndex;
}
queuefam_vk* queuesys_vk::check_windowsupport(GPU_VKOBJ* vkgpu, VkSurfaceKHR WindowSurface) {
	queuefam_vk* supported_queue = nullptr;
	for (unsigned int i = 0; i < rendergpu->QUEUEFAMSCOUNT(); i++) {
		VkBool32 Does_Support = 0;
		queuefam_vk* queuefam = rendergpu->QUEUEFAMS()[i];
		vkGetPhysicalDeviceSurfaceSupportKHR(rendergpu->PHYSICALDEVICE(), queuefam->queueFamIndex, WindowSurface, &Does_Support);
		if (Does_Support) {
			if (!queuefam->supportflag.is_PRESENTATIONsupported) { queuefam->supportflag.is_PRESENTATIONsupported = true; queuefam->featurescore++; }
			supported_queue = queuefam;
		}
	}
	return supported_queue;
}
commandbuffer_vk* queuesys_vk::get_commandbuffer(GPU_VKOBJ* vkgpu, queuefam_vk* family, unsigned char FrameIndex) {
	commandpool_vk& CP = family->CommandPools[FrameIndex];

	VkCommandBufferAllocateInfo cb_ai = {};
	cb_ai.commandBufferCount = 1;
	cb_ai.commandPool = CP.CPHandle;
	cb_ai.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cb_ai.pNext = nullptr;
	cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

	commandbuffer_vk* VK_CB = new commandbuffer_vk;
	if (vkAllocateCommandBuffers(rendergpu->LOGICALDEVICE(), &cb_ai, &VK_CB->CB) != VK_SUCCESS) {
		printer(result_tgfx_FAIL, "vkAllocateCommandBuffers() failed while creating command buffers for RGBranches, report this please!");
		return nullptr;
	}
	VK_CB->is_Used = false;

	CP.CBs.push_back(VK_CB);

	return VK_CB;
}
VkCommandBuffer queuesys_vk::get_commandbufferobj(commandbuffer_vk* id) {
	return id->CB;
}
fence_idtype_vk queuesys_vk::queueSubmit(GPU_VKOBJ* vkgpu, queuefam_vk* family, const std::vector<semaphore_idtype_vk>& WaitSemaphores, const std::vector<semaphore_idtype_vk>& SignalSemaphores, const VkCommandBuffer* commandbuffers, const VkPipelineStageFlags* cb_flags, unsigned int CBCount) {
	printer(result_tgfx_WARNING, "QueueSystem->queueSubmit() isn't properly implemented. We should keep track of command buffers");
	fence_vk& fence = fencesys->CreateFence();
	
	std::vector<VkSemaphore> waits(WaitSemaphores.size()), signals(SignalSemaphores.size());
	for (unsigned int wait_i = 0; wait_i < WaitSemaphores.size(); wait_i++) {
		waits[wait_i] = semaphoresys->GetSemaphore_byID(WaitSemaphores[wait_i]).vksemaphore();
	}
	for (unsigned int signal_i = 0; signal_i < SignalSemaphores.size(); signal_i++) {
		signals[signal_i] = semaphoresys->GetSemaphore_byID(SignalSemaphores[signal_i]).vksemaphore();
	}

	VkSubmitInfo info = {};
	info.commandBufferCount = CBCount;
	info.pCommandBuffers = commandbuffers;
	info.pNext = nullptr;
	info.pWaitDstStageMask = cb_flags;
	info.pWaitSemaphores = waits.data();
	info.waitSemaphoreCount = waits.size();
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.signalSemaphoreCount = signals.size();
	info.pSignalSemaphores = signals.data();
	if (vkQueueSubmit(family->queues[0].Queue, 1, &info, fence.Fence_o) != VK_SUCCESS) {
		printer(result_tgfx_FAIL, "Queue Submission has failed!");
		fence_idtype_vk fenceid = fence.getID();
		fencesys->DestroyFence(fenceid);
		return invalid_fenceid;
	}
	fence.wait_semaphores = WaitSemaphores;
	return fence.getID();
}
bool queuesys_vk::does_queuefamily_support(GPU_VKOBJ* vkgpu, queuefam_vk* family, const queueflag_vk& flag) {
	printer(result_tgfx_NOTCODED, "does_queuefamily_support() isn't coded");
	return false;
}
VkQueue queuesys_vk::get_queue(GPU_VKOBJ* vkgpu, queuefam_vk* queuefam) {
	return queuefam->queues[0].Queue;
}
extern void Create_QueueSys() {
	queuesys = new queuesys_vk;
	queuesys->data = new queuesys_data;
}