#pragma once
#include "vk_predefinitions.h"
#include "vk_resource.h"

struct gpudatamanager_private;
struct gpudatamanager_public {
  gpudatamanager_private* hidden;

  VK_LINEAR_OBJARRAY<BUFFER_VKOBJ, buffer_tgfxhnd>&                          GETBUFFER_ARRAY();
  VK_LINEAR_OBJARRAY<RTSLOTSET_VKOBJ, RTSlotset_tgfxhnd, 1 << 10>&           GETRTSLOTSET_ARRAY();
  VK_LINEAR_OBJARRAY<IRTSLOTSET_VKOBJ, inheritedRTSlotset_tgfxhnd, 1 << 10>& GETIRTSLOTSET_ARRAY();
  VK_LINEAR_OBJARRAY<COMPUTEPIPELINE_VKOBJ, computePipeline_tgfxhnd>&        GETCOMPUTETYPE_ARRAY();
  VK_LINEAR_OBJARRAY<TEXTURE_VKOBJ, texture_tgfxhnd, 1 << 24>&               GETTEXTURES_ARRAY();
  VK_LINEAR_OBJARRAY<RASTERPIPELINE_VKOBJ, rasterPipeline_tgfxhnd, 1 << 24>&
  GETGRAPHICSPIPETYPE_ARRAY();
  VK_LINEAR_OBJARRAY<BINDINGTABLEINST_VKOBJ, bindingTable_tgfxhnd, 1 << 16>& GETDESCSET_ARRAY();
  VK_LINEAR_OBJARRAY<VIEWPORT_VKOBJ, viewport_tgfxhnd, 1 << 16>&             GETVIEWPORT_ARRAY();
};