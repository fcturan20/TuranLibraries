#pragma once
#include "Vulkan/VulkanSource/Vulkan_Includes.h"
#include "GFX/Renderer/GFX_Renderer.h"
#include "Vulkan_Resource.h"

namespace Vulkan {
	class VK_API Renderer : public GFX_API::Renderer {
	public:
		vector<VK_DrawPass*> DrawPasses;
		vector<VK_TransferPass*> TransferPasses;
		VK_FrameGraph FrameGraphs[2];
		//All of this vectors are at a size that is divisible by FrameCount
		vector<VK_CommandBuffer*> CommandBuffers;
		vector<VkSemaphore> Semaphores;
		VkFence RenderGraph_RunFences[2];


		//Swapchain Sync
		VkSemaphore Waitfor_PresentingSwapchain,	//Wait for this framebuffer to finish presenting on the window
			Waitfor_SwapchainRenderGraphIdle;				//Wait for RenderGraph to finish the passes that uses the swapchain
		VK_CommandBuffer* Swapchain_CreationCB;


		bool Is_ConstructingRenderGraph();
		unsigned char Get_FrameIndex(bool is_LastFrame);

		//INHERITANCE
		Renderer();

		//RenderGraph operations
		virtual void Start_RenderGraphConstruction() override;
		virtual void Finish_RenderGraphConstruction() override;
		virtual GFX_API::GFXHandle Create_DrawPass(const vector<GFX_API::SubDrawPass_Description>& SubDrawPasses, GFX_API::GFXHandle RTSLOTSET_ID, const vector<GFX_API::PassWait_Description>& WAITs, GFX_API::GFXHandle* SubDrawPassIDs, const char* NAME) override;
		virtual GFX_API::GFXHandle Create_TransferPass(const GFX_API::TransferPass_Description& TransferPassDescription) override;
		bool Check_WaitHandles();
		void Create_VulkanCalls();


		//Rendering operations
		virtual void Run() override;
		virtual void Render_DrawCall(GFX_API::DrawCall_Description drawcall) override;

		/*
		//These functions are defined here because I don't want user to access these functions
		VK_DrawPass* Find_DrawPass_byID(unsigned int ID);
		//If you want to get the ID of the DrawPass that contains intended SubDrawPass, you should pass a uint to DrawPass_ID
		VK_SubDrawPass* Find_SubDrawPass_byID(GFX_API::GFXHandle ID, unsigned int* DrawPass_ID = nullptr);*/
	};
}