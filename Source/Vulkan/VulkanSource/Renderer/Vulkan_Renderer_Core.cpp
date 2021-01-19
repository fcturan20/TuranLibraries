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
		if (Record_RenderGraphConstruction) {
			LOG_CRASHING_TAPI("GFXRENDERER->Start_RenderGraphCreation() has failed because you called it before!");
			return;
		}
		Record_RenderGraphConstruction = true;
	}
	TPType FindWaitedTPType(GFX_API::SHADERSTAGEs_FLAG flag) {
		bool SupportedOnes = 0;
		if (flag.COLORRTOUTPUT || flag.FRAGMENTSHADER || flag.VERTEXSHADER) {
			if (flag.SWAPCHAINDISPLAY || flag.TRANSFERCMD) {
				LOG_CRASHING_TAPI("You set Wait Info->Stage wrong!");
				return TPType::ERROR;
			}
			return TPType::DP;
		}
		if (flag.SWAPCHAINDISPLAY) {
			if (flag.TRANSFERCMD) {
				LOG_CRASHING_TAPI("You set Wait Info->Stage wrong!");
				return TPType::ERROR;
			}
			return TPType::WP;
		}
		if (flag.TRANSFERCMD) {
			return TPType::TP;
		}
		LOG_NOTCODED_TAPI("You set WaitInfo->Stage to a stage that isn't supported right now by FindWaitedTPType!", true);
		return TPType::ERROR;
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
				switch (FindWaitedTPType(Wait_desc.WaitedStage)) {
					case TPType::DP:
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
					case TPType::TP:
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
					case TPType::WP:
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
					case TPType::ERROR:
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

				switch (FindWaitedTPType(Wait_desc.WaitedStage)) {
				case TPType::DP:
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
				case TPType::TP:
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
				case TPType::WP:
					LOG_CRASHING_TAPI("A window pass can't wait for another window pass, Check Wait Handle failed!");
					return false;
				case TPType::ERROR:
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

				switch (FindWaitedTPType(Wait_desc.WaitedStage)) {
					case TPType::DP:
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
					case TPType::TP:
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
					case TPType::WP: 
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
					case TPType::ERROR:
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

	void Renderer::Record_CurrentFramegraph() {
		TURAN_PROFILE_SCOPE_MCS("Run_CurrentFramegraph()");
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
					case TPType::TP:
					{
						VK_TransferPass* TP = GFXHandleConverter(VK_TransferPass*, CorePass->Handle);
						if (!TP->TransferDatas) {
							//continue;
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
						}
							break;
						case GFX_API::TRANFERPASS_TYPE::TP_COPY:
							LOG_NOTCODED_TAPI("VulkanRenderer: COPY TP Datas isn't coded, so traversing it has failed!", false);
							break;
						case GFX_API::TRANFERPASS_TYPE::TP_DOWNLOAD:
							LOG_NOTCODED_TAPI("VulkanRenderer: DOWNLOAD TP Datas isn't coded, so traversing it has failed!", false);
							break;
						case GFX_API::TRANFERPASS_TYPE::TP_UPLOAD:
							if (//GFXHandleConverter(VK_TPUploadDatas*, TP->TransferDatas)->BufferUploads.size() ||
								//GFXHandleConverter(VK_TPUploadDatas*, TP->TransferDatas)->TextureUploads.size()
								true) {
								Branch.CurrentFramePassesIndexes[CurrentFrameIndexesArray_Element] = PassIndex + 1;
								CurrentFrameIndexesArray_Element++;
								Branch.CFNeeded_QueueSpecs.is_TRANSFERsupported = true;
							}
							break;
						default:
							LOG_NOTCODED_TAPI("VulkanRenderer: CurrentFrame_WorkloadAnalysis() doesn't support this type of transfer pass type!", true);
							break;
						}
				}
					break;
					case TPType::DP: 
					{
						VK_DrawPass* DP = GFXHandleConverter(VK_DrawPassInstance*, CorePass->Handle)->BasePass;
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
					case TPType::WP:
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
	}
	//Each framegraph is constructed as same, so there is no difference about passing different framegraphs here
	void Create_VkDataofRGBranches(const VK_FrameGraph& FrameGraph, vector<VK_Semaphore>& Semaphores);
	void Renderer::Finish_RenderGraphConstruction() {
		if (!Record_RenderGraphConstruction) {
			LOG_CRASHING_TAPI("VulkanRenderer->Finish_RenderGraphCreation() has failed because you either didn't start it or finished before!");
			return;
		}
		Record_RenderGraphConstruction = false;


		VKContentManager->Resource_Finalizations();

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
			VkCommandPoolCreateInfo cp_ci_g = {};
			cp_ci_g.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cp_ci_g.queueFamilyIndex = VKGPU->QUEUEs[QUEUEIndex].QueueFamilyIndex;
			cp_ci_g.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			cp_ci_g.pNext = nullptr;

			if (vkCreateCommandPool(VKGPU->Logical_Device, &cp_ci_g, nullptr, &VKGPU->QUEUEs[QUEUEIndex].CommandPool.CPHandle) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("VulkanCore: Logical Device Setup has failed at vkCreateCommandPool()!");
				return;
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
				if (vkCreateFence(VKGPU->Logical_Device, &Fence_ci, nullptr, &VKGPU->QUEUEs[QueueIndex].RenderGraphFences[i]) != VK_SUCCESS) {
					LOG_CRASHING_TAPI("VulkanRenderer: Fence creation has failed!");
				}
				if (vkResetFences(VKGPU->Logical_Device, 1, &VKGPU->QUEUEs[QueueIndex].RenderGraphFences[i]) != VK_SUCCESS) {
					LOG_CRASHING_TAPI("Fix the reset fence at FinishConstruction!");
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
			DS_Attach = new VkAttachmentReference;
			DS_Attach->attachment = 0;
			if (slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->RT->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32) {
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
			else if(slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->RT->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8) {
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
			if (SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::UNUSED || SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->STENCIL_OPTYPE == GFX_API::OPERATION_TYPE::UNUSED) {
				LOG_ERROR_TAPI("Create_DrawPass() has failed because you can't give a unused Depth or Stencil RT Slot to the Draw Pass! You either don't give it, or use it");
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
		case GFX_API::TRANFERPASS_TYPE::TP_UPLOAD:
			TRANSFERPASS->TransferDatas = new VK_TPUploadDatas;
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
		//Wait for command buffers to end
		for (unsigned char QueueIndex = 0; QueueIndex < VKGPU->QUEUEs.size(); QueueIndex++) {
			vkWaitForFences(VKGPU->Logical_Device, 1, &VKGPU->QUEUEs[QueueIndex].RenderGraphFences[FrameIndex], true, UINT64_MAX);
			vkResetFences(VKGPU->Logical_Device, 1, &VKGPU->QUEUEs[QueueIndex].RenderGraphFences[FrameIndex]);
		}

		//Record command buffers
		Record_CurrentFramegraph();
		
		//Maybe the rendering of the last frame took less than V-Sync, this is to handle this case
		//Because that means the swapchain texture we will render to in above command buffers is being displayed
		for(unsigned char WindowIndex = 0; WindowIndex < ((Vulkan_Core*)GFX)->Get_WindowHandles().size(); WindowIndex++) {
			WINDOW* VKWINDOW = GFXHandleConverter(WINDOW*, ((Vulkan_Core*)GFX)->Get_WindowHandles()[WindowIndex]);
			uint32_t SwapchainImage_Index;
			vkAcquireNextImageKHR(VKGPU->Logical_Device, VKWINDOW->Window_SwapChain, UINT64_MAX,
			VKWINDOW->PresentationWaitSemaphores[FrameIndex], VK_NULL_HANDLE, &SwapchainImage_Index);
			if (SwapchainImage_Index != FrameIndex) {
				std::cout << "Current SwapchainImage_Index: " << SwapchainImage_Index << std::endl;
				std::cout << "Current FrameCount: " << FrameIndex << std::endl;
				LOG_CRASHING_TAPI("Renderer's FrameCount and Vulkan's SwapchainIndex don't match, there is something!");
			}
		}

		//Send command buffers
		for (unsigned char SubmitIndex = 0; SubmitIndex < FrameGraphs[FrameIndex].CurrentFrameSubmits.size(); SubmitIndex++) {
			VK_Submit* Submit = FrameGraphs[FrameIndex].CurrentFrameSubmits[SubmitIndex];
			
		}
		
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
		

		//Shift all WindowCall buffers of all Window Passes!
		for (unsigned char WPIndex = 0; WPIndex < WindowPasses.size(); WPIndex++) {
			VK_WindowPass* WP = WindowPasses[WPIndex];
			WP->WindowCalls[0] = WP->WindowCalls[1];
			WP->WindowCalls[1] = WP->WindowCalls[2];
			WP->WindowCalls[2].clear();
		}

		//Current frame has finished, so every call after this call affects to the next frame
		Set_NextFrameIndex();

	}
	void Renderer::Render_DrawCall(GFX_API::GFXHandle VertexBuffer_ID, GFX_API::GFXHandle IndexBuffer_ID, GFX_API::GFXHandle MaterialInstance_ID, GFX_API::GFXHandle SubDrawPass_ID) {
		VK_SubDrawPass* SP = GFXHandleConverter(VK_SubDrawPass*, SubDrawPass_ID);
		LOG_NOTCODED_TAPI("GFXRenderer->Render_DrawCall() isn't coded yet!", true);
	}
	void Renderer::SwapBuffers(GFX_API::GFXHandle WindowHandle, GFX_API::GFXHandle WindowPassHandle, const GFX_API::IMAGEUSAGE& PREVIOUS_IMUSAGE, const GFX_API::SHADERSTAGEs_FLAG& PREVIOUS_SHADERSTAGE) {
		LOG_NOTCODED_TAPI("SwapBuffers() is not coded yet!", true);
	}



	void Renderer::TransferCall_ImUpload(VK_TransferPass* TP, VK_Texture* Image, VkDeviceSize StagingBufferOffset) {
		VK_TPUploadDatas* UploadData = GFXHandleConverter(VK_TPUploadDatas*, TP->TransferDatas);
		VK_ImUploadInfo info;
		info.IMAGE = Image;
		info.StagingBufferOffset = StagingBufferOffset;
		UploadData->TextureUploads.push_back(GFX->JobSys->GetThisThreadIndex(), info);
	}





	bool Renderer::Is_ConstructingRenderGraph() {
		return Record_RenderGraphConstruction;
	}
}