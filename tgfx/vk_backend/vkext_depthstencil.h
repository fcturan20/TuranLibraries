#pragma once
#include <float.h>

#include "vk_extension.h"
#include "vk_predefinitions.h"

struct vkext_depthStencil : public vkext_interface {
  vkext_depthStencil(GPU_VKOBJ* gpu);
  virtual void inspect() override;
  virtual void manage(VkStructureType structType, void* structPtr, unsigned int extCount,
                      struct tgfx_extension* const* exts) override;
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