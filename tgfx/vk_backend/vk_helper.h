#pragma once
#include "vk_predefinitions.h"

inline VkShaderStageFlags GetVkShaderStageFlags_fromTGFXHandle(shaderStageFlag_tgfxhnd handle) {
  if (VKCONST_isPointerContainVKFLAG) {
    return *(( VkShaderStageFlags* )&handle);
  } else {
    return *(( VkShaderStageFlags* )handle);
  }
}

inline textureUsageFlag_tgfxhnd Get_TGFXTextureUsageFlag_byVk(VkImageUsageFlags flag) {
  if constexpr (VKCONST_isPointerContainVKFLAG) {
    return *(( textureUsageFlag_tgfxhnd* )&flag);
  } else {
    return nullptr;
  }
}

inline bufferUsageFlag_tgfxhnd Get_TGFXBufferUsageFlag_byVk(VkBufferUsageFlags flag) {
  if constexpr (VKCONST_isPointerContainVKFLAG) {
    return *(( bufferUsageFlag_tgfxhnd* )&flag);
  } else {
    return nullptr;
  }
}

inline VkImageUsageFlags Get_VkTextureUsageFlag_byTGFX(textureUsageFlag_tgfxhnd flag) {
  if constexpr (VKCONST_isPointerContainVKFLAG) {
    return *( VkImageUsageFlags* )&flag;
  } else {
    return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
  }
}

inline VkBufferUsageFlags Get_VkBufferUsageFlag_byTGFX(bufferUsageFlag_tgfxhnd flag) {
  if constexpr (VKCONST_isPointerContainVKFLAG) {
    return *( VkBufferUsageFlags* )&flag;
  } else {
    return VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
  }
}

// Supported extension: QueueOwnershipTransfer
// Use o_ params with uint32_t queueFamList[VKCONST_MAXQUEUEFAMCOUNT] etc.
void VK_getQueueAndSharingInfos(gpuQueue_tgfxlsthnd i_queueList, extension_tgfxlsthnd i_exts,
                                uint32_t* o_famList, uint32_t* o_famListSize,
                                VkSharingMode* o_sharingMode);