#pragma once
#include <float.h>

#include "vk_extension.h"
#include "vk_predefinitions.h"

struct FENCE_VKOBJ {
  bool            isALIVE    = false;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::FENCE;
  static uint16_t GET_EXTRAFLAGS(FENCE_VKOBJ* obj) { return obj->m_gpuIndx; }

  VkSemaphore          vk_timelineSemaphore;
  std::atomic_uint64_t m_curValue = 0, m_nextValue = 0;
  uint8_t              m_gpuIndx = 0;

  FENCE_VKOBJ& operator=(const FENCE_VKOBJ& src) {
    m_curValue.store(src.m_curValue.load());
    m_nextValue.store(src.m_nextValue.load());
    m_gpuIndx            = src.m_gpuIndx;
    vk_timelineSemaphore = src.vk_timelineSemaphore;
    isALIVE              = true;
    return *this;
  }
};

struct vkext_timelineSemaphore : public vkext_interface {
  vkext_timelineSemaphore(GPU_VKOBJ* gpu);
  virtual void inspect() override;
  virtual void manage(VkStructureType structType, void* structPtr,
                      extension_tgfx_handle extData) override;

  VkPhysicalDeviceTimelineSemaphoreFeatures   features;
  VkPhysicalDeviceTimelineSemaphoreProperties props;

  VK_STATICVECTOR<FENCE_VKOBJ, fence_tgfxhnd, 1 << 10> fences;
};

fence_tgfxhnd vk_createTGFXFence(GPU_VKOBJ* gpu, uint64_t initValue);

void vk_createSubmit_FenceWaitSignals(
  QUEUE_VKOBJ* queue, const FENCE_VKOBJ* waitFences[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT],
  const FENCE_VKOBJ* signalFences[VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT]);
result_tgfx vk_setFenceValue(fence_tgfxhnd fence, unsigned long long value);
result_tgfx vk_getFenceValue(fence_tgfxhnd fence, unsigned long long* value);

#define getTimelineSemaphoreEXT(gpu, extVarName) \
  vkext_timelineSemaphore* extVarName =          \
    ( vkext_timelineSemaphore* )gpu->ext()->m_exts[vkext_interface::timelineSemaphores_vkExtEnum]