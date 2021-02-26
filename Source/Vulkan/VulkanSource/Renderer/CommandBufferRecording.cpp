#include "Vulkan_Renderer_Core.h"
#include "VK_GPUContentManager.h"
#include "Vulkan/VulkanSource/Renderer/Vulkan_Resource.h"
#include "TuranAPI/Profiler_Core.h"
#include "GFX/GFX_Core.h"
#include "Vulkan/VulkanSource/Vulkan_Core.h"
#define VKContentManager ((Vulkan::GPU_ContentManager*)GFXContentManager)
#define VKGPU (((Vulkan_Core*)GFX)->VK_States.GPU_TO_RENDER)
#define VKRENDERER ((Renderer*)GFX->RENDERER)
#define VKCORE ((Vulkan_Core*)GFX)

// Vulkan Render Command Buffer Recordings
namespace Vulkan {

	void FindBufferOBJ_byBufType(const GFX_API::GFXHandle Handle, GFX_API::BUFFER_TYPE TYPE, VkBuffer& TargetBuffer, VkDeviceSize& TargetOffset);

	void Record_RenderPass(VkCommandBuffer CB, VK_DrawPass* DrawPass) {
		unsigned char FRAMEINDEX = GFXRENDERER->GetCurrentFrameIndex();

		VK_RTSLOTs& CF_SLOTs = DrawPass->SLOTSET->PERFRAME_SLOTSETs[FRAMEINDEX];
		VkRenderPassBeginInfo rp_bi = {};
		rp_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rp_bi.renderPass = DrawPass->RenderPassObject;
		rp_bi.renderArea.extent.width = DrawPass->RenderRegion.Width;
		rp_bi.renderArea.extent.height = DrawPass->RenderRegion.Height;
		rp_bi.renderArea.offset.x = DrawPass->RenderRegion.WidthOffset;
		rp_bi.renderArea.offset.y = DrawPass->RenderRegion.HeightOffset;
		rp_bi.pNext = nullptr;
		rp_bi.pClearValues = nullptr;
		vector<VkClearValue> CLEARVALUEs(CF_SLOTs.COLORSLOTs_COUNT + (CF_SLOTs.DEPTHSTENCIL_SLOT ? 1 : 0));
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
		rp_bi.framebuffer = DrawPass->FBs[GFXRENDERER->GetCurrentFrameIndex()];

		vkCmdBeginRenderPass(CB, &rp_bi, VK_SUBPASS_CONTENTS_INLINE);
		for (unsigned char SubPassIndex = 0; SubPassIndex < DrawPass->Subpass_Count; SubPassIndex++) {
			VK_SubDrawPass& SP = DrawPass->Subpasses[SubPassIndex];
			if (SP.Binding_Index != SubPassIndex) {
				LOG_NOTCODED_TAPI("Subpass Binding Index and the SubDrawPass' element index doesn't match, handle this case!", true);
			}
			if (SP.render_dearIMGUI) {
				VKCORE->VK_IMGUI->Render_toCB(CB);
			}
			//NonIndexed Draw Calls
			{
				std::unique_lock<std::mutex> DrawCallLocker;
				SP.NonIndexedDrawCalls.PauseAllOperations(DrawCallLocker);
				for (unsigned char ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
					for (unsigned char DrawCallIndex = 0; DrawCallIndex < SP.NonIndexedDrawCalls.size(ThreadIndex); DrawCallIndex++) {
						VK_NonIndexedDrawCall& DrawCall = SP.NonIndexedDrawCalls.get(ThreadIndex, DrawCallIndex);
						VkViewport DefaultViewport = {};
						DefaultViewport.height = (float)DrawPass->RenderRegion.Height;
						DefaultViewport.maxDepth = 1.0f;
						DefaultViewport.minDepth = 0.0f;
						DefaultViewport.width = (float)DrawPass->RenderRegion.Width;
						DefaultViewport.x = (float)DrawPass->RenderRegion.WidthOffset;
						DefaultViewport.y = (float)DrawPass->RenderRegion.HeightOffset;
						vkCmdSetViewport(CB, 0, 1, &DefaultViewport);
						VkRect2D Scissor;
						Scissor.extent.height = DrawPass->RenderRegion.Height;
						Scissor.extent.width = DrawPass->RenderRegion.Width;
						Scissor.offset.x = DrawPass->RenderRegion.WidthOffset;
						Scissor.offset.y = DrawPass->RenderRegion.HeightOffset;
						vkCmdSetScissor(CB, 0, 1, &Scissor);


						vkCmdBindPipeline(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeObj);
						if (*DrawCall.PerInstanceSet) {
							if (*DrawCall.GeneralSet) {
								VkDescriptorSet Sets[3] = { VKContentManager->GlobalBuffers_DescSet, *DrawCall.GeneralSet, *DrawCall.PerInstanceSet };
								vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 3, Sets, 0, nullptr);
							}
							else {
								VkDescriptorSet Sets[2] = { VKContentManager->GlobalBuffers_DescSet, *DrawCall.PerInstanceSet };
								vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 2, Sets, 0, nullptr);
							}
						}
						else if (*DrawCall.GeneralSet) {
							VkDescriptorSet Sets[2] = { VKContentManager->GlobalBuffers_DescSet, *DrawCall.GeneralSet };
							vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 2, Sets, 0, nullptr);
						}
						else {
							VkDescriptorSet Sets[1] = { VKContentManager->GlobalBuffers_DescSet };
							vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 1, Sets, 0, nullptr);
						}
						if (DrawCall.VBuffer != VK_NULL_HANDLE) {
							vkCmdBindVertexBuffers(CB, 0, 1, &DrawCall.VBuffer, &DrawCall.VOffset);
						}
						vkCmdDraw(CB, DrawCall.VertexCount, DrawCall.InstanceCount, DrawCall.FirstVertex, DrawCall.FirstInstance);
					}
					SP.NonIndexedDrawCalls.clear(GFX->JobSys->GetThisThreadIndex());
				}
			}
			//Indexed Draw Calls
			{
				std::unique_lock<std::mutex> DrawCallLocker;
				SP.IndexedDrawCalls.PauseAllOperations(DrawCallLocker);
				for (unsigned char ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
					for (unsigned char DrawCallIndex = 0; DrawCallIndex < SP.IndexedDrawCalls.size(ThreadIndex); DrawCallIndex++) {
						VK_IndexedDrawCall& DrawCall = SP.IndexedDrawCalls.get(ThreadIndex, DrawCallIndex);
						VkViewport DefaultViewport = {};
						DefaultViewport.height = (float)DrawPass->RenderRegion.Height;
						DefaultViewport.maxDepth = 1.0f;
						DefaultViewport.minDepth = 0.0f;
						DefaultViewport.width = (float)DrawPass->RenderRegion.Width;
						DefaultViewport.x = (float)DrawPass->RenderRegion.WidthOffset;
						DefaultViewport.y = (float)DrawPass->RenderRegion.HeightOffset;
						vkCmdSetViewport(CB, 0, 1, &DefaultViewport);
						VkRect2D Scissor;
						Scissor.extent.height = DrawPass->RenderRegion.Height;
						Scissor.extent.width = DrawPass->RenderRegion.Width;
						Scissor.offset.x = DrawPass->RenderRegion.WidthOffset;
						Scissor.offset.y = DrawPass->RenderRegion.HeightOffset;
						vkCmdSetScissor(CB, 0, 1, &Scissor);

						vkCmdBindPipeline(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeObj);
						if (*DrawCall.PerInstanceSet) {
							if (*DrawCall.GeneralSet) {
								VkDescriptorSet Sets[3] = { VKContentManager->GlobalBuffers_DescSet, *DrawCall.GeneralSet, *DrawCall.PerInstanceSet };
								vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 3, Sets, 0, nullptr);
							}
							else {
								vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 1, &VKContentManager->GlobalBuffers_DescSet, 0, nullptr);
								vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 2, 1, DrawCall.PerInstanceSet, 0, nullptr);
							}
						}
						else if(*DrawCall.GeneralSet){
							VkDescriptorSet Sets[2] = { VKContentManager->GlobalBuffers_DescSet, *DrawCall.GeneralSet };
							vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 2, Sets, 0, nullptr);
						}
						else {
							vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 1, &VKContentManager->GlobalBuffers_DescSet, 0, nullptr);
						}
						if (DrawCall.VBuffer != VK_NULL_HANDLE) {
							vkCmdBindVertexBuffers(CB, 0, 1, &DrawCall.VBuffer, &DrawCall.VBOffset);
						}
						vkCmdBindIndexBuffer(CB, DrawCall.IBuffer, DrawCall.IBOffset, DrawCall.IType);
						vkCmdDrawIndexed(CB, DrawCall.IndexCount, DrawCall.InstanceCount, DrawCall.FirstIndex, DrawCall.VOffset, DrawCall.FirstInstance);
					}
					SP.IndexedDrawCalls.clear(GFX->JobSys->GetThisThreadIndex());
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
		default:
			LOG_NOTCODED_TAPI("This pass type isn't coded for recording yet!", true);
		}
	}

	void Record_BarrierTP(VkCommandBuffer CB, VkPipelineStageFlags srcPipelineStage, VK_TPBarrierDatas* DATAs) {
		std::unique_lock<std::mutex> Locker;
		DATAs->TextureBarriers.PauseAllOperations(Locker);
		vector<VkImageMemoryBarrier> IMBARRIES;
		unsigned int LatestIndex = 0;
		for (unsigned char ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
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
			for (unsigned char ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
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
			for (unsigned char ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
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
			for (unsigned char ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
				for (unsigned int i = 0; i < DATAs->IMIMCopies.size(ThreadIndex); i++) {
					VK_IMtoIMinfo& info = DATAs->IMIMCopies.get(ThreadIndex, i);
					vkCmdCopyImage(CB, info.SourceTexture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, info.TargetTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &info.info);
					LastElementIndex++;
				}
				DATAs->IMIMCopies.clear(ThreadIndex);
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
				case GFX_API::TRANFERPASS_TYPE::TP_BARRIER:
				{
					VK_TPBarrierDatas* TPDatas = GFXHandleConverter(VK_TPBarrierDatas*, TP->TransferDatas);
					Record_BarrierTP(CB, srcPipelineStage, TPDatas);
				}
				break;
				case GFX_API::TRANFERPASS_TYPE::TP_COPY:
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
			default:
				LOG_NOTCODED_TAPI("This pass type isn't coded for recording yet!", true);
			}

			LastPass = CurrentPass;
			PassElementIndex++;
		}
	}
}