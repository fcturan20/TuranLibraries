#pragma once
#include <vulkan/vulkan.h>

struct core_public {
	
};


struct gpu_vk{
    
};

struct monitor_vk {
	unsigned int width, height, color_bites, refresh_rate, physical_width, physical_height;
	const char* name;
	//I will fill this structure when I investigate monitor configurations deeper!
};

struct window_vk {

};

struct initialization_secondstageinfo{
	gpu_vk* renderergpu;
	//Global Descriptors Info
	unsigned int MaxMaterialCount, MaxSumMaterial_SampledTexture, MaxSumMaterial_ImageTexture, MaxSumMaterial_UniformBuffer, MaxSumMaterial_StorageBuffer,
		GlobalShaderInput_UniformBufferCount, GlobalShaderInput_StorageBufferCount, GlobalShaderInput_SampledTextureCount, GlobalShaderInput_ImageTextureCount;
	bool shouldActivate_DearIMGUI : 1, isUniformBuffer_Index1 : 1, isSampledTexture_Index1 : 1;
};
struct VK_ShaderStageFlag {
	VkShaderStageFlags flag;
};