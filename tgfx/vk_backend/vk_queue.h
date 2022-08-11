#pragma once
#include "vk_predefinitions.h"
#include <vector>

struct commandpool_vk;
struct commandbuffer_vk;
struct queuesys_data;


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
	inline queueflag_vk(const queueflag_vk& copy) {
		doesntNeedAnything = copy.doesntNeedAnything; is_GRAPHICSsupported = copy.is_GRAPHICSsupported; is_PRESENTATIONsupported = copy.is_PRESENTATIONsupported; is_COMPUTEsupported = copy.is_COMPUTEsupported; is_TRANSFERsupported = copy.is_TRANSFERsupported;
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

struct queuesys_vk {
	queuesys_data* data;
	//Analize queues to fill gpu description
	void analize_queues(GPU_VKOBJ* vkgpu);
	//While creating VK Logical Device, we need which queues to create. Get that info from here.
	std::vector<VkDeviceQueueCreateInfo> get_queue_cis(GPU_VKOBJ* vkgpu);
	//Get VkQueue objects from logical device
	void get_queue_objects(GPU_VKOBJ* vkgpu);
	//Searches for an available command buffer, if not found creates one
	commandbuffer_vk* get_commandbuffer(GPU_VKOBJ* vkgpu, queuefam_vk* family, unsigned char FrameIndex);
	VkCommandBuffer get_commandbufferobj(commandbuffer_vk* id);
	fence_idtype_vk queueSubmit(GPU_VKOBJ* vkgpu, queuefam_vk* family, const std::vector<semaphore_idtype_vk>& WaitSemaphores, const std::vector<semaphore_idtype_vk>& SignalSemaphores, const VkCommandBuffer* commandbuffers, const VkPipelineStageFlags* cb_flags, unsigned int CBCount);
	uint32_t get_queuefam_index(GPU_VKOBJ* vkgpu, queuefam_vk* fam);
	queuefam_vk* check_windowsupport(GPU_VKOBJ* vkgpu, VkSurfaceKHR surface);
	bool does_queuefamily_support(GPU_VKOBJ* vkgpu, queuefam_vk* family, const queueflag_vk& flag);
	//Use this for 3rd party libraries (like dear ImGui)
	VkQueue get_queue(GPU_VKOBJ* vkgpu, queuefam_vk* queuefam);
};