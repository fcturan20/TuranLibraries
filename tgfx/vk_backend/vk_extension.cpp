#include "vk_extension.h"

#include <tgfx_forwarddeclarations.h>

#include <atomic>
#include <glm/glm.hpp>

#include "vk_core.h"
#include "vk_includes.h"
#include "vk_resource.h"

// Extensions
#include "vkext_depthstencil.h"
#include "vkext_descIndexing.h"
#include "vkext_timelineSemaphore.h"
#include "vkext_dynamic_rendering.h"
#include "vkext_dynamicStates.h"
vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_EXTS = nullptr;

vkext_interface::vkext_interface(GPU_VKOBJ* gpu, void* propsStruct, void* featuresStruct) {
  m_gpu = gpu;
  if (propsStruct) {
    pNext_addToLast(&gpu->vk_propsDev, propsStruct);
  }
  if (featuresStruct) {
    pNext_addToLast(&gpu->vk_featuresDev, featuresStruct);
  }
}

void extManager_vkDevice::createExtManager(GPU_VKOBJ* gpu) {
  if (!VKGLOBAL_VIRMEM_EXTS) {
    // 4MB is fine i guess?
    VKGLOBAL_VIRMEM_EXTS = vk_virmem::allocate_dynamicmem(1 << 20);
  }
  // %99.3 of all hardware supports, so why not
  gpu->vk_featuresDev.features.independentBlend = VK_TRUE;

  gpu->vk_featuresDev.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  gpu->vk_featuresDev.pNext = nullptr;
  gpu->vk_propsDev.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  gpu->vk_propsDev.pNext    = nullptr;
  extManager_vkDevice* mngr = new (VKGLOBAL_VIRMEM_EXTS) extManager_vkDevice;
  mngr->m_exts = new (VKGLOBAL_VIRMEM_EXTS) vkext_interface*[vkext_interface::vkext_count];
  mngr->m_GPU  = gpu;

  mngr->m_exts[vkext_interface::depthStencil_vkExtEnum] =
    new (VKGLOBAL_VIRMEM_EXTS) vkext_depthStencil(gpu);
  mngr->m_exts[vkext_interface::descIndexing_vkExtEnum] =
    new (VKGLOBAL_VIRMEM_EXTS) vkext_descIndexing(gpu);
  mngr->m_exts[vkext_interface::timelineSemaphores_vkExtEnum] =
    new (VKGLOBAL_VIRMEM_EXTS) vkext_timelineSemaphore(gpu);
  mngr->m_exts[vkext_interface::dynamicRendering_vkExtEnum] =
    new (VKGLOBAL_VIRMEM_EXTS) vkext_dynamicRendering(gpu);
  mngr->m_exts[vkext_interface::dynamicStates_vkExtEnum] =
    new (VKGLOBAL_VIRMEM_EXTS) vkext_dynamicStates(gpu);

  gpu->ext() = mngr;
}

void extManager_vkDevice::inspect() {
  vkGetPhysicalDeviceFeatures2(m_GPU->vk_physical, &m_GPU->vk_featuresDev);
  vkGetPhysicalDeviceProperties2(m_GPU->vk_physical, &m_GPU->vk_propsDev);

  for (size_t extIndx = 0; extIndx < vkext_interface::vkext_count; extIndx++) {
    vkext_interface* ext = m_exts[extIndx];
    if (ext) {
      ext->inspect();
    }
  }
  m_activeDevExtNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}