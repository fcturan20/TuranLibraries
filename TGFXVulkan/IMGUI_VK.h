#pragma once
#include "Vulkan_Includes.h"

class IMGUI_VK {
public:
	enum class IMGUI_STATUS {
		UNINITIALIZED = 0,
		INITIALIZED = 1,
		NEW_FRAME = 2,
		RENDERED = 3
	};
	tgfx_result Initialize(tgfx_subdrawpass SubPass, VkDescriptorPool DescPool);
	tgfx_subdrawpass Get_SubDrawPassHandle() const;
	IMGUI_STATUS Get_IMGUIStatus();
	void Change_DrawPass(tgfx_subdrawpass Subpass);
	void NewFrame();
	void Render_AdditionalWindows();
	void Render_toCB(VkCommandBuffer cb);
	//Creates command pool/buffer and sends font textures
	void UploadFontTextures();
	void Destroy_IMGUIResources();
private:
	IMGUI_STATUS STAT = IMGUI_STATUS::UNINITIALIZED;
	tgfx_subdrawpass Subdrawpass;
};