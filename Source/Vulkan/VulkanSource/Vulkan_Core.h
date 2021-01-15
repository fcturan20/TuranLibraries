#pragma once
#include "Vulkan_Includes.h"
#include "GFX/GFX_Core.h"

#include "Renderer/Vulkan_Resource.h"
#include "Renderer/VK_GPUContentManager.h"
#include "Renderer/Vulkan_Renderer_Core.h"


namespace Vulkan {
	class VK_API Vulkan_Core : public GFX_API::GFX_Core {
	public:
		Vulkan_States VK_States;
		VkCommandPool FirstTriangle_CommandPool;
		vector<VkCommandBuffer> FirstTriangle_CommandBuffers;
		VkSemaphore Wait_GettingFramebuffer;
		VkSemaphore Wait_RenderingFirstTriangle;
		vector<VkFence> SwapchainFences;
		VkShaderModule* FirstShaderProgram;

		//Initialization Processes

		void Check_Computer_Specs(vector<GFX_API::GPUDescription>& GPUdescs);
		void Save_Monitors(vector<GFX_API::MonitorDescription>& Monitors);

		virtual void Check_Errors() override;
		//Window Operations

		virtual GFX_API::GFXHandle CreateWindow(const GFX_API::WindowDescription& Desc, GFX_API::GFXHandle* SwapchainTextureHandles, GFX_API::Texture_Properties& SwapchainTextureProperties) override;
		virtual void Change_Window_Resolution(GFX_API::GFXHandle Window, unsigned int width, unsigned int height) override;

		//Input (Keyboard-Controller) Operations
		virtual void Take_Inputs() override;

		//Callbacks
		static void GFX_Error_Callback(int error_code, const char* description);
		static void Window_ResizeCallback(GLFWwindow* window, int WIDTH, int HEIGHT);

		//Validation Layers are actived if TURAN_DEBUGGING is defined
		void Create_Instance();
		//Initialization calls this function if TURAN_DEBUGGING is defined
		void Setup_Debugging();
		//A Graphics Queue is created for learning purpose
		void Setup_LogicalDevice();


		//Destroy Operations
		virtual void Destroy_GFX_Resources() override;

		Vulkan_Core(vector<GFX_API::MonitorDescription>& Monitors, vector<GFX_API::GPUDescription>& GPUs, TuranAPI::Threading::JobSystem& JobSystem);
		virtual ~Vulkan_Core();
	};

}