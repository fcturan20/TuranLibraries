#pragma once
#include "Vulkan/VulkanSource/Vulkan_Includes.h"
#include "GFX/Renderer/GFX_Renderer.h"
#include "Vulkan_Resource.h"

namespace Vulkan {
	class VK_API Renderer : public GFX_API::Renderer {
	public:
		vector<VK_DrawPass*> DrawPasses;
		vector<VK_TransferPass*> TransferPasses;
		vector<VK_WindowPass*> WindowPasses;
		VK_FrameGraph FrameGraphs[2];
		//All of this vectors are at a size that is divisible by FrameCount
		vector<VK_CommandBuffer*> CommandBuffers;
		vector<VkSemaphore> Semaphores;
		VkFence RenderGraph_RunFences[2];


		//Swapchain Sync
		VkSemaphore Waitfor_PresentingSwapchain,			//Wait for this framebuffer to finish presenting on the window
			Waitfor_SwapchainRenderGraphIdle;				//Wait for RenderGraph to finish the passes that uses the swapchain


		bool Is_ConstructingRenderGraph();
		unsigned char Get_FrameIndex(bool is_LastFrame);

		//INHERITANCE
		Renderer();

		//RenderGraph operations
		virtual void Start_RenderGraphConstruction() override;
		virtual void Finish_RenderGraphConstruction() override;
		virtual TAPIResult Create_DrawPass(const vector<GFX_API::SubDrawPass_Description>& SubDrawPasses, GFX_API::GFXHandle RTSLOTSET_ID, const vector<GFX_API::PassWait_Description>& WAITs, const char* NAME, vector<GFX_API::GFXHandle>& SubDrawPassIDs, GFX_API::GFXHandle& DPHandle) override;
		virtual TAPIResult Create_TransferPass(const vector<GFX_API::PassWait_Description>& WaitDescriptions, const GFX_API::TRANFERPASS_TYPE& TP_TYPE, const string& NAME, GFX_API::GFXHandle& TPHandle) override;
		virtual TAPIResult Create_WindowPass(const vector<GFX_API::PassWait_Description>& WaitDescriptions, const string& NAME, GFX_API::GFXHandle& WindowPassHandle) override;
		bool Check_WaitHandles();
		void Run_CurrentFramegraph();
		void Create_VulkanCalls();


		//Rendering operations
		virtual void Run() override;
		virtual void Render_DrawCall(GFX_API::GFXHandle VertexBuffer_ID, GFX_API::GFXHandle IndexBuffer_ID, GFX_API::GFXHandle MaterialInstance_ID, GFX_API::GFXHandle SubDrawPass_ID) override;
		virtual void SwapBuffers(GFX_API::GFXHandle WindowHandle, GFX_API::GFXHandle WindowPassHandle, const GFX_API::IMAGEUSAGE& PREVIOUS_IMUSAGE, const GFX_API::SHADERSTAGEs_FLAG& PREVIOUS_SHADERSTAGE) override;

		//Transfer Operations
		virtual void TransferCall_ImUpload(VK_TransferPass* TP, VK_Texture* Image, VkDeviceSize StagingBufferOffset);
	};
}