#include "vkext_depthstencil.h"

#include "vk_core.h"
#include "vk_includes.h"
#include "vk_predefinitions.h"
#include "vk_resource.h"

Fill_DepthAttachmentDescriptionFnc fillDepthAttachDesc = nullptr;
Fill_DepthAttachmentReferenceFnc   fillDepthAttachRef  = nullptr;

// Seperated Depth Stencil Layouts
void Fill_DepthAttachmentReference_SeperatedDSLayouts(VkAttachmentReference& Ref,
                                                      unsigned int           index,
                                                      textureChannels_tgfx   channels,
                                                      operationtype_tgfx     DEPTHOPTYPE,
                                                      operationtype_tgfx     STENCILOPTYPE) {
  Ref.attachment = index;
  if (channels == texture_channels_tgfx_D32) {
    switch (DEPTHOPTYPE) {
      case operationtype_tgfx_READ_ONLY:
        Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      case operationtype_tgfx_READ_AND_WRITE:
      case operationtype_tgfx_WRITE_ONLY:
        Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        break;
      case operationtype_tgfx_UNUSED:
        Ref.attachment = VK_ATTACHMENT_UNUSED;
        Ref.layout     = VK_IMAGE_LAYOUT_UNDEFINED;
        break;
      default: vkPrint(58);
    }
  } else if (channels == texture_channels_tgfx_D24S8) {
    switch (STENCILOPTYPE) {
      case operationtype_tgfx_READ_ONLY:
        if (DEPTHOPTYPE == operationtype_tgfx_UNUSED ||
            DEPTHOPTYPE == operationtype_tgfx_READ_ONLY) {
          Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        } else if (DEPTHOPTYPE == operationtype_tgfx_READ_AND_WRITE ||
                   DEPTHOPTYPE == operationtype_tgfx_WRITE_ONLY) {
          Ref.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
        }
        break;
      case operationtype_tgfx_READ_AND_WRITE:
      case operationtype_tgfx_WRITE_ONLY:
        if (DEPTHOPTYPE == operationtype_tgfx_UNUSED ||
            DEPTHOPTYPE == operationtype_tgfx_READ_ONLY) {
          Ref.layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
        } else if (DEPTHOPTYPE == operationtype_tgfx_READ_AND_WRITE ||
                   DEPTHOPTYPE == operationtype_tgfx_WRITE_ONLY) {
          Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        break;
      case operationtype_tgfx_UNUSED:
        if (DEPTHOPTYPE == operationtype_tgfx_UNUSED) {
          Ref.attachment = VK_ATTACHMENT_UNUSED;
          Ref.layout     = VK_IMAGE_LAYOUT_UNDEFINED;
        } else if (DEPTHOPTYPE == operationtype_tgfx_READ_AND_WRITE ||
                   DEPTHOPTYPE == operationtype_tgfx_WRITE_ONLY) {
          Ref.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
        } else if (DEPTHOPTYPE == operationtype_tgfx_READ_ONLY) {
          Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        }
        break;
      default: vkPrint(58);
    }
  }
}
void Fill_DepthAttachmentReference_NOSeperated(VkAttachmentReference& Ref, unsigned int index,
                                               textureChannels_tgfx channels,
                                               operationtype_tgfx   DEPTHOPTYPE,
                                               operationtype_tgfx   STENCILOPTYPE) {
  Ref.attachment = index;
  if (DEPTHOPTYPE == operationtype_tgfx_UNUSED) {
    Ref.attachment = VK_ATTACHMENT_UNUSED;
    Ref.layout     = VK_IMAGE_LAYOUT_UNDEFINED;
  } else {
    Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  }
}
void Fill_DepthAttachmentDescription_SeperatedDSLayouts(VkAttachmentDescription& Desc,
                                                        depthstencilslot_vk*     DepthSlot) {
  Desc               = {};
  Desc.format        = vk_findFormatVk(DepthSlot->RT->m_channels);
  Desc.samples       = VK_SAMPLE_COUNT_1_BIT;
  Desc.flags         = 0;
  Desc.loadOp        = vk_findLoadTypeVk(DepthSlot->DEPTH_LOAD);
  Desc.stencilLoadOp = vk_findLoadTypeVk(DepthSlot->STENCIL_LOAD);
  if (DepthSlot->IS_USED_LATER) {
    Desc.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  } else {
    Desc.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  }
  if (DepthSlot->RT->m_channels == texture_channels_tgfx_D32) {
    if (DepthSlot->DEPTH_OPTYPE == operationtype_tgfx_READ_ONLY) {
      Desc.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
      Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
    } else {
      Desc.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
      Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    }
  } else if (DepthSlot->RT->m_channels == texture_channels_tgfx_D24S8) {
    if (DepthSlot->DEPTH_OPTYPE == operationtype_tgfx_READ_ONLY) {
      if (DepthSlot->STENCIL_OPTYPE != operationtype_tgfx_READ_ONLY &&
          DepthSlot->STENCIL_OPTYPE != operationtype_tgfx_UNUSED) {
        Desc.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
        Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
      } else {
        Desc.finalLayout   = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        Desc.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      }
    } else {
      if (DepthSlot->STENCIL_OPTYPE != operationtype_tgfx_READ_ONLY &&
          DepthSlot->STENCIL_OPTYPE != operationtype_tgfx_UNUSED) {
        Desc.finalLayout   = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        Desc.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      } else {
        Desc.finalLayout =
          VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
        Desc.initialLayout =
          VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
      }
    }
  }
}
void Fill_DepthAttachmentDescription_NOSeperated(VkAttachmentDescription& Desc,
                                                 depthstencilslot_vk*     DepthSlot) {
  Desc               = {};
  Desc.format        = vk_findFormatVk(DepthSlot->RT->m_channels);
  Desc.samples       = VK_SAMPLE_COUNT_1_BIT;
  Desc.flags         = 0;
  Desc.loadOp        = vk_findLoadTypeVk(DepthSlot->DEPTH_LOAD);
  Desc.stencilLoadOp = vk_findLoadTypeVk(DepthSlot->STENCIL_LOAD);
  if (DepthSlot->IS_USED_LATER) {
    Desc.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  } else {
    Desc.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  }
  Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  Desc.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

vkext_depthStencil::vkext_depthStencil(GPU_VKOBJ* gpu) : vkext_interface(gpu, nullptr, &features) {
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES;
  features.pNext = nullptr;
}
void vkext_depthStencil::inspect() {
  if (features.separateDepthStencilLayouts) {
    fillDepthAttachDesc = Fill_DepthAttachmentDescription_SeperatedDSLayouts;
    fillDepthAttachRef  = Fill_DepthAttachmentReference_SeperatedDSLayouts;
  } else {
    fillDepthAttachDesc = Fill_DepthAttachmentDescription_NOSeperated;
    fillDepthAttachRef  = Fill_DepthAttachmentReference_NOSeperated;

    vkPrint(56, m_gpu->desc.name);
  }
  if (!m_gpu->vk_featuresDev.features.depthBounds) {
    vkPrint(57, m_gpu->desc.name);
  }
}
void vkext_depthStencil::manage(VkStructureType structType, void* structPtr, unsigned int extCount,
                                struct tgfx_extension* const* exts) {}