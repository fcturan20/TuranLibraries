#pragma once
#include <float.h>

#include "vk_extension.h"
#include "vk_predefinitions.h"

struct vkext_dynamicStates : public vkext_interface {
  vkext_dynamicStates(GPU_VKOBJ* gpu);
  virtual void inspect() override;
  virtual void manage(VkStructureType structType, void* structPtr,
                      extension_tgfx_handle extData) override;
  VkPhysicalDeviceExtendedDynamicStateFeaturesEXT    features1;
  VkPhysicalDeviceExtendedDynamicState2FeaturesEXT   features2;
  VkPhysicalDeviceExtendedDynamicState3FeaturesEXT   features3;
  VkPhysicalDeviceExtendedDynamicState3PropertiesEXT props3;
};

// These are TGFX CORE functionalities, so func pointers are here
typedef struct tgfx_raster_pipeline_description rasterPipelineDescription_tgfx;
typedef void (*vk_fillRasterPipelineStateInfoFnc)(GPU_VKOBJ* gpu, VkGraphicsPipelineCreateInfo* ci,
                                                  const rasterPipelineDescription_tgfx* desc,
                                                  extension_tgfxlsthnd                  exts);
extern vk_fillRasterPipelineStateInfoFnc vk_fillRasterPipelineStateInfo;

// vk_EXT_extended_dynamic_state
declareVkExtFunc(vkCmdSetCullModeEXT);
declareVkExtFunc(vkCmdSetDepthCompareOpEXT);
declareVkExtFunc(vkCmdSetDepthBoundsTestEnableEXT);
declareVkExtFunc(vkCmdSetDepthTestEnableEXT);
declareVkExtFunc(vkCmdSetDepthWriteEnableEXT);

// vk_EXT_extended_dynamic_state_2
declareVkExtFunc(vkCmdSetDepthBiasEnableEXT);

// vk_EXT_extended_dynamic_state_3
declareVkExtFunc(vkCmdSetDepthClampEnableEXT);
declareVkExtFunc(vkCmdSetDepthClipEnableEXT);
declareVkExtFunc(vkCmdSetColorWriteEnableEXT);
declareVkExtFunc(vkCmdSetPrimitiveTopologyEXT);
declareVkExtFunc(vkCmdSetStencilOpEXT);
declareVkExtFunc(vkCmdSetStencilTestEnableEXT);
