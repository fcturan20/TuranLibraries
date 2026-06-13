#pragma once
#include <float.h>

#include "vk_extension.h"
#include "vk_predefinitions.h"

struct vkext_dynamicStates : public vkext_interface {
  vkext_dynamicStates(GPU_VKOBJ* gpu);
  virtual void inspect() override;
  virtual void manage(VkStructureType structType, void* structPtr, unsigned int extCount,
                      struct tgfx_extension* const* exts) override;
  VkPhysicalDeviceExtendedDynamicStateFeaturesEXT    features1;
  VkPhysicalDeviceExtendedDynamicState2FeaturesEXT   features2;
  VkPhysicalDeviceExtendedDynamicState3FeaturesEXT   features3;
  VkPhysicalDeviceMaintenance4Features               featuresMaintenance4;
  VkPhysicalDeviceExtendedDynamicState3PropertiesEXT props3;
};

// These are TGFX CORE functionalities, so func pointers are here
typedef struct tgfx_rasterPipelineDescription tgfx_rasterPipelineDescription;
typedef void (*vk_fillRasterPipelineStateInfoFnc)(GPU_VKOBJ* gpu, VkGraphicsPipelineCreateInfo* ci,
                                                  const tgfx_rasterPipelineDescription* desc,
                                                  unsigned int                          extCount,
                                                  struct tgfx_extension* const*              exts);
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
