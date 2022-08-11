#pragma once
#include "vk_predefinitions.h"
#ifndef NO_IMGUI
struct imgui_vk {
public:
	enum class IMGUI_STATUS {
		UNINITIALIZED = 0,
		INITIALIZED = 1,
		NEW_FRAME = 2,
		RENDERED = 3
	};
	inline renderSubPass_tgfxhnd Get_SubDrawPassHandle() const { return Subdrawpass; }
	inline IMGUI_STATUS Get_IMGUIStatus() { return STAT; }
	void Change_DrawPass(renderSubPass_tgfxhnd Subpass);
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
	renderSubPass_tgfxhnd Subdrawpass;
	friend void Create_IMGUI();
};
#endif