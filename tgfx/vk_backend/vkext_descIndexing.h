#pragma once
#include <float.h>

#include "vk_extension.h"
#include "vk_predefinitions.h"

struct vkext_descIndexing : public vkext_interface {
  vkext_descIndexing(GPU_VKOBJ* gpu);
  virtual void                                 inspect() override;
  virtual void manage(VkStructureType structType, void* structPtr, unsigned int extCount,
                      const extension_tgfxhnd* exts) override;
  VkPhysicalDeviceDescriptorIndexingFeatures   features;
  VkPhysicalDeviceDescriptorIndexingProperties props;
};