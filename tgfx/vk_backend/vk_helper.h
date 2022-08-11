#pragma once
#include "vk_predefinitions.h"


inline VkShaderStageFlags GetVkShaderStageFlags_fromTGFXHandle(shaderStageFlag_tgfxhnd handle) {
	if (VKCONST_isPointerContainVKFLAG) { return *((VkShaderStageFlags*)&handle); }
	else {return *((VkShaderStageFlags*)handle);}
}