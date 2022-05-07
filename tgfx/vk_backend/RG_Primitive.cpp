//This is to test other backend functions with minimum rendergraph complexity
#include "renderer.h"
#include "gpucontentmanager.h"
#include "imgui_vktgfx.h"
#include "resource.h"
#include <tgfx_forwarddeclarations.h>
#include "queue.h"
#include "core.h"
#include "synchronization_sys.h"
#include "RG.h"

struct framegraphsys_vk {
	static std::vector<VK_PassDesc*> VKPASSes;
	static std::vector<VK_PassDesc*> Execution_Order;

	static std::vector<fence_idtype_vk> RENDERING_FENCEs[2];
	//We need this struct to destroy rendering submission semaphores waited by VkQueuePresentKHR
	//Each submission should have current frame dependencies (normal passes + WP) + 1 (for next frame, but not supported in rendergraph_primitive) number of semaphores
	struct swapchain_fence {
		std::vector<WINDOW_VKOBJ*> windows;
		std::vector<semaphore_idtype_vk> semaphores;
	};
	static std::vector<swapchain_fence> swapchain_fences;
public:
};
void Create_FrameGraphs() {
	//While constructing main pass info, we should find the merged passes by pointer comparison
	//Because merged pass info doesn't help while constructing RG


	//Check merged pass consistency across all passes
	//For example: If DP-A is merged with TP-A, TP-A shouldn't merge with any other DP
	//Return if fail


	//Check if two pass waits for each other in first subpass (not-merged) 
	//This shouldn't happen, only happen in a merged way or either of them wait for last frame
	//Return if fail

	//Find passes that has either no pass or waits only for last frame passes
	//They create the first branch abd



	VKPASSes.clear();

	VK_PassDesc* searched_dependency = nullptr;
	Execution_Order.clear();
	bool is_searching = true;
	while (is_searching) {
		is_searching = false;
		for (unsigned int pass_i = 0; pass_i < VKPASSes.size(); pass_i++) {
			VK_PassDesc* main_pass = VKPASSes[pass_i];
			if (main_pass->WAITsCOUNT == 0) {
				if (!searched_dependency) {
					Execution_Order.push_back(main_pass);
					searched_dependency = main_pass;
					is_searching = true;
					break;
				}
				else {
					continue;
				}
			}
			//Primitive RenderGraph doesn't support multiple dependencies, each pass can have only one dependency and it's a valid pass!
			if (*main_pass->WAITs[0].WaitedPass == searched_dependency) {
				Execution_Order.push_back(main_pass);
				searched_dependency = main_pass;
				is_searching = true;
				break;
			}
		}
	}
}
inline static void find_and_clear_swapchainfence(WINDOW_VKOBJ* window) {
	for (unsigned int fence_i = 0; fence_i < swapchain_fences.size(); fence_i++) {
		framegraphsys_vk::swapchain_fence& fence = swapchain_fences[fence_i];
		for (unsigned int window_i = 0; window_i < swapchain_fences[fence_i].windows.size(); window_i++) {
			if (fence.windows[window_i] == window) {
				for (unsigned int semaphore_i = 0; semaphore_i < fence.semaphores.size(); semaphore_i++) {
					semaphoresys->DestroySemaphore(fence.semaphores[semaphore_i]);
				}
			}
		}
		swapchain_fences.erase(swapchain_fences.begin() + fence_i);
		fence_i--;
	}
}
std::vector<VK_PassDesc*> framegraphsys_vk::VKPASSes = std::vector<VK_PassDesc*>(), framegraphsys_vk::Execution_Order = std::vector<VK_PassDesc*>();
std::vector<fence_idtype_vk> framegraphsys_vk::RENDERING_FENCEs[2] = { {}, {} };
std::vector<framegraphsys_vk::swapchain_fence> framegraphsys_vk::swapchain_fences = {};



void SetBarrier_BetweenPasses(VkCommandBuffer CB, VK_PassDesc* CurrentPass, VK_PassDesc* LastPass, VkPipelineStageFlags& srcPipelineStage) {
	printer(result_tgfx_WARNING, "SetBarrier_BetweenPasses() isn't coded");
}
void Record_RenderPass(VkCommandBuffer CB, RenderGraph::DP_VK* DrawPass) {
	const unsigned char FRAMEINDEX = renderer->Get_FrameIndex(false);

	if (!DrawPass->isWorkloaded()) { return; }
	rtslots_vk& CF_SLOTs = contentmanager->GETRTSLOTSET_ARRAY().getOBJbyINDEX(DrawPass->BASESLOTSET_ID)->PERFRAME_SLOTSETs[FRAMEINDEX];
	VkRenderPassBeginInfo rp_bi = {};
	rp_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_bi.renderPass = DrawPass->RenderPassObject;
	rp_bi.renderArea.extent.width = DrawPass->RenderRegion.WIDTH;
	rp_bi.renderArea.extent.height = DrawPass->RenderRegion.HEIGHT;
	rp_bi.renderArea.offset.x = DrawPass->RenderRegion.XOffset;
	rp_bi.renderArea.offset.y = DrawPass->RenderRegion.YOffset;
	rp_bi.pNext = nullptr;
	rp_bi.pClearValues = nullptr;
	std::vector<VkClearValue> CLEARVALUEs(CF_SLOTs.COLORSLOTs_COUNT + (CF_SLOTs.DEPTHSTENCIL_SLOT ? 1 : 0));
	if (CF_SLOTs.COLORSLOTs_COUNT) {
		for (unsigned char COLORRTIndex = 0; COLORRTIndex < CF_SLOTs.COLORSLOTs_COUNT; COLORRTIndex++) {
			CLEARVALUEs[COLORRTIndex] = { CF_SLOTs.COLOR_SLOTs[COLORRTIndex].CLEAR_COLOR.x,
			CF_SLOTs.COLOR_SLOTs[COLORRTIndex].CLEAR_COLOR.y,
			CF_SLOTs.COLOR_SLOTs[COLORRTIndex].CLEAR_COLOR.z,
			CF_SLOTs.COLOR_SLOTs[COLORRTIndex].CLEAR_COLOR.w };
		}
		rp_bi.pClearValues = CLEARVALUEs.data();
	}
	if (CF_SLOTs.DEPTHSTENCIL_SLOT) {
		CLEARVALUEs[CF_SLOTs.COLORSLOTs_COUNT] = { CF_SLOTs.DEPTHSTENCIL_SLOT->CLEAR_COLOR.x,
			CF_SLOTs.DEPTHSTENCIL_SLOT->CLEAR_COLOR.y, 0.0f, 0.0f };
		rp_bi.pClearValues = CLEARVALUEs.data();
	}
	rp_bi.clearValueCount = CLEARVALUEs.size();
	rp_bi.framebuffer = DrawPass->FBs[FRAMEINDEX];

	vkCmdBeginRenderPass(CB, &rp_bi, VK_SUBPASS_CONTENTS_INLINE);
	RenderGraph::SubDP_VK* Subpasses = DrawPass->getSUBDPs();
	for (unsigned char SubPassIndex = 0; SubPassIndex < DrawPass->ALLSubDPsCOUNT; SubPassIndex++) {
		RenderGraph::SubDP_VK& SP = Subpasses[SubPassIndex];
		if (SP.RG_BINDINDEX != SubPassIndex) {
			printer(result_tgfx_FAIL, "Subpass Binding Index and the SubDrawPass' element index doesn't match, handle this case!");
		}
		//NonIndexed Draw Calls
		{
			VkDescriptorSet Sets[VKCONST_MAXDESCSET_PERLIST * 2];
			for (unsigned char DrawCallIndex = 0; DrawCallIndex < SP.NonIndexedDrawCalls.size(); DrawCallIndex++) {
				RenderGraph::nonindexeddrawcall_vk& DrawCall = SP.NonIndexedDrawCalls[DrawCallIndex];
				VkViewport DefaultViewport = {};
				DefaultViewport.height = (float)DrawPass->RenderRegion.HEIGHT;
				DefaultViewport.maxDepth = 1.0f;
				DefaultViewport.minDepth = 0.0f;
				DefaultViewport.width = (float)DrawPass->RenderRegion.WIDTH;
				DefaultViewport.x = (float)DrawPass->RenderRegion.XOffset;
				DefaultViewport.y = (float)DrawPass->RenderRegion.YOffset;
				vkCmdSetViewport(CB, 0, 1, &DefaultViewport);
				VkRect2D Scissor;
				Scissor.extent.height = DrawPass->RenderRegion.HEIGHT;
				Scissor.extent.width = DrawPass->RenderRegion.WIDTH;
				Scissor.offset.x = DrawPass->RenderRegion.XOffset;
				Scissor.offset.y = DrawPass->RenderRegion.YOffset;
				vkCmdSetScissor(CB, 0, 1, &Scissor);


				vkCmdBindPipeline(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.base.MatTypeObj);
				uint32_t offset = 0, bindingtablelist_size = 0;
				for (uint32_t i = 0; i < VKCONST_MAXDESCSET_PERLIST && DrawCall.base.SETs[i] != UINT16_MAX; i++, bindingtablelist_size++) {
					VkDescriptorSet set = contentmanager->GETDESCSET_ARRAY().getOBJbyINDEX(DrawCall.base.SETs[i])->Sets[VKGLOBAL_FRAMEINDEX];
					if (set == Sets[i] && offset == i) { offset++; continue; }
					Sets[i] = set;
				}
				vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_COMPUTE, DrawCall.base.MatTypeLayout, offset, bindingtablelist_size - offset, Sets, 0, nullptr);
				if (DrawCall.VBuffer != VK_NULL_HANDLE) {
					vkCmdBindVertexBuffers(CB, 0, 1, &DrawCall.VBuffer, &DrawCall.VOffset);
				}
				vkCmdDraw(CB, DrawCall.VertexCount, DrawCall.InstanceCount, DrawCall.FirstVertex, DrawCall.FirstInstance);
			}
			SP.NonIndexedDrawCalls.clear();
		}
		//Indexed Draw Calls
		{
			VkDescriptorSet Sets[VKCONST_MAXDESCSET_PERLIST * 2];
			for (unsigned char DrawCallIndex = 0; DrawCallIndex < SP.IndexedDrawCalls.size(); DrawCallIndex++) {
				RenderGraph::indexeddrawcall_vk& DrawCall = SP.IndexedDrawCalls[DrawCallIndex];
				VkViewport DefaultViewport = {};
				DefaultViewport.height = (float)DrawPass->RenderRegion.HEIGHT;
				DefaultViewport.maxDepth = 1.0f;
				DefaultViewport.minDepth = 0.0f;
				DefaultViewport.width = (float)DrawPass->RenderRegion.WIDTH;
				DefaultViewport.x = (float)DrawPass->RenderRegion.XOffset;
				DefaultViewport.y = (float)DrawPass->RenderRegion.YOffset;
				vkCmdSetViewport(CB, 0, 1, &DefaultViewport);
				VkRect2D Scissor;
				Scissor.extent.height = DrawPass->RenderRegion.HEIGHT;
				Scissor.extent.width = DrawPass->RenderRegion.WIDTH;
				Scissor.offset.x = DrawPass->RenderRegion.XOffset;
				Scissor.offset.y = DrawPass->RenderRegion.YOffset;
				vkCmdSetScissor(CB, 0, 1, &Scissor);

				vkCmdBindPipeline(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.base.MatTypeObj);
				uint32_t offset = 0, bindingtablelist_size = 0;
				for (uint32_t i = 0; i < VKCONST_MAXDESCSET_PERLIST && DrawCall.base.SETs[i] != UINT16_MAX; i++, bindingtablelist_size++) {
					VkDescriptorSet set = contentmanager->GETDESCSET_ARRAY().getOBJbyINDEX(DrawCall.base.SETs[i])->Sets[VKGLOBAL_FRAMEINDEX];
					if (set == Sets[i] && offset == i) { offset++; continue; }
					Sets[i] = set;
				}
				vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_COMPUTE, DrawCall.base.MatTypeLayout, offset, bindingtablelist_size - offset, Sets, 0, nullptr);
				if (DrawCall.VBuffer != VK_NULL_HANDLE) {
					vkCmdBindVertexBuffers(CB, 0, 1, &DrawCall.VBuffer, &DrawCall.VBOffset);
				}
				vkCmdBindIndexBuffer(CB, DrawCall.IBuffer, DrawCall.IBOffset, DrawCall.IType);
				vkCmdDrawIndexed(CB, DrawCall.IndexCount, DrawCall.InstanceCount, DrawCall.FirstIndex, DrawCall.VOffset, DrawCall.FirstInstance);
			}
			SP.IndexedDrawCalls.clear();
		}

		if (SP.render_dearIMGUI) {
			imgui->Render_toCB(CB);
		}
		if (SP.is_RENDERPASSRELATED) {
			vkCmdNextSubpass(CB, VK_SUBPASS_CONTENTS_INLINE);
		}
	}
	vkCmdEndRenderPass(CB);
}

void Record_BarrierTP(VkCommandBuffer CB, VkPipelineStageFlags srcPipelineStage, RenderGraph::SubTPBARRIER_VK* subpass) {
	VkImageMemoryBarrier* IMBARRIERS = (VkImageMemoryBarrier*)VK_ALLOCATE_AND_GETPTR(VKGLOBAL_VIRMEM_CURRENTFRAME, sizeof(VkImageMemoryBarrier) * subpass->TextureBarriers.size());
	for (unsigned int i = 0; i < subpass->TextureBarriers.size(); i++) {
		const RenderGraph::VK_ImBarrierInfo& info = subpass->TextureBarriers[i];
		VkImageMemoryBarrier& Barrier = IMBARRIERS[i];
		Find_AccessPattern_byIMAGEACCESS(info.lastaccess, Barrier.srcAccessMask, Barrier.oldLayout);
		Find_AccessPattern_byIMAGEACCESS(info.nextaccess, Barrier.dstAccessMask, Barrier.newLayout);
		Barrier.image = info.image;
		Barrier.pNext = nullptr;
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		Barrier.subresourceRange.aspectMask = (info.isColorBit ? VK_IMAGE_ASPECT_COLOR_BIT : 0) | (info.isDepthBit ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) | (info.isStencilBit ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);

		if (!info.targetmip_c) { Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS; }
		else { Barrier.subresourceRange.levelCount = info.targetmip_c; }
		Barrier.subresourceRange.baseMipLevel = info.targetmip_i;

		if (info.cubemapface == cubeface_tgfx_ALL) {
			Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
			Barrier.subresourceRange.baseArrayLayer = 0;
		}
		else {
			Barrier.subresourceRange.layerCount = 1;
			Barrier.subresourceRange.baseArrayLayer = Find_TextureLayer_fromtgfx_cubeface(info.cubemapface);
		}
	}


	VkBufferMemoryBarrier* BUFBARRIERS = (VkBufferMemoryBarrier*)VK_ALLOCATE_AND_GETPTR(VKGLOBAL_VIRMEM_CURRENTFRAME, sizeof(VkBufferMemoryBarrier) * subpass->BufferBarriers.size());
	for (unsigned int i = 0; i < subpass->BufferBarriers.size(); i++) {
		const RenderGraph::VK_BufBarrierInfo& info = subpass->BufferBarriers[i];
		VkBufferMemoryBarrier& Barrier = BUFBARRIERS[i];
		Barrier.buffer = info.buf;
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.offset = info.offset;
		Barrier.pNext = nullptr;
		Barrier.size = info.size;
		Barrier.srcAccessMask = 0;
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	}


	if (srcPipelineStage == 0) {
		srcPipelineStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
	vkCmdPipelineBarrier(CB, srcPipelineStage, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, subpass->TextureBarriers.size(), IMBARRIERS);
}

void Record_CopyTP(VkCommandBuffer CB, RenderGraph::SubTPCOPY_VK* subpass) {
	for (unsigned int i = 0; i < subpass->BUFBUFCopies.size(); i++) {
		RenderGraph::VK_BUFtoBUFinfo& info = subpass->BUFBUFCopies[i];
		vkCmdCopyBuffer(CB, info.SourceBuffer, info.DistanceBuffer, 1, &info.info);
	}

	for (unsigned int i = 0; i < subpass->BUFIMCopies.size(); i++) {
		RenderGraph::VK_BUFtoIMinfo& info = subpass->BUFIMCopies[i];
		vkCmdCopyBufferToImage(CB, info.SourceBuffer, info.TargetImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &info.BufferImageCopy);
	}
	for (unsigned int i = 0; i < subpass->IMIMCopies.size(); i++) {
		RenderGraph::VK_IMtoIMinfo& info = subpass->IMIMCopies[i];
		vkCmdCopyImage(CB, info.SourceTexture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, info.TargetTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &info.info);
	}
}

void Record_ComputePass(VkCommandBuffer CB, RenderGraph::CP_VK* CP) {
	RenderGraph::SubCP_VK* Subpasses = CP->GetSubpasses();
	for (unsigned int SPIndex = 0; SPIndex < CP->SubPassesCount; SPIndex++) {
		RenderGraph::SubCP_VK& SP = Subpasses[SPIndex];
		VkDescriptorSet Sets[VKCONST_MAXDESCSET_PERLIST];
		for (unsigned int DispatchIndex = 0; DispatchIndex < SP.Dispatches.size(); DispatchIndex++) {
			RenderGraph::dispatchcall_vk& dc = SP.Dispatches[DispatchIndex];
			vkCmdBindPipeline(CB, VK_PIPELINE_BIND_POINT_COMPUTE, dc.Pipeline);
			uint32_t offset = 0, bindingtablelist_size = 0;
			for (uint32_t i = 0; i < VKCONST_MAXDESCSET_PERLIST && dc.BINDINGTABLEIDs[i] != UINT16_MAX; i++, bindingtablelist_size++) {
				VkDescriptorSet set = contentmanager->GETDESCSET_ARRAY().getOBJbyINDEX(dc.BINDINGTABLEIDs[i])->Sets[VKGLOBAL_FRAMEINDEX];
				if (set == Sets[i] && offset == i) { offset++; continue; }
				Sets[i] = set;
			}
			vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_COMPUTE, dc.Layout, offset, bindingtablelist_size - offset, Sets, 0, nullptr);
			vkCmdDispatch(CB, dc.DispatchSize.x, dc.DispatchSize.y, dc.DispatchSize.z);
		}
	}
}


extern void WaitForRenderGraphCommandBuffers() {
	unsigned char FrameIndex = renderer->Get_FrameIndex(false);
	//Wait for penultimate frame's rendering operations to end
	fencesys->waitfor_fences(framegraphsys_vk::RENDERING_FENCEs[FrameIndex]);

}
result_tgfx Execute_RenderGraph() {
	unsigned char FrameIndex = renderer->Get_FrameIndex(false);
	commandbuffer_vk* cb = queuesys->get_commandbuffer(rendergpu, rendergpu->GRAPHICSQUEUEFAM(), renderer->Get_FrameIndex(false));
	VkCommandBuffer CB = queuesys->get_commandbufferobj(cb);

	VkCommandBufferBeginInfo cb_bi = {};
	cb_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cb_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cb_bi.pInheritanceInfo = nullptr;
	cb_bi.pNext = nullptr;
	if (vkBeginCommandBuffer(CB, &cb_bi) != VK_SUCCESS) {
		printer(result_tgfx_FAIL, "vkBeginCommandBuffer() has failed!");
		return result_tgfx_FAIL;
	}
	for (unsigned int pass_i = 0; pass_i < framegraphsys_vk::Execution_Order.size(); pass_i++) {
		VK_PassDesc* CurrentPass = framegraphsys_vk::Execution_Order[pass_i];

		//Set barrier before next pass
		VkPipelineStageFlags srcPipelineStage = 0;
		if (pass_i > 0) {
			SetBarrier_BetweenPasses(CB, framegraphsys_vk::Execution_Order[pass_i], framegraphsys_vk::Execution_Order[pass_i - 1], srcPipelineStage);
		}

		switch (CurrentPass->TYPE) {
		case VK_PassDesc::PassType::DP:
			Record_RenderPass(CB, (drawpass_vk*)CurrentPass);
			break;
		case VK_PassDesc::PassType::TP:
		{
			transferpass_vk* TP = (transferpass_vk*)CurrentPass;
			switch (TP->TYPE) {
			case transferpasstype_tgfx_BARRIER:
			{
				VK_TPBarrierDatas* TPDatas = (VK_TPBarrierDatas*)TP->TransferDatas;
				Record_BarrierTP(CB, srcPipelineStage, TPDatas);
			}
			break;
			case transferpasstype_tgfx_COPY:
			{
				VK_TPCopyDatas* TPDATAs = (VK_TPCopyDatas*)TP->TransferDatas;
				Record_CopyTP(CB, TPDATAs);
			}
			break;
			default:
				printer(result_tgfx_NOTCODED, "Recording vulkan commands of this TP type isn't coded yet!");
				return result_tgfx_NOTCODED;
			}
		}
		break;
		case VK_PassDesc::PassType::WP:
			if (pass_i != framegraphsys_vk::Execution_Order.size() - 1) {
				printer(result_tgfx_FAIL, "Primitive Rendergraph supports windows passes only to be in last in the execution order, please fix your execution order!");
			}
			break;
		case VK_PassDesc::PassType::CP:
			Record_ComputePass(CB, (computepass_vk*)CurrentPass);
			break;
		default:
			printer(result_tgfx_NOTCODED, "This pass type isn't coded for recording yet!");
			return result_tgfx_NOTCODED;
		}
	}
	if (vkEndCommandBuffer(CB) != VK_SUCCESS) {
		printer(result_tgfx_FAIL, "vkEndCommandBuffer() has failed!");
		return result_tgfx_FAIL;
	}


	//Before sending render command buffers, wait for presentation of current frame (that has the penultimate frame's content) to end
	//We can avoid tearing this way
	VK_STATICVECTOR<WINDOW_VKOBJ, 50>& windows = core_vk->GETWINDOWs();
	std::vector<fence_idtype_vk> fences;
	for (unsigned char WindowIndex = 0; WindowIndex < windows.size(); WindowIndex++) {
		WINDOW_VKOBJ* VKWINDOW = (WINDOW_VKOBJ*)(windows[WindowIndex]);
		if (!VKWINDOW->isSwapped.load()) { continue; }
		fences.push_back(VKWINDOW->PresentationFences[1]);
	}
	fencesys->waitfor_fences(fences);
	//Destroy semaphores waited by presentation
	for (unsigned char WindowIndex = 0; WindowIndex < windows.size(); WindowIndex++) {
		WINDOW_VKOBJ* VKWINDOW = (WINDOW_VKOBJ*)(windows[WindowIndex]);
		if (!VKWINDOW->isSwapped.load()) { continue; }
		find_and_clear_swapchainfence(VKWINDOW);
		VKWINDOW->isSwapped.store(false);
	}



	//Find how many display queues will be used (to detect how many semaphores render command buffers should signal)
	struct PresentationInfos {
		std::vector<VkSwapchainKHR> swapchains;
		std::vector<uint32_t> image_indexes;
		std::vector<WINDOW_VKOBJ*> windows;
		queuefam_vk* queuefam;
	};
	std::vector<PresentationInfos> PresentationInfo_PerQueue;
	std::vector<semaphore_idtype_vk> SwapchainAcquires;
	//Send displays (primitive rendergraph supports to have only one window pass and it is at the end)
	for (unsigned char i = 0; i < 1; i++) {
		RenderGraph::WP_VK* WP = (RenderGraph::WP_VK*)VKGLOBAL_RG->GetNextPass(nullptr, VK_PASSTYPE::WP);
		if (!WP->WindowCalls.size()) {
			continue;
		}
		for (unsigned char WindowIndex = 0; WindowIndex < WP->WindowCalls.size(); WindowIndex++) {
			WINDOW_VKOBJ* window = core_vk->GETWINDOWs()[WP->WindowCalls[WindowIndex].WindowID];
			SwapchainAcquires.push_back(window->PresentationSemaphores[1]);
			queuefam_vk* window_qfam = window->presentationqueue;
			bool is_found = false;
			for (unsigned char queue_i = 0; queue_i < PresentationInfo_PerQueue.size() && !is_found; queue_i++) {
				PresentationInfos& presentation = PresentationInfo_PerQueue[queue_i];
				if (window_qfam == presentation.queuefam) {
					presentation.image_indexes.push_back(window->CurrentFrameSWPCHNIndex);
					window->CurrentFrameSWPCHNIndex = (window->CurrentFrameSWPCHNIndex + 1) % 2;
					presentation.swapchains.push_back(window->Window_SwapChain);
					presentation.windows.push_back(window);
					is_found = true;
				}
			}
			if (!is_found) {
				PresentationInfo_PerQueue.push_back(PresentationInfos());
				PresentationInfos& presentation = PresentationInfo_PerQueue[PresentationInfo_PerQueue.size() - 1];
				presentation.image_indexes.push_back(window->CurrentFrameSWPCHNIndex);
				window->CurrentFrameSWPCHNIndex = (window->CurrentFrameSWPCHNIndex + 1) % 2;
				presentation.swapchains.push_back(window->Window_SwapChain);
				presentation.windows.push_back(window);
				presentation.queuefam = window_qfam;
			}
		}
		WP->WindowCalls.clear();
	}

	//There is only one command buffer, so we need only one submit
	//It should wait for penultimate frame's display presentation to end
	std::vector<semaphore_idtype_vk> signalSemaphores(PresentationInfo_PerQueue.size());
	for (unsigned char semaphore_i = 0; semaphore_i < signalSemaphores.size(); semaphore_i++) {
		signalSemaphores[semaphore_i] = semaphoresys->Create_Semaphore().get_id();
	}
	VkPipelineStageFlags waitdststagemask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	framegraphsys_vk::RENDERING_FENCEs[FrameIndex] = { queuesys->queueSubmit(rendergpu, rendergpu->GRAPHICSQUEUEFAM(), SwapchainAcquires, signalSemaphores, &CB, &waitdststagemask, 1) };


	if (imgui) { imgui->Render_AdditionalWindows(); }
	//Send displays (primitive rendergraph supports to have only one window pass and it is at the end)
	for (unsigned char i = 0; i < 1; i++) {
		//Fill swapchain and image indices vectors
		for (unsigned char presentation_i = 0; presentation_i < PresentationInfo_PerQueue.size(); presentation_i++) {
			VkResult result;
			VkPresentInfoKHR SwapchainImage_PresentationInfo = {};
			SwapchainImage_PresentationInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			SwapchainImage_PresentationInfo.pNext = nullptr;
			SwapchainImage_PresentationInfo.swapchainCount = PresentationInfo_PerQueue[presentation_i].swapchains.size();
			SwapchainImage_PresentationInfo.pSwapchains = PresentationInfo_PerQueue[presentation_i].swapchains.data();
			SwapchainImage_PresentationInfo.pImageIndices = PresentationInfo_PerQueue[presentation_i].image_indexes.data();
			SwapchainImage_PresentationInfo.pResults = &result;
			SwapchainImage_PresentationInfo.waitSemaphoreCount = signalSemaphores.size();
			std::vector<VkSemaphore> signalSemaphores_vk(signalSemaphores.size());
			for (unsigned int signalSemaphoreIndex = 0; signalSemaphoreIndex < signalSemaphores_vk.size(); signalSemaphoreIndex++) {
				signalSemaphores_vk[signalSemaphoreIndex] = semaphoresys->GetSemaphore_byID(signalSemaphores[signalSemaphoreIndex]).vksemaphore();
			}
			SwapchainImage_PresentationInfo.pWaitSemaphores = signalSemaphores_vk.data();

			framegraphsys_vk::swapchain_fence fence;
			fence.semaphores = signalSemaphores;
			fence.windows = PresentationInfo_PerQueue[presentation_i].windows;
			framegraphsys_vk::swapchain_fences.push_back(fence);
			if (vkQueuePresentKHR(queuesys->get_queue(rendergpu, PresentationInfo_PerQueue[presentation_i].queuefam), &SwapchainImage_PresentationInfo) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "Submitting Presentation Queue has failed!");
				return result_tgfx_FAIL;
			}
		}
	}

	return result_tgfx_SUCCESS;
}