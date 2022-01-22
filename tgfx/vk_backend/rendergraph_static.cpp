#include "renderer.h"
#include "rendergraph.h"
#include "gpucontentmanager.h"
#include "imgui.h"
#include "queue.h"



//Each framegraph is constructed as same, so there is no difference about passing different framegraphs here
void framegraphsys_vk::Create_VkDataofRGBranches(const framegraph_vk& FrameGraph) {
	printer(result_tgfx_NOTCODED, "Couldn't find anything to do in Create_VkDataofRGBranches()");
}
//This structure is to create RGBranches easily
struct VK_RenderingPath {
	unsigned char ID;	//For now, I give each RenderingPath an ID to debug the algorithm
	//This passes are executed in the First-In-First-Executed order which means element 0 is executed first, then 1...
	BranchIDType BranchIDs[2]{ INVALID_BranchID };
	std::vector<VK_Pass*> ExecutionOrder;
	std::vector<VK_RenderingPath*> CFDependentRPs, LFDependentRPs, CFLaterExecutedRPs, NFExecutedRPs;
	queueflag_vk QueueFeatureRequirements;
};


//If anything (PRESENTATION is ignored) is false, that means this pass doesn't require its existence
//If everything (PRESENTATION is ignored) is false, that means every queue type can call this pass
queueflag_vk FindRequiredQueue_ofGP(const VK_Pass* GP) {
	queueflag_vk flag;
	flag.is_COMPUTEsupported = false;
	flag.is_GRAPHICSsupported = false;
	flag.is_TRANSFERsupported = false;
	flag.is_PRESENTATIONsupported = false;
	switch (GP->TYPE) {
	case VK_Pass::PassType::DP:
		flag.is_GRAPHICSsupported = true;
		break;
	case VK_Pass::PassType::TP:
		if (((transferpass_vk*)GP)->TYPE == transferpasstype_tgfx_BARRIER) {
			//All is false means any queue can call this pass!
			break;
		}
		flag.is_TRANSFERsupported = true;
		break;
	case VK_Pass::PassType::CP:
		flag.is_COMPUTEsupported = true;
		break;
	case VK_Pass::PassType::WP:
		flag.is_PRESENTATIONsupported = true;
		break;
	default:
		printer(result_tgfx_FAIL, "FindRequiredQueue_ofGP() doesn't support given TPType!");
	}
	return flag;
}

unsigned int Generate_RPID() {
	static unsigned int Next_RPID = 0;
	Next_RPID++;
	return Next_RPID;
}


//Check if Pass is already in a RP, if it is not then Create New RP
//Create New RP: Set dependent RPs of the new RP as DependentRPs and give newRP to them as LaterExecutedRPs element
VK_RenderingPath* Create_NewRP(VK_Pass* Pass, std::vector<VK_RenderingPath*>& RPs) {
	VK_RenderingPath* RP;
	//Check if the pass is in a RP
	for (unsigned int RPIndex = 0; RPIndex < RPs.size(); RPIndex++) {
		VK_RenderingPath* RP_Check = RPs[RPIndex];
		for (unsigned int RPPassIndex = 0; RPPassIndex < RP_Check->ExecutionOrder.size(); RPPassIndex++) {
			VK_Pass* GP_Check = RP_Check->ExecutionOrder[RPPassIndex];
			if (GP_Check == Pass) {
				printer(result_tgfx_FAIL, ("Pass is already in a RP!" + Pass->NAME).c_str());
				return nullptr;
			}
		}
	}
	RP = new VK_RenderingPath;
	//Create dependencies for all
	for (unsigned int PassWaitIndex = 0; PassWaitIndex < Pass->WAITsCOUNT; PassWaitIndex++) {
		VK_Pass::WaitDescription& Wait_desc = Pass->WAITs[PassWaitIndex];
		for (unsigned int RPIndex = 0; RPIndex < RPs.size(); RPIndex++) {
			VK_RenderingPath* RP_Check = RPs[RPIndex];
			bool is_alreadydependent = false;
			for (unsigned int DependentRPIndex = 0; DependentRPIndex < RP->CFDependentRPs.size() && !is_alreadydependent; DependentRPIndex++) {
				if (RP_Check == RP->CFDependentRPs[DependentRPIndex]) {
					is_alreadydependent = true;
					break;
				}
			}
			if (is_alreadydependent) {
				continue;
			}
			for (unsigned int RPPassIndex = 0; RPPassIndex < RP_Check->ExecutionOrder.size(); RPPassIndex++) {
				VK_Pass* GP_Check = RP_Check->ExecutionOrder[RPPassIndex];
				if (!Wait_desc.WaitLastFramesPass &&
					*Wait_desc.WaitedPass == GP_Check) {
					RP->CFDependentRPs.push_back(RP_Check);
					RP_Check->CFLaterExecutedRPs.push_back(RP);
				}
			}
		}
	}
	RP->ExecutionOrder.clear();
	RP->ExecutionOrder.push_back(Pass);
	RP->ID = Generate_RPID();
	RP->QueueFeatureRequirements = FindRequiredQueue_ofGP(Pass);

	RPs.push_back(RP);
	return RP;
}



void framegraphsys_vk::Create_FrameGraphs() {

}