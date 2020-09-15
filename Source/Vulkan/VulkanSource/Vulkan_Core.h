#pragma once
#include "Vulkan_Includes.h"
#include "GFX/GFX_Core.h"

#include "Renderer/Vulkan_Resource.h"
#include "Renderer/VK_GPUContentManager.h"
#include "Renderer/Vulkan_Renderer_Core.h"


namespace Vulkan {
	class VK_API Vulkan_Core : public GFX_API::GFX_Core {
		void Create_MainWindow();

		Vulkan_States VK_States;
		VkCommandPool FirstTriangle_CommandPool;
		vector<VkCommandBuffer> FirstTriangle_CommandBuffers;
		VkSemaphore Wait_GettingFramebuffer;
		VkSemaphore Wait_RenderingFirstTriangle;
		vector<VkFence> SwapchainFences;
		VkShaderModule* FirstShaderProgram;

		virtual void Initialization() override;
		virtual void Check_Computer_Specs() override;
		virtual void Save_Monitors() override;
		virtual void Create_Renderer() override;

		virtual void Check_Errors() override;
		//Window Operations
		virtual void Change_Window_Resolution(unsigned int width, unsigned int height) override;
		virtual void Swapbuffers_ofMainWindow() override;
		virtual void Show_RenderTarget_onWindow(unsigned int RenderTarget_GFXID) override;

		//Input (Keyboard-Controller) Operations
		virtual void Take_Inputs() override;

		//Callbacks
		static void GFX_Error_Callback(int error_code, const char* description);
		static void Window_ResizeCallback(GLFWwindow* window, int WIDTH, int HEIGHT);

		//Validation Layers are actived if TURAN_DEBUGGING is defined
		void Create_Instance();
		//Initialization calls this function if TURAN_DEBUGGING is defined
		void Setup_Debugging();
		void Create_Surface_forWindow();
		//A Graphics Queue is created for learning purpose
		void Setup_LogicalDevice();
		void Create_MainWindow_SwapChain();
		void Create_CommandBuffers();
		void Begin_RenderPass(unsigned int CB_Index);
		void Begin_DrawingFirstTriangle(unsigned int CB_Index);
		void Finish_DrawingProgresses(unsigned int CB_Index);
		void Create_Semaphores();

		//Recreation Operations
		void Recreate_SwapchainDependentData();
		void RecordCommandBuffer_Again();

		//Destroy Operations
		void Destroy_SwapchainDependentData();
		virtual void Destroy_GFX_Resources() override;
	public:
		Vulkan_Core();
		virtual ~Vulkan_Core();
	};

}