#include "queue.h"
#include "core.h"

//Initializes as everything is false (same as CreateInvalidNullFlag)
struct queueflag_vk {
	bool is_GRAPHICSsupported : 1;
	bool is_PRESENTATIONsupported : 1;
	bool is_COMPUTEsupported : 1;
	bool is_TRANSFERsupported : 1;
	bool doesntNeedAnything : 1;	//This is a special flag to be used as "Don't care other parameters, this is a special operation"
	//bool is_VTMEMsupported : 1;	Not supported for now!
	inline queueflag_vk() {
		doesntNeedAnything = false; is_GRAPHICSsupported = false; is_PRESENTATIONsupported = false; is_COMPUTEsupported = false; is_TRANSFERsupported = false;
	}
	inline static queueflag_vk CreateInvalidNullFlag() {	//Returned flag's every bit is false. You should set at least one of them as true.
		return queueflag_vk();
	}
	inline bool isFlagValid() const {
		if (doesntNeedAnything && (is_GRAPHICSsupported || is_COMPUTEsupported || is_PRESENTATIONsupported || is_TRANSFERsupported)) {
			printer(result_tgfx_FAIL, "(This flag doesn't need anything but it also needs something, this shouldn't happen!");
			return false;
		}
		if (!doesntNeedAnything && !is_GRAPHICSsupported && !is_COMPUTEsupported && !is_PRESENTATIONsupported && !is_TRANSFERsupported) {
			printer(result_tgfx_FAIL, "(This flag needs something but it doesn't support anything");
			return false;
		}
		return true;
	}
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
};
struct queuesys_data {
	std::vector<queuefam_vk*> queuefams;
};

void queuesys_vk::analize_queues(gpu_public* vkgpu) {
	bool is_presentationfound = false;
	vkgpu->queuefams = new queuefam_vk * [vkgpu->QUEUEFAMSCOUNT()];
	for (unsigned int queuefamily_index = 0; queuefamily_index < vkgpu->QUEUEFAMSCOUNT(); queuefamily_index++) {
		VkQueueFamilyProperties* QueueFamily = &vkgpu->QUEUEFAMILYPROPERTIES()[queuefamily_index];
		queuefam_vk* queuefam = new queuefam_vk;


		queuefam->queueFamIndex = static_cast<uint32_t>(queuefamily_index);
		if (QueueFamily->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			vkgpu->desc.is_GraphicOperations_Supported = true;
			queuefam->supportflag.is_GRAPHICSsupported = true;
			vkgpu->GRAPHICS_supportedqueuecount++;
			queuefam->featurescore++;
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
std::vector<VkDeviceQueueCreateInfo> queuesys_vk::get_queue_cis(gpu_public* vkgpu) {
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


void queuesys_vk::get_queue_objects(gpu_public* vkgpu) {
	rendergpu->AllQueueFamilies = new uint32_t[rendergpu->QUEUEFAMSCOUNT()];
	for (unsigned int i = 0; i < rendergpu->QUEUEFAMSCOUNT(); i++) {
		printer(result_tgfx_SUCCESS, ("Queue Feature Score: " + std::to_string(rendergpu->queuefams[i]->featurescore)).c_str());
		for (unsigned int queueindex = 0; queueindex < rendergpu->queuefams[i]->queuecount; queueindex++) {
			vkGetDeviceQueue(rendergpu->Logical_Device, rendergpu->queuefams[i]->queueFamIndex, queueindex, &(rendergpu->queuefams[i]->queues[queueindex].Queue));
		}
		printer(result_tgfx_SUCCESS, ("After vkGetDeviceQueue() " + std::to_string(i)).c_str());
		rendergpu->AllQueueFamilies[i] = rendergpu->queuefams[i]->queueFamIndex;
	}
	printer(result_tgfx_SUCCESS, "After vkGetDeviceQueue()");
}
uint32_t queuesys_vk::get_queuefam_index(gpu_public* vkgpu, queuefam_vk* fam) {
#ifdef VULKAN_DEBUGGING
	bool isfound = false;
	for (unsigned int i = 0; i < vkgpu->QUEUEFAMSCOUNT(); i++) {
		if (vkgpu->queuefams[i] == fam) { isfound = true; }
	}
	if (!isfound) { printer(result_tgfx_FAIL, "Queue Family Handle is invalid!"); }
#endif
	return fam->queueFamIndex;
}
bool queuesys_vk::check_windowsupport(gpu_public* vkgpu, VkSurfaceKHR WindowSurface) {
	bool issupported = false;
	for (unsigned int i = 0; i < rendergpu->QUEUEFAMSCOUNT(); i++) {
		VkBool32 Does_Support = 0;
		queuefam_vk* queuefam = rendergpu->QUEUEFAMS()[i];
		vkGetPhysicalDeviceSurfaceSupportKHR(rendergpu->PHYSICALDEVICE(), queuefam->queueFamIndex, WindowSurface, &Does_Support);
		if (Does_Support) {
			if (!queuefam->supportflag.is_PRESENTATIONsupported) { queuefam->supportflag.is_PRESENTATIONsupported = true; queuefam->featurescore++; }
			issupported = true;
		}
	}
	return issupported;
}
extern void Create_QueueSys() {
	queuesys = new queuesys_vk;
	queuesys->data = new queuesys_data;
}