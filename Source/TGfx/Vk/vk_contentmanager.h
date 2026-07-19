#pragma once
#include "vk_predefinitions.h"
#include "vk_resource.h"

namespace TGFX
{
namespace Vulkan
{

class ContentManagerContext* GContentManagerContext = nullptr;
class ContentManagerContext
{
public:
	VK_LINEAR_OBJARRAY<TEXTURE_VKOBJ, TGfxTexture, 1 << 24> textures;
	VK_LINEAR_OBJARRAY<PIPELINE_VKOBJ, TGfxPipeline, 1 << 24> pipelines;
	VK_LINEAR_OBJARRAY<SHADERSOURCE_VKOBJ, TGfxShaderSource, 1 << 24> shadersources;
	VK_LINEAR_OBJARRAY<SAMPLER_VKOBJ, TGfxSampler, 1 << 16> samplers;
	VK_LINEAR_OBJARRAY<BUFFER_VKOBJ, TGfxBuffer> buffers;
	VK_LINEAR_OBJARRAY<BINDINGTABLEINST_VKOBJ, TGfxBindingTable, 1 << 16> bindingtableinsts;
	VK_LINEAR_OBJARRAY<HEAP_VKOBJ, TGfxHeap, 1 << 10> heaps;

	static void BindContentManagerFunctions(ITGfxResourceManager* r);
	static TCResult Initialize();
};
struct BINDINGTABLEINST_VKOBJ
{
	VkConstHndType HANDLETYPE = VKHANDLETYPEs::BINDINGTABLEINST;

	static uint16_t GET_EXTRAFLAGS(BINDINGTABLEINST_VKOBJ* obj) { return UINT16_MAX; }
	uint8_t m_gpu;
	uint32_t m_elementCount;
	bool m_isStatic = false; // Static binding table or dynamic?
	void* m_descs = nullptr;

	VkDescriptorSetLayout vk_layout;
	VkDescriptorSet vk_set = {};
	VkDescriptorPool vk_pool = {};
	VkShaderStageFlags vk_stages = 0;
	VkDescriptorType vk_descType;
};

} // namespace Vulkan
} // namespace TGFX