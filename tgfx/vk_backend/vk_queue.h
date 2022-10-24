#pragma once
#include <vector>

#include "vk_core.h"
#include "vk_predefinitions.h"

vk_uint32c                    VKCONST_MAXSUBMITCOUNT = 32, VKCONST_MAXQUEUEFENCECOUNT = 32;
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

  inline bool isFlagValid() const {
    if (doesntNeedAnything &&
        (is_GRAPHICSsupported || is_COMPUTEsupported || is_TRANSFERsupported)) {
      printer(
        result_tgfx_FAIL,
        "(This flag doesn't need anything but it also needs something, this shouldn't happen!");
      return false;
    }
    if (!doesntNeedAnything && !is_GRAPHICSsupported && !is_COMPUTEsupported &&
        !is_TRANSFERsupported) {
      printer(result_tgfx_FAIL, "(This flag needs something but it doesn't support anything");
      return false;
    }
    return true;
  }

  operator uint8_t() const;
};
struct cmdBufferPrim_vk;
struct cmdPool_vk;

struct submit_vk {
  bool            isALIVE    = false;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::INTERNAL;
  static uint16_t GET_EXTRAFLAGS(submit_vk* obj) { return 0; }

  VkSubmitInfo                  vk_submit        = {};
  VkTimelineSemaphoreSubmitInfo vk_semaphoreInfo = {};
  VkPresentInfoKHR              vk_present       = {};
  VkBindSparseInfo              vk_bindSparse    = {};

  cmdBufferPrim_vk* cmdBuffers[VKCONST_MAXCMDBUFFERCOUNT_PERSUBMIT] = {};
  uint32_t          cmdBufferCount                                  = 0;

  VkSemaphore vk_signalSemaphores[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {},
              vk_waitSemaphores[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT]   = {};

  uint64_t vk_signalSemaphoreValues[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {},
           vk_waitSemaphoreValues[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT]   = {};

  WINDOW_VKOBJ* m_windows[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {};
};

struct submission_vk {
  bool            isALIVE    = false;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::INTERNAL;
  static uint16_t GET_EXTRAFLAGS(submit_vk* obj) { return 0; }

  VkFence           vk_fence                                                             = {};
  cmdBufferPrim_vk* vk_cbs[VKCONST_MAXCMDBUFFERCOUNT_PERSUBMIT * VKCONST_MAXSUBMITCOUNT] = {};
};

// Handle both has GPU's ID & QueueFamily's ID
struct QUEUEFAM_VK;
struct QUEUE_VKOBJ {
  bool            isALIVE    = false;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::GPUQUEUE;
  static uint16_t GET_EXTRAFLAGS(QUEUE_VKOBJ* obj) {
    return (obj->m_gpu->gpuIndx() << 8) | (obj->vk_queueFamIndex);
  }
  static GPU_VKOBJ*   getGPUfromHandle(gpuQueue_tgfxhnd hnd);
  static QUEUEFAM_VK* getFAMfromHandle(gpuQueue_tgfxhnd hnd);

  uint32_t vk_queueFamIndex = 0;
  VkQueue  vk_queue;

  enum vk_queueOpType : uint8_t { ERROR_QUEUEOPTYPE = 0, CMDBUFFER = 1, PRESENT = 2, SPARSE = 3 };
  vk_queueOpType m_activeQueueOp = ERROR_QUEUEOPTYPE, m_prevQueueOp = ERROR_QUEUEOPTYPE;
  GPU_VKOBJ*     m_gpu = nullptr;
  // Operations that wait to be sent to GPU
  VK_STATICVECTOR<submit_vk, void*, VKCONST_MAXSUBMITCOUNT> m_submitInfos;
  // This is a binary semaphore to sync sequential executeCmdBufferList calls in the same queue
  // (DX12 way)
  VkSemaphore                                                       vk_callSynchronizer = {};
  VK_STATICVECTOR<submission_vk, void*, VKCONST_MAXQUEUEFENCECOUNT> vk_submitTracker;

  QUEUE_VKOBJ& operator=(const QUEUE_VKOBJ& src) {
    vk_queue         = src.vk_queue;
    vk_queueFamIndex = src.vk_queueFamIndex;
    return *this;
  }
};

struct QUEUEFAM_VK {
  bool            isALIVE    = false;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::INTERNAL;
  static uint16_t GET_EXTRAFLAGS(QUEUEFAM_VK* obj) {
    return (obj->m_gpu->gpuIndx() << 8) | (obj->m_supportFlag);
  }

  queueflag_vk m_supportFlag    = {};
  uint32_t     vk_queueFamIndex = 0;
  GPU_VKOBJ*   m_gpu            = nullptr;
  cmdPool_vk*  m_pools          = nullptr;

  VK_STATICVECTOR<QUEUE_VKOBJ, gpuQueue_tgfxhnd, VKCONST_MAXQUEUECOUNT_PERFAM> m_queues;

  QUEUEFAM_VK& operator=(const QUEUEFAM_VK& src) {
    isALIVE = true;
    m_gpu   = src.m_gpu;
    m_queues.clear();
    return *this;
  }
};
/*
  This class manages queues, command buffer and descriptor set allocations per GPU
    This is important in multi-threaded cases because;
  1) You can't use the same command pool from different threads at the same time
  2) You can't allocate-free descriptor sets from the same descPool at the same time
      But you can write to them, so no issue except descriptor count is limited by hardware
  3) Command buffers should be tracked as if their execution has done, free command buffer
  4) Execution tracking is achieved by tracking queues, fences etc. So queues should be managed
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
  queueCreateInfoList get_queue_cis() const;
  // Get VkQueue objects from logical device
  void get_queue_objects();
  // Searches for an available command buffer; if not found -> create one
  VkCommandBuffer getPrimaryCmdBuffer(QUEUEFAM_VK* family);
  // Submit queue operations to GPU
  // Adds the queue's binary semaphore to the first&last submit to synchronize queue submissions.
  // This is because some queue operations are not synchronized by Vulkan (present and sparse).
  // NOTE: Queue's synchronizer binary semaphore is named : "QueueBinSem aka QBS".
  // For ex: Present 1 -> CmdBufferSubmitList (Submits A, B & C) -> Present 2.
  //  Present 1 signals QueueBinSem. SubmitA waits (un-signals) QBS. SubmitC signals QBS. Present 2
  //  both waits then signals QBS. With this way, we're

  void               queueSubmit(QUEUE_VKOBJ* family);
  uint32_t           get_queuefam_index(QUEUE_VKOBJ* fam);
  bool               does_queuefamily_support(QUEUE_VKOBJ* family, const queueflag_vk& flag);
  static manager_vk* createManager(GPU_VKOBJ* gpu);

 public:
  GPU_VKOBJ* m_gpu;
  // Internal queue will be used to implement timeline <-> binary semaphore conversions
  QUEUE_VKOBJ* m_internalQueue;

  VK_STATICVECTOR<QUEUEFAM_VK, gpuQueue_tgfxhnd, VKCONST_MAXQUEUEFAMCOUNT_PERGPU> m_queueFams;
};

void createCmdBuffer(QUEUEFAM_VK* queueFam, VkCommandBufferLevel level, VkCommandBuffer* cbs, uint32_t count);