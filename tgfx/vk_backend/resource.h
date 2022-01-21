#pragma once
#include <glm/glm.hpp>
#include <atomic>
#include <vector>
#include "predefinitions_vk.h"



struct texture_vk {
	struct MemoryBlock {
		unsigned int MemAllocIndex = UINT32_MAX;
		VkDeviceSize Offset;
	};
	unsigned int WIDTH, HEIGHT, DATA_SIZE;
	unsigned char MIPCOUNT;
	texture_channels_tgfx CHANNELs;
	VkImageUsageFlags USAGE;
	texture_dimensions_tgfx DIMENSION;

	VkImage Image = {};
	VkImageView ImageView = {};
	MemoryBlock Block;
};

struct colorslot_vk {
	texture_vk* RT;
	drawpassload_tgfx LOADSTATE;
	bool IS_USED_LATER;
	operationtype_tgfx RT_OPERATIONTYPE;
	glm::vec4 CLEAR_COLOR;
	std::atomic_bool IsChanged = false;
};
struct depthstencilslot_vk {
	texture_vk* RT;
	drawpassload_tgfx DEPTH_LOAD, STENCIL_LOAD;
	bool IS_USED_LATER;
	operationtype_tgfx DEPTH_OPTYPE, STENCIL_OPTYPE;
	glm::vec2 CLEAR_COLOR;
	std::atomic_bool IsChanged = false;
};
struct rtslots_vk {
	colorslot_vk* COLOR_SLOTs = nullptr;
	unsigned char COLORSLOTs_COUNT = 0;
	depthstencilslot_vk* DEPTHSTENCIL_SLOT = nullptr;	//There is one, but there may not be a Depth Slot. So if there is no, then this is nullptr.
	//Unused Depth and NoDepth are different. Unused Depth means RenderPass does have one but current Subpass doesn't use, but NoDepth means RenderPass doesn't have one!
	std::atomic_bool IsChanged = false;
};
struct rtslotset_vk {
	rtslots_vk PERFRAME_SLOTSETs[2];
	//You should change this struct's vkRenderPass object pointer as your vkRenderPass object
	VkFramebufferCreateInfo FB_ci[2];
	std::vector<VkImageView> ImageViews[2];
};
struct irtslotset_vk {
	rtslotset_vk* BASESLOTSET;
	operationtype_tgfx* COLOR_OPTYPEs;
	operationtype_tgfx DEPTH_OPTYPE;
	operationtype_tgfx STENCIL_OPTYPE;
};