#pragma once
#include <glm/glm.hpp>
#include <atomic>
#include <vector>
#include "predefinitions_vk.h"
#include <tgfx_structs.h>


struct memoryblock_vk {
	unsigned int MemAllocIndex = UINT32_MAX;
	VkDeviceSize Offset;
};
struct texture_vk {
	unsigned int WIDTH, HEIGHT, DATA_SIZE;
	unsigned char MIPCOUNT;
	texture_channels_tgfx CHANNELs;
	VkImageUsageFlags USAGE;
	texture_dimensions_tgfx DIMENSION;

	VkImage Image = {};
	VkImageView ImageView = {};
	memoryblock_vk Block;
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
struct subdrawpassdesc_vk {
	irtslotset_vk* INHERITEDSLOTSET;
	subdrawpassaccess_tgfx WaitOp, ContinueOp;
};
struct rtslotdesc_vk {
	texture_vk* textures[2];
	operationtype_tgfx optype;
	drawpassload_tgfx loadtype;
	bool isUsedLater;
	vec4_tgfx clear_value;
};
struct rtslotusage_vk {
	bool IS_DEPTH = false;
	operationtype_tgfx OPTYPE = operationtype_tgfx_UNUSED, OPTYPESTENCIL = operationtype_tgfx_UNUSED;
	drawpassload_tgfx LOADTYPE = drawpassload_tgfx_CLEAR, LOADTYPESTENCIL = drawpassload_tgfx_CLEAR;
};
struct descriptor_vk {
	desctype_vk Type;
	void* Elements = nullptr;
	unsigned int ElementCount = 0;
};
struct descset_vk {
	VkDescriptorSet Set = VK_NULL_HANDLE;
	VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
	descriptor_vk* Descs = nullptr;
	//DescCount is size of the "Descs" pointer above, others are including element counts of each descriptor
	unsigned int DescCount = 0, DescUBuffersCount = 0, DescSBuffersCount = 0, DescSamplersCount = 0, DescImagesCount = 0;
	std::atomic_bool ShouldRecreate = false;
};
struct graphicspipelinetype_vk {
	subdrawpass_vk* GFX_Subpass;

	VkPipelineLayout PipelineLayout;
	VkPipeline PipelineObject;
	descset_vk General_DescSet, Instance_DescSet;
};
struct graphicspipelineinst_vk {
	graphicspipelinetype_vk* PROGRAM;
	descset_vk DescSet;
};
struct depthsettingsdesc_vk {
	VkBool32 ShouldWrite = VK_FALSE;
	VkCompareOp DepthCompareOP = VkCompareOp::VK_COMPARE_OP_MAX_ENUM;
	//DepthBounds Extension
	VkBool32 DepthBoundsEnable = VK_FALSE; float DepthBoundsMin = FLT_MIN, DepthBoundsMax = FLT_MAX;
};
struct stencildesc_vk {
	VkStencilOpState OPSTATE;
};
struct blendinginfo_vk {
	unsigned char COLORSLOT_INDEX = 255;
	glm::vec4 BLENDINGCONSTANTs = glm::vec4(FLT_MAX);
	VkPipelineColorBlendAttachmentState BlendState = {};
};

/* Vertex Attribute Layout Specification:
		All vertex attributes should be interleaved because there is no easy way in Vulkan for de-interleaved path.
		Vertex Attributes are created as seperate objects because this helps debug visualization of the data
		Vertex Attributes are gonna be ordered by VertexAttributeLayout's std::vector elements order and this also defines attribute's in-shader location
		That means if position attribute is second element in "Attributes" std::vector, MaterialType that using this layout uses position attribute at location = 1 instead of 0.
*/
struct vertexattriblayout_vk {
	datatype_tgfx* Attributes;
	unsigned int Attribute_Number, size_perVertex;


	VkVertexInputBindingDescription BindingDesc;	//Currently, only one binding is supported because I didn't understand bindings properly.
	VkVertexInputAttributeDescription* AttribDescs;
	VkPrimitiveTopology PrimitiveTopology;
	unsigned char AttribDesc_Count;
};
struct vertexbuffer_vk {
	unsigned int VERTEX_COUNT;
	vertexattriblayout_vk* Layout;
	memoryblock_vk Block;
};
struct indexbuffer_vk {
	VkIndexType DATATYPE;
	VkDeviceSize IndexCount = 0;
	memoryblock_vk Block;
};
struct sampler_vk {
	VkSampler Sampler;
};