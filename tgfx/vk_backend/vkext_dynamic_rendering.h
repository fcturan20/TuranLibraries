#pragma once
#include <float.h>

#include "tgfx_structs.h"
#include "vk_extension.h"
#include "vk_predefinitions.h"

struct vkext_dynamicRendering : public vkext_interface {
  vkext_dynamicRendering(GPU_VKOBJ* gpu);
  virtual void inspect() override;
  virtual void manage(VkStructureType structType, void* structPtr,
                      extension_tgfx_handle extData) override;
  void         vk_beginRenderpass(VkCommandBuffer cb, unsigned int colorAttachmentCount,
                                  const rasterpassBeginSlotInfo_tgfx* colorAttachments,
                                  rasterpassBeginSlotInfo_tgfx depthAttachment, extension_tgfxlsthnd exts);
  void         vk_endRenderpass(VkCommandBuffer cb, extension_tgfxlsthnd exts);

  VkPhysicalDeviceDynamicRenderingFeatures features;
};
