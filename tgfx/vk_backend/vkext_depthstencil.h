#pragma once
#include <float.h>

#include "vk_predefinitions.h"
#include "vk_extension.h"

struct vkext_depthStencil : public vkext_interface{
  vkext_depthStencil(GPU_VKOBJ* gpu);
  virtual void inspect() override;
  virtual void manage(VkStructureType structType, void* structPtr,
                      extension_tgfx_handle extData) override;
  VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures features;
};

struct depthstencilslot_vk;
// These are TGFX CORE functionalities, so func pointers are here
typedef void (*Fill_DepthAttachmentDescriptionFnc)(VkAttachmentDescription& desc,
                                               depthstencilslot_vk*     slot);
typedef void (*Fill_DepthAttachmentReferenceFnc)(VkAttachmentReference& Ref, unsigned int index,
                                             textureChannels_tgfx channels,
                                             operationtype_tgfx   DEPTHOPTYPE,
                                             operationtype_tgfx   STENCILOPTYPE);
extern Fill_DepthAttachmentDescriptionFnc fillDepthAttachDesc;
extern Fill_DepthAttachmentReferenceFnc   fillDepthAttachRef;