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
	unsigned int QueueFamilyIndex, QueueFeatureScore;
	queueflag_vk SupportFlag;
};
struct queuefam_vk {
	unsigned int queuecount = 0;
	queue_vk* queues = nullptr;
};

std::vector<VkDeviceQueueCreateInfo> queuesys_vk::analize_queues(gpu_public* vkgpu) {
	std::vector<VkDeviceQueueCreateInfo> queuefam_ci(vkgpu->QUEUEFAMSCOUNT());


	
	/*

	std::vector<VkDeviceQueueCreateInfo> QueueCreationInfos(0);
	//Queue Creation Processes
	float QueuePriority = 1.0f;
	for (unsigned int QueueIndex = 0; QueueIndex < VKGPU->QUEUEs.size(); QueueIndex++) {
		queue_vk& QUEUE = VKGPU->QUEUEs[QueueIndex];
		VkDeviceQueueCreateInfo QueueInfo = {};
		QueueInfo.flags = 0;
		QueueInfo.pNext = nullptr;
		QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueInfo.queueFamilyIndex = QUEUE.QueueFamilyIndex;
		QueueInfo.pQueuePriorities = &QueuePriority;
		QueueInfo.queueCount = 1;
		QueueCreationInfos.push_back(QueueInfo);
	}

	bool is_presentationfound = false;
	for (unsigned int queuefamily_index = 0; queuefamily_index < vkgpu->QUEUEFAMSCOUNT(); queuefamily_index++) {
		VkQueueFamilyProperties* QueueFamily = &vkgpu->QUEUEFAMILYPROPERTIES()[queuefamily_index];
		queue_vk VKQUEUE;
		VKQUEUE.QueueFamilyIndex = static_cast<uint32_t>(queuefamily_index);
		if (QueueFamily->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			vkgpu->desc.is_GraphicOperations_Supported = true;
			VKQUEUE.SupportFlag.is_GRAPHICSsupported = true;
			VKQUEUE.QueueFeatureScore++;
		}
		if (QueueFamily->queueFlags & VK_QUEUE_COMPUTE_BIT) {
			vkgpu->desc.is_ComputeOperations_Supported = true;
			VKQUEUE.SupportFlag.is_COMPUTEsupported = true;
			vkgpu->COMPUTE_supportedqueuecount++;
			VKQUEUE.QueueFeatureScore++;
		}
		if (QueueFamily->queueFlags & VK_QUEUE_TRANSFER_BIT) {
			vkgpu->desc.is_TransferOperations_Supported = true;
			VKQUEUE.SupportFlag.is_TRANSFERsupported = true;
			vkgpu->TRANSFERs_supportedqueuecount++;
			VKQUEUE.QueueFeatureScore++;
		}

		vkgpu->QUEUEs.push_back(VKQUEUE);
		if (VKQUEUE.SupportFlag.is_GRAPHICSsupported) {
			vkgpu->GRAPHICS_QUEUEIndex = vkgpu->QUEUEs.size() - 1;
		}
	}
	if (!GPUdesc.is_GraphicOperations_Supported || !GPUdesc.is_TransferOperations_Supported || !GPUdesc.is_ComputeOperations_Supported) {
		LOG_CRASHING_TAPI("The GPU doesn't support one of the following operations, so we can't let you use this GPU: Compute, Transfer, Graphics");
		return;
	}
	//Sort the queues by their feature count (Example: Element 0 is Transfer Only, Element 1 is Transfer-Compute, Element 2 is Graphics-Transfer-Compute etc)
	//QuickSort Algorithm
	if (vkgpu->QUEUEs.size()) {
		bool should_Sort = true;
		while (should_Sort) {
			should_Sort = false;
			for (unsigned char QueueIndex = 0; QueueIndex < vkgpu->QUEUEs.size() - 1; QueueIndex++) {
				if (vkgpu->QUEUEs[QueueIndex + 1].QueueFeatureScore < vkgpu->QUEUEs[QueueIndex].QueueFeatureScore) {
					should_Sort = true;
					VK_QUEUE SecondQueue;
					SecondQueue = vkgpu->QUEUEs[QueueIndex + 1];
					vkgpu->QUEUEs[QueueIndex + 1] = vkgpu->QUEUEs[QueueIndex];
					vkgpu->QUEUEs[QueueIndex] = SecondQueue;
				}
			}
		}
	}*/
	return queuefam_ci;
}