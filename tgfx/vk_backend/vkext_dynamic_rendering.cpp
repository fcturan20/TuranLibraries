#include "vkext_dynamic_rendering.h"

#include <string>

#include "vk_contentmanager.h"
#include "vk_core.h"
#include "vk_includes.h"
#include "vk_predefinitions.h"
#include "vk_resource.h"

defineVkExtFunc(vkCmdBeginRenderingKHR);
defineVkExtFunc(vkCmdEndRenderingKHR);

vkext_dynamicRendering::vkext_dynamicRendering(GPU_VKOBJ* gpu)
  : vkext_interface(gpu, nullptr, &features) {
  m_type         = dynamicRendering_vkExtEnum;
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
  features.pNext = nullptr;
}

declareVkExtFunc(vkCmdBeginRenderingKHR);
declareVkExtFunc(vkCmdEndRenderingKHR);
void vkext_dynamicRendering::inspect() {
  if (features.dynamicRendering) {
    m_gpu->ext()->m_activeDevExtNames[m_gpu->ext()->m_devExtCount++] =
      (VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
    m_gpu->ext()->m_activeDevExtNames[m_gpu->ext()->m_devExtCount++] =
      (VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);
    m_gpu->ext()->m_activeDevExtNames[m_gpu->ext()->m_devExtCount++] =
      (VK_KHR_MULTIVIEW_EXTENSION_NAME);
    m_gpu->ext()->m_activeDevExtNames[m_gpu->ext()->m_devExtCount++] =
      (VK_KHR_MAINTENANCE2_EXTENSION_NAME);
    m_gpu->ext()->m_activeDevExtNames[m_gpu->ext()->m_devExtCount++] =
      (VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    loadVkExtFunc(vkCmdBeginRenderingKHR);
    loadVkExtFunc(vkCmdEndRenderingKHR);
  }
}
void vkext_dynamicRendering::manage(VkStructureType structType, void* structPtr,
                                    unsigned int extCount, struct tgfx_extension* const* exts) {}

void vkext_beginDynamicRenderPass(VkCommandBuffer cb, unsigned int colorAttachmentCount,
                                  const tgfx_rasterpassBeginSlotInfo* colorAttachments,
                                  tgfx_rasterpassBeginSlotInfo        depthAttachment) {
  TEXTURE_VKOBJ* baseTexture = nullptr;
  if (depthAttachment.texture) {
    baseTexture = getOBJ<TEXTURE_VKOBJ>(depthAttachment.texture);
  } else {
    baseTexture = getOBJ<TEXTURE_VKOBJ>(colorAttachments[0].texture);
  }

  VkRenderingAttachmentInfo attachmentInfos[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT + 1] = {};
  for (uint32_t colorSlotIndx = 0; colorSlotIndx < colorAttachmentCount; colorSlotIndx++) {
    const tgfx_rasterpassBeginSlotInfo& colorAttachment = colorAttachments[colorSlotIndx];
    TEXTURE_VKOBJ*                      texture = getOBJ<TEXTURE_VKOBJ>(colorAttachment.texture);

    VkFormat format = vk_findFormatVk(texture->m_channels);
    void*    target = nullptr;
    switch (vk_findTextureDataType(format)) {
      case datatype_tgfx_VAR_UINT32:
        for (uint32_t i = 0; i < 4; i++) {
          attachmentInfos[colorSlotIndx].clearValue.color.uint32[i] =
            *( uint32_t* )&colorAttachments[colorSlotIndx].clearValue.data[i * 4];
        }
        break;
      case datatype_tgfx_VAR_INT32:
        for (uint32_t i = 0; i < 4; i++) {
          attachmentInfos[colorSlotIndx].clearValue.color.int32[i] =
            *( int32_t* )&colorAttachments[colorSlotIndx].clearValue.data[i * 4];
        }
        break;
      case datatype_tgfx_VAR_FLOAT32:
        for (uint32_t i = 0; i < 4; i++) {
          attachmentInfos[colorSlotIndx].clearValue.color.float32[i] =
            *( float* )&colorAttachments[colorSlotIndx].clearValue.data[i * 4];
        }
        break;
    }

    attachmentInfos[colorSlotIndx].imageView = texture->vk_imageView;
    attachmentInfos[colorSlotIndx].sType     = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    VkAccessFlags unused                     = {};
    vk_findImageAccessPattern(colorAttachment.imageAccess, unused,
                              attachmentInfos[colorSlotIndx].imageLayout);
    attachmentInfos[colorSlotIndx].loadOp  = vk_findLoadTypeVk(colorAttachment.loadOp);
    attachmentInfos[colorSlotIndx].storeOp = vk_findStoreTypeVk(colorAttachment.storeOp);
  }
  if (depthAttachment.texture) {
    attachmentInfos[colorAttachmentCount].imageView =
      getOBJ<TEXTURE_VKOBJ>(depthAttachment.texture)->vk_imageView;
    VkAccessFlags unused = {};
    vk_findImageAccessPattern(depthAttachment.imageAccess, unused,
                              attachmentInfos[colorAttachmentCount].imageLayout);
    attachmentInfos[colorAttachmentCount].loadOp  = vk_findLoadTypeVk(depthAttachment.loadOp);
    attachmentInfos[colorAttachmentCount].storeOp = vk_findStoreTypeVk(depthAttachment.storeOp);
    attachmentInfos[colorAttachmentCount].sType   = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachmentInfos[colorAttachmentCount].clearValue.depthStencil.depth =
      *( float* )depthAttachment.clearValue.data;
    attachmentInfos[colorAttachmentCount].clearValue.depthStencil.stencil =
      *( uint32_t* )&depthAttachment.clearValue.data[5];
  }

  VkRenderingInfo ri      = {};
  ri.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
  ri.colorAttachmentCount = colorAttachmentCount;
  ri.flags                = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;
  ri.layerCount           = 1;
  ri.pColorAttachments    = attachmentInfos;
  ri.pDepthAttachment = depthAttachment.texture ? &attachmentInfos[colorAttachmentCount] : nullptr;
  ri.pStencilAttachment       = nullptr;
  ri.renderArea.extent.width  = baseTexture->m_width;
  ri.renderArea.extent.height = baseTexture->m_height;
  ri.renderArea.offset        = {};
  vkCmdBeginRenderingKHR_loaded(cb, &ri);
}

void vkext_beginStaticRenderPass(VkCommandBuffer cb, unsigned int extCount,
                                 struct tgfx_extension* const* exts) {}

void vkext_dynamicRendering::vk_beginRenderpass(
  VkCommandBuffer cb, unsigned int colorAttachmentCount,
  const tgfx_rasterpassBeginSlotInfo* colorAttachments,
  tgfx_rasterpassBeginSlotInfo depthAttachment, unsigned int extCount,
  struct tgfx_extension* const* exts) {
  if (features.dynamicRendering) {
    vkext_beginDynamicRenderPass(cb, colorAttachmentCount, colorAttachments, depthAttachment);
  } else {
    if (!exts) {
      vkPrint(62, L"Extension isn't supported yet");
      return;
    }

    vkext_beginStaticRenderPass(cb, extCount, exts);
  }
}

void vkext_dynamicRendering::vk_endRenderpass(VkCommandBuffer cb, unsigned int extCount,
                                              struct tgfx_extension* const* exts) {
  if (features.dynamicRendering) {
    vkCmdEndRenderingKHR_loaded(cb);
  } else {
    vkCmdEndRenderPass(cb);
  }
}