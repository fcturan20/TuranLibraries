#include "Vulkan_Renderer_Core.h"
#include "VK_GPUContentManager.h"
#include "Vulkan/VulkanSource/Renderer/Vulkan_Resource.h"
#include "TuranAPI/Profiler_Core.h"
#define VKContentManager ((Vulkan::GPU_ContentManager*)GFXContentManager)
#define VKGPU ((Vulkan::GPU*)GFX->GPU_TO_RENDER)
#define VKWINDOW ((Vulkan::WINDOW*)GFX->Main_Window)

namespace Vulkan {
	Renderer::Renderer() {

	}

	//Create framegraphs to lower the cost of per frame rendergraph optimization (Pass Culling, Pass Merging, Wait Culling etc) proccess!
	void Create_FrameGraphs(const vector<VK_DrawPass*>& DrawPasses, const vector<VK_TransferPass*>& TransferPasses, VK_FrameGraph* FrameGraphs);

	void Renderer::Start_RenderGraphConstruction() {
		if (Record_RenderGraphConstruction) {
			LOG_CRASHING_TAPI("GFXRENDERER->Start_RenderGraphCreation() has failed because you called it before!");
			return;
		}
		Record_RenderGraphConstruction = true;
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
				if (Wait_desc.WaitedStage.TRANSFERCMD) {
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
				}
				else {
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
				if (Wait_desc.WaitedStage.TRANSFERCMD) {
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
				}
				else {
					bool is_Found = false;
					for (unsigned int CheckedDP = 0; CheckedDP < DrawPasses.size(); CheckedDP++) {
						if (*Wait_desc.WaitedPass == DrawPasses[CheckedDP]) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						LOG_ERROR_TAPI("One of the transfer passes waits for an draw pass but given pass isn't found!");
						return false;
					}
				}
			}
		}
		return true;
	}

	void Renderer::Finish_RenderGraphConstruction() {
		if (!Record_RenderGraphConstruction) {
			LOG_CRASHING_TAPI("VulkanRenderer->Finish_RenderGraphCreation() has failed because you either didn't start it or finished before!");
			return;
		}
		Record_RenderGraphConstruction = false;


		VKContentManager->Resource_Finalizations();

		{
			TURAN_PROFILE_SCOPE("Checking Wait Handles of RenderNodes");
			if (!Check_WaitHandles()) {
				LOG_CRASHING_TAPI("VulkanRenderer->Finish_RenderGraphCreation() has failed because some wait handles have issues!");
				return;
			}
		}

		Create_FrameGraphs(DrawPasses, TransferPasses, FrameGraphs);

		for (unsigned int QUEUEIndex = 0; QUEUEIndex < VKGPU->QUEUEs.size(); QUEUEIndex++) {
			//Create Command Pool per QUEUE
			if (VKGPU->QUEUEs[QUEUEIndex].SupportFlag.is_PRESENTATIONsupported &&
				!
				(VKGPU->QUEUEs[QUEUEIndex].SupportFlag.is_COMPUTEsupported || VKGPU->QUEUEs[QUEUEIndex].SupportFlag.is_GRAPHICSsupported || VKGPU->QUEUEs[QUEUEIndex].SupportFlag.is_TRANSFERsupported)
				) {
				LOG_STATUS_TAPI("VulkanCore: One of the VkQueues only supports Presentation QUEUE, so GFX API didn't create a command pool for it!");
				continue;
			}
			VkCommandPoolCreateInfo cp_ci_g = {};
			cp_ci_g.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cp_ci_g.queueFamilyIndex = VKGPU->QUEUEs[QUEUEIndex].QueueFamilyIndex;
			cp_ci_g.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			cp_ci_g.pNext = nullptr;

			if (vkCreateCommandPool(VKGPU->Logical_Device, &cp_ci_g, nullptr, &VKGPU->QUEUEs[QUEUEIndex].CommandPool) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("VulkanCore: Logical Device Setup has failed at vkCreateCommandPool()!");
				return;
			}
		}

		//Swapchain Synch Data Creation
		{
			VkSemaphoreCreateInfo Semaphore_ci = {};
			Semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			Semaphore_ci.flags = 0;
			Semaphore_ci.pNext = nullptr;
			if (vkCreateSemaphore(VKGPU->Logical_Device, &Semaphore_ci, nullptr, &Waitfor_PresentingSwapchain) != VK_SUCCESS ||
				vkCreateSemaphore(VKGPU->Logical_Device, &Semaphore_ci, nullptr, &Waitfor_SwapchainRenderGraphIdle) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("VulkanRenderer: Semaphore creation has failed!");
			}

			//Set fences as signaled for the first frame!
			for (unsigned int i = 0; i < 2; i++) {
				VkFenceCreateInfo Fence_ci = {};
				Fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				Fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				Fence_ci.pNext = nullptr;
				if (vkCreateFence(VKGPU->Logical_Device, &Fence_ci, nullptr, &RenderGraph_RunFences[i]) != VK_SUCCESS) {
					LOG_CRASHING_TAPI("VulkanRenderer: Fence creation has failed!");
				}
			}
		}

		//Swapchain Layout Operations
		//Note: One CB is created because this will be used only at the start of the frame
		{
			Swapchain_CreationCB = new VK_CommandBuffer;
			Swapchain_CreationCB->CBs.resize(1);
			Swapchain_CreationCB->CBs[0] = new VkCommandBuffer;
			Swapchain_CreationCB->Queue = &VKGPU->QUEUEs[VKGPU->GRAPHICS_QUEUEIndex].Queue;
			Swapchain_CreationCB->SignalSemaphores.clear();
			Swapchain_CreationCB->WaitSemaphores.clear();
			Swapchain_CreationCB->WaitSemaphoreStages.clear();
			VkCommandBufferAllocateInfo CB_AI = {};
			CB_AI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			CB_AI.commandBufferCount = 1;
			CB_AI.commandPool = VKGPU->QUEUEs[VKGPU->GRAPHICS_QUEUEIndex].CommandPool;
			CB_AI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			CB_AI.pNext = nullptr;
			if (vkAllocateCommandBuffers(VKGPU->Logical_Device, &CB_AI, Swapchain_CreationCB->CBs[0]) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("GFXRenderer->Finish_RenderGraphCreation() has failed at vkAllocateCommandBuffers() of the Swapchain Creation CB!");
				return;
			}


			VkCommandBufferBeginInfo bi = {};
			bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			bi.pInheritanceInfo = nullptr;
			bi.pNext = nullptr;

			if (vkBeginCommandBuffer(*Swapchain_CreationCB->CBs[0], &bi) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("VulkanRenderer: SWPCHN_IMLAYCB has has failed at vkBeginCommandBuffer()!");
				return;
			}

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

			vkCmdPipelineBarrier(*Swapchain_CreationCB->CBs[0], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0
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
		}
	}
	
	unsigned char Renderer::Get_FrameIndex(bool is_LastFrame) {
		if (is_LastFrame) {
			return Get_LastFrameIndex();
		}
		else {
			return FrameIndex;
		}
	}


	void Fill_ColorVkAttachmentDescription(VkAttachmentDescription& Desc, const VK_COLORRTSLOT* Attachment) {
		Desc = {};
		Desc.format = Find_VkFormat_byTEXTURECHANNELs(Attachment->RT->CHANNELs);
		Desc.samples = VK_SAMPLE_COUNT_1_BIT;
		switch (Attachment->LOADSTATE) {
		case GFX_API::DRAWPASS_LOAD::CLEAR:
			Desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			break;
		case GFX_API::DRAWPASS_LOAD::FULL_OVERWRITE:
			Desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			break;
		case GFX_API::DRAWPASS_LOAD::LOAD:
			Desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			break;
		}
		if (Attachment->IS_USED_LATER) {
			Desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		}
		else {
			Desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}

		if(Attachment)
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
	void Fill_DepthVkAttachmentDescription(VkAttachmentDescription& Desc, const VK_DEPTHSTENCILSLOT* Attachment) {
		if (Attachment->RT->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32) {
			if (Attachment->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::READ_ONLY) {
				Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
			}
			else {
				Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			}
		}
		else if (Attachment->RT->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8) {
			if (Attachment->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::READ_ONLY) {
				if (Attachment->STENCIL_OPTYPE != GFX_API::OPERATION_TYPE::READ_ONLY) {
					Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else {
					Desc.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				}
			}
			else{
				if (Attachment->STENCIL_OPTYPE != GFX_API::OPERATION_TYPE::READ_ONLY) {
					Desc.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else {
					Desc.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
				}
			}
		}
	}
	void Fill_SubpassStructs(VK_IRTSLOTSET* slotset, VkSubpassDescription& descs, VkSubpassDependency& dependencies) {
		VkAttachmentReference* COLOR_ATTACHMENTs = new VkAttachmentReference[slotset->BASESLOTSET->COLORSLOTs_COUNT];
		VkAttachmentReference* DS_Attach = nullptr;
		
		for (unsigned int i = 0; i < slotset->BASESLOTSET->COLORSLOTs_COUNT; i++) {
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
		if(slotset->BASESLOTSET->DEPTHSTENCIL_SLOT) {
			DS_Attach = new VkAttachmentReference;
			DS_Attach->attachment = 0;
			if (slotset->BASESLOTSET->DEPTHSTENCIL_SLOT->RT->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32) {
				switch (slotset->DEPTH_OPTYPE) {
				case GFX_API::OPERATION_TYPE::READ_ONLY:
					DS_Attach->layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
					break;
				case GFX_API::OPERATION_TYPE::READ_AND_WRITE:
				case GFX_API::OPERATION_TYPE::WRITE_ONLY:
					DS_Attach->layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
					break;
				case GFX_API::OPERATION_TYPE::UNUSED:
					DS_Attach->attachment = VK_ATTACHMENT_UNUSED;
					DS_Attach->layout = VK_IMAGE_LAYOUT_UNDEFINED;
					break;
				default:
					LOG_NOTCODED_TAPI("VK::Fill_SubpassStructs() doesn't support this type of Operation Type for DepthBuffer!", true);
				}
			}
			else if(slotset->BASESLOTSET->DEPTHSTENCIL_SLOT->RT->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8) {
				switch (slotset->STENCIL_OPTYPE) {
				case GFX_API::OPERATION_TYPE::READ_ONLY:
					if (slotset->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::UNUSED || slotset->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::READ_ONLY) {
						DS_Attach->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
					}
					else if (slotset->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::READ_AND_WRITE || slotset->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::WRITE_ONLY) {
						DS_Attach->layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
					}
					break;
				case GFX_API::OPERATION_TYPE::READ_AND_WRITE:
				case GFX_API::OPERATION_TYPE::WRITE_ONLY:
					if (slotset->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::UNUSED || slotset->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::READ_ONLY) {
						DS_Attach->layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
					}
					else if (slotset->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::READ_AND_WRITE || slotset->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::WRITE_ONLY) {
						DS_Attach->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					}
					break;
				case GFX_API::OPERATION_TYPE::UNUSED:
					if (slotset->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::UNUSED) {
						DS_Attach->attachment = VK_ATTACHMENT_UNUSED;
						DS_Attach->layout = VK_IMAGE_LAYOUT_UNDEFINED;
					}
					else if (slotset->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::READ_AND_WRITE || slotset->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::WRITE_ONLY) {
						DS_Attach->layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
					}
					else if(slotset->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::READ_ONLY){
						DS_Attach->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
					}
					break;
				default:
					LOG_NOTCODED_TAPI("VK::Fill_SubpassStructs() doesn't support this type of Operation Type for DepthSTENCILBuffer!", true);
				}
			}
		}

		descs = {};
		descs.colorAttachmentCount = slotset->BASESLOTSET->COLORSLOTs_COUNT;
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
	GFX_API::GFXHandle Renderer::Create_DrawPass(const vector<GFX_API::SubDrawPass_Description>& Subpass_gfxdescs, GFX_API::GFXHandle RTSLOTSET_ID, const vector<GFX_API::PassWait_Description>& WAITs, GFX_API::GFXHandle* GeneratedspIDs, const char* NAME) {
		if (!Record_RenderGraphConstruction) {
			LOG_CRASHING_TAPI("GFXRENDERER->CreateDrawPass() has failed because you first need to call Start_RenderGraphCreation()!");
			return 0;
		}
		VK_RTSLOTSET* SLOTSET = GFXHandleConverter(VK_RTSLOTSET*, RTSLOTSET_ID);
		for (unsigned int i = 0; i < SLOTSET->COLORSLOTs_COUNT; i++) {
			VK_COLORRTSLOT& slot = SLOTSET->COLOR_SLOTs[i];
			if (slot.RT_OPERATIONTYPE == GFX_API::OPERATION_TYPE::UNUSED) {
				LOG_ERROR_TAPI("Create_DrawPass() has failed because you can't give a unused Color RT Slot to the Draw Pass! You either don't give it, or use it");
				return nullptr;
			}
		}
		if (SLOTSET->DEPTHSTENCIL_SLOT) {
			if (SLOTSET->DEPTHSTENCIL_SLOT->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::UNUSED || SLOTSET->DEPTHSTENCIL_SLOT->STENCIL_OPTYPE == GFX_API::OPERATION_TYPE::UNUSED) {
				LOG_ERROR_TAPI("Create_DrawPass() has failed because you can't give a unused Depth or Stencil RT Slot to the Draw Pass! You either don't give it, or use it");
				return nullptr;
			}
		}
		if (!SLOTSET) {
			LOG_ERROR_TAPI("Create_DrawPass() has failed because RTSLOTSET_ID is invalid!");
			return 0;
		}
		for (unsigned int i = 0; i < Subpass_gfxdescs.size(); i++) {
			VK_IRTSLOTSET* ISET = GFXHandleConverter(VK_IRTSLOTSET*, Subpass_gfxdescs[i].INHERITEDSLOTSET);
			if (!ISET) {
				LOG_ERROR_TAPI("GFXRenderer->Create_DrawPass() has failed because one of the inherited slotsets is nullptr!");
				return nullptr;
			}
			if (ISET->BASESLOTSET != RTSLOTSET_ID) {
				LOG_ERROR_TAPI("GFXRenderer->Create_DrawPass() has failed because one of the inherited slotsets isn't inherited from the DrawPass' Base Slotset!");
				return nullptr;
			}
		}



		VK_SubDrawPass* Final_Subpasses = new VK_SubDrawPass[Subpass_gfxdescs.size()];
		VK_DrawPass* VKDrawPass = new VK_DrawPass;
		VKDrawPass->SLOTSET = SLOTSET;
		VKDrawPass->Subpass_Count = Subpass_gfxdescs.size();
		VKDrawPass->Subpasses = Final_Subpasses;
		VKDrawPass->NAME = NAME;
		VKDrawPass->WAITs = WAITs;
		DrawPasses.push_back(VKDrawPass);



		//SubDrawPass creation
		vector<VkSubpassDependency> SubpassDepends(Subpass_gfxdescs.size());
		vector<VkSubpassDescription> SubpassDescs(Subpass_gfxdescs.size());
		vector<vector<VkAttachmentReference>> SubpassAttachments(Subpass_gfxdescs.size());
		for (unsigned int i = 0; i < Subpass_gfxdescs.size(); i++) {
			const GFX_API::SubDrawPass_Description& Subpass_gfxdesc = Subpass_gfxdescs[i];
			VK_IRTSLOTSET* Subpass_Slotset = GFXHandleConverter(VK_IRTSLOTSET*, Subpass_gfxdesc.INHERITEDSLOTSET);

			Fill_SubpassStructs(Subpass_Slotset, SubpassDescs[i], SubpassDepends[i]);

			Final_Subpasses[i].Binding_Index = Subpass_gfxdesc.SubDrawPass_Index;
			Final_Subpasses[i].SLOTSET = Subpass_Slotset;
			Final_Subpasses[i].DrawPass = VKDrawPass;
			GeneratedspIDs[i] = &Final_Subpasses[i];
		}

		//vkRenderPass generation
		{
			vector<VkAttachmentDescription> AttachmentDescs;
			//Create Attachment Descriptions
			for (unsigned int i = 0; i < VKDrawPass->SLOTSET->COLORSLOTs_COUNT; i++) {
				VkAttachmentDescription Attachmentdesc = {};
				Fill_ColorVkAttachmentDescription(Attachmentdesc, &VKDrawPass->SLOTSET->COLOR_SLOTs[i]);
				AttachmentDescs.push_back(Attachmentdesc);
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
				return nullptr;
			}
		}

		return VKDrawPass;
	}



	GFX_API::GFXHandle Renderer::Create_TransferPass(const GFX_API::TransferPass_Description& tp_desc) {
		VK_TransferPass* TRANSFERPASS = new VK_TransferPass;

		TRANSFERPASS->WAITs = tp_desc.WaitDescriptions;
		TRANSFERPASS->NAME = tp_desc.NAME;
		TRANSFERPASS->TYPE = tp_desc.TP_TYPE;

		TransferPasses.push_back(TRANSFERPASS);
		return TRANSFERPASS;
	}


	void Create_QueriedTexture(VK_Texture* TEXTURE, const VK_QUEUE& QUEUE) {
		//Create Image
		{
			VkImage VKIMAGE;
			VkImageCreateInfo im_ci = {};
			im_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			im_ci.arrayLayers = 1;
			im_ci.extent.width = TEXTURE->WIDTH;
			im_ci.extent.height = TEXTURE->HEIGHT;
			im_ci.extent.depth = 1;
			im_ci.flags = 0;
			im_ci.format = Find_VkFormat_byTEXTURECHANNELs(TEXTURE->CHANNELs);
			im_ci.imageType = VK_IMAGE_TYPE_2D;
			im_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			im_ci.mipLevels = 1;
			im_ci.pNext = nullptr;
			im_ci.pQueueFamilyIndices = &QUEUE.QueueFamilyIndex;
			im_ci.queueFamilyIndexCount = 1;
			im_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			im_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
			im_ci.usage = Find_VKImageUsage_forVKTexture(*TEXTURE);
			im_ci.samples = VK_SAMPLE_COUNT_1_BIT;

			if (vkCreateImage(VKGPU->Logical_Device, &im_ci, nullptr, &TEXTURE->Image) != VK_SUCCESS) {
				LOG_ERROR_TAPI("Create_QueriedTexture() has failed in vkCreateImage()!");
				return;
			}

			VKContentManager->Suballocate_Image(VKIMAGE, SUBALLOCATEBUFFERTYPEs::GPULOCALTEX, TEXTURE->GPUMemoryOffset);
		}

		//Create Image View
		{
			VkImageView VKIMAGEVIEW;
			VkImageViewCreateInfo ci = {};
			ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ci.flags = 0;
			ci.pNext = nullptr;

			ci.image = TEXTURE->Image;
			ci.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
			ci.format = Find_VkFormat_byTEXTURECHANNELs(TEXTURE->CHANNELs);
			ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ci.subresourceRange.baseArrayLayer = 0;
			ci.subresourceRange.layerCount = 1;
			ci.subresourceRange.baseMipLevel = 0;
			ci.subresourceRange.levelCount = 1;

			if (vkCreateImageView(VKGPU->Logical_Device, &ci, nullptr, &VKIMAGEVIEW) != VK_SUCCESS) {
				LOG_ERROR_TAPI("Create_QueriedTexture() has failed in vkCreateImageView()!");
				return;
			}
			TEXTURE->ImageView = VKIMAGEVIEW;
		}
	}
	/*
	void Renderer::Create_VulkanCalls() {
		LOG_NOTCODED_TAPI("Create_VulkanCalls() isn't coded!", true);

		VkSubmitInfo CB_si = {};
		CB_si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		CB_si.pNext = nullptr;
		VkSemaphore SwapchainAvailabilitySemaphore[] = { Waitfor_PresentingSwapchain };
		VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
		CB_si.waitSemaphoreCount = 1;
		CB_si.pWaitSemaphores = SwapchainAvailabilitySemaphore;
		CB_si.pWaitDstStageMask = WaitStages;
		CB_si.commandBufferCount = 1;
		CB_si.pCommandBuffers = &PerFrame_CommandBuffers[SwapchainIndex];

		VkSemaphore RenderCBFinishSemaphore[] = { Waitfor_SwapchainRenderGraphIdle };
		CB_si.signalSemaphoreCount = 1;
		CB_si.pSignalSemaphores = RenderCBFinishSemaphore;
		if (vkQueueSubmit(VKGPU->GRAPHICSQueue.Queue, 1, &CB_si, RenderGraph_RunFences[FrameIndex]) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Command Buffer submit has failed!");
		}
	}
	void Renderer::Run_RenderGraph() {
		//Begin Command Buffer
		{
			VkCommandBufferBeginInfo bi = {};
			bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			bi.flags = 0;
			bi.pInheritanceInfo = nullptr;
			bi.pNext = nullptr;
			if (vkBeginCommandBuffer(PerFrame_CommandBuffers[SwapchainIndex], &bi) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Renderer::Run_RenderGraph() has failed at vkBeginCommandBuffer()!");
			}
		}
		//Begin Render Pass
		{
			VK_DrawPass* VKDP = DrawPasses[0];
			VkRenderPassBeginInfo bi = {};
			bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			bi.renderPass = VKDP->RenderPassObject;
			//All RT_SLOTs should be in the same size and I checked it while creating the draw pass, so there is no problem using 0!
			bi.renderArea.extent.width = VKDP->SLOTSET[0].COLOR_SLOTs[0]->RT->WIDTH;
			bi.renderArea.extent.height = VKDP->SLOTSET[0].COLOR_SLOTs[0]->RT->HEIGHT;
			bi.renderArea.offset.x = 0; bi.renderArea.offset.y = 0;
			bi.pNext = nullptr;

			vec4 colorvalue = VKDP->SLOTSET->COLOR_SLOTs[0]->CLEAR_COLOR;
			VkClearValue Clear = { colorvalue.x, colorvalue.y, colorvalue.z, colorvalue.w };
			bi.clearValueCount = 1;
			bi.pClearValues = &Clear;
			bi.framebuffer = VKDP->FramebufferObjects[0];
			vkCmdBeginRenderPass(PerFrame_CommandBuffers[SwapchainIndex], &bi, VK_SUBPASS_CONTENTS_INLINE);
		}
		VK_SubDrawPass& FirstSubpass = DrawPasses[0]->Subpasses[0];
		VkViewport Viewport;
		Viewport.width = VKWINDOW->Get_Window_Mode().x;
		Viewport.height = VKWINDOW->Get_Window_Mode().y;
		Viewport.x = 0;	Viewport.y = 0;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;
		VkRect2D Scissor;
		Scissor.extent.width = VKWINDOW->Get_Window_Mode().x;
		Scissor.extent.height = VKWINDOW->Get_Window_Mode().y;
		Scissor.offset.x = 0; Scissor.offset.y = 0;
		for (unsigned int i = 0; i < FirstSubpass.DrawCalls.size(); i++) {
			vkCmdSetViewport(PerFrame_CommandBuffers[SwapchainIndex], 0, 1, &Viewport);
			vkCmdSetScissor(PerFrame_CommandBuffers[SwapchainIndex], 0, 1, &Scissor);
			GFX_API::DrawCall_Description* DrawCall = &FirstSubpass.DrawCalls[i];
			VK_PipelineInstance* VKInstance = GFXHandleConverter(VK_PipelineInstance*, DrawCall->ShaderInstance_ID);
			VK_GraphicsPipeline* VKType = VKInstance->PROGRAM;
			vkCmdBindPipeline(PerFrame_CommandBuffers[SwapchainIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, VKType->PipelineObject);
			VkDescriptorSet SETs[3] = { VKContentManager->GlobalBuffers_DescSet, VKType->General_DescSet, VKInstance->DescSet };
			vkCmdBindDescriptorSets(PerFrame_CommandBuffers[SwapchainIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, VKType->PipelineLayout, 0, 3, SETs, 0, nullptr);
			VK_Mesh* MESH = GFXHandleConverter(VK_Mesh*, DrawCall->MeshBuffer_ID);
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(PerFrame_CommandBuffers[SwapchainIndex], 0, 1, &MESH->Buffer, &offset);
			vkCmdDraw(PerFrame_CommandBuffers[SwapchainIndex], MESH->VERTEX_COUNT, 1, 0, 0);
		}
		FirstSubpass.DrawCalls.clear();
		vkCmdEndRenderPass(PerFrame_CommandBuffers[SwapchainIndex]);
		VkImageMemoryBarrier Barriers[2];
		VK_Texture* VKTEXTURE = FinalColorTexture;
		{
			//Barrier 0 is for Source Image
			Barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			Barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			Barriers[0].image = VKTEXTURE->Image;
			Barriers[0].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			Barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			Barriers[0].pNext = nullptr;
			Barriers[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			Barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			Barriers[0].subresourceRange.baseArrayLayer = 0;
			Barriers[0].subresourceRange.baseMipLevel = 0;
			Barriers[0].subresourceRange.layerCount = 1;
			Barriers[0].subresourceRange.levelCount = 1;


			Barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			Barriers[1].image = VKWINDOW->SwapChain_Images[SwapchainIndex];
			Barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			Barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			Barriers[1].pNext = nullptr;
			Barriers[1].srcAccessMask = 0;
			Barriers[1].oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			Barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			Barriers[1].subresourceRange.baseArrayLayer = 0;
			Barriers[1].subresourceRange.baseMipLevel = 0;
			Barriers[1].subresourceRange.layerCount = 1;
			Barriers[1].subresourceRange.levelCount = 1;
		}
		vkCmdPipelineBarrier(PerFrame_CommandBuffers[SwapchainIndex], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
			, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 2, Barriers);
		VkImageCopy copyregion = {};
		{
			copyregion.dstOffset.x = 0; copyregion.dstOffset.y = 0; copyregion.dstOffset.z = 0;
			copyregion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyregion.dstSubresource.baseArrayLayer = 0;
			copyregion.dstSubresource.layerCount = 1;
			copyregion.dstSubresource.mipLevel = 0;
			copyregion.extent.depth = 1; copyregion.extent.width = FinalColorTexture->WIDTH; copyregion.extent.height = FinalColorTexture->HEIGHT;
			copyregion.srcOffset.x = 0; copyregion.srcOffset.y = 0; copyregion.srcOffset.z = 0;
			copyregion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyregion.srcSubresource.baseArrayLayer = 0;
			copyregion.srcSubresource.layerCount = 1;
			copyregion.srcSubresource.mipLevel = 0;
		}

		vkCmdCopyImage(PerFrame_CommandBuffers[SwapchainIndex], VKTEXTURE->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			, VKWINDOW->SwapChain_Images[SwapchainIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyregion);
		VkImageMemoryBarrier LastBarriers[2];
		{
			//0 is Swapchain image, 1 is FinalColor_Texture
			LastBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			LastBarriers[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			LastBarriers[0].image = VKWINDOW->SwapChain_Images[SwapchainIndex];
			LastBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			LastBarriers[0].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			LastBarriers[0].pNext = nullptr;
			LastBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			LastBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			LastBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			LastBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			LastBarriers[0].subresourceRange.baseArrayLayer = 0;
			LastBarriers[0].subresourceRange.baseMipLevel = 0;
			LastBarriers[0].subresourceRange.layerCount = 1;
			LastBarriers[0].subresourceRange.levelCount = 1;

			LastBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			LastBarriers[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			LastBarriers[1].image = VKTEXTURE->Image;
			LastBarriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			LastBarriers[1].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			LastBarriers[1].pNext = nullptr;
			LastBarriers[1].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			LastBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			LastBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			LastBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			LastBarriers[1].subresourceRange.baseArrayLayer = 0;
			LastBarriers[1].subresourceRange.baseMipLevel = 0;
			LastBarriers[1].subresourceRange.layerCount = 1;
			LastBarriers[1].subresourceRange.levelCount = 1;
		}
		vkCmdPipelineBarrier(PerFrame_CommandBuffers[SwapchainIndex], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
			, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 2, LastBarriers);
		vkEndCommandBuffer(PerFrame_CommandBuffers[SwapchainIndex]);
	}*/
	void Renderer::Run() {
		/*
		//Get current swapchain!
		{
			uint32_t SwapchainImage_Index;
			vkAcquireNextImageKHR(VKGPU->Logical_Device, VKWINDOW->Window_SwapChain, UINT64_MAX, Waitfor_PresentingSwapchain, VK_NULL_HANDLE, &SwapchainImage_Index);
			if (SwapchainImage_Index != FrameIndex) {
				std::cout << "Current SwapchainImage_Index: " << SwapchainImage_Index << std::endl;
				std::cout << "Current FrameCount: " << FrameIndex << std::endl;
				LOG_CRASHING_TAPI("Renderer's FrameCount and Vulkan's SwapchainIndex don't match, there is something!");
			}
		}
		vkWaitForFences(VKGPU->Logical_Device, 1, &RenderGraph_RunFences[FrameIndex], true, UINT64_MAX);
		vkResetFences(VKGPU->Logical_Device, 1, &RenderGraph_RunFences[FrameIndex]);

		RenderGraph_ResolveThisFrame();


		VkPresentInfoKHR SwapchainImage_PresentationInfo = {};
		SwapchainImage_PresentationInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		SwapchainImage_PresentationInfo.pNext = nullptr;
		SwapchainImage_PresentationInfo.waitSemaphoreCount = 1;
		SwapchainImage_PresentationInfo.pWaitSemaphores = &Waitfor_SwapchainRenderGraphIdle;

		VkSwapchainKHR Swapchains[] = { VKWINDOW->Window_SwapChain };
		SwapchainImage_PresentationInfo.swapchainCount = 1;
		SwapchainImage_PresentationInfo.pSwapchains = Swapchains;
		uint32_t FrameIndex_uint32_t = FrameIndex;
		SwapchainImage_PresentationInfo.pImageIndices = &FrameIndex_uint32_t;
		SwapchainImage_PresentationInfo.pResults = nullptr;
		if (vkQueuePresentKHR(VKGPU->DISPLAYQueue.Queue, &SwapchainImage_PresentationInfo) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Submitting Presentation Queue has failed!");
		}

		//We presented this frame, there's no way to continue this frame!
		FrameIndex++;*/
	}
	void Renderer::Render_DrawCall(GFX_API::DrawCall_Description drawcall) {
		VK_SubDrawPass* SP = GFXHandleConverter(VK_SubDrawPass*, drawcall.SubDrawPass_ID);
		SP->DrawCalls.push_back(drawcall);
	}


	/*
	unsigned int Renderer::Create_BarrierID() {
		unsigned int ID = BARRIERID_BITSET.GetIndex_FirstFalse() + 1;
		BARRIERID_BITSET.SetBit_True(ID - 1);
		return ID;
	}
	void Renderer::Delete_BarrierID(unsigned int ID) {
		BARRIERID_BITSET.SetBit_False(ID - 1);
	}
	VK_Barrier* Renderer::Find_Barrier_byID(unsigned int PacketID, unsigned int* vector_index) {
		VK_Barrier* BR = nullptr;
		for (unsigned int i = 0; i < BARRIERs.size(); i++) {
			BR = BARRIERs[i];
			if (BR->ID == PacketID) {
				if (vector_index) {
					*vector_index = i;
				}
				return BR;
			}
		}
		LOG_ERROR("GPU_ContentManager::Find_Barrier_byID() has failed!");
		return nullptr;
	}

	void Renderer::Add_to_Barrier(unsigned int TransferPacketID, void* TransferHandle) {
		VK_Barrier* pack = Find_Barrier_byID(TransferPacketID);
		pack->Packet.push_back(TransferHandle);
	}*/








	bool Renderer::Is_ConstructingRenderGraph() {
		return Record_RenderGraphConstruction;
	}

	//ID OPERATIONs
	/*
	VK_DrawPass* Renderer::Find_DrawPass_byID(unsigned int ID) {
		for (unsigned int i = 0; i < DrawPasses.size(); i++) {
			VK_DrawPass* DrawPass = DrawPasses[i];
			if (DrawPass->ID == ID) {
				return DrawPass;
			}
		}
		LOG_ERROR("Find_DrawPass_byID() has failed, ID: " + to_string(ID));
		return nullptr;
	}
	VK_SubDrawPass* Renderer::Find_SubDrawPass_byID(unsigned int ID, unsigned int* i_drawpass_id) {
		for (unsigned int i = 0; i < DrawPasses.size(); i++) {
			VK_DrawPass* DrawPass = DrawPasses[i];
			for (unsigned int j = 0; j < DrawPass->Subpass_Count; j++) {
				VK_SubDrawPass& Subpass = DrawPass->Subpasses[i];
				if (Subpass.ID == ID) {
					if (i_drawpass_id) {
						*i_drawpass_id = DrawPass->ID;
					}
					return &Subpass;
				}
			}
		}
		LOG_ERROR("Find_SubDrawPass_byID() has failed, ID: " + to_string(ID));
		return nullptr;
	}*/
}