#include "RGAlgorithm_TempDatas.h"
#define LOG VKCORE->TGFXCORE.DebugCallback
#define SEMAPHORESYS ((VK_SemaphoreSystem*)VKRENDERER->Semaphores)
#define BRANCHSYS ((VK_RGBranchSystem*)VKRENDERER->Branches)
#define SUBMITSYS ((VK_SubmitSystem*)VKRENDERER->Submits)

														//RENDERGRAPH ALGORITHM - STATIC STEP
														
//DECLARATIONs

void PrintRPSpecs(VK_RenderingPath* RP);
void PrintRBSpecs(VK_RGBranch& RB);

//Each framegraph is constructed as same, so there is no difference about passing different framegraphs here
void Create_VkDataofRGBranches(const VK_FrameGraph& FrameGraph) {
	for (unsigned char BranchIndex = 0; BranchIndex < FrameGraph.BranchCount; BranchIndex++) {
		VK_RGBranch& MainBranch = ((VK_RGBranch*)FrameGraph.FrameGraphTree)[BranchIndex];


		//For each possible queue for the branch, create a command buffer in the related pool!
		for (unsigned char QueueIndex = 0; QueueIndex < RENDERGPU->QUEUEFAMS().size(); QueueIndex++) {
			VK_QUEUEFAM& VKQUEUE = RENDERGPU->QUEUEFAMS()[QueueIndex];
			if (!RENDERGPU->DoesQueue_Support(&RENDERGPU->QUEUEFAMS()[QueueIndex], MainBranch.GetNeededQueueFlag())) {
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
				if (vkAllocateCommandBuffers(RENDERGPU->LOGICALDEVICE(), &cb_ai, &NewCB) != VK_SUCCESS) {
					printer(result_tgfx_FAIL, "vkAllocateCommandBuffers() failed while creating command buffers for RGBranches, report this please!");
					return;
				}
				VK_CommandBuffer VK_CB;
				VK_CB.CB = NewCB;
				VK_CB.is_Used = false;

				CP.CBs.push_back(VK_CB);
			}
		}
	}
}

//This structure is to create RGBranches easily
struct VK_RenderingPath {
	unsigned char ID;	//For now, I give each RenderingPath an ID to debug the algorithm
	//This passes are executed in the First-In-First-Executed order which means element 0 is executed first, then 1...
	VK_RGBranch::BranchIDType BranchIDs[2]{ VK_RGBranch::INVALID_BRANCHID };
	std::vector<VK_Pass*> ExecutionOrder;
	std::vector<VK_RenderingPath*> CFDependentRPs, LFDependentRPs, CFLaterExecutedRPs, NFExecutedRPs;
	VK_QUEUEFLAG QueueFeatureRequirements;
};


//If anything (PRESENTATION is ignored) is false, that means this pass doesn't require its existence
//If everything (PRESENTATION is ignored) is false, that means every queue type can call this pass
VK_QUEUEFLAG FindRequiredQueue_ofGP(const VK_Pass* GP) {
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
		if (((transferpass_vk*)GP)->TYPE == tgfx_transferpass_type_BARRIER) {
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
		VK_PassWaitDescription& Wait_desc = Pass->WAITs[PassWaitIndex];
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
void AttachGPTo_RPBack(VK_Pass* Pass, VK_RenderingPath* RP) {
	RP->ExecutionOrder.push_back(Pass);
	VK_QUEUEFLAG FLAG = FindRequiredQueue_ofGP(Pass);
	RP->QueueFeatureRequirements.is_COMPUTEsupported |= FLAG.is_COMPUTEsupported;
	RP->QueueFeatureRequirements.is_GRAPHICSsupported |= FLAG.is_GRAPHICSsupported;
	RP->QueueFeatureRequirements.is_TRANSFERsupported |= FLAG.is_TRANSFERsupported;
}

bool is_AnyWaitPass_Common(VK_Pass* gp1, VK_Pass* gp2) {
	for (unsigned int gp1waitIndex = 0; gp1waitIndex < gp1->WAITsCOUNT; gp1waitIndex++) {
		const VK_PassWaitDescription& gp1Wait = gp1->WAITs[gp1waitIndex];
		for (unsigned int gp2waitIndex = 0; gp2waitIndex < gp2->WAITsCOUNT; gp2waitIndex++) {
			const VK_PassWaitDescription& gp2Wait = gp2->WAITs[gp2waitIndex];
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
VK_PassLinkedList* Create_LastFrameDependentRPs(VK_PassLinkedList* ListHeader, std::vector<VK_RenderingPath*>& RPs, std::vector<VK_RenderingPath*>& LFD_RPs, std::vector<VK_RenderingPath*>& NFD_RPs, std::vector<VK_RenderingPath*>& STEP3_RPs) {
	//Step 1
	LFD_RPs.clear();
	VK_PassLinkedList* CurrentCheck = ListHeader;
	if (!CurrentCheck->CurrentGP) {
		return ListHeader;
	}
	while (CurrentCheck) {
		VK_Pass* GP = CurrentCheck->CurrentGP;
		bool is_RPCreated = false;
		for (unsigned int WaitIndex = 0; WaitIndex < GP->WAITsCOUNT && !is_RPCreated; WaitIndex++) {
			VK_PassWaitDescription& Waitdesc = GP->WAITs[WaitIndex];
			if (Waitdesc.WaitLastFramesPass || (*Waitdesc.WaitedPass)->TYPE == PassType::WP) {
				LFD_RPs.push_back(Create_NewRP(GP, RPs));
				is_RPCreated = true;
				ListHeader = DeleteP_FromPLinkedList(ListHeader, GP);
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
		VK_Pass* GP = CurrentCheck->CurrentGP;
		bool is_RPCreated = false;
		for (unsigned int LFDRPIndex = 0; LFDRPIndex < LFD_RPs.size() && !is_RPCreated; LFDRPIndex++) {
			for (unsigned int LFDRPWaitIndex = 0; LFDRPWaitIndex < LFD_RPs[LFDRPIndex]->ExecutionOrder[0]->WAITsCOUNT; LFDRPWaitIndex++) {
				VK_PassWaitDescription& Waitdesc = LFD_RPs[LFDRPIndex]->ExecutionOrder[0]->WAITs[LFDRPWaitIndex];
				if ((Waitdesc.WaitLastFramesPass || (*Waitdesc.WaitedPass)->TYPE == PassType::WP) && *Waitdesc.WaitedPass == GP) {
					NFD_RPs.push_back(Create_NewRP(GP, RPs));
					is_RPCreated = true;
					ListHeader = DeleteP_FromPLinkedList(ListHeader, GP);
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
		VK_Pass* NFDRP_Pass = NFD_RPs[NFDRPIndex]->ExecutionOrder[0];
		CurrentCheck = ListHeader;

		if (!CurrentCheck) {
			continue;
		}
		//This is an exception, because passes that waits for a window pass with "WaitLastFrame == false" are already in their own RPs
		if (CurrentCheck->CurrentGP->TYPE == PassType::WP) {
			continue;
		}
		while (CurrentCheck) {
			VK_Pass* RPlessPass = CurrentCheck->CurrentGP;
			bool is_RPCreated = false;
			for (unsigned int WaitIndex = 0; WaitIndex < RPlessPass->WAITsCOUNT && !is_RPCreated; WaitIndex++) {
				VK_PassWaitDescription& Waitdesc = RPlessPass->WAITs[WaitIndex];
				if (!Waitdesc.WaitLastFramesPass && (*Waitdesc.WaitedPass)->TYPE != PassType::WP && *Waitdesc.WaitedPass == NFDRP_Pass) {
					STEP3_RPs.push_back(Create_NewRP(RPlessPass, RPs));
					is_RPCreated = true;
					ListHeader = DeleteP_FromPLinkedList(ListHeader, RPlessPass);
					break;
				}
			}

			CurrentCheck = CurrentCheck->NextElement;
		}
	}


	return ListHeader;
}

void Latest_LinkageProcess(std::vector<VK_RenderingPath*>& LFD_RPs, std::vector<VK_RenderingPath*>& NFD_RPs, std::vector<VK_RenderingPath*>& STEP3RPs, std::vector<VK_RenderingPath*>& RPs) {
	for (unsigned int LFDRPIndex = 0; LFDRPIndex < LFD_RPs.size(); LFDRPIndex++) {
		VK_RenderingPath* LFDRP = LFD_RPs[LFDRPIndex];
		VK_PassWaitDescription* WAITs = LFDRP->ExecutionOrder[0]->WAITs;
		for (unsigned int WaitIndex = 0; WaitIndex < LFDRP->ExecutionOrder[0]->WAITsCOUNT; WaitIndex++) {
			VK_PassWaitDescription& Wait = WAITs[WaitIndex];
			for (unsigned int MainListIndex = 0; MainListIndex < RPs.size(); MainListIndex++) {
				VK_RenderingPath* MainRP = RPs[MainListIndex];
				if (LFDRP == MainRP) {
					continue;
				}
				if (*Wait.WaitedPass == MainRP->ExecutionOrder[MainRP->ExecutionOrder.size() - 1]) {
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
		VK_PassWaitDescription* WAITs = NFDRP->ExecutionOrder[0]->WAITs;
		for (unsigned int WaitIndex = 0; WaitIndex < NFDRP->ExecutionOrder[0]->WAITsCOUNT; WaitIndex++) {
			VK_PassWaitDescription& Wait = WAITs[WaitIndex];
			for (unsigned int MainRPIndex = 0; MainRPIndex < RPs.size(); MainRPIndex++) {
				VK_RenderingPath* MainRP = RPs[MainRPIndex];
				if (NFDRP == MainRP) {
					continue;
				}
				if (*Wait.WaitedPass == MainRP->ExecutionOrder[MainRP->ExecutionOrder.size() - 1]) {
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
		VK_PassWaitDescription* WAITs = STEP3RP->ExecutionOrder[0]->WAITs;
		for (unsigned int WaitIndex = 0; WaitIndex < STEP3RP->ExecutionOrder[0]->WAITsCOUNT; WaitIndex++) {
			VK_PassWaitDescription& Wait = WAITs[WaitIndex];
			for (unsigned int MainRPIndex = 0; MainRPIndex < RPs.size(); MainRPIndex++) {
				VK_RenderingPath* MainRP = RPs[MainRPIndex];
				if (STEP3RP == MainRP) {
					continue;
				}
				if (*Wait.WaitedPass == MainRP->ExecutionOrder[MainRP->ExecutionOrder.size() - 1]) {
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
void FindPasses_dependentRPs(VK_PassLinkedList* RPless_passes, std::vector<VK_RenderingPath*>& RPs) {
	if (!RPless_passes) {
		return;
	}
	if (!RPless_passes->CurrentGP) {
		return;
	}
	//These are the passes that all of their wait passes are in RPs, so store them to create new RPs
	VK_PassLinkedList* AttachableGPs_ListHeader = Create_PLinkedList();

	VK_PassLinkedList* MainLoop_Element = RPless_passes;
	while (MainLoop_Element) {
		VK_Pass* GP_Find = MainLoop_Element->CurrentGP;
		{
			bool DoesAnyWaitPass_RPless = false;
			bool should_addinnextiteration = false;
			for (unsigned int WaitIndex = 0; WaitIndex < GP_Find->WAITsCOUNT && !DoesAnyWaitPass_RPless; WaitIndex++) {
				VK_PassWaitDescription& Wait_Desc = GP_Find->WAITs[WaitIndex];

				VK_PassLinkedList* WaitLoop_Element = RPless_passes;
				while (WaitLoop_Element && !DoesAnyWaitPass_RPless) {
					VK_Pass* GP_WaitCheck = WaitLoop_Element->CurrentGP;
					if (WaitLoop_Element == MainLoop_Element) {
						//Here is empty because we don't to do anything if wait and main loop elements are the same, please don't change the code!
					}
					else
						if (*Wait_Desc.WaitedPass == GP_WaitCheck
							&& !Wait_Desc.WaitLastFramesPass
							)
						{
							DoesAnyWaitPass_RPless = true;
						}

					WaitLoop_Element = WaitLoop_Element->NextElement;
				}

				VK_PassLinkedList* AttachableGPs_CheckLoopElement = AttachableGPs_ListHeader;
				while (AttachableGPs_CheckLoopElement && !should_addinnextiteration && !DoesAnyWaitPass_RPless) {
					if (AttachableGPs_CheckLoopElement->CurrentGP) {
						VK_Pass* AttachableGP_WaitCheck = AttachableGPs_CheckLoopElement->CurrentGP;
						if (*Wait_Desc.WaitedPass == AttachableGP_WaitCheck
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
				PushBack_ToPLinkedList(GP_Find, AttachableGPs_ListHeader);	//We don't care storing the last element because AttachableGPs_CheckLoop should check all the elements
				RPless_passes = DeleteP_FromPLinkedList(RPless_passes, GP_Find);
			}
		}

		MainLoop_Element = MainLoop_Element->NextElement;
	}

	//Attach GPs to RPs or Create RPs for GPs
	VK_PassLinkedList* AttachableGP_MainLoopElement = AttachableGPs_ListHeader;
	while (AttachableGP_MainLoopElement) {
		VK_Pass* GP_Find = AttachableGP_MainLoopElement->CurrentGP;
		bool doeshave_anycommonWaitPass = false;
		VK_PassLinkedList* CommonlyWaited_Passes = Create_PLinkedList();
		VK_PassLinkedList* AttachableGP_CheckLoopElement = AttachableGPs_ListHeader;
		while (AttachableGP_CheckLoopElement) {
			VK_Pass* GP_Check = AttachableGP_CheckLoopElement->CurrentGP;
			if (AttachableGP_CheckLoopElement != AttachableGP_MainLoopElement && is_AnyWaitPass_Common(GP_Find, GP_Check)) {
				doeshave_anycommonWaitPass = true;
				PushBack_ToPLinkedList(GP_Check, CommonlyWaited_Passes);
			}

			AttachableGP_CheckLoopElement = AttachableGP_CheckLoopElement->NextElement;
		}
		if (doeshave_anycommonWaitPass) {
			PushBack_ToPLinkedList(GP_Find, CommonlyWaited_Passes);
			VK_PassLinkedList* CommonlyWaitedPasses_CreateElement = CommonlyWaited_Passes;
			while (CommonlyWaitedPasses_CreateElement->NextElement) {
				Create_NewRP(CommonlyWaitedPasses_CreateElement->CurrentGP, RPs);
				CommonlyWaitedPasses_CreateElement = CommonlyWaitedPasses_CreateElement->NextElement;
			}
		}
		else {
			bool break_allloops = false;
			bool shouldattach_toaRP = false;
			VK_RenderingPath* Attached_RP = nullptr;
			for (unsigned int WaitPassIndex = 0; WaitPassIndex < GP_Find->WAITsCOUNT && !break_allloops; WaitPassIndex++) {
				VK_PassWaitDescription& Waitdesc = GP_Find->WAITs[WaitPassIndex];
				if (Waitdesc.WaitLastFramesPass
					|| (*Waitdesc.WaitedPass)->TYPE == PassType::WP //I don't know if this should be here!
					) {
					printer(result_tgfx_NOTCODED, "I added a condition but don't think it will work!");
					continue;
				}
				for (unsigned int CheckedRPIndex = 0; CheckedRPIndex < RPs.size() && !break_allloops; CheckedRPIndex++) {
					VK_RenderingPath* CheckedRP = RPs[CheckedRPIndex];
					for (unsigned int CheckedPassIndex = 0; CheckedPassIndex < CheckedRP->ExecutionOrder.size() && !break_allloops; CheckedPassIndex++) {
						VK_Pass* CheckedExecutionGP = CheckedRP->ExecutionOrder[CheckedPassIndex];
						if (*Waitdesc.WaitedPass == CheckedExecutionGP) {
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
									if (!waitrp_check->ExecutionOrder.size())
										for (unsigned int lastwaitcheck_index = 0; lastwaitcheck_index < waitrp_check->ExecutionOrder[0]->WAITsCOUNT && !break_allloops; lastwaitcheck_index++) {
											VK_PassWaitDescription& lastwaitcheckdesc = waitrp_check->ExecutionOrder[0]->WAITs[lastwaitcheck_index];
											if (*lastwaitcheckdesc.WaitedPass == CheckedRP->ExecutionOrder[CheckedRP->ExecutionOrder.size() - 1]) {
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
void Create_VkFrameBuffers(drawpass_vk* DP, unsigned int FrameGraphIndex) {
	std::vector<VkImageView> Attachments;
	for (unsigned int i = 0; i < DP->SLOTSET->PERFRAME_SLOTSETs[FrameGraphIndex].COLORSLOTs_COUNT; i++) {
		VK_Texture* VKTexture = DP->SLOTSET->PERFRAME_SLOTSETs[FrameGraphIndex].COLOR_SLOTs[i].RT;
		if (!VKTexture->ImageView) {
			printer(result_tgfx_FAIL, "One of your RTs doesn't have a VkImageView! You can't use such a texture as RT. Generally this case happens when you forgot to specify your swapchain texture's usage (while creating a window).");
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

	if (vkCreateFramebuffer(RENDERGPU->LOGICALDEVICE(), &FrameBuffer_ci, nullptr, &DP->FBs[FrameGraphIndex]) != VK_SUCCESS) {
		printer(result_tgfx_FAIL, "Renderer::Create_DrawPassFrameBuffers() has failed!");
		return;
	}
}

inline VK_RGBranch::BranchIDType* GetFrameGraphTree(VK_FrameGraph& fg);
inline const VK_RGBranch::BranchIDType* ReadBranchList(const VK_FrameGraph& fg);
inline std::vector<SubmitIDType>& GetFrameGraphSubmits(VK_FrameGraph& fg);

void Create_FrameGraphs() {
	unsigned long long duration = 0;
	TURAN_PROFILE_SCOPE_MCS("Create_FrameGraphs()", &duration)
		std::vector<VK_Pass*> AllPasses;
	std::vector<VK_RenderingPath*> RPs;
	{
		//Passes in this std::vector means
		VK_PassLinkedList* GPHeader = Create_PLinkedList();

		//Fill both AllPasses and RPless_passes std::vectors!
		//Also create root Passes
		for (unsigned char DPIndex = 0; DPIndex < VKRENDERER->DrawPasses.size(); DPIndex++) {
			drawpass_vk* DP = VKRENDERER->DrawPasses[DPIndex];
			AllPasses.push_back(DP);
		}
		//Do the same thing to the WPs too
		for (unsigned char WPIndex = 0; WPIndex < VKRENDERER->WindowPasses.size(); WPIndex++) {
			windowpass_vk* WP = VKRENDERER->WindowPasses[WPIndex];
			AllPasses.push_back(WP);
		}
		//Do the same thing to the TPs too
		for (unsigned char TPIndex = 0; TPIndex < VKRENDERER->TransferPasses.size(); TPIndex++) {
			transferpass_vk* TP = VKRENDERER->TransferPasses[TPIndex];
			AllPasses.push_back(TP);
		}
		//Do the same thing to the CPs too
		for (unsigned char CPIndex = 0; CPIndex < VKRENDERER->ComputePasses.size(); CPIndex++) {
			computepass_vk* CP = VKRENDERER->ComputePasses[CPIndex];
			AllPasses.push_back(CP);
		}
		for (unsigned char DPIndex = 0; DPIndex < VKRENDERER->DrawPasses.size(); DPIndex++) {
			drawpass_vk* DP = VKRENDERER->DrawPasses[DPIndex];
			//Check if it is root DP and if it is not, then pass it to the RPless_passes
			bool is_RootDP = false;
			if (DP->WAITsCOUNT == 0) {
				is_RootDP = true;
			}
			for (unsigned char WaitIndex = 0; WaitIndex < DP->WAITsCOUNT; WaitIndex++) {
				if (!DP->WAITs[WaitIndex].WaitLastFramesPass && (*DP->WAITs[WaitIndex].WaitedPass)->TYPE != PassType::WP) {
					is_RootDP = false;
				}
			}
			if (is_RootDP) {
				Create_NewRP(AllPasses[DPIndex], RPs);
			}
			else {
				PushBack_ToPLinkedList(AllPasses[DPIndex], GPHeader);
			}
		}
		for (unsigned char WPIndex = 0; WPIndex < VKRENDERER->WindowPasses.size(); WPIndex++) {
			windowpass_vk* WP = VKRENDERER->WindowPasses[WPIndex];
			//Check if it is root DP and if it is not, then pass it to the RPless_passes
			bool is_RootWP = false;
			if (WP->WAITsCOUNT == 0) {
				is_RootWP = true;
			}
			for (unsigned char WaitIndex = 0; WaitIndex < WP->WAITsCOUNT; WaitIndex++) {
				if (!WP->WAITs[WaitIndex].WaitLastFramesPass && (*WP->WAITs[WaitIndex].WaitedPass)->TYPE != PassType::WP) {
					is_RootWP = false;
				}
			}
			if (is_RootWP) {
				Create_NewRP(AllPasses[WPIndex + VKRENDERER->DrawPasses.size()], RPs);
			}
			else {
				PushBack_ToPLinkedList(AllPasses[WPIndex + VKRENDERER->DrawPasses.size()], GPHeader);
			}
		}
		for (unsigned char TPIndex = 0; TPIndex < VKRENDERER->TransferPasses.size(); TPIndex++) {
			transferpass_vk* TP = VKRENDERER->TransferPasses[TPIndex];
			//Check if it is root TP and if it is not, then pass it to the RPless_passes
			bool is_RootTP = false;
			if (TP->WAITsCOUNT == 0) {
				is_RootTP = true;
			}
			for (unsigned char WaitIndex = 0; WaitIndex < TP->WAITsCOUNT; WaitIndex++) {
				if (!TP->WAITs[WaitIndex].WaitLastFramesPass && (*TP->WAITs[WaitIndex].WaitedPass)->TYPE != PassType::WP) {
					is_RootTP = false;
				}
			}
			if (is_RootTP) {
				Create_NewRP(AllPasses[TPIndex + VKRENDERER->DrawPasses.size() + VKRENDERER->WindowPasses.size()], RPs);
			}
			else {
				PushBack_ToPLinkedList(AllPasses[TPIndex + VKRENDERER->DrawPasses.size() + VKRENDERER->WindowPasses.size()], GPHeader);
			}
		}
		for (unsigned char CPIndex = 0; CPIndex < VKRENDERER->ComputePasses.size(); CPIndex++) {
			computepass_vk* CP = VKRENDERER->ComputePasses[CPIndex];
			//Check if it is root CP and if it is not, then pass it to the RPless_passes
			bool is_RootCP = false;
			if (CP->WAITsCOUNT == 0) {
				is_RootCP = true;
			}
			for (unsigned char WaitIndex = 0; WaitIndex < CP->WAITsCOUNT; WaitIndex++) {
				if (!CP->WAITs[WaitIndex].WaitLastFramesPass && (*CP->WAITs[WaitIndex].WaitedPass)->TYPE != PassType::WP) {
					is_RootCP = false;
				}
			}
			if (is_RootCP) {
				Create_NewRP(AllPasses[CPIndex + VKRENDERER->DrawPasses.size() + VKRENDERER->WindowPasses.size() + VKRENDERER->TransferPasses.size()], RPs);
			}
			else {
				PushBack_ToPLinkedList(AllPasses[CPIndex + VKRENDERER->DrawPasses.size() + VKRENDERER->WindowPasses.size() + VKRENDERER->TransferPasses.size()], GPHeader);
			}
		}

		std::vector<VK_RenderingPath*> LastFrameDependentRPs, NextFrameDependentRPs, Step3RPs;
		GPHeader = Create_LastFrameDependentRPs(GPHeader, RPs, LastFrameDependentRPs, NextFrameDependentRPs, Step3RPs);
		FindPasses_dependentRPs(GPHeader, RPs);
		Latest_LinkageProcess(LastFrameDependentRPs, NextFrameDependentRPs, Step3RPs, RPs);

		for (unsigned char RPIndex = 0; RPIndex < RPs.size(); RPIndex++) {
			VK_RenderingPath* RP = RPs[RPIndex];
			bool isSupported = false;
			for (unsigned char QUEUEIndex = 0; QUEUEIndex < RENDERGPU->QUEUEFAMS().size(); QUEUEIndex++) {
				if (RENDERGPU->DoesQueue_Support(&RENDERGPU->QUEUEFAMS()[QUEUEIndex], RP->QueueFeatureRequirements)) {
					isSupported = true;
					break;
				}
			}
			if (!isSupported) {
				printer(result_tgfx_NOTCODED, "One of the Rendering Paths is not supported by your GPU's Vulkan Queues, this shouldn't have happened! Please contact me!");
				return;
			}
		}
	}


	if (RPs.size()) {
		printer(result_tgfx_SUCCESS, "Started to print RP specs!");
		for (unsigned char RPIndex = 0; RPIndex < RPs.size(); RPIndex++) {
			std::cout << "Next RP!\n";
			PrintRPSpecs(RPs[RPIndex]);
		}
	}
	else {
		printer(result_tgfx_SUCCESS, "There is no RP to print!");
	}


	//Create RGBranches from RenderingPath data list
	unsigned char CurrentFrameIndex = VKRENDERER->Get_FrameIndex(false), LastFrameIndex = VKRENDERER->Get_FrameIndex(true);
	VK_FrameGraph& CurrentFG = VKRENDERER->FrameGraphs[CurrentFrameIndex];
	{
		if (CurrentFG.BranchCount || CurrentFG.FrameGraphTree) {
			delete[] CurrentFG.FrameGraphTree;	//Delete previously allocated branch list
		}
		if (CurrentFG.SubmitList) {
			delete[] CurrentFG.SubmitList;		//Delete previously allocated submit list
		}
		//Create Branches and VkFramebuffers, fill passes
		CurrentFG.BranchCount = RPs.size();
		CurrentFG.FrameGraphTree = new VK_RGBranch::BranchIDType[CurrentFG.BranchCount];

		//Create and fill current framegraph's branches
		for (VK_RGBranch::BranchIDListType RBIndex = 0; RBIndex < CurrentFG.BranchCount; RBIndex++) {
			VK_RenderingPath* RP = RPs[RBIndex];
			VK_RGBranch& Branch = VK_RGBranchSystem::CreateBranch(RP->ExecutionOrder.size());

			VK_RGBranch::BranchIDType BranchID = Branch.GetID();
			GetFrameGraphTree(CurrentFG)[RBIndex] = BranchID;
			RP->BranchIDs[0] = BranchID;
			for (unsigned char PassIndex = 0; PassIndex < RP->ExecutionOrder.size(); PassIndex++) {
				VK_Pass* GP = RP->ExecutionOrder[PassIndex];
				Branch.PushBack_CorePass(GP, GP->TYPE);
			}
		}
		//Create branches for the next frame, because the two will link
		//These will be deleted 3 frames later if rendergraph is gonna be reconstructed next frame
		for (VK_RGBranch::BranchIDListType RBIndex = 0; RBIndex < CurrentFG.BranchCount; RBIndex++) {
			VK_RenderingPath* RP = RPs[RBIndex];
			VK_RGBranch& Branch = VK_RGBranchSystem::CreateBranch(RP->ExecutionOrder.size());

			VK_RGBranch::BranchIDType BranchID = Branch.GetID();
			RP->BranchIDs[1] = BranchID;
			Branch.SetSiblingID(RP->BranchIDs[0]);	//Store sibling branches in each other
			for (unsigned char PassIndex = 0; PassIndex < RP->ExecutionOrder.size(); PassIndex++) {
				VK_Pass* GP = RP->ExecutionOrder[PassIndex];
				Branch.PushBack_CorePass(GP, GP->TYPE);
			}
		}

		printer(result_tgfx_FAIL, "VulkanRenderer: There are unnecessary VkSemaphore creations for Barrier TPs, fix it when you have time!");


		//Link RGBranches to each other across frames
		for (VK_RGBranch::BranchIDListType RBIndex = 0; RBIndex < CurrentFG.BranchCount; RBIndex++) {
			VK_RGBranch& FillBranch = VK_RGBranchSystem::GetBranch_byID(GetFrameGraphTree(CurrentFG)[RBIndex]);
			VK_RenderingPath* RP = RPs[RBIndex];
			FillBranch.SetSiblingID(RP->BranchIDs[1]);	//Store sibling branches in each other

			//RPs store penultimate waits as CFDRP, because I didn't want to complicate the algorithm
			//And also, because I want to support only window passes as CFDRP.
			//We need to count them as seperate
			VK_RGBranch::BranchIDListType CFDependentBranchCount = RP->CFDependentRPs.size();
			for (unsigned int CFDRPIndex = 0; CFDRPIndex < RP->CFDependentRPs.size(); CFDRPIndex++) {
				VK_RenderingPath* CFDRP = RP->CFDependentRPs[CFDRPIndex];
				if (CFDRP->ExecutionOrder[0]->TYPE == PassType::WP) {
					CFDependentBranchCount--;
				}
			}
			VK_RGBranch::BranchIDListType PenultimateBranchCount = RP->CFDependentRPs.size() - CFDependentBranchCount;
			FillBranch.SetListSizes(PenultimateBranchCount, RP->LFDependentRPs.size(), CFDependentBranchCount, RP->CFLaterExecutedRPs.size());

			//Add both current frame dependent branches and penultimate branches at one loop, because RPs store them one
			//If RPs' first pass is a WindowPass, it's enough to set it as a PenultimateBranch for now
			for (unsigned int CFDependentRPIndex = 0; CFDependentRPIndex < RP->CFDependentRPs.size(); CFDependentRPIndex++) {
				VK_RGBranch::BranchIDType CFDB_ID = RP->CFDependentRPs[CFDependentRPIndex]->BranchIDs[0];
				if (RP->CFDependentRPs[CFDependentRPIndex]->ExecutionOrder[0]->TYPE == PassType::WP) {
					FillBranch.PushBack_PenultimateDependentBranch(CFDB_ID);
				}
				else {
					FillBranch.PushBack_CFDependentBranch(CFDB_ID);
				}
			}
			for (unsigned int LFDependentRPIndex = 0; LFDependentRPIndex < RP->LFDependentRPs.size(); LFDependentRPIndex++) {
				VK_RGBranch::BranchIDType LFDB_ID = RP->LFDependentRPs[LFDependentRPIndex]->BranchIDs[1];
				FillBranch.PushBack_LFDependentBranch(LFDB_ID);
			}
			for (unsigned int CFLaterExecutedRPIndex = 0; CFLaterExecutedRPIndex < RP->CFLaterExecutedRPs.size(); CFLaterExecutedRPIndex++) {
				VK_RGBranch::BranchIDType CFLaterExecutedBranchIndex = RP->CFLaterExecutedRPs[CFLaterExecutedRPIndex]->BranchIDs[0];
				FillBranch.PushBack_CFLaterExecutedBranch(CFLaterExecutedBranchIndex);
			}
		}
	}

	if (VKRENDERER->FrameGraphs[0].BranchCount) {
		printer(result_tgfx_SUCCESS, "Started to print RB specs!");
		for (unsigned char RBIndex = 0; RBIndex < VKRENDERER->FrameGraphs[0].BranchCount; RBIndex++) {
			std::cout << "Next RP!\n";
			PrintRBSpecs(VK_RGBranchSystem::GetBranch_byID(GetFrameGraphTree(CurrentFG)[RBIndex]));
		}
	}
	else {
		printer(result_tgfx_FAIL, "There is no RB to print!");
	}

	for (unsigned int RPClearIndex = 0; RPClearIndex < RPs.size(); RPClearIndex++) {
		VK_RenderingPath* RP = RPs[RPClearIndex];
		delete RP;
	}
	STOP_PROFILE_PRINTFUL_TAPI()
}

//Instead of re-running static rendergraph construction algorithm, just create a sibling of the last frame
//Because they are the same (except they point to each other, means references should be flipped)
//Call this function if you reconstructed framegraph in last frame and didn't reconstruct this frame.
void DuplicateFrameGraph(VK_FrameGraph& TargetFrameGraph, const VK_FrameGraph& SourceFrameGraph) {
	if (TargetFrameGraph.BranchCount || TargetFrameGraph.FrameGraphTree) {
		delete[] TargetFrameGraph.FrameGraphTree;	//Delete previously allocated branch list
	}
	if (TargetFrameGraph.SubmitList) {
		delete[] TargetFrameGraph.SubmitList;
	}

	TargetFrameGraph.BranchCount = SourceFrameGraph.BranchCount;
	TargetFrameGraph.FrameGraphTree = new VK_RGBranch::BranchIDType[TargetFrameGraph.BranchCount];
	VK_RGBranch::BranchIDType* TargetList = GetFrameGraphTree(TargetFrameGraph);

	//Fill framegraph tree with IDs then copy branch info with reversed references
	for (unsigned char RBIndex = 0; RBIndex < TargetFrameGraph.BranchCount; RBIndex++) {
		VK_RGBranch& SourceBranch = VK_RGBranchSystem::GetBranch_byID(ReadBranchList(SourceFrameGraph)[RBIndex]);
		VK_RGBranch& TargetBranch = VK_RGBranchSystem::GetBranch_byID(SourceBranch.GetSiblingID());
		TargetList[RBIndex] = SourceBranch.GetSiblingID();

		//Copy lists
		TargetBranch.SetListSizes(SourceBranch.Get_PenultimateDependentBranchCount(), SourceBranch.Get_LFDependentBranchCount(),
			SourceBranch.Get_CFDependentBranchCount(), SourceBranch.Get_CFLaterExecutedBranchCount());
		for (VK_RGBranch::BranchIDListType penultimate_i = 0; penultimate_i < SourceBranch.Get_PenultimateDependentBranchCount(); penultimate_i++) {
			TargetBranch.PushBack_PenultimateDependentBranch(VK_RGBranchSystem::GetBranch_byID(SourceBranch.Get_PenultimateDependentBranch_byIndex(penultimate_i)).GetSiblingID());
		}
		for (VK_RGBranch::BranchIDListType cfdependent_i = 0; cfdependent_i < SourceBranch.Get_CFDependentBranchCount(); cfdependent_i++) {
			TargetBranch.PushBack_CFDependentBranch(VK_RGBranchSystem::GetBranch_byID(SourceBranch.Get_CFDependentBranch_byIndex(cfdependent_i)).GetSiblingID());
		}
		for (VK_RGBranch::BranchIDListType lfdependent_i = 0; lfdependent_i < SourceBranch.Get_LFDependentBranchCount(); lfdependent_i++) {
			TargetBranch.PushBack_LFDependentBranch(VK_RGBranchSystem::GetBranch_byID(SourceBranch.Get_LFDependentBranch_byIndex(lfdependent_i)).GetSiblingID());
		}
		for (VK_RGBranch::BranchIDListType cflater_i = 0; cflater_i < SourceBranch.Get_CFLaterExecutedBranchCount(); cflater_i++) {
			TargetBranch.PushBack_CFLaterExecutedBranch(VK_RGBranchSystem::GetBranch_byID(SourceBranch.Get_CFLaterExecutedBranch_byIndex(cflater_i)).GetSiblingID());
		}

		if (!TargetBranch.IsValid()) {
			printer(result_tgfx_FAIL, "Duplicating has failed, target branch isn't valid!");
		}
	}
}

void PrintRPSpecs(VK_RenderingPath* RP) {
	std::cout << ("RP ID: " + std::to_string(RP->ID)) << std::endl;
	for (unsigned int GPIndex = 0; GPIndex < RP->ExecutionOrder.size(); GPIndex++) {
		std::cout << ("Execution Order: " + std::to_string(GPIndex) + " and Pass Name: " + RP->ExecutionOrder[GPIndex]->NAME) << std::endl;
	}
	for (unsigned int CFDependentRPIndex = 0; CFDependentRPIndex < RP->CFDependentRPs.size(); CFDependentRPIndex++) {
		std::cout << ("Current Frame Dependent RP ID: " + std::to_string(RP->CFDependentRPs[CFDependentRPIndex]->ID)) << std::endl;
	}
	for (unsigned int LFDependentRPIndex = 0; LFDependentRPIndex < RP->LFDependentRPs.size(); LFDependentRPIndex++) {
		std::cout << ("Last Frame Dependent RP ID: " + std::to_string(RP->LFDependentRPs[LFDependentRPIndex]->ID)) << std::endl;
	}
	for (unsigned int CFLaterExecuteRPIndex = 0; CFLaterExecuteRPIndex < RP->CFLaterExecutedRPs.size(); CFLaterExecuteRPIndex++) {
		std::cout << ("Current Frame Later Executed RP ID: " + std::to_string(RP->CFLaterExecutedRPs[CFLaterExecuteRPIndex]->ID)) << std::endl;
	}
	for (unsigned int LFLaterExecuteRPIndex = 0; LFLaterExecuteRPIndex < RP->NFExecutedRPs.size(); LFLaterExecuteRPIndex++) {
		std::cout << ("Next Frame Executed RP ID: " + std::to_string(RP->NFExecutedRPs[LFLaterExecuteRPIndex]->ID)) << std::endl;
	}
}
void PrintRBSpecs(VK_RGBranch& RB) {
	std::cout << ("Branch ID: " + VK_RGBranch::ConvertID_tostring(RB.GetID())) << std::endl;
	for (VK_RGBranch::PassIndexType GPIndex = 0; GPIndex < RB.GetPassCount(); GPIndex++) {
		VK_Pass* Pass = RB.GetCorePass(GPIndex);
		std::cout << "Execution Order: " + std::to_string(GPIndex) + " and Pass Name: " + Pass->NAME << std::endl;
	}
	for (VK_RGBranch::PassIndexType CFDependentRPIndex = 0; CFDependentRPIndex < RB.Get_CFDependentBranchCount(); CFDependentRPIndex++) {
		std::cout << "Current Frame Dependent RB ID: " << VK_RGBranch::ConvertID_tostring(RB.Get_CFDependentBranch_byIndex(CFDependentRPIndex)) << std::endl;
	}
	for (VK_RGBranch::PassIndexType LFDependentRPIndex = 0; LFDependentRPIndex < RB.Get_LFDependentBranchCount(); LFDependentRPIndex++) {
		std::cout << "Last Frame Dependent RB ID: " << VK_RGBranch::ConvertID_tostring(RB.Get_LFDependentBranch_byIndex(LFDependentRPIndex)) << std::endl;
	}
	for (VK_RGBranch::PassIndexType CFLaterExecuteRPIndex = 0; CFLaterExecuteRPIndex < RB.Get_LFDependentBranchCount(); CFLaterExecuteRPIndex++) {
		std::cout << "Current Frame Later Executed RB ID: " << VK_RGBranch::ConvertID_tostring(RB.Get_CFLaterExecutedBranch_byIndex(CFLaterExecuteRPIndex)) << std::endl;
	}
	for (VK_RGBranch::PassIndexType PenultimateDependentWPBranchIndex = 0; PenultimateDependentWPBranchIndex < RB.Get_PenultimateDependentBranchCount(); PenultimateDependentWPBranchIndex++) {
		std::cout << "Penultimate Dependent RB ID: " << VK_RGBranch::ConvertID_tostring(RB.Get_PenultimateDependentBranch_byIndex(PenultimateDependentWPBranchIndex)) << std::endl;
	}
}