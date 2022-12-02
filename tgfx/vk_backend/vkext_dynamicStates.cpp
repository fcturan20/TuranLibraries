#include "vkext_dynamicStates.h"

#include <string>

#include "tgfx_structs.h"
#include "vk_core.h"
#include "vk_includes.h"
#include "vk_predefinitions.h"
#include "vk_resource.h"

// vk_EXT_extended_dynamic_state
defineVkExtFunc(vkCmdSetCullModeEXT);
defineVkExtFunc(vkCmdSetDepthCompareOpEXT);
defineVkExtFunc(vkCmdSetDepthBoundsTestEnableEXT);
defineVkExtFunc(vkCmdSetDepthTestEnableEXT);
defineVkExtFunc(vkCmdSetDepthWriteEnableEXT);

// vk_EXT_extended_dynamic_state_2
defineVkExtFunc(vkCmdSetDepthBiasEnableEXT);

// vk_EXT_extended_dynamic_state_3
defineVkExtFunc(vkCmdSetDepthClampEnableEXT);
defineVkExtFunc(vkCmdSetDepthClipEnableEXT);
defineVkExtFunc(vkCmdSetColorWriteEnableEXT);
defineVkExtFunc(vkCmdSetPrimitiveTopologyEXT);
defineVkExtFunc(vkCmdSetStencilOpEXT);
defineVkExtFunc(vkCmdSetStencilTestEnableEXT);

vk_fillRasterPipelineStateInfoFnc vk_fillRasterPipelineStateInfo = {};
void vk_fillRasterPipelineStateInfo_dynamicState(GPU_VKOBJ* gpu, VkGraphicsPipelineCreateInfo* ci,
                                                 const rasterPipelineDescription_tgfx* desc,
                                                 extension_tgfxlsthnd                  exts) {
  /*
  auto* ext = ( vkext_dynamicStates* )gpu->ext()->m_exts[vkext_interface::dynamicStates_vkExtEnum];
  VkDynamicState* states     = *( VkDynamicState** )&ci->pDynamicState->pDynamicStates;
  uint32_t&       stateCount = *( uint32_t* )&ci->pDynamicState->dynamicStateCount;
  states[stateCount++]       = VK_DYNAMIC_STATE_CULL_MODE_EXT;
  states[stateCount++]       = VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT;
  states[stateCount++]       = VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT;
  states[stateCount++]       = VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE_EXT;
  states[stateCount++]       = VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT;
  if (ext->features2.extendedDynamicState2) {
    states[stateCount++] = VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE_EXT;
  }
  if (ext->features3.extendedDynamicState3DepthClampEnable &&
      ext->features3.extendedDynamicState3DepthClipEnable &&
      ext->features3.extendedDynamicState3PolygonMode &&
      ext->props3.dynamicPrimitiveTopologyUnrestricted) {
    states[stateCount++] = VK_DYNAMIC_STATE_DEPTH_CLAMP_ENABLE_EXT;
    states[stateCount++] = VK_DYNAMIC_STATE_DEPTH_CLIP_ENABLE_EXT;
    states[stateCount++] = VK_DYNAMIC_STATE_POLYGON_MODE_EXT;
    states[stateCount++] = VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT;
  }
  */
}

vkext_dynamicStates::vkext_dynamicStates(GPU_VKOBJ* gpu)
  : vkext_interface(gpu, &props3, &features1) {
  features1.pNext = &features2;
  features2.pNext = &features3;
  features1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
  features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
  features3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;

  props3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_PROPERTIES_EXT;
  props3.pNext = nullptr;
}
void vkext_dynamicStates::inspect() {
  if (!features1.extendedDynamicState) {
    return;
  }

  vk_fillRasterPipelineStateInfo = vk_fillRasterPipelineStateInfo_dynamicState;
  loadVkExtFunc(vkCmdSetCullModeEXT);
  loadVkExtFunc(vkCmdSetDepthCompareOpEXT);
  loadVkExtFunc(vkCmdSetDepthBoundsTestEnableEXT);
  loadVkExtFunc(vkCmdSetDepthTestEnableEXT);
  loadVkExtFunc(vkCmdSetDepthWriteEnableEXT);
  m_gpu->ext()->m_activeDevExtNames.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);

  if (features2.extendedDynamicState2) {
    loadVkExtFunc(vkCmdSetDepthBiasEnableEXT);
    m_gpu->ext()->m_activeDevExtNames.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
  }

  if (features3.extendedDynamicState3DepthClampEnable &&
      features3.extendedDynamicState3DepthClipEnable &&
      features3.extendedDynamicState3PolygonMode && props3.dynamicPrimitiveTopologyUnrestricted) {
    loadVkExtFunc(vkCmdSetDepthClampEnableEXT);
    loadVkExtFunc(vkCmdSetDepthClipEnableEXT);
    loadVkExtFunc(vkCmdSetColorWriteEnableEXT);
    loadVkExtFunc(vkCmdSetPrimitiveTopologyEXT);
    loadVkExtFunc(vkCmdSetStencilOpEXT);
    loadVkExtFunc(vkCmdSetStencilTestEnableEXT);
    m_gpu->ext()->m_activeDevExtNames.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
  }
}
void vkext_dynamicStates::manage(VkStructureType structType, void* structPtr,
                                 extension_tgfx_handle extData) {}