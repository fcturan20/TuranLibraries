#pragma once
#include <vector>

#include "vk_core.h"
#include "vk_predefinitions.h"
#include "vk_resource.h"

extern vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_MANAGER;
// Initializes as everything is false (same as CreateInvalidNullFlag)
struct queueflag_vk {
  bool is_GRAPHICSsupported : 1;
  bool is_COMPUTEsupported : 1;
  bool is_TRANSFERsupported : 1;
  bool doesntNeedAnything : 1; // This is a special flag to be used as "Don't care other parameters,
                               // this is a special operation"
  // bool is_VTMEMsupported : 1;	Not supported for now!
  inline queueflag_vk() {
    doesntNeedAnything   = false;
    is_GRAPHICSsupported = false;
    is_COMPUTEsupported  = false;
    is_TRANSFERsupported = false;
  }

  inline queueflag_vk(const queueflag_vk& copy) {
    doesntNeedAnything   = copy.doesntNeedAnything;
    is_GRAPHICSsupported = copy.is_GRAPHICSsupported;
    is_COMPUTEsupported  = copy.is_COMPUTEsupported;
    is_TRANSFERsupported = copy.is_TRANSFERsupported;
  }
  // Returned flag's every bit is false. You should set at least one of them as true.
  inline static queueflag_vk CreateInvalidNullFlag() { return queueflag_vk(); }
  static constexpr wchar_t*  VKCONST_FLAG_INVALID_ERROR_TEXT = L"Some inner flag is invalid";
  inline bool                isFlagValid() const {
    if (doesntNeedAnything &&
        (is_GRAPHICSsupported || is_COMPUTEsupported || is_TRANSFERsupported)) {
      vkPrint(16, VKCONST_FLAG_INVALID_ERROR_TEXT);
      return false;
    }
    if (!doesntNeedAnything && !is_GRAPHICSsupported && !is_COMPUTEsupported &&
        !is_TRANSFERsupported) {
      vkPrint(16, VKCONST_FLAG_INVALID_ERROR_TEXT);
      return false;
    }
    return true;
  }

  operator uint8_t() const;
};
struct CMDBUFFER_VKOBJ;
struct cmdPool_vk;

typedef void (*vk_submissionCallback)(GPU_VKOBJ* gpu, VkFence fence, void* userData);
// Handle both has GPU's ID & QueueFamily's ID
struct QUEUEFAM_VK;
struct QUEUE_VKOBJ;
/*
This class manages queues and command buffer allocations per GPU
  This is important in multi-threaded cases because;
1) You can't use the same command pool from different threads at the same time
2) Command buffers should be tracked as if their execution has done, free command buffer
3) Execution tracking is achieved by tracking queues, fences etc. So queues should be managed
    here too
NOTE: User-side command buffers are different because they're not actual calls (sorting
is needed) So after sorting, thread calls poolManager to get command buffer. poolManager find
available command pool, if there isn't then creates. Then attaches command pool to queue, so if
queue finishes executing then command pool is freed. *POSSIBILITY: Maybe no need to bind command
pools, just bind command buffers to queue.
  *  While creating cmdbuffers, search all previous pools and select the one with
      no actively recorded-executed cmdbuffer. Because pool creation-destruction is costly.
*/
struct manager_vk {
 public:
  struct queueCreateInfoList {
    VkDeviceQueueCreateInfo list[VKCONST_MAXQUEUEFAMCOUNT_PERGPU];
  };
  // While creating VK Logical Device, we need which queues to create. Get that info from here.
  queueCreateInfoList get_queue_cis(GPU_VKOBJ* gpu) const;
  // Get VkQueue objects from logical device
  void get_queue_objects(GPU_VKOBJ* gpu);
  // Submit queue operations to GPU
  // Adds the queue's binary semaphore to the first&last submit to synchronize queue submissions.
  // This is because some queue operations are not synchronized by Vulkan (present and sparse).
  // NOTE: Queue's synchronizer binary semaphore is named : "QueueBinSem aka QBS".
  // For ex: Present 1 -> CmdBufferSubmitList (Submits A, B & C) -> Present 2.
  //  Present 1 signals QueueBinSem. SubmitA waits (un-signals) QBS. SubmitC signals QBS. Present 2
  //  both waits then signals QBS. With this way, we're

  void     queueSubmit(QUEUE_VKOBJ* family);
  uint32_t get_queuefam_index(QUEUE_VKOBJ* fam);
  bool     does_queuefamily_support(QUEUE_VKOBJ* family, const queueflag_vk& flag);
};

void vk_allocateCmdBuffer(QUEUEFAM_VK* queueFam, VkCommandBufferLevel level, cmdPool_vk*& cmdPool,
                          VkCommandBuffer* cbs, uint32_t count);
void vk_freeCmdBuffer(cmdPool_vk* cmdPool, VkCommandBuffer cb);
VkQueue      getQueueVkObj(QUEUE_VKOBJ* queue);
QUEUE_VKOBJ* getQueue(gpuQueue_tgfxhnd hnd);
QUEUEFAM_VK* getQueueFam(GPU_VKOBJ* gpu, unsigned int queueFamIndx);
QUEUE_VKOBJ* getQueue(QUEUEFAM_VK* queueFam, uint32_t queueIndx);
// Extension: QueueOwnershipTransfer
// Use o_ params with uint32_t queueFamList[VKCONST_MAXQUEUEFAMCOUNT] etc.
void VK_getQueueAndSharingInfos(unsigned int queueList, const gpuQueue_tgfxhnd* i_queueList,
                                unsigned int extCount, const extension_tgfxhnd* i_exts,
                                uint32_t* o_famList, uint32_t* o_famListSize,
                                VkSharingMode* o_sharingMode);
void vk_getWindowSupportedQueues(GPU_VKOBJ* GPU, WINDOW_VKOBJ* window, windowGPUsupport_tgfx* info);