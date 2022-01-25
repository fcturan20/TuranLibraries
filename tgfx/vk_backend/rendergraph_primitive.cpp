//This is to test other backend functions with minimum rendergraph complexity
#include "renderer.h"
#include "gpucontentmanager.h"
#include "imgui_vktgfx.h"
#include "resource.h"
#include <tgfx_forwarddeclarations.h>

void Start_RenderGraphConstruction() {
	//Apply resource changes made by user
	contentmanager->Resource_Finalizations();

	if (renderer->RGSTATUS() == RenderGraphStatus::FinishConstructionCalled || renderer->RGSTATUS() == RenderGraphStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "GFXRENDERER->Start_RenderGraphCreation() has failed because you have wrong function call order!"); return;
	}
	renderer->RG_Status = RenderGraphStatus::StartedConstruction;
}

struct framegraphsys_vk {
public:
	inline static bool Check_WaitHandles();
	inline static void Create_FrameGraphs() {
		printer(result_tgfx_NOTCODED, "Create_FrameGraphs() isn't coded");
	}
};

bool framegraphsys_vk::Check_WaitHandles() {
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
			//If subdrawpass has only one color slot, return false
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
		}

		imgui->NewFrame();
	}
#endif

	renderer->RG_Status = RenderGraphStatus::FinishConstructionCalled;
	return true;
}

result_tgfx Execute_RenderGraph() {
	return result_tgfx_FAIL;
}

void Destroy_RenderGraph() {

}