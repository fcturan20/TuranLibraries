//This is to test other backend functions with minimum rendergraph complexity
#include "renderer.h"
#include "gpucontentmanager.h"
#include "imgui_vktgfx.h"
#include "resource.h"
#include <tgfx_forwarddeclarations.h>
#include "queue.h"
#include "core.h"
#include "synchronization_sys.h"

void Start_RenderGraphConstruction() {
	//Apply resource changes made by user
	contentmanager->Resource_Finalizations();

	if (renderer->RGSTATUS() == RenderGraphStatus::FinishConstructionCalled || renderer->RGSTATUS() == RenderGraphStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "GFXRENDERER->Start_RenderGraphCreation() has failed because you have wrong function call order!"); return;
	}
	renderer->RG_Status = RenderGraphStatus::StartedConstruction;
}

struct framegraphsys_vk {
	static std::vector<VK_Pass*> VKPASSes;
	static std::vector<VK_Pass*> Execution_Order;
public:
	inline static bool framegraphsys_vk::Check_WaitHandles() {
		for (unsigned int DPIndex = 0; DPIndex < renderer->DrawPasses.size(); DPIndex++) {
			drawpass_vk* DP = renderer->DrawPasses[DPIndex];
			for (unsigned int WaitIndex = 0; WaitIndex < DP->base_data.WAITsCOUNT; WaitIndex++) {
				VK_Pass::WaitDescription& Wait_desc = DP->base_data.WAITs[WaitIndex];
				if ((*Wait_desc.WaitedPass) == nullptr) {
					printer(result_tgfx_FAIL, "You forgot to set wait handle of one of the draw passes!");
					return false;
				}

				//Search through all pass types
				switch ((*Wait_desc.WaitedPass)->TYPE) {
				case VK_Pass::PassType::DP:
				{
					bool is_Found = false;
					for (unsigned int CheckedDP = 0; CheckedDP < renderer->DrawPasses.size(); CheckedDP++) {
						if (*Wait_desc.WaitedPass == &renderer->DrawPasses[CheckedDP]->base_data) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						printer(result_tgfx_FAIL, "One of the draw passes waits for an draw pass but given pass isn't found!");
						return false;
					}
				}
				break;
				case VK_Pass::PassType::TP:
				{
					bool is_Found = false;
					for (unsigned int CheckedTP = 0; CheckedTP < renderer->TransferPasses.size(); CheckedTP++) {
						if (*Wait_desc.WaitedPass == &renderer->TransferPasses[CheckedTP]->base_data) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						printer(result_tgfx_FAIL, "One of the draw passes waits for an transfer pass but given pass isn't found!");
						return false;
					}
				}
				break;
				case VK_Pass::PassType::WP:
				{
					bool is_Found = false;
					for (unsigned int CheckedWP = 0; CheckedWP < renderer->WindowPasses.size(); CheckedWP++) {
						if (*Wait_desc.WaitedPass == &renderer->WindowPasses[CheckedWP]->base_data) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						printer(result_tgfx_FAIL, "One of the draw passes waits for an window pass but given pass isn't found!");
						return false;
					}
				}
				break;
				case VK_Pass::PassType::CP:
				{
					bool is_Found = false;
					for (unsigned int CheckedCP = 0; CheckedCP < renderer->ComputePasses.size(); CheckedCP++) {
						if (*Wait_desc.WaitedPass == &renderer->ComputePasses[CheckedCP]->base_data) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						printer(result_tgfx_FAIL, "One of the draw passes waits for a compute pass but given pass isn't found!");
						return false;
					}
				}
				break;
				case VK_Pass::PassType::INVALID:
				{
					printer(result_tgfx_FAIL, "Finding a proper TPType has failed, so Check Wait has failed!");
					return false;
				}
				default:
					printer(result_tgfx_FAIL, "Finding a Waited TP Type has failed, so Check Wait Handle has failed!");
					return false;
				}
			}
		}
		for (unsigned int WPIndex = 0; WPIndex < renderer->WindowPasses.size(); WPIndex++) {
			windowpass_vk* WP = ((windowpass_vk*)renderer->WindowPasses[WPIndex]);
			for (unsigned int WaitIndex = 0; WaitIndex < WP->base_data.WAITsCOUNT; WaitIndex++) {
				VK_Pass::WaitDescription& Wait_desc = WP->base_data.WAITs[WaitIndex];
				if (!(*Wait_desc.WaitedPass)) {
					printer(result_tgfx_FAIL, "You forgot to set wait handle of one of the window passes!");
					return false;
				}

				switch ((*Wait_desc.WaitedPass)->TYPE) {
				case VK_Pass::PassType::DP:
				{
					bool is_Found = false;
					for (unsigned int CheckedDP = 0; CheckedDP < renderer->DrawPasses.size(); CheckedDP++) {
						if (*Wait_desc.WaitedPass == &renderer->DrawPasses[CheckedDP]->base_data) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						printer(result_tgfx_FAIL, "One of the window passes waits for an draw pass but given pass isn't found!");
						return false;
					}
				}
				break;
				case VK_Pass::PassType::TP:
				{
					transferpass_vk* currentTP = ((transferpass_vk*)*Wait_desc.WaitedPass);
					bool is_Found = false;
					for (unsigned int CheckedTP = 0; CheckedTP < renderer->TransferPasses.size(); CheckedTP++) {
						if (currentTP == renderer->TransferPasses[CheckedTP]) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						printer(result_tgfx_FAIL, "One of the window passes waits for an transfer pass but given pass isn't found!");
						return false;
					}
				}
				break;
				case VK_Pass::PassType::CP:
				{
					bool is_Found = false;
					for (unsigned int CheckedCP = 0; CheckedCP < renderer->ComputePasses.size(); CheckedCP++) {
						if (*Wait_desc.WaitedPass == &renderer->ComputePasses[CheckedCP]->base_data) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						printer(result_tgfx_FAIL, "One of the window passes waits for a compute pass but given pass isn't found!");
						return false;
					}
				}
				break;
				case VK_Pass::PassType::WP:
					printer(result_tgfx_FAIL, "A window pass can't wait for another window pass, Check Wait Handle failed!");
					return false;
				case VK_Pass::PassType::INVALID:
					printer(result_tgfx_FAIL, "Finding a TPType has failed, so Check Wait Handle too!");
					return false;
				default:
					printer(result_tgfx_FAIL, "TP Type is not supported for now, so Check Wait Handle failed too!");
					return false;
				}
			}
		}
		for (unsigned int TPIndex = 0; TPIndex < renderer->TransferPasses.size(); TPIndex++) {
			transferpass_vk* TP = renderer->TransferPasses[TPIndex];
			for (unsigned int WaitIndex = 0; WaitIndex < TP->base_data.WAITsCOUNT; WaitIndex++) {
				VK_Pass::WaitDescription& Wait_desc = TP->base_data.WAITs[WaitIndex];
				if (!(*Wait_desc.WaitedPass)) {
					printer(result_tgfx_FAIL, "You forgot to set wait handle of one of the transfer passes!");
					return false;
				}

				switch ((*Wait_desc.WaitedPass)->TYPE) {
				case VK_Pass::PassType::DP:
				{
					bool is_Found = false;
					for (unsigned int CheckedDP = 0; CheckedDP < renderer->DrawPasses.size(); CheckedDP++) {
						if (*Wait_desc.WaitedPass == &renderer->DrawPasses[CheckedDP]->base_data) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						std::cout << TP->base_data.NAME << std::endl;
						printer(result_tgfx_FAIL, "One of the transfer passes waits for an draw pass but given pass isn't found!");
						return false;
					}
				}
				break;
				case VK_Pass::PassType::TP:
				{
					transferpass_vk* currentTP = ((transferpass_vk*)*Wait_desc.WaitedPass);
					bool is_Found = false;
					for (unsigned int CheckedTP = 0; CheckedTP < renderer->TransferPasses.size(); CheckedTP++) {
						if (currentTP == renderer->TransferPasses[CheckedTP]) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						printer(result_tgfx_FAIL, "One of the transfer passes waits for an transfer pass but given pass isn't found!");
						return false;
					}
				}
				break;
				case VK_Pass::PassType::WP:
				{
					bool is_Found = false;
					for (unsigned int CheckedWP = 0; CheckedWP < renderer->WindowPasses.size(); CheckedWP++) {
						if (*Wait_desc.WaitedPass == &renderer->WindowPasses[CheckedWP]->base_data) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						printer(result_tgfx_FAIL, "One of the transfer passes waits for an window pass but given pass isn't found!");
						return false;
					}
				}
				break;
				case VK_Pass::PassType::CP:
				{
					bool is_Found = false;
					for (unsigned int CheckedCP = 0; CheckedCP < renderer->ComputePasses.size(); CheckedCP++) {
						if (*Wait_desc.WaitedPass == &renderer->ComputePasses[CheckedCP]->base_data) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						printer(result_tgfx_FAIL, "One of the transfer passes waits for a compute pass but given pass isn't found!");
						return false;
					}
				}
				break;
				case VK_Pass::PassType::INVALID:
				{
					printer(result_tgfx_FAIL, "Finding a TPType has failed, so Check Wait Handle too!");
					return false;
				}
				default:
				{
					printer(result_tgfx_FAIL, "TP Type is not supported for now, so Check Wait Handle failed too!");
					return false;
				}
				}

			}
		}
		for (unsigned int CPIndex = 0; CPIndex < renderer->ComputePasses.size(); CPIndex++) {
			computepass_vk* CP = renderer->ComputePasses[CPIndex];
			for (unsigned int WaitIndex = 0; WaitIndex < CP->base_data.WAITsCOUNT; WaitIndex++) {
				VK_Pass::WaitDescription& Wait_desc = CP->base_data.WAITs[WaitIndex];
				if (!(*Wait_desc.WaitedPass)) {
					printer(result_tgfx_FAIL, "You forgot to set wait handle of one of the compute passes!");
					return false;
				}

				switch ((*Wait_desc.WaitedPass)->TYPE) {
				case VK_Pass::PassType::DP:
				{
					bool is_Found = false;
					for (unsigned int CheckedDP = 0; CheckedDP < renderer->DrawPasses.size(); CheckedDP++) {
						if (*Wait_desc.WaitedPass == &renderer->DrawPasses[CheckedDP]->base_data) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						std::cout << CP->base_data.NAME << std::endl;
						printer(result_tgfx_FAIL, "One of the compute passes waits for an draw pass but given pass isn't found!");
						return false;
					}
				}
				break;
				case VK_Pass::PassType::TP:
				{
					transferpass_vk* currentTP = ((transferpass_vk*)*Wait_desc.WaitedPass);
					bool is_Found = false;
					for (unsigned int CheckedTP = 0; CheckedTP < renderer->TransferPasses.size(); CheckedTP++) {
						if (currentTP == renderer->TransferPasses[CheckedTP]) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						printer(result_tgfx_FAIL, "One of the compute passes waits for a transfer pass but given pass isn't found!");
						return false;
					}
					break;
				}
				case VK_Pass::PassType::WP:
				{
					bool is_Found = false;
					for (unsigned int CheckedWP = 0; CheckedWP < renderer->WindowPasses.size(); CheckedWP++) {
						if (*Wait_desc.WaitedPass == &renderer->WindowPasses[CheckedWP]->base_data) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						printer(result_tgfx_FAIL, "One of the compute passes waits for a window pass but given pass isn't found!");
						return false;
					}
					break;
				}
				case VK_Pass::PassType::CP:
				{
					bool is_Found = false;
					for (unsigned int CheckedCP = 0; CheckedCP < renderer->ComputePasses.size(); CheckedCP++) {
						if (*Wait_desc.WaitedPass == &renderer->ComputePasses[CheckedCP]->base_data) {
							is_Found = true;
							break;
						}
					}
					if (!is_Found) {
						printer(result_tgfx_FAIL, "One of the compute passes waits for a compute pass but given pass isn't found!");
						return false;
					}
					break;
				}
				case VK_Pass::PassType::INVALID:
				{
					printer(result_tgfx_FAIL, "Finding a TPType has failed, so Check Wait Handle too!");
					return false;
				}
				default:
				{
					printer(result_tgfx_FAIL, "TP Type is not supported for now, so Check Wait Handle failed too!");
					return false;
				}
				}

			}
		}
		return true;
	}
	inline static void Create_FrameGraphs() {
		VKPASSes.clear();
		for (unsigned int i = 0; i < renderer->DrawPasses.size(); i++) { VKPASSes.push_back(&renderer->DrawPasses[i]->base_data); }
		for (unsigned int i = 0; i < renderer->ComputePasses.size(); i++) { VKPASSes.push_back(&renderer->ComputePasses[i]->base_data); }
		for (unsigned int i = 0; i < renderer->TransferPasses.size(); i++) { VKPASSes.push_back(&renderer->TransferPasses[i]->base_data); }
		for (unsigned int i = 0; i < renderer->WindowPasses.size(); i++) { VKPASSes.push_back(&renderer->WindowPasses[i]->base_data); }

		VK_Pass* searched_dependency = nullptr;
		Execution_Order.clear();
		bool is_searching = true;
		while (is_searching) {
			is_searching = false;
			for (unsigned int pass_i = 0; pass_i < VKPASSes.size(); pass_i++) {
				VK_Pass* main_pass = VKPASSes[pass_i];
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
};
std::vector<VK_Pass*> framegraphsys_vk::VKPASSes = std::vector<VK_Pass*>(), framegraphsys_vk::Execution_Order = std::vector<VK_Pass*>();



unsigned char Finish_RenderGraphConstruction(subdrawpass_tgfx_handle IMGUI_Subpass) {
	if (renderer->RG_Status != RenderGraphStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "VulkanRenderer->Finish_RenderGraphCreation() has failed because you didn't call Start_RenderGraphConstruction() before!");
		return false;
	}

	//Checks wait handles of the render nodes, if matching fails return false
	if (!framegraphsys_vk::Check_WaitHandles()) {
		printer(result_tgfx_FAIL, "VulkanRenderer->Finish_RenderGraphConstruction() has failed because some wait handles have issues!");
		renderer->RG_Status = RenderGraphStatus::Invalid;	//User can change only waits of a pass, so all rendergraph falls to invalid in this case
		//It needs to be reconstructed by calling Start_RenderGraphConstruction()
		return false;
	}


	//Create static information
	framegraphsys_vk::Create_FrameGraphs();


#ifndef NO_IMGUI
	//If dear Imgui is created, apply subpass changes and render UI
	if (imgui) {
		//Initialize IMGUI if it isn't initialized
		if (IMGUI_Subpass) {
			//If subdrawpass has more than one color slot, return false
			if (((drawpass_vk*)((subdrawpass_vk*)IMGUI_Subpass)->DrawPass)->SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT != 1) {
				printer(result_tgfx_FAIL, "The Drawpass that's gonna render dear IMGUI should only have one color slot!");
				renderer->RG_Status = RenderGraphStatus::Invalid;	//User can delete a draw pass, dear Imgui fails in this case.
				//To avoid this, it needs to be reconstructed by calling Start_RenderGraphConstruction()
				return false;
			}

			if (imgui->Get_IMGUIStatus() == imgui_vk::IMGUI_STATUS::UNINITIALIZED) {
				imgui->Initialize(IMGUI_Subpass);
				imgui->UploadFontTextures();
			}
			//If given subpass is different from the previous one, change subpass
			else if (((subdrawpass_vk*)imgui->Get_SubDrawPassHandle()) != ((subdrawpass_vk*)IMGUI_Subpass)) {
				((subdrawpass_vk*)imgui->Get_SubDrawPassHandle())->render_dearIMGUI = false;
				((subdrawpass_vk*)IMGUI_Subpass)->render_dearIMGUI = true;
				imgui->Change_DrawPass(IMGUI_Subpass);
			}

			imgui->NewFrame();
		}
	}
#endif

	renderer->RG_Status = RenderGraphStatus::FinishConstructionCalled;
	return true;
}

void SetBarrier_BetweenPasses(VkCommandBuffer CB, VK_Pass* CurrentPass, VK_Pass* LastPass, VkPipelineStageFlags& srcPipelineStage) {
	printer(result_tgfx_WARNING, "SetBarrier_BetweenPasses() isn't coded");
}
void Record_RenderPass(VkCommandBuffer CB, drawpass_vk* DrawPass) {
	const unsigned char FRAMEINDEX = renderer->Get_FrameIndex(false);

	rtslots_vk& CF_SLOTs = DrawPass->SLOTSET->PERFRAME_SLOTSETs[FRAMEINDEX];
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
	for (unsigned char SubPassIndex = 0; SubPassIndex < DrawPass->Subpass_Count; SubPassIndex++) {
		subdrawpass_vk& SP = DrawPass->Subpasses[SubPassIndex];
		if (SP.Binding_Index != SubPassIndex) {
			printer(result_tgfx_FAIL, "Subpass Binding Index and the SubDrawPass' element index doesn't match, handle this case!");
		}
		if (SP.render_dearIMGUI) {
			imgui->Render_toCB(CB);
		}
		//NonIndexed Draw Calls
		{
			std::unique_lock<std::mutex> DrawCallLocker;
			SP.NonIndexedDrawCalls.PauseAllOperations(DrawCallLocker);
			VkDescriptorSet Sets[4] = { contentmanager->get_GlobalBuffersDescSet(), contentmanager->get_GlobalTexturesDescsSet(), VK_NULL_HANDLE, VK_NULL_HANDLE };
			for (unsigned char ThreadIndex = 0; ThreadIndex < threadcount; ThreadIndex++) {
				for (unsigned char DrawCallIndex = 0; DrawCallIndex < SP.NonIndexedDrawCalls.size(ThreadIndex); DrawCallIndex++) {
					nonindexeddrawcall_vk& DrawCall = SP.NonIndexedDrawCalls.get(ThreadIndex, DrawCallIndex);
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
							Sets[2] = *DrawCall.GeneralSet;
							Sets[3] = *DrawCall.PerInstanceSet;
							vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 4, Sets, 0, nullptr);
						}
						else {
							Sets[2] = *DrawCall.PerInstanceSet;
							vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 3, Sets, 0, nullptr);
						}
					}
					else if (*DrawCall.GeneralSet) {
						Sets[2] = *DrawCall.GeneralSet;
						vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 3, Sets, 0, nullptr);
					}
					else {
						vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 2, Sets, 0, nullptr);
					}
					if (DrawCall.VBuffer != VK_NULL_HANDLE) {
						vkCmdBindVertexBuffers(CB, 0, 1, &DrawCall.VBuffer, &DrawCall.VOffset);
					}
					vkCmdDraw(CB, DrawCall.VertexCount, DrawCall.InstanceCount, DrawCall.FirstVertex, DrawCall.FirstInstance);
				}
				SP.NonIndexedDrawCalls.clear(ThreadIndex);
			}
		}
		//Indexed Draw Calls
		{
			std::unique_lock<std::mutex> DrawCallLocker;
			SP.IndexedDrawCalls.PauseAllOperations(DrawCallLocker);
			VkDescriptorSet Sets[4] = { contentmanager->get_GlobalBuffersDescSet(), contentmanager->get_GlobalTexturesDescsSet(), VK_NULL_HANDLE, VK_NULL_HANDLE };
			for (unsigned char ThreadIndex = 0; ThreadIndex < threadcount; ThreadIndex++) {
				for (unsigned char DrawCallIndex = 0; DrawCallIndex < SP.IndexedDrawCalls.size(ThreadIndex); DrawCallIndex++) {
					indexeddrawcall_vk& DrawCall = SP.IndexedDrawCalls.get(ThreadIndex, DrawCallIndex);
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
							Sets[2] = *DrawCall.GeneralSet;
							Sets[3] = *DrawCall.PerInstanceSet;
							vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 4, Sets, 0, nullptr);
						}
						else {
							Sets[2] = *DrawCall.PerInstanceSet;
							vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 3, Sets, 0, nullptr);
						}
					}
					else if (*DrawCall.GeneralSet) {
						Sets[2] = *DrawCall.GeneralSet;
						vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 3, Sets, 0, nullptr);
					}
					else {
						vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, DrawCall.MatTypeLayout, 0, 2, Sets, 0, nullptr);
					}
					if (DrawCall.VBuffer != VK_NULL_HANDLE) {
						vkCmdBindVertexBuffers(CB, 0, 1, &DrawCall.VBuffer, &DrawCall.VBOffset);
					}
					vkCmdBindIndexBuffer(CB, DrawCall.IBuffer, DrawCall.IBOffset, DrawCall.IType);
					vkCmdDrawIndexed(CB, DrawCall.IndexCount, DrawCall.InstanceCount, DrawCall.FirstIndex, DrawCall.VOffset, DrawCall.FirstInstance);
				}
				SP.IndexedDrawCalls.clear(ThreadIndex);
			}
		}

		if (SubPassIndex < DrawPass->Subpass_Count - 1) {
			vkCmdNextSubpass(CB, VK_SUBPASS_CONTENTS_INLINE);
		}
	}
	vkCmdEndRenderPass(CB);
}

void Record_BarrierTP(VkCommandBuffer CB, VkPipelineStageFlags srcPipelineStage, VK_TPBarrierDatas* DATAs) {
	std::unique_lock<std::mutex> Locker;
	DATAs->TextureBarriers.PauseAllOperations(Locker);
	std::vector<VkImageMemoryBarrier> IMBARRIES;
	unsigned int LatestIndex = 0;
	for (unsigned char ThreadIndex = 0; ThreadIndex < threadcount; ThreadIndex++) {
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
		for (unsigned char ThreadIndex = 0; ThreadIndex < threadcount; ThreadIndex++) {
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
		for (unsigned char ThreadIndex = 0; ThreadIndex < threadcount; ThreadIndex++) {
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
		for (unsigned char ThreadIndex = 0; ThreadIndex < threadcount; ThreadIndex++) {
			for (unsigned int i = 0; i < DATAs->IMIMCopies.size(ThreadIndex); i++) {
				VK_IMtoIMinfo& info = DATAs->IMIMCopies.get(ThreadIndex, i);
				vkCmdCopyImage(CB, info.SourceTexture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, info.TargetTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &info.info);
				LastElementIndex++;
			}
			DATAs->IMIMCopies.clear(ThreadIndex);
		}
	}
}

void Record_ComputePass(VkCommandBuffer CB, computepass_vk* CP) {
	for (unsigned int SPIndex = 0; SPIndex < CP->Subpasses.size(); SPIndex++) {
		subcomputepass_vk& SP = CP->Subpasses[SPIndex];
		std::unique_lock<std::mutex> Locker;
		SP.Dispatches.PauseAllOperations(Locker);
		VkDescriptorSet Sets[4] = {contentmanager->get_GlobalBuffersDescSet(), contentmanager->get_GlobalTexturesDescsSet(), VK_NULL_HANDLE, VK_NULL_HANDLE};
		for (unsigned char ThreadID = 0; ThreadID < threadcount; ThreadID++) {
			for (unsigned int DispatchIndex = 0; DispatchIndex < SP.Dispatches.size(ThreadID); DispatchIndex++) {
				dispatchcall_vk& dc = SP.Dispatches.get(ThreadID, DispatchIndex);
				vkCmdBindPipeline(CB, VK_PIPELINE_BIND_POINT_COMPUTE, dc.Pipeline);
				if (*dc.InstanceSet) {
					if (*dc.GeneralSet) {
						Sets[2] = *dc.GeneralSet;
						Sets[3] = *dc.InstanceSet;
						vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_COMPUTE, dc.Layout, 0, 4, Sets, 0, nullptr);
					}
					else {
						Sets[2] = *dc.InstanceSet;
						vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_COMPUTE, dc.Layout, 0, 3, Sets, 0, nullptr);
					}
				}
				else if (*dc.GeneralSet) {
					Sets[2] = *dc.GeneralSet;
					vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_COMPUTE, dc.Layout, 0, 3, Sets, 0, nullptr);
				}
				else {
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
			for (unsigned int ThreadID = 0; ThreadID < threadcount; ThreadID++) {
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

static std::vector<VkSemaphore> SwapchainAcquireSemaphores;

result_tgfx Execute_RenderGraph() {
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
		VK_Pass* CurrentPass = framegraphsys_vk::Execution_Order[pass_i];

		//Set barrier before next pass
		VkPipelineStageFlags srcPipelineStage = 0;
		if (pass_i > 0) {
			SetBarrier_BetweenPasses(CB, framegraphsys_vk::Execution_Order[pass_i], framegraphsys_vk::Execution_Order[pass_i - 1], srcPipelineStage);
		}

		switch (CurrentPass->TYPE) {
		case VK_Pass::PassType::DP:
			Record_RenderPass(CB, (drawpass_vk*)CurrentPass);
			break;
		case VK_Pass::PassType::TP:
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
		case VK_Pass::PassType::WP:
			if (pass_i != framegraphsys_vk::Execution_Order.size() - 1) {
				printer(result_tgfx_FAIL, "Primitive Rendergraph supports windows passes only to be in last in the execution order, please fix your execution order!");
			}
			break;
		case VK_Pass::PassType::CP:
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

	//Find how many display queues will be used (to detect how many semaphores render command buffers should signal)
	struct PresentationInfos {
		std::vector<VkSwapchainKHR> swapchains;
		std::vector<uint32_t> image_indexes;
		queuefam_vk* queuefam;
	};
	std::vector<PresentationInfos> PresentationInfo_PerQueue;
	//Send displays (primitive rendergraph supports to have only one window pass and it is at the end)
	for (unsigned char i = 0; i < 1; i++) {
		windowpass_vk* WP = renderer->WindowPasses[0];
		if (!WP->WindowCalls[2].size()) {
			continue;
		}
		for (unsigned char WindowIndex = 0; WindowIndex < WP->WindowCalls[2].size(); WindowIndex++) {
			queuefam_vk* window_qfam = WP->WindowCalls[2][WindowIndex].Window->presentationqueue;
			bool is_found = false;
			for (unsigned char queue_i = 0; queue_i < PresentationInfo_PerQueue.size() && !is_found; queue_i++) {
				if (window_qfam == PresentationInfo_PerQueue[queue_i].queuefam) {
					PresentationInfo_PerQueue[queue_i].image_indexes.push_back(WP->WindowCalls[2][WindowIndex].Window->CurrentFrameSWPCHNIndex);
					WP->WindowCalls[2][WindowIndex].Window->CurrentFrameSWPCHNIndex = (WP->WindowCalls[2][WindowIndex].Window->CurrentFrameSWPCHNIndex + 1) % 2;
					PresentationInfo_PerQueue[queue_i].swapchains.push_back(WP->WindowCalls[2][WindowIndex].Window->Window_SwapChain);
					is_found = true;
				}
			}
			if (!is_found) {
				PresentationInfo_PerQueue.push_back(PresentationInfos());
				PresentationInfos& presentation = PresentationInfo_PerQueue[PresentationInfo_PerQueue.size() - 1];
				presentation.image_indexes.push_back(WP->WindowCalls[2][WindowIndex].Window->CurrentFrameSWPCHNIndex);
				WP->WindowCalls[2][WindowIndex].Window->CurrentFrameSWPCHNIndex = (WP->WindowCalls[2][WindowIndex].Window->CurrentFrameSWPCHNIndex + 1) % 2;
				presentation.swapchains.push_back(WP->WindowCalls[2][WindowIndex].Window->Window_SwapChain);
				presentation.queuefam = window_qfam;
			}
		}

		std::vector<windowcall_vk> windowcall0 = WP->WindowCalls[0];
		WP->WindowCalls[0] = WP->WindowCalls[1];
		WP->WindowCalls[1] = WP->WindowCalls[2];
		WP->WindowCalls[2] = windowcall0;
	}

	//There is only one command buffer, so we need only one submit
	//It should wait for penultimate frame's display presentation to end
	VkSubmitInfo submit_info = {};
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &CB;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = SwapchainAcquireSemaphores.size();
	submit_info.pWaitSemaphores = SwapchainAcquireSemaphores.data();
	submit_info.signalSemaphoreCount = PresentationInfo_PerQueue.size();
	std::vector<VkSemaphore> signalSemaphores(PresentationInfo_PerQueue.size());
	for (unsigned char semaphore_i = 0; semaphore_i < signalSemaphores.size(); semaphore_i++) {
		signalSemaphores[semaphore_i] = semaphoresys->Create_Semaphore().vksemaphore();
	}
	submit_info.pSignalSemaphores = signalSemaphores.data();
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	queuesys->queueSubmit(rendergpu, rendergpu->GRAPHICSQUEUEFAM(), submit_info);


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
			SwapchainImage_PresentationInfo.pWaitSemaphores = signalSemaphores.data();

			if (vkQueuePresentKHR(queuesys->get_queue(rendergpu, PresentationInfo_PerQueue[presentation_i].queuefam), &SwapchainImage_PresentationInfo) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "Submitting Presentation Queue has failed!");
				return result_tgfx_FAIL;
			}
		}
	}

	return result_tgfx_SUCCESS;
}

void Destroy_RenderGraph() {

}