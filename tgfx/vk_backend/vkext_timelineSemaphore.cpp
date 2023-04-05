#include "vkext_timelineSemaphore.h"

#include "vk_core.h"
#include "vk_queue.h"

declareVkExtFunc(vkWaitSemaphoresKHR);
declareVkExtFunc(vkGetSemaphoreCounterValueKHR);
declareVkExtFunc(vkSignalSemaphoreKHR);
defineVkExtFunc(vkWaitSemaphoresKHR);
defineVkExtFunc(vkGetSemaphoreCounterValueKHR);
defineVkExtFunc(vkSignalSemaphoreKHR);

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

  FENCE_VKOBJ* fence          = ext->fences.create_OBJ();
  fence->vk_timelineSemaphore = sem;
  fence->m_curValue           = initValue;
  fence->m_gpuIndx            = gpu->gpuIndx();
  return getHANDLE<fence_tgfxhnd>(fence);
}

#define findGPUfromFence(i_fence)                    \
  FENCE_VKOBJ* fence = getOBJ<FENCE_VKOBJ>(i_fence); \
  GPU_VKOBJ*   gpu   = core_vk->getGPU(fence->m_gpuIndx)

result_tgfx vk_getFenceValue(fence_tgfxhnd i_fence, unsigned long long* value) {
  findGPUfromFence(i_fence);

  getTimelineSemaphoreEXT(gpu, ext);

  THROW_RETURN_IF_FAIL(
    vkGetSemaphoreCounterValueKHR_loaded(gpu->vk_logical, fence->vk_timelineSemaphore, value),
    "Failed to get fence value!", result_tgfx_FAIL);
  return result_tgfx_SUCCESS;
}
result_tgfx vk_setFenceValue(fence_tgfxhnd i_fence, unsigned long long value) {
  findGPUfromFence(i_fence);
  getTimelineSemaphoreEXT(gpu, ext);

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

  m_gpu->ext()->m_activeDevExtNames[m_gpu->ext()->m_devExtCount++] =
    (VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
  loadVkExtFunc(vkWaitSemaphoresKHR);
  loadVkExtFunc(vkGetSemaphoreCounterValueKHR);
  loadVkExtFunc(vkSignalSemaphoreKHR);
}

void vkext_timelineSemaphore::manage(VkStructureType structType, void* structPtr,
                                     unsigned int extCount, const extension_tgfxhnd* exts) {
  switch (structType) {}
}