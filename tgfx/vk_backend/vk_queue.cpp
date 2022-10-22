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
  bool                 isALIVE  = false;
  VkCommandBuffer      vk_cb    = nullptr;
  VkCommandBufferLevel vk_level = VK_COMMAND_BUFFER_LEVEL_MAX_ENUM;
  bool                 m_isFree = false;
};

struct cmdPool_vk {
  cmdPool_vk() = default;
  cmdPool_vk(const cmdPool_vk& RefCP) { vk_cp = RefCP.vk_cp; }
  void          operator=(const cmdPool_vk& RefCP) { vk_cp = RefCP.vk_cp; }
  cmdBuffer_vk  createCmdBuffer();
  void          destroyCmdBuffer();
  VkCommandPool vk_cp = VK_NULL_HANDLE;

  VK_STATICVECTOR<cmdBuffer_vk, void*, 1 << 8> m_cbs;

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
    VKGLOBAL_VIRMEM_MANAGER = vk_virmem::allocate_dynamicmem(1 << 25);
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
                                        &m_queueFams[queuefamindex]->m_pools[i].vk_cp),
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
  cb_ai.commandPool                 = CP.vk_cp;
  cb_ai.level = isSecondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cb_ai.pNext = nullptr;
  cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

  cmdBuffer_vk* VK_CB = CP.m_cbs.add();
  if (vkAllocateCommandBuffers(m_gpu->vk_logical, &cb_ai, &VK_CB->vk_cb) != VK_SUCCESS) {
    printer(result_tgfx_FAIL,
            "vkAllocateCommandBuffers() failed while creating command buffers for RGBranches, "
            "report this please!");
    return nullptr;
  }
  VK_CB->vk_level = cb_ai.level;
  VK_CB->m_isFree = false;
  return VK_CB;
}

VkCommandBuffer manager_vk::get_commandbufferobj(cmdBuffer_vk* id) { return id->vk_cb; }

static constexpr VkPipelineStageFlags
  VKCONST_PRESENTWAITSTAGEs[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

void manager_vk::queueSubmit(QUEUE_VKOBJ* queue) {
  // Check previously sent submission to detect if they're still executing
  for (int32_t i = 0; i < queue->vk_submitTracker.size(); i++) {
    submission_vk* submission = queue->vk_submitTracker[i];
    if (!submission->isALIVE) {
      continue;
    }
    if (vkGetFenceStatus(queue->m_gpu->vk_logical, submission->vk_fence) == VK_SUCCESS) {
      for (uint32_t cbIndx = 0;
           cbIndx < VKCONST_MAXCMDBUFFERCOUNT_PERSUBMIT * VKCONST_MAXSUBMITCOUNT; cbIndx++) {
        if (submission->vk_cbs[cbIndx] == nullptr) {
          break;
        }
        submission->vk_cbs[cbIndx]->m_isFree = true;
      }
      vkDestroyFence(queue->m_gpu->vk_logical, submission->vk_fence, nullptr);
      // Index is int, so this works fine
      queue->vk_submitTracker.erase(i--);
    }
  }
  switch (queue->m_activeQueueOp) {
    case QUEUE_VKOBJ::ERROR_QUEUEOPTYPE: queue->m_activeQueueOp = QUEUE_VKOBJ::CMDBUFFER;
    case QUEUE_VKOBJ::CMDBUFFER: {
      VkSubmitInfo    infos[VKCONST_MAXSUBMITCOUNT]                                           = {};
      VkCommandBuffer cmdBuffers[VKCONST_MAXSUBMITCOUNT][VKCONST_MAXCMDBUFFERCOUNT_PERSUBMIT] = {};
      VkFence         submitFence                                                             = {};
      {
        VkFenceCreateInfo f_ci = {};
        f_ci.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        ThrowIfFailed(vkCreateFence(m_gpu->vk_logical, &f_ci, nullptr, &submitFence),
                      "Submission tracker fence creation failed!");
      }
      for (uint32_t submitIndx = 0; submitIndx < queue->m_submitInfos.size(); submitIndx++) {
        submit_vk* submit = queue->m_submitInfos[submitIndx];
        // Add queue call synchronizer semaphore as wait to sync sequential executeCmdLists calls
        if (submitIndx == 0 && queue->m_prevQueueOp == QUEUE_VKOBJ::CMDBUFFER) {
          submit->vk_waitSemaphores[submit->vk_submit.waitSemaphoreCount++] =
            queue->vk_callSynchronizer;
          submit->vk_waitSemaphoreValues[submit->vk_semaphoreInfo.waitSemaphoreValueCount++] = 0;
        }
        // Add queue call synchronizer semaphore as signal to the last submit
        if (submitIndx == queue->m_submitInfos.size() - 1) {
          submit->vk_signalSemaphores[submit->vk_submit.signalSemaphoreCount++] =
            queue->vk_callSynchronizer;
          submit->vk_signalSemaphoreValues[submit->vk_semaphoreInfo.signalSemaphoreValueCount++] =
            0;
        }
        for (uint32_t cmdBufferIndx = 0; cmdBufferIndx < submit->vk_submit.commandBufferCount;
             cmdBufferIndx++) {
          cmdBuffers[submitIndx][cmdBufferIndx] = submit->cmdBuffers[cmdBufferIndx]->vk_cb;
        }
        // submit->vk_submit.pCommandBuffers = cmdBuffers[submitIndx];
        infos[submitIndx] = submit->vk_submit;
      }
      ThrowIfFailed(vkQueueSubmit(queue->vk_queue, queue->m_submitInfos.size(), infos, submitFence),
                    "Queue Submission has failed!");

      // Add this submit call to tracker
      submission_vk* tracker  = queue->vk_submitTracker.add();
      tracker->vk_fence       = submitFence;
      uint32_t cmdBufferCount = 0;
      for (uint32_t submitIndx = 0; submitIndx < queue->m_submitInfos.size(); submitIndx++) {
        submit_vk* submit = queue->m_submitInfos[submitIndx];
        for (uint32_t cmdBufferIndx = 0; cmdBufferIndx < submit->vk_submit.commandBufferCount;
             cmdBufferIndx++) {
          tracker->vk_cbs[cmdBufferCount++] = submit->cmdBuffers[cmdBufferIndx];
        }
      }
    } break;
    case QUEUE_VKOBJ::PRESENT: {
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

      for (uint32_t submitIndx = 0; submitIndx < queue->m_submitInfos.size(); submitIndx++) {
        submit_vk* submit = queue->m_submitInfos[submitIndx];

        // If queue call is present
        if (submit->vk_present.sType == VK_STRUCTURE_TYPE_PRESENT_INFO_KHR) {
          if (swpchnCount + submit->vk_present.swapchainCount > VKCONST_MAXSWPCHNCOUNT_PERSUBMIT) {
            printer(result_tgfx_FAIL, "Max swapchain count per submit is exceeded!");
            break;
          }
          for (uint32_t swpchnIndx = 0; swpchnIndx < submit->vk_present.swapchainCount &&
                                        swpchnCount < VKCONST_MAXSWPCHNCOUNT_PERSUBMIT;
               swpchnIndx++) {
            swpchnIndices[swpchnCount] =
              submit->m_windows[swpchnIndx]->m_swapchainCurrentTextureIndx;
            swpchns[swpchnCount] = submit->m_windows[swpchnIndx]->vk_swapchain;
            windows[swpchnCount] = submit->m_windows[swpchnIndx];
            generalToPresent[swpchnCount] =
              submit->m_windows[swpchnIndx]
                ->vk_generalToPresent[queue->vk_queueFamIndex][swpchnIndices[swpchnCount]];
            presentToGeneral[swpchnCount] =
              submit->m_windows[swpchnIndx]
                ->vk_presentToGeneral[queue->vk_queueFamIndex][swpchnIndices[swpchnCount]];

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
          printer(result_tgfx_FAIL, "One of the submits is invalid!");
        }
      }

      VkSemaphore binAcquireSemaphores[VKCONST_MAXSWPCHNCOUNT_PERSUBMIT] = {};
      for (uint32_t i = 0; i < swpchnCount; i++) {
        binAcquireSemaphores[i] = windows[i]->vk_acquireSemaphore;
      }

      VkSemaphore binarySignalSemaphores[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {};
      // Submit for timeline -> binary conversion
      {
        // TODO: Use vk_synchronization.h to get unsignaled semaphores, because of semaphore leak
        // (these semaphores aren't destroyed)
        for (uint32_t i = 0; i < waitSemCount; i++) {
          VkSemaphoreCreateInfo ci = {};
          ci.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
          ThrowIfFailed(
            vkCreateSemaphore(m_gpu->vk_logical, &ci, nullptr, &binarySignalSemaphores[i]),
            "Present's Timeline->Binary converter failed at binary semaphore creation!");
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
        si.pNext                              = nullptr;
        si.commandBufferCount                 = swpchnCount;
        si.pCommandBuffers                    = generalToPresent;
        si.waitSemaphoreCount                 = waitSemCount;
        si.pWaitDstStageMask                  = VKCONST_PRESENTWAITSTAGEs;
        si.pWaitSemaphores                    = waitSemaphores;
        if (queue->m_prevQueueOp == QUEUE_VKOBJ::CMDBUFFER) {
          si.waitSemaphoreCount = waitSemCount + 1;
          timInfo.waitSemaphoreValueCount++;
          waitValues[waitSemCount]     = 0;
          waitSemaphores[waitSemCount] = queue->vk_callSynchronizer;
        }
        si.signalSemaphoreCount = waitSemCount;
        si.pSignalSemaphores    = binarySignalSemaphores;
        ThrowIfFailed(vkQueueSubmit(queue->vk_queue, 1, &si, VK_NULL_HANDLE),
                      "Present's Timeline->Binary converter failed at queue submission!");
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
      if (info.swapchainCount == 0) {
        assert(0);
      }
      ThrowIfFailed(vkQueuePresentKHR(queue->vk_queue, &info), "Queue Present has failed!");

      // Send a submit to signal timeline semaphore when all binary semaphore are signaled
      {
        VkTimelineSemaphoreSubmitInfo temSignalSemaphoresInfo = {};
        temSignalSemaphoresInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        temSignalSemaphoresInfo.pWaitSemaphoreValues      = nullptr;
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
        ThrowIfFailed(
          vkQueueSubmit(m_internalQueue->vk_queue, 1, &acquireSubmit, VK_NULL_HANDLE),
          "Internal queue submission for binary -> timeline semaphore conversion has failed!");
      }
    } break;
    default:
      printer(result_tgfx_NOTCODED, "Active queue operation type isn't supported by VK backend!");
      break;
  }
  queue->m_submitInfos.clear();
  queue->m_prevQueueOp   = queue->m_activeQueueOp;
  queue->m_activeQueueOp = QUEUE_VKOBJ::ERROR_QUEUEOPTYPE;
}
bool manager_vk::does_queuefamily_support(QUEUE_VKOBJ* family, const queueflag_vk& flag) {
  printer(result_tgfx_NOTCODED, "does_queuefamily_support() isn't coded");
  return false;
}