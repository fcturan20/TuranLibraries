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
#include "vk_renderer.h"
#include "vk_resource.h"
#include "vk_queue.h"

// Hardware Capability Helpers
static unsigned char GetTextureTypeLimits(texture_dimensions_tgfx dims, textureOrder_tgfx dataorder,
                                          textureChannels_tgfx     chnnltype,
                                          textureUsageFlag_tgfxhnd usageflag, gpu_tgfxhnd GPUHandle,
                                          unsigned int* MAXWIDTH, unsigned int* MAXHEIGHT,
                                          unsigned int* MAXDEPTH, unsigned int* MAXMIPLEVEL) {
  /*
  GPU_VKOBJ*              GPU = core_vk->getGPUs().getOBJfromHANDLE(GPUHandle);
  VkImageFormatProperties props;
  VkImageUsageFlags       flag = *( VkImageUsageFlags* )usageflag;
  if (chnnltype == texture_channels_tgfx_D24S8 || chnnltype == texture_channels_tgfx_D32) {
    flag &= ~(1UL << 4);
  } else {
    flag &= ~(1UL << 5);
  }
  VkResult result = vkGetPhysicalDeviceImageFormatProperties(
    GPU->PHYSICALDEVICE(), Find_VkFormat_byTEXTURECHANNELs(chnnltype), Find_VkImageType(dims),
    Find_VkTiling(dataorder), flag, 0, &props);
  if (result != VK_SUCCESS) {
    printer(result_tgfx_FAIL,
            ("GFX->GetTextureTypeLimits() has failed with: " + std::to_string(result)).c_str());
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
  return true;*/
  return false;
}

static textureUsageFlag_tgfxhnd vk_createUsageFlag_Texture(unsigned char isCopiableFrom,
                                                           unsigned char isCopiableTo,
                                                           unsigned char isRenderableTo,
                                                           unsigned char isSampledReadOnly,
                                                           unsigned char isRandomlyWrittenTo) {
  VkImageUsageFlags *UsageFlag = nullptr, FlagObj = 0;
  if (VKCONST_isPointerContainVKFLAG) {
    UsageFlag = &FlagObj;
  } else {
    UsageFlag = new VkImageUsageFlags;
  }
  *UsageFlag = ((isCopiableFrom) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0) |
               ((isCopiableTo) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0) |
               ((isRenderableTo) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                                 : 0) |
               ((isSampledReadOnly) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0) |
               ((isRandomlyWrittenTo) ? VK_IMAGE_USAGE_STORAGE_BIT : 0);
  if (VKCONST_isPointerContainVKFLAG) {
    return *( textureUsageFlag_tgfxhnd* )&FlagObj;
  } else {
    return ( textureUsageFlag_tgfxhnd )UsageFlag;
  }
}
static bufferUsageFlag_tgfxhnd vk_createUsageFlag_Buffer(
  unsigned char copyFrom, unsigned char copyTo, unsigned char uniformBuffer,
  unsigned char storageBuffer, unsigned char vertexBuffer, unsigned char indexBuffer,
  unsigned char indirectBuffer, unsigned char accessByPointerInShader) {
  VkBufferUsageFlags *UsageFlag = nullptr, FlagObj = 0;
  if (VKCONST_isPointerContainVKFLAG) {
    UsageFlag = &FlagObj;
  } else {
    UsageFlag = new VkBufferUsageFlags;
  }

  *UsageFlag = ((copyFrom) ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0) |
               ((copyTo) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0) |
               ((uniformBuffer) ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0) |
               ((storageBuffer) ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0) |
               ((vertexBuffer) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0) |
               ((indexBuffer) ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0) |
               ((indirectBuffer) ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0) |
               ((accessByPointerInShader) ? VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT : 0);

  if (VKCONST_isPointerContainVKFLAG) {
    return *( bufferUsageFlag_tgfxhnd* )&FlagObj;
  } else {
    return ( bufferUsageFlag_tgfxhnd )UsageFlag;
  }
}
// You can't create a memory type, only set allocation size of a given Memory Type (Memory Type is
// given by the GFX initialization process)
extern result_tgfx SetMemoryTypeInfo(unsigned int MemoryType_id, unsigned long long AllocationSize,
                                     extension_tgfxlsthnd Extensions);

static shaderStageFlag_tgfxhnd CreateShaderStageFlag(unsigned char count, ...) {
  // If size of a pointer isn't bigger than or equal to a VkShaderStageFlag, we should return a
  // pointer to the flag otherwise data is lost
  VkShaderStageFlags* flag   = nullptr;
  VkShaderStageFlags  flag_d = 0;
  if (VKCONST_isPointerContainVKFLAG) {
    flag = &flag_d;
  } else {
    flag  = new VkShaderStageFlags;
    *flag = flag_d;
  }
  va_list args;
  va_start(args, count);
  for (unsigned char i = 0; i < count; i++) {
    shaderstage_tgfx type = va_arg(args, shaderstage_tgfx);
    if (type == shaderstage_tgfx_VERTEXSHADER) {
      *flag = *flag | VK_SHADER_STAGE_VERTEX_BIT;
    }
    if (type == shaderstage_tgfx_FRAGMENTSHADER) {
      *flag = *flag | VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    if (type == shaderstage_tgfx_COMPUTESHADER) {
      *flag = *flag | VK_SHADER_STAGE_COMPUTE_BIT;
    }
  }
  va_end(args);
  if (VKCONST_isPointerContainVKFLAG) {
    return ( shaderStageFlag_tgfxhnd )*flag;
  } else {
    return ( shaderStageFlag_tgfxhnd )flag;
  }
}
static RTSlotDescription_tgfxhnd CreateRTSlotDescription_Color(
  texture_tgfxhnd Texture0, texture_tgfxhnd Texture1, operationtype_tgfx OPTYPE,
  drawpassload_tgfx LOADTYPE, unsigned char isUsedLater, unsigned char SLOTINDEX,
  vec4_tgfx clear_value) {
  rtslot_create_description_vk* desc = new rtslot_create_description_vk;
  desc->clear_value                  = clear_value;
  desc->isUsedLater                  = isUsedLater;
  desc->loadtype                     = LOADTYPE;
  desc->optype                       = OPTYPE;
  desc->textures[0] = contentmanager->GETTEXTURES_ARRAY().getOBJfromHANDLE(Texture0);
  desc->textures[1] = contentmanager->GETTEXTURES_ARRAY().getOBJfromHANDLE(Texture1);
  return ( RTSlotDescription_tgfxhnd )desc;
}
static RTSlotDescription_tgfxhnd CreateRTSlotDescription_DepthStencil(
  texture_tgfxhnd Texture0, texture_tgfxhnd Texture1, operationtype_tgfx DEPTHOP,
  drawpassload_tgfx DEPTHLOAD, operationtype_tgfx STENCILOP, drawpassload_tgfx STENCILLOAD,
  float DEPTHCLEARVALUE, unsigned char STENCILCLEARVALUE) {
  printer(result_tgfx_WARNING,
          "CreateRTSlotDescription_DepthStencil should be implemented properly!");
  rtslot_create_description_vk* desc = new rtslot_create_description_vk;
  desc->clear_value.x                = DEPTHCLEARVALUE;
  desc->clear_value.y                = STENCILCLEARVALUE;
  desc->isUsedLater                  = true;
  desc->loadtype                     = DEPTHLOAD;
  desc->optype                       = DEPTHOP;
  desc->textures[0] = contentmanager->GETTEXTURES_ARRAY().getOBJfromHANDLE(Texture0);
  desc->textures[1] = contentmanager->GETTEXTURES_ARRAY().getOBJfromHANDLE(Texture1);
  return ( RTSlotDescription_tgfxhnd )desc;
}
static rtslotusage_tgfx_handle CreateRTSlotUsage_Color(RTSlotDescription_tgfxhnd base_slot,
                                                       operationtype_tgfx        OPTYPE,
                                                       drawpassload_tgfx         LOADTYPE) {
  printer(result_tgfx_WARNING, "Vulkan backend doesn't use LOADTYPE for now!");
  if (!base_slot) {
    printer(result_tgfx_INVALIDARGUMENT,
            "CreateRTSlotUsage_Color() has failed because base_slot is nullptr!");
    return nullptr;
  }
  rtslot_create_description_vk* baseslot = ( rtslot_create_description_vk* )base_slot;
  if (baseslot->optype == operationtype_tgfx_READ_ONLY &&
      (OPTYPE == operationtype_tgfx_WRITE_ONLY || OPTYPE == operationtype_tgfx_READ_AND_WRITE)) {
    printer(result_tgfx_INVALIDARGUMENT,
            "Inherite_RTSlotSet() has failed because you can't use a Read-Only ColorSlot with "
            "Write Access in a Inherited Set!");
    return nullptr;
  }
  rtslot_inheritance_descripton_vk* usage = new rtslot_inheritance_descripton_vk;
  usage->IS_DEPTH                         = false;
  usage->OPTYPE                           = OPTYPE;
  usage->OPTYPESTENCIL                    = operationtype_tgfx_UNUSED;
  usage->LOADTYPE                         = LOADTYPE;
  usage->LOADTYPESTENCIL                  = drawpassload_tgfx_CLEAR;
  return ( rtslotusage_tgfx_handle )usage;
}
static rtslotusage_tgfx_handle CreateRTSlotUsage_Depth(RTSlotDescription_tgfxhnd base_slot,
                                                       operationtype_tgfx        DEPTHOP,
                                                       drawpassload_tgfx         DEPTHLOAD,
                                                       operationtype_tgfx        STENCILOP,
                                                       drawpassload_tgfx         STENCILLOAD) {
  if (!base_slot) {
    printer(result_tgfx_INVALIDARGUMENT,
            "CreateRTSlotUsage_Color() has failed because base_slot is nullptr!");
    return nullptr;
  }
  rtslot_create_description_vk* baseslot = ( rtslot_create_description_vk* )base_slot;
  if (baseslot->optype == operationtype_tgfx_READ_ONLY &&
      (DEPTHOP == operationtype_tgfx_WRITE_ONLY || DEPTHOP == operationtype_tgfx_READ_AND_WRITE)) {
    printer(result_tgfx_INVALIDARGUMENT,
            "Inherite_RTSlotSet() has failed because you can't use a Read-Only DepthSlot with "
            "Write Access in a Inherited Set!");
    return nullptr;
  }
  rtslot_inheritance_descripton_vk* usage = new rtslot_inheritance_descripton_vk;
  usage->IS_DEPTH                         = true;
  usage->OPTYPE                           = DEPTHOP;
  usage->OPTYPESTENCIL                    = STENCILOP;
  usage->LOADTYPE                         = DEPTHLOAD;
  usage->LOADTYPESTENCIL                  = STENCILLOAD;
  return ( rtslotusage_tgfx_handle )usage;
}
static depthsettings_tgfxhnd CreateDepthConfiguration(unsigned char        ShouldWrite,
                                                      depthtest_tgfx       COMPAREOP,
                                                      extension_tgfxlsthnd EXTENSIONS) {
  /*
  depthsettingsdesc_vk* desc = new depthsettingsdesc_vk;
  TGFXLISTCOUNT(core_tgfx_main, EXTENSIONS, extcount);
  for (unsigned int ext_i = 0; ext_i < extcount; ext_i++) {
    if (*( VKEXT_TYPES* )(EXTENSIONS[ext_i]) != VKEXT_DEPTHBOUNDS) {
      VKEXT_DEPTHBOUNDS_STRUCT* ext = ( VKEXT_DEPTHBOUNDS_STRUCT* )EXTENSIONS[ext_i];
      desc->DepthBoundsEnable       = ext->DepthBoundsEnable;
      desc->DepthBoundsMax          = ext->DepthBoundsMax;
      desc->DepthBoundsMin          = ext->DepthBoundsMin;
    }
  }
  desc->ShouldWrite    = ShouldWrite;
  desc->DepthCompareOP = Find_CompareOp_byGFXDepthTest(COMPAREOP);
  return ( depthsettings_tgfxhnd )desc;*/
  return nullptr;
}
static stencilcnfg_tgfxnd CreateStencilConfiguration(
  unsigned char Reference, unsigned char WriteMask, unsigned char CompareMask,
  stencilcompare_tgfx CompareOP, stencilop_tgfx DepthFailOP, stencilop_tgfx StencilFailOP,
  stencilop_tgfx AllSuccessOP) {
  return nullptr;
}
static blendinginfo_tgfx_handle CreateBlendingConfiguration(
  unsigned char ColorSlotIndex, vec4_tgfx Constant, blendfactor_tgfx SRCFCTR_CLR,
  blendfactor_tgfx SRCFCTR_ALPHA, blendfactor_tgfx DSTFCTR_CLR, blendfactor_tgfx DSTFCTR_ALPHA,
  blendmode_tgfx BLENDOP_CLR, blendmode_tgfx BLENDOP_ALPHA, colorcomponents_tgfx WRITECHANNELs) {
  return nullptr;
}

void VK_getQueueAndSharingInfos(gpuQueue_tgfxlsthnd i_queueList, extension_tgfxlsthnd i_exts,
                                uint32_t  o_famList[VKCONST_MAXQUEUEFAMCOUNT_PERGPU],
                                uint32_t* o_famListSize, VkSharingMode* o_sharingMode) {
  TGFXLISTCOUNT(core_tgfx_main, i_queueList, i_listSize);
  uint32_t   validQueueFamCount = 0;
  GPU_VKOBJ* theGPU             = nullptr;
  for (uint32_t listIndx = 0; listIndx < i_listSize; listIndx++) {
    getGPUfromQueueHnd(i_queueList[listIndx]);
    if (!theGPU) {
      theGPU = gpu;
    }
    assert(theGPU == gpu && "Queues from different devices!");
    bool isPreviouslyAdded = false;
    for (uint32_t validQueueIndx = 0; validQueueIndx < validQueueFamCount; validQueueIndx++) {
      if (o_famList[validQueueIndx] == queue->vk_queueFamIndex) {
        isPreviouslyAdded = true;
        break;
      }
    }
    if (!isPreviouslyAdded) {
      o_famList[validQueueFamCount++] = queue->vk_queueFamIndex;
    }
  }
  if (validQueueFamCount > 1) {
    *o_sharingMode = VK_SHARING_MODE_CONCURRENT;
  } else {
    *o_sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }
  *o_famListSize = validQueueFamCount;
  for (uint32_t i = *o_famListSize; i < VKCONST_MAXQUEUEFAMCOUNT_PERGPU; i++) {
    o_famList[i] = UINT32_MAX;
  }
}

// EXTENSION HELPERS

static void                  Destroy_ExtensionData(extension_tgfx_handle ExtensionToDestroy) {}
static extension_tgfx_handle EXT_DepthBoundsInfo(float BoundMin, float BoundMax) {
  /*
  VKEXT_DEPTHBOUNDS_STRUCT* ext = new VKEXT_DEPTHBOUNDS_STRUCT;
  ext->type                     = VKEXT_DEPTHBOUNDS;
  ext->DepthBoundsEnable        = VK_TRUE;
  ext->DepthBoundsMin           = BoundMin;
  ext->DepthBoundsMax           = BoundMax;
  return ( extension_tgfx_handle )ext;*/
  return nullptr;
}
void vk_setHelperFuncPtrs() {
  core_tgfx_main->helpers->createUsageFlag_Texture = vk_createUsageFlag_Texture;
  core_tgfx_main->helpers->createUsageFlag_Buffer  = vk_createUsageFlag_Buffer;
}