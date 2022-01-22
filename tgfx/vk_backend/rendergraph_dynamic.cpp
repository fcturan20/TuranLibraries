#include "renderer.h"
#include "rendergraph.h"
#include "gpucontentmanager.h"
#include "imgui.h"


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

void framegraphsys_vk::WaitForLastFrame(framegraph_vk& Last_FrameGraph){}


void framegraphsys_vk::UnsignalLastFrameSemaphores(framegraph_vk& Last_FrameGraph){}
void framegraphsys_vk::CreateSubmits_Intermediate(framegraph_vk& Current_FrameGraph){}
void framegraphsys_vk::DuplicateFrameGraph(framegraph_vk& TargetFrameGraph, const framegraph_vk& SourceFrameGraph){}
void framegraphsys_vk::Prepare_forSubmitCreation(){}
void framegraphsys_vk::CreateSubmits_Fast(framegraph_vk& Current_FrameGraph, framegraph_vk& LastFrameGraph){}
void framegraphsys_vk::Send_RenderCommands(){}
void framegraphsys_vk::Send_PresentationCommands(){}
void framegraphsys_vk::RenderGraph_DataShifting(){}