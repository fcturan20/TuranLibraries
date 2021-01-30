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
		void Record_CurrentFramegraph();
		void Create_VulkanCalls();


		//Rendering operations
		virtual void Run() override;
		virtual void Render_DrawCall(GFX_API::GFXHandle VertexBuffer_ID, GFX_API::GFXHandle IndexBuffer_ID
			, GFX_API::GFXHandle MaterialInstance_ID, GFX_API::GFXHandle SubDrawPass_ID) override;
		virtual void SwapBuffers(GFX_API::GFXHandle WindowHandle, GFX_API::GFXHandle WindowPassHandle) override;
		//Source Buffer should be created with HOSTVISIBLE or FASTHOSTVISIBLE
		//Target Buffer should be created with DEVICELOCAL
		virtual void UploadTo_Buffer(GFX_API::GFXHandle SourceBuffer_Handle, GFX_API::GFXHandle TargetBuffer_Handle
			, unsigned int SourceBuffer_Offset, unsigned int TargetBuffer_Offset, unsigned int Size) override;
		//Source Buffer should be created with HOSTVISIBLE or FASTHOSTVISIBLE
		virtual void UploadTo_Image(GFX_API::GFXHandle SourceBuffer_Handle, GFX_API::GFXHandle Texture_Handle
			, unsigned int SourceBuffer_Offset, unsigned int Size, GFX_API::BoxRegion Texture_TargetRegion) override;
		virtual void ImageBarrier(GFX_API::GFXHandle TextureHandle, const GFX_API::IMAGE_ACCESS& LAST_ACCESS
			, const GFX_API::IMAGE_ACCESS& NEXT_ACCESS, GFX_API::GFXHandle BarrierTPHandle) override;


		//Transfer Operations
		virtual void TransferCall_ImUpload(VK_TransferPass* TP, VK_Texture* Image, VkDeviceSize StagingBufferOffset);
	};
}