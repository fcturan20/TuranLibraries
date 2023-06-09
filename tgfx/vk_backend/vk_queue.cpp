/*
  This file is responsible for managing TGFX Command Buffer sending queue operations to GPU and.
  TGFX Command Buffer = Primary Command Buffer.
  TGFX GPU QUEUE = VkQueue

*/

#include "vk_queue.h"

#include <threadingsys_tapi.h>

#include <mutex>

#include "tgfx_helper.h"
#include "tgfx_renderer.h"
#include "vk_contentmanager.h"
#include "vk_core.h"
#include "vk_renderer.h"
#include "vkext_dynamic_rendering.h"
#include "vkext_timelineSemaphore.h"

manager_vk*            manager                         = nullptr;
vk_uint32c             VKCONST_MAXFENCECOUNT_PERSUBMIT = 8, VKCONST_MAXCMDBUFFER_PRIMARY_COUNT = 32;
vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_MANAGER = nullptr;

struct submission_vk {
  vk_handleType HANDLETYPE = VKHANDLETYPEs::INTERNAL;

  VkFence               vk_fence   = nullptr;
  GPU_VKOBJ*            m_gpu      = nullptr;
  void*                 m_userData = nullptr;
  vk_submissionCallback m_callback = nullptr;
  uint32_t              queueIndx  = UINT32_MAX;
};

enum vk_queueOpType : uint8_t { ERROR_QUEUEOPTYPE = 0, CMDBUFFER = 1, PRESENT = 2, SPARSE = 3 };
struct submit_vk {
  vk_queueOpType type = CMDBUFFER;
  union {
    VkSubmitInfo     vk_submit;
    VkPresentInfoKHR vk_present;
  };

  uint32_t signalSemaphoreCount = 0, waitSemaphoreCount = 0, cmdBufferCount = 0, windowCount = 0;
  struct tgfx_commandBuffer** cmdBuffers = {};

  VkSemaphore *vk_signalSemaphores = {}, *vk_waitSemaphores = {};

  uint64_t *                    vk_signalSemaphoreValues = {}, *vk_waitSemaphoreValues = {};
  VkTimelineSemaphoreSubmitInfo vk_semaphoreInfo;

  WINDOW_VKOBJ** m_windows = {};
  // Allocates enough memory and sets pointers to valid arrays (waiting to be filled)
  static submit_vk* allocateSubmit(uint64_t i_signalSemaphoreCount, uint64_t i_waitSemaphoreCount,
                                   uint64_t i_cmdBufferCount, uint32_t i_windowCount) {
    uint32_t allocSize = sizeof(submit_vk) +
                         (sizeof(void*) * (i_signalSemaphoreCount + i_waitSemaphoreCount +
                                           i_cmdBufferCount + i_windowCount + 4)) +
                         (sizeof(uint64_t) * (i_signalSemaphoreCount + i_waitSemaphoreCount));
    submit_vk* submit = ( submit_vk* )VK_MEMOFFSET_TO_POINTER(vk_virmem::allocatePage(allocSize));
    virmemsys->virtual_commit(submit, allocSize);
    submit->signalSemaphoreCount = i_signalSemaphoreCount;
    submit->waitSemaphoreCount   = i_waitSemaphoreCount;
    submit->cmdBufferCount       = i_cmdBufferCount;
    submit->windowCount          = i_windowCount;

    uintptr_t lastPos = uintptr_t(submit + 1);
    // Allocate cmd buffer object list
    {
      submit->cmdBuffers = ( struct tgfx_commandBuffer** )lastPos;
      lastPos += sizeof(CMDBUFFER_VKOBJ**) * (submit->cmdBufferCount + 1ull);
    }
    // Allocate signal semaphore list
    {
      submit->vk_signalSemaphores = ( VkSemaphore* )lastPos;
      lastPos += sizeof(VkSemaphore) * (submit->signalSemaphoreCount + 1ull);
      submit->vk_signalSemaphoreValues = ( uint64_t* )lastPos;
      lastPos += sizeof(uint64_t) * (submit->signalSemaphoreCount + 1ull);
    }
    // Allocate wait semaphore list
    {
      submit->vk_waitSemaphores = ( VkSemaphore* )lastPos;
      lastPos += sizeof(VkSemaphore) * (submit->waitSemaphoreCount + 1ull);
      submit->vk_waitSemaphoreValues = ( uint64_t* )lastPos;
      lastPos += sizeof(uint64_t) * (submit->waitSemaphoreCount + 1ull);
    }
    // Allocate window list
    {
      submit->m_windows = ( WINDOW_VKOBJ** )lastPos;
      lastPos += sizeof(WINDOW_VKOBJ*) * (submit->windowCount + 1ull);
    }
    return submit;
  }
};
vk_uint32c VKCONST_MAXUNSENTSUBMITCOUNT = 32;
struct QUEUE_VKOBJ {
  bool            isALIVE    = false;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::GPUQUEUE;
  static uint16_t GET_EXTRAFLAGS(QUEUE_VKOBJ* obj) {
    return (obj->m_gpu->gpuIndx() << 8) | (obj->vk_queueFamIndex);
  }
  static GPU_VKOBJ*   getGPUfromHandle(struct tgfx_gpuQueue* hnd);
  static QUEUEFAM_VK* getFAMfromHandle(struct tgfx_gpuQueue* hnd);

  uint32_t vk_queueFamIndex = 0, m_queueIndx = 0;
  VkQueue  vk_queue;

  vk_queueOpType m_activeQueueOp = ERROR_QUEUEOPTYPE, m_prevQueueOp = ERROR_QUEUEOPTYPE;
  GPU_VKOBJ*     m_gpu = nullptr;
  // This is a binary semaphore to sync sequential executeCmdBufferList calls in the same queue
  // (DX12 way)
  VkSemaphore                                       vk_callSynchronizer = {};
  submit_vk*                                        m_unsentSubmits[VKCONST_MAXUNSENTSUBMITCOUNT];
  VK_LINEAR_OBJARRAY<submission_vk, void*, 1 << 10> m_submissions;
  uint32_t                                          sizeUnsetSubmits();

  // Check previously sent submissions
  void checkSubmissions();
  void createSubmission(VkFence fence, void* data, vk_submissionCallback callback);
};

struct QUEUEFAM_VK {
  bool            isALIVE    = false;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::INTERNAL;
  static uint16_t GET_EXTRAFLAGS(QUEUEFAM_VK* obj) {
    return (obj->m_gpu->gpuIndx() << 8) | (obj->m_supportFlag);
  }

  queueflag_vk m_supportFlag    = {};
  uint32_t     vk_queueFamIndex = 0, m_queueListStartIndx = 0;
  GPU_VKOBJ*   m_gpu   = nullptr;
  cmdPool_vk*  m_pools = nullptr;
};
queueflag_vk::operator uint8_t() const {
  if constexpr (sizeof(queueflag_vk) == 1) {
    return *(( uint8_t* )this);
  } else {
    vkPrint(16, L"Backend: queueflag_vk shouldn't be more than 1 byte");
  }
  return 0;
};
inline void vk_getGPUInfoQueues(struct tgfx_gpu* GPUhnd, unsigned int queueFamIndx,
                                unsigned int* queueCount, struct tgfx_gpuQueue** queueList) {
  GPU_VKOBJ*   VKGPU = getOBJ<GPU_VKOBJ>(GPUhnd);
  QUEUEFAM_VK* fam   = getQueueFam(VKGPU, queueFamIndx);
  if (*queueCount == VKGPU->vk_propsQueue[queueFamIndx].queueFamilyProperties.queueCount) {
    for (uint32_t i = 0; i < *queueCount; i++) {
      queueList[i] = getHANDLE<struct tgfx_gpuQueue*>(getQueue(fam, i));
    }
  }
  *queueCount = VKGPU->vk_propsQueue[queueFamIndx].queueFamilyProperties.queueCount;
}
void vk_getWindowSupportedQueues(GPU_VKOBJ* GPU, WINDOW_VKOBJ* window,
                                 tgfx_windowGPUsupport* info) {
  uint32_t supportedQueueCount = 0;
  for (uint32_t queueFamIndx = 0; queueFamIndx < GPU->desc.queueFamilyCount; queueFamIndx++) {
    VkBool32 isSupported = false;
    if (vkGetPhysicalDeviceSurfaceSupportKHR(GPU->vk_physical, queueFamIndx, window->vk_surface,
                                             &isSupported) != VK_SUCCESS) {
      vkPrint(50, L"at vkGetPhysicalDeviceSurfaceSupportKHR");
    }
    QUEUEFAM_VK*   queueFam   = getQueueFam(GPU, queueFamIndx);
    const uint32_t queueCount = GPU->vk_propsQueue[queueFamIndx].queueFamilyProperties.queueCount;
    if (isSupported) {
      for (uint32_t queueIndx = 0;
           queueIndx < queueCount && supportedQueueCount < TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT;
           queueIndx++) {
        info->queues[supportedQueueCount++] =
          getHANDLE<struct tgfx_gpuQueue*>(getQueue(queueFam, queueIndx));
      }
    }
  }
  if (supportedQueueCount < TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT) {
    info->queues[supportedQueueCount] = nullptr;
  }
}
struct CMDBUFFER_VKOBJ {
  vk_handleType     HANDLETYPE = VKHANDLETYPEs::CMDBUFFER;
  static uint16_t   GET_EXTRAFLAGS(CMDBUFFER_VKOBJ* obj) { return uint16_t(obj->m_gpuIndx) << 8; }
  static GPU_VKOBJ* getGPUfromHandle(struct tgfx_commandBuffer* hnd) {
    VKOBJHANDLE handle  = *( VKOBJHANDLE* )&hnd;
    uint32_t    gpuIndx = handle.EXTRA_FLAGs >> 8;
  }
  VkCommandBuffer vk_cb = {};
  VkCommandPool   vk_cp{};
  uint8_t         m_gpuIndx = 0, m_queueFamIndx = 0;
};

struct manager_vk_private {
  VK_ARRAY<QUEUEFAM_VK, void*>                               m_queueFams;
  VK_ARRAY<QUEUE_VKOBJ, struct tgfx_gpuQueue*>                    m_queues;
  VK_LINEAR_OBJARRAY<CMDBUFFER_VKOBJ, struct tgfx_commandBuffer*> m_cmdBuffers;
};
manager_vk_private* mngrPriv = nullptr;
void                vk_createManager() {
  if (!VKGLOBAL_VIRMEM_MANAGER) {
    // 16MB is enough I guess?
    VKGLOBAL_VIRMEM_MANAGER = vk_virmem::allocate_dynamicmem(1 << 25);
  }
  manager  = new (VKGLOBAL_VIRMEM_MANAGER) manager_vk;
  mngrPriv = new (VKGLOBAL_VIRMEM_MANAGER) manager_vk_private;

  uint32_t totalQueueFamCount = 0, totalQueueCount = 0;
  for (uint32_t gpuIndx = 0; gpuIndx < core_vk->gpuCount(); gpuIndx++) {
    totalQueueFamCount;
    GPU_VKOBJ* gpu                = core_vk->getGPU(gpuIndx);
    uint32_t   queueFamiliesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(gpu->vk_physical, &queueFamiliesCount, nullptr);
#ifdef VULKAN_DEBUGGING
    if (VKCONST_MAXQUEUEFAMCOUNT_PERGPU < queueFamiliesCount) {
      vkPrint(16, L"VKCONST_MAXQUEUEFAMCOUNT_PERGPU is exceeded, please report this!");
    }
#endif
    for (uint32_t i = 0; i < VKCONST_MAXQUEUEFAMCOUNT_PERGPU; i++) {
      gpu->vk_propsQueue[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
      gpu->vk_propsQueue[i].pNext = nullptr;
    }
    vkGetPhysicalDeviceQueueFamilyProperties2(gpu->vk_physical, &queueFamiliesCount,
                                              gpu->vk_propsQueue);

    gpu->desc.queueFamilyCount = queueFamiliesCount;
    for (uint32_t queueFamIndx = 0; queueFamIndx < VKCONST_MAXQUEUEFAMCOUNT_PERGPU;
         queueFamIndx++) {
      totalQueueCount += gpu->vk_propsQueue[queueFamIndx].queueFamilyProperties.queueCount;
      gpu->m_queueFamPtrs[queueFamIndx] = totalQueueFamCount + queueFamIndx;
    }
    totalQueueFamCount += queueFamiliesCount;
  }
  mngrPriv->m_queueFams.init(new (VKGLOBAL_VIRMEM_MANAGER) QUEUEFAM_VK[totalQueueFamCount],
                             totalQueueFamCount);
  mngrPriv->m_queues.init(new (VKGLOBAL_VIRMEM_MANAGER) QUEUE_VKOBJ[totalQueueCount],
                          totalQueueCount);

  totalQueueFamCount = 0, totalQueueCount = 0;
  for (uint32_t gpuIndx = 0; gpuIndx < core_vk->gpuCount(); gpuIndx++) {
    GPU_VKOBJ*   gpu          = core_vk->getGPU(gpuIndx);
    QUEUEFAM_VK* bestQueueFam = nullptr;
    for (uint32_t queueFamIndx = 0; queueFamIndx < gpu->desc.queueFamilyCount; queueFamIndx++) {
      VkQueueFamilyProperties2* props    = &gpu->vk_propsQueue[queueFamIndx];
      QUEUEFAM_VK*              queueFam = mngrPriv->m_queueFams[totalQueueFamCount++];

      queueFam->vk_queueFamIndex = queueFamIndx;
      if (props->queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        gpu->desc.operationSupport_raster            = true;
        queueFam->m_supportFlag.is_GRAPHICSsupported = true;
      }
      if (props->queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) {
        gpu->desc.operationSupport_compute          = true;
        queueFam->m_supportFlag.is_COMPUTEsupported = true;
      }
      if (props->queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) {
        gpu->desc.operationSupport_transfer          = true;
        queueFam->m_supportFlag.is_TRANSFERsupported = true;
      }
      queueFam->m_gpu = gpu;

      // FIND BEST QUEUE FAM
      // Select internal queue, then create other queue backend objects

      if (!bestQueueFam) {
        bestQueueFam = queueFam;
        continue;
      }
      const VkQueueFamilyProperties& bestQueueProps =
        gpu->vk_propsQueue[bestQueueFam->vk_queueFamIndex].queueFamilyProperties;

      if (props->queueFamilyProperties.queueCount > bestQueueProps.queueCount) {
        bestQueueFam = queueFam;
        continue;
      }

      // Bigger values in VkQueueFlags means more useful the queue gets
      // For internal operations, we need a queue as useless as possible.
      // For example: 4 is tranfer, 8 is sparse, 16 is protected bits which are supported by all
      //  queues if device supports. Main difference between is generally in Graphics and Compute
      //  bits. So Graphics & Compute bits are main usefulness detector.
      if (props->queueFamilyProperties.queueFlags < bestQueueProps.queueFlags &&
          props->queueFamilyProperties.queueCount > 1) {
        bestQueueFam = queueFam;
        continue;
      }
    }
    if (!gpu->desc.operationSupport_raster || !gpu->desc.operationSupport_transfer ||
        !gpu->desc.operationSupport_compute) {
      vkPrint(51, gpu->desc.name);
      continue;
    }

    if (gpu->vk_propsQueue[bestQueueFam->vk_queueFamIndex].queueFamilyProperties.queueCount > 1) {
      gpu->vk_propsQueue[bestQueueFam->vk_queueFamIndex].queueFamilyProperties.queueCount--;

      // Create internal queue
      {
        QUEUE_VKOBJ* internalQueue      = new (VKGLOBAL_VIRMEM_MANAGER) QUEUE_VKOBJ;
        gpu->m_internalQueue            = getHANDLE<struct tgfx_gpuQueue*>(internalQueue);
        internalQueue->m_queueIndx      = 0;
        internalQueue->m_gpu            = gpu;
        internalQueue->vk_queueFamIndex = bestQueueFam->vk_queueFamIndex;
        internalQueue->vk_queue         = VK_NULL_HANDLE;
        internalQueue->isALIVE          = true;
      }
    } else {
      // First queue will be internal queue
      gpu->m_internalQueue = getHANDLE<struct tgfx_gpuQueue*>(mngrPriv->m_queues[totalQueueCount]);
    }

    // Create user queues
    for (uint32_t queueFamIndx = 0; queueFamIndx < gpu->desc.queueFamilyCount; queueFamIndx++) {
      const VkQueueFamilyProperties& props = gpu->vk_propsQueue[queueFamIndx].queueFamilyProperties;
      QUEUEFAM_VK*                   queueFam = getQueueFam(gpu, queueFamIndx);
      queueFam->m_queueListStartIndx          = totalQueueCount;

      for (uint32_t i = 0; i < props.queueCount; i++) {
        QUEUE_VKOBJ* queue      = mngrPriv->m_queues[totalQueueCount++];
        queue->m_gpu            = gpu;
        queue->m_queueIndx      = i;
        queue->vk_queueFamIndex = queueFamIndx;
        queue->vk_queue         = VK_NULL_HANDLE;
      }
    }
  }
}
VkQueue      getQueueVkObj(QUEUE_VKOBJ* queue) { return queue->vk_queue; }
QUEUE_VKOBJ* getQueue(struct tgfx_gpuQueue* hnd) { return getOBJ<QUEUE_VKOBJ>(hnd); }
QUEUEFAM_VK* getQueueFam(GPU_VKOBJ* gpu, unsigned int queueFamIndx) {
  return mngrPriv->m_queueFams[gpu->m_queueFamPtrs[queueFamIndx]];
}
QUEUE_VKOBJ* getQueue(QUEUEFAM_VK* queueFam, uint32_t queueIndx) {
  return mngrPriv->m_queues[queueFam->m_queueListStartIndx + queueIndx];
}

struct cmdPool_vk {
  cmdPool_vk()                 = default;
  VkCommandPool vk_secondaryCP = VK_NULL_HANDLE;
  QUEUEFAM_VK*  m_queueFam     = nullptr;
  std::mutex    m_secondarySync;
};
GPU_VKOBJ* QUEUE_VKOBJ::getGPUfromHandle(struct tgfx_gpuQueue* hnd) {
#ifdef VK_USE_STD
  QUEUE_VKOBJ* o = getOBJ<QUEUE_VKOBJ>(hnd);
  return o->m_gpu;
#else
  VKOBJHANDLE handle  = *( VKOBJHANDLE* )&hnd;
  uint32_t    gpuIndx = handle.EXTRA_FLAGs >> 8;
  return core_vk->getGPU(gpuIndx);
#endif
}
QUEUEFAM_VK* QUEUE_VKOBJ::getFAMfromHandle(struct tgfx_gpuQueue* hnd) {
#ifdef VK_USE_STD
  QUEUE_VKOBJ* o = getOBJ<QUEUE_VKOBJ>(hnd);
  return mngrPriv->m_queueFams[o->vk_queueFamIndex];
#else
  GPU_VKOBJ*  gpu          = getGPUfromHandle(hnd);
  VKOBJHANDLE handle       = *( VKOBJHANDLE* )&hnd;
  uint32_t    queueFamIndx = (uint32_t(handle.EXTRA_FLAGs) << 24) >> 24;
  return mngrPriv->m_queueFams[queueFamIndx];
#endif
}

vk_uint32c   queueCountMax                                   = 256;
static float VKCONST_DEFAULT_QUEUE_PRIORITIES[queueCountMax] = {};
// While creating VK Logical Device, we need which queues to create. Get that info from here.
manager_vk::queueCreateInfoList manager_vk::get_queue_cis(GPU_VKOBJ* gpu) const {
  for (uint32_t i = 0; i < queueCountMax; i++) {
    VKCONST_DEFAULT_QUEUE_PRIORITIES[i] = std::min(float(i) / queueCountMax, 1.0f);
  }
  queueCreateInfoList QueueCreationInfos;
  // Queue Creation Processes
  for (unsigned int queueFamIndx = 0; queueFamIndx < gpu->desc.queueFamilyCount; queueFamIndx++) {
    const QUEUEFAM_VK*      queuefam  = getQueueFam(gpu, queueFamIndx);
    VkDeviceQueueCreateInfo QueueInfo = {};
    QueueInfo.flags                   = 0;
    QueueInfo.pNext                   = nullptr;
    QueueInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    QueueInfo.queueFamilyIndex        = queuefam->vk_queueFamIndex;
    QueueInfo.pQueuePriorities        = VKCONST_DEFAULT_QUEUE_PRIORITIES;
    QueueInfo.queueCount = gpu->vk_propsQueue[queueFamIndx].queueFamilyProperties.queueCount;
    QueueCreationInfos.list[queueFamIndx] = QueueInfo;
  }
  return QueueCreationInfos;
}

void vk_allocateCmdBuffer(QUEUEFAM_VK* queueFam, VkCommandBufferLevel level, cmdPool_vk*& cmdPool,
                          VkCommandBuffer* cbs, uint32_t count) {
  bool        isFound      = false;
  std::mutex* synchronizer = nullptr;
  uint32_t    cmdPoolIndx  = 0;
  while (!isFound && cmdPoolIndx < threadcount) {
    cmdPool      = &queueFam->m_pools[cmdPoolIndx];
    synchronizer = &cmdPool->m_secondarySync;
    if (synchronizer->try_lock()) {
      break;
    }
    // Loop again and again till find an unlocked one
    cmdPoolIndx = (cmdPoolIndx + 1) % threadcount;
  }

  VkCommandBufferAllocateInfo cb_ai = {};
  cb_ai.pNext                       = nullptr;
  cb_ai.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cb_ai.commandBufferCount          = count;
  cb_ai.level                       = level;
  cb_ai.commandPool                 = queueFam->m_pools[cmdPoolIndx].vk_secondaryCP;
  if (vkAllocateCommandBuffers(queueFam->m_gpu->vk_logical, &cb_ai, cbs) != VK_SUCCESS) {
    vkPrint(16, L"at vkAllocateCommandBuffers()");
  }
  synchronizer->unlock();
}
void vk_freeCmdBuffer(cmdPool_vk* cmdPool, VkCommandBuffer cb) {
  vkFreeCommandBuffers(cmdPool->m_queueFam->m_gpu->vk_logical, cmdPool->vk_secondaryCP, 1, &cb);
}
void manager_vk::get_queue_objects(GPU_VKOBJ* gpu) {
  for (uint32_t queueIndx = 0; queueIndx < mngrPriv->m_queues.size(); queueIndx++) {
    QUEUE_VKOBJ* queue = mngrPriv->m_queues[queueIndx];
    if (queue->m_gpu != gpu) {
      continue;
    }

    vkGetDeviceQueue(gpu->vk_logical, queue->vk_queueFamIndex, queue->m_queueIndx,
                     &queue->vk_queue);

    // Create synchronizer binary semaphore
    {
      VkSemaphoreCreateInfo sem_ci = {};
      sem_ci.flags                 = 0;
      sem_ci.pNext                 = nullptr;
      sem_ci.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
      if (vkCreateSemaphore(gpu->vk_logical, &sem_ci, nullptr, &queue->vk_callSynchronizer) !=
          VK_SUCCESS) {
        vkPrint(16, L"Queue Synchronizer Binary Semaphore Creation failed!");
      }
    }
  }
  QUEUE_VKOBJ* internalQueue = getOBJ<QUEUE_VKOBJ>(gpu->m_internalQueue);
  if (!internalQueue->vk_queue) {
    vkGetDeviceQueue(
      gpu->vk_logical, internalQueue->vk_queueFamIndex,
      gpu->vk_propsQueue[internalQueue->vk_queueFamIndex].queueFamilyProperties.queueCount - 1,
      &internalQueue->vk_queue);

    // Create synchronizer binary semaphore
    {
      VkSemaphoreCreateInfo sem_ci = {};
      sem_ci.flags                 = 0;
      sem_ci.pNext                 = nullptr;
      sem_ci.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
      if (vkCreateSemaphore(gpu->vk_logical, &sem_ci, nullptr,
                            &internalQueue->vk_callSynchronizer) != VK_SUCCESS) {
        vkPrint(16, L"Queue Synchronizer Binary Semaphore Creation failed!");
      }
    }
  }

  // Create cmdPool for each queue family and CPU thread
  for (unsigned int queueFamIndx = 0; queueFamIndx < gpu->desc.queueFamilyCount; queueFamIndx++) {
    QUEUEFAM_VK* queueFam = getQueueFam(gpu, queueFamIndx);
    if (!(queueFam->m_supportFlag.is_COMPUTEsupported ||
          queueFam->m_supportFlag.is_GRAPHICSsupported ||
          queueFam->m_supportFlag.is_TRANSFERsupported)) {
      continue;
    }
    queueFam->m_pools = new (VKGLOBAL_VIRMEM_MANAGER) cmdPool_vk[threadcount];
    for (unsigned char threadIndx = 0; threadIndx < threadcount; threadIndx++) {
      cmdPool_vk& CP                  = queueFam->m_pools[threadIndx];
      CP.m_queueFam                   = queueFam;
      VkCommandPoolCreateInfo cp_ci_g = {};
      cp_ci_g.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      cp_ci_g.queueFamilyIndex        = queueFamIndx;
      cp_ci_g.pNext                   = nullptr;

      {
        cp_ci_g.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if (vkCreateCommandPool(gpu->vk_logical, &cp_ci_g, nullptr, &CP.vk_secondaryCP) !=
            VK_SUCCESS) {
          vkPrint(16, L"Secondary command pool creation for a queue has failed!");
        }
      }
    }
  }
}
uint32_t manager_vk::get_queuefam_index(QUEUE_VKOBJ* fam) { return fam->vk_queueFamIndex; }

static VkPipelineStageFlags VKCONST_PRESENTWAITSTAGEs[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {
  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

struct vk_submitList {
  unsigned int submitCount = 0, binarySemCount = 0;
  submit_vk**  submits       = nullptr;
  VkSemaphore* vk_binarySems = nullptr;
  QUEUE_VKOBJ* queue         = nullptr;
};
void destroyCBsubmission(GPU_VKOBJ* gpu, VkFence fence, void* data) {
  vk_submitList* submission = ( vk_submitList* )data;
  for (uint32_t submitIndx = 0; submitIndx < submission->submitCount; submitIndx++) {
    submit_vk* submit = submission->submits[submitIndx];

    for (uint32_t cbIndx = 0; cbIndx < submit->cmdBufferCount; cbIndx++) {
      CMDBUFFER_VKOBJ* cmdBuffer = getOBJ<CMDBUFFER_VKOBJ>(submit->cmdBuffers[cbIndx]);
      vkDestroyCommandPool(gpu->vk_logical, cmdBuffer->vk_cp, nullptr);
      mngrPriv->m_cmdBuffers.destroyObj(mngrPriv->m_cmdBuffers.getINDEXbyOBJ(cmdBuffer));
    }

    vk_virmem::free_page(VK_POINTER_TO_MEMOFFSET(submit));
  }
  for (uint32_t binarySemaphoreIndx = 0; binarySemaphoreIndx < submission->binarySemCount;
       binarySemaphoreIndx++) {
    vkDestroySemaphore(gpu->vk_logical, submission->vk_binarySems[binarySemaphoreIndx], nullptr);
  }

  vkDestroyFence(gpu->vk_logical, fence, nullptr);
  vk_virmem::free_page(VK_POINTER_TO_MEMOFFSET(submission));
}
void QUEUE_VKOBJ::checkSubmissions() {
  for (int32_t i = 0; i < m_submissions.size(); i++) {
    submission_vk* submission = m_submissions[i];
    if (!submission) {
      continue;
    }
    if (!submission->m_gpu) {
      continue;
    }
    if (vkGetFenceStatus(m_gpu->vk_logical, submission->vk_fence) == VK_SUCCESS) {
      submission->m_callback(submission->m_gpu, submission->vk_fence, submission->m_userData);
      m_submissions.destroyObj(i);
    }
  }
}
void QUEUE_VKOBJ::createSubmission(VkFence fence, void* data, vk_submissionCallback callback) {
  submission_vk* sm = m_submissions.create_OBJ();
  sm->m_gpu         = m_gpu;
  sm->m_callback    = callback;
  sm->m_userData    = data;
  sm->vk_fence      = fence;
  sm->queueIndx     = m_queueIndx;
}
void createQueueSubmitSubmission(VkFence submitFence, QUEUE_VKOBJ* queue, uint32_t binarySemCount,
                                 const VkSemaphore* binarySems) {
  uint32_t submitCount = 0;
  for (; queue->m_unsentSubmits[submitCount]; submitCount++) {
  }
  uint32_t allocSize = sizeof(vk_submitList) + (sizeof(void*) * (submitCount)) +
                       (sizeof(VkSemaphore) * binarySemCount);
  vk_submitList* list =
    ( vk_submitList* )VK_MEMOFFSET_TO_POINTER(vk_virmem::allocatePage(allocSize));
  virmemsys->virtual_commit(list, allocSize);
  list->submitCount    = submitCount;
  list->binarySemCount = binarySemCount;
  list->submits        = ( submit_vk** )(list + 1);
  for (uint32_t submitIndx = 0; submitIndx < submitCount; submitIndx++) {
    list->submits[submitIndx] = queue->m_unsentSubmits[submitIndx];
  }
  list->vk_binarySems = ( VkSemaphore* )(list->submits + list->submitCount);
  for (uint32_t semIndx = 0; semIndx < binarySemCount; semIndx++) {
    list->vk_binarySems[semIndx] = binarySems[semIndx];
  }
  list->queue = queue;
  queue->createSubmission(submitFence, list, destroyCBsubmission);
}
uint32_t QUEUE_VKOBJ::sizeUnsetSubmits() {
  uint32_t submitCount = 0;
  for (; m_unsentSubmits[submitCount] && submitCount < VKCONST_MAXUNSENTSUBMITCOUNT;
       submitCount++) {
  }
  return submitCount;
}

void vkQueueSubmit_CmdBuffers(QUEUE_VKOBJ* queue, VkFence submitFence) {
  VkSubmitInfo    infos[VKCONST_MAXUNSENTSUBMITCOUNT]           = {};
  VkCommandBuffer cmdBuffers[VKCONST_MAXUNSENTSUBMITCOUNT * 16] = {};
  uint32_t        lastCmdBufferIndx                             = 0;
  const uint32_t  submitCount                                   = queue->sizeUnsetSubmits();
  for (uint32_t submitIndx = 0; submitIndx < submitCount; submitIndx++) {
    submit_vk* submit                   = queue->m_unsentSubmits[submitIndx];
    submit->vk_submit.pWaitDstStageMask = VKCONST_PRESENTWAITSTAGEs;
    // Add queue call synchronizer semaphore as wait to sync sequential executeCmdLists calls
    if (submitIndx == 0 && queue->m_prevQueueOp == CMDBUFFER) {
      submit->vk_waitSemaphores[submit->vk_submit.waitSemaphoreCount++] =
        queue->vk_callSynchronizer;
      submit->vk_waitSemaphoreValues[submit->vk_semaphoreInfo.waitSemaphoreValueCount++] = 0;
    }
    // Add queue call synchronizer semaphore as signal to the last submit
    if (submitIndx == submitCount - 1) {
      submit->vk_signalSemaphores[submit->vk_submit.signalSemaphoreCount++] =
        queue->vk_callSynchronizer;
      submit->vk_signalSemaphoreValues[submit->vk_semaphoreInfo.signalSemaphoreValueCount++] = 1;
    }
    submit->vk_submit.pCommandBuffers = &cmdBuffers[lastCmdBufferIndx];
    for (uint32_t cmdBufferIndx = 0; cmdBufferIndx < submit->vk_submit.commandBufferCount;
         cmdBufferIndx++) {
      CMDBUFFER_VKOBJ* cmdBffr        = getOBJ<CMDBUFFER_VKOBJ>(submit->cmdBuffers[cmdBufferIndx]);
      cmdBuffers[lastCmdBufferIndx++] = cmdBffr->vk_cb;
      if (cmdBffr->vk_cb == nullptr) {
        vkPrint(16, L"vkCommandBuffer is nullptr");
      }
    }
    infos[submitIndx] = submit->vk_submit;
  }
  if (vkQueueSubmit(queue->vk_queue, submitCount, infos, submitFence) != VK_SUCCESS) {
    vkPrint(16, L"at vkQueueSubmit()");
  }

  // Add this submit call to submission tracker
  createQueueSubmitSubmission(submitFence, queue, 0, nullptr);
}
void vkQueueSubmit_Present(QUEUE_VKOBJ* queue, VkFence submitFence) {
  VkSwapchainKHR swpchns[VKCONST_MAXSWPCHNCOUNT_PERSUBMIT]       = {};
  uint32_t       swpchnIndices[VKCONST_MAXSWPCHNCOUNT_PERSUBMIT] = {}, swpchnCount = 0;
  WINDOW_VKOBJ*  windows[VKCONST_MAXSWPCHNCOUNT_PERSUBMIT] = {};
  // Timeline Semaphores
  VkSemaphore waitSemaphores[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT]   = {},
              signalSemaphores[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {};
  uint64_t waitValues[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT]          = {},
           signalValues[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT]        = {};
  uint32_t signalSemCount = 0, waitSemCount = 0;
  // Layout transition command buffers
  VkCommandBuffer generalToPresent[VKCONST_MAXSWPCHNCOUNT_PERSUBMIT] = {},
                  presentToGeneral[VKCONST_MAXSWPCHNCOUNT_PERSUBMIT] = {};

  for (uint32_t submitIndx = 0; queue->m_unsentSubmits[submitIndx]; submitIndx++) {
    submit_vk* submit = queue->m_unsentSubmits[submitIndx];

    // If queue call is present
    if (submit->vk_present.sType == VK_STRUCTURE_TYPE_PRESENT_INFO_KHR) {
      if (swpchnCount + submit->vk_present.swapchainCount > VKCONST_MAXSWPCHNCOUNT_PERSUBMIT) {
        vkPrint(16, L"Max swapchain count per submit is exceeded!");
        break;
      }
      for (uint32_t swpchnIndx = 0; swpchnIndx < submit->vk_present.swapchainCount &&
                                    swpchnCount < VKCONST_MAXSWPCHNCOUNT_PERSUBMIT;
           swpchnIndx++) {
        swpchnIndices[swpchnCount] = submit->m_windows[swpchnIndx]->m_swapchainCurrentTextureIndx;
        swpchns[swpchnCount]       = submit->m_windows[swpchnIndx]->vk_swapchain;
        windows[swpchnCount]       = submit->m_windows[swpchnIndx];
        generalToPresent[swpchnCount] =
          submit->m_windows[swpchnIndx]
            ->vk_generalToPresent[queue->vk_queueFamIndex][swpchnIndices[swpchnCount]];
        presentToGeneral[swpchnCount] =
          submit->m_windows[swpchnIndx]
            ->vk_presentToGeneral[getOBJ<QUEUE_VKOBJ>(queue->m_gpu->m_internalQueue)
                                    ->vk_queueFamIndex][swpchnIndices[swpchnCount]];

        swpchnCount++;
      }
    } // If queue call is wait/signal
    else if (submit->vk_submit.sType == VK_STRUCTURE_TYPE_SUBMIT_INFO) {
      for (uint32_t i = 0; i < submit->vk_submit.waitSemaphoreCount; i++) {
        waitSemaphores[i + waitSemCount] = submit->vk_submit.pWaitSemaphores[i];
        waitValues[i + waitSemCount]     = submit->vk_waitSemaphoreValues[i];
      }
      waitSemCount += submit->vk_submit.waitSemaphoreCount;

      for (uint32_t i = 0; i < submit->vk_submit.signalSemaphoreCount; i++) {
        signalSemaphores[i + signalSemCount] = submit->vk_submit.pSignalSemaphores[i];
        signalValues[i + signalSemCount]     = submit->vk_signalSemaphoreValues[i];
      }
      signalSemCount += submit->vk_submit.signalSemaphoreCount;
    } // Queue call invalid
    else {
      vkPrint(16, L"One of the submits is invalid!");
    }
  }

  VkSemaphore binarySignalSemaphores[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {};
  // Submit for timeline -> binary conversion
  {
    for (uint32_t i = 0; i < waitSemCount; i++) {
      VkSemaphoreCreateInfo ci = {};
      ci.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
      if (vkCreateSemaphore(queue->m_gpu->vk_logical, &ci, nullptr, &binarySignalSemaphores[i]) !=
          VK_SUCCESS) {
        vkPrint(16, L"Present's Timeline->Binary converter failed at binary semaphore creation!");
      }
    }
    VkTimelineSemaphoreSubmitInfo timInfo = {};
    timInfo.pNext                         = nullptr;
    timInfo.pSignalSemaphoreValues        = nullptr;
    timInfo.signalSemaphoreValueCount     = 0;
    timInfo.pWaitSemaphoreValues          = waitValues;
    timInfo.waitSemaphoreValueCount       = waitSemCount;
    timInfo.sType                         = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    VkSubmitInfo si                       = {};
    si.sType                              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext                              = &timInfo;
    si.commandBufferCount                 = swpchnCount;
    si.pCommandBuffers                    = generalToPresent;
    si.waitSemaphoreCount                 = waitSemCount;
    si.pWaitDstStageMask                  = VKCONST_PRESENTWAITSTAGEs;
    si.pWaitSemaphores                    = waitSemaphores;
    if (queue->m_prevQueueOp == CMDBUFFER) {
      si.waitSemaphoreCount = waitSemCount + 1;
      timInfo.waitSemaphoreValueCount++;
      waitValues[waitSemCount]     = 0;
      waitSemaphores[waitSemCount] = queue->vk_callSynchronizer;
    }
    si.signalSemaphoreCount = signalSemCount;
    si.pSignalSemaphores    = binarySignalSemaphores;
    if (vkQueueSubmit(queue->vk_queue, 1, &si, VK_NULL_HANDLE) != VK_SUCCESS) {
      vkPrint(16, L"at vkQueueSubmit() of Present's Timeline->Binary converter");
    }
  }

  VkPresentInfoKHR info = {};
  info.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  info.pNext            = nullptr;
  info.pSwapchains      = swpchns;
  info.swapchainCount   = swpchnCount;
  info.pImageIndices    = swpchnIndices;
  // For now, all calls are synchronized after each other
  // Because we don't have timeline semaphore emulation in binary semaphores
  info.waitSemaphoreCount = waitSemCount;
  info.pWaitSemaphores    = binarySignalSemaphores;
  info.pResults           = nullptr;
  VkResult result = vkQueuePresentKHR(queue->vk_queue, &info);
  if (result != VK_SUCCESS) {
    vkPrint(16, L"at vkQueuePresentKHR()");
  }

  // Send a submit to signal timeline semaphore when all binary semaphore are signaled
  {
    VkSemaphore binAcquireSemaphores[VKCONST_MAXSWPCHNCOUNT_PERSUBMIT] = {};
    for (uint32_t i = 0; i < swpchnCount; i++) {
      binAcquireSemaphores[i] = windows[i]->vk_acquireSemaphore;
    }

    VkTimelineSemaphoreSubmitInfo temSignalSemaphoresInfo = {};
    temSignalSemaphoresInfo.sType                = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    temSignalSemaphoresInfo.pWaitSemaphoreValues = nullptr;
    temSignalSemaphoresInfo.waitSemaphoreValueCount   = 0;
    temSignalSemaphoresInfo.pNext                     = nullptr;
    temSignalSemaphoresInfo.signalSemaphoreValueCount = signalSemCount;
    temSignalSemaphoresInfo.pSignalSemaphoreValues    = signalValues;
    VkSubmitInfo acquireSubmit                        = {};
    acquireSubmit.waitSemaphoreCount                  = swpchnCount;
    acquireSubmit.pWaitSemaphores                     = binAcquireSemaphores;
    acquireSubmit.signalSemaphoreCount                = signalSemCount;
    acquireSubmit.pSignalSemaphores                   = signalSemaphores;
    acquireSubmit.commandBufferCount                  = swpchnCount;
    acquireSubmit.pCommandBuffers                     = presentToGeneral;
    acquireSubmit.pNext                               = nullptr;
    acquireSubmit.pWaitDstStageMask                   = VKCONST_PRESENTWAITSTAGEs;
    acquireSubmit.sType                               = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    if (vkQueueSubmit(getOBJ<QUEUE_VKOBJ>(queue->m_gpu->m_internalQueue)->vk_queue, 1,
                      &acquireSubmit, submitFence)) {
      vkPrint(16, L"at vkQueueSubmit() for binary -> timeline semaphore conversion");
    }
  }

  // Add this submit call to tracker
  createQueueSubmitSubmission(submitFence, queue, waitSemCount, binarySignalSemaphores);
}
void manager_vk::queueSubmit(QUEUE_VKOBJ* queue) {
  GPU_VKOBJ* gpu = queue->m_gpu;
  // Check previously sent submission to detect if they're still executing
  queue->checkSubmissions();

  VkFence submitFence = {};
  {
    VkFenceCreateInfo f_ci = {};
    f_ci.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (vkCreateFence(gpu->vk_logical, &f_ci, nullptr, &submitFence) != VK_SUCCESS) {
      vkPrint(52, L"at Submission tracker fence creation");
    }
  }
  switch (queue->m_activeQueueOp) {
    case ERROR_QUEUEOPTYPE: queue->m_activeQueueOp = CMDBUFFER;
    case CMDBUFFER: vkQueueSubmit_CmdBuffers(queue, submitFence); break;
    case PRESENT: vkQueueSubmit_Present(queue, submitFence); break;
    default: vkPrint(52, L"Active queue operation type isn't supported by backend!"); break;
  }

  queue->m_prevQueueOp   = queue->m_activeQueueOp;
  queue->m_activeQueueOp = ERROR_QUEUEOPTYPE;
  for (uint32_t submitIndx = 0; submitIndx < VKCONST_MAXUNSENTSUBMITCOUNT; submitIndx++) {
    queue->m_unsentSubmits[submitIndx] = nullptr;
  }
}
bool manager_vk::does_queuefamily_support(QUEUE_VKOBJ* family, const queueflag_vk& flag) {
  vkPrint(16, L"does_queuefamily_support() isn't coded");
  return false;
}

// Helper functions

void VK_getQueueAndSharingInfos(unsigned int queueList, struct tgfx_gpuQueue* const* i_queueList,
                                unsigned int extCount, struct tgfx_extension* const* i_exts,
                                uint32_t* o_famList, uint32_t* o_famListSize,
                                VkSharingMode* o_sharingMode) {
  uint32_t   validQueueFamCount = 0;
  GPU_VKOBJ* theGPU             = nullptr;
  for (uint32_t listIndx = 0; listIndx < queueList; listIndx++) {
    QUEUE_VKOBJ* queue = getOBJ<QUEUE_VKOBJ>(i_queueList[listIndx]);
    if (!queue) {
      continue;
    }
    GPU_VKOBJ*   gpu   = queue->m_gpu;
    if (!theGPU) {
      theGPU = gpu;
    }
    if (theGPU != gpu) {
      vkPrint(37);
      return;
    }
    bool isPreviouslyAdded = false;
    for (uint32_t validQueueIndx = 0; validQueueIndx < validQueueFamCount; validQueueIndx++) {
      if (o_famList[validQueueIndx] == queue->vk_queueFamIndex) {
        isPreviouslyAdded = true;
        break;
      }
    }
    if (!isPreviouslyAdded) {
      o_famList[validQueueFamCount++] = queue->vk_queueFamIndex;
    }
  }
  if (validQueueFamCount > 1) {
    *o_sharingMode = VK_SHARING_MODE_CONCURRENT;
  } else {
    *o_sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }
  *o_famListSize = validQueueFamCount;
  for (uint32_t i = *o_famListSize; i < VKCONST_MAXQUEUEFAMCOUNT_PERGPU; i++) {
    o_famList[i] = UINT32_MAX;
  }
}

// Renderer Command Buffer Functionality

struct tgfx_commandBuffer* vk_beginCommandBuffer(struct tgfx_gpuQueue* i_queue, unsigned int extCount,
                                            struct tgfx_extension* const* exts) {
  getGPUfromQueueHnd(i_queue);
  QUEUEFAM_VK* queueFam = getQueueFam(gpu, queue->vk_queueFamIndex);

  VkCommandPoolCreateInfo cp_ci = {};
  cp_ci.flags                   = 0;
  cp_ci.pNext                   = nullptr;
  cp_ci.queueFamilyIndex        = queue->vk_queueFamIndex;
  cp_ci.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  VkCommandPool cp              = {};
  if (vkCreateCommandPool(gpu->vk_logical, &cp_ci, nullptr, &cp) != VK_SUCCESS) {
    vkPrint(53, L"at vkCreateCommandPool()");
    return nullptr;
  }
  VkCommandBuffer             cb    = {};
  VkCommandBufferAllocateInfo cb_ai = {};
  cb_ai.commandBufferCount          = 1;
  cb_ai.commandPool                 = cp;
  cb_ai.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cb_ai.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cb_ai.pNext                       = nullptr;
  if (vkAllocateCommandBuffers(gpu->vk_logical, &cb_ai, &cb) != VK_SUCCESS) {
    vkPrint(53, L"at vkAllocatteCommandBuffers()");
    return nullptr;
  }
  VkCommandBufferBeginInfo cb_bi = {};
  cb_bi.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  cb_bi.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (vkBeginCommandBuffer(cb, &cb_bi) != VK_SUCCESS) {
    vkPrint(53, L"at vkBeginCommandBuffer() failed!");
    vkDestroyCommandPool(gpu->vk_logical, cp, nullptr);
    return nullptr;
  }
  CMDBUFFER_VKOBJ* cmdBuffer = mngrPriv->m_cmdBuffers.create_OBJ();
  cmdBuffer->vk_cp           = cp;
  cmdBuffer->vk_cb           = cb;
  cmdBuffer->m_gpuIndx       = gpu->gpuIndx();
  cmdBuffer->m_queueFamIndx  = queueFam->vk_queueFamIndex;
  return getHANDLE<struct tgfx_commandBuffer*>(cmdBuffer);
}

#define getCmdBufferfromHnd(cmdBufferHnd)                             \
  CMDBUFFER_VKOBJ* cmdBuffer = getOBJ<CMDBUFFER_VKOBJ>(cmdBufferHnd); \
  GPU_VKOBJ*       gpu       = core_vk->getGPU(cmdBuffer->m_gpuIndx); \
  QUEUEFAM_VK*     queueFam  = getQueueFam(gpu, cmdBuffer->m_queueFamIndx);

#define checkCmdBufferHnd()                    \
  if (gpu == nullptr || queueFam == nullptr) { \
    vkPrint(11);                               \
    return;                                    \
  }

void vk_endCommandBuffer(struct tgfx_commandBuffer* cb) {
  getCmdBufferfromHnd(cb);
#ifdef VULKAN_DEBUGGING
  checkCmdBufferHnd();
#endif
  if (vkEndCommandBuffer(cmdBuffer->vk_cb) != VK_SUCCESS) {
    vkPrint(53, L"at vkEndCommandBuffer()");
  }
}
void vk_executeBundles(struct tgfx_commandBuffer* cb, unsigned int bundleCount,
                       struct tgfx_commandBundle* const* bundles, unsigned int extCount,
                       struct tgfx_extension* const* exts) {
  getCmdBufferfromHnd(cb);
#ifdef VULKAN_DEBUGGING
  checkCmdBufferHnd();
#endif

  if (bundleCount > VKCONST_MAXCMDBUNDLE_PERCALL) {
    vkPrint(16, L"Bundle list size exceeded -VKCONST_MAXCMDBUNDLE_PERCALL-, please report this");
    return;
  }
  VkCommandBuffer secCmdBuffers[VKCONST_MAXCMDBUNDLE_PERCALL] = {};
  if (!bundleCount) {
    return;
  }
  vk_getSecondaryCmdBuffers(bundleCount, bundles, queueFam->vk_queueFamIndex, secCmdBuffers);
  vkCmdExecuteCommands(cmdBuffer->vk_cb, bundleCount, secCmdBuffers);
}

void vk_beginRasterpass(struct tgfx_commandBuffer* commandBuffer, unsigned int colorAttachmentCount,
                        const tgfx_rasterpassBeginSlotInfo* colorAttachments,
                        const tgfx_rasterpassBeginSlotInfo* depthAttachment, unsigned int extCount,
                        struct tgfx_extension* const* exts) {
  getCmdBufferfromHnd(commandBuffer);
#ifdef VULKAN_DEBUGGING
  checkCmdBufferHnd();
#endif

  vkext_dynamicRendering* dynRenderingExt =
    ( vkext_dynamicRendering* )gpu->ext()->m_exts[vkext_interface::dynamicRendering_vkExtEnum];
  dynRenderingExt->vk_beginRenderpass(cmdBuffer->vk_cb, colorAttachmentCount, colorAttachments,
                                      *depthAttachment, extCount, exts);
}
void vk_nextRendersubpass(struct tgfx_commandBuffer* commandBuffer) {
  getCmdBufferfromHnd(commandBuffer);
#ifdef VULKAN_DEBUGGING
  checkCmdBufferHnd();
#endif

  vkCmdNextSubpass(cmdBuffer->vk_cb, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}
void vk_endRasterpass(struct tgfx_commandBuffer* commandBuffer, unsigned int extCount,
                      struct tgfx_extension* const* exts) {
  getCmdBufferfromHnd(commandBuffer);
#ifdef VULKAN_DEBUGGING
  checkCmdBufferHnd();
#endif

  vkext_dynamicRendering* dynRenderingExt =
    ( vkext_dynamicRendering* )gpu->ext()->m_exts[vkext_interface::dynamicRendering_vkExtEnum];
  dynRenderingExt->vk_endRenderpass(cmdBuffer->vk_cb, extCount, exts);
}

void vk_addSubmitToUnsentList(QUEUE_VKOBJ* queue, submit_vk* submit) {
  for (uint32_t submitIndx = 0; submitIndx < VKCONST_MAXUNSENTSUBMITCOUNT; submitIndx++) {
    if (!queue->m_unsentSubmits[submitIndx]) {
      queue->m_unsentSubmits[submitIndx] = submit;
      return;
    }
  }
  vkPrint(16, L"Unsent submit count limit is exceeded!");
}
static constexpr VkPipelineStageFlags vk_waitDstStageMask[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT * 2] =
  {VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
void vk_queueExecuteCmdBuffers(struct tgfx_gpuQueue* i_queue, unsigned int cmdBufferCount,
                               struct tgfx_commandBuffer* const* i_cmdBuffers, unsigned int extCount,
                               struct tgfx_extension* const* exts) {
  getGPUfromQueueHnd(i_queue);
  getTimelineSemaphoreEXT(gpu, semSys);

  if (!cmdBufferCount) {
    return;
  }
  if (queue->m_activeQueueOp != ERROR_QUEUEOPTYPE && queue->m_activeQueueOp != CMDBUFFER) {
    vkPrint(54);
    return;
  }
  queue->m_activeQueueOp = CMDBUFFER;

  {
    for (uint32_t i = 0; i < cmdBufferCount; i++) {
      getCmdBufferfromHnd(i_cmdBuffers[i]);
      checkCmdBufferHnd();
    }
  }

  submit_vk* submit = submit_vk::allocateSubmit(0, 0, cmdBufferCount, 0);
  memcpy(submit->cmdBuffers, i_cmdBuffers, sizeof(struct tgfx_commandBuffer*) * cmdBufferCount);
  submit->vk_submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit->vk_submit.pNext              = nullptr;
  submit->vk_submit.pSignalSemaphores  = submit->vk_signalSemaphores;
  submit->vk_submit.pWaitSemaphores    = submit->vk_waitSemaphores;
  submit->vk_submit.pWaitDstStageMask  = vk_waitDstStageMask;
  submit->vk_submit.commandBufferCount = cmdBufferCount;
  vk_addSubmitToUnsentList(queue, submit);
}
void vk_queueFenceWaitSignal(struct tgfx_gpuQueue* i_queue, unsigned int waitsCount,
                             struct tgfx_fence* const* waitFences, const unsigned long long* waitValues,
                             unsigned int signalsCount, struct tgfx_fence* const* signalFences,
                             const unsigned long long* signalValues) {
  getGPUfromQueueHnd(i_queue);
  getTimelineSemaphoreEXT(gpu, semSys);
  const FENCE_VKOBJ* waits[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {};
  {
    if (waitsCount > VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT) {
      vkPrint(52, L"Max semaphore count per submit is exceeded!");
      return;
    }
    for (uint32_t i = 0; i < waitsCount; i++) {
      FENCE_VKOBJ* wait = getOBJ<FENCE_VKOBJ>(waitFences[i]);
      assert(wait);
      waits[i] = wait;
      wait->m_curValue.store(waitValues[i]);
    }
  }
  const FENCE_VKOBJ* signals[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {};
  {
    if (signalsCount > VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT) {
      vkPrint(52, L"Max semaphore count per submit is exceeded!");
      return;
    }
    for (uint32_t i = 0; i < signalsCount; i++) {
      FENCE_VKOBJ* signal = getOBJ<FENCE_VKOBJ>(signalFences[i]);
      assert(signal);
      signals[i] = signal;
      signal->m_nextValue.store(signalValues[i]);
    }
  }

  // TIMELINE SEMAPHORE

  // Create and fill submit struct
  submit_vk* submit = submit_vk::allocateSubmit(signalsCount, waitsCount, 0, 0);
  {
    uint32_t& waitSemaphoreCount = submit->vk_submit.waitSemaphoreCount;
    for (waitSemaphoreCount = 0; waitSemaphoreCount < VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT;
         waitSemaphoreCount++) {
      const FENCE_VKOBJ* fence = waits[waitSemaphoreCount];
      if (!fence) {
        break;
      }
      submit->vk_waitSemaphoreValues[waitSemaphoreCount] = fence->m_curValue;
      submit->vk_waitSemaphores[waitSemaphoreCount]      = fence->vk_timelineSemaphore;
    }
    uint32_t& signalSemaphoreCount = submit->vk_submit.signalSemaphoreCount;
    for (signalSemaphoreCount = 0; signalSemaphoreCount < VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT;
         signalSemaphoreCount++) {
      const FENCE_VKOBJ* fence = signals[signalSemaphoreCount];
      if (!fence) {
        break;
      }
      submit->vk_signalSemaphoreValues[signalSemaphoreCount] = fence->m_nextValue;
      submit->vk_signalSemaphores[signalSemaphoreCount]      = fence->vk_timelineSemaphore;
    }

    submit->vk_semaphoreInfo.pNext                     = nullptr;
    submit->vk_semaphoreInfo.pSignalSemaphoreValues    = submit->vk_signalSemaphoreValues;
    submit->vk_semaphoreInfo.pWaitSemaphoreValues      = submit->vk_waitSemaphoreValues;
    submit->vk_semaphoreInfo.signalSemaphoreValueCount = signalSemaphoreCount;
    submit->vk_semaphoreInfo.waitSemaphoreValueCount   = waitSemaphoreCount;
    submit->vk_semaphoreInfo.sType       = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    submit->vk_submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit->vk_submit.pNext              = &submit->vk_semaphoreInfo;
    submit->vk_submit.pCommandBuffers    = nullptr;
    submit->vk_submit.commandBufferCount = 0;
    submit->vk_submit.pSignalSemaphores  = submit->vk_signalSemaphores;
    submit->vk_submit.pWaitSemaphores    = submit->vk_waitSemaphores;
    submit->vk_submit.pWaitDstStageMask  = vk_waitDstStageMask;
  }
  vk_addSubmitToUnsentList(queue, submit);
}
void vk_queueSubmit(struct tgfx_gpuQueue* i_queue) {
  getGPUfromQueueHnd(i_queue);
  manager->queueSubmit(queue);
}
void vk_queuePresent(struct tgfx_gpuQueue* i_queue, unsigned int windowCount,
                     struct tgfx_window* const* windowlist) {
  getGPUfromQueueHnd(i_queue);

  if (queue->m_activeQueueOp != ERROR_QUEUEOPTYPE && queue->m_activeQueueOp != PRESENT) {
    vkPrint(54);
    return;
  }

  queue->m_activeQueueOp = PRESENT;
  submit_vk* submit      = submit_vk::allocateSubmit(0, 0, 0, windowCount);

  for (uint32_t i = 0; i < windowCount; i++) {
    submit->m_windows[i] = getOBJ<WINDOW_VKOBJ>(windowlist[i]);
  }
  submit->type                          = PRESENT;
  submit->vk_present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  submit->vk_present.pNext              = nullptr;
  submit->vk_present.pWaitSemaphores    = nullptr;
  submit->vk_present.waitSemaphoreCount = 0;
  submit->vk_present.pResults           = nullptr;
  submit->vk_present.pImageIndices      = nullptr;
  submit->vk_present.pSwapchains        = nullptr;
  submit->vk_present.swapchainCount     = windowCount;
  vk_addSubmitToUnsentList(queue, submit);
}

VkCommandPool vk_getSecondaryCmdPool(QUEUEFAM_VK* queueFam, uint32_t cmdPoolIndx) {
  cmdPool_vk& cmdPool = queueFam->m_pools[cmdPoolIndx];
  if (!cmdPool.vk_secondaryCP) {
    vkPrint(16, L"at vk_getSecondaryCmdPool()");
    return nullptr;
  }
  return cmdPool.vk_secondaryCP;
}
void vk_setQueueFncPtrs() {
  core_tgfx_main->renderer->beginCommandBuffer     = vk_beginCommandBuffer;
  core_tgfx_main->renderer->endCommandBuffer       = vk_endCommandBuffer;
  core_tgfx_main->renderer->executeBundles         = vk_executeBundles;
  core_tgfx_main->renderer->beginRasterpass        = vk_beginRasterpass;
  core_tgfx_main->renderer->nextSubRasterpass      = vk_nextRendersubpass;
  core_tgfx_main->renderer->endRasterpass          = vk_endRasterpass;
  core_tgfx_main->renderer->queueExecuteCmdBuffers = vk_queueExecuteCmdBuffers;
  core_tgfx_main->renderer->queueFenceSignalWait   = vk_queueFenceWaitSignal;
  core_tgfx_main->renderer->queueSubmit            = vk_queueSubmit;
  core_tgfx_main->renderer->queuePresent           = vk_queuePresent;
  core_tgfx_main->helpers->getGPUInfo_Queues       = &vk_getGPUInfoQueues;

  for (uint32_t i = 0; i < VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT; i++) {
    VKCONST_PRESENTWAITSTAGEs[i] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  }
}