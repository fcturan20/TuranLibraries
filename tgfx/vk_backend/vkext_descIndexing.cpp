#include "vkext_descIndexing.h"

#include "vk_core.h"

vkext_descIndexing::vkext_descIndexing(GPU_VKOBJ* gpu) : vkext_interface(gpu, &props, &features) {
  m_type         = descIndexing_vkExtEnum;
  props.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;
  props.pNext    = nullptr;
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
  features.pNext = nullptr;
};

void vkext_descIndexing::inspect() {
  if (!features.descriptorBindingPartiallyBound) {
    return;
  }

  m_gpu->ext()->m_activeDevExtNames.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
  m_gpu->ext()->m_activeDevExtNames.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
}
void vkext_descIndexing::manage(VkStructureType structType, void* structPtr,
                                extension_tgfx_handle extData) {}