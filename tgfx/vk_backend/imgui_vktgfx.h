#pragma once
#include "predefinitions_vk.h"
#ifndef NO_IMGUI
struct imgui_vk {
public:
	enum class IMGUI_STATUS {
		UNINITIALIZED = 0,
		INITIALIZED = 1,
		NEW_FRAME = 2,
		RENDERED = 3
	};
	inline subdrawpass_tgfx_handle Get_SubDrawPassHandle() const { return Subdrawpass; }
	inline IMGUI_STATUS Get_IMGUIStatus() { return STAT; }
	void Change_DrawPass(subdrawpass_tgfx_handle Subpass);
	void NewFrame();
	void Render_AdditionalWindows();
	void Render_toCB(VkCommandBuffer cb);
	//Creates command pool/buffer and sends font textures
	void UploadFontTextures();
	void Destroy_IMGUIResources();
	struct imgui_vk_hidden;
	imgui_vk_hidden* hidden;
	VkDescriptorPool descpool;
private:
	IMGUI_STATUS STAT = IMGUI_STATUS::UNINITIALIZED;
	subdrawpass_tgfx_handle Subdrawpass;
	friend void Create_IMGUI();
};
#endif