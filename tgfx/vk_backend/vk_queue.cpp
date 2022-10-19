#include "vk_queue.h"

#include <threadingsys_tapi.h>

#include <mutex>

#include "tgfx_renderer.h"
#include "vk_core.h"
#include "vkext_timelineSemaphore.h"

vk_uint32c VKCONST_MAXFENCECOUNT_PERSUBMIT = 8, VKCONST_MAXCOMMANDBUFFERCOUNT_PERSUBMIT = 256;
vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_MANAGER = nullptr;

queueflag_vk::operator uint8_t() const {
  if constexpr (sizeof(queueflag_vk) == 1) {
    return *(( uint8_t* )this);
  } else {
    printer(result_tgfx_NOTCODED, "queueflag_vk shouldn't be more than 1 byte");
  }
  return 0;
};
struct cmdBuffer_vk {
  bool            isALIVE = true;
  VkCommandBuffer CB;
  bool            is_Used = false;
  // Pointer to allocated space for storing render calls below
  unsigned int             DATAPTR = 0;
  tgfx_rendererKeySortFunc sortFunc;
};

struct cmdPool_vk {
  cmdPool_vk() = default;
  cmdPool_vk(const cmdPool_vk& RefCP) { CPHandle = RefCP.CPHandle; }
  void          operator=(const cmdPool_vk& RefCP) { CPHandle = RefCP.CPHandle; }
  cmdBuffer_vk& createCmdBuffer();
  void          destroyCmdBuffer();
  VkCommandPool CPHandle = VK_NULL_HANDLE;
  // First cmdBuffer is the empty CB
  VK_STATICVECTOR<cmdBuffer_vk, void*, 1 << 8> CBs;

 private:
  std::mutex Sync;
};
GPU_VKOBJ* QUEUE_VKOBJ::getGPUfromHandle(gpuQueue_tgfxhnd hnd) {
  VKOBJHANDLE handle  = *( VKOBJHANDLE* )&hnd;
  uint32_t    gpuIndx = handle.EXTRA_FLAGs >> 8;
  return core_vk->getGPUs()[gpuIndx];
}
QUEUEFAM_VK* QUEUE_VKOBJ::getFAMfromHandle(gpuQueue_tgfxhnd hnd) {
  GPU_VKOBJ*  gpu          = getGPUfromHandle(hnd);
  VKOBJHANDLE handle       = *( VKOBJHANDLE* )&hnd;
  uint32_t    queueFamIndx = (handle.EXTRA_FLAGs << 24) >> 24;
  return gpu->manager()->m_queueFams[queueFamIndx];
}

manager_vk* manager_vk::createManager(GPU_VKOBJ* gpu) {
  if (!VKGLOBAL_VIRMEM_MANAGER) {
    // 16MB is enough I guess?
    VKGLOBAL_VIRMEM_MANAGER = vk_virmem::allocate_dynamicmem(1 << 24);
  }
  manager_vk* mngr = new (VKGLOBAL_VIRMEM_MANAGER) manager_vk;
  mngr->m_gpu      = gpu;
  gpu->manager()   = mngr;

  uint32_t queueFamiliesCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties2(gpu->vk_physical, &queueFamiliesCount, nullptr);
  bool is_presentationfound = false;
  for (uint32_t i = 0; i < VKCONST_MAXQUEUEFAMCOUNT_PERGPU; i++) {
    gpu->vk_propsQueue[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
    gpu->vk_propsQueue[i].pNext = nullptr;
  }
#ifdef VULKAN_DEBUGGING
  if (VKCONST_MAXQUEUEFAMCOUNT_PERGPU < queueFamiliesCount) {
    printer(result_tgfx_FAIL,
            "VK backend's queue analization process should support more queue families! Please "
            "report this!");
  }
#endif
  vkGetPhysicalDeviceQueueFamilyProperties2(gpu->vk_physical, &queueFamiliesCount,
                                            gpu->vk_propsQueue);
  gpu->desc.queueFamilyCount = queueFamiliesCount;
  for (uint32_t queueFamIndx = 0; queueFamIndx < queueFamiliesCount; queueFamIndx++) {
    VkQueueFamilyProperties2* props    = &gpu->vk_propsQueue[queueFamIndx];
    QUEUEFAM_VK*              queueFam = mngr->m_queueFams.add();

    queueFam->vk_queueFamIndex = queueFamIndx;
    if (props->queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      gpu->desc.is_GraphicOperations_Supported     = true;
      queueFam->m_supportFlag.is_GRAPHICSsupported = true;
    }
    if (props->queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) {
      gpu->desc.is_ComputeOperations_Supported    = true;
      queueFam->m_supportFlag.is_COMPUTEsupported = true;
    }
    if (props->queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) {
      gpu->desc.is_TransferOperations_Supported    = true;
      queueFam->m_supportFlag.is_TRANSFERsupported = true;
    }
    if (props->queueFamilyProperties.queueCount > VKCONST_MAXQUEUECOUNT_PERFAM) {
      printer(result_tgfx_WARNING, "Your device supports more queue than Vulkan backend supports!");
      props->queueFamilyProperties.queueCount = VKCONST_MAXQUEUECOUNT_PERFAM;
    }
    queueFam->m_gpu = gpu;
  }
  if (!gpu->desc.is_GraphicOperations_Supported || !gpu->desc.is_TransferOperations_Supported ||
      !gpu->desc.is_ComputeOperations_Supported) {
    printer(result_tgfx_FAIL,
            "The GPU doesn't support one of the following operations, so we can't let you use this "
            "GPU: Compute, Transfer, Graphics");
    return nullptr;
  }
  // Select internal queue, then create other queue backend objects
  QUEUEFAM_VK* bestQueueFam = nullptr;
  for (uint32_t queueFamIndx = 0; queueFamIndx < queueFamiliesCount; queueFamIndx++) {
    VkQueueFamilyProperties2* props    = &gpu->vk_propsQueue[queueFamIndx];
    QUEUEFAM_VK*              queueFam = mngr->m_queueFams[queueFamIndx];
    if (!bestQueueFam) {
      bestQueueFam = queueFam;
      break;
    }
    const VkQueueFamilyProperties& bestQueueProps =
      gpu->vk_propsQueue[bestQueueFam->vk_queueFamIndex].queueFamilyProperties;

    if (props->queueFamilyProperties.queueCount > bestQueueProps.queueCount) {
      bestQueueFam = queueFam;
      break;
    }

    // Bigger values in VkQueueFlags means more useful the queue gets
    // For internal operations, we need a queue as useless as possible.
    // For example: 4 is tranfer, 8 is sparse, 16 is protected bits which are supported by all
    //  queues if device supports. Main difference between is generally in Graphics and Compute
    //  bits. So Graphics & Compute bits are main usefulness detector.
    if (props->queueFamilyProperties.queueFlags < bestQueueProps.queueFlags) {
      bestQueueFam = queueFam;
      break;
    }
  }
  gpu->vk_propsQueue[bestQueueFam->vk_queueFamIndex].queueFamilyProperties.queueCount--;

  // Create user queues
  for (uint32_t queueFamIndx = 0; queueFamIndx < queueFamiliesCount; queueFamIndx++) {
    const VkQueueFamilyProperties& props = gpu->vk_propsQueue[queueFamIndx].queueFamilyProperties;
    QUEUEFAM_VK*                   queueFam = mngr->m_queueFams[queueFamIndx];

    for (uint32_t i = 0; i < props.queueCount; i++) {
      QUEUE_VKOBJ* queue      = queueFam->m_queues.add();
      queue->m_gpu            = gpu;
      queue->vk_queueFamIndex = queueFamIndx;
      queue->vk_queue         = VK_NULL_HANDLE;
    }
  }

  // Create internal queue
  {
    mngr->m_internalQueue                   = new (VKGLOBAL_VIRMEM_MANAGER) QUEUE_VKOBJ;
    mngr->m_internalQueue->m_gpu            = gpu;
    mngr->m_internalQueue->vk_queueFamIndex = bestQueueFam->vk_queueFamIndex;
    mngr->m_internalQueue->vk_queue         = VK_NULL_HANDLE;
    gpu->vk_propsQueue[bestQueueFam->vk_queueFamIndex].queueFamilyProperties.queueCount++;
  }
  return mngr;
}

static float VKCONST_DEFAULT_QUEUE_PRIORITIES[VKCONST_MAXQUEUECOUNT_PERFAM] = {};
// While creating VK Logical Device, we need which queues to create. Get that info from here.
manager_vk::queueCreateInfoList manager_vk::get_queue_cis() const {
  for (uint32_t i = 0; i < VKCONST_MAXQUEUECOUNT_PERFAM; i++) {
    VKCONST_DEFAULT_QUEUE_PRIORITIES[i] = std::min(float(i) / VKCONST_MAXQUEUECOUNT_PERFAM, 1.0f);
  }
  queueCreateInfoList QueueCreationInfos;
  // Queue Creation Processes
  for (unsigned int queueFamIndx = 0; queueFamIndx < m_queueFams.size(); queueFamIndx++) {
    const QUEUEFAM_VK*      queuefam  = m_queueFams.get(queueFamIndx);
    VkDeviceQueueCreateInfo QueueInfo = {};
    QueueInfo.flags                   = 0;
    QueueInfo.pNext                   = nullptr;
    QueueInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    QueueInfo.queueFamilyIndex        = queuefam->vk_queueFamIndex;
    QueueInfo.pQueuePriorities        = VKCONST_DEFAULT_QUEUE_PRIORITIES;
    QueueInfo.queueCount = m_gpu->vk_propsQueue[queueFamIndx].queueFamilyProperties.queueCount;
    QueueCreationInfos.list[queueFamIndx] = QueueInfo;
  }
  return QueueCreationInfos;
}

void manager_vk::get_queue_objects() {
  for (unsigned int i = 0; i < m_queueFams.size(); i++) {
    for (unsigned int queueindex = 0; queueindex < m_queueFams[i]->m_queues.size(); queueindex++) {
      vkGetDeviceQueue(m_gpu->vk_logical, m_queueFams[i]->vk_queueFamIndex, queueindex,
                       &(m_queueFams[i]->m_queues[queueindex]->vk_queue));

      // Create synchronizer binary semaphore
      {
        VkSemaphoreCreateInfo sem_ci = {};
        sem_ci.flags                 = 0;
        sem_ci.pNext                 = nullptr;
        sem_ci.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        ThrowIfFailed(vkCreateSemaphore(m_gpu->vk_logical, &sem_ci, nullptr,
                                        &m_queueFams[i]->m_queues[queueindex]->vk_callSynchronizer),
                      "Queue Synchronizer Binary Semaphore Creation failed!");
      }
    }
  }

  // Get internal queue
  {
    QUEUEFAM_VK* internalQueueFam = m_queueFams[m_internalQueue->vk_queueFamIndex];
    vkGetDeviceQueue(m_gpu->vk_logical, m_internalQueue->vk_queueFamIndex,
                     internalQueueFam->m_queues.size(), &m_internalQueue->vk_queue);
  }

  // Create cmdPool for each queue family and CPU thread
  for (unsigned int queuefamindex = 0; queuefamindex < m_queueFams.size(); queuefamindex++) {
    if (!(m_queueFams[queuefamindex]->m_supportFlag.is_COMPUTEsupported ||
          m_queueFams[queuefamindex]->m_supportFlag.is_GRAPHICSsupported ||
          m_queueFams[queuefamindex]->m_supportFlag.is_TRANSFERsupported)) {
      continue;
    }
    m_queueFams[queuefamindex]->m_pools = new (VKGLOBAL_VIRMEM_MANAGER) cmdPool_vk[threadcount];
    for (unsigned char i = 0; i < threadcount; i++) {
      VkCommandPoolCreateInfo cp_ci_g = {};
      cp_ci_g.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      cp_ci_g.queueFamilyIndex        = queuefamindex;
      cp_ci_g.flags =
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
      cp_ci_g.pNext = nullptr;

      ThrowIfFailed(vkCreateCommandPool(m_gpu->vk_logical, &cp_ci_g, nullptr,
                                        &m_queueFams[queuefamindex]->m_pools[i].CPHandle),
                    "Command pool creation for a queue has failed at vkCreateCommandPool()!");
    }
  }
}
uint32_t manager_vk::get_queuefam_index(QUEUE_VKOBJ* fam) {
  /*
#ifdef VULKAN_DEBUGGING
  bool isfound = false;
  for (unsigned int i = 0; i < m_queueFamiliesCount; i++) {
    if (m_queueFams[i] == fam) {
      isfound = true;
    }
  }
  if (!isfound) {
    printer(result_tgfx_FAIL, "Queue Family Handle is invalid!");
  }
#endif
*/
  return fam->vk_queueFamIndex;
}
cmdBuffer_vk* manager_vk::get_commandbuffer(QUEUEFAM_VK* family, bool isSecondary) {
  cmdPool_vk& CP = family->m_pools[threadingsys->this_thread_index()];

  VkCommandBufferAllocateInfo cb_ai = {};
  cb_ai.commandBufferCount          = 1;
  cb_ai.commandPool                 = CP.CPHandle;
  cb_ai.level = isSecondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cb_ai.pNext = nullptr;
  cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

  cmdBuffer_vk* VK_CB = CP.CBs.add();
  if (vkAllocateCommandBuffers(m_gpu->vk_logical, &cb_ai, &VK_CB->CB) != VK_SUCCESS) {
    printer(result_tgfx_FAIL,
            "vkAllocateCommandBuffers() failed while creating command buffers for RGBranches, "
            "report this please!");
    return nullptr;
  }

  VK_CB->is_Used = false;
  return VK_CB;
}

VkCommandBuffer manager_vk::get_commandbufferobj(cmdBuffer_vk* id) { return id->CB; }

void manager_vk::queueSubmit(QUEUE_VKOBJ* queue) {
  switch (queue->activeQueueOp) {
    case QUEUE_VKOBJ::ERROR_QUEUEOPTYPE:
    case QUEUE_VKOBJ::CMDBUFFER: {
      VkSubmitInfo infos[VKCONST_MAXSUBMITCOUNT] = {};
      for (uint32_t i = 0; i < queue->m_submitInfos.size(); i++) {
        infos[i] = queue->m_submitInfos[i]->vk_submit;
      }
      ThrowIfFailed(
        vkQueueSubmit(queue->vk_queue, queue->m_submitInfos.size(), infos, VK_NULL_HANDLE),
        "Queue Submission has failed!");
    } break;
    case QUEUE_VKOBJ::PRESENT: {
      VkSwapchainKHR swpchns[VKCONST_MAXPRESENTCOUNT_PERSUBMIT]       = {};
      uint32_t       swpchnIndices[VKCONST_MAXPRESENTCOUNT_PERSUBMIT] = {}, swpchnCount = 0;
      window_tgfxhnd windows[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {};

      for (uint32_t submitIndx = 0; submitIndx < queue->m_submitInfos.size(); submitIndx++) {
        submit_vk* submit = queue->m_submitInfos[submitIndx];

        // If queue call is present
        if (submit->vk_present.sType == VK_STRUCTURE_TYPE_PRESENT_INFO_KHR) {
          if (swpchnCount + submit->vk_present.swapchainCount > VKCONST_MAXPRESENTCOUNT_PERSUBMIT) {
            printer(result_tgfx_FAIL, "Max swapchain count per submit is exceeded!");
            break;
          }
          for (uint32_t swpchnIndx = 0; swpchnIndx < submit->vk_present.swapchainCount &&
                                        swpchnCount < VKCONST_MAXPRESENTCOUNT_PERSUBMIT;
               swpchnIndx++) {
            swpchnIndices[swpchnCount] = submit->vk_present.pImageIndices[swpchnIndx];
            swpchns[swpchnCount]       = submit->vk_present.pSwapchains[swpchnIndx];

            swpchnCount++;
          }
        } // If queue call is wait/signal
        else if (submit->vk_submit.sType == VK_STRUCTURE_TYPE_SUBMIT_INFO) {
          printer(result_tgfx_NOTCODED, "Wait/Signal isn't supported in Present calls for now!");
        } // Queue call invalid
        else {
          printer(result_tgfx_FAIL, "One of the submits is invalid!");
        }
      }
      // Submit for timeline -> binary conversion
      {}

      VkPresentInfoKHR info = {};
      info.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      info.pNext            = nullptr;
      info.pSwapchains      = swpchns;
      info.swapchainCount   = swpchnCount;
      info.pImageIndices    = swpchnIndices;
      // For now, all calls are synchronized after each other
      // Because we don't have timeline semaphore emulation in binary semaphores
      info.waitSemaphoreCount = 1;
      info.pWaitSemaphores    = &queue->vk_callSynchronizer;
      info.pResults           = nullptr;
      ThrowIfFailed(vkQueuePresentKHR(queue->vk_queue, &info), "Queue Present has failed!");

      VkSemaphore binAcquireSemaphores[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {};
      for (uint32_t i = 0; i < swpchnCount; i++) {
        uint32_t swpchnIndx = 0;
        ThrowIfFailed(
          vkAcquireNextImageKHR(
            queue->m_gpu->vk_logical, swpchns[i], UINT64_MAX,
            core_vk->GETWINDOWs().getOBJfromHANDLE(windows[i])->binarySemaphores[swpchnIndices[i]],
            nullptr, &swpchnIndx),
          "Acquiring swapchain texture has failed!");
      }

      /*
      // Send a submit to signal timeline semaphore when all binary semaphore are signaled
      {
        VkPipelineStageFlags binWaitDstStageMasks[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkTimelineSemaphoreSubmitInfo temSignalSemaphoresInfo = {};
        temSignalSemaphoresInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        temSignalSemaphoresInfo.pWaitSemaphoreValues      = nullptr;
        temSignalSemaphoresInfo.waitSemaphoreValueCount   = 0;
        temSignalSemaphoresInfo.pNext                     = nullptr;
        temSignalSemaphoresInfo.signalSemaphoreValueCount = ;
        temSignalSemaphoresInfo.pSignalSemaphoreValues    = ;
        VkSubmitInfo acquireSubmit                        = {};
        acquireSubmit.waitSemaphoreCount                  = swpchnCount;
        acquireSubmit.pWaitSemaphores                     = binAcquireSemaphores;
        acquireSubmit.signalSemaphoreCount                = ;
        acquireSubmit.pSignalSemaphores                   = ;
        acquireSubmit.commandBufferCount                  = 0;
        acquireSubmit.pNext                               = nullptr;
        acquireSubmit.pWaitDstStageMask                   = binWaitDstStageMasks;
        acquireSubmit.sType                               = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        ThrowIfFailed(
          vkQueueSubmit(m_internalQueue->vk_queue, 1, &acquireSubmit, VK_NULL_HANDLE),
          "Internal queue submission for binary -> timeline semaphore conversion has failed!");
      }*/
    } break;
    default:
      printer(result_tgfx_NOTCODED, "Active queue operation type isn't supported by VK backend!");
      break;
  }
  queue->m_submitInfos.clear();
}
bool manager_vk::does_queuefamily_support(QUEUE_VKOBJ* family, const queueflag_vk& flag) {
  printer(result_tgfx_NOTCODED, "does_queuefamily_support() isn't coded");
  return false;
}