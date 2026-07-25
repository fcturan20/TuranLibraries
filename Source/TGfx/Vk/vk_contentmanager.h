#pragma once
#include "vk_predefinitions.h"
#include "vk_resource.h"

namespace TGFX
{
namespace Vulkan
{

struct BindingTableInstance : public VkObjectBase<BindingTableInstance, TGfxBindingTable, VkObjTypes::BINDINGTABLEINST>,
							  public GpuObject
{
	BindingTableInstance(GPU* gpu)
		: GpuObject(gpu), vk_layout(gpu->ReferenceManager), vk_set(gpu->ReferenceManager),
		  vk_pool(gpu->ReferenceManager)
	{
	}
	uint16_t GetExtraFlags() { return UINT16_MAX; }
	uint8_t m_gpu;
	uint32_t m_elementCount;
	bool m_isStatic = false; // Static binding table or dynamic?
	void* m_descs = nullptr;

	VkDescriptorSetLayoutHnd vk_layout;
	VkDescriptorSetHnd vk_set;
	VkDescriptorPoolHnd vk_pool;
	VkShaderStageFlags vk_stages = 0;
	VkDescriptorType vk_descType;
};

extern class ContentManagerContext* GContentManagerContext = nullptr;
class ContentManagerContext
{
public:
	VK_LINEAR_OBJARRAY<Texture, 1 << 24> textures;
	VK_LINEAR_OBJARRAY<Pipeline, 1 << 24> pipelines;
	VK_LINEAR_OBJARRAY<ShaderSource, 1 << 24> shadersources;
	VK_LINEAR_OBJARRAY<Sampler, 1 << 16> samplers;
	VK_LINEAR_OBJARRAY<Buffer> buffers;
	VK_LINEAR_OBJARRAY<BindingTableInstance, 1 << 16> bindingtableinsts;
	VK_LINEAR_OBJARRAY<Heap, 1 << 10> heaps;
	VK_LINEAR_OBJARRAY<Fence, 1 << 20> Fences;

	static void HookContentManager(ITGfxResourceManager* r);
	static TCResult Initialize();
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(BindingTableInstance, Vk)

} // namespace Vulkan
} // namespace TGFX