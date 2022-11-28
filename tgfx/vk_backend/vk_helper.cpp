#include "vk_helper.h"

#include <stdarg.h>
#include <tgfx_core.h>
#include <tgfx_forwarddeclarations.h>
#include <tgfx_helper.h>
#include <tgfx_structs.h>

#include <iostream>

#include "tgfx_structs.h"
#include "vk_contentmanager.h"
#include "vk_core.h"
#include "vk_extension.h"
#include "vk_includes.h"
#include "vk_predefinitions.h"
#include "vk_queue.h"
#include "vk_resource.h"

// Hardware Capability Helpers
static unsigned char vk_getTextureTypeLimits(
  texture_dimensions_tgfx dims, textureOrder_tgfx dataorder, textureChannels_tgfx chnnltype,
  textureUsageMask_tgfxflag usageflag, gpu_tgfxhnd GPUHandle, unsigned int* MAXWIDTH,
  unsigned int* MAXHEIGHT, unsigned int* MAXDEPTH, unsigned int* MAXMIPLEVEL) {
  GPU_VKOBJ*              GPU = core_vk->getGPUs().getOBJfromHANDLE(GPUHandle);
  VkImageFormatProperties props;
  VkImageUsageFlags       flag = vk_findTextureUsageFlagVk(usageflag);
  if (chnnltype == texture_channels_tgfx_D24S8 || chnnltype == texture_channels_tgfx_D32) {
    flag &= ~(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
  } else {
    flag &= ~(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
  }
  VkResult result = vkGetPhysicalDeviceImageFormatProperties(
    GPU->vk_physical, vk_findFormatVk(chnnltype), vk_findImageTypeVk(dims),
    Find_VkTiling(dataorder), flag, 0, &props);
  if (result != VK_SUCCESS) {
    printer(result_tgfx_FAIL, "helpers->GetTextureTypeLimits() has failed!");
    return false;
  }

  if (MAXWIDTH) {
    *MAXWIDTH = props.maxExtent.width;
  }
  if (MAXHEIGHT) {
    *MAXHEIGHT = props.maxExtent.height;
  }
  if (MAXDEPTH) {
    *MAXDEPTH = props.maxExtent.depth;
  }
  if (MAXMIPLEVEL) {
    *MAXMIPLEVEL = props.maxMipLevels;
  }
  return true;
}

static void vk_getTextureSupportedMemTypes(texture_tgfxhnd texture,
                                           unsigned int*   SupportedMemoryTypesBitset) {
  *SupportedMemoryTypesBitset = contentmanager->GETTEXTURES_ARRAY()
                                  .getOBJfromHANDLE(texture)
                                  ->m_memReqs.vk_memReqs.memoryTypeBits;
}

// EXTENSION HELPERS

void Destroy_ExtensionData(extension_tgfx_handle ExtensionToDestroy) {}

void vk_setHelperFuncPtrs() {
  core_tgfx_main->helpers->getTextureTypeLimits = vk_getTextureTypeLimits;
  core_tgfx_main->helpers->getTextureSupportedMemTypes = vk_getTextureSupportedMemTypes;
}