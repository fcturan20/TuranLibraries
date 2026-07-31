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
	VkGpuObjectArray<Texture, 1 << 24> Textures;
	VkGpuObjectArray<Pipeline, 1 << 24> Pipelines;
	VkGpuObjectArray<ShaderSource, 1 << 24> ShaderSources;
	VkGpuObjectArray<Sampler, 1 << 16> Samplers;
	VkGpuObjectArray<Buffer, 1 << 24> Buffers;
	VkGpuObjectArray<BindingTableInstance, 1 << 16> BindingTableInsts;
	VkGpuObjectArray<Heap, 1 << 10> Heaps;
	VkGpuObjectArray<Fence, 1 << 20> Fences;
	VkGpuObjectArray<Swapchain, 1 << 20> Swapchains;

	static void HookContentManager(ITGfxResourceManager* r);
	static TCResult Initialize();
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(BindingTableInstance, Vk)

} // namespace Vulkan
} // namespace TGFX