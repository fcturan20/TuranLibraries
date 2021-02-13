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

namespace Vulkan {
	struct VK_GeneralPass {
		GFX_API::GFXHandle Handle;
		PassType TYPE;
		vector<GFX_API::PassWait_Description>* WAITs;
		string* PASSNAME;
	};

	PassType FindWaitedPassType(GFX_API::SHADERSTAGEs_FLAG flag);

	struct VK_GeneralPassLinkedList {
		VK_GeneralPass* CurrentGP = nullptr;
		VK_GeneralPassLinkedList* NextElement = nullptr;
	};
	VK_GeneralPassLinkedList* Create_GPLinkedList() {
		return new VK_GeneralPassLinkedList;
	}
	void PushBack_ToGPLinkedList(VK_GeneralPass* GP, VK_GeneralPassLinkedList* GPLL) {
		//Go to the last element in the list!
		while (GPLL->NextElement != nullptr) {
			GPLL = GPLL->NextElement;
		}
		//If element points to a GP, create new element
		if (GPLL->CurrentGP != nullptr) {
			VK_GeneralPassLinkedList* newelement = Create_GPLinkedList();
			GPLL->NextElement = newelement;
			newelement->CurrentGP = GP;
		}
		//Otherwise, make element point to the GP
		else {
			GPLL->CurrentGP = GP;
		}
	}
	//Return the new header if the header changes!
	//If header isn't found, given header is returned
	//If header is the last element and should be deleted, then nullptr is returned
	//If header isn't the last element but should be deleted, then next element is returned
	VK_GeneralPassLinkedList* DeleteGP_FromGPLinkedList(VK_GeneralPassLinkedList* Header, VK_GeneralPass* ElementToDelete) {
		VK_GeneralPassLinkedList* CurrentCheckItem = Header;
		VK_GeneralPassLinkedList* LastChecked_Item = nullptr;
		while (CurrentCheckItem->CurrentGP != ElementToDelete) {
			if (!CurrentCheckItem->NextElement) {
				LOG_STATUS_TAPI("VulkanRenderer: DeleteGP_FromGPLinkedList() couldn't find the GP in the list!");
				return Header;
			}
			LastChecked_Item = CurrentCheckItem;
			CurrentCheckItem = CurrentCheckItem->NextElement;
		}
		if (!LastChecked_Item) {
			CurrentCheckItem->CurrentGP = nullptr;
			if (CurrentCheckItem->NextElement) {
				return CurrentCheckItem->NextElement;
			}
			return nullptr;
		}
		LastChecked_Item->NextElement = CurrentCheckItem->NextElement;
		CurrentCheckItem->CurrentGP = nullptr;
		return Header;
	}

	//This structure is to create RGBranches easily
	struct VK_RenderingPath {
		unsigned char ID;	//For now, I give each RenderingPath an ID to debug the algorithm
		//This passes are executed in the First-In-First-Executed order which means element 0 is executed first, then 1...
		vector<VK_GeneralPass*> ExecutionOrder;
		vector<VK_RenderingPath*> CFDependentRPs, LFDependentRPs, CFLaterExecutedRPs, NFExecutedRPs;
		VK_QUEUEFLAG QueueFeatureRequirements;
	};


	//If anything (PRESENTATION is ignored) is false, that means this pass doesn't require its existence
	//If everything (PRESENTATION is ignored) is false, that means every queue type can call this pass
	VK_QUEUEFLAG FindRequiredQueue_ofGP(const VK_GeneralPass* GP) {
		VK_QUEUEFLAG flag;
		flag.is_COMPUTEsupported = false;
		flag.is_GRAPHICSsupported = false;
		flag.is_TRANSFERsupported = false;
		flag.is_PRESENTATIONsupported = false;
		switch (GP->TYPE) {
		case PassType::DP:
			flag.is_GRAPHICSsupported = true;
			break;
		case PassType::TP:
			if (GFXHandleConverter(VK_TransferPass*, GP->Handle)->TYPE == GFX_API::TRANFERPASS_TYPE::TP_BARRIER) {
				//All is false means any queue can call this pass!
				break;
			}
			flag.is_TRANSFERsupported = true;
			break;
		case PassType::CP:
			flag.is_COMPUTEsupported = true;
			break;
		case PassType::WP:
			flag.is_PRESENTATIONsupported = true;
			break;
		default:
			LOG_CRASHING_TAPI("FindRequiredQueue_ofGP() doesn't support given TPType!");
		}
		return flag;
	}

	unsigned int Generate_RPID() {
		static unsigned int Next_RPID = 0;
		Next_RPID++;
		return Next_RPID;
	}

	void PrintRPSpecs(VK_RenderingPath* RP) {
		std::cout << ("RP ID: " + to_string(RP->ID)) << std::endl;
		for (unsigned int GPIndex = 0; GPIndex < RP->ExecutionOrder.size(); GPIndex++) {
			std::cout << ("Execution Order: " + to_string(GPIndex) + " and Pass Name: " + *RP->ExecutionOrder[GPIndex]->PASSNAME) << std::endl;
		}
		for (unsigned int CFDependentRPIndex = 0; CFDependentRPIndex < RP->CFDependentRPs.size(); CFDependentRPIndex++) {
			std::cout << ("Current Frame Dependent RP ID: " + to_string(RP->CFDependentRPs[CFDependentRPIndex]->ID)) << std::endl;
		}
		for (unsigned int LFDependentRPIndex = 0; LFDependentRPIndex < RP->LFDependentRPs.size(); LFDependentRPIndex++) {
			std::cout << ("Last Frame Dependent RP ID: " + to_string(RP->LFDependentRPs[LFDependentRPIndex]->ID)) << std::endl;
		}
		for (unsigned int CFLaterExecuteRPIndex = 0; CFLaterExecuteRPIndex < RP->CFLaterExecutedRPs.size(); CFLaterExecuteRPIndex++) {
			std::cout << ("Current Frame Later Executed RP ID: " + to_string(RP->CFLaterExecutedRPs[CFLaterExecuteRPIndex]->ID)) << std::endl;
		}
		for (unsigned int LFLaterExecuteRPIndex = 0; LFLaterExecuteRPIndex < RP->NFExecutedRPs.size(); LFLaterExecuteRPIndex++) {
			std::cout << ("Next Frame Executed RP ID: " + to_string(RP->NFExecutedRPs[LFLaterExecuteRPIndex]->ID)) << std::endl;
		}
	}
	void PrintRBSpecs(const VK_RGBranch& RB) {
		std::cout << ("Branch ID: " + to_string(RB.ID)) << std::endl;
		for (unsigned int GPIndex = 0; GPIndex < RB.PassCount; GPIndex++) {
			VK_BranchPass& Pass = RB.CorePasses[GPIndex];
			switch (Pass.TYPE) {
			case PassType::DP:
				std::cout << "Execution Order: " + to_string(GPIndex) + " and DP Name: " + GFXHandleConverter(VK_DrawPass*, Pass.Handle)->NAME << std::endl;
				break;
			case PassType::TP:
				std::cout << "Execution Order: " + to_string(GPIndex) + " and TP Name: " + GFXHandleConverter(VK_TransferPass*, Pass.Handle)->NAME << std::endl;
				break;
			case PassType::WP:
				std::cout << "Execution Order: " + to_string(GPIndex) + " and WP Name: " + GFXHandleConverter(VK_WindowPass*, Pass.Handle)->NAME << std::endl;
				break;
			default:
				LOG_NOTCODED_TAPI("PrintRBSpecs() is not coded for this type of Pass!", true);
				break;
			}
		}
		for (unsigned int CFDependentRPIndex = 0; CFDependentRPIndex < RB.CFDependentBranchCount; CFDependentRPIndex++) {
			std::cout << ("Current Frame Dependent RB ID: " + to_string(RB.CFDependentBranches[CFDependentRPIndex]->ID)) << std::endl;
		}
		for (unsigned int LFDependentRPIndex = 0; LFDependentRPIndex < RB.LFDependentBranchCount; LFDependentRPIndex++) {
			std::cout << ("Last Frame Dependent RB ID: " + to_string(RB.LFDependentBranches[LFDependentRPIndex]->ID)) << std::endl;
		}
		for (unsigned int CFLaterExecuteRPIndex = 0; CFLaterExecuteRPIndex < RB.LaterExecutedBranchCount; CFLaterExecuteRPIndex++) {
			std::cout << ("Current Frame Later Executed RB ID: " + to_string(RB.LaterExecutedBranches[CFLaterExecuteRPIndex]->ID)) << std::endl;
		}
		for (unsigned int PenultimateDependentWPBranchIndex = 0; PenultimateDependentWPBranchIndex < RB.PenultimateSwapchainBranchCount; PenultimateDependentWPBranchIndex++) {
			std::cout << ("Penultimate Dependent RB ID: " + to_string(RB.PenultimateSwapchainBranches[PenultimateDependentWPBranchIndex]->ID)) << std::endl;
		}
	}

	//Check if Pass is already in a RP, if it is not then Create New RP
	//Create New RP: Set dependent RPs of the new RP as DependentRPs and give newRP to them as LaterExecutedRPs element
	VK_RenderingPath* Create_NewRP(VK_GeneralPass* Pass, vector<VK_RenderingPath*>& RPs) {
		VK_RenderingPath* RP;
		//Check if the pass is in a RP
		for (unsigned int RPIndex = 0; RPIndex < RPs.size(); RPIndex++) {
			VK_RenderingPath* RP_Check = RPs[RPIndex];
			for (unsigned int RPPassIndex = 0; RPPassIndex < RP_Check->ExecutionOrder.size(); RPPassIndex++) {
				VK_GeneralPass* GP_Check = RP_Check->ExecutionOrder[RPPassIndex];
				if (GP_Check->Handle == Pass->Handle) {
					LOG_CRASHING_TAPI("Pass is already in a RP!" + *Pass->PASSNAME);
					return nullptr;
				}
			}
		}
		RP = new VK_RenderingPath;
		//Create dependencies for all
		for (unsigned int PassWaitIndex = 0; PassWaitIndex < Pass->WAITs->size(); PassWaitIndex++) {
			GFX_API::PassWait_Description& Wait_desc = (*Pass->WAITs)[PassWaitIndex];
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
					VK_GeneralPass* GP_Check = RP_Check->ExecutionOrder[RPPassIndex];
					if (!Wait_desc.WaitLastFramesPass &&
						*Wait_desc.WaitedPass == GP_Check->Handle) {
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
	void AttachGPTo_RPBack(VK_GeneralPass* Pass, VK_RenderingPath* RP) {
		RP->ExecutionOrder.push_back(Pass);
		VK_QUEUEFLAG FLAG = FindRequiredQueue_ofGP(Pass);
		RP->QueueFeatureRequirements.is_COMPUTEsupported |= FLAG.is_COMPUTEsupported;
		RP->QueueFeatureRequirements.is_GRAPHICSsupported |= FLAG.is_GRAPHICSsupported;
		RP->QueueFeatureRequirements.is_TRANSFERsupported |= FLAG.is_TRANSFERsupported;
	}

	bool is_AnyWaitPass_Common(VK_GeneralPass* gp1, VK_GeneralPass* gp2) {
		for (unsigned int gp1waitIndex = 0; gp1waitIndex < gp1->WAITs->size(); gp1waitIndex++) {
			const GFX_API::PassWait_Description& gp1Wait = (*gp1->WAITs)[gp1waitIndex];
			for (unsigned int gp2waitIndex = 0; gp2waitIndex < gp2->WAITs->size(); gp2waitIndex++) {
				const GFX_API::PassWait_Description& gp2Wait = (*gp2->WAITs)[gp2waitIndex];
				if (*gp1Wait.WaitedPass == *gp2Wait.WaitedPass &&
					!gp1Wait.WaitLastFramesPass && !gp2Wait.WaitLastFramesPass
					//gp1Wait.WaitLastFramesPass == gp2Wait.WaitLastFramesPass
					)
				{
					return true;
				}
			}
		}
		return false;
	}

	//Return created LastFrameDependentRPs in LFD_RPs argument
	VK_GeneralPassLinkedList* Create_LastFrameDependentRPs(VK_GeneralPassLinkedList* ListHeader, vector<VK_RenderingPath*>& RPs, vector<VK_RenderingPath*>& LFD_RPs, vector<VK_RenderingPath*>& NFD_RPs, vector<VK_RenderingPath*>& STEP3_RPs) {
		//Step 1
		LFD_RPs.clear();
		VK_GeneralPassLinkedList* CurrentCheck = ListHeader;
		if (!CurrentCheck->CurrentGP) {
			return ListHeader;
		}
		while (CurrentCheck) {
			VK_GeneralPass* GP = CurrentCheck->CurrentGP;
			bool is_RPCreated = false;
			for (unsigned int WaitIndex = 0; WaitIndex < GP->WAITs->size() && !is_RPCreated; WaitIndex++) {
				GFX_API::PassWait_Description& Waitdesc = (*GP->WAITs)[WaitIndex];
				if (Waitdesc.WaitLastFramesPass || FindWaitedPassType(Waitdesc.WaitedStage) == PassType::WP) {
					LFD_RPs.push_back(Create_NewRP(GP, RPs));
					is_RPCreated = true;
					ListHeader = DeleteGP_FromGPLinkedList(ListHeader, GP);
					break;
				}
			}

			CurrentCheck = CurrentCheck->NextElement;
		}

		
		//Step 2
		//Create RPs for passes that's being dependent on by next frame
		//I mean, create RPs for passes that Step1 RPs above depends on from next frame
		CurrentCheck = ListHeader;
		while (CurrentCheck) {
			VK_GeneralPass* GP = CurrentCheck->CurrentGP;
			bool is_RPCreated = false;
			for (unsigned int LFDRPIndex = 0; LFDRPIndex < LFD_RPs.size() && !is_RPCreated; LFDRPIndex++) {
				for (unsigned int LFDRPWaitIndex = 0; LFDRPWaitIndex < LFD_RPs[LFDRPIndex]->ExecutionOrder[0]->WAITs->size(); LFDRPWaitIndex++) {
					GFX_API::PassWait_Description& Waitdesc = (*LFD_RPs[LFDRPIndex]->ExecutionOrder[0]->WAITs)[LFDRPWaitIndex];
					if ((Waitdesc.WaitLastFramesPass || FindWaitedPassType(Waitdesc.WaitedStage) == PassType::WP) && *Waitdesc.WaitedPass == GP->Handle) {
						NFD_RPs.push_back(Create_NewRP(GP, RPs));
						is_RPCreated = true;
						ListHeader = DeleteGP_FromGPLinkedList(ListHeader, GP);
						break;
					}
				}
			}

			CurrentCheck = CurrentCheck->NextElement;
		}


		//Step 3
		//Main algorithm attaches a Pass to an RP if there isn't any other pass that waits for the same pass
		//But RPs created at Step 2 are special, they must be alone!
		//So create RPs for these attachable passes!
		for (unsigned int NFDRPIndex = 0; NFDRPIndex < NFD_RPs.size(); NFDRPIndex++) {
			VK_GeneralPass* NFDRP_Pass = NFD_RPs[NFDRPIndex]->ExecutionOrder[0];
			CurrentCheck = ListHeader;

			if (!CurrentCheck) {
				continue;
			}
			//This is an exception, because passes that waits for a window pass with "WaitLastFrame == false" are already in their own RPs
			if (CurrentCheck->CurrentGP->TYPE == PassType::WP) {
				continue;
			}
			while (CurrentCheck) {
				VK_GeneralPass* RPlessPass = CurrentCheck->CurrentGP;
				bool is_RPCreated = false;
				for (unsigned int WaitIndex = 0; WaitIndex < RPlessPass->WAITs->size() && !is_RPCreated; WaitIndex++) {
					GFX_API::PassWait_Description& Waitdesc = (*RPlessPass->WAITs)[WaitIndex];
					if (!Waitdesc.WaitLastFramesPass && FindWaitedPassType(Waitdesc.WaitedStage) != PassType::WP && *Waitdesc.WaitedPass == NFDRP_Pass->Handle) {
						STEP3_RPs.push_back(Create_NewRP(RPlessPass, RPs));
						is_RPCreated = true;
						ListHeader = DeleteGP_FromGPLinkedList(ListHeader, RPlessPass);
						break;
					}
				}

				CurrentCheck = CurrentCheck->NextElement;
			}
		}


		return ListHeader;
	}

	void Latest_LinkageProcess(vector<VK_RenderingPath*>& LFD_RPs, vector<VK_RenderingPath*>&NFD_RPs, vector<VK_RenderingPath*>& STEP3RPs, vector<VK_RenderingPath*>& RPs) {
		for (unsigned int LFDRPIndex = 0; LFDRPIndex < LFD_RPs.size(); LFDRPIndex++) {
			VK_RenderingPath* LFDRP = LFD_RPs[LFDRPIndex];
			vector<GFX_API::PassWait_Description>* WAITs = LFDRP->ExecutionOrder[0]->WAITs;
			for (unsigned int WaitIndex = 0; WaitIndex < WAITs->size(); WaitIndex++) {
				GFX_API::PassWait_Description& Wait = (*WAITs)[WaitIndex];
				for (unsigned int MainListIndex = 0; MainListIndex < RPs.size(); MainListIndex++) {
					VK_RenderingPath* MainRP = RPs[MainListIndex];
					if (LFDRP == MainRP) {
						continue;
					}
					if (*Wait.WaitedPass == MainRP->ExecutionOrder[MainRP->ExecutionOrder.size() - 1]->Handle) {
						if (Wait.WaitLastFramesPass) {
							bool is_previously_linked = false;
							for (unsigned int CheckDependentPassesIndex = 0; CheckDependentPassesIndex < MainRP->NFExecutedRPs.size(); CheckDependentPassesIndex++) {
								if (MainRP->NFExecutedRPs[CheckDependentPassesIndex] == LFDRP) {
									is_previously_linked = true;
									break;
								}
							}
							if (!is_previously_linked) {
								LFDRP->LFDependentRPs.push_back(MainRP);
								MainRP->NFExecutedRPs.push_back(LFDRP);
							}
						}
						else {
							bool is_previously_linked = false;
							for (unsigned int CheckDependentPassesIndex = 0; CheckDependentPassesIndex < MainRP->CFLaterExecutedRPs.size(); CheckDependentPassesIndex++) {
								if (MainRP->CFLaterExecutedRPs[CheckDependentPassesIndex] == LFDRP) {
									is_previously_linked = true;
									break;
								}
							}
							if (!is_previously_linked) {
								LFDRP->CFDependentRPs.push_back(MainRP);
								MainRP->CFLaterExecutedRPs.push_back(LFDRP);
							}
						}
					}
				}
			}
		}
		//Link RPs that're waited by next frame passes, to the RPs they depend on
		for (unsigned int NFDRPIndex = 0; NFDRPIndex < NFD_RPs.size(); NFDRPIndex++) {
			VK_RenderingPath* NFDRP = NFD_RPs[NFDRPIndex];
			vector<GFX_API::PassWait_Description>* WAITs = NFDRP->ExecutionOrder[0]->WAITs;
			for (unsigned int WaitIndex = 0; WaitIndex < WAITs->size(); WaitIndex++) {
				GFX_API::PassWait_Description& Wait = (*WAITs)[WaitIndex];
				for (unsigned int MainRPIndex = 0; MainRPIndex < RPs.size(); MainRPIndex++) {
					VK_RenderingPath* MainRP = RPs[MainRPIndex];
					if (NFDRP == MainRP) {
						continue;
					}
					if (*Wait.WaitedPass == MainRP->ExecutionOrder[MainRP->ExecutionOrder.size() - 1]->Handle) {
						if (Wait.WaitLastFramesPass) {
							bool is_previously_linked = false;
							for (unsigned int CheckDependentPassesIndex = 0; CheckDependentPassesIndex < MainRP->NFExecutedRPs.size(); CheckDependentPassesIndex++) {
								if (MainRP->NFExecutedRPs[CheckDependentPassesIndex] == NFDRP) {
									is_previously_linked = true;
									break;
								}
							}
							if (!is_previously_linked) {
								NFDRP->LFDependentRPs.push_back(MainRP);
								MainRP->NFExecutedRPs.push_back(NFDRP);
							}
						}
						else {
							bool is_previously_linked = false;
							for (unsigned int CheckDependentPassesIndex = 0; CheckDependentPassesIndex < MainRP->CFLaterExecutedRPs.size(); CheckDependentPassesIndex++) {
								if (MainRP->CFLaterExecutedRPs[CheckDependentPassesIndex] == NFDRP) {
									is_previously_linked = true;
									break;
								}
							}
							if (!is_previously_linked) {
								NFDRP->CFDependentRPs.push_back(MainRP);
								MainRP->CFLaterExecutedRPs.push_back(NFDRP);
							}
						}
					}
				}
			}
		}
		//Link RPs that waits for NFDRP to the RPs that it depends on
		for (unsigned int STEP3RPIndex = 0; STEP3RPIndex < STEP3RPs.size(); STEP3RPIndex++) {
			VK_RenderingPath* STEP3RP = STEP3RPs[STEP3RPIndex];
			vector<GFX_API::PassWait_Description>* WAITs = STEP3RP->ExecutionOrder[0]->WAITs;
			for (unsigned int WaitIndex = 0; WaitIndex < WAITs->size(); WaitIndex++) {
				GFX_API::PassWait_Description& Wait = (*WAITs)[WaitIndex];
				for (unsigned int MainRPIndex = 0; MainRPIndex < RPs.size(); MainRPIndex++) {
					VK_RenderingPath* MainRP = RPs[MainRPIndex];
					if (STEP3RP == MainRP) {
						continue;
					}
					if (*Wait.WaitedPass == MainRP->ExecutionOrder[MainRP->ExecutionOrder.size() - 1]->Handle) {
						if (Wait.WaitLastFramesPass) {
							bool is_previously_linked = false;
							for (unsigned int CheckDependentPassesIndex = 0; CheckDependentPassesIndex < MainRP->NFExecutedRPs.size(); CheckDependentPassesIndex++) {
								if (MainRP->NFExecutedRPs[CheckDependentPassesIndex] == STEP3RP) {
									is_previously_linked = true;
									break;
								}
							}
							if (!is_previously_linked) {
								STEP3RP->LFDependentRPs.push_back(MainRP);
								MainRP->NFExecutedRPs.push_back(STEP3RP);
							}
						}
						else {
							bool is_previously_linked = false;
							for (unsigned int CheckDependentPassesIndex = 0; CheckDependentPassesIndex < MainRP->CFLaterExecutedRPs.size(); CheckDependentPassesIndex++) {
								if (MainRP->CFLaterExecutedRPs[CheckDependentPassesIndex] == STEP3RP) {
									is_previously_linked = true;
									break;
								}
							}
							if (!is_previously_linked) {
								STEP3RP->CFDependentRPs.push_back(MainRP);
								MainRP->CFLaterExecutedRPs.push_back(STEP3RP);
							}
						}
					}
				}
			}
		}
	}


	//This is a Recursive Function that calls itself to find or create a Rendering Path for each of the RPless_passes
	//But has "new"s that isn't followed by "delete", fix it when you have time
	void FindPasses_dependentRPs(VK_GeneralPassLinkedList* RPless_passes, vector<VK_RenderingPath*>& RPs) {
		if (!RPless_passes) {
			return;
		}
		if (!RPless_passes->CurrentGP) {
			return;
		}
		//These are the passes that all of their wait passes are in RPs, so store them to create new RPs
		VK_GeneralPassLinkedList* AttachableGPs_ListHeader = Create_GPLinkedList();

		VK_GeneralPassLinkedList* MainLoop_Element = RPless_passes;
		while (MainLoop_Element) {
			VK_GeneralPass* GP_Find = MainLoop_Element->CurrentGP;
			{
				bool DoesAnyWaitPass_RPless = false;
				bool should_addinnextiteration = false;
				for (unsigned int WaitIndex = 0; WaitIndex < GP_Find->WAITs->size() && !DoesAnyWaitPass_RPless; WaitIndex++) {
					GFX_API::PassWait_Description& Wait_Desc = (*GP_Find->WAITs)[WaitIndex];

					VK_GeneralPassLinkedList* WaitLoop_Element = RPless_passes;
					while (WaitLoop_Element && !DoesAnyWaitPass_RPless) {
						VK_GeneralPass* GP_WaitCheck = WaitLoop_Element->CurrentGP;
						if (WaitLoop_Element == MainLoop_Element) {
							//Here is empty because we don't to do anything if wait and main loop elements are the same, please don't change the code!
						}
						else 
						if (*Wait_Desc.WaitedPass == GP_WaitCheck->Handle
							&& !Wait_Desc.WaitLastFramesPass
							) 
						{
							DoesAnyWaitPass_RPless = true;
						}

						WaitLoop_Element = WaitLoop_Element->NextElement;
					}

					VK_GeneralPassLinkedList* AttachableGPs_CheckLoopElement = AttachableGPs_ListHeader;
					while (AttachableGPs_CheckLoopElement && !should_addinnextiteration && !DoesAnyWaitPass_RPless) {
						if (AttachableGPs_CheckLoopElement->CurrentGP) {
							VK_GeneralPass* AttachableGP_WaitCheck = AttachableGPs_CheckLoopElement->CurrentGP;
							if (*Wait_Desc.WaitedPass == AttachableGP_WaitCheck->Handle
								&& !Wait_Desc.WaitLastFramesPass
								) {
								DoesAnyWaitPass_RPless = true;
								should_addinnextiteration = true;
							}
						}

						AttachableGPs_CheckLoopElement = AttachableGPs_CheckLoopElement->NextElement;
					}
				}

				//That means all of the wait passes are already in RPs, so just store it to create or directly attach to a RP
				if (!DoesAnyWaitPass_RPless && !should_addinnextiteration) {
					PushBack_ToGPLinkedList(GP_Find, AttachableGPs_ListHeader);	//We don't care storing the last element because AttachableGPs_CheckLoop should check all the elements
					RPless_passes = DeleteGP_FromGPLinkedList(RPless_passes, GP_Find);
				}
			}

			MainLoop_Element = MainLoop_Element->NextElement;
		}

		//Attach GPs to RPs or Create RPs for GPs
		VK_GeneralPassLinkedList* AttachableGP_MainLoopElement = AttachableGPs_ListHeader;
		while (AttachableGP_MainLoopElement) {
			VK_GeneralPass* GP_Find = AttachableGP_MainLoopElement->CurrentGP;
			bool doeshave_anycommonWaitPass = false;
			VK_GeneralPassLinkedList* CommonlyWaited_Passes = Create_GPLinkedList();
			VK_GeneralPassLinkedList* AttachableGP_CheckLoopElement = AttachableGPs_ListHeader;
			while (AttachableGP_CheckLoopElement) {
				VK_GeneralPass* GP_Check = AttachableGP_CheckLoopElement->CurrentGP;
				if (AttachableGP_CheckLoopElement != AttachableGP_MainLoopElement && is_AnyWaitPass_Common(GP_Find, GP_Check)) {
					doeshave_anycommonWaitPass = true;
					PushBack_ToGPLinkedList(GP_Check, CommonlyWaited_Passes);
				}

				AttachableGP_CheckLoopElement = AttachableGP_CheckLoopElement->NextElement;
			}
			if (doeshave_anycommonWaitPass) {
				PushBack_ToGPLinkedList(GP_Find, CommonlyWaited_Passes);
				VK_GeneralPassLinkedList* CommonlyWaitedPasses_CreateElement = CommonlyWaited_Passes;
				while (CommonlyWaitedPasses_CreateElement->NextElement) {
					Create_NewRP(CommonlyWaitedPasses_CreateElement->CurrentGP, RPs);
					CommonlyWaitedPasses_CreateElement = CommonlyWaitedPasses_CreateElement->NextElement;
				}
			}
			else {
				bool break_allloops = false;
				bool shouldattach_toaRP = false;
				VK_RenderingPath* Attached_RP = nullptr;
				for (unsigned int WaitPassIndex = 0; WaitPassIndex < GP_Find->WAITs->size() && !break_allloops; WaitPassIndex++) {
					GFX_API::PassWait_Description& Waitdesc = (*GP_Find->WAITs)[WaitPassIndex];
					if (Waitdesc.WaitLastFramesPass
						|| FindWaitedPassType(Waitdesc.WaitedStage) == PassType::WP //I don't know if this should be here!
						) {
						LOG_NOTCODED_TAPI("I added a condition but don't think it will work!", false);
						continue;
					}
					for (unsigned int CheckedRPIndex = 0; CheckedRPIndex < RPs.size() && !break_allloops; CheckedRPIndex++) {
						VK_RenderingPath* CheckedRP = RPs[CheckedRPIndex];
						for (unsigned int CheckedPassIndex = 0; CheckedPassIndex < CheckedRP->ExecutionOrder.size() && !break_allloops; CheckedPassIndex++) {
							VK_GeneralPass* CheckedExecutionGP = CheckedRP->ExecutionOrder[CheckedPassIndex];
							if (*Waitdesc.WaitedPass == CheckedExecutionGP->Handle) {
								if (shouldattach_toaRP) {
									if (Attached_RP != CheckedRP) {
										Create_NewRP(GP_Find, RPs);
										break_allloops = true;
										continue;	//You may ask why there is a continue instead of break, because we set break_allloops to true so it will
									}
								}
								else {
									//If any RP that's created before waits for the Checked RP, that means we need to create a RP for this pass
									bool is_CheckedRP_WaitedByAnyOtherRP = false;
									for (unsigned int RPIndex = 0; RPIndex < RPs.size() && !is_CheckedRP_WaitedByAnyOtherRP && !break_allloops; RPIndex++) {
										VK_RenderingPath* waitrp_check = RPs[RPIndex];
										if (CheckedRP == waitrp_check) {
											continue;
										}
										if(!waitrp_check->ExecutionOrder.size())
										for (unsigned int lastwaitcheck_index = 0; lastwaitcheck_index < waitrp_check->ExecutionOrder[0]->WAITs->size() && !break_allloops; lastwaitcheck_index++) {
											GFX_API::PassWait_Description& lastwaitcheckdesc = (*waitrp_check->ExecutionOrder[0]->WAITs)[lastwaitcheck_index];
											if (*lastwaitcheckdesc.WaitedPass == CheckedRP->ExecutionOrder[CheckedRP->ExecutionOrder.size() - 1]->Handle) {
												is_CheckedRP_WaitedByAnyOtherRP = true;
												Create_NewRP(GP_Find, RPs);
												break_allloops = true;
												break;
											}
										}
									}
									if (!is_CheckedRP_WaitedByAnyOtherRP) {
										shouldattach_toaRP = true;
										Attached_RP = CheckedRP;
									}
								}
							}
						}
					}
				}
				//All loops before isn't broken because there is only one RP that GP should be attached
				if (!break_allloops & shouldattach_toaRP) {
					AttachGPTo_RPBack(GP_Find, Attached_RP);
				}
			}


			AttachableGP_MainLoopElement = AttachableGP_MainLoopElement->NextElement;
		}

		if (!RPless_passes) {
			return;
		}
		if (RPless_passes->NextElement || RPless_passes->CurrentGP) {
			FindPasses_dependentRPs(RPless_passes, RPs);
		}
	}
	void Create_VkFrameBuffers(VK_BranchPass* DrawPassInstance, unsigned int FrameGraphIndex) {
		if (DrawPassInstance->TYPE != PassType::DP) {
			return;
		}

		VK_DrawPass* DP = GFXHandleConverter(VK_DrawPass*, DrawPassInstance->Handle);


		vector<VkImageView> Attachments;
		for (unsigned int i = 0; i < DP->SLOTSET->PERFRAME_SLOTSETs[FrameGraphIndex].COLORSLOTs_COUNT; i++) {
			VK_Texture* VKTexture = DP->SLOTSET->PERFRAME_SLOTSETs[FrameGraphIndex].COLOR_SLOTs[i].RT;
			if (!VKTexture->ImageView) {
				LOG_CRASHING_TAPI("One of your RTs doesn't have a VkImageView! You can't use such a texture as RT. Generally this case happens when you forgot to specify your swapchain texture's usage (while creating a window).");
				return;
			}
			Attachments.push_back(VKTexture->ImageView);
		}
		if (DP->SLOTSET->PERFRAME_SLOTSETs[FrameGraphIndex].DEPTHSTENCIL_SLOT) {
			Attachments.push_back(DP->SLOTSET->PERFRAME_SLOTSETs[FrameGraphIndex].DEPTHSTENCIL_SLOT->RT->ImageView);
		}
		VkFramebufferCreateInfo FrameBuffer_ci = {};
		FrameBuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FrameBuffer_ci.renderPass = DP->RenderPassObject;
		FrameBuffer_ci.attachmentCount = Attachments.size();
		FrameBuffer_ci.pAttachments = Attachments.data();
		FrameBuffer_ci.width = DP->SLOTSET->PERFRAME_SLOTSETs[FrameGraphIndex].COLOR_SLOTs[0].RT->WIDTH;
		FrameBuffer_ci.height = DP->SLOTSET->PERFRAME_SLOTSETs[FrameGraphIndex].COLOR_SLOTs[0].RT->HEIGHT;
		FrameBuffer_ci.layers = 1;
		FrameBuffer_ci.pNext = nullptr;
		FrameBuffer_ci.flags = 0;

		if (vkCreateFramebuffer(VKGPU->Logical_Device, &FrameBuffer_ci, nullptr, &DP->FBs[FrameGraphIndex]) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Renderer::Create_DrawPassFrameBuffers() has failed!");
			return;
		}
	}
	void Create_FrameGraphs(const vector<VK_DrawPass*>& DrawPasses, const vector<VK_TransferPass*>& TransferPasses, const vector<VK_WindowPass*>& WindowPasses, VK_FrameGraph* FrameGraphs) {
		TURAN_PROFILE_SCOPE_MCS("Create_FrameGraphs()");
		vector<VK_GeneralPass> AllPasses;
		vector<VK_RenderingPath*> RPs;
		{
			//Passes in this vector means
			VK_GeneralPassLinkedList* GPHeader = Create_GPLinkedList();

			//Fill both AllPasses and RPless_passes vectors!
			//Also create root Passes
			for (unsigned char DPIndex = 0; DPIndex < DrawPasses.size(); DPIndex++) {
				VK_DrawPass* DP = DrawPasses[DPIndex];
				VK_GeneralPass GP;
				GP.TYPE = PassType::DP;
				GP.Handle = DP;
				GP.WAITs = &DP->WAITs;
				GP.PASSNAME = &DP->NAME;
				AllPasses.push_back(GP);
			}
			//Do the same thing to the WPs too
			for (unsigned char WPIndex = 0; WPIndex < WindowPasses.size(); WPIndex++) {
				VK_WindowPass* WP = WindowPasses[WPIndex];
				VK_GeneralPass GP;
				GP.TYPE = PassType::WP;
				GP.Handle = WP;
				GP.WAITs = &WP->WAITs;
				GP.PASSNAME = &WP->NAME;
				AllPasses.push_back(GP);
			}
			//Do the same thing to the TPs too
			for (unsigned char TPIndex = 0; TPIndex < TransferPasses.size(); TPIndex++) {
				VK_TransferPass* TP = TransferPasses[TPIndex];
				VK_GeneralPass GP;
				GP.TYPE = PassType::TP;
				GP.Handle = TP;
				GP.WAITs = &TP->WAITs;
				GP.PASSNAME = &TP->NAME;
				AllPasses.push_back(GP);
			}
			for (unsigned char DPIndex = 0; DPIndex < DrawPasses.size(); DPIndex++) {
				VK_DrawPass* DP = DrawPasses[DPIndex];
				//Check if it is root DP and if it is not, then pass it to the RPless_passes
				bool is_RootDP = false;
				if (DP->WAITs.size() == 0) {
					is_RootDP = true;
				}
				for (unsigned char WaitIndex = 0; WaitIndex < DP->WAITs.size(); WaitIndex++) {
					if (!DP->WAITs[WaitIndex].WaitLastFramesPass && FindWaitedPassType(DP->WAITs[WaitIndex].WaitedStage) != PassType::WP) {
						is_RootDP = false;
					}
				}
				if (is_RootDP) {
					Create_NewRP(&AllPasses[DPIndex], RPs);
				}
				else {
					PushBack_ToGPLinkedList(&AllPasses[DPIndex], GPHeader);
				}
			}
			for (unsigned char WPIndex = 0; WPIndex < WindowPasses.size(); WPIndex++) {
				VK_WindowPass* WP = WindowPasses[WPIndex];
				//Check if it is root DP and if it is not, then pass it to the RPless_passes
				bool is_RootWP = false;
				if (WP->WAITs.size() == 0) {
					is_RootWP = true;
				}
				for (unsigned char WaitIndex = 0; WaitIndex < WP->WAITs.size(); WaitIndex++) {
					if (!WP->WAITs[WaitIndex].WaitLastFramesPass && FindWaitedPassType(WP->WAITs[WaitIndex].WaitedStage) != PassType::WP) {
						is_RootWP = false;
					}
				}
				if (is_RootWP) {
					Create_NewRP(&AllPasses[WPIndex + DrawPasses.size()], RPs);
				}
				else {
					PushBack_ToGPLinkedList(&AllPasses[WPIndex + DrawPasses.size()], GPHeader);
				}
			}
			for (unsigned char TPIndex = 0; TPIndex < TransferPasses.size(); TPIndex++) {
				VK_TransferPass* TP = TransferPasses[TPIndex];
				//Check if it is root TP and if it is not, then pass it to the RPless_passes
				bool is_RootTP = false;
				if (TP->WAITs.size() == 0) {
					is_RootTP = true;
				}
				for (unsigned char WaitIndex = 0; WaitIndex < TP->WAITs.size(); WaitIndex++) {
					if (!TP->WAITs[WaitIndex].WaitLastFramesPass && FindWaitedPassType(TP->WAITs[WaitIndex].WaitedStage) != PassType::WP) {
						is_RootTP = false;
					}
				}
				if (is_RootTP) {
					Create_NewRP(&AllPasses[TPIndex + DrawPasses.size() + WindowPasses.size()], RPs);
				}
				else {
					PushBack_ToGPLinkedList(&AllPasses[TPIndex + DrawPasses.size() + WindowPasses.size()], GPHeader);
				}
			}

			vector<VK_RenderingPath*> LastFrameDependentRPs, NextFrameDependentRPs, Step3RPs;
			GPHeader = Create_LastFrameDependentRPs(GPHeader, RPs, LastFrameDependentRPs, NextFrameDependentRPs, Step3RPs);
			FindPasses_dependentRPs(GPHeader, RPs);
			Latest_LinkageProcess(LastFrameDependentRPs, NextFrameDependentRPs, Step3RPs, RPs);

			for (unsigned char RPIndex = 0; RPIndex < RPs.size(); RPIndex++) {
				VK_RenderingPath* RP = RPs[RPIndex];
				bool isSupported = false;
				for (unsigned char QUEUEIndex = 0; QUEUEIndex < VKGPU->QUEUEs.size(); QUEUEIndex++) {
					if (VKGPU->DoesQueue_Support(&VKGPU->QUEUEs[QUEUEIndex], RP->QueueFeatureRequirements)) {
						isSupported = true;
						break;
					}
				}
				if (!isSupported) {
					LOG_NOTCODED_TAPI("One of the Rendering Paths is not supported by your GPU's Vulkan Queues, this shouldn't have happened! Please contact me!", true);
					return;
				}
			}
		}
		
		
		if (RPs.size()) {
			LOG_STATUS_TAPI("Started to print RP specs!");
			for (unsigned char RPIndex = 0; RPIndex < RPs.size(); RPIndex++) {
				std::cout << "Next RP!\n";
				PrintRPSpecs(RPs[RPIndex]);
			}
		}
		else {
			LOG_STATUS_TAPI("There is no RP to print!");
		}
		
		
		//Create RGBranches
		{
			//Create Branches and VkFramebuffers, fill passes
			for (unsigned char FGIndex = 0; FGIndex < 2; FGIndex++) {
				FrameGraphs[FGIndex].BranchCount = RPs.size();
				FrameGraphs[FGIndex].FrameGraphTree = new VK_RGBranch[FrameGraphs[FGIndex].BranchCount];

				for (unsigned char RBIndex = 0; RBIndex < FrameGraphs[FGIndex].BranchCount; RBIndex++) {
					VK_RGBranch& Branch = FrameGraphs[FGIndex].FrameGraphTree[RBIndex];
					VK_RenderingPath* RP = RPs[RBIndex];

					Branch.ID = RP->ID;
					Branch.PassCount = RP->ExecutionOrder.size();
					Branch.CorePasses = new VK_BranchPass[Branch.PassCount];
					Branch.CurrentFramePassesIndexes = new unsigned char[Branch.PassCount];
					for (unsigned char PassIndex = 0; PassIndex < RP->ExecutionOrder.size(); PassIndex++) {
						VK_GeneralPass* GP = RP->ExecutionOrder[PassIndex];

						switch (GP->TYPE) {
							case PassType::DP:
							{
								Branch.CorePasses[PassIndex].TYPE = GP->TYPE;
								//Branch.CorePasses[PassIndex].Handle = GFXHandleConverter(VK_DrawPass*, RP->ExecutionOrder[0]->Handle);
								Branch.CorePasses[PassIndex].Handle = GP->Handle;
								Create_VkFrameBuffers(&Branch.CorePasses[PassIndex], FGIndex);
							}
							break;
							case PassType::TP:
							{
								Branch.CorePasses[PassIndex].TYPE = GP->TYPE;
								Branch.CorePasses[PassIndex].Handle = GP->Handle;	//Which means Handle is VK_TransferPass*
							}
							break;
							case PassType::WP:
							{
								Branch.CorePasses[PassIndex].Handle = GP->Handle;
								Branch.CorePasses[PassIndex].TYPE = GP->TYPE;
							}
							break;
							default:
							{
								LOG_NOTCODED_TAPI("Create RGBRanch doesn't support this type of TP for now!", true);
							}
							break;
						}

					}
				}
			}

			//Link RGBranches to each other across frames
			LOG_NOTCODED_TAPI("VulkanRenderer: There are unnecessary VkSemaphore creations for Barrier TPs, fix it when you have time!", false);
			LOG_NOTCODED_TAPI("VulkanRenderer: Create RGBranches's Linkage process has unnecessary loops, fix them when all the algorithm is finished!", false);
			for (unsigned char FGIndex = 0; FGIndex < 2; FGIndex++) {
				unsigned char LastFrameIndex = FGIndex ? 0 : 1;
				for (unsigned char RBIndex = 0; RBIndex < FrameGraphs[FGIndex].BranchCount; RBIndex++) {
					VK_RGBranch& FillBranch = FrameGraphs[FGIndex].FrameGraphTree[RBIndex];
					VK_RenderingPath* RP = RPs[RBIndex];

					//Count the number of Current Frame RPs (Respect to Window Pass dependencies)
					{
						unsigned int CFDRPCountNaive = RP->CFDependentRPs.size();
						for (unsigned int CFDRPIndex = 0; CFDRPIndex < RP->CFDependentRPs.size(); CFDRPIndex++) {
							VK_RenderingPath* CFDRP = RP->CFDependentRPs[CFDRPIndex];
							if (CFDRP->ExecutionOrder[0]->TYPE == PassType::WP) {
								CFDRPCountNaive--;
							}
						}
						FillBranch.CFDependentBranchCount = CFDRPCountNaive;

						FillBranch.PenultimateSwapchainBranchCount = RP->CFDependentRPs.size() - CFDRPCountNaive;
						FillBranch.PenultimateSwapchainBranches = new VK_RGBranch * [FillBranch.PenultimateSwapchainBranchCount];
					}
					FillBranch.LFDependentBranchCount = RP->LFDependentRPs.size();
					if (FillBranch.LFDependentBranchCount) {
						FillBranch.LFDependentBranches = new VK_RGBranch * [FillBranch.LFDependentBranchCount];
					}
					if (FillBranch.CFDependentBranchCount) {
						FillBranch.CFDependentBranches = new VK_RGBranch * [FillBranch.CFDependentBranchCount];
					}
					//A WP is an end point of the framegraph, so this is not Later Executes.
					if (FillBranch.CorePasses[0].TYPE != PassType::WP) {
						FillBranch.LaterExecutedBranchCount = RP->CFLaterExecutedRPs.size();
						if (FillBranch.LaterExecutedBranchCount) {
							FillBranch.LaterExecutedBranches = new VK_RGBranch * [FillBranch.LaterExecutedBranchCount];
						}
					}
					for (unsigned char LFDependentRPIndex = 0; LFDependentRPIndex < RP->LFDependentRPs.size(); LFDependentRPIndex++) {
						unsigned char SearchID = RP->LFDependentRPs[LFDependentRPIndex]->ID;
						bool is_found = false;
						for (unsigned char BranchSearchIndex = 0; BranchSearchIndex < FrameGraphs[LastFrameIndex].BranchCount; BranchSearchIndex++) {
							VK_RGBranch& LFDBranch = FrameGraphs[LastFrameIndex].FrameGraphTree[BranchSearchIndex];
							if (SearchID == LFDBranch.ID) {
								FillBranch.LFDependentBranches[LFDependentRPIndex] = &FrameGraphs[LastFrameIndex].FrameGraphTree[BranchSearchIndex];
								is_found = true;
								break;
							}
						}
					}
					{
						unsigned char CFDependentArrayIndex = 0;
						unsigned char PenultimateWindowPassIndex = 0;
						for (unsigned char CFDependentRPIndex = 0; CFDependentRPIndex < RP->CFDependentRPs.size(); CFDependentRPIndex++) {
							unsigned char SearchID = RP->CFDependentRPs[CFDependentRPIndex]->ID;
							bool is_found = false;
							for (unsigned char BranchSearchIndex = 0; BranchSearchIndex < FrameGraphs[FGIndex].BranchCount; BranchSearchIndex++) {
								VK_RGBranch& CFDBranch = FrameGraphs[FGIndex].FrameGraphTree[BranchSearchIndex];
								if (SearchID == CFDBranch.ID) {
									//It depends on penultimate window pass, so we should add id 
									if (CFDBranch.CorePasses[0].TYPE == PassType::WP) {
										FillBranch.PenultimateSwapchainBranches[PenultimateWindowPassIndex] = &FrameGraphs[FGIndex].FrameGraphTree[BranchSearchIndex];
										PenultimateWindowPassIndex++;
										is_found = true;
										break;
									}
									else {
										FillBranch.CFDependentBranches[CFDependentArrayIndex] = &FrameGraphs[FGIndex].FrameGraphTree[BranchSearchIndex];
										CFDependentArrayIndex++;
										is_found = true;
										break;
									}
								}
							}
							if (!is_found) {
								LOG_ERROR_TAPI("NFExecutedRP linkage has failed!");
							}
						}
					}
					for (unsigned char CFLaterExecutedRPIndex = 0; CFLaterExecutedRPIndex < FillBranch.LaterExecutedBranchCount; CFLaterExecutedRPIndex++) {
						unsigned char SearchID = RP->CFLaterExecutedRPs[CFLaterExecutedRPIndex]->ID;
						bool is_found = false;
						for (unsigned char BranchSearchIndex = 0; BranchSearchIndex < FrameGraphs[FGIndex].BranchCount; BranchSearchIndex++) {
							VK_RGBranch& CFLBranch = FrameGraphs[FGIndex].FrameGraphTree[BranchSearchIndex];
							if (SearchID == CFLBranch.ID) {
								FillBranch.LaterExecutedBranches[CFLaterExecutedRPIndex] = &FrameGraphs[FGIndex].FrameGraphTree[BranchSearchIndex];
								is_found = true;
								break;
							}
						}
						if (!is_found) {
							LOG_ERROR_TAPI("CFLaterExecutedRP linkage has failed!");
						}
					}
				}
			}
		}
		
		if (FrameGraphs[0].BranchCount) {
			LOG_STATUS_TAPI("Started to print RB specs!");
			for (unsigned char RBIndex = 0; RBIndex < FrameGraphs[0].BranchCount; RBIndex++) {
				std::cout << "Next RP!\n";
				PrintRBSpecs(FrameGraphs[0].FrameGraphTree[RBIndex]);
			}
		}
		else {
			LOG_STATUS_TAPI("There is no RB to print!");
		}

		for (unsigned int RPClearIndex = 0; RPClearIndex < RPs.size(); RPClearIndex++) {
			VK_RenderingPath* RP = RPs[RPClearIndex];
			delete RP;
		}
		
	}
	//Each framegraph is constructed as same, so there is no difference about passing different framegraphs here
	void Create_VkDataofRGBranches(const VK_FrameGraph& FrameGraph, vector<VK_Semaphore>& Semaphores) {
		for (unsigned char BranchIndex = 0; BranchIndex < FrameGraph.BranchCount; BranchIndex++) {
			VK_RGBranch& MainBranch = FrameGraph.FrameGraphTree[BranchIndex];

			bool isWPonly = false;
			if (MainBranch.CorePasses[0].TYPE == PassType::WP) {
				LOG_STATUS_TAPI("There is one Window Pass only branch, we don't create a CB for it!");
				isWPonly = true;
			}


			//Create Command Buffers if submit is not Window Pass only
			if (isWPonly) {
				continue;
			}

			//For each possible queue for the branch, create a command buffer in the related pool!
			for (unsigned char QueueIndex = 0; QueueIndex < VKGPU->QUEUEs.size(); QueueIndex++) {
				VK_QUEUE& VKQUEUE = VKGPU->QUEUEs[QueueIndex];
				if (!VKGPU->DoesQueue_Support(&VKGPU->QUEUEs[QueueIndex], MainBranch.CFNeeded_QueueSpecs)) {
					continue;
				}
				for (unsigned char i = 0; i < 2; i++) {
					VK_CommandPool& CP = VKQUEUE.CommandPools[i];


					VkCommandBufferAllocateInfo cb_ai = {};
					cb_ai.commandBufferCount = 1;
					cb_ai.commandPool = CP.CPHandle;
					cb_ai.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					cb_ai.pNext = nullptr;
					cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

					VkCommandBuffer NewCB;
					if (vkAllocateCommandBuffers(VKGPU->Logical_Device, &cb_ai, &NewCB) != VK_SUCCESS) {
						LOG_CRASHING_TAPI("vkAllocateCommandBuffers() failed while creating command buffers for RGBranches, report this please!");
						return;
					}
					VK_CommandBuffer VK_CB;
					VK_CB.CB = NewCB;
					VK_CB.is_Used = false;

					CP.CBs.push_back(VK_CB);
				}
			}
		}

		unsigned int SemaphoreCount = FrameGraph.BranchCount * 2;


		for (unsigned int SemaphoreIndex = 0; SemaphoreIndex < SemaphoreCount; SemaphoreIndex++) {
			VkSemaphoreCreateInfo Semaphore_ci = {};
			Semaphore_ci.flags = 0;
			Semaphore_ci.pNext = nullptr;
			Semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VkSemaphore NewSemaphore;

			if (vkCreateSemaphore(VKGPU->Logical_Device, &Semaphore_ci, nullptr, &NewSemaphore) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("vkCreateSemaphore has failed while creating semaphores of RGBranches, please report!");
				return;
			}

			VK_Semaphore data;
			data.isUsed = false;
			data.SPHandle = NewSemaphore;
			Semaphores.push_back(data);
		}
	}



	//DYNAMIC FRAMEGRAPH ALGORITHM!

	void PrintSubmits(VK_FrameGraph* CurrentFG) {
		for (unsigned int SubmitIndex = 0; SubmitIndex < CurrentFG->CurrentFrameSubmits.size(); SubmitIndex++) {
			VK_Submit* Submit = CurrentFG->CurrentFrameSubmits[SubmitIndex];
			std::cout << "Current Submit Index: " << SubmitIndex << std::endl;
			std::cout << "Queue Feature Score: " << unsigned int(Submit->Run_Queue->QueueFeatureScore) << std::endl;
			for (unsigned int SubmitBranchIndex = 0; SubmitBranchIndex < Submit->BranchIndexes.size(); SubmitBranchIndex++) {
				VK_RGBranch& Branch = CurrentFG->FrameGraphTree[Submit->BranchIndexes[SubmitBranchIndex] - 1];
				std::cout << "Branch ID: " << unsigned int(Branch.ID) << std::endl;
			}
		}
	}
	VK_QUEUE* FindAvailableQueue(VK_QUEUEFLAG FLAG) {
		VK_QUEUE* PossibleQueue = nullptr;
		for (unsigned char QueueIndex = 0; QueueIndex < VKGPU->QUEUEs.size(); QueueIndex++) {
			VK_QUEUE* Queue = &VKGPU->QUEUEs[QueueIndex];
			//As first step, try to find a queue that has no workload from last frame
			if (VKGPU->DoesQueue_Support(Queue, FLAG)) {
				if (!Queue->ActiveSubmits.size()) {
					return Queue;
				}
				//If there are multiple queues that supports the branch but both of them are busy,
				//then previous possible queue should be used because it has less feature
				else if (!PossibleQueue) {
					PossibleQueue = Queue;
				}
			}
		}
		//None of the idle queues supports this branch, so return a busy queue
		return PossibleQueue;
	}
	void EndSubmit(VK_Submit* Submit) {
		if (Submit->is_Ended) {
			return;
		}
		Submit->is_Ended = true;
		VK_QUEUE* CFDQueue = Submit->Run_Queue;
		for (unsigned char ActiveSubmitIndex = 0; ActiveSubmitIndex < CFDQueue->ActiveSubmits.size(); ActiveSubmitIndex++) {
			if (CFDQueue->ActiveSubmits[ActiveSubmitIndex] == Submit) {
				CFDQueue->ActiveSubmits.erase(CFDQueue->ActiveSubmits.begin() + ActiveSubmitIndex);
				break;
			}
		}
	}
	//Clear from the submitless branches list
	void EraseFromSubmitlessList(unsigned char BranchIndex, vector<unsigned char>& SubmitlessBranches, unsigned char& NumberofSubmitlessBranches) {
		for (unsigned char SubmitlessBranchesIndex = 0; SubmitlessBranchesIndex < SubmitlessBranches.size(); SubmitlessBranchesIndex++) {
			if (BranchIndex + 1 == SubmitlessBranches[SubmitlessBranchesIndex]) {
				SubmitlessBranches[SubmitlessBranchesIndex] = 0;
				NumberofSubmitlessBranches--;
			}
		}
	}
	//End dependent submits and create new one
	//Merge dependent branches that has no submit (That should only be the case with dependency to Barrier TP only branches)
	//Don't call this function if you are not sure to create a submit
	//That means, function never returns nullptr
	void CreateSubmit_ForRGB(VK_FrameGraph* CurrentFG, unsigned char BranchIndex, VK_FrameGraph* LastFG, VK_QUEUE* AttachQueue, vector<unsigned char>& SubmitlessBranches, unsigned char& NumberofSubmitless_RGBs) {
		VK_RGBranch& MainBranch = CurrentFG->FrameGraphTree[BranchIndex];
		VK_Submit* NewSubmit = new VK_Submit;
		NewSubmit->Run_Queue = AttachQueue;
		if (!MainBranch.DynamicLaterExecutes.size()) {
			NewSubmit->is_EndSubmit = true;
			NewSubmit->is_Ended = true;
		}
		//End Submit should be called to deactivate this submit!
		else {
			AttachQueue->ActiveSubmits.push_back(NewSubmit);
		}
		CurrentFG->CurrentFrameSubmits.push_back(NewSubmit);

		//Merge Barrier TP only branches
		if (!MainBranch.CFNeeded_QueueSpecs.is_PRESENTATIONsupported) {
			for (unsigned char CFDBVectorIndex = 0; CFDBVectorIndex < MainBranch.CFDynamicDependents.size(); CFDBVectorIndex++) {
				unsigned char CFDBIndex = MainBranch.CFDynamicDependents[CFDBVectorIndex];
				VK_RGBranch& CFDBranch = CurrentFG->FrameGraphTree[CFDBIndex];
				VK_Submit* CFDSubmit = CFDBranch.AttachedSubmit;
				//Dependent branch is Barrier TP only, so merge it
				if (!CFDSubmit) {
					NewSubmit->BranchIndexes.push_back(CFDBIndex + 1);
					CFDBranch.AttachedSubmit = NewSubmit;
					EraseFromSubmitlessList(CFDBIndex, SubmitlessBranches, NumberofSubmitless_RGBs);
					continue;
				}
				else {
					EndSubmit(CFDSubmit);
				}
			}
		}

		//Attach the main branch to new submit
		NewSubmit->BranchIndexes.push_back(BranchIndex + 1);
		EraseFromSubmitlessList(BranchIndex, SubmitlessBranches, NumberofSubmitless_RGBs);
		MainBranch.AttachedSubmit = NewSubmit;

		//Find Last Frame dependent submits to find wait semaphores
		for (unsigned char LFDBranchIndex = 0; LFDBranchIndex < MainBranch.LFDynamicDependents.size(); LFDBranchIndex++) {
			VK_Submit* LFDSubmit = LastFG->FrameGraphTree[MainBranch.LFDynamicDependents[LFDBranchIndex]].AttachedSubmit;
			NewSubmit->WaitSemaphoreIndexes.push_back(LFDSubmit->SignalSemaphoreIndex);
			//This is laziness but there is gonna probably a little gain while searching for a more "tight" one cost more 
			NewSubmit->WaitSemaphoreStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		}

		//Set wait semaphores
		//LOG_NOTCODED_TAPI("CreateSubmit_ForRGB() doesn't support Semaphore index storage, fix it!", false);
		
	}
	void AttachRGB(VK_FrameGraph* CurrentFG, unsigned char BranchIndex, VK_FrameGraph* LastFG, vector<unsigned char>& SubmitlessBranches, unsigned char& NumberofSubmitlessBranches) {
		VK_RGBranch* MainBranch = &CurrentFG->FrameGraphTree[BranchIndex];
		if (CurrentFG->FrameGraphTree[MainBranch->CFDynamicDependents[0]].AttachedSubmit->is_Ended || 
			MainBranch->CFNeeded_QueueSpecs.is_PRESENTATIONsupported ||
			!VKGPU->DoesQueue_Support(CurrentFG->FrameGraphTree[MainBranch->CFDynamicDependents[0]].AttachedSubmit->Run_Queue, MainBranch->CFNeeded_QueueSpecs)) {
			CreateSubmit_ForRGB(CurrentFG, BranchIndex, LastFG, FindAvailableQueue(MainBranch->CFNeeded_QueueSpecs), SubmitlessBranches, NumberofSubmitlessBranches);
			return;
		}
		VK_Submit* Submit = CurrentFG->FrameGraphTree[MainBranch->CFDynamicDependents[0]].AttachedSubmit;

		MainBranch->AttachedSubmit = Submit;
		Submit->BranchIndexes.push_back(BranchIndex + 1);
		if (MainBranch->DynamicLaterExecutes.size() > 1) {
			EndSubmit(Submit);
		}
		EraseFromSubmitlessList(BranchIndex, SubmitlessBranches, NumberofSubmitlessBranches);
	}
	void EndDependentSubmits(VK_FrameGraph* CurrentFG, unsigned char BranchIndex) {
		VK_RGBranch* MainBranch = &CurrentFG->FrameGraphTree[BranchIndex];
		for (unsigned char CFDIndex = 0; CFDIndex < MainBranch->CFDynamicDependents.size(); CFDIndex++) {
			EndSubmit(CurrentFG->FrameGraphTree[MainBranch->CFDynamicDependents[CFDIndex]].AttachedSubmit);
		}
	}
	/*
		1) Create submit for branches that depends on multiple branches (Barrier TP only or not) or depends on any of the last frame branches
		Because even though there are some basic cases that doesn't need multiple branches
		For example: 1 Non-BarrierTPOnly branch and 1+ BarrierTPOnly branches depend on the same branch and also executes the same branch
		There are some cases that needs multiple branches (Same example as above but they depend on different branches)
		So user should carefully design their RenderGraph and its workload to use GPU as optimal as possible
		2) Also if branch has multiple non-BarrierTPOnly branches to execute, create a submit
		3) To decide where to attach a branch, look at the branch it depends on CF. 
	*/
	void FindRGBranches_dependentSubmits(VK_FrameGraph* CurrentFG, VK_FrameGraph* LastFG, vector<unsigned char>& SubmitlessBranches, unsigned char& NumberOfSubmitlessBranches) {
		if (!NumberOfSubmitlessBranches) {
			return;
		}
		//Real indexes, so there is no sentinel
		vector<unsigned char> ProcessableBranches;
		for (unsigned char SlessBranchIndex = 0; SlessBranchIndex < SubmitlessBranches.size(); SlessBranchIndex++) {
			if (!SubmitlessBranches[SlessBranchIndex]) {
				continue;
			}
			unsigned char MainBranchIndex = SubmitlessBranches[SlessBranchIndex] - 1;
			VK_RGBranch* MainBranch = &CurrentFG->FrameGraphTree[MainBranchIndex];
			//If a CFDD branch has no submit and is not a Barrier TP only branch, skip this Main Branch
			bool DoesAnyCFDDSubmitless = false;
			for (unsigned char CFDDIndex = 0; CFDDIndex < MainBranch->CFDynamicDependents.size(); CFDDIndex++) {
				VK_RGBranch* CFDDBranch = &CurrentFG->FrameGraphTree[MainBranch->CFDynamicDependents[CFDDIndex]];
				if (!CFDDBranch->AttachedSubmit && VKGPU->Find_BestQueue(CFDDBranch->CFNeeded_QueueSpecs)) {
					DoesAnyCFDDSubmitless = true;
					break;
				}
			}
			if (DoesAnyCFDDSubmitless) {
				continue;
			}
			//Now, all of the branches that MainBranch depends on has real submits
			//Note: Barrier TP only branches ignored because CreateSubmit_ForRGB will merge them
			//So we need to store it in ProcessableBranches to decide what to do!

			ProcessableBranches.push_back(MainBranchIndex);
		}
		//Before processing the branches, search all of the dependent branches
		//If one of the dependent branch
		for (unsigned char ProccesableIndex = 0; ProccesableIndex < ProcessableBranches.size(); ProccesableIndex++) {
			unsigned char MainBranchIndex = ProcessableBranches[ProccesableIndex];
			VK_RGBranch* MainBranch = &CurrentFG->FrameGraphTree[MainBranchIndex];
			//If there are more than one dependent branches or
			//there is one branch but it executes more than one branch,
			//then end all dependent branches' submits because there should be semaphore mess
			if (MainBranch->CFDynamicDependents.size() > 1) {
				EndDependentSubmits(CurrentFG, MainBranchIndex);
			}
			else {
				if (MainBranch->CFDynamicDependents.size() == 1) {
					if (CurrentFG->FrameGraphTree[MainBranch->CFDynamicDependents[0]].DynamicLaterExecutes.size() > 1) {
						EndDependentSubmits(CurrentFG, MainBranchIndex);
					}
				}
			}
		}
		for (unsigned char ProcessableIndex = 0; ProcessableIndex < ProcessableBranches.size(); ProcessableIndex++) {
			unsigned char MainBranchIndex = ProcessableBranches[ProcessableIndex];
			VK_RGBranch* MainBranch = &CurrentFG->FrameGraphTree[MainBranchIndex];
			if (MainBranch->LFDynamicDependents.size() || MainBranch->CFDynamicDependents.size() > 1) {
				CreateSubmit_ForRGB(CurrentFG, MainBranchIndex, LastFG, FindAvailableQueue(MainBranch->CFNeeded_QueueSpecs), SubmitlessBranches, NumberOfSubmitlessBranches);
				continue;
			}
			if (MainBranch->CFDynamicDependents.size() == 1) {
				if (CurrentFG->FrameGraphTree[MainBranch->CFDynamicDependents[0]].DynamicLaterExecutes.size() > 1) {
					CreateSubmit_ForRGB(CurrentFG, MainBranchIndex, LastFG, FindAvailableQueue(MainBranch->CFNeeded_QueueSpecs), SubmitlessBranches, NumberOfSubmitlessBranches);
					continue;
				}
			}

			AttachRGB(CurrentFG, MainBranchIndex, LastFG, SubmitlessBranches, NumberOfSubmitlessBranches);
		}

		FindRGBranches_dependentSubmits(CurrentFG, LastFG, SubmitlessBranches, NumberOfSubmitlessBranches);
	}
	void SearchandLink_CFDependency(unsigned char LinkBranchIndex, unsigned char RecursiveSearchBranchIndex, VK_FrameGraph* CurrentFG, VK_FrameGraph* LastFG) {
		VK_RGBranch* LinkBranch = &CurrentFG->FrameGraphTree[LinkBranchIndex];
		VK_RGBranch* RecursiveSearchBranch = &CurrentFG->FrameGraphTree[RecursiveSearchBranchIndex];
		if (LinkBranchIndex != RecursiveSearchBranchIndex && RecursiveSearchBranch->CurrentFramePassesIndexes[0]) {
			LinkBranch->CFDynamicDependents.push_back(RecursiveSearchBranchIndex);
			RecursiveSearchBranch->DynamicLaterExecutes.push_back(LinkBranchIndex);
			return;
		}
		for (unsigned char CFDBIndex = 0; CFDBIndex < RecursiveSearchBranch->CFDependentBranchCount; CFDBIndex++) {
			VK_RGBranch* CurrentSearchBranch = RecursiveSearchBranch->CFDependentBranches[CFDBIndex];
			unsigned char CurrentSearchBranchIndex = 0;
			//Find the CurrentESearchBranch's index in FrameGraphTree
			for (unsigned char CFBIndex = 0; CFBIndex < CurrentFG->BranchCount; CFBIndex++) {
				if (&CurrentFG->FrameGraphTree[CFBIndex] == CurrentSearchBranch) {
					CurrentSearchBranchIndex = CFBIndex;
					break;
				}
			}

			//If this branch has no workloads
			if (!CurrentSearchBranch->CurrentFramePassesIndexes[0]) {
				SearchandLink_CFDependency(LinkBranchIndex, CurrentSearchBranchIndex, CurrentFG, LastFG);
				for (unsigned char LFDBIndex = 0; LFDBIndex < CurrentSearchBranch->LFDependentBranchCount; LFDBIndex++) {
					for (unsigned char LFBIndex = 0; LFBIndex < LastFG->BranchCount; LFBIndex++) {
						if (CurrentSearchBranch->LFDependentBranches[LFDBIndex] == &LastFG->FrameGraphTree[LFBIndex]) {
							LinkBranch->LFDynamicDependents.push_back(LFBIndex);
							break;
						}
					}
				}
				continue;
			}

			LinkBranch->CFDynamicDependents.push_back(CurrentSearchBranchIndex);
			CurrentSearchBranch->DynamicLaterExecutes.push_back(LinkBranchIndex);
		}
	}


	void Create_VkSubmits(VK_FrameGraph* Current_FrameGraph, VK_FrameGraph* LastFrameGraph, vector<VK_Semaphore>& Semaphores) {
		//Index + 1
		vector<unsigned char> Submitless_RGBs;
		//Count the submitless RGBs, because traversing the list to check if there is submitless RGB each time is meaningless
		unsigned char NumberOfSubmitless_RGBs = 0;

		//Step 1: Create submits for root branches (Except Barrier TP only branches)
		{
			//These root branches differs from FrameGraph's Tree by being frame workload dynamic
			//I mean, if none of the Branch B's current frame dependent branches are used, then Branch B becomes a root branch dynamically
			//By the way, all these indexes are +1. So RootBranch with index 0 is stored as 1.
			for (unsigned char BranchIndex = 0; BranchIndex < Current_FrameGraph->BranchCount; BranchIndex++) {
				VK_RGBranch& MainBranch = Current_FrameGraph->FrameGraphTree[BranchIndex];
				if (!MainBranch.CurrentFramePassesIndexes[0]) {
					continue;
				}
				SearchandLink_CFDependency(BranchIndex, BranchIndex, Current_FrameGraph, LastFrameGraph);
			}
			for(unsigned char BranchIndex = 0; BranchIndex < Current_FrameGraph->BranchCount; BranchIndex++){
				VK_RGBranch& MainBranch = Current_FrameGraph->FrameGraphTree[BranchIndex];
				if (!MainBranch.CurrentFramePassesIndexes[0]) {
					continue;
				}
				//If barrier TP only branch, skip it
				if (!VKGPU->Find_BestQueue(MainBranch.CFNeeded_QueueSpecs)) {
					Submitless_RGBs.push_back(BranchIndex + 1);
					NumberOfSubmitless_RGBs++;
					continue;
				}
				if (!MainBranch.CFDynamicDependents.size()) {
					CreateSubmit_ForRGB(Current_FrameGraph, BranchIndex, LastFrameGraph, FindAvailableQueue(MainBranch.CFNeeded_QueueSpecs), Submitless_RGBs, NumberOfSubmitless_RGBs);
					//std::cout << "Created submit for root branch: " << unsigned int(MainBranch.ID) << std::endl;
				}
				else if (MainBranch.CFNeeded_QueueSpecs.is_PRESENTATIONsupported) {
					CreateSubmit_ForRGB(Current_FrameGraph, BranchIndex, LastFrameGraph, FindAvailableQueue(MainBranch.CFNeeded_QueueSpecs), Submitless_RGBs, NumberOfSubmitless_RGBs);
				}
				//If Create_NewSubmit() attached this branch to a Submit already, skip it
				else if(!MainBranch.AttachedSubmit){
					Submitless_RGBs.push_back(BranchIndex + 1);
					NumberOfSubmitless_RGBs++;
				}
			}
		}

		//Step 2: Handle Barrier TP only branches
		{
			//If there are more than one branches that the branch depends on, then create new submit
			//Else if there is no branch to execute later or there are more than one, create a new submit
			//All other situations will be found by either FindRGBranches_dependentSubmits() or CreateSubmit_ForRGB() and merged
			//Also there are some ridiculous merge chains that can't be found by neither of them, but leave them be!
			//Because it means RenderGraph isn't created in optimal way
			for (unsigned char SubmitlessRGBIndex = 0; SubmitlessRGBIndex < Submitless_RGBs.size(); SubmitlessRGBIndex++) {
				if (!Submitless_RGBs[SubmitlessRGBIndex]) {
					continue;
				}
				unsigned char BranchIndex = Submitless_RGBs[SubmitlessRGBIndex] - 1;
				VK_RGBranch* MainRGB = &Current_FrameGraph->FrameGraphTree[BranchIndex];
				if (MainRGB->AttachedSubmit) {
					continue;
				}
				if (VKGPU->Find_BestQueue(MainRGB->CFNeeded_QueueSpecs)) {
					continue;
				}
				if (MainRGB->CFDynamicDependents.size() > 1 || MainRGB->DynamicLaterExecutes.size() != 1) {
					CreateSubmit_ForRGB(Current_FrameGraph, BranchIndex, LastFrameGraph, &VKGPU->QUEUEs[VKGPU->GRAPHICS_QUEUEIndex], Submitless_RGBs, NumberOfSubmitless_RGBs);
					continue;
				}
			}
		}


		FindRGBranches_dependentSubmits(Current_FrameGraph, LastFrameGraph, Submitless_RGBs, NumberOfSubmitless_RGBs);
		PrintSubmits(Current_FrameGraph);

		//Give a Signal Semaphore and Command Buffer to each Submit
		for (unsigned char SubmitIndex = 0; SubmitIndex < Current_FrameGraph->CurrentFrameSubmits.size(); SubmitIndex++) {
			VK_Submit* Submit = Current_FrameGraph->CurrentFrameSubmits[SubmitIndex];

			//Skip this process if Submit is WP only, because signal semaphores are set with VkAcquireNextImageKHR() at the end
			//and also VK_Submit structure only supports one signal semaphore for now
			if (Current_FrameGraph->FrameGraphTree[Submit->BranchIndexes[0] - 1].CorePasses[0].TYPE == PassType::WP) {
				continue;
			}
			for (unsigned char SemaphoreIndex = 0; SemaphoreIndex < Semaphores.size(); SemaphoreIndex++) {
				if (!Semaphores[SemaphoreIndex].isUsed) {
					Submit->SignalSemaphoreIndex = SemaphoreIndex;
					Semaphores[SemaphoreIndex].isUsed = true;
					break;
				}
			}
			VK_CommandPool& CP = Submit->Run_Queue->CommandPools[GFXRENDERER->GetCurrentFrameIndex()];
			for (unsigned char CBIndex = 0; CBIndex < CP.CBs.size(); CBIndex++) {
				VK_CommandBuffer& VK_CB = CP.CBs[CBIndex];
				if (!VK_CB.is_Used) {
					Submit->CBIndex = CBIndex;
					VK_CB.is_Used = true;
					break;
				}
			}
			if (Submit->CBIndex == 255) {
				LOG_CRASHING_TAPI("There is a problem!");
			}
		}
		//Find all Wait Semaphores of each Submit
		for (unsigned char SubmitIndex = 0; SubmitIndex < Current_FrameGraph->CurrentFrameSubmits.size(); SubmitIndex++) {
			VK_Submit* Submit = Current_FrameGraph->CurrentFrameSubmits[SubmitIndex];

			const VK_RGBranch& MainBranch = Current_FrameGraph->FrameGraphTree[Submit->BranchIndexes[0] - 1];

			//Find last frame dynamic dependent branches and get their signal semaphores to wait
			for (unsigned char LFDBIndex = 0; LFDBIndex < MainBranch.LFDynamicDependents.size(); LFDBIndex++) {
				const VK_RGBranch& LFDBranch = LastFrameGraph->FrameGraphTree[MainBranch.LFDynamicDependents[LFDBIndex]];
				//This is a window pass, so we should access to each Window to get the related semaphores
				if (LFDBranch.CorePasses[0].TYPE == PassType::WP) {
					VK_WindowPass* WP = GFXHandleConverter(VK_WindowPass*, LFDBranch.CorePasses[0].Handle);
					for (unsigned char WindowIndex = 0; WindowIndex < WP->WindowCalls[1].size(); WindowIndex++) {
						Submit->WaitSemaphoreIndexes.push_back(
							WP->WindowCalls[1][WindowIndex].Window->PresentationWaitSemaphoreIndexes[1]
						);
						Submit->WaitSemaphoreStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
					}
				}
				else {
					Submit->WaitSemaphoreIndexes.push_back(LFDBranch.AttachedSubmit->SignalSemaphoreIndex);
					//This is laziness but there is gonna probably a little gain while searching for a more "tight" one cost more 
					Submit->WaitSemaphoreStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
				}
			}
			//Find Penultimate Window Pass only branches that current submit depends on and get their signal semaphores to wait
			for (unsigned char PenultimateBIndex = 0; PenultimateBIndex < MainBranch.PenultimateSwapchainBranchCount; PenultimateBIndex++) {
				const VK_RGBranch* PenultimateWPBranch = MainBranch.PenultimateSwapchainBranches[PenultimateBIndex];
				VK_WindowPass* WP = GFXHandleConverter(VK_WindowPass*, PenultimateWPBranch->CorePasses[0].Handle);
				for (unsigned char WindowIndex = 0; WindowIndex < WP->WindowCalls[0].size(); WindowIndex++) {
					Submit->WaitSemaphoreIndexes.push_back(
						WP-> WindowCalls[0][WindowIndex].Window->PresentationWaitSemaphoreIndexes[0]
					);
					Submit->WaitSemaphoreStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
				}
			}
			//Find Current Frame dependent submits and get their signal semaphores to wait
			for (unsigned char CFDBIndex = 0; CFDBIndex < MainBranch.CFDynamicDependents.size(); CFDBIndex++) {
				const VK_RGBranch* CFDBranch = &Current_FrameGraph->FrameGraphTree[MainBranch.CFDynamicDependents[CFDBIndex]];
				Submit->WaitSemaphoreIndexes.push_back(CFDBranch->AttachedSubmit->SignalSemaphoreIndex);
				//This is laziness but there is gonna probably a little gain while searching for a more "tight" one cost more
				VkPipelineStageFlags flag = 0;
				if (CFDBranch->CFNeeded_QueueSpecs.is_TRANSFERsupported) {
					flag |= VK_PIPELINE_STAGE_TRANSFER_BIT;
				}
				if (CFDBranch->CFNeeded_QueueSpecs.is_COMPUTEsupported) {
					flag |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
				}
				if (CFDBranch->CFNeeded_QueueSpecs.is_GRAPHICSsupported) {
					flag |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
				}
				
				Submit->WaitSemaphoreStages.push_back(flag);
			}
		}
	}




}