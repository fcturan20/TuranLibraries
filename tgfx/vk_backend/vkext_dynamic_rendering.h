#pragma once
#include <float.h>

#include "tgfx_structs.h"
#include "vk_extension.h"
#include "vk_predefinitions.h"

struct vkext_dynamicRendering : public vkext_interface {
  vkext_dynamicRendering(GPU_VKOBJ* gpu);
  virtual void inspect() override;
  virtual void manage(VkStructureType structType, void* structPtr, unsigned int extCount,
                      struct tgfx_extension* const* exts) override;
  void         vk_beginRenderpass(VkCommandBuffer cb, unsigned int colorAttachmentCount,
                                  const tgfx_rasterpassBeginSlotInfo* colorAttachments,
                                  tgfx_rasterpassBeginSlotInfo depthAttachment, unsigned int extCount,
                                  struct tgfx_extension* const* exts);
  void vk_endRenderpass(VkCommandBuffer cb, unsigned int extCount, struct tgfx_extension* const* exts);

  VkPhysicalDeviceDynamicRenderingFeatures features;
};
