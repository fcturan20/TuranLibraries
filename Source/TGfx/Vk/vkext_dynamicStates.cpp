#include "vkext_dynamicStates.h"

#include <string>

#include "TGfxStructs.h"
#include "vk_core.h"
#include "vk_includes.h"
#include "vk_predefinitions.h"
#include "vk_resource.h"

namespace TGFX
{
namespace Vulkan
{

FillRasterPipelineStateInfoPFN FillRasterPipelineStateInfo = {};
void FillRasterPipelineStateInfo_DynamicState(GPU_VKOBJ* gpu,
											  VkGraphicsPipelineCreateInfo* ci,
											  const TGfxRasterPipelineDescription* desc,
											  TGfxExtension* exts)
{
	/*
	auto* ext = ( vkext_dynamicStates* )gpu->ext()->m_exts[vkext_interface::dynamicStates_vkExtEnum];
	VkDynamicState* states     = *( VkDynamicState** )&ci->pDynamicState->pDynamicStates;
	uint32_t&       stateCount = *( uint32_t* )&ci->pDynamicState->dynamicStateCount;
	states[stateCount++]       = VK_DYNAMIC_STATE_CULL_MODE_EXT;
	states[stateCount++]       = VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT;
	states[stateCount++]       = VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT;
	states[stateCount++]       = VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE_EXT;
	states[stateCount++]       = VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT;
	if (ext->features2.extendedDynamicState2) {
	  states[stateCount++] = VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE_EXT;
	}
	if (ext->features3.extendedDynamicState3DepthClampEnable &&
		ext->features3.extendedDynamicState3DepthClipEnable &&
		ext->features3.extendedDynamicState3PolygonMode &&
		ext->props3.dynamicPrimitiveTopologyUnrestricted) {
	  states[stateCount++] = VK_DYNAMIC_STATE_DEPTH_CLAMP_ENABLE_EXT;
	  states[stateCount++] = VK_DYNAMIC_STATE_DEPTH_CLIP_ENABLE_EXT;
	  states[stateCount++] = VK_DYNAMIC_STATE_POLYGON_MODE_EXT;
	  states[stateCount++] = VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT;
	}
	*/
}

VkExtDynamicStates::VkExtDynamicStates(GPU_VKOBJ* gpu) : IVkExt(gpu, &props3, &features1)
{
	features1.pNext = &features2;
	features2.pNext = &featuresMaintenance4;
	// features2.pNext = &features3;
	features1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
	features3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
	featuresMaintenance4.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES;

	props3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_PROPERTIES_EXT;
	props3.pNext = nullptr;
}
void VkExtDynamicStates::inspect()
{
	if (!features1.extendedDynamicState)
	{
		return;
	}

	FillRasterPipelineStateInfo = FillRasterPipelineStateInfo_DynamicState;

	m_gpu->ext()->m_activeDevExtNames[m_gpu->ext()->m_devExtCount++] = (VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);

	if (features2.extendedDynamicState2)
	{
		m_gpu->ext()->m_activeDevExtNames[m_gpu->ext()->m_devExtCount++] =
			(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
	}
	/*
	if (features3.extendedDynamicState3DepthClampEnable &&
		features3.extendedDynamicState3DepthClipEnable &&
		features3.extendedDynamicState3PolygonMode && props3.dynamicPrimitiveTopologyUnrestricted) {
	  loadVkExtFunc(vkCmdSetDepthClampEnableEXT);
	  loadVkExtFunc(vkCmdSetDepthClipEnableEXT);
	  loadVkExtFunc(vkCmdSetColorWriteEnableEXT);
	  loadVkExtFunc(vkCmdSetPrimitiveTopologyEXT);
	  loadVkExtFunc(vkCmdSetStencilOpEXT);
	  loadVkExtFunc(vkCmdSetStencilTestEnableEXT);
	  m_gpu->ext()->m_activeDevExtNames[m_gpu->ext()->m_devExtCount++] =
		(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
	}*/
	if (featuresMaintenance4.maintenance4)
	{
		m_gpu->ext()->m_activeDevExtNames[m_gpu->ext()->m_devExtCount++] = VK_KHR_MAINTENANCE_4_EXTENSION_NAME;
	}
}
void VkExtDynamicStates::manage(VkStructureType structType, void* structPtr, unsigned int extCount, TGfxExtension* exts)
{
}
} // namespace Vulkan
} // namespace TGFX