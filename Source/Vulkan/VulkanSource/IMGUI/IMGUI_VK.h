#pragma once
#include "Vulkan/VulkanSource/Vulkan_Includes.h"

namespace Vulkan {
	class VK_API IMGUI_VK {
	public:
		TAPIResult Initialize(GFX_API::GFXHandle SubPass, VkDescriptorPool DescPool);
		void NewFrame();
		void Render_AdditionalWindows();
		void Render_toCB(VkCommandBuffer cb);
		//Creates command pool/buffer and sends font textures
		void UploadFontTextures();
		void Destroy_IMGUIResources();
		static void DisplayTexture(GFX_API::GFXHandle TextureHandle, const unsigned int& Display_WIDTH, const unsigned int& Display_HEIGHT, bool should_Flip_Vertically);
	};
}