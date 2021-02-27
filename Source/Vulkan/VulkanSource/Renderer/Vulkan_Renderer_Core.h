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
		vector<VK_Semaphore> Semaphores;


		VkDescriptorPool IMGUIPOOL = VK_NULL_HANDLE;


		bool Is_ConstructingRenderGraph();
		unsigned char Get_FrameIndex(bool is_LastFrame);

		//INHERITANCE
		Renderer();

		//RenderGraph operations
		virtual void Start_RenderGraphConstruction() override;
		virtual void Finish_RenderGraphConstruction(GFX_API::GFXHandle IMGUI_Subpass) override;
		//This function is defined in the FGAlgorithm.cpp
		virtual void Destroy_RenderGraph() override;
		virtual TAPIResult Create_DrawPass(const vector<GFX_API::SubDrawPass_Description>& SubDrawPasses, GFX_API::GFXHandle RTSLOTSET_ID, const vector<GFX_API::PassWait_Description>& WAITs, const char* NAME, vector<GFX_API::GFXHandle>& SubDrawPassIDs, GFX_API::GFXHandle& DPHandle) override;
		virtual TAPIResult Create_TransferPass(const vector<GFX_API::PassWait_Description>& WaitDescriptions, const GFX_API::TRANFERPASS_TYPE& TP_TYPE, const string& NAME, GFX_API::GFXHandle& TPHandle) override;
		virtual TAPIResult Create_WindowPass(const vector<GFX_API::PassWait_Description>& WaitDescriptions, const string& NAME, GFX_API::GFXHandle& WindowPassHandle) override;
		bool Check_WaitHandles();
		void Record_CurrentFramegraph();



		//Rendering operations
		virtual void Run() override;
		virtual void DrawDirect(GFX_API::GFXHandle VertexBuffer_ID, GFX_API::GFXHandle IndexBuffer_ID, unsigned int Count, unsigned int VertexOffset,
			unsigned int FirstIndex, unsigned int InstanceCount, unsigned int FirstInstance, GFX_API::GFXHandle MaterialInstance_ID, GFX_API::GFXHandle SubDrawPass_ID) override;
		virtual void SwapBuffers(GFX_API::GFXHandle WindowHandle, GFX_API::GFXHandle WindowPassHandle) override;
		//Source Buffer should be created with HOSTVISIBLE or FASTHOSTVISIBLE
		//Target Buffer should be created with DEVICELOCAL
		virtual void CopyBuffer_toBuffer(GFX_API::GFXHandle TransferPassHandle, GFX_API::GFXHandle SourceBuffer_Handle, GFX_API::BUFFER_TYPE SourceBufferTYPE,
			GFX_API::GFXHandle TargetBuffer_Handle, GFX_API::BUFFER_TYPE TargetBufferTYPE, unsigned int SourceBuffer_Offset, unsigned int TargetBuffer_Offset, unsigned int Size) override;
		//Source Buffer should be created with HOSTVISIBLE or FASTHOSTVISIBLE
		virtual void CopyBuffer_toImage(GFX_API::GFXHandle TransferPassHandle, GFX_API::GFXHandle SourceBuffer_Handle, GFX_API::BUFFER_TYPE SourceBufferTYPE,
			GFX_API::GFXHandle TextureHandle, unsigned int SourceTextureLayer, GFX_API::BoxRegion TargetTextureRegion, unsigned int TargetTextureLayer) override;
		virtual void CopyImage_toImage(GFX_API::GFXHandle TransferPassHandle, GFX_API::GFXHandle SourceTextureHandle, GFX_API::GFXHandle TargetTextureHandle,
			unsigned int SourceTextureLayer, uvec3 SourceTextureOffset, uvec3 CopySize, uvec3 TargetTextureOffset, unsigned int TargetTextureLayer) override;
		virtual void ImageBarrier(GFX_API::GFXHandle TextureHandle, const GFX_API::IMAGE_ACCESS& LAST_ACCESS
			, const GFX_API::IMAGE_ACCESS& NEXT_ACCESS, unsigned int LayerIndex, GFX_API::GFXHandle BarrierTPHandle) override;

	};
}