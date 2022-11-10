#include "vkext_timelineSemaphore.h"

#include "vk_core.h"
#include "vk_queue.h"
loadVkExtFunc(vkWaitSemaphoresKHR);

vkext_timelineSemaphore::vkext_timelineSemaphore(GPU_VKOBJ* gpu)
  : vkext_interface(gpu, &props, &features) {
  m_type         = timelineSemaphores_vkExtEnum;
  props.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES;
  props.pNext    = nullptr;
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
  features.pNext = nullptr;
}

fence_tgfxhnd vk_createTGFXFence(GPU_VKOBJ* gpu, uint64_t initValue) {
  vkext_timelineSemaphore* ext =
    ( vkext_timelineSemaphore* )gpu->ext()->m_exts[vkext_interface::timelineSemaphores_vkExtEnum];
  VkSemaphoreTypeCreateInfo timelineCi = {};
  timelineCi.initialValue              = initValue;
  timelineCi.pNext                     = nullptr;
  timelineCi.semaphoreType             = VK_SEMAPHORE_TYPE_TIMELINE;
  timelineCi.sType                     = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
  VkSemaphoreCreateInfo ci             = {};
  ci.flags                             = 0;
  ci.pNext                             = &timelineCi;
  ci.sType                             = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkSemaphore sem;
  ThrowIfFailed(vkCreateSemaphore(gpu->vk_logical, &ci, nullptr, &sem),
                "Timeline Semaphore creation has failed!");

  FENCE_VKOBJ* fence          = ext->fences.add();
  fence->vk_timelineSemaphore = sem;
  fence->m_curValue           = initValue;
  fence->m_gpuIndx            = gpu->gpuIndx();
  return ext->fences.returnHANDLEfromOBJ(fence);
}
static constexpr VkPipelineStageFlags vk_waitDstStageMask[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT * 2] =
  {VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
void vk_createSubmit_FenceWaitSignals(
  QUEUE_VKOBJ* queue, const FENCE_VKOBJ* waitFences[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT],
  const FENCE_VKOBJ* signalFences[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT]) {
  getTimelineSemaphoreEXT(queue->m_gpu, ext);
  submit_vk* submit             = queue->m_submitInfos.add();
  uint32_t&  waitSemaphoreCount = submit->vk_submit.waitSemaphoreCount;
  for (waitSemaphoreCount = 0; waitSemaphoreCount < VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT;
       waitSemaphoreCount++) {
    const FENCE_VKOBJ* fence = waitFences[waitSemaphoreCount];
    if (!fence) {
      break;
    }
    submit->vk_waitSemaphoreValues[waitSemaphoreCount] = fence->m_curValue;
    submit->vk_waitSemaphores[waitSemaphoreCount]      = fence->vk_timelineSemaphore;
  }
  uint32_t& signalSemaphoreCount = submit->vk_submit.signalSemaphoreCount;
  for (signalSemaphoreCount = 0; signalSemaphoreCount < VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT;
       signalSemaphoreCount++) {
    const FENCE_VKOBJ* fence = signalFences[signalSemaphoreCount];
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

loadVkExtFunc(vkGetSemaphoreCounterValueKHR);
#define findGPUfromFence(i_fence)                     \
  VKOBJHANDLE fenceHnd = *( VKOBJHANDLE* )&##i_fence; \
  GPU_VKOBJ*  gpu      = core_vk->getGPUs()[fenceHnd.EXTRA_FLAGs]

result_tgfx vk_getFenceValue(fence_tgfxhnd i_fence, unsigned long long* value) {
  findGPUfromFence(i_fence);

  getTimelineSemaphoreEXT(gpu, ext);
  FENCE_VKOBJ* fence = ext->fences.getOBJfromHANDLE(i_fence);

  THROW_RETURN_IF_FAIL(
    vkGetSemaphoreCounterValueKHR_loaded(gpu->vk_logical, fence->vk_timelineSemaphore, value),
    "Failed to get fence value!", result_tgfx_FAIL);
  return result_tgfx_SUCCESS;
}
loadVkExtFunc(vkSignalSemaphoreKHR);
result_tgfx vk_setFenceValue(fence_tgfxhnd i_fence, unsigned long long value) {
  findGPUfromFence(i_fence);
  getTimelineSemaphoreEXT(gpu, ext);
  FENCE_VKOBJ* fence = ext->fences.getOBJfromHANDLE(i_fence);

  VkSemaphoreSignalInfoKHR info = {};
  info.sType                    = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO_KHR;
  info.value                    = fence->m_nextValue.load();
  info.semaphore                = fence->vk_timelineSemaphore;
  info.pNext                    = nullptr;
  THROW_RETURN_IF_FAIL(vkSignalSemaphoreKHR_loaded(gpu->vk_logical, &info),
                       "Failed to signal fence on CPU!", result_tgfx_FAIL);
  return result_tgfx_SUCCESS;
}

void vkext_timelineSemaphore::inspect() {
  if (!features.timelineSemaphore) {
    // For now, don't create fence if timelineSemaphore isn't supported
    return;
  }

  m_gpu->ext()->m_activeDevExtNames.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
  vkWaitSemaphoresKHR_loaded           = vkWaitSemaphoresKHR_loadVkFunc();
  vkGetSemaphoreCounterValueKHR_loaded = vkGetSemaphoreCounterValueKHR_loadVkFunc();
  vkSignalSemaphoreKHR_loaded          = vkSignalSemaphoreKHR_loadVkFunc();
}

void vkext_timelineSemaphore::manage(VkStructureType structType, void* structPtr,
                                     extension_tgfx_handle extData) {
  switch (structType) {}
}