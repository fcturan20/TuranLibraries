/*
  This file is responsible for managing TGFX Command Buffer sending queue operations to GPU and.
  TGFX Command Buffer = Primary Command Buffer.
  TGFX GPU QUEUE = VkQueue

*/

#include "vk_queue.h"

#include <threadingsys_tapi.h>

#include <mutex>

#include "tgfx_renderer.h"
#include "vk_core.h"
#include "vk_renderer.h"
#include "vkext_timelineSemaphore.h"

vk_uint32c             VKCONST_MAXFENCECOUNT_PERSUBMIT = 8, VKCONST_MAXCMDBUFFER_PRIMARY_COUNT = 32;
vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_MANAGER = nullptr;

queueflag_vk::operator uint8_t() const {
  if constexpr (sizeof(queueflag_vk) == 1) {
    return *(( uint8_t* )this);
  } else {
    printer(result_tgfx_NOTCODED, "queueflag_vk shouldn't be more than 1 byte");
  }
  return 0;
};
struct CMDBUFFER_VKOBJ {
  bool            isALIVE    = false;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::CMDBUFFER;
  static uint16_t GET_EXTRAFLAGS(CMDBUFFER_VKOBJ* obj) {
    return (uint16_t(obj->m_gpuIndx) << 8) | (uint16_t(obj->m_cmdPoolIndx));
  }
  static GPU_VKOBJ* getGPUfromHandle(commandBuffer_tgfxhnd hnd) {
    VKOBJHANDLE handle  = *( VKOBJHANDLE* )&hnd;
    uint32_t    gpuIndx = handle.EXTRA_FLAGs >> 8;
  }
  static uint32_t getCmdPoolIndxFromHandle(commandBuffer_tgfxhnd hnd) {
    VKOBJHANDLE handle = *( VKOBJHANDLE* )&hnd;
    return (uint32_t(handle.EXTRA_FLAGs) << 24) >> 24;
  }
  VkCommandBuffer vk_cb     = nullptr;
  uint8_t         m_gpuIndx = 0, m_queueFamIndx = 0, m_cmdPoolIndx = 0;
};

struct cmdPool_vk {
  cmdPool_vk() = default;
  void operator=(const cmdPool_vk& RefCP) {
    vk_primaryCP   = RefCP.vk_primaryCP;
    vk_secondaryCP = RefCP.vk_secondaryCP;
  }
  VkCommandPool vk_primaryCP = VK_NULL_HANDLE, vk_secondaryCP = VK_NULL_HANDLE;

  VK_STATICVECTOR<CMDBUFFER_VKOBJ, commandBuffer_tgfxhnd, VKCONST_MAXCMDBUFFER_PRIMARY_COUNT>
    m_cbsPrimary;

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
  uint32_t    queueFamIndx = (uint32_t(handle.EXTRA_FLAGs) << 24) >> 24;
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

void vk_allocateCmdBuffer(QUEUEFAM_VK* queueFam, VkCommandBufferLevel level, VkCommandBuffer* cbs,
                          uint32_t count) {
  VkCommandBufferAllocateInfo cb_ai = {};
  cb_ai.pNext                       = nullptr;
  cb_ai.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cb_ai.commandBufferCount          = count;
  cb_ai.level                       = level;
  if (level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    cb_ai.commandPool = queueFam->m_pools[threadingsys->this_thread_index()].vk_primaryCP;
  } else {
    cb_ai.commandPool = queueFam->m_pools[threadingsys->this_thread_index()].vk_secondaryCP;
  }
  ThrowIfFailed(vkAllocateCommandBuffers(queueFam->m_gpu->vk_logical, &cb_ai, cbs),
                "vkAllocateCommandBuffers() failed while creating a command buffer, "
                "report this please!");
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
  for (unsigned int queueFamIndx = 0; queueFamIndx < m_queueFams.size(); queueFamIndx++) {
    QUEUEFAM_VK* queueFam = m_queueFams[queueFamIndx];
    if (!(queueFam->m_supportFlag.is_COMPUTEsupported ||
          queueFam->m_supportFlag.is_GRAPHICSsupported ||
          queueFam->m_supportFlag.is_TRANSFERsupported)) {
      continue;
    }
    queueFam->m_pools = new (VKGLOBAL_VIRMEM_MANAGER) cmdPool_vk[threadcount];
    for (unsigned char threadIndx = 0; threadIndx < threadcount; threadIndx++) {
      cmdPool_vk&             CP      = queueFam->m_pools[threadIndx];
      VkCommandPoolCreateInfo cp_ci_g = {};
      cp_ci_g.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      cp_ci_g.queueFamilyIndex        = queueFamIndx;
      cp_ci_g.pNext                   = nullptr;

      // Create primary Command Pool
      {
        cp_ci_g.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        ThrowIfFailed(vkCreateCommandPool(m_gpu->vk_logical, &cp_ci_g, nullptr, &CP.vk_primaryCP),
                      "Primary command pool creation for a queue has failed!");
      }

      {
        cp_ci_g.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        ThrowIfFailed(vkCreateCommandPool(m_gpu->vk_logical, &cp_ci_g, nullptr, &CP.vk_secondaryCP),
                      "Secondary command pool creation for a queue has failed!");
      }

      // Allocate Primary Command Buffers
      {
        VkCommandBuffer primaryCBs[VKCONST_MAXCMDBUFFER_PRIMARY_COUNT];
        vk_allocateCmdBuffer(queueFam, VK_COMMAND_BUFFER_LEVEL_PRIMARY, primaryCBs,
                             VKCONST_MAXCMDBUFFER_PRIMARY_COUNT);

        for (uint32_t cmdBufferIndx = 0; cmdBufferIndx < VKCONST_MAXCMDBUFFER_PRIMARY_COUNT;
             cmdBufferIndx++) {
          CMDBUFFER_VKOBJ* cb = CP.m_cbsPrimary.add();
          cb->isALIVE         = false;
          cb->vk_cb           = primaryCBs[cmdBufferIndx];
          cb->m_gpuIndx       = m_gpu->gpuIndx();
          cb->m_cmdPoolIndx   = threadIndx;
          cb->m_queueFamIndx  = queueFamIndx;
        }
      }
    }
  }
}
uint32_t        manager_vk::get_queuefam_index(QUEUE_VKOBJ* fam) { return fam->vk_queueFamIndex; }
VkCommandBuffer manager_vk::getPrimaryCmdBuffer(QUEUEFAM_VK* family) {
  cmdPool_vk& CP = family->m_pools[threadingsys->this_thread_index()];
  for (uint32_t i = 0; i < CP.m_cbsPrimary.size(); i++) {
    if (CP.m_cbsPrimary[i]->isALIVE) {
      continue;
    }
    return CP.m_cbsPrimary[i]->vk_cb;
  }
  assert(0 && "Command Buffer Count is exceeded!");
}

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
        submission->vk_cbs[cbIndx]->isALIVE = false;
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

// Renderer Command Buffer Functionality

commandBuffer_tgfxhnd vk_beginCommandBuffer(gpuQueue_tgfxhnd i_queue, extension_tgfxlsthnd exts) {
  getGPUfromQueueHnd(i_queue);
  QUEUEFAM_VK* queueFam = gpu->manager()->m_queueFams[queue->vk_queueFamIndex];
  cmdPool_vk&  cmdPool  = queueFam->m_pools[threadingsys->this_thread_index()];
  for (uint32_t cmdBufferIndx = 0; cmdBufferIndx < cmdPool.m_cbsPrimary.max(); cmdBufferIndx++) {
    CMDBUFFER_VKOBJ* cmdBuffer = cmdPool.m_cbsPrimary[cmdBufferIndx];
    if (!cmdBuffer->isALIVE) {
      cmdBuffer->isALIVE = true;

      VkCommandBufferBeginInfo bi = {};
      bi.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      bi.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      if (ThrowIfFailed(vkBeginCommandBuffer(cmdBuffer->vk_cb, &bi),
                        "VkBeginCommandBuffer() failed!")) {
        return nullptr;
      }

      return cmdPool.m_cbsPrimary.returnHANDLEfromOBJ(cmdBuffer);
    }
  }
  printer(result_tgfx_FAIL,
          "No empty command buffer found! Increase the primary cb count in VK backend!");
  return nullptr;
}

#define getCmdBufferfromHnd(cmdBufferHnd)                      \
  VKOBJHANDLE      hnd       = *( VKOBJHANDLE* )&cmdBufferHnd; \
  CMDBUFFER_VKOBJ* cmdBuffer = ( CMDBUFFER_VKOBJ* )VK_MEMOFFSET_TO_POINTER(hnd.OBJ_memoffset);

#define checkCmdBufferHnd()                                                                       \
  GPU_VKOBJ* gpu = core_vk->getGPUs()[cmdBuffer->m_gpuIndx];                                      \
  assert(gpu && "GPU isn't found, wrong commandBuffer_tgfxhnd in vk_endCommandBuffer");           \
  QUEUEFAM_VK* queueFam = gpu->manager()->m_queueFams[cmdBuffer->m_queueFamIndx];                 \
  assert(queueFam && "QueueFam isn't found, wrong commandBuffer_tgfxhnd in vk_endCommandBuffer"); \
  cmdPool_vk& cmdPool = queueFam->m_pools[cmdBuffer->m_cmdPoolIndx];                              \
  assert(cmdPool.m_cbsPrimary.returnHANDLEfromOBJ(cmdBuffer) &&                                   \
         "CmdPool failed to find, wrong commandBuffer_tgfxhnd in vk_endCommandBuffer");

void vk_endCommandBuffer(commandBuffer_tgfxhnd cb) {
  getCmdBufferfromHnd(cb);
#ifdef VULKAN_DEBUGGING
  checkCmdBufferHnd();
#endif
  ThrowIfFailed(vkEndCommandBuffer(cmdBuffer->vk_cb), "VkEndCommandBuffer() failed!");
}
void vk_executeBundles(commandBuffer_tgfxhnd cb, commandBundle_tgfxlsthnd bundles,
                       tgfx_rendererKeySortFunc sortFnc, const unsigned long long* bundleSortKeys,
                       void* userData, extension_tgfxlsthnd exts) {
  getCmdBufferfromHnd(cb);
#ifdef VULKAN_DEBUGGING
  checkCmdBufferHnd();
#endif

  uint32_t        cmdBundleCount                              = 0;
  VkCommandBuffer secCmdBuffers[VKCONST_MAXCMDBUNDLE_PERCALL] = {};
  vk_GetSecondaryCmdBuffers(bundles, secCmdBuffers, &cmdBundleCount);
  assert(cmdBundleCount <= VKCONST_MAXCMDBUNDLE_PERCALL &&
         "vk_GetSecondaryCmdBuffers() can't exceed VKCONST_MAXCMDBUNDLE_PERCALL!");
  if (!cmdBundleCount) {
    return;
  }
  vkCmdExecuteCommands(cmdBuffer->vk_cb, cmdBundleCount, secCmdBuffers);
}
void vk_startRenderpass(commandBuffer_tgfxhnd commandBuffer, renderPass_tgfxhnd renderPass) {}
void vk_nextRendersubpass(commandBuffer_tgfxhnd commandBuffer,
                          renderSubPass_tgfxhnd renderSubPass) {}
void vk_endRenderpass(commandBuffer_tgfxhnd commandBuffer) {}

void vk_queueExecuteCmdBuffers(gpuQueue_tgfxhnd i_queue, commandBuffer_tgfxlsthnd i_cmdBuffersList,
                               extension_tgfxlsthnd exts) {
  getGPUfromQueueHnd(i_queue);
  getTimelineSemaphoreEXT(gpu, semSys);

  uint32_t cmdBufferCount = 0;
  {
    TGFXLISTCOUNT(core_tgfx_main, i_cmdBuffersList, listSize);
    if (listSize > VKCONST_MAXCMDBUFFERCOUNT_PERSUBMIT) {
      printer(result_tgfx_FAIL,
              "Vulkan backend only supports limited cmdBuffers to be executed in the same call! "
              "Please report this");
    }
    for (uint32_t i = 0; i < listSize; i++) {
      if (i_cmdBuffersList[i]) {
        // cmdBuffers[cmdBufferCount++] = findAndSetCmdBufferObj(i_cmdBuffersList[i]);
      }
    }
  }
  if (!cmdBufferCount) {
    return;
  }

  submit_vk* submit                      = queue->m_submitInfos.add();
  submit->vk_submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit->vk_submit.pNext                = nullptr;
  submit->vk_submit.signalSemaphoreCount = 1;
  submit->vk_submit.waitSemaphoreCount   = 1;
  submit->vk_submit.pWaitSemaphores      = &queue->vk_callSynchronizer;
  submit->vk_submit.pSignalSemaphores    = &queue->vk_callSynchronizer;
  submit->vk_submit.commandBufferCount   = cmdBufferCount;
}
void vk_queueFenceWaitSignal(gpuQueue_tgfxhnd i_queue, fence_tgfxlsthnd waitFences,
                             const unsigned long long* waitValues, fence_tgfxlsthnd signalFences,
                             const unsigned long long* signalValues) {
  getGPUfromQueueHnd(i_queue);
  getTimelineSemaphoreEXT(gpu, semSys);
  const FENCE_VKOBJ* waits[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {};
  {
    uint32_t waitCount = 0;
    TGFXLISTCOUNT(core_tgfx_main, waitFences, waitListSize);
    if (waitListSize > VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT) {
      printer(result_tgfx_FAIL, "Max semaphore count per submit is exceeded!");
      return;
    }
    for (uint32_t i = 0; i < waitListSize; i++) {
      FENCE_VKOBJ* wait = semSys->fences.getOBJfromHANDLE(waitFences[i]);
      if (!wait) {
        continue;
      }
      waits[waitCount++] = wait;
      wait->m_curValue.store(waitValues[i]);
    }
  }
  const FENCE_VKOBJ* signals[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT] = {};
  {
    uint32_t signalCount = 0;
    TGFXLISTCOUNT(core_tgfx_main, signalFences, signalListSize);
    if (signalListSize > VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT) {
      printer(result_tgfx_FAIL, "Max semaphore count per submit is exceeded!");
      return;
    }
    for (uint32_t i = 0; i < signalListSize; i++) {
      FENCE_VKOBJ* signal = semSys->fences.getOBJfromHANDLE(signalFences[i]);
      if (!signal) {
        continue;
      }
      signals[signalCount++] = signal;
      signal->m_nextValue.store(signalValues[i]);
    }
  }
  vk_createSubmit_FenceWaitSignals(queue, waits, signals);
}
void vk_queueSubmit(gpuQueue_tgfxhnd i_queue) {
  getGPUfromQueueHnd(i_queue);
  gpu->manager()->queueSubmit(queue);
}
void vk_queuePresent(gpuQueue_tgfxhnd i_queue, const window_tgfxlsthnd windowlist) {
  getGPUfromQueueHnd(i_queue);

  if (queue->m_activeQueueOp != QUEUE_VKOBJ::ERROR_QUEUEOPTYPE &&
      queue->m_activeQueueOp != QUEUE_VKOBJ::PRESENT) {
    printer(result_tgfx_FAIL,
            "You can't call present while queue's active operation type is different (cmdbuffer or "
            "sparse)");
    return;
  }

  queue->m_activeQueueOp = QUEUE_VKOBJ::PRESENT;
  submit_vk* submit      = queue->m_submitInfos.add();

  uint32_t windowCount = 0;
  {
    TGFXLISTCOUNT(core_tgfx_main, windowlist, windowListSize);
    for (uint32_t i = 0; i < windowListSize; i++) {
      WINDOW_VKOBJ* window = core_vk->GETWINDOWs().getOBJfromHANDLE(windowlist[i]);
      if (!window) {
        continue;
      }
      submit->m_windows[windowCount] = window;
      windowCount++;
    }
  }

  submit->vk_present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  submit->vk_present.pNext              = nullptr;
  submit->vk_present.pWaitSemaphores    = nullptr;
  submit->vk_present.waitSemaphoreCount = 0;
  submit->vk_present.pResults           = nullptr;
  submit->vk_present.pImageIndices      = nullptr;
  submit->vk_present.pSwapchains        = nullptr;
  submit->vk_present.swapchainCount     = windowCount;
}

void vk_setQueueFuncPtrs() {
  core_tgfx_main->renderer->beginCommandBuffer     = vk_beginCommandBuffer;
  core_tgfx_main->renderer->endCommandBuffer       = vk_endCommandBuffer;
  core_tgfx_main->renderer->executeBundles         = vk_executeBundles;
  core_tgfx_main->renderer->startRenderpass        = vk_startRenderpass;
  core_tgfx_main->renderer->nextRendersubpass      = vk_nextRendersubpass;
  core_tgfx_main->renderer->queueExecuteCmdBuffers = vk_queueExecuteCmdBuffers;
  core_tgfx_main->renderer->queueFenceSignalWait   = vk_queueFenceWaitSignal;
  core_tgfx_main->renderer->queueSubmit            = vk_queueSubmit;
  core_tgfx_main->renderer->queuePresent           = vk_queuePresent;
}