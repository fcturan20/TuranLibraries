#pragma once
#include "Vulkan_Includes.h"
#include "GFX/GFX_Core.h"

#include "Renderer/Vulkan_Resource.h"
#include "Renderer/VK_GPUContentManager.h"
#include "Renderer/Vulkan_Renderer_Core.h"
#include "IMGUI/IMGUI_VK.h"


namespace Vulkan {
	class VK_API Vulkan_Core : public GFX_API::GFX_Core {
	public:
		Vulkan_States VK_States;
		IMGUI_VK* VK_IMGUI = nullptr;
		//Instead of checking each window each frame, just check this
		bool isAnyWindowResized = false;

		//Initialization Processes

		void Check_Computer_Specs(vector<GFX_API::GPUDescription>& GPUdescs);
		void Save_Monitors(vector<GFX_API::MonitorDescription>& Monitors);

		virtual void Check_Errors() override;
		virtual bool GetTextureTypeLimits(GFX_API::TEXTURE_DIMENSIONs dims, GFX_API::TEXTURE_ORDER dataorder, GFX_API::TEXTURE_CHANNELs channeltype, GFX_API::TEXTUREUSAGEFLAG usageflag,
			unsigned int GPUIndex, unsigned int& MAXWIDTH, unsigned int& MAXHEIGHT, unsigned int& MAXDEPTH, unsigned int& MAXMIPLEVEL) override;
		virtual void GetSupportedAllocations_ofTexture(const GFX_API::Texture_Description& TEXTURE_desc, unsigned int GPUIndex, 
			unsigned int& SupportedMemoryTypesBitset) override;
		//Window Operations

		virtual GFX_API::GFXHandle CreateWindow(const GFX_API::WindowDescription& Desc, GFX_API::GFXHandle* SwapchainTextureHandles, GFX_API::Texture_Description& SwapchainTextureProperties) override;
		virtual void Change_Window_Resolution(GFX_API::GFXHandle Window, unsigned int width, unsigned int height) override;
		virtual unsigned char Get_WindowFrameIndex(GFX_API::GFXHandle WindowHandle) override;
		vector<GFX_API::GFXHandle>& Get_WindowHandles();

		//Input (Keyboard-Controller) Operations
		virtual void Take_Inputs() override;

		bool Create_WindowSwapchain(WINDOW* Vulkan_Window, unsigned int WIDTH, unsigned int HEIGHT, VkSwapchainKHR* SwapchainOBJ, GFX_API::GFXHandle* SwapchainTextureHandles);
		//Callbacks
		static void GFX_Error_Callback(int error_code, const char* description);
		static void Window_ResizeCallback(GLFWwindow* window, int WIDTH, int HEIGHT);

		void Create_Instance();
		void Setup_Debugging();


		//Destroy Operations
		virtual void Destroy_GFX_Resources() override;

		Vulkan_Core(vector<GFX_API::MonitorDescription>& Monitors, vector<GFX_API::GPUDescription>& GPUs, TuranAPI::Threading::JobSystem* JobSystem);
		//All of the sizes should be in bytes
		TAPIResult Start_SecondStage(unsigned char GPUIndex, const vector<GFX_API::MemoryType>& Allocations, bool Activate_dearIMGUI);
		virtual ~Vulkan_Core();
	};

}