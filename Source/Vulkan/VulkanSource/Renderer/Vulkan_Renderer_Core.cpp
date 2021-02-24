#include "Vulkan_Renderer_Core.h"
#include "VK_GPUContentManager.h"
#include "Vulkan/VulkanSource/Renderer/Vulkan_Resource.h"
#include "TuranAPI/Profiler_Core.h"
#include "Vulkan/VulkanSource/Vulkan_Core.h"
#define VKContentManager ((Vulkan::GPU_ContentManager*)GFXContentManager)
#define VKGPU (((Vulkan::Vulkan_Core*)GFX)->VK_States.GPU_TO_RENDER)

namespace Vulkan {
	Renderer::Renderer() {

	}

	//Create framegraphs to lower the cost of per frame rendergraph optimization (Pass Culling, Pass Merging, Wait Culling etc) proccess!
	void Create_FrameGraphs(const vector<VK_DrawPass*>& DrawPasses, const vector<VK_TransferPass*>& TransferPasses, const vector<VK_WindowPass*>& WindowPasses, VK_FrameGraph* FrameGraphs);
	//Merge branches into submits that will execute on GPU's Queues
	//Also link submits (attach signal semaphores to submits and set wait semaphores according to it)
	void Create_VkSubmits(VK_FrameGraph* CurrentFramegraph, VK_FrameGraph* LastFrameGraph, vector<VK_Semaphore>& Semaphores);

	void Renderer::Start_RenderGraphConstruction() {
		VKContentManager->Resource_Finalizations();

		if (Record_RenderGraphConstruction) {
			LOG_CRASHING_TAPI("GFXRENDERER->Start_RenderGraphCreation() has failed because you called it before!");
			return;
		}
		Record_RenderGraphConstruction = true;
	}
	PassType FindWaitedPassType(GFX_API::SHADERSTAGEs_FLAG flag) {
		bool SupportedOnes = 0;
		if (flag.COLORRTOUTPUT || flag.FRAGMENTSHADER || flag.VERTEXSHADER) {
			if (flag.SWAPCHAINDISPLAY || flag.TRANSFERCMD) {
				LOG_CRASHING_TAPI("You set Wait Info->Stage wrong!");
				return PassType::ERROR;
			}
			return PassType::DP;
		}
		if (flag.SWAPCHAINDISPLAY) {
			if (flag.TRANSFERCMD) {
				LOG_CRASHING_TAPI("You set Wait Info->Stage wrong!");
				return PassType::ERROR;
			}
			return PassType::WP;
		}
		if (flag.TRANSFERCMD) {
			return PassType::TP;
		}
		LOG_NOTCODED_TAPI("You set WaitInfo->Stage to a stage that isn't supported right now by FindWaitedTPType!", true);
		return PassType::ERROR;
	}
	bool Renderer::Check_WaitHandles() {
		for (unsigned int DPIndex = 0; DPIndex < DrawPasses.size(); DPIndex++) {
			VK_DrawPass* DP = DrawPasses[DPIndex];
			for (unsigned int WaitIndex = 0; WaitIndex < DP->WAITs.size(); WaitIndex++) {
				GFX_API::PassWait_Description& Wait_desc = DP->WAITs[WaitIndex];
				if ((*Wait_desc.WaitedPass) == nullptr) {
					LOG_ERROR_TAPI("You forgot to set wait handle of one of the draw passes!");
					return false;
				}

				//Search through all pass types
				switch (FindWaitedPassType(Wait_desc.WaitedStage)) {
					case PassType::DP:
					{
						bool is_Found = false;
						for (unsigned int CheckedDP = 0; CheckedDP < DrawPasses.size(); CheckedDP++) {
							if (*Wait_desc.WaitedPass == DrawPasses[CheckedDP]) {
								is_Found = true;
								break;
							}
						}
						if (!is_Found) {
							LOG_ERROR_TAPI("One of the draw passes waits for an draw pass but given pass isn't found!");
							return false;
						}
						break;
					}
					case PassType::TP:
					{
						bool is_Found = false;
						for (unsigned int CheckedTP = 0; CheckedTP < TransferPasses.size(); CheckedTP++) {
							if (*Wait_desc.WaitedPass == TransferPasses[CheckedTP]) {
								is_Found = true;
								break;
							}
						}
						if (!is_Found) {
							LOG_ERROR_TAPI("One of the draw passes waits for an transfer pass but given pass isn't found!");
							return false;
						}
						break;
					}
					case PassType::WP:
					{
						bool is_Found = false;
						for (unsigned int CheckedWP = 0; CheckedWP < WindowPasses.size(); CheckedWP++) {
							if (*Wait_desc.WaitedPass == WindowPasses[CheckedWP]) {
								is_Found = true;
								break;
							}
						}
						if (!is_Found) {
							LOG_ERROR_TAPI("One of the draw passes waits for an window pass but given pass isn't found!");
							return false;
						}
						break;
					}
					case PassType::ERROR:
					{
						LOG_CRASHING_TAPI("Finding a proper TPType has failed, so Check Wait has failed!");
						return false;
					}
					default:
						LOG_NOTCODED_TAPI("Finding a Waited TP Type has failed, so Check Wait Handle has failed!", true);
						return false;
				}
			}
		}
		for (unsigned int WPIndex = 0; WPIndex < WindowPasses.size(); WPIndex++) {
			VK_WindowPass* WP = GFXHandleConverter(VK_WindowPass*, WindowPasses[WPIndex]);
			for (unsigned int WaitIndex = 0; WaitIndex < WP->WAITs.size(); WaitIndex++) {
				GFX_API::PassWait_Description& Wait_desc = WP->WAITs[WaitIndex];
				if (!(*Wait_desc.WaitedPass)) {
					LOG_ERROR_TAPI("You forgot to set wait handle of one of the window passes!");
					return false;
				}

				switch (FindWaitedPassType(Wait_desc.WaitedStage)) {
				case PassType::DP:
				{
					bool is_Found = false;
					for (unsigned int CheckedDP = 0; CheckedDP < DrawPasses.size(); CheckedDP++) {
						if (*Wait_desc.WaitedPass == DrawPasses[CheckedDP]) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						LOG_ERROR_TAPI("One of the window passes waits for an draw pass but given pass isn't found!");
						return false;
					}
				}
				break;
				case PassType::TP:
				{
					VK_TransferPass* currentTP = GFXHandleConverter(VK_TransferPass*, *Wait_desc.WaitedPass);
					bool is_Found = false;
					for (unsigned int CheckedTP = 0; CheckedTP < TransferPasses.size(); CheckedTP++) {
						if (currentTP == TransferPasses[CheckedTP]) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						LOG_ERROR_TAPI("One of the window passes waits for an transfer pass but given pass isn't found!");
						return false;
					}
				}
				break;
				case PassType::WP:
					LOG_CRASHING_TAPI("A window pass can't wait for another window pass, Check Wait Handle failed!");
					return false;
				case PassType::ERROR:
					LOG_CRASHING_TAPI("Finding a TPType has failed, so Check Wait Handle too!");
					return false;
				default:
					LOG_NOTCODED_TAPI("TP Type is not supported for now, so Check Wait Handle failed too!", true);
					return false;
				}
			}
		}
		for (unsigned int TPIndex = 0; TPIndex < TransferPasses.size(); TPIndex++) {
			VK_TransferPass* TP = TransferPasses[TPIndex];
			for (unsigned int WaitIndex = 0; WaitIndex < TP->WAITs.size(); WaitIndex++) {
				GFX_API::PassWait_Description& Wait_desc = TP->WAITs[WaitIndex];
				if (!(*Wait_desc.WaitedPass)) {
					LOG_ERROR_TAPI("You forgot to set wait handle of one of the transfer passes!");
					return false;
				}

				switch (FindWaitedPassType(Wait_desc.WaitedStage)) {
					case PassType::DP:
					{
						bool is_Found = false;
						for (unsigned int CheckedDP = 0; CheckedDP < DrawPasses.size(); CheckedDP++) {
							if (*Wait_desc.WaitedPass == DrawPasses[CheckedDP]) {
								is_Found = true;
								break;
							}
						}
						if (!is_Found) {
							std::cout << TP->NAME << std::endl;
							LOG_ERROR_TAPI("One of the transfer passes waits for an draw pass but given pass isn't found!");
							return false;
						}
					}
					break;
					case PassType::TP:
					{
						VK_TransferPass* currentTP = GFXHandleConverter(VK_TransferPass*, *Wait_desc.WaitedPass);
						bool is_Found = false;
						for (unsigned int CheckedTP = 0; CheckedTP < TransferPasses.size(); CheckedTP++) {
							if (currentTP == TransferPasses[CheckedTP]) {
								is_Found = true;
								break;
							}
						}
						if (!is_Found) {
							LOG_ERROR_TAPI("One of the transfer passes waits for an transfer pass but given pass isn't found!");
							return false;
						}
						break;
					}
					case PassType::WP: 
					{
						bool is_Found = false;
						for (unsigned int CheckedWP = 0; CheckedWP < WindowPasses.size(); CheckedWP++) {
							if (*Wait_desc.WaitedPass == WindowPasses[CheckedWP]) {
								is_Found = true;
								break;
							}
						}
						if (!is_Found) {
							LOG_ERROR_TAPI("One of the transfer passes waits for an window pass but given pass isn't found!");
							return false;
						}
						break;
					}
					case PassType::ERROR:
					{
						LOG_CRASHING_TAPI("Finding a TPType has failed, so Check Wait Handle too!");
						return false;
					}
					default:
					{
						LOG_NOTCODED_TAPI("TP Type is not supported for now, so Check Wait Handle failed too!", true);
						return false;
					}
				}

			}
		}
		return true;
	}

	//Each framegraph is constructed as same, so there is no difference about passing different framegraphs here
	void Create_VkDataofRGBranches(const VK_FrameGraph& FrameGraph, vector<VK_Semaphore>& Semaphores);
	void Renderer::Finish_RenderGraphConstruction() {
		if (!Record_RenderGraphConstruction) {
			LOG_CRASHING_TAPI("VulkanRenderer->Finish_RenderGraphCreation() has failed because you either didn't start it or finished before!");
			return;
		}
		Record_RenderGraphConstruction = false;

		{
			TURAN_PROFILE_SCOPE_MCS("Checking Wait Handles of RenderNodes");
			if (!Check_WaitHandles()) {
				LOG_CRASHING_TAPI("VulkanRenderer->Finish_RenderGraphCreation() has failed because some wait handles have issues!");
				return;
			}
		}

		Create_FrameGraphs(DrawPasses, TransferPasses, WindowPasses, FrameGraphs);


		//Create Command Pool per QUEUE
		for (unsigned int QUEUEIndex = 0; QUEUEIndex < VKGPU->QUEUEs.size(); QUEUEIndex++) {
			if (VKGPU->QUEUEs[QUEUEIndex].SupportFlag.is_PRESENTATIONsupported &&
				!
				(VKGPU->QUEUEs[QUEUEIndex].SupportFlag.is_COMPUTEsupported || VKGPU->QUEUEs[QUEUEIndex].SupportFlag.is_GRAPHICSsupported || VKGPU->QUEUEs[QUEUEIndex].SupportFlag.is_TRANSFERsupported)
				) {
				LOG_STATUS_TAPI("VulkanCore: One of the VkQueues only supports Presentation QUEUE, so GFX API didn't create a command pool for it!");
				continue;
			}
			for (unsigned char i = 0; i < 2; i++) {
				VkCommandPoolCreateInfo cp_ci_g = {};
				cp_ci_g.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				cp_ci_g.queueFamilyIndex = VKGPU->QUEUEs[QUEUEIndex].QueueFamilyIndex;
				cp_ci_g.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				cp_ci_g.pNext = nullptr;

				if (vkCreateCommandPool(VKGPU->Logical_Device, &cp_ci_g, nullptr, &VKGPU->QUEUEs[QUEUEIndex].CommandPools[i].CPHandle) != VK_SUCCESS) {
					LOG_CRASHING_TAPI("VulkanCore: Logical Device Setup has failed at vkCreateCommandPool()!");
					return;
				}
			}
		}


		//Create Command Buffer for each RGBranch for maximum workload cases
		Create_VkDataofRGBranches(FrameGraphs[0], Semaphores);
		
		//RenderGraph Synch Fence Creation
		for (unsigned char QueueIndex = 0; QueueIndex < VKGPU->QUEUEs.size(); QueueIndex++) {
			for (unsigned char i = 0; i < 2; i++) {
				VkFenceCreateInfo Fence_ci = {};
				Fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				Fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				Fence_ci.pNext = nullptr;
				if (vkCreateFence(VKGPU->Logical_Device, &Fence_ci, nullptr, &VKGPU->QUEUEs[QueueIndex].RenderGraphFences[i].Fence_o) != VK_SUCCESS) {
					LOG_CRASHING_TAPI("VulkanRenderer: Fence creation has failed!");
				}
				if (vkResetFences(VKGPU->Logical_Device, 1, &VKGPU->QUEUEs[QueueIndex].RenderGraphFences[i].Fence_o) != VK_SUCCESS) {
					LOG_CRASHING_TAPI("VulkanRenderer: Fence reset has failed!");
				}
			}
		}


		/*
		//Swapchain Layout Operations
		//Note: One CB is created because this will be used only at the start of the frame
		{
			VkImageMemoryBarrier* SWPCHN_IMLAYBARRIER = new VkImageMemoryBarrier[VKWINDOW->Swapchain_Textures.size()];
			for (unsigned int swpchn_index = 0; swpchn_index < VKWINDOW->Swapchain_Textures.size(); swpchn_index++) {
				VK_Texture* SWPCHN_im = GFXHandleConverter(VK_Texture*, VKWINDOW->Swapchain_Textures[swpchn_index]);
				SWPCHN_IMLAYBARRIER[swpchn_index].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				SWPCHN_IMLAYBARRIER[swpchn_index].image = SWPCHN_im->Image;
				SWPCHN_IMLAYBARRIER[swpchn_index].pNext = nullptr;
				SWPCHN_IMLAYBARRIER[swpchn_index].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				SWPCHN_IMLAYBARRIER[swpchn_index].subresourceRange.baseArrayLayer = 0;
				SWPCHN_IMLAYBARRIER[swpchn_index].subresourceRange.baseMipLevel = 0;
				SWPCHN_IMLAYBARRIER[swpchn_index].subresourceRange.layerCount = 1;
				SWPCHN_IMLAYBARRIER[swpchn_index].subresourceRange.levelCount = 1;

				//Vulkan Core already created the swapchain for the Display Queue, so we don't need to specify its queue!
				SWPCHN_IMLAYBARRIER[swpchn_index].srcAccessMask = 0;
				SWPCHN_IMLAYBARRIER[swpchn_index].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				SWPCHN_IMLAYBARRIER[swpchn_index].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				//Vulkan Core already created the swapchain for the Display Queue, so we don't need to change its queue!
				SWPCHN_IMLAYBARRIER[swpchn_index].dstAccessMask = 0;
				SWPCHN_IMLAYBARRIER[swpchn_index].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				SWPCHN_IMLAYBARRIER[swpchn_index].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			}

			vkCmdPipelineBarrier(*Swapchain_CreationCB->CBs[0], VK_PIPELINE_STAGE, VK_PIPELINE_STAGE_TRANSFER_BIT, 0
				, 0, nullptr, 0, nullptr, VKWINDOW->Swapchain_Textures.size(), SWPCHN_IMLAYBARRIER);

			vkEndCommandBuffer(*Swapchain_CreationCB->CBs[0]);


			vkDeviceWaitIdle(VKGPU->Logical_Device);
			VkSubmitInfo QueueSubmitInfo = {};
			QueueSubmitInfo.commandBufferCount = 1;
			QueueSubmitInfo.pCommandBuffers = Swapchain_CreationCB->CBs[0];
			QueueSubmitInfo.pNext = nullptr;
			VkPipelineStageFlags WaitStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			QueueSubmitInfo.pWaitDstStageMask = &WaitStageMask;
			QueueSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			QueueSubmitInfo.signalSemaphoreCount = 0;
			QueueSubmitInfo.waitSemaphoreCount = 0;
			if (vkQueueSubmit(VKGPU->QUEUEs[VKGPU->GRAPHICS_QUEUEIndex].Queue, 1, &QueueSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("VulkanRenderer: SWPCHN_IMLAYCB submit to Graphics Queue has failed!");
				return;
			}
			vkDeviceWaitIdle(VKGPU->Logical_Device);
			vkFreeCommandBuffers(VKGPU->Logical_Device, VKGPU->QUEUEs[VKGPU->GRAPHICS_QUEUEIndex].CommandPool, 1, Swapchain_CreationCB->CBs[0]);
			delete Swapchain_CreationCB;
		}
		*/
	}
	
	unsigned char Renderer::Get_FrameIndex(bool is_LastFrame) {
		if (is_LastFrame) {
			return (FrameIndex) ? 0 : 1;
		}
		else {
			return FrameIndex;
		}
	}


	void Fill_ColorVkAttachmentDescription(VkAttachmentDescription& Desc, const VK_COLORRTSLOT* Attachment) {
		Desc = {};
		Desc.format = Find_VkFormat_byTEXTURECHANNELs(Attachment->RT->CHANNELs);
		Desc.samples = VK_SAMPLE_COUNT_1_BIT;
		Desc.flags = 0;
		Desc.loadOp = Find_LoadOp_byGFXLoadOp(Attachment->LOADSTATE);
		Desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		if (Attachment->IS_USED_LATER) {
			Desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
		else {
			Desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}

		Desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		Desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		if (Attachment->RT->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32) {
			if (Attachment->RT_OPERATIONTYPE == GFX_API::OPERATION_TYPE::READ_ONLY) {
				Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
			}
			else {
				Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			}
		}
		else if (Attachment->RT->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8) {
			if (Attachment->RT_OPERATIONTYPE == GFX_API::OPERATION_TYPE::READ_ONLY) {
				Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
			}
			else {
				Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
		}
		else {
		}
		Desc.flags = 0;
	}
	void Fill_SubpassStructs(VK_IRTSLOTSET* slotset, VkSubpassDescription& descs, VkSubpassDependency& dependencies) {
		VkAttachmentReference* COLOR_ATTACHMENTs = new VkAttachmentReference[slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT];
		VkAttachmentReference* DS_Attach = nullptr;
		
		for (unsigned int i = 0; i < slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT; i++) {
			GFX_API::OPERATION_TYPE RT_OPTYPE = slotset->COLOR_OPTYPEs[i];
			VkAttachmentReference Attachment_Reference = {};
			if (RT_OPTYPE == GFX_API::OPERATION_TYPE::UNUSED) {
				Attachment_Reference.attachment = VK_ATTACHMENT_UNUSED;
				Attachment_Reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
			else {
				Attachment_Reference.attachment = i;
				Attachment_Reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
			COLOR_ATTACHMENTs[i] = Attachment_Reference;
		}

		//Depth Stencil Attachment
		if(slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
			VK_DEPTHSTENCILSLOT* depthslot = slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT;
			DS_Attach = new VkAttachmentReference;
			VKGPU->Fill_DepthAttachmentReference(*DS_Attach, slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT, 
				depthslot->RT->CHANNELs, slotset->DEPTH_OPTYPE, slotset->STENCIL_OPTYPE);
		}

		descs = {};
		descs.colorAttachmentCount = slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT;
		descs.pColorAttachments = COLOR_ATTACHMENTs;
		descs.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		descs.flags = 0;
		descs.pDepthStencilAttachment = DS_Attach;

		dependencies = {};
		dependencies.dependencyFlags = 0;
		dependencies.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies.dstSubpass = 0;
		dependencies.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies.srcAccessMask = 0;
		dependencies.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	TAPIResult Renderer::Create_DrawPass(const vector<GFX_API::SubDrawPass_Description>& SubDrawPasses, GFX_API::GFXHandle RTSLOTSET_ID, 
		const vector<GFX_API::PassWait_Description>& WAITs, const char* NAME, vector<GFX_API::GFXHandle>& SubDrawPassIDs, GFX_API::GFXHandle& DPHandle) {
		if (!Record_RenderGraphConstruction) {
			LOG_CRASHING_TAPI("GFXRENDERER->CreateDrawPass() has failed because you first need to call Start_RenderGraphCreation()!");
			return TAPI_FAIL;
		}
		VK_RTSLOTSET* SLOTSET = GFXHandleConverter(VK_RTSLOTSET*, RTSLOTSET_ID);
		for (unsigned int i = 0; i < SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT; i++) {
			VK_COLORRTSLOT& slot = SLOTSET->PERFRAME_SLOTSETs[0].COLOR_SLOTs[i];
			if (slot.RT_OPERATIONTYPE == GFX_API::OPERATION_TYPE::UNUSED) {
				LOG_ERROR_TAPI("Create_DrawPass() has failed because you can't give a unused Color RT Slot to the Draw Pass! You either don't give it, or use it");
				return TAPI_INVALIDARGUMENT;
			}
		}
		if (SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
			if (SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::UNUSED) {
				LOG_ERROR_TAPI("Create_DrawPass() has failed because you can't give a unused Depth RT Slot to the Draw Pass! You either don't give it, or use it");
				return TAPI_INVALIDARGUMENT;
			}
		}
		if (!SLOTSET) {
			LOG_ERROR_TAPI("Create_DrawPass() has failed because RTSLOTSET_ID is invalid!");
			return TAPI_INVALIDARGUMENT;
		}
		for (unsigned int i = 0; i < SubDrawPasses.size(); i++) {
			VK_IRTSLOTSET* ISET = GFXHandleConverter(VK_IRTSLOTSET*, SubDrawPasses[i].INHERITEDSLOTSET);
			if (!ISET) {
				LOG_ERROR_TAPI("GFXRenderer->Create_DrawPass() has failed because one of the inherited slotsets is nullptr!");
				return TAPI_INVALIDARGUMENT;
			}
			if (ISET->BASESLOTSET != RTSLOTSET_ID) {
				LOG_ERROR_TAPI("GFXRenderer->Create_DrawPass() has failed because one of the inherited slotsets isn't inherited from the DrawPass' Base Slotset!");
				return TAPI_INVALIDARGUMENT;
			}
		}



		VK_SubDrawPass* Final_Subpasses = new VK_SubDrawPass[SubDrawPasses.size()];
		VK_DrawPass* VKDrawPass = new VK_DrawPass;
		VKDrawPass->SLOTSET = SLOTSET;
		VKDrawPass->Subpass_Count = SubDrawPasses.size();
		VKDrawPass->Subpasses = Final_Subpasses;
		VKDrawPass->NAME = NAME;
		VKDrawPass->WAITs = WAITs;
		VKDrawPass->RenderRegion.WidthOffset = 0;
		VKDrawPass->RenderRegion.HeightOffset = 0;
		VKDrawPass->RenderRegion.Depth = 1;
		VKDrawPass->RenderRegion.DepthOffset = 0;
		if (SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT) {
			VKDrawPass->RenderRegion.Width = SLOTSET->PERFRAME_SLOTSETs[0].COLOR_SLOTs[0].RT->WIDTH;
			VKDrawPass->RenderRegion.Height = SLOTSET->PERFRAME_SLOTSETs[0].COLOR_SLOTs[0].RT->HEIGHT;
		}
		DrawPasses.push_back(VKDrawPass);



		//SubDrawPass creation
		vector<VkSubpassDependency> SubpassDepends(SubDrawPasses.size());
		vector<VkSubpassDescription> SubpassDescs(SubDrawPasses.size());
		vector<vector<VkAttachmentReference>> SubpassAttachments(SubDrawPasses.size());
		SubDrawPassIDs.clear();
		SubDrawPassIDs.resize(SubDrawPasses.size());
		for (unsigned int i = 0; i < SubDrawPasses.size(); i++) {
			const GFX_API::SubDrawPass_Description& Subpass_gfxdesc = SubDrawPasses[i];
			VK_IRTSLOTSET* Subpass_Slotset = GFXHandleConverter(VK_IRTSLOTSET*, Subpass_gfxdesc.INHERITEDSLOTSET);

			Fill_SubpassStructs(Subpass_Slotset, SubpassDescs[i], SubpassDepends[i]);

			Final_Subpasses[i].Binding_Index = Subpass_gfxdesc.SubDrawPass_Index;
			Final_Subpasses[i].SLOTSET = Subpass_Slotset;
			Final_Subpasses[i].DrawPass = VKDrawPass;
			SubDrawPassIDs[i] = &Final_Subpasses[i];
		}

		//vkRenderPass generation
		{
			vector<VkAttachmentDescription> AttachmentDescs;
			//Create Attachment Descriptions
			for (unsigned int i = 0; i < VKDrawPass->SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT; i++) {
				VkAttachmentDescription Attachmentdesc = {};
				Fill_ColorVkAttachmentDescription(Attachmentdesc, &VKDrawPass->SLOTSET->PERFRAME_SLOTSETs[0].COLOR_SLOTs[i]);
				AttachmentDescs.push_back(Attachmentdesc);
			}
			if (VKDrawPass->SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
				VkAttachmentDescription DepthDesc;
				VKGPU->Fill_DepthAttachmentDescription(DepthDesc, VKDrawPass->SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT);
				AttachmentDescs.push_back(DepthDesc);
			}
			

			//RenderPass Creation
			VkRenderPassCreateInfo RenderPass_CreationInfo = {};
			RenderPass_CreationInfo.flags = 0;
			RenderPass_CreationInfo.pNext = nullptr;
			RenderPass_CreationInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			RenderPass_CreationInfo.subpassCount = SubpassDescs.size();
			RenderPass_CreationInfo.pSubpasses = SubpassDescs.data();
			RenderPass_CreationInfo.attachmentCount = AttachmentDescs.size();
			RenderPass_CreationInfo.pAttachments = AttachmentDescs.data();
			RenderPass_CreationInfo.dependencyCount = SubpassDepends.size();
			RenderPass_CreationInfo.pDependencies = SubpassDepends.data();

			if (vkCreateRenderPass(((GPU*)VKGPU)->Logical_Device, &RenderPass_CreationInfo, nullptr, &VKDrawPass->RenderPassObject) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("VkCreateRenderPass() has failed!");
				delete VKDrawPass;
				return TAPI_FAIL;
			}
		}

		DPHandle = VKDrawPass;
		return TAPI_SUCCESS;
	}



	TAPIResult Renderer::Create_TransferPass(const vector<GFX_API::PassWait_Description>& WaitDescriptions,
		const GFX_API::TRANFERPASS_TYPE& TP_TYPE, const string& NAME, GFX_API::GFXHandle& TPHandle) {
		VK_TransferPass* TRANSFERPASS = new VK_TransferPass;

		TRANSFERPASS->WAITs = WaitDescriptions;
		TRANSFERPASS->NAME = NAME;
		TRANSFERPASS->TYPE = TP_TYPE;
		switch (TP_TYPE) {
		case GFX_API::TRANFERPASS_TYPE::TP_BARRIER:
			TRANSFERPASS->TransferDatas = new VK_TPBarrierDatas;
			break;
		case GFX_API::TRANFERPASS_TYPE::TP_COPY:
			TRANSFERPASS->TransferDatas = new VK_TPCopyDatas;
			break;
		default:
			LOG_NOTCODED_TAPI("VulkanRenderer: Create_TransferPass() has failed because this type of TP creation isn't supported!", true);
			return TAPI_NOTCODED;
		}

		TransferPasses.push_back(TRANSFERPASS);
		TPHandle = TRANSFERPASS;
		return TAPI_SUCCESS;
	}
	TAPIResult Renderer::Create_WindowPass(const vector<GFX_API::PassWait_Description>& WaitDescriptions, const string& NAME, GFX_API::GFXHandle& WindowPassHandle) {
		VK_WindowPass* WP = new VK_WindowPass;

		WP->NAME = NAME;
		WP->WAITs = WaitDescriptions;
		WP->WindowCalls[0].clear();
		WP->WindowCalls[1].clear();
		WP->WindowCalls[2].clear();

		WindowPassHandle = WP;
		WindowPasses.push_back(WP);
		return TAPI_SUCCESS;
	}


	void RecordRGBranchCalls(VK_RGBranch& Branch, VkCommandBuffer CB);
	void Renderer::Record_CurrentFramegraph() {
		//TURAN_PROFILE_SCOPE_MCS("Run_CurrentFramegraph()");
		VK_FrameGraph& Current_FrameGraph = FrameGraphs[Get_FrameIndex(false)];
		VK_FrameGraph& Last_FrameGraph = FrameGraphs[Get_FrameIndex(true)];
		for (unsigned char CFSubmitIndex = 0; CFSubmitIndex < Current_FrameGraph.CurrentFrameSubmits.size(); CFSubmitIndex++) {
			delete Current_FrameGraph.CurrentFrameSubmits[CFSubmitIndex];
		}
		Current_FrameGraph.CurrentFrameSubmits.clear();
		//CPU Vulkan Workload Analysis
		//When multi-threading comes, this part will be the first one that's multi-threaded
		//But of course, more precise workload analysis (both on GPU and CPU side) will be added
		//But GPU side workload analysis needs custom shading language (Without it, only transfers maybe analyzed)
		for (unsigned char BranchIndex = 0; BranchIndex < Current_FrameGraph.BranchCount; BranchIndex++) {
			VK_RGBranch& Branch = Current_FrameGraph.FrameGraphTree[BranchIndex];
			//If branch is used previous frame, clear frame dependent dynamic datas
			if (Branch.CurrentFramePassesIndexes[0]) {
				unsigned char CheckIndex = 0;
				while (Branch.CurrentFramePassesIndexes[CheckIndex]) {
					Branch.CurrentFramePassesIndexes[CheckIndex] = 0;
					CheckIndex++;
				}
				Branch.AttachedSubmit = nullptr;
				Branch.CFDynamicDependents.clear();
				Branch.LFDynamicDependents.clear();
				Branch.DynamicLaterExecutes.clear();
				VK_QUEUEFLAG empty;
				Branch.CFNeeded_QueueSpecs = empty;
			}

			//Find active passes
			unsigned char CurrentFrameIndexesArray_Element = 0;
			for (unsigned char PassIndex = 0; PassIndex < Branch.PassCount; PassIndex++) {
				VK_BranchPass* CorePass = &Branch.CorePasses[PassIndex];
				switch (CorePass->TYPE) {
				case PassType::TP:
				{
					VK_TransferPass* TP = GFXHandleConverter(VK_TransferPass*, CorePass->Handle);
					if (!TP->TransferDatas) {
						continue;
					}
					switch (TP->TYPE) {
					case GFX_API::TRANFERPASS_TYPE::TP_BARRIER:
					{
						bool isThereAny = false;
						VK_TPBarrierDatas* TPB = GFXHandleConverter(VK_TPBarrierDatas*, TP->TransferDatas);
						//Check Buffer Barriers
						{
							std::unique_lock<std::mutex> BufferLocker;
							TPB->BufferBarriers.PauseAllOperations(BufferLocker);
							for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
								if (TPB->BufferBarriers.size(ThreadID)) {
									isThereAny = true;
									break;
								}
							}
						}
						//Check Texture Barriers
						if (!isThereAny) {
							std::unique_lock<std::mutex> TextureLocker;
							TPB->TextureBarriers.PauseAllOperations(TextureLocker);
							for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
								if (TPB->TextureBarriers.size(ThreadID)) {
									isThereAny = true;
									break;
								}
							}
						}
						if (isThereAny) {
							Branch.CurrentFramePassesIndexes[CurrentFrameIndexesArray_Element] = PassIndex + 1;
							CurrentFrameIndexesArray_Element++;
						}
					}
					break;
					case GFX_API::TRANFERPASS_TYPE::TP_COPY:
					{
						bool isThereAny = false;
						VK_TPCopyDatas* DATAs = GFXHandleConverter(VK_TPCopyDatas*, TP->TransferDatas);
						{
							std::unique_lock<std::mutex> BUFBUFLOCKER;
							DATAs->BUFBUFCopies.PauseAllOperations(BUFBUFLOCKER);
							for (unsigned char ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
								if (DATAs->BUFBUFCopies.size(ThreadIndex)) {
									isThereAny = true;
									break;
								}
							}
						}
						if(!isThereAny){
							std::unique_lock<std::mutex> BUFIMLOCKER;
							DATAs->BUFIMCopies.PauseAllOperations(BUFIMLOCKER);
							for (unsigned char ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
								if (DATAs->BUFIMCopies.size(ThreadIndex)) {
									isThereAny = true;
									break;
								}
							}
						}
						if (!isThereAny) {
							std::unique_lock<std::mutex> IMIMLOCKER;
							DATAs->IMIMCopies.PauseAllOperations(IMIMLOCKER);
							for (unsigned char ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
								if (DATAs->IMIMCopies.size(ThreadIndex)) {
									isThereAny = true;
									break;
								}
							}
						}

						if (isThereAny) {
							Branch.CurrentFramePassesIndexes[CurrentFrameIndexesArray_Element] = PassIndex + 1;
							CurrentFrameIndexesArray_Element++;
							Branch.CFNeeded_QueueSpecs.is_TRANSFERsupported = true;
						}
					}
					break;
					default:
						LOG_NOTCODED_TAPI("VulkanRenderer: CurrentFrame_WorkloadAnalysis() doesn't support this type of transfer pass type!", true);
						break;
					}
				}
				break;
				case PassType::DP:
				{
					VK_DrawPass* DP = GFXHandleConverter(VK_DrawPass*, CorePass->Handle);
					for (unsigned char SubpassIndex = 0; SubpassIndex < DP->Subpass_Count; SubpassIndex++) {
						const VK_SubDrawPass& SP = DP->Subpasses[SubpassIndex];
						if (//SP.DrawCalls.size()
							true) {
							Branch.CurrentFramePassesIndexes[CurrentFrameIndexesArray_Element] = PassIndex + 1;
							CurrentFrameIndexesArray_Element++;
							Branch.CFNeeded_QueueSpecs.is_GRAPHICSsupported = true;
						}
					}
				}
				break;
				case PassType::WP:
				{
					VK_WindowPass* WP = GFXHandleConverter(VK_WindowPass*, CorePass->Handle);
					if (//WP->WindowCalls.size()
						true) {
						Branch.CurrentFramePassesIndexes[CurrentFrameIndexesArray_Element] = PassIndex + 1;
						CurrentFrameIndexesArray_Element++;
						Branch.CFNeeded_QueueSpecs.is_PRESENTATIONsupported = true;
					}

				}
				break;
				default:
					LOG_NOTCODED_TAPI("Specified Pass Type isn't coded yet to pass as a Branch!", true);
				}
			}
		}
		
		Create_VkSubmits(&FrameGraphs[Get_FrameIndex(false)], &FrameGraphs[Get_FrameIndex(true)], Semaphores);

		for (unsigned char SubmitIndex = 0; SubmitIndex < Current_FrameGraph.CurrentFrameSubmits.size(); SubmitIndex++) {
			VK_Submit* Submit = Current_FrameGraph.CurrentFrameSubmits[SubmitIndex];
			if (Current_FrameGraph.FrameGraphTree[Submit->BranchIndexes[0] - 1].CorePasses[0].TYPE == PassType::WP) {
				continue;
			}
			
			VkCommandBufferBeginInfo cb_bi = {};
			cb_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			cb_bi.pInheritanceInfo = nullptr;
			cb_bi.pNext = nullptr;
			cb_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			vkBeginCommandBuffer(Submit->Run_Queue->CommandPools[FrameIndex].CBs[Submit->CBIndex].CB, &cb_bi);
			
			for (unsigned char BranchIndex = 0; BranchIndex < Submit->BranchIndexes.size(); BranchIndex++) {
				VK_RGBranch& Branch = Current_FrameGraph.FrameGraphTree[Submit->BranchIndexes[BranchIndex] - 1];
				RecordRGBranchCalls(Branch, Submit->Run_Queue->CommandPools[FrameIndex].CBs[Submit->CBIndex].CB);
			}
			vkEndCommandBuffer(Submit->Run_Queue->CommandPools[FrameIndex].CBs[Submit->CBIndex].CB);
		}
	}
	void CopyDescriptorSets(VK_DescSet& Set, vector<VkCopyDescriptorSet>& CopyVector, vector<VkDescriptorSet*>& CopyTargetSets) {
		unsigned int DescCount = Set.DescImagesCount + Set.DescSamplersCount + Set.DescSBuffersCount + Set.DescUBuffersCount;
		for (unsigned int DescIndex = 0; DescIndex < DescCount; DescIndex++) {
			VkCopyDescriptorSet copyinfo;
			VK_Descriptor& SourceDesc = GFXHandleConverter(VK_Descriptor*, Set.Descs)[DescIndex];
			copyinfo.pNext = nullptr;
			copyinfo.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
			copyinfo.descriptorCount = SourceDesc.ElementCount;
			//We will copy all descriptor array's elements
			copyinfo.srcArrayElement = 0;
			copyinfo.dstArrayElement = 0;
			copyinfo.dstBinding = DescIndex;
			copyinfo.srcBinding = DescIndex;
			copyinfo.srcSet = Set.Set;
			//We will fill this DistanceSet after creating it!
			copyinfo.dstSet = VK_NULL_HANDLE;
			CopyVector.push_back(copyinfo);
			CopyTargetSets.push_back(&Set.Set);
		}
	}
	void Renderer::Run() {
		//Maybe the rendering of the last frame took less than V-Sync, this is to handle this case
		//Because that means the swapchain texture we will render to in above command buffers is being displayed
		for (unsigned char WindowIndex = 0; WindowIndex < ((Vulkan_Core*)GFX)->Get_WindowHandles().size(); WindowIndex++) {
			WINDOW* VKWINDOW = GFXHandleConverter(WINDOW*, ((Vulkan_Core*)GFX)->Get_WindowHandles()[WindowIndex]);

			uint32_t SwapchainImage_Index;
			vkAcquireNextImageKHR(VKGPU->Logical_Device, VKWINDOW->Window_SwapChain, UINT64_MAX,
				Semaphores[VKWINDOW->PresentationWaitSemaphoreIndexes[2]].SPHandle, VK_NULL_HANDLE, &SwapchainImage_Index);
			if (SwapchainImage_Index != FrameIndex) {
				std::cout << "Current SwapchainImage_Index: " << SwapchainImage_Index << std::endl;
				std::cout << "Current FrameCount: " << unsigned int(FrameIndex) << std::endl;
				LOG_CRASHING_TAPI("Renderer's FrameCount and Vulkan's SwapchainIndex don't match, there is something missing!");
			}
		}

		//Handle Descriptor Sets

		//Clear 2 frames before's unbound descriptor set list
		if (VKContentManager->UnboundDescSetList[FrameIndex].size()) {
			VK_DescPool& DP = VKContentManager->MaterialRelated_DescPool;
			vkFreeDescriptorSets(VKGPU->Logical_Device, DP.pool, VKContentManager->UnboundDescSetList[FrameIndex].size(),
				VKContentManager->UnboundDescSetList[FrameIndex].data());
			DP.REMAINING_SET.DirectAdd(VKContentManager->UnboundDescSetList[FrameIndex].size());

			DP.REMAINING_IMAGE.DirectAdd(VKContentManager->UnboundDescSetImageCount[FrameIndex]);
			VKContentManager->UnboundDescSetImageCount[FrameIndex] = 0;
			DP.REMAINING_SAMPLER.DirectAdd(VKContentManager->UnboundDescSetSamplerCount[FrameIndex]);
			VKContentManager->UnboundDescSetSamplerCount[FrameIndex] = 0;
			DP.REMAINING_SBUFFER.DirectAdd(VKContentManager->UnboundDescSetSBufferCount[FrameIndex]);
			VKContentManager->UnboundDescSetSBufferCount[FrameIndex] = 0;
			DP.REMAINING_UBUFFER.DirectAdd(VKContentManager->UnboundDescSetUBufferCount[FrameIndex]);
			VKContentManager->UnboundDescSetUBufferCount[FrameIndex] = 0;
			VKContentManager->UnboundDescSetList[FrameIndex].clear();
		}
		//Create Descriptor Sets for material types/instances that are created this frame
		{
			vector<VkDescriptorSet> Sets;
			vector<VkDescriptorSet*> SetPTRs;
			vector<VkDescriptorSetLayout> SetLayouts;
			std::unique_lock<std::mutex> Locker;
			VKContentManager->DescSets_toCreate.PauseAllOperations(Locker);
			for (unsigned int ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
				for (unsigned int SetIndex = 0; SetIndex < VKContentManager->DescSets_toCreate.size(ThreadIndex); SetIndex++) {
					VK_DescSet* Set = VKContentManager->DescSets_toCreate.get(ThreadIndex, SetIndex);
					Sets.push_back(VkDescriptorSet());
					SetLayouts.push_back(Set->Layout);
					SetPTRs.push_back(&Set->Set);
					VKContentManager->UnboundDescSetImageCount[FrameIndex] += Set->DescImagesCount;
					VKContentManager->UnboundDescSetSamplerCount[FrameIndex] += Set->DescSamplersCount;
					VKContentManager->UnboundDescSetSBufferCount[FrameIndex] += Set->DescSBuffersCount;
					VKContentManager->UnboundDescSetUBufferCount[FrameIndex] += Set->DescUBuffersCount;
				}
				VKContentManager->DescSets_toCreate.clear(ThreadIndex);
			}
			Locker.unlock();

			if (Sets.size()) {
				VkDescriptorSetAllocateInfo al_in = {};
				al_in.descriptorPool = VKContentManager->MaterialRelated_DescPool.pool;
				al_in.descriptorSetCount = Sets.size();
				al_in.pNext = nullptr;
				al_in.pSetLayouts = SetLayouts.data();
				al_in.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				vkAllocateDescriptorSets(VKGPU->Logical_Device, &al_in, Sets.data());
				for (unsigned int SetIndex = 0; SetIndex < Sets.size(); SetIndex++) {
					*SetPTRs[SetIndex] = Sets[SetIndex];
				}
			}
		}
		//Create Descriptor Sets for material types/instances that are created before and used recently (last 2 frames), to update their descriptor sets
		{
			vector<VkDescriptorSet> NewSets;
			vector<VkDescriptorSet*> SetPTRs;
			vector<VkDescriptorSetLayout> SetLayouts;

			//Copy descriptor sets exactly, then update with this frame's SetMaterial_xxx calls
			vector<VkCopyDescriptorSet> CopySetInfos;
			vector<VkDescriptorSet*> CopyTargetSets;


			std::unique_lock<std::mutex> Locker;
			VKContentManager->DescSets_toCreateUpdate.PauseAllOperations(Locker);
			for (unsigned int ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
				for (unsigned int SetIndex = 0; SetIndex < VKContentManager->DescSets_toCreateUpdate.size(ThreadIndex); SetIndex++) {
					VK_DescSetUpdateCall& Call = VKContentManager->DescSets_toCreateUpdate.get(ThreadIndex, SetIndex);
					VK_DescSet* Set = Call.Set;
					bool SetStatus = Set->ShouldRecreate.load();
					switch (SetStatus) {
					case 0:
						continue;
					case 1:
						NewSets.push_back(VkDescriptorSet());
						SetPTRs.push_back(&Set->Set);
						SetLayouts.push_back(Set->Layout);
						VKContentManager->UnboundDescSetImageCount[FrameIndex] += Set->DescImagesCount;
						VKContentManager->UnboundDescSetSamplerCount[FrameIndex] += Set->DescSamplersCount;
						VKContentManager->UnboundDescSetSBufferCount[FrameIndex] += Set->DescSBuffersCount;
						VKContentManager->UnboundDescSetUBufferCount[FrameIndex] += Set->DescUBuffersCount;
						VKContentManager->UnboundDescSetList[FrameIndex].push_back(Set->Set);

						CopyDescriptorSets(*Set, CopySetInfos, CopyTargetSets);
						Set->ShouldRecreate.store(0);
						break;
					default:
						LOG_NOTCODED_TAPI("Descriptor Set atomic_uchar isn't supposed to have a value that's 2+! Please check 'Handle Descriptor Sets' in Vulkan Renderer->Run()", true);
						break;
					}
				}
			}

			if (NewSets.size()) {
				VkDescriptorSetAllocateInfo al_in = {};
				al_in.descriptorPool = VKContentManager->MaterialRelated_DescPool.pool;
				al_in.descriptorSetCount = NewSets.size();
				al_in.pNext = nullptr;
				al_in.pSetLayouts = SetLayouts.data();
				al_in.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				vkAllocateDescriptorSets(VKGPU->Logical_Device, &al_in, NewSets.data());
				for (unsigned int SetIndex = 0; SetIndex < NewSets.size(); SetIndex++) {
					*SetPTRs[SetIndex] = NewSets[SetIndex];
				}

				for (unsigned int CopyIndex = 0; CopyIndex < CopySetInfos.size(); CopyIndex++) {
					CopySetInfos[CopyIndex].dstSet = *CopyTargetSets[CopyIndex];
				}
				vkUpdateDescriptorSets(VKGPU->Logical_Device, 0, nullptr, CopySetInfos.size(), CopySetInfos.data());
			}
		}
		//Update descriptor sets
		{
			vector<VkWriteDescriptorSet> UpdateInfos;
			std::unique_lock<std::mutex> Locker1;
			VKContentManager->DescSets_toCreateUpdate.PauseAllOperations(Locker1);
			for (unsigned int ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
				for (unsigned int CallIndex = 0; CallIndex < VKContentManager->DescSets_toCreateUpdate.size(ThreadIndex); CallIndex++) {
					VK_DescSetUpdateCall& Call = VKContentManager->DescSets_toCreateUpdate.get(ThreadIndex, CallIndex);
					VkWriteDescriptorSet info = {};
					info.descriptorCount = 1;
					VK_Descriptor& Desc = Call.Set->Descs[Call.BindingIndex];
					switch (Desc.Type) {
					case DescType::IMAGE:
						info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pImageInfo = &GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].info;
						GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case DescType::SAMPLER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pImageInfo = &GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].info;
						GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case DescType::UBUFFER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pBufferInfo = &GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].Info;
						GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case DescType::SBUFFER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pBufferInfo = &GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].Info;
						GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					}
					info.dstSet = Call.Set->Set;
					info.pNext = nullptr;
					info.pTexelBufferView = nullptr;
					info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					UpdateInfos.push_back(info);
				}
				VKContentManager->DescSets_toCreateUpdate.clear(ThreadIndex);
			}
			Locker1.unlock();


			std::unique_lock<std::mutex> Locker2;
			VKContentManager->DescSets_toJustUpdate.PauseAllOperations(Locker2);
			for (unsigned int ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
				for (unsigned int CallIndex = 0; CallIndex < VKContentManager->DescSets_toJustUpdate.size(ThreadIndex); CallIndex++) {
					VK_DescSetUpdateCall& Call = VKContentManager->DescSets_toJustUpdate.get(ThreadIndex, CallIndex);
					VkWriteDescriptorSet info = {};
					info.descriptorCount = 1;
					VK_Descriptor& Desc = Call.Set->Descs[Call.BindingIndex];
					switch (Desc.Type) {
					case DescType::IMAGE:
						info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pImageInfo = &GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].info;
						GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case DescType::SAMPLER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pImageInfo = &GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].info;
						GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case DescType::UBUFFER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pBufferInfo = &GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].Info;
						GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case DescType::SBUFFER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pBufferInfo = &GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].Info;
						GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					}
					info.dstSet = Call.Set->Set;
					info.pNext = nullptr;
					info.pTexelBufferView = nullptr;
					info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					UpdateInfos.push_back(info);
				}
				VKContentManager->DescSets_toJustUpdate.clear(ThreadIndex);
			}
			Locker2.unlock();

			vkUpdateDescriptorSets(VKGPU->Logical_Device, UpdateInfos.size(), UpdateInfos.data(), 0, nullptr);
		}


		//Wait for command buffers to end
		for (unsigned char QueueIndex = 0; QueueIndex < VKGPU->QUEUEs.size(); QueueIndex++) {
			if (!VKGPU->QUEUEs[QueueIndex].RenderGraphFences[FrameIndex].is_Used) {
				continue;
			}
			TURAN_PROFILE_SCOPE_MCS("Waiting for fences!");
			if (vkWaitForFences(VKGPU->Logical_Device, 1, &VKGPU->QUEUEs[QueueIndex].RenderGraphFences[FrameIndex].Fence_o, true, UINT64_MAX) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("VulkanRenderer: Fence wait has failed!");
			}
			if (vkResetFences(VKGPU->Logical_Device, 1, &VKGPU->QUEUEs[QueueIndex].RenderGraphFences[FrameIndex].Fence_o) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("VulkanRenderer: Fence reset has failed!");
			}
		}
		//Reset semaphore infos
		for (unsigned char SubmitIndex = 0; SubmitIndex < FrameGraphs[FrameIndex].CurrentFrameSubmits.size(); SubmitIndex++) {
			VK_Submit* Submit = FrameGraphs[FrameIndex].CurrentFrameSubmits[SubmitIndex];

			Submit->WaitSemaphoreIndexes.clear();
			Submit->WaitSemaphoreStages.clear();
			if (FrameGraphs[FrameIndex].FrameGraphTree[Submit->BranchIndexes[0] - 1].CorePasses[0].TYPE != PassType::WP) {
				Submit->Run_Queue->CommandPools[FrameIndex].CBs[Submit->CBIndex].is_Used = false;
				Submit->CBIndex = 255;
				Semaphores[Submit->SignalSemaphoreIndex].isUsed = false;
				Submit->SignalSemaphoreIndex = 255;
			}
		}



		//Record command buffers
		Record_CurrentFramegraph();
		//Send rendercommand buffers
		for (unsigned char QueueIndex = 0; QueueIndex < VKGPU->QUEUEs.size(); QueueIndex++) {
			vector<VkSubmitInfo> FINALSUBMITs;
			vector<vector<VkSemaphore>> WaitSemaphoreLists;
			for (unsigned char SubmitIndex = 0; SubmitIndex < FrameGraphs[FrameIndex].CurrentFrameSubmits.size(); SubmitIndex++) {
				VK_Submit* Submit = FrameGraphs[FrameIndex].CurrentFrameSubmits[SubmitIndex];
				if (Submit->Run_Queue != &VKGPU->QUEUEs[QueueIndex] || Submit->CBIndex == 255) {
					continue;
				}

				VkSubmitInfo submitinfo = {};
				submitinfo.commandBufferCount = 1;
				submitinfo.pCommandBuffers = &VKGPU->QUEUEs[QueueIndex].CommandPools[FrameIndex].CBs[Submit->CBIndex].CB;
				submitinfo.pNext = nullptr;
				submitinfo.pSignalSemaphores = &Semaphores[Submit->SignalSemaphoreIndex].SPHandle;
				WaitSemaphoreLists.push_back(vector<VkSemaphore>());
				vector<VkSemaphore>* WaitSemaphoreList = &WaitSemaphoreLists[WaitSemaphoreLists.size() - 1];
				for (unsigned char WaitIndex = 0; WaitIndex < Submit->WaitSemaphoreIndexes.size(); WaitIndex++) {
					WaitSemaphoreList->push_back(Semaphores[Submit->WaitSemaphoreIndexes[WaitIndex]].SPHandle);
				}
				
				submitinfo.pWaitSemaphores = WaitSemaphoreList->data();
				submitinfo.pWaitDstStageMask = Submit->WaitSemaphoreStages.data();
				submitinfo.waitSemaphoreCount = Submit->WaitSemaphoreIndexes.size();
				submitinfo.signalSemaphoreCount = 1;
				submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				FINALSUBMITs.push_back(submitinfo);
			}
			if (!FINALSUBMITs.size()) {
				VKGPU->QUEUEs[QueueIndex].RenderGraphFences[FrameIndex].is_Used = false;
				continue;
			}
			TURAN_PROFILE_SCOPE_MCS("CB sendage!");
			if (vkQueueSubmit(VKGPU->QUEUEs[QueueIndex].Queue, FINALSUBMITs.size(), FINALSUBMITs.data(), VKGPU->QUEUEs[QueueIndex].RenderGraphFences[FrameIndex].Fence_o) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Vulkan Queue Submission has failed!");
				return;
			}
			VKGPU->QUEUEs[QueueIndex].RenderGraphFences[FrameIndex].is_Used = true;
		}

		//Send displays
		for (unsigned char WindowPassIndex = 0; WindowPassIndex < WindowPasses.size(); WindowPassIndex++) {
			VK_WindowPass* WP = WindowPasses[WindowPassIndex];
			if (!WP->WindowCalls[2].size()) {
				continue;
			}

			//Fill swapchain and image indices vectors
			uint32_t FrameIndex_uint32_t = FrameIndex;
			vector<VkSwapchainKHR> Swapchains;	Swapchains.resize(WP->WindowCalls[2].size());
			vector<uint32_t> ImageIndices;		ImageIndices.resize(WP->WindowCalls[2].size());
			for (unsigned char WindowIndex = 0; WindowIndex < WP->WindowCalls[2].size(); WindowIndex++) {
				ImageIndices[WindowIndex] = FrameIndex;	//All windows are using the same swapchain index, because otherwise it'd be complicated
				Swapchains[WindowIndex] = WP->WindowCalls[2][WindowIndex].Window->Window_SwapChain;
			}

			//Find the submit to get the wait semaphores
			vector<VkSemaphore> WaitSemaphores;
			for (unsigned char SubmitIndex = 0; SubmitIndex < FrameGraphs[FrameIndex].CurrentFrameSubmits.size(); SubmitIndex++) {
				VK_Submit* Submit = FrameGraphs[FrameIndex].CurrentFrameSubmits[SubmitIndex];

				//Window Pass only submits, contains one branch and the branch is Window Pass only too!
				if (Submit->BranchIndexes[0]) {
					VK_BranchPass& Pass = FrameGraphs[FrameIndex].FrameGraphTree[Submit->BranchIndexes[0] - 1].CorePasses[0];
					if (Pass.Handle == WP) {
						for (unsigned char WaitSemaphoreIndex = 0; WaitSemaphoreIndex < Submit->WaitSemaphoreIndexes.size(); WaitSemaphoreIndex++) {
							WaitSemaphores.push_back(Semaphores[Submit->WaitSemaphoreIndexes[WaitSemaphoreIndex]].SPHandle);
						}
					}
				}
			}

			VkPresentInfoKHR SwapchainImage_PresentationInfo = {};
			SwapchainImage_PresentationInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			SwapchainImage_PresentationInfo.pNext = nullptr;
			SwapchainImage_PresentationInfo.swapchainCount = Swapchains.size();
			SwapchainImage_PresentationInfo.pSwapchains = Swapchains.data();
			SwapchainImage_PresentationInfo.pImageIndices = ImageIndices.data();
			SwapchainImage_PresentationInfo.pResults = nullptr;

			SwapchainImage_PresentationInfo.waitSemaphoreCount = WaitSemaphores.size();
			SwapchainImage_PresentationInfo.pWaitSemaphores = WaitSemaphores.data();
			VkQueue DisplayQueue = {};
			for (unsigned char QueueIndex = 0; QueueIndex < VKGPU->QUEUEs.size(); QueueIndex++) {
				if (VKGPU->QUEUEs[QueueIndex].SupportFlag.is_PRESENTATIONsupported) {
					DisplayQueue = VKGPU->QUEUEs[QueueIndex].Queue;
				}
			}
			TURAN_PROFILE_SCOPE_MCS("Display sendage!");
			if (vkQueuePresentKHR(DisplayQueue, &SwapchainImage_PresentationInfo) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Submitting Presentation Queue has failed!");
				return;
			}
		}



		//Shift all WindowCall buffers of all Window Passes!
		//Also shift all of the window semaphores
		for (unsigned char WPIndex = 0; WPIndex < WindowPasses.size(); WPIndex++) {
			VK_WindowPass* WP = WindowPasses[WPIndex];
			WP->WindowCalls[0] = WP->WindowCalls[1];
			WP->WindowCalls[1] = WP->WindowCalls[2];
			WP->WindowCalls[2].clear();
		}
		for (unsigned char WindowIndex = 0; WindowIndex < ((Vulkan_Core*)GFX)->Get_WindowHandles().size(); WindowIndex++) {
			WINDOW* Window = GFXHandleConverter(WINDOW*, ((Vulkan_Core*)GFX)->Get_WindowHandles()[WindowIndex]);
			unsigned char penultimate_semaphore = Window->PresentationWaitSemaphoreIndexes[0];
			Window->PresentationWaitSemaphoreIndexes[0] = Window->PresentationWaitSemaphoreIndexes[1];
			Window->PresentationWaitSemaphoreIndexes[1] = Window->PresentationWaitSemaphoreIndexes[2];
			Window->PresentationWaitSemaphoreIndexes[2] = penultimate_semaphore;
		}

		//Current frame has finished, so every call after this call affects to the next frame
		Set_NextFrameIndex();
	}
	void FindBufferOBJ_byBufType(const GFX_API::GFXHandle Handle, GFX_API::BUFFER_TYPE TYPE, VkBuffer& TargetBuffer, VkDeviceSize& TargetOffset);
	void Renderer::DrawNonInstancedDirect(GFX_API::GFXHandle VertexBuffer_ID, GFX_API::GFXHandle IndexBuffer_ID, unsigned int Count, unsigned int VertexOffset,
		unsigned int FirstIndex, GFX_API::GFXHandle MaterialInstance_ID, GFX_API::GFXHandle SubDrawPass_ID) {
		TURAN_PROFILE_SCOPE_MCS("DrawNonInstanceDirect!");
		VK_SubDrawPass* SP = GFXHandleConverter(VK_SubDrawPass*, SubDrawPass_ID);
		if (IndexBuffer_ID) {
			VK_IndexedDrawCall call;
			if (VertexBuffer_ID) {
				FindBufferOBJ_byBufType(VertexBuffer_ID, GFX_API::BUFFER_TYPE::VERTEX, call.VBuffer, call.VBOffset);
			}
			else {
				call.VBuffer = VK_NULL_HANDLE;
			}
			FindBufferOBJ_byBufType(IndexBuffer_ID, GFX_API::BUFFER_TYPE::INDEX, call.IBuffer, call.IBOffset);
			call.IType = GFXHandleConverter(VK_IndexBuffer*, IndexBuffer_ID)->DATATYPE;
			if (Count) {
				call.IndexCount = Count;
			}
			else {
				call.IndexCount = GFXHandleConverter(VK_IndexBuffer*, IndexBuffer_ID)->IndexCount;
			}
			call.FirstIndex = FirstIndex;
			call.VOffset = VertexOffset;
			VK_PipelineInstance* PI = GFXHandleConverter(VK_PipelineInstance*, MaterialInstance_ID);
			call.MatTypeObj = PI->PROGRAM->PipelineObject;
			call.MatTypeLayout = PI->PROGRAM->PipelineLayout;
			call.GeneralSet = &PI->PROGRAM->General_DescSet.Set;
			call.PerInstanceSet = &PI->DescSet.Set;
			SP->IndexedDrawCalls.push_back(GFX->JobSys->GetThisThreadIndex(), call);
		}
		else {
			VK_NonIndexedDrawCall call;
			if (VertexBuffer_ID) {
				FindBufferOBJ_byBufType(VertexBuffer_ID, GFX_API::BUFFER_TYPE::VERTEX, call.VBuffer, call.VOffset);
			}
			else {
				call.VBuffer = VK_NULL_HANDLE;
			}
			if (Count) {
				call.VertexCount = Count;
			}
			else {
				call.VertexCount = static_cast<VkDeviceSize>(GFXHandleConverter(VK_VertexBuffer*, VertexBuffer_ID)->VERTEX_COUNT);
			}
			call.FirstVertex = VertexOffset;
			VK_PipelineInstance* PI = GFXHandleConverter(VK_PipelineInstance*, MaterialInstance_ID);
			call.MatTypeObj = PI->PROGRAM->PipelineObject;
			call.MatTypeLayout = PI->PROGRAM->PipelineLayout;
			call.GeneralSet = &PI->PROGRAM->General_DescSet.Set;
			call.PerInstanceSet = &PI->DescSet.Set;
			SP->NonIndexedDrawCalls.push_back(GFX->JobSys->GetThisThreadIndex(), call);
		}
	}
	void Renderer::SwapBuffers(GFX_API::GFXHandle WindowHandle, GFX_API::GFXHandle WindowPassHandle) {
		VK_WindowPass* WP = GFXHandleConverter(VK_WindowPass*, WindowPassHandle);
		WINDOW* Window = GFXHandleConverter(WINDOW*, WindowHandle);
		VK_WindowCall WC;
		WC.Window = Window;
		WP->WindowCalls[2].push_back(WC);
	}
	void FindBufferOBJ_byBufType(const GFX_API::GFXHandle Handle, GFX_API::BUFFER_TYPE TYPE, VkBuffer& TargetBuffer, VkDeviceSize& TargetOffset) {
		switch (TYPE) {
		case GFX_API::BUFFER_TYPE::GLOBAL:
			{
				VK_GlobalBuffer* buf = GFXHandleConverter(VK_GlobalBuffer*, Handle);
				TargetBuffer = VKGPU->ALLOCs[buf->Block.MemAllocIndex].Buffer;
				TargetOffset += buf->Block.Offset;
			}
			break;
		case GFX_API::BUFFER_TYPE::VERTEX:
			{
				VK_VertexBuffer* buf = GFXHandleConverter(VK_VertexBuffer*, Handle);
				TargetBuffer = VKGPU->ALLOCs[buf->Block.MemAllocIndex].Buffer;
				TargetOffset += buf->Block.Offset;
			}
			break;
		case GFX_API::BUFFER_TYPE::STAGING:
			{
				MemoryBlock* Staging = GFXHandleConverter(MemoryBlock*, Handle);
				TargetBuffer = VKGPU->ALLOCs[Staging->MemAllocIndex].Buffer;
				TargetOffset += Staging->Offset;
			}
			break;
		case GFX_API::BUFFER_TYPE::INDEX:
			{
				VK_IndexBuffer* IB = GFXHandleConverter(VK_IndexBuffer*, Handle);
				TargetBuffer = VKGPU->ALLOCs[IB->Block.MemAllocIndex].Buffer;
				TargetOffset += IB->Block.Offset;
			}
			break;
		default:
			LOG_NOTCODED_TAPI("FindBufferOBJ_byBufType() doesn't support this type of buffer!", true);
		}
	}
	void Renderer::CopyBuffer_toBuffer(GFX_API::GFXHandle TransferPassHandle, GFX_API::GFXHandle SourceBuffer_Handle, GFX_API::BUFFER_TYPE SourceBufferTYPE, 
		GFX_API::GFXHandle TargetBuffer_Handle, GFX_API::BUFFER_TYPE TargetBufferTYPE, unsigned int SourceBuffer_Offset, unsigned int TargetBuffer_Offset, unsigned int Size) {
		VkBuffer SourceBuffer, DistanceBuffer;
		VkDeviceSize SourceOffset = static_cast<VkDeviceSize>(SourceBuffer_Offset), DistanceOffset = static_cast<VkDeviceSize>(TargetBuffer_Offset);
		FindBufferOBJ_byBufType(SourceBuffer_Handle, SourceBufferTYPE, SourceBuffer, SourceOffset);
		FindBufferOBJ_byBufType(TargetBuffer_Handle, TargetBufferTYPE, DistanceBuffer, DistanceOffset);
		VkBufferCopy Copy_i = {};
		Copy_i.dstOffset = DistanceOffset;
		Copy_i.srcOffset = SourceOffset;
		Copy_i.size = static_cast<VkDeviceSize>(Size);
		VK_TPCopyDatas* DATAs = GFXHandleConverter(VK_TPCopyDatas*, GFXHandleConverter(VK_TransferPass*, TransferPassHandle)->TransferDatas);
		VK_BUFtoBUFinfo finalinfo;
		finalinfo.DistanceBuffer = DistanceBuffer;
		finalinfo.SourceBuffer = SourceBuffer;
		finalinfo.info = Copy_i;
		DATAs->BUFBUFCopies.push_back(GFX->JobSys->GetThisThreadIndex(), finalinfo);
	}
	void Renderer::CopyBuffer_toImage(GFX_API::GFXHandle TransferPassHandle, GFX_API::GFXHandle SourceBuffer_Handle, GFX_API::BUFFER_TYPE SourceBufferTYPE,
		GFX_API::GFXHandle TextureHandle, unsigned int SourceBuffer_offset, GFX_API::BoxRegion TargetTextureRegion) {
		VK_Texture* TEXTURE = GFXHandleConverter(VK_Texture*, TextureHandle);
		VkDeviceSize finaloffset = static_cast<VkDeviceSize>(SourceBuffer_offset);
		VK_BUFtoIMinfo x;
		x.BufferImageCopy.bufferImageHeight = 0;
		x.BufferImageCopy.bufferRowLength = 0;
		if (TargetTextureRegion.Depth) {
			x.BufferImageCopy.imageExtent.depth = TargetTextureRegion.Depth;
		}
		else {
			x.BufferImageCopy.imageExtent.depth = 1;
		}
		if (TargetTextureRegion.Height) {
			x.BufferImageCopy.imageExtent.height = TargetTextureRegion.Height;
		}
		else {
			x.BufferImageCopy.imageExtent.height = TEXTURE->HEIGHT;
		}
		if (TargetTextureRegion.Width) {
			x.BufferImageCopy.imageExtent.width = TargetTextureRegion.Width;
		}
		else {
			x.BufferImageCopy.imageExtent.width = TEXTURE->WIDTH;
		}
		x.BufferImageCopy.imageOffset.x = TargetTextureRegion.WidthOffset;
		x.BufferImageCopy.imageOffset.y = TargetTextureRegion.HeightOffset;
		x.BufferImageCopy.imageOffset.z = TargetTextureRegion.DepthOffset;
		if (TEXTURE->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8) {
			x.BufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else if (TEXTURE->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32) {
			x.BufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else {
			x.BufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		x.BufferImageCopy.imageSubresource.baseArrayLayer = 0;
		x.BufferImageCopy.imageSubresource.layerCount = 1;
		x.BufferImageCopy.imageSubresource.mipLevel = 0;
		x.TargetImage = TEXTURE->Image;
		FindBufferOBJ_byBufType(SourceBuffer_Handle, SourceBufferTYPE, x.SourceBuffer, finaloffset);
		x.BufferImageCopy.bufferOffset = finaloffset;
		VK_TransferPass* TP = GFXHandleConverter(VK_TransferPass*, TransferPassHandle);
		if (TP->TYPE == GFX_API::TRANFERPASS_TYPE::TP_BARRIER) {
			LOG_ERROR_TAPI("You gave an barrier TP handle to CopyBuffer_toImage(), this is wrong!");
			return;
		}
		VK_TPCopyDatas* DATAs = GFXHandleConverter(VK_TPCopyDatas*, TP->TransferDatas);
		DATAs->BUFIMCopies.push_back(GFX->JobSys->GetThisThreadIndex(), x);
	}
	void Renderer::CopyImage_toImage(GFX_API::GFXHandle TransferPassHandle, GFX_API::GFXHandle SourceTextureHandle, GFX_API::GFXHandle TargetTextureHandle,
		uvec3 SourceTextureOffset, uvec3 CopySize, uvec3 TargetTextureOffset) {
		VK_Texture* SourceIm = GFXHandleConverter(VK_Texture*, SourceTextureHandle);
		VK_Texture* TargetIm = GFXHandleConverter(VK_Texture*, TargetTextureHandle);
		VK_IMtoIMinfo x;

		VkImageCopy copy_i = {};
		copy_i.dstOffset.x = TargetTextureOffset.x;
		copy_i.dstOffset.y = TargetTextureOffset.y;
		copy_i.dstOffset.z = TargetTextureOffset.z;
		copy_i.srcOffset.x = SourceTextureOffset.x;
		copy_i.srcOffset.y = SourceTextureOffset.y;
		copy_i.srcOffset.z = SourceTextureOffset.z;
		if (SourceIm->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32) {
			copy_i.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			copy_i.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if (SourceIm->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8) {
			copy_i.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			copy_i.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else {
			copy_i.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy_i.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		copy_i.dstSubresource.baseArrayLayer = 0; copy_i.srcSubresource.baseArrayLayer = 0;
		copy_i.dstSubresource.layerCount = 1; copy_i.srcSubresource.layerCount = 1;
		copy_i.dstSubresource.mipLevel = 0; copy_i.srcSubresource.mipLevel = 0;
		
		copy_i.extent.width = CopySize.x;
		copy_i.extent.height = CopySize.y;
		copy_i.extent.depth = CopySize.z;

		x.info = copy_i;
		x.SourceTexture = SourceIm->Image;
		x.TargetTexture = TargetIm->Image;

		VK_TPCopyDatas* DATAs = GFXHandleConverter(VK_TPCopyDatas*, GFXHandleConverter(VK_TransferPass*, TransferPassHandle)->TransferDatas);
		DATAs->IMIMCopies.push_back(GFX->JobSys->GetThisThreadIndex(), x);
	}

	void Renderer::ImageBarrier(GFX_API::GFXHandle TextureHandle, const GFX_API::IMAGE_ACCESS& LAST_ACCESS
		, const GFX_API::IMAGE_ACCESS& NEXT_ACCESS, GFX_API::GFXHandle BarrierTPHandle) {
		VK_Texture* Texture = GFXHandleConverter(VK_Texture*, TextureHandle);
		VK_TransferPass* TP = GFXHandleConverter(VK_TransferPass*, BarrierTPHandle);
		if (TP->TYPE != GFX_API::TRANFERPASS_TYPE::TP_BARRIER) {
			LOG_ERROR_TAPI("You should give the handle of a Barrier Transfer Pass! Given Transfer Pass' type isn't Barrier.");
			return;
		}
		VK_TPBarrierDatas* TPDatas = GFXHandleConverter(VK_TPBarrierDatas*, TP->TransferDatas);

		
		VK_ImBarrierInfo im_bi;
		im_bi.Barrier.image = Texture->Image;
		Find_AccessPattern_byIMAGEACCESS(LAST_ACCESS, im_bi.Barrier.srcAccessMask, im_bi.Barrier.oldLayout);
		Find_AccessPattern_byIMAGEACCESS(NEXT_ACCESS, im_bi.Barrier.dstAccessMask, im_bi.Barrier.newLayout);
		if (Texture->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8 ) {
			im_bi.Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else if (Texture->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32) {
			im_bi.Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else {
			im_bi.Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		//Mipmap settings, but mipmap is not supported right now
		LOG_NOTCODED_TAPI("Mipmapping isn't coded, so subresourceRange mipmap settings aren't set either!", false);
		im_bi.Barrier.subresourceRange.baseArrayLayer = 0;
		im_bi.Barrier.subresourceRange.layerCount = 1;
		im_bi.Barrier.subresourceRange.levelCount = 1;
		im_bi.Barrier.subresourceRange.baseMipLevel = 0;
		im_bi.Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		im_bi.Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		im_bi.Barrier.pNext = nullptr;
		im_bi.Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;


		TPDatas->TextureBarriers.push_back(GFX->JobSys->GetThisThreadIndex(), im_bi);
	}





	bool Renderer::Is_ConstructingRenderGraph() {
		return Record_RenderGraphConstruction;
	}
}