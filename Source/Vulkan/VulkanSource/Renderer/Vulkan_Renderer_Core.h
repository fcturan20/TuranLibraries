#pragma once
#include "Vulkan/VulkanSource/Vulkan_Includes.h"
#include "GFX/Renderer/GFX_Renderer.h"
#include "Vulkan_Resource.h"

namespace Vulkan {
	class VK_API Renderer : public GFX_API::Renderer {
		friend class Vulkan_Core;
		friend class GPU_ContentManager;


	public:
		VkCommandPool GraphicsCmdPool;
		vector<VkCommandBuffer> PerFrame_CommandBuffers;

		void Create_SwapchainDependentData();
		void Create_DrawPassFrameBuffers(unsigned char create_count, GFX_API::DrawPass& DrawPass);

		//INHERITANCE
		Renderer();

		//RenderGraph operations
		virtual void Start_RenderGraphCreation() override;
		virtual void Finish_RenderGraphCreation() override;
		virtual void Create_DrawPass(vector<GFX_API::SubDrawPass_Description>& SubDrawPasses, unsigned int RTSLOTSET_ID) override;

		//These functions are defined here because I don't want user to access these functions
		GFX_API::DrawPass& Find_DrawPass_byID(unsigned int ID);
		//If you want to get the ID of the DrawPass that contains intended SubDrawPass, you should pass a uint to DrawPass_ID
		GFX_API::SubDrawPass& Find_SubDrawPass_byID(unsigned int ID, unsigned int* DrawPass_ID = nullptr);

		void Recreate_Swapchain_RelatedResources();
		void Create_PrimaryCommandBuffer();
	};
}