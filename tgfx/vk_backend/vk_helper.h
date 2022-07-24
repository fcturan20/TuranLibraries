#pragma once
#include "predefinitions_vk.h"


inline VkShaderStageFlags GetVkShaderStageFlags_fromTGFXHandle(shaderstageflag_tgfx_handle handle) {
	if (VKCONST_isPointerContainVKFLAG) { return *((VkShaderStageFlags*)&handle); }
	else {return *((VkShaderStageFlags*)handle);}
}