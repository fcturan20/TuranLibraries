#pragma once
#include "vk_predefinitions.h"
#include "vk_resource.h"

struct BINDINGTABLEINST_VKOBJ {
  std::atomic_bool isALIVE    = false;
  vk_handleType    HANDLETYPE = VKHANDLETYPEs::BINDINGTABLEINST;

  static uint16_t GET_EXTRAFLAGS(BINDINGTABLEINST_VKOBJ* obj) { return UINT16_MAX; }
  uint8_t         m_gpu;
  uint32_t        m_elementCount;
  bool            m_isStatic = false; // Static binding table or dynamic?
  void*           m_descs    = nullptr;

  VkDescriptorSetLayout vk_layout;
  VkDescriptorSet       vk_set    = {};
  VkDescriptorPool      vk_pool   = {};
  VkShaderStageFlags    vk_stages = 0;
  VkDescriptorType      vk_descType;
};

struct gpudatamanager_private;
struct gpudatamanager_public {
  gpudatamanager_private* hidden;

  VK_LINEAR_OBJARRAY<BUFFER_VKOBJ, buffer_tgfxhnd>&              GETBUFFER_ARRAY();
  VK_LINEAR_OBJARRAY<TEXTURE_VKOBJ, texture_tgfxhnd, 1 << 24>&   GETTEXTURES_ARRAY();
  VK_LINEAR_OBJARRAY<PIPELINE_VKOBJ, pipeline_tgfxhnd, 1 << 24>& GETPIPELINE_ARRAY();
  VK_LINEAR_OBJARRAY<BINDINGTABLEINST_VKOBJ, bindingTable_tgfxhnd, 1 << 16>&
                                                                           GETBINDINGTABLE_ARRAY();
  VK_LINEAR_OBJARRAY<SUBRASTERPASS_VKOBJ, subRasterpass_tgfxhnd, 1 << 16>& GETSUBRASTERPASS_ARRAY();
};