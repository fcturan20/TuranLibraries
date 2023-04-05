#pragma once
#include <float.h>

#include "vk_extension.h"
#include "vk_predefinitions.h"

struct FENCE_VKOBJ {
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::FENCE;
  static uint16_t GET_EXTRAFLAGS(FENCE_VKOBJ* obj) { return obj->m_gpuIndx; }
  FENCE_VKOBJ& FENCE_VKOBJ::operator=(const FENCE_VKOBJ& src) {
    vk_fence             = src.vk_fence;
    vk_timelineSemaphore = src.vk_timelineSemaphore;
    m_curValue.store(src.m_curValue.load());
    m_nextValue.store(src.m_nextValue.load());
    m_gpuIndx = src.m_gpuIndx;
    return *this;
  }

  VkFence              vk_fence; // !-< Mostly unused (in timeline semaphores)
  VkSemaphore          vk_timelineSemaphore;
  std::atomic_uint64_t m_curValue = 0, m_nextValue = 0;
  uint8_t              m_gpuIndx = 0;
};

struct vkext_timelineSemaphore : public vkext_interface {
  vkext_timelineSemaphore(GPU_VKOBJ* gpu);
  virtual void inspect() override;
  virtual void manage(VkStructureType structType, void* structPtr, unsigned int extCount,
                      const extension_tgfxhnd* exts) override;

  VkPhysicalDeviceTimelineSemaphoreFeatures   features;
  VkPhysicalDeviceTimelineSemaphoreProperties props;

  VK_LINEAR_OBJARRAY<FENCE_VKOBJ, fence_tgfxhnd, 1 << 10> fences;
};

fence_tgfxhnd vk_createTGFXFence(GPU_VKOBJ* gpu, uint64_t initValue);

result_tgfx vk_setFenceValue(fence_tgfxhnd fence, unsigned long long value);
result_tgfx vk_getFenceValue(fence_tgfxhnd fence, unsigned long long* value);

#define getTimelineSemaphoreEXT(gpu, extVarName) \
  vkext_timelineSemaphore* extVarName =          \
    ( vkext_timelineSemaphore* )gpu->ext()->m_exts[vkext_interface::timelineSemaphores_vkExtEnum]