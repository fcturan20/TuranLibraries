#include "predefinitions_vk.h"
#include <vector>

struct commandpool_vk;
struct commandbuffer_vk;
struct queuesys_data;
#ifdef VULKAN_DEBUGGING
typedef unsigned int CommandBufferIDType;
static constexpr CommandBufferIDType INVALID_CommandBufferID = UINT32_MAX;
#else
typedef commandbuffer_vk* CommandBufferIDType;
static constexpr CommandBufferIDType INVALID_CommandBufferID = nullptr;
#endif


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

struct queuesys_vk {
	//Analize queues to fill gpu description
	void analize_queues(gpu_public* vkgpu);
	//While creating VK Logical Device, we need which queues to create. Get that info from here.
	std::vector<VkDeviceQueueCreateInfo> get_queue_cis(gpu_public* vkgpu);
	//Get VkQueue objects from logical device
	void get_queue_objects(gpu_public* vkgpu);
	commandbuffer_vk* get_commandbuffer(gpu_public* vkgpu, queuefam_vk* family);
	uint32_t get_queuefam_index(gpu_public* vkgpu, queuefam_vk* fam);
	bool check_windowsupport(gpu_public* vkgpu, VkSurfaceKHR surface);
	queuesys_data* data;
};