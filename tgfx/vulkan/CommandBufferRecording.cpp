#include "Vulkan_Renderer_Core.h"
#include "VK_GPUContentManager.h"
#include "Vulkan_Resource.h"
#include <TAPI/Profiler_Core.h>
#include <TGFX/GFX_Core.h>
#include "Vulkan_Core.h"
#include "FGAlgorithm_TempDatas.h"
#define LOG VKCORE->TGFXCORE.DebugCallback

// Vulkan Render Command Buffer Recordings
void FindBufferOBJ_byBufType(const tgfx_buffer Handle, tgfx_buffertype TYPE, VkBuffer& TargetBuffer, VkDeviceSize& TargetOffset);

void Record_RenderPass(VkCommandBuffer CB, VK_DrawPass* DrawPass) {
	unsigned char FRAMEINDEX = VKRENDERER->GetCurrentFrameIndex();

	VK_RTSLOTs& CF_SLOTs = DrawPass->SLOTSET->PERFRAME_SLOTSETs[FRAMEINDEX];
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
	rp_bi.framebuffer = DrawPass->FBs[VKRENDERER->GetCurrentFrameIndex()];

	vkCmdBeginRenderPass(CB, &rp_bi, VK_SUBPASS_CONTENTS_INLINE);
	for (unsigned char SubPassIndex = 0; SubPassIndex < DrawPass->Subpass_Count; SubPassIndex++) {
		VK_SubDrawPass& SP = DrawPass->Subpasses[SubPassIndex];
		if (SP.Binding_Index != SubPassIndex) {
			LOG(tgfx_result_FAIL, "Subpass Binding Index and the SubDrawPass' element index doesn't match, handle this case!");
		}
		if (SP.render_dearIMGUI) {
			VK_IMGUI->Render_toCB(CB);
		}
		//NonIndexed Draw Calls
		{
			std::unique_lock<std::mutex> DrawCallLocker;
			SP.NonIndexedDrawCalls.PauseAllOperations(DrawCallLocker);
			for (unsigned char ThreadIndex = 0; ThreadIndex < tapi_GetThreadCount(JobSys); ThreadIndex++) {
				for (unsigned char DrawCallIndex = 0; DrawCallIndex < SP.NonIndexedDrawCalls.size(ThreadIndex); DrawCallIndex++) {
					VK_NonIndexedDrawCall& DrawCall = SP.NonIndexedDrawCalls.get(ThreadIndex, DrawCallIndex);
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


					vkCmdBindPipeline(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeObj);
					if (*DrawCall.PerInstanceSet) {
						if (*DrawCall.GeneralSet) {
							VkDescriptorSet Sets[4] = { VKContentManager->GlobalBuffers_DescSet.Set, VKContentManager->GlobalTextures_DescSet.Set,
								*DrawCall.GeneralSet, *DrawCall.PerInstanceSet };
							vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 4, Sets, 0, nullptr);
						}
						else {
							VkDescriptorSet Sets[3] = { VKContentManager->GlobalBuffers_DescSet.Set, VKContentManager->GlobalTextures_DescSet.Set,
								*DrawCall.PerInstanceSet };
							vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 3, Sets, 0, nullptr);
						}
					}
					else if (*DrawCall.GeneralSet) {
						VkDescriptorSet Sets[3] = { VKContentManager->GlobalBuffers_DescSet.Set, VKContentManager->GlobalTextures_DescSet.Set,
								*DrawCall.GeneralSet };
						vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 3, Sets, 0, nullptr);
					}
					else {
						VkDescriptorSet Sets[2] = { VKContentManager->GlobalBuffers_DescSet.Set, VKContentManager->GlobalTextures_DescSet.Set };
						vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 2, Sets, 0, nullptr);
					}
					if (DrawCall.VBuffer != VK_NULL_HANDLE) {
						vkCmdBindVertexBuffers(CB, 0, 1, &DrawCall.VBuffer, &DrawCall.VOffset);
					}
					vkCmdDraw(CB, DrawCall.VertexCount, DrawCall.InstanceCount, DrawCall.FirstVertex, DrawCall.FirstInstance);
				}
				SP.NonIndexedDrawCalls.clear(tapi_GetThisThreadIndex(JobSys));
			}
		}
		//Indexed Draw Calls
		{
			std::unique_lock<std::mutex> DrawCallLocker;
			SP.IndexedDrawCalls.PauseAllOperations(DrawCallLocker);
			for (unsigned char ThreadIndex = 0; ThreadIndex < tapi_GetThreadCount(JobSys); ThreadIndex++) {
				for (unsigned char DrawCallIndex = 0; DrawCallIndex < SP.IndexedDrawCalls.size(ThreadIndex); DrawCallIndex++) {
					VK_IndexedDrawCall& DrawCall = SP.IndexedDrawCalls.get(ThreadIndex, DrawCallIndex);
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

					vkCmdBindPipeline(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeObj);
					if (*DrawCall.PerInstanceSet) {
						if (*DrawCall.GeneralSet) {
							VkDescriptorSet Sets[4] = { VKContentManager->GlobalBuffers_DescSet.Set, VKContentManager->GlobalTextures_DescSet.Set,
								*DrawCall.GeneralSet, *DrawCall.PerInstanceSet };
							vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 4, Sets, 0, nullptr);
						}
						else {
							VkDescriptorSet Sets[3] = { VKContentManager->GlobalBuffers_DescSet.Set, VKContentManager->GlobalTextures_DescSet.Set,
								*DrawCall.PerInstanceSet };
							vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 3, Sets, 0, nullptr);
						}
					}
					else if (*DrawCall.GeneralSet) {
						VkDescriptorSet Sets[3] = { VKContentManager->GlobalBuffers_DescSet.Set, VKContentManager->GlobalTextures_DescSet.Set,
								*DrawCall.GeneralSet };
						vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 3, Sets, 0, nullptr);
					}
					else {
						VkDescriptorSet Sets[2] = { VKContentManager->GlobalBuffers_DescSet.Set, VKContentManager->GlobalTextures_DescSet.Set };
						vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 2, Sets, 0, nullptr);
					}
					if (DrawCall.VBuffer != VK_NULL_HANDLE) {
						vkCmdBindVertexBuffers(CB, 0, 1, &DrawCall.VBuffer, &DrawCall.VBOffset);
					}
					vkCmdBindIndexBuffer(CB, DrawCall.IBuffer, DrawCall.IBOffset, DrawCall.IType);
					vkCmdDrawIndexed(CB, DrawCall.IndexCount, DrawCall.InstanceCount, DrawCall.FirstIndex, DrawCall.VOffset, DrawCall.FirstInstance);
				}
				SP.IndexedDrawCalls.clear(tapi_GetThisThreadIndex(JobSys));
			}
		}

		if (SubPassIndex < DrawPass->Subpass_Count - 1) {
			vkCmdNextSubpass(CB, VK_SUBPASS_CONTENTS_INLINE);
		}
	}
	vkCmdEndRenderPass(CB);
}


void SetBarrier_BetweenBranchPasses(VkCommandBuffer CB, VK_BranchPass* LastPass, VK_BranchPass* CurrentPass, VkPipelineStageFlags& srcPipelineStage) {
	switch (CurrentPass->TYPE) {
	case PassType::DP:
	{
		VK_DrawPass* DP = GFXHandleConverter(VK_DrawPass*, CurrentPass->Handle);
		bool isdependencyfound = false;
		for (unsigned char WaitIndex = 0; WaitIndex < DP->WAITs.size(); WaitIndex++) {
			if (*DP->WAITs[WaitIndex].WaitedPass == LastPass->Handle) {
				isdependencyfound = true;
				vkCmdPipelineBarrier(CB, Find_VkPipelineStages(DP->WAITs[WaitIndex].WaitedStage), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
					0, nullptr, 0, nullptr, 0, nullptr);
				break;
			}
		}

		if (!isdependencyfound) {
			switch (LastPass->TYPE) {
			case PassType::DP:
				vkCmdPipelineBarrier(CB, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
					0, nullptr, 0, nullptr, 0, nullptr);
				break;
			case PassType::TP:
				vkCmdPipelineBarrier(CB, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
					0, nullptr, 0, nullptr, 0, nullptr);
				break;
			default:
				LOG_NOTCODED_TAPI("This type of barrier isn't supported to set in wait barrier for now!", true);
			}
		}
	}
	break;
	case PassType::TP:
	{
		VK_TransferPass* TP = GFXHandleConverter(VK_TransferPass*, CurrentPass->Handle);
		bool isdependencyfound = false;
		for (unsigned char WaitIndex = 0; WaitIndex < TP->WAITs.size(); WaitIndex++) {
			if (*TP->WAITs[WaitIndex].WaitedPass == LastPass->Handle) {
				isdependencyfound = true;
				srcPipelineStage = Find_VkShaderStages(TP->WAITs[WaitIndex].WaitedStage);
				break;
			}
		}

		if (!isdependencyfound) {
			switch (LastPass->TYPE) {
			case PassType::DP:
				srcPipelineStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
				break;
			case PassType::TP:
				srcPipelineStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
				break;
			default:
				LOG_NOTCODED_TAPI("This type of barrier isn't supported to set in wait barrier for now!", true);
			}
		}
	}
	break;
	case PassType::CP:
	{
		VK_ComputePass* CP = GFXHandleConverter(VK_ComputePass*, CurrentPass->Handle);
		bool isdependencyfound = false;
		for (unsigned char WaitIndex = 0; WaitIndex < CP->WAITs.size(); WaitIndex++) {
			if (*CP->WAITs[WaitIndex].WaitedPass == LastPass->Handle) {
				isdependencyfound = true;
				vkCmdPipelineBarrier(CB, Find_VkPipelineStages(CP->WAITs[WaitIndex].WaitedStage), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
					0, nullptr, 0, nullptr, 0, nullptr);
				break;
			}
		}

		if (!isdependencyfound) {
			switch (LastPass->TYPE) {
			case PassType::DP:
				vkCmdPipelineBarrier(CB, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
					0, nullptr, 0, nullptr, 0, nullptr);
				break;
			case PassType::TP:
				vkCmdPipelineBarrier(CB, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
					0, nullptr, 0, nullptr, 0, nullptr);
				break;
			case PassType::CP:
				vkCmdPipelineBarrier(CB, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
					0, nullptr, 0, nullptr, 0, nullptr);
				break;
			default:
				LOG_NOTCODED_TAPI("This type of barrier isn't supported to set in wait barrier for now!", true);
			}
		}
	}
	break;
	default:
		LOG_NOTCODED_TAPI("This pass type isn't coded for recording yet!", true);
	}
}

void Record_BarrierTP(VkCommandBuffer CB, VkPipelineStageFlags srcPipelineStage, VK_TPBarrierDatas* DATAs) {
	std::unique_lock<std::mutex> Locker;
	DATAs->TextureBarriers.PauseAllOperations(Locker);
	std::vector<VkImageMemoryBarrier> IMBARRIES;
	unsigned int LatestIndex = 0;
	for (unsigned char ThreadIndex = 0; ThreadIndex < tapi_GetThreadCount(JobSys); ThreadIndex++) {
		IMBARRIES.resize(IMBARRIES.size() + DATAs->TextureBarriers.size(ThreadIndex));
		for (unsigned int i = 0; i < DATAs->TextureBarriers.size(ThreadIndex); i++) {
			VK_ImBarrierInfo& info = DATAs->TextureBarriers.get(ThreadIndex, i);
			VkImageMemoryBarrier& Barrier = IMBARRIES[LatestIndex];
			Barrier = info.Barrier;
			LatestIndex++;
		}
		DATAs->TextureBarriers.clear(ThreadIndex);
	}
	if (srcPipelineStage == 0) {
		srcPipelineStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
	vkCmdPipelineBarrier(CB, srcPipelineStage, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, IMBARRIES.size(), IMBARRIES.data());
}

void Record_CopyTP(VkCommandBuffer CB, VK_TPCopyDatas* DATAs) {
	{
		std::unique_lock<std::mutex> BUFBUFLocker;
		DATAs->BUFBUFCopies.PauseAllOperations(BUFBUFLocker);
		unsigned int LastElementIndex = 0;
		for (unsigned char ThreadIndex = 0; ThreadIndex < tapi_GetThreadCount(JobSys); ThreadIndex++) {
			for (unsigned int i = 0; i < DATAs->BUFBUFCopies.size(ThreadIndex); i++) {
				VK_BUFtoBUFinfo& info = DATAs->BUFBUFCopies.get(ThreadIndex, i);
				vkCmdCopyBuffer(CB, info.SourceBuffer, info.DistanceBuffer, 1, &info.info);
				LastElementIndex++;
			}
			DATAs->BUFBUFCopies.clear(ThreadIndex);
		}
	}

	{
		std::unique_lock<std::mutex> BUFIMLocker;
		DATAs->BUFIMCopies.PauseAllOperations(BUFIMLocker);
		unsigned int LastElementIndex = 0;
		for (unsigned char ThreadIndex = 0; ThreadIndex < tapi_GetThreadCount(JobSys); ThreadIndex++) {
			for (unsigned int i = 0; i < DATAs->BUFIMCopies.size(ThreadIndex); i++) {
				VK_BUFtoIMinfo& info = DATAs->BUFIMCopies.get(ThreadIndex, i);
				vkCmdCopyBufferToImage(CB, info.SourceBuffer, info.TargetImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &info.BufferImageCopy);
				LastElementIndex++;
			}
			DATAs->BUFIMCopies.clear(ThreadIndex);
		}
	}
	{
		std::unique_lock<std::mutex> IMIMLocker;
		DATAs->IMIMCopies.PauseAllOperations(IMIMLocker);
		unsigned int LastElementIndex = 0;
		for (unsigned char ThreadIndex = 0; ThreadIndex < tapi_GetThreadCount(JobSys); ThreadIndex++) {
			for (unsigned int i = 0; i < DATAs->IMIMCopies.size(ThreadIndex); i++) {
				VK_IMtoIMinfo& info = DATAs->IMIMCopies.get(ThreadIndex, i);
				vkCmdCopyImage(CB, info.SourceTexture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, info.TargetTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &info.info);
				LastElementIndex++;
			}
			DATAs->IMIMCopies.clear(ThreadIndex);
		}
	}
}

void Record_ComputePass(VkCommandBuffer CB, VK_ComputePass* CP) {
	for (unsigned int SPIndex = 0; SPIndex < CP->Subpasses.size(); SPIndex++) {
		VK_SubComputePass& SP = CP->Subpasses[SPIndex];
		std::unique_lock<std::mutex> Locker;
		SP.Dispatches.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int DispatchIndex = 0; DispatchIndex < SP.Dispatches.size(ThreadID); DispatchIndex++) {
				VK_DispatchCall& dc = SP.Dispatches.get(ThreadID, DispatchIndex);
				vkCmdBindPipeline(CB, VK_PIPELINE_BIND_POINT_COMPUTE, dc.Pipeline);
				if (*dc.InstanceSet) {
					if (*dc.GeneralSet) {
						VkDescriptorSet Sets[4] = { VKContentManager->GlobalBuffers_DescSet.Set, VKContentManager->GlobalTextures_DescSet.Set,
							*dc.GeneralSet, *dc.InstanceSet };
						vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_COMPUTE, dc.Layout, 0, 4, Sets, 0, nullptr);
					}
					else {
						VkDescriptorSet Sets[3] = { VKContentManager->GlobalBuffers_DescSet.Set, VKContentManager->GlobalTextures_DescSet.Set,
							*dc.InstanceSet };
						vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_COMPUTE, dc.Layout, 0, 3, Sets, 0, nullptr);
					}
				}
				else if (*dc.GeneralSet) {
					VkDescriptorSet Sets[3] = { VKContentManager->GlobalBuffers_DescSet.Set, VKContentManager->GlobalTextures_DescSet.Set,
							*dc.GeneralSet };
					vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_COMPUTE, dc.Layout, 0, 3, Sets, 0, nullptr);
				}
				else {
					VkDescriptorSet Sets[2] = { VKContentManager->GlobalBuffers_DescSet.Set, VKContentManager->GlobalTextures_DescSet.Set };
					vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_COMPUTE, dc.Layout, 0, 2, Sets, 0, nullptr);
				}
				vkCmdDispatch(CB, dc.DispatchSize.x, dc.DispatchSize.y, dc.DispatchSize.z);
			}
			SP.Dispatches.clear(ThreadID);
		}
		{
			std::vector<VkBufferMemoryBarrier> Buf_MBs;
			std::unique_lock<std::mutex> BufferLocker;
			SP.Barriers_AfterSubpassExecutions.BufferBarriers.PauseAllOperations(BufferLocker);
			std::vector<VkImageMemoryBarrier> Im_MBs;
			std::unique_lock<std::mutex> ImLocker;
			SP.Barriers_AfterSubpassExecutions.TextureBarriers.PauseAllOperations(ImLocker);
			for (unsigned int ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
				for (unsigned int BarrierIndex = 0; BarrierIndex < SP.Barriers_AfterSubpassExecutions.BufferBarriers.size(ThreadID); BarrierIndex++) {
					VK_BufBarrierInfo& buf_i = SP.Barriers_AfterSubpassExecutions.BufferBarriers.get(ThreadID, BarrierIndex);
					Buf_MBs.push_back(buf_i.Barrier);
				}
				for (unsigned int BarrierIndex = 0; BarrierIndex < SP.Barriers_AfterSubpassExecutions.TextureBarriers.size(ThreadID); BarrierIndex++) {
					VK_ImBarrierInfo& im_i = SP.Barriers_AfterSubpassExecutions.TextureBarriers.get(ThreadID, BarrierIndex);
					Im_MBs.push_back(im_i.Barrier);
				}
			}
			vkCmdPipelineBarrier(CB, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr,
				Buf_MBs.size(), Buf_MBs.data(), Im_MBs.size(), Im_MBs.data());
		}
	}
}

void RecordRGBranchCalls(VK_RGBranch& Branch, VkCommandBuffer CB) {
	unsigned char PassElementIndex = 0;
	VK_BranchPass* LastPass = nullptr;
	while (PassElementIndex < Branch.PassCount) {
		VK_BranchPass* CurrentPass = nullptr;
		for (PassElementIndex; PassElementIndex < Branch.PassCount; PassElementIndex++) {
			if (Branch.CurrentFramePassesIndexes[PassElementIndex]) {
				CurrentPass = &Branch.CorePasses[Branch.CurrentFramePassesIndexes[PassElementIndex] - 1];
				break;
			}
		}
		if (!CurrentPass) {
			return;
		}

		VkPipelineStageFlags srcPipelineStage = 0;
		//Set barriers between passes
		if (LastPass) {
			SetBarrier_BetweenBranchPasses(CB, LastPass, CurrentPass, srcPipelineStage);
		}

		switch (CurrentPass->TYPE) {
		case PassType::DP:
			Record_RenderPass(CB, GFXHandleConverter(VK_DrawPass*, CurrentPass->Handle));
			break;
		case PassType::TP:
		{
			VK_TransferPass* TP = GFXHandleConverter(VK_TransferPass*, CurrentPass->Handle);
			switch (TP->TYPE) {
			case tgfx_transferpass_type_BARRIER:
			{
				VK_TPBarrierDatas* TPDatas = GFXHandleConverter(VK_TPBarrierDatas*, TP->TransferDatas);
				Record_BarrierTP(CB, srcPipelineStage, TPDatas);
			}
			break;
			case tgfx_transferpass_type_COPY:
			{
				VK_TPCopyDatas* TPDATAs = GFXHandleConverter(VK_TPCopyDatas*, TP->TransferDatas);
				Record_CopyTP(CB, TPDATAs);
			}
			break;
			default:
				LOG_NOTCODED_TAPI("Recording vulkan commands of this TP type isn't coded yet!", true);
			}
		}
		break;
		case PassType::WP:
			LOG_CRASHING_TAPI("This shouldn't happen, it should've been returned early!");
			break;
		case PassType::CP:
			Record_ComputePass(CB, GFXHandleConverter(VK_ComputePass*, CurrentPass->Handle));
			break;
		default:
			LOG_NOTCODED_TAPI("This pass type isn't coded for recording yet!", true);
		}

		LastPass = CurrentPass;
		PassElementIndex++;
	}
}
