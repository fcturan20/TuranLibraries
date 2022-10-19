#include "vk_renderer.h"

#include "tgfx_renderer.h"
#include "vk_contentmanager.h"
#include "vk_core.h"
#include "vk_predefinitions.h"
#include "vk_queue.h"
#include "vkext_timelineSemaphore.h"

struct vk_commandBuffer {
  VkCommandBuffer CB;
};

struct vk_renderer_private {};

commandbuffer_tgfx_handle vk_beginCommandBuffer(gpuQueue_tgfxhnd queue, extension_tgfxlsthnd exts) {
  return nullptr;
}
void vk_executeBundles(commandbuffer_tgfx_handle commandBuffer, commandBundle_tgfxhnd* bundles,
                       const unsigned long long* bundleSortKeys, tgfx_rendererKeySortFunc sortFunc,
                       extension_tgfxlsthnd exts) {}
void vk_start_renderpass(commandbuffer_tgfx_handle commandBuffer, renderPass_tgfxhnd renderPass) {}
void vk_next_rendersubpass(commandbuffer_tgfx_handle commandBuffer,
                           renderSubPass_tgfxhnd     renderSubPass) {}
void vk_end_renderpass(commandbuffer_tgfx_handle commandBuffer) {}

#define getGPUfromQueueHnd(i_queue)                               \
  VKOBJHANDLE  queueHnd = *( VKOBJHANDLE* )&i_queue;              \
  GPU_VKOBJ*   gpu      = QUEUE_VKOBJ::getGPUfromHandle(i_queue); \
  QUEUEFAM_VK* fam      = QUEUE_VKOBJ::getFAMfromHandle(i_queue); \
  QUEUE_VKOBJ* queue    = fam->m_queues.getOBJfromHANDLE(i_queue);

void vk_queueExecuteCmdBuffers(gpuQueue_tgfxhnd i_queue, commandbuffer_tgfxlsthnd i_cmdBuffersList,
                               extension_tgfxlsthnd exts) {
  getGPUfromQueueHnd(i_queue);
  getTimelineSemaphoreEXT(gpu, semSys);

  VkCommandBuffer cmdBuffers[VKCONST_MAXCMDBUFFERCOUNT_PERSUBMIT] = {};
  uint32_t        cmdBufferCount                                  = 0;
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

  submit_vk* submit                      = queue->m_submitInfos.add();
  submit->vk_submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit->vk_submit.pNext                = nullptr;
  submit->vk_submit.signalSemaphoreCount = 1;
  submit->vk_submit.waitSemaphoreCount   = 1;
  submit->vk_submit.pWaitSemaphores      = &queue->vk_callSynchronizer;
  submit->vk_submit.pSignalSemaphores    = &queue->vk_callSynchronizer;
  submit->vk_submit.commandBufferCount   = cmdBufferCount;
  submit->vk_submit.pCommandBuffers      = cmdBuffers;
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
void vk_queuePresent(gpuQueue_tgfxhnd i_queue, const window_tgfxlsthnd windowlist,
                     const uint32_t* imageIndices) {
  getGPUfromQueueHnd(i_queue);

  if (queue->activeQueueOp != QUEUE_VKOBJ::ERROR_QUEUEOPTYPE &&
      queue->activeQueueOp != QUEUE_VKOBJ::PRESENT) {
    printer(result_tgfx_FAIL,
            "You can't call present while queue's active operation type is different (cmdbuffer or "
            "sparse)");
    return;
  }

  queue->activeQueueOp = QUEUE_VKOBJ::PRESENT;
  submit_vk* submit    = queue->m_submitInfos.add();

  
  uint32_t windowCount = 0;
  {
    TGFXLISTCOUNT(core_tgfx_main, windowlist, windowListSize);
    for (uint32_t i = 0; i < windowListSize; i++) {
      WINDOW_VKOBJ* window = core_vk->GETWINDOWs().getOBJfromHANDLE(windowlist[i]);
      if (!window) {
        continue;
      }
      submit->m_windows[windowCount] = window;
      window->m_swapchainCurrentTextureIndx = imageIndices[i];
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

// Synchronization Functions

void vk_createFences(gpu_tgfxhnd gpu, unsigned int fenceCount, uint32_t isSignaled,
                     fence_tgfxlsthnd fenceList) {
  GPU_VKOBJ* GPU = core_vk->getGPUs().getOBJfromHANDLE(gpu);
  for (uint32_t i = 0; i < fenceCount; i++) {
    fenceList[i] = vk_createTGFXFence(GPU, isSignaled);
  }
}

// Command Bundle Functions
////////////////////////////

commandBundle_tgfxhnd vk_beginCommandBundle(gpuQueue_tgfxhnd      queue,
                                            renderSubPass_tgfxhnd subpassHandle,
                                            extension_tgfxlsthnd  exts) {
  return nullptr;
}
void vk_finishCommandBundle(commandBundle_tgfxhnd bundle, rendererKeySortFunc_tgfx sortFunc,
                            extension_tgfxlsthnd exts) {}
void vk_cmdBindBindingTable(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                            bindingTable_tgfxhnd bindingtable) {}
void vk_cmdBindVertexBuffer(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                            buffer_tgfxhnd buffer, unsigned long long offset,
                            unsigned long long dataSize) {
  // BUFFER_VKOBJ* vb = contentmanager->GETBUFFER_ARRAY().getOBJfromHANDLE(buffer);
  //  vkCmdBindVertexBuffers();
}
void vk_cmdBindIndexBuffers(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                            buffer_tgfxhnd buffer, unsigned long long offset,
                            unsigned char IndexTypeSize) {}
void vk_cmdDrawNonIndexedDirect(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                                unsigned int vertexCount, unsigned int instanceCount,
                                unsigned int firstVertex, unsigned int firstInstance) {}
void vk_cmdDrawIndexedDirect(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                             unsigned int indexCount, unsigned int instanceCount,
                             unsigned int firstIndex, int vertexOffset,
                             unsigned int firstInstance) {}
void vk_cmdDrawNonIndexedIndirect(commandBundle_tgfxhnd, unsigned long long sortKey,
                                  buffer_tgfxhnd       drawDataBuffer,
                                  unsigned long long   drawDataBufferOffset,
                                  buffer_tgfxhnd       drawCountBuffer,
                                  unsigned long long   drawCountBufferOffset,
                                  extension_tgfxlsthnd exts) {}
void vk_cmdDrawIndexedIndirect(commandBundle_tgfxhnd, unsigned long long sortKey,
                               buffer_tgfxhnd       drawCountBuffer,
                               unsigned long long   drawCountBufferOffset,
                               extension_tgfxlsthnd exts) {}
void vk_destroyCommandBundle(commandBundle_tgfxhnd hnd) {}
void renderer_public::RendererResource_Finalizations() {
  /*
  // Handle RTSlotSet changes by recreating framebuffers of draw passes
  //  by "RTSlotSet changes": DrawPass' slotset is changed to different one or slotset's slots is
  changed. RenderGraph::DP_VK* DP = (RenderGraph::DP_VK*)VKGLOBAL_RG->GetNextPass(nullptr,
  VK_PASSTYPE::DP); while (DP) { unsigned char ChangeInfo = DP->SlotSetChanged.load();
    RTSLOTSET_VKOBJ* slotset =
  contentmanager->GETRTSLOTSET_ARRAY().getOBJbyINDEX(DP->BASESLOTSET_ID); if (ChangeInfo ==
  VKGLOBAL_FRAMEINDEX + 1 || ChangeInfo == 3 ||
  slotset->PERFRAME_SLOTSETs[VKGLOBAL_FRAMEINDEX].IsChanged.load()) { if
  (DP->FBs[VKGLOBAL_FRAMEINDEX]) {
        //This is safe because this FB is used 2 frames ago and CPU already waits for the frame's
  GPU process to end vkDestroyFramebuffer(rendergpu->devLogical, DP->FBs[VKGLOBAL_FRAMEINDEX],
  nullptr);
      }

      VkFramebufferCreateInfo fb_ci = slotset->FB_ci[VKGLOBAL_FRAMEINDEX];
      fb_ci.renderPass = DP->RenderPassObject;
      if (vkCreateFramebuffer(rendergpu->devLogical, &fb_ci, nullptr,
  &DP->FBs[VKGLOBAL_FRAMEINDEX]) != VK_SUCCESS) { printer(result_tgfx_FAIL, "vkCreateFramebuffer()
  has failed while changing one of the drawpasses' current frame slot's texture! Please report
  this!"); return;
      }

      slotset->PERFRAME_SLOTSETs[VKGLOBAL_FRAMEINDEX].IsChanged.store(false);
      for (unsigned int SlotIndex = 0; SlotIndex <
  slotset->PERFRAME_SLOTSETs[VKGLOBAL_FRAMEINDEX].COLORSLOTs_COUNT; SlotIndex++) {
        slotset->PERFRAME_SLOTSETs[VKGLOBAL_FRAMEINDEX].COLOR_SLOTs[SlotIndex].IsChanged.store(false);
      }

      if (ChangeInfo) {
        DP->SlotSetChanged.store(ChangeInfo - VKGLOBAL_FRAMEINDEX - 1);
      }
    }
  }*/
}
RGReconstructionStatus renderer_public::RGSTATUS() { return RGReconstructionStatus::Valid; }

void set_VkRenderer_funcPtrs() {
  core_tgfx_main->renderer->cmdBindBindingTable       = vk_cmdBindBindingTable;
  core_tgfx_main->renderer->cmdBindIndexBuffers       = vk_cmdBindIndexBuffers;
  core_tgfx_main->renderer->cmdBindVertexBuffer       = vk_cmdBindVertexBuffer;
  core_tgfx_main->renderer->cmdDrawIndexedDirect      = vk_cmdDrawIndexedDirect;
  core_tgfx_main->renderer->cmdDrawIndexedIndirect    = vk_cmdDrawIndexedIndirect;
  core_tgfx_main->renderer->cmdDrawNonIndexedDirect   = vk_cmdDrawNonIndexedDirect;
  core_tgfx_main->renderer->cmdDrawNonIndexedIndirect = vk_cmdDrawNonIndexedIndirect;
  core_tgfx_main->renderer->beginCommandBundle        = vk_beginCommandBundle;
  core_tgfx_main->renderer->finishCommandBundle       = vk_finishCommandBundle;
  core_tgfx_main->renderer->createFences              = vk_createFences;
  core_tgfx_main->renderer->destroyCommandBundle      = vk_destroyCommandBundle;
  core_tgfx_main->renderer->endRenderpass             = vk_end_renderpass;
  core_tgfx_main->renderer->executeBundles            = vk_executeBundles;
  core_tgfx_main->renderer->beginCommandBuffer        = vk_beginCommandBuffer;
  core_tgfx_main->renderer->nextRendersubpass         = vk_next_rendersubpass;
  core_tgfx_main->renderer->startRenderpass           = vk_start_renderpass;
  core_tgfx_main->renderer->queueExecuteCmdBuffers    = vk_queueExecuteCmdBuffers;
  core_tgfx_main->renderer->queueFenceSignalWait      = vk_queueFenceWaitSignal;
  core_tgfx_main->renderer->getFenceValue             = vk_getFenceValue;
  core_tgfx_main->renderer->setFence                  = vk_setFenceValue;
  core_tgfx_main->renderer->queueSubmit               = vk_queueSubmit;
  core_tgfx_main->renderer->queuePresent              = vk_queuePresent;
}

void vk_initRenderer() { set_VkRenderer_funcPtrs(); }