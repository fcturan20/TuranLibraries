#include "predefinitions_vk.h"

class IMGUI_VK {
public:
	enum class IMGUI_STATUS {
		UNINITIALIZED = 0,
		INITIALIZED = 1,
		NEW_FRAME = 2,
		RENDERED = 3
	};
	result_tgfx Initialize(subdrawpass_tgfx_handle SubPass);
	subdrawpass_tgfx_handle Get_SubDrawPassHandle() const;
	IMGUI_STATUS Get_IMGUIStatus();
	void Change_DrawPass(subdrawpass_tgfx_handle Subpass);
	void NewFrame();
	void Render_AdditionalWindows();
	void Render_toCB(VkCommandBuffer cb);
	//Creates command pool/buffer and sends font textures
	void UploadFontTextures();
	void Destroy_IMGUIResources();
private:
	IMGUI_STATUS STAT = IMGUI_STATUS::UNINITIALIZED;
	subdrawpass_tgfx_handle Subdrawpass;
	VkDescriptorPool descpool;
};