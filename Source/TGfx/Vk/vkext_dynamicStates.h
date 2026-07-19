#pragma once
#include <float.h>

#include "vk_extension.h"
#include "vk_predefinitions.h"

namespace TGFX
{
namespace Vulkan
{

struct VkExtDynamicStates : public IVkExt
{
	VkExtDynamicStates(GPU_VKOBJ* gpu);
	virtual void Inspect() override;
	virtual void Manage(VkStructureType structType,
						void* structPtr,
						unsigned int extCount,
						TGfxExtension* exts) override;
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT features1;
	VkPhysicalDeviceExtendedDynamicState2FeaturesEXT features2;
	VkPhysicalDeviceExtendedDynamicState3FeaturesEXT features3;
	VkPhysicalDeviceMaintenance4Features featuresMaintenance4;
	VkPhysicalDeviceExtendedDynamicState3PropertiesEXT props3;
};

// These are TGFX CORE functionalities, so func pointers are here
typedef void (*FillRasterPipelineStateInfoPFN)(GPU_VKOBJ* gpu,
											   VkGraphicsPipelineCreateInfo* ci,
											   const TGfxRasterPipelineDescription* desc,
											   TGfxExtension* exts);
extern FillRasterPipelineStateInfoPFN FillRasterPipelineStateInfo;

} // namespace Vulkan
} // namespace TGFX