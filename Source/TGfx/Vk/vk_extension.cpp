#include <atomic>
#include <glm/glm.hpp>

#include "vk_core.h"
#include "vk_includes.h"
#include "vk_resource.h"
#include "vk_extension.h"

// Extensions
#include "vkext_depthstencil.h"
#include "vkext_descIndexing.h"
#include "vkext_dynamicStates.h"
#include "vkext_dynamic_rendering.h"
#include "vkext_timelineSemaphore.h"

namespace TGFX
{
namespace Vulkan
{

IVkExt::IVkExt(GPU_VKOBJ* gpu, void* propsStruct, void* featuresStruct)
{
	m_gpu = gpu;
	if (propsStruct)
	{
		Append_pNext(&gpu->vk_propsDev, propsStruct);
	}
	if (featuresStruct)
	{
		Append_pNext(&gpu->vk_featuresDev, featuresStruct);
	}
}

void ExtensionManager::createExtManager(GPU_VKOBJ* gpu)
{
	ExtensionManager* mngr = new (VKGLOBAL_VIRMEM_EXTS) ExtensionManager;
	mngr->m_exts = new (VKGLOBAL_VIRMEM_EXTS) vkext_interface*[vkext_interface::vkext_count];
	mngr->m_GPU = gpu;

	gpu->vk_featuresDev.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	gpu->vk_featuresDev.pNext = nullptr;
	gpu->vk_propsDev.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	gpu->vk_propsDev.pNext = nullptr;

	mngr->m_exts[IVkExt::DepthStencilExtension] = new (VKGLOBAL_VIRMEM_EXTS) vkext_depthStencil(gpu);
	mngr->m_exts[IVkExt::DescriptorIndexingExtension] = new (VKGLOBAL_VIRMEM_EXTS) vkext_descIndexing(gpu);
	mngr->m_exts[IVkExt::TimelineSemaphoresExtension] = new (VKGLOBAL_VIRMEM_EXTS) vkext_timelineSemaphore(gpu);
	mngr->m_exts[IVkExt::DynamicRenderingExtension] = new (VKGLOBAL_VIRMEM_EXTS) vkext_dynamicRendering(gpu);
	mngr->m_exts[IVkExt::DynamicStatesExtension] = new (VKGLOBAL_VIRMEM_EXTS) VkExtDynamicStates(gpu);

	gpu->ext() = mngr;
}

void ExtensionManager::inspect()
{
	uint32_t extCount = 0;
	vkEnumerateDeviceExtensionProperties(m_GPU->vk_physical, nullptr, &extCount, nullptr);
	uint32_t allocSize = sizeof(const char*) * extCount;
	m_activeDevExtNames = (const char**)VK_MEMOFFSET_TO_POINTER(vk_virmem::allocatePage(allocSize));
	vm->commit(m_activeDevExtNames, sizeof(allocSize));

	VkPhysicalDeviceSubgroupProperties subgroupProps;
	subgroupProps.pNext = nullptr;
	subgroupProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
	Append_pNext(&m_GPU->vk_propsDev, &subgroupProps);

	vkGetPhysicalDeviceFeatures2(m_GPU->vk_physical, &m_GPU->vk_featuresDev);
	vkGetPhysicalDeviceProperties2(m_GPU->vk_physical, &m_GPU->vk_propsDev);

	// SAVE BASIC INFOs TO THE GPU DESC
	{
		stringSys->createString(
			tlStringUTF16, (void**)&m_GPU->desc.name, L"%s", m_GPU->vk_propsDev.properties.deviceName);
		m_GPU->desc.driverVersion = m_GPU->vk_propsDev.properties.driverVersion;
		m_GPU->desc.gfxApiVersion = m_GPU->vk_propsDev.properties.apiVersion;
		m_GPU->desc.driverVersion = m_GPU->vk_propsDev.properties.driverVersion;
		m_GPU->desc.type = vk_findGPUTypeTgfx(m_GPU->vk_propsDev.properties.deviceType);
	}

	for (size_t extIndx = 0; extIndx < vkext_interface::vkext_count; extIndx++)
	{
		vkext_interface* ext = m_exts[extIndx];
		if (ext)
		{
			ext->inspect();
		}
	}

	m_activeDevExtNames[m_devExtCount++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	m_activeDevExtNames[m_devExtCount++] = VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME;
}
const char** ExtensionManager::getEnabledExtensionNames(uint32_t* count)
{
	*count = m_devExtCount;
	for (uint32_t i = 0; i < *count; i++)
	{
		printf("Extension: %s\n", m_activeDevExtNames[i]);
	}
	return m_activeDevExtNames;
}

} // namespace Vulkan
} // namespace TGFX