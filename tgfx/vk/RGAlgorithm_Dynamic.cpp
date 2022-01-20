#include "RGAlgorithm_TempDatas.h"
#define LOG VKCORE->TGFXCORE.DebugCallback
#define SEMAPHORESYS ((VK_SemaphoreSystem*)VKRENDERER->Semaphores)
#define BRANCHSYS ((VK_RGBranchSystem*)VKRENDERER->Branches)
#define SUBMITSYS ((VK_SubmitSystem*)VKRENDERER->Submits)

															//RENDERGRAPH ALGORITHM - DYNAMIC STEP

inline VK_RGBranch::BranchIDType* GetFrameGraphTree(VK_FrameGraph& fg);
inline const VK_RGBranch::BranchIDType* ReadBranchList(const VK_FrameGraph& fg);
inline std::vector<SubmitIDType>& GetFrameGraphSubmits(VK_FrameGraph& fg);

void PrintSubmits(VK_FrameGraph& CurrentFG) {
	std::vector<SubmitIDType>& Submits = GetFrameGraphSubmits(CurrentFG);
	printer(result_tgfx_NOTCODED, "Print submit isn't coded!");
	/*
	for (unsigned int SubmitIndex = 0; SubmitIndex < Submits.size(); SubmitIndex++) {
		VK_Submit* Submit = Submits[SubmitIndex];
		std::cout << "Current Submit Index: " << SubmitIndex << std::endl;
		std::cout << "Queue Feature Score: " << unsigned int(Submit->Run_Queue->QueueFeatureScore) << std::endl;
		for (unsigned int SubmitBranchIndex = 0; SubmitBranchIndex < Submit->BranchIndexes.size(); SubmitBranchIndex++) {
			VK_RGBranch& Branch = CurrentFG->FrameGraphTree[Submit->BranchIndexes[SubmitBranchIndex] - 1];
			std::cout << "Branch ID: " << unsigned int(Branch.ID) << std::endl;
		}
	}*/
}
VK_QUEUEFAM* FindAvailableQueue(VK_QUEUEFLAG FLAG) {
	VK_QUEUEFAM* PossibleQueue = nullptr;
	if (FLAG.is_PRESENTATIONsupported && !(FLAG.is_COMPUTEsupported || FLAG.is_GRAPHICSsupported || FLAG.is_TRANSFERsupported)) {
		PossibleQueue = &RENDERGPU->QUEUEFAMS()[RENDERGPU->GRAPHICSQUEUEINDEX()];
		return PossibleQueue;
	}
	for (unsigned char QueueIndex = 0; QueueIndex < RENDERGPU->QUEUEFAMS().size(); QueueIndex++) {
		VK_QUEUEFAM* Queue = &RENDERGPU->QUEUEFAMS()[QueueIndex];
		//As first step, try to find a queue that has no workload from last frame
		if (RENDERGPU->DoesQueue_Support(Queue, FLAG)) {
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
//Clear from the submitless branches list
void EraseFromSubmitlessList(VK_RGBranch::BranchIDType BranchID, std::vector<VK_RGBranch::BranchIDType>& SubmitlessBranches) {
	for (VK_RGBranch::BranchIDListType SubmitlessBranchesIndex = 0; SubmitlessBranchesIndex < SubmitlessBranches.size(); SubmitlessBranchesIndex++) {
		if (BranchID == SubmitlessBranches[SubmitlessBranchesIndex]) {
			SubmitlessBranches[SubmitlessBranchesIndex] = VK_RGBranch::INVALID_BRANCHID;
			return;
		}
	}
#ifdef VULKAN_DEBUGGING
	printer(result_tgfx_FAIL, "BranchID isn't found in SubmitlessBranches list, so EraseFromSubmitlessList has failed!");
#endif
}
//Create a submit for the branch but don't give any queue
inline void CreateSubmit_ForRGB(VK_FrameGraph& CurrentFG, VK_RGBranch::BranchIDType BranchID, std::vector<VK_RGBranch::BranchIDType>& SubmitlessBranches) {
	VK_RGBranch& MainBranch = VK_RGBranchSystem::GetBranch_byID(BranchID);

	//There's gonna be a command buffer system, INVALID_ID will be changed then!
	VK_Submit& Submit = SUBMITSYS->CreateSubmit(VK_CommandBuffer::INVALID_ID, BranchID);

	//Submit Mark Process
	{
		const std::vector<VK_RGBranch::BranchIDType>& DynLaterExecutes = MainBranch.ReadDynamicLaterExecuteBranches();

		if (DynLaterExecutes.size() != 1) {
			Submit.Set_SubmitType(SubmitType::NoMoreBranch);
		}
	}

	//This is a algorithm correction check, nothing user related
#ifdef VULKAN_DEBUGGING
	for (VK_RGBranch::BranchIDListType CFDynDepIndex = 0; CFDynDepIndex < MainBranch.ReadCFDynamicDependentBranches().size(); CFDynDepIndex++) {
		VK_RGBranch& CFDynDepBranch = VK_RGBranchSystem::GetBranch_byID(MainBranch.ReadCFDynamicDependentBranches()[CFDynDepIndex]);
		if (CFDynDepBranch.is_AttachedSubmitValid()) {
			VK_Submit& Submit = CFDynDepBranch.Get_AttachedSubmit();
			if (Submit.Get_SubmitType() == SubmitType::NoMoreBranch) {
				printer(result_tgfx_FAIL, "Submit is assumed to have NoMoreBranch but it hasn't! Please report to the author!");
				break;
			}
		}


		//Erase the branch from submitless list, set the submit as attached submit to the branch and add submit to the framegraph submits list
		SubmitIDType SubmitID = Submit.Get_ID();
		GetFrameGraphSubmits(CurrentFG).push_back(SubmitID);
		MainBranch.SetAttachedSubmit(SubmitID);
		EraseFromSubmitlessList(MainBranch.GetID(), SubmitlessBranches);
	}
#endif
}
inline void SubmitQueueFlag_EQOR_BranchQueueFlag(VK_QUEUEFLAG& SubmitQueueFlag, VK_QUEUEFLAG BranchQueueFlag) {
	if (BranchQueueFlag.doesntNeedAnything) { return; }
	SubmitQueueFlag.is_COMPUTEsupported |= BranchQueueFlag.is_COMPUTEsupported;
	SubmitQueueFlag.is_GRAPHICSsupported |= BranchQueueFlag.is_GRAPHICSsupported;
	SubmitQueueFlag.is_PRESENTATIONsupported |= BranchQueueFlag.is_PRESENTATIONsupported;
	SubmitQueueFlag.is_TRANSFERsupported |= BranchQueueFlag.is_TRANSFERsupported;
	SubmitQueueFlag.doesntNeedAnything = false;
}
//If you are not sure to attach the branch, please be sure first!
//Adds branch to the submit list, then marks the submit according to the new list, then changes queue flags etc.
inline void AttachRGB(VK_Submit& Submit, VK_RGBranch& Branch, std::vector<VK_RGBranch::BranchIDType>& SubmitlessBranches) {
	Submit.PushBack_RGBranch(Branch.GetID());

	VK_QUEUEFLAG& submitflag = Submit.Get_QueueFlag();
	SubmitQueueFlag_EQOR_BranchQueueFlag(submitflag, Branch.GetNeededQueueFlag());


	//Mark the submit according to the branch
	switch (Branch.GetBranchType()) {
	case BranchType::DynLaterExecs:
		if (Branch.GetDynamicLaterExecuteBranches().size() > 1) {
			Submit.Set_SubmitType(SubmitType::NoMoreBranch);
		}
		break;
	case BranchType::NoDynLaterExec:
		Submit.Set_SubmitType(SubmitType::NoMoreBranch);
		break;
	default:
		printer(result_tgfx_FAIL, ("This type of branch isn't supported by the AttachRGB(). Branch Type's value is: " +
			std::to_string(static_cast<unsigned char>(Branch.GetBranchType()))).c_str());
	}


	Branch.SetAttachedSubmit(Submit.Get_ID());
	EraseFromSubmitlessList(Branch.GetID(), SubmitlessBranches);
}


//Processible branches are branches that are ready to create a submit or be attached to a submit
inline bool isBranchProcessible(VK_RGBranch& Branch) {
	switch (Branch.GetBranchType()) {
	case BranchType::DynLaterExecs:
	case BranchType::NoDynLaterExec:
		//These branches only depends on one branch (current frame), so if it's submit is valid then they're processible
		VK_RGBranch& CFDDBranch = VK_RGBranchSystem::GetBranch_byID(Branch.GetCFDynamicDependentBranches()[0]);
		if (CFDDBranch.is_AttachedSubmitValid()) {
			return true;
		}

		return false;
	default:
		printer(result_tgfx_FAIL, ("isBranchProcessible() isn't coded for this type of branch! BranchType's value is: " +
			std::to_string(static_cast<unsigned char>(Branch.GetBranchType()))).c_str());
		return false;
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
void FindRGBranches_dependentSubmits(VK_FrameGraph& CurrentFG, std::vector<VK_RGBranch::BranchIDType>& SubmitlessBranches) {
	//This is Step 2
	std::vector<VK_RGBranch&> ProcessibleBranches;
	for (VK_RGBranch::BranchIDListType SlessBranchIndex = 0; SlessBranchIndex < SubmitlessBranches.size(); SlessBranchIndex++) {
		if (SubmitlessBranches[SlessBranchIndex] == VK_RGBranch::INVALID_BRANCHID) {
			continue;
		}


		VK_RGBranch& MainBranch = VK_RGBranchSystem::GetBranch_byID(SubmitlessBranches[SlessBranchIndex]);
		if (isBranchProcessible(MainBranch)) {
			ProcessibleBranches.push_back(MainBranch);
		}
	}

	//Create submits or attach branches to submits according to processible branch info!
	//This is called BranchProcessing Stage.
	for (VK_RGBranch::BranchIDListType ProcessableIndex = 0; ProcessableIndex < ProcessibleBranches.size(); ProcessableIndex++) {
		VK_RGBranch& MainBranch = ProcessibleBranches[ProcessableIndex];

		switch (MainBranch.GetBranchType()) {
		case BranchType::DynLaterExecs:
		case BranchType::NoDynLaterExec: {
			VK_RGBranch& CFDD = VK_RGBranchSystem::GetBranch_byID(MainBranch.ReadCFDynamicDependentBranches()[0]);
			VK_Submit& Submit = CFDD.Get_AttachedSubmit();
			if (Submit.Get_SubmitType() == SubmitType::NoMoreBranch) {
				//Submit is ended, so we can only create a new submit for it
				CreateSubmit_ForRGB(CurrentFG, MainBranch.GetID(), SubmitlessBranches);
				continue;
			}

			AttachRGB(Submit, MainBranch, SubmitlessBranches);
		}
									   continue;
		default:
			printer(result_tgfx_FAIL, ("FindRGBranches_dependentSubmits() doesn't support this branch type in processing stage! Branch Type's value: " +
				std::to_string(static_cast<unsigned char>(MainBranch.GetBranchType()))).c_str());
		}
	}

	//At each recursive call, there should be at least one processible branch
	//If there isn't, that means framegraph is processed completely so just return (break all recursive calls).
	if (ProcessibleBranches.size()) {
		FindRGBranches_dependentSubmits(CurrentFG, SubmitlessBranches);
	}
}


void Find_LastFrameDynamicDependentsFast(VK_RGBranch::BranchIDType LinkBranchID, VK_RGBranch::BranchIDType LastFramesBranchID) {
	VK_RGBranch& LinkBranch = VK_RGBranchSystem::GetBranch_byID(LinkBranchID);
	VK_RGBranch& LastFrameBranch = VK_RGBranchSystem::GetBranch_byID(LastFramesBranchID);
	if (LastFrameBranch.IsWorkloaded()) {
		LinkBranch.GetLastFDynamicDBranches().push_back(LastFramesBranchID);
		return;
	}

	//If last frame dependent branch didn't have any workload,
	//find its current frame dependent branches (which are probably being executed because they are called to run) and 
	//set as last frame dynamic dependent for LinkBranch
	//To find its CFDBranches, we need to search through its static CFDBranches because it didn't have workload and its dynamic information isn't calculated 
	for (VK_RGBranch::BranchIDListType CFDBIndex = 0; CFDBIndex < LastFrameBranch.Get_CFDependentBranchCount(); CFDBIndex++) {
		Find_LastFrameDynamicDependentsFast(LinkBranchID, LastFrameBranch.Get_CFDependentBranch_byIndex(CFDBIndex));
	}
}

//This recursive function will keep searching until it finds all dependents that has workload
//But this only works for RGBranches that has copy of each other (That means, you shouldn't call this if you reconstructed the framegraph)
//Because this only checks if CFDependentBranches has workload, but reconstructed framegraph needs more precise check
void Find_DynamicDependentsFast(VK_RGBranch::BranchIDType LinkBranchID, VK_RGBranch::BranchIDType RecursiveSearchBranchID) {
	VK_RGBranch& LinkBranch = VK_RGBranchSystem::GetBranch_byID(LinkBranchID);
	VK_RGBranch& RecursiveSearchBranch = VK_RGBranchSystem::GetBranch_byID(RecursiveSearchBranchID);
	if (LinkBranchID != RecursiveSearchBranchID && RecursiveSearchBranch.IsWorkloaded()) {
		bool isFound = false;
		//Check if the recursive branch is added before, if it is then skip it
		for (VK_RGBranch::BranchIDListType i = 0; i < LinkBranch.GetPsuedoCFDynDBranches().size(); i++) {
			if (LinkBranch.GetPsuedoCFDynDBranches()[i] == RecursiveSearchBranchID) {
				isFound = true;
				break;
			}
		}
		if (!isFound) {
			LinkBranch.GetPsuedoCFDynDBranches().push_back(RecursiveSearchBranchID);
			return;
		}
	}

	//Search through all of static last frame dependent branches of the RecursiveSearchBranch
	//If static last frame dependent branches didn't have any workload, inherit their current frame (that means, last frame's) dependencies (recursive)
	for (VK_RGBranch::BranchIDListType LFDynamicDependentIndex = 0; LFDynamicDependentIndex < RecursiveSearchBranch.Get_LFDependentBranchCount(); LFDynamicDependentIndex++) {
		Find_LastFrameDynamicDependentsFast(LinkBranchID, RecursiveSearchBranch.Get_LFDependentBranch_byIndex(LFDynamicDependentIndex));
	}

	//Search through all of static current frame dependent branches of the RecursiveSearchBranch
	//If current frame dependent branches didn't have any workload, inherit their depende
	for (VK_RGBranch::BranchIDListType CFDBIndex = 0; CFDBIndex < RecursiveSearchBranch.Get_CFDependentBranchCount(); CFDBIndex++) {
		VK_RGBranch& CurrentSearchBranch = VK_RGBranchSystem::GetBranch_byID(RecursiveSearchBranch.Get_CFDependentBranch_byIndex(CFDBIndex));
		Find_DynamicDependentsFast(LinkBranchID, CurrentSearchBranch.GetID());
	}

}

//This function tries to find recursively if Branch is already waited from any branch in the RecursiveList
//This is recursive because maybe a branch waits for it but only by waiting another branch.
//If isCF is true, then searches for CFDynDepBranches; searches for LFDynDepBranches otherwise
bool IsBranchAlreadyDepted_toPsuedoList(VK_RGBranch& MainBranch, std::vector<VK_RGBranch::BranchIDType>& RecursiveList, bool isCF) {
	bool isThereAnyDepBranch = false;
	for (VK_RGBranch::BranchIDListType SearchListIndex = 0; SearchListIndex < RecursiveList.size(); SearchListIndex++) {
		VK_RGBranch& SearchBranch = VK_RGBranchSystem::GetBranch_byID(RecursiveList[SearchListIndex]);
		if (SearchBranch.GetID() == MainBranch.GetID()) {
			continue;
		}

		std::vector<VK_RGBranch::BranchIDType>& PsuedoBranchList = (isCF) ? (SearchBranch.GetPsuedoCFDynDBranches()) : (SearchBranch.GetPsuedoLFDynDBranches());

		if (PsuedoBranchList.size()) {
			isThereAnyDepBranch = true;
		}
		for (VK_RGBranch::BranchIDListType PsuedoBranchIndex = 0; PsuedoBranchIndex < PsuedoBranchList.size(); PsuedoBranchIndex++) {
			if (PsuedoBranchList[SearchListIndex] == MainBranch.GetID()) {
				return true;
			}
		}
	}
	if (!isThereAnyDepBranch) {
		return false;
	}
	for (VK_RGBranch::BranchIDListType SearchListIndex = 0; SearchListIndex < RecursiveList.size(); SearchListIndex++) {
		VK_RGBranch& SearchBranch = VK_RGBranchSystem::GetBranch_byID(RecursiveList[SearchListIndex]);
		if (SearchBranch.GetID() == MainBranch.GetID()) {
			continue;
		}
		std::vector<VK_RGBranch::BranchIDType>& PsuedoBranchList = (isCF) ? (SearchBranch.GetPsuedoCFDynDBranches()) : (SearchBranch.GetPsuedoLFDynDBranches());
		if (IsBranchAlreadyDepted_toPsuedoList(MainBranch, PsuedoBranchList, isCF)) {
			return true;
		}
	}
	return false;
}


//When all dynamic deps are found (which unnecessary deps), check temporary cfdyndepslist and lfdyndepslist for unnecessary deps
//then push the fixed list to the related branch lists, then link cfdyndeps with dynlaterexecutes
//then find a processible type for the branch
void Fix_PsuedoDynamicLists(VK_RGBranch::BranchIDType* BranchList, VK_RGBranch::BranchIDListType BranchCount) {
	for (VK_RGBranch::BranchIDListType i = 0; i < BranchCount; i++) {
		VK_RGBranch& MainBranch = VK_RGBranchSystem::GetBranch_byID(BranchList[i]);


		for (VK_RGBranch::BranchIDListType CFDepSearchIndex = 0; CFDepSearchIndex < MainBranch.GetPsuedoCFDynDBranches().size(); CFDepSearchIndex++) {
			VK_RGBranch& CFDynDepBranch = VK_RGBranchSystem::GetBranch_byID(MainBranch.GetPsuedoCFDynDBranches()[CFDepSearchIndex]);
			if (!IsBranchAlreadyDepted_toPsuedoList(CFDynDepBranch, MainBranch.GetPsuedoCFDynDBranches(), true)) {
				MainBranch.GetCFDynamicDependentBranches().push_back(CFDynDepBranch.GetID());
				CFDynDepBranch.GetDynamicLaterExecuteBranches().push_back(MainBranch.GetID());
			}
		}

		for (VK_RGBranch::BranchIDListType LFDepSearchIndex = 0; LFDepSearchIndex < MainBranch.GetPsuedoLFDynDBranches().size(); LFDepSearchIndex++) {
			VK_RGBranch& LFDynDepBranch = VK_RGBranchSystem::GetBranch_byID(MainBranch.GetPsuedoLFDynDBranches()[LFDepSearchIndex]);
			if (!IsBranchAlreadyDepted_toPsuedoList(LFDynDepBranch, MainBranch.GetPsuedoLFDynDBranches(), false)) {
				MainBranch.GetLastFDynamicDBranches().push_back(LFDynDepBranch.GetID());
			}
		}


		MainBranch.FindProcessibleType();
	}
}

//Link submits to each other, then find the best queue for them. Also define the order of queue send order
inline void LinkSubmits(std::vector<SubmitIDType>& SubmitList) {
	for (unsigned int SubmitIndex = 0; SubmitIndex < SubmitList.size(); SubmitIndex++) {
		VK_Submit& Submit = VK_SubmitSystem::GetSubmit_byID(SubmitList[SubmitIndex]);
		const std::vector<VK_RGBranch::BranchIDType>& ActiveBranches = Submit.Get_ActiveBranchList();
		VK_RGBranch& StartBranch = VK_RGBranchSystem::GetBranch_byID(ActiveBranches[0]);
		VK_RGBranch& LastBranch = VK_RGBranchSystem::GetBranch_byID(ActiveBranches[ActiveBranches.size() - 1]);


		//Set the type of the submit
		switch (StartBranch.GetBranchType()) {
		case BranchType::RootBranch:
			SubmitType TypeofSubmit = LastBranch.ReadDynamicLaterExecuteBranches().size() ? SubmitType::Root_NonEndSubmit : SubmitType::Root_EndSubmit;
			Submit.Set_SubmitType(TypeofSubmit);
			break;
		case BranchType::SeperateBranch:
		case BranchType::DynLaterExecs:
		case BranchType::NoDynLaterExec:
			//Set the type of the submit
			SubmitType TypeofSubmit = LastBranch.ReadDynamicLaterExecuteBranches().size() ? SubmitType::NonRoot_NonEndSubmit : SubmitType::NonRoot_EndSubmit;
			Submit.Set_SubmitType(TypeofSubmit);
			break;
		default:
			printer(result_tgfx_FAIL, ("LinkSubmits() has failed because StartBranch's Type is unsupported. Type's value is: " +
				std::to_string(static_cast<unsigned char>(StartBranch.GetBranchType()))).c_str());
		}

		const std::vector<VK_RGBranch::BranchIDType>& CFDynDepsList = StartBranch.ReadCFDynamicDependentBranches();
		for (VK_RGBranch::BranchIDListType CFDynDepIndex = 0; CFDynDepIndex < CFDynDepsList.size(); CFDynDepIndex++) {
			VK_RGBranch& CFDynDepBranch = VK_RGBranchSystem::GetBranch_byID(CFDynDepsList[CFDynDepIndex]);
#ifdef VULKAN_DEBUGGING
			if (!CFDynDepBranch.is_AttachedSubmitValid())
			{
				printer(result_tgfx_FAIL, "CFDynDepBranch shouldn't have an invalid attachedsubmit, please report this to the author!");
			}
#endif
			VK_Submit& CFDynSubmit = CFDynDepBranch.Get_AttachedSubmit();
			printer(result_tgfx_SUCCESS, "I left submit dependency as VK_PIPELINE_STAGE_ALL_COMMANDS_BIT but it should be more precise!");
			Submit.PushBack_WaitInfo(CFDynSubmit.Get_ID(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, SubmitWaitType::CURRENTFRAME);
		}

		const std::vector<VK_RGBranch::BranchIDType>& LFDynDepsList = StartBranch.ReadLastFDynamicDBranches();
		for (VK_RGBranch::BranchIDListType LFDynDepIndex = 0; LFDynDepIndex < LFDynDepsList.size(); LFDynDepIndex++) {
			VK_RGBranch& LFDynDepBranch = VK_RGBranchSystem::GetBranch_byID(LFDynDepsList[LFDynDepIndex]);
#ifdef VULKAN_DEBUGGING
			if (!LFDynDepBranch.is_AttachedSubmitValid())
			{
				printer(result_tgfx_FAIL, "LFDynDepBranch shouldn't have an invalid attachedsubmit, please report this to the author!");
			}
#endif

			VK_Submit& LFDynSubmit = LFDynDepBranch.Get_AttachedSubmit();
			printer(result_tgfx_SUCCESS, "I left submit dependency as VK_PIPELINE_STAGE_ALL_COMMANDS_BIT but it should be more precise!");
			Submit.PushBack_WaitInfo(LFDynSubmit.Get_ID(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, SubmitWaitType::LASTFRAME);
		}
	}
	for (unsigned int SubmitIndex = 0; SubmitIndex < SubmitList.size(); SubmitIndex++) {
		FindProperQueue();
	}
}

inline void CreateNullSubmits(std::vector<SubmitIDType>& SubmitList) {

}

//This only works for RGBranches that has copy of each other (That means, you shouldn't call this if you reconstructed the framegraph in this frame)
inline void CreateSubmits_Fast(VK_FrameGraph& Current_FrameGraph, VK_FrameGraph& LastFrameGraph) {
	std::vector<VK_RGBranch::BranchIDType> Submitless_RGBs;
	//Count the submitless RGBs, because traversing the list to check if there is submitless RGB each time is meaningless
	VK_RGBranch::BranchIDListType NumberOfSubmitless_RGBs = 0;

	//Stage 0: Find dynamic dependencies between branches
	for (VK_RGBranch::BranchIDListType BranchIndex = 0; BranchIndex < Current_FrameGraph.BranchCount; BranchIndex++) {
		VK_RGBranch& MainBranch = VK_RGBranchSystem::GetBranch_byID(GetFrameGraphTree(Current_FrameGraph)[BranchIndex]);
		if (!MainBranch.IsWorkloaded()) {
			continue;
		}
		Find_DynamicDependentsFast(MainBranch.GetID(), MainBranch.GetID());
	}
	Fix_PsuedoDynamicLists(GetFrameGraphTree(Current_FrameGraph), Current_FrameGraph.BranchCount);

	//Stage 1: Create submits for unattachable branches (root, seperate, window etc.)
	for (unsigned char BranchIndex = 0; BranchIndex < Current_FrameGraph.BranchCount; BranchIndex++) {
		VK_RGBranch& MainBranch = VK_RGBranchSystem::GetBranch_byID(GetFrameGraphTree(Current_FrameGraph)[BranchIndex]);

		bool ShouldAddto_SubmitlessList = true;
		switch (MainBranch.GetBranchType()) {
		case BranchType::WindowBranch:
		case BranchType::RootBranch:
		case BranchType::SeperateBranch:
			CreateSubmit_ForRGB(Current_FrameGraph, MainBranch.GetID(), Submitless_RGBs);
			ShouldAddto_SubmitlessList = false;
			break;
		case BranchType::DynLaterExecs:
		case BranchType::NoDynLaterExec:
			//These Processible Types are handled later!
			break;
		case BranchType::RenderBranch:
		case BranchType::BarrierTPOnlyBranch:
			printer(result_tgfx_FAIL, "Branch's dynamic dependencies isn't solved or there is a error because it has dynamic type instead of processible type!");
			break;
		case BranchType::UNDEFINED:
			//UNDEFINED means branch has no workload, so skip the branch
			ShouldAddto_SubmitlessList = false;
			break;
		default:
			printer(result_tgfx_FAIL, "BranchType is an invalid value (not UNDEFINED), this shouldn't happen. Please check CreateSubmits_Fast()!");
			break;
		}

		if (ShouldAddto_SubmitlessList) {
			Submitless_RGBs.push_back(MainBranch.GetID());
			NumberOfSubmitless_RGBs++;
		}
	}

	//Stage 2: Create submits or attach branches to submits recursively
	FindRGBranches_dependentSubmits(Current_FrameGraph, Submitless_RGBs);

	//Stage 3: Created submits doesn't have queues and their dependencies aren't set, so link submits to each other
	LinkSubmits(GetFrameGraphSubmits(Current_FrameGraph));

	//Stage 4: Create empty submits for multiple last frame dependencies


	//Stage 5: Create signal semaphores and set wait semaphores

	//Stage 6: 


	PrintSubmits(Current_FrameGraph);


	//Give a Signal Semaphore and Command Buffer to each Submit
	///BUT THIS SHOULD BE DELETED BECAUSE SUBMITS CAN SIGNAL MORE THAN ONE SEMAPHORES
	for (unsigned int SubmitIndex = 0; SubmitIndex < GetFrameGraphSubmits(Current_FrameGraph).size(); SubmitIndex++) {
		VK_Submit* Submit = Current_FrameGraph.SubmitList[SubmitIndex];

		//Skip this process if Submit is WP only, because signal semaphores are set with VkAcquireNextImageKHR() at the end
		//and also VK_Submit structure only supports one signal semaphore for now
		if (Current_FrameGraph.FrameGraphTree[Submit->BranchIndexes[0] - 1].CorePasses[0].TYPE == PassType::WP) {
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
			printer(result_tgfx_FAIL, "There is a problem!");
		}
	}
	//Find all Wait Semaphores of each Submit
	for (unsigned char SubmitIndex = 0; SubmitIndex < GetFrameGraphSubmits(Current_FrameGraph).size(); SubmitIndex++) {
		VK_Submit* Submit = Current_FrameGraph.CurrentFrameSubmits[SubmitIndex];

		const VK_RGBranch& MainBranch = Current_FrameGraph.FrameGraphTree[Submit->BranchIndexes[0] - 1];

		//Find last frame dynamic dependent branches and get their signal semaphores to wait
		for (unsigned char LFDBIndex = 0; LFDBIndex < MainBranch.LFDynamicDependents.size(); LFDBIndex++) {
			const VK_RGBranch& LFDBranch = LastFrameGraph->FrameGraphTree[MainBranch.LFDynamicDependents[LFDBIndex]];
			//This is a window pass, so we should access to each Window to get the related semaphores
			if (LFDBranch.CorePasses[0].TYPE == PassType::WP) {
				windowpass_vk* WP = GFXHandleConverter(windowpass_vk*, LFDBranch.CorePasses[0].Handle);
				for (unsigned char WindowIndex = 0; WindowIndex < WP->WindowCalls[1].size(); WindowIndex++) {
					Submit->WaitSemaphoreIDs.push_back(
						WP->WindowCalls[1][WindowIndex].Window->PresentationSemaphores[0]
					);
					Submit->WaitSemaphoreStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
				}
			}
			else if (LFDBranch.AttachedSubmit)
			{
				bool isAddedPreviously = false;
				for (unsigned int SemaphoreIndexSearchIndex = 0; SemaphoreIndexSearchIndex < Submit->WaitSemaphoreIDs.size(); SemaphoreIndexSearchIndex++) {
					if (LFDBranch.AttachedSubmit->SignalSemaphoreIndex == Submit->WaitSemaphoreIDs[SemaphoreIndexSearchIndex]) {
						isAddedPreviously = true;
						break;
					}
				}
				if (!isAddedPreviously) {
					Submit->WaitSemaphoreIDs.push_back(LFDBranch.AttachedSubmit->SignalSemaphoreIndex);
					//This is laziness but there is gonna probably a little gain while searching for a more "tight" one cost more 
					Submit->WaitSemaphoreStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
				}
			}
		}
		//Find Penultimate Window Pass only branches that current submit depends on and get their signal semaphores to wait
		for (unsigned char PenultimateBIndex = 0; PenultimateBIndex < MainBranch.PenultimateSwapchainBranchCount; PenultimateBIndex++) {
			const VK_RGBranch* PenultimateWPBranch = MainBranch.PenultimateSwapchainBranches[PenultimateBIndex];
			windowpass_vk* WP = GFXHandleConverter(windowpass_vk*, PenultimateWPBranch->CorePasses[0].Handle);
			for (unsigned char WindowIndex = 0; WindowIndex < WP->WindowCalls[0].size(); WindowIndex++) {
				bool isAddedPreviously = false;
				for (unsigned int SemaphoreIndexSearchIndex = 0; SemaphoreIndexSearchIndex < Submit->WaitSemaphoreIDs.size(); SemaphoreIndexSearchIndex++) {
					if (WP->WindowCalls[0][WindowIndex].Window->PresentationSemaphores[1] == Submit->WaitSemaphoreIDs[SemaphoreIndexSearchIndex]) {
						isAddedPreviously = true;
						break;
					}
				}
				if (!isAddedPreviously) {
					Submit->WaitSemaphoreIDs.push_back(
						WP->WindowCalls[0][WindowIndex].Window->PresentationSemaphores[1]
					);
					Submit->WaitSemaphoreStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
				}
			}
		}
		//Find Current Frame dependent submits and get their signal semaphores to wait
		for (unsigned char CFDBIndex = 0; CFDBIndex < MainBranch.CFDynamicDependents.size(); CFDBIndex++) {
			const VK_RGBranch* CFDBranch = &Current_FrameGraph.FrameGraphTree[MainBranch.CFDynamicDependents[CFDBIndex]];

			bool isAddedPreviously = false;
			for (unsigned int SemaphoreIndexSearchIndex = 0; SemaphoreIndexSearchIndex < Submit->WaitSemaphoreIDs.size(); SemaphoreIndexSearchIndex++) {
				if (CFDBranch->AttachedSubmit->SignalSemaphoreIndex == Submit->WaitSemaphoreIDs[SemaphoreIndexSearchIndex]) {
					isAddedPreviously = true;
					break;
				}
			}
			if (!isAddedPreviously) {
				Submit->WaitSemaphoreIDs.push_back(CFDBranch->AttachedSubmit->SignalSemaphoreIndex);
				//Wait dependencies are hard in semaphores, so just use one of the most common ones
				VkPipelineStageFlags flag = 0;
				if (CFDBranch->AttachedSubmit->Run_Queue != Submit->Run_Queue) {
					flag = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
				}
				else {
					if (CFDBranch->CFNeeded_QueueSpecs.is_TRANSFERsupported) {
						flag |= VK_PIPELINE_STAGE_TRANSFER_BIT;
					}
					if (CFDBranch->CFNeeded_QueueSpecs.is_COMPUTEsupported) {
						flag |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
					}
					if (CFDBranch->CFNeeded_QueueSpecs.is_GRAPHICSsupported) {
						flag |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
					}
					if (!CFDBranch->CFNeeded_QueueSpecs.is_COMPUTEsupported &&
						!CFDBranch->CFNeeded_QueueSpecs.is_GRAPHICSsupported &&
						!CFDBranch->CFNeeded_QueueSpecs.is_PRESENTATIONsupported &&
						!CFDBranch->CFNeeded_QueueSpecs.is_TRANSFERsupported) {
						flag |= VK_PIPELINE_STAGE_TRANSFER_BIT;
					}
				}

				Submit->WaitSemaphoreStages.push_back(flag);
			}

		}
	}


	//Delete wait semaphores that is already waited on another waited submit
	//TO-DO: This case should be eliminated in a previous process, this is just a hack
	bool isAnyChanged = true;
	while (isAnyChanged) {
		isAnyChanged = false;
		for (unsigned char SubmitIndex = 0; SubmitIndex < Current_FrameGraph.CurrentFrameSubmits.size() && !isAnyChanged; SubmitIndex++) {
			VK_Submit* MainSubmit = Current_FrameGraph.CurrentFrameSubmits[SubmitIndex];
			if (MainSubmit->WaitSemaphoreIDs.size() <= 1) {
				continue;
			}
			//Find every waited semaphore's submit, then search through each wait semaphore of the found submit
			//If any of the searched wait semaphores collides with the main submit's wait semaphore, delete the
			for (unsigned char MainWaitIndex = 0; MainWaitIndex < MainSubmit->WaitSemaphoreIDs.size() && !isAnyChanged; MainWaitIndex++) {
				unsigned char MainWaitSemaphore = MainSubmit->WaitSemaphoreIDs[MainWaitIndex];
				for (unsigned char SearchSubmitIndex = 0; SearchSubmitIndex < Current_FrameGraph.CurrentFrameSubmits.size() && !isAnyChanged; SearchSubmitIndex++) {
					if (SearchSubmitIndex == SubmitIndex) {
						continue;
					}
					VK_Submit* SearchSubmit = Current_FrameGraph.CurrentFrameSubmits[SearchSubmitIndex];
					for (unsigned char SearchWaitIndex = 0; SearchWaitIndex < SearchSubmit->WaitSemaphoreIDs.size() && !isAnyChanged; SearchWaitIndex++) {
						if (SearchSubmit->WaitSemaphoreIDs[SearchWaitIndex] == MainWaitSemaphore) {
							MainSubmit->WaitSemaphoreIDs.erase(MainSubmit->WaitSemaphoreIDs.begin() + MainWaitIndex);
							MainSubmit->WaitSemaphoreStages.erase(MainSubmit->WaitSemaphoreStages.begin() + MainWaitIndex);
							isAnyChanged = true;
						}
					}
				}
			}
		}
	}
}
//This only works if Framegraph is reconstructed this frame because this doesn't care last frame dependencies
void CreateSubmits_Intermediate(VK_FrameGraph& Current_FrameGraph) {

}
//This function checks all semaphores of the last frame to find signaled ones at the end of the frame
//Then sends a empty command buffer to the queue to unsignal all these semaphores
//With that way, semaphores are recycled
void UnsignalLastFrameSemaphores(VK_FrameGraph& Last_FrameGraph) {

}
//This function waits for GPU operations of the last frame to end
//This is necessary if framegraph is reconstructed in current frame, because there may strong changes that dependencies can't be solved
void WaitForLastFrame(VK_FrameGraph& Last_FrameGraph) {
	std::vector<SubmitIDType>& Submits = GetFrameGraphSubmits(Last_FrameGraph);
	for (unsigned int SubmitIndex = 0; SubmitIndex < Submits.size(); SubmitIndex++) {
#ifdef VULKAN_DEBUGGING
		if (Submits[SubmitIndex] == INVALID_SubmitID) {
			printer(result_tgfx_FAIL, "Submit has an invalid id, which shouldn't have!");
		}
#endif
		VK_Submit& Submit = VK_SubmitSystem::GetSubmit_byID(GetFrameGraphSubmits(Last_FrameGraph)[SubmitIndex]);
	}
}

//This is defined in Command Buffer Recording.cpp
void RecordRGBranchCalls(VK_RGBranch& Branch, VkCommandBuffer CB);
void WaitForRenderGraphCommandBuffers() {
	unsigned char FrameIndex = VKRENDERER->GetCurrentFrameIndex();
	//Wait for command buffers to end
	for (unsigned char QueueIndex = 0; QueueIndex < RENDERGPU->QUEUEFAMS().size(); QueueIndex++) {
		printer(result_tgfx_SUCCESS, ("Queue Index: " + std::to_string(QueueIndex) + " & Feature Score: " +
			std::to_string(RENDERGPU->QUEUEFAMS()[QueueIndex].QueueFeatureScore)).c_str());
		std::vector<VkFence> Fences;
		for (unsigned int FenceIndex = 0; FenceIndex < RENDERGPU->QUEUEFAMS()[QueueIndex].RenderGraphFences[FrameIndex].size(); FenceIndex++) {
			if (!RENDERGPU->QUEUEFAMS()[QueueIndex].RenderGraphFences[FrameIndex][FenceIndex].is_Used) {
				continue;
			}
			RENDERGPU->QUEUEFAMS()[QueueIndex].RenderGraphFences[FrameIndex][FenceIndex].is_Used = false;
			Fences.push_back(RENDERGPU->QUEUEFAMS()[QueueIndex].RenderGraphFences[FrameIndex][FenceIndex].Fence_o);
		}
		if (vkWaitForFences(RENDERGPU->LOGICALDEVICE(), Fences.size(), Fences.data(), VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "VulkanRenderer: Fence wait has failed!");
		}
		if (vkResetFences(RENDERGPU->LOGICALDEVICE(), Fences.size(), Fences.data()) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "VulkanRenderer: Fence reset has failed!");
		}
	}

	//Because current frame's framegraph isn't constructed (changes in rendergraph isn't processed yet), this is Penultimate Framegraph
	VK_FrameGraph& PenultimateFG = VKRENDERER->FrameGraphs[FrameIndex];
	//Set all semaphores used in PenultimateFG as unused
	for (VK_RGBranch::BranchIDType i = 0; i < PenultimateFG.BranchCount; i++) {
		VK_RGBranch& PenultimateBranch = ((VK_RGBranch*)PenultimateFG.FrameGraphTree)[i];
		VK_Submit& Submit = PenultimateBranch.Get_AttachedSubmit();
		const std::vector<VK_Semaphore::SemaphoreIDType>& SemaphoreIDs = Submit.Get_SignalSemaphoreIDs();
		//There is a iterator because I may use pointers as IDs later
		for (std::vector<VK_Semaphore::SemaphoreIDType>::const_iterator SignalElement = SemaphoreIDs.begin(); SignalElement < SemaphoreIDs.end(); SignalElement++) {
			SEMAPHORESYS->DestroySemaphore(*SignalElement);
		}
	}
}

//This function will do the cleaning from the penultimate frame's datas and will do some analyze for faster submit creation (branch workload analysis etc.)
//Don't call this function if framegraph is gonna be reconstructed
void Prepare_forSubmitCreation() {
	unsigned char CurrentFrameIndex = VKRENDERER->GetCurrentFrameIndex();
	VK_FrameGraph& Current_FrameGraph = VKRENDERER->FrameGraphs[CurrentFrameIndex];


	//Reset submit infos
	std::vector<SubmitIDType>& PenultimateSubmits = GetFrameGraphSubmits(Current_FrameGraph);
	for (unsigned int i = 0; i < PenultimateSubmits.size(); i++) {
		VK_SubmitSystem::DestroySubmit(PenultimateSubmits[i]);
	}
	PenultimateSubmits.clear();

	//Prepare each branch for the new frame
	//Preparation means: Clear dynamic infos, create list of workloaded passes, calculate branch type and needed queue flag
	for (unsigned char BranchIndex = 0; BranchIndex < Current_FrameGraph.BranchCount; BranchIndex++) {
		VK_RGBranch& Branch = ((VK_RGBranch*)Current_FrameGraph.FrameGraphTree)[BranchIndex];
		if (!Branch.PrepareForNewFrame()) {
			printer(result_tgfx_SUCCESS, "One of the branches has no workload!");
		}
	}
}
void Send_RenderCommands() {
	unsigned char CurrentFrameIndex = VKRENDERER->GetCurrentFrameIndex();
	VK_FrameGraph& Current_FrameGraph = VKRENDERER->FrameGraphs[CurrentFrameIndex];


	for (unsigned char SubmitIndex = 0; SubmitIndex < Current_FrameGraph.CurrentFrameSubmits.size(); SubmitIndex++) {
		VK_Submit& Submit = Current_FrameGraph.CurrentFrameSubmits[SubmitIndex];
		if (Current_FrameGraph.FrameGraphTree[Submit.BranchIndexes[0] - 1].CorePasses[0].TYPE == PassType::WP) {
			continue;
		}

		VkCommandBufferBeginInfo cb_bi = {};
		cb_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		cb_bi.pInheritanceInfo = nullptr;
		cb_bi.pNext = nullptr;
		cb_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(Submit.Run_Queue->CommandPools[CurrentFrameIndex].CBs[Submit->CBIndex].CB, &cb_bi);

		for (unsigned char BranchIndex = 0; BranchIndex < Submit->BranchIndexes.size(); BranchIndex++) {
			VK_RGBranch& Branch = Current_FrameGraph.FrameGraphTree[Submit->BranchIndexes[BranchIndex] - 1];
			RecordRGBranchCalls(Branch, Submit.Run_Queue->CommandPools[CurrentFrameIndex].CBs[Submit->CBIndex].CB);
		}
		vkEndCommandBuffer(Submit.Run_Queue->CommandPools[CurrentFrameIndex].CBs[Submit->CBIndex].CB);
	}


	//Send rendercommand buffers
	bool isThereUnsentCB = true;
	while (isThereUnsentCB) {
		isThereUnsentCB = false;
		for (unsigned char QueueIndex = 0; QueueIndex < RENDERGPU->QUEUEFAMS().size(); QueueIndex++) {
			printer(result_tgfx_SUCCESS, ("QueueIndex: " + std::to_string(QueueIndex) + " & FeatureCount: " + std::to_string(RENDERGPU->QUEUEFAMS()[QueueIndex].QueueFeatureScore)).c_str());
			std::vector<VkSubmitInfo> FINALSUBMITs;
			std::vector<std::vector<VkSemaphore>> WaitSemaphoreLists;
			for (unsigned char SubmitIndex = 0; SubmitIndex < VKRENDERER->FrameGraphs[CurrentFrameIndex].CurrentFrameSubmits.size(); SubmitIndex++) {
				VK_Submit* Submit = VKRENDERER->FrameGraphs[CurrentFrameIndex].CurrentFrameSubmits[SubmitIndex];
				if (Submit->Run_Queue != &RENDERGPU->QUEUEFAMS()[QueueIndex] || Submit->CBIndex == 255) {
					continue;
				}
				if (Submit->IsAddedToQueueSubmitList) {
					printer(result_tgfx_SUCCESS, "Submit is already added, skipped it!");
					continue;
				}
				bool isDependentSubmits_notSent_or_notListed = false;
				for (unsigned char SubmitIndex = 0; SubmitIndex < VKRENDERER->FrameGraphs[CurrentFrameIndex].CurrentFrameSubmits.size() && !isDependentSubmits_notSent_or_notListed; SubmitIndex++) {
					VK_Submit* SearchSubmit = VKRENDERER->FrameGraphs[CurrentFrameIndex].CurrentFrameSubmits[SubmitIndex];
					for (unsigned char WaitIndex = 0; WaitIndex < Submit->WaitSemaphoreIDs.size(); WaitIndex++) {
						if (SearchSubmit->SignalSemaphoreIndex == Submit->WaitSemaphoreIDs[WaitIndex] && !SearchSubmit->IsAddedToQueueSubmitList && SearchSubmit->Run_Queue != Submit->Run_Queue) {
							isDependentSubmits_notSent_or_notListed = true;
							break;
						}
					}
				}
				if (isDependentSubmits_notSent_or_notListed) {
					isThereUnsentCB = true;
					printer(result_tgfx_SUCCESS, "One of the dependent submits isn't sent, so don't send this submit!");
					continue;
				}

				VkSubmitInfo submitinfo = {};
				submitinfo.commandBufferCount = 1;
				submitinfo.pCommandBuffers = &RENDERGPU->QUEUEFAMS()[QueueIndex].CommandPools[CurrentFrameIndex].CBs[Submit->CBIndex].CB;
				submitinfo.pNext = nullptr;
				submitinfo.pSignalSemaphores = &VKRENDERER->Semaphores[Submit->SignalSemaphoreIndex].SPHandle;
				std::cout << "\n\nNew Submit";
				printer(result_tgfx_SUCCESS, "Signal SemaphoreIndex: " + std::to_string(Submit->SignalSemaphoreIndex));
				WaitSemaphoreLists.push_back(std::vector<VkSemaphore>());
				std::vector<VkSemaphore>* WaitSemaphoreList = &WaitSemaphoreLists[WaitSemaphoreLists.size() - 1];
				for (unsigned char WaitIndex = 0; WaitIndex < Submit->WaitSemaphoreIDs.size(); WaitIndex++) {
					WaitSemaphoreList->push_back(VKRENDERER->Semaphores[Submit->WaitSemaphoreIDs[WaitIndex]].SPHandle);
					printer(result_tgfx_SUCCESS, "Wait Semaphore Index: " + std::to_string(Submit->WaitSemaphoreIDs[WaitIndex]));
				}

				submitinfo.pWaitSemaphores = WaitSemaphoreList->data();
				submitinfo.pWaitDstStageMask = Submit->WaitSemaphoreStages.data();
				submitinfo.waitSemaphoreCount = Submit->WaitSemaphoreIDs.size();
				submitinfo.signalSemaphoreCount = 1;
				submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				FINALSUBMITs.push_back(submitinfo);
				Submit->IsAddedToQueueSubmitList = true;
			}
			if (!FINALSUBMITs.size()) {
				continue;
			}
			VkFence TargetFence = VK_NULL_HANDLE;
			printer(result_tgfx_SUCCESS, "Queue's fence count: " + std::to_string(RENDERGPU->QUEUEFAMS()[QueueIndex].RenderGraphFences[CurrentFrameIndex].size()));
			for (unsigned char FenceIndex = 0; FenceIndex < RENDERGPU->QUEUEFAMS()[QueueIndex].RenderGraphFences[CurrentFrameIndex].size(); FenceIndex++) {
				VK_Fence& searchfence = RENDERGPU->QUEUEFAMS()[QueueIndex].RenderGraphFences[CurrentFrameIndex][FenceIndex];
				if (!searchfence.is_Used) {
					TargetFence = searchfence.Fence_o;
					if (TargetFence == VK_NULL_HANDLE) {
						printer(result_tgfx_FAIL, "WTF!?");
					}
					searchfence.is_Used = true;
				}
			}
			if (TargetFence == VK_NULL_HANDLE) {
				RENDERGPU->QUEUEFAMS()[QueueIndex].RenderGraphFences[CurrentFrameIndex].push_back(VK_Fence());
				VK_Fence& newfence = RENDERGPU->QUEUEFAMS()[QueueIndex].RenderGraphFences[CurrentFrameIndex][RENDERGPU->QUEUEFAMS()[QueueIndex].RenderGraphFences[CurrentFrameIndex].size() - 1];
				VkFenceCreateInfo Fence_ci = {};
				Fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				Fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				Fence_ci.pNext = nullptr;
				if (vkCreateFence(RENDERGPU->LOGICALDEVICE(), &Fence_ci, nullptr, &newfence.Fence_o) != VK_SUCCESS) {
					printer(result_tgfx_FAIL, "VulkanRenderer: Fence creation has failed!");
				}
				if (vkResetFences(RENDERGPU->LOGICALDEVICE(), 1, &newfence.Fence_o) != VK_SUCCESS) {
					printer(result_tgfx_FAIL, "VulkanRenderer: Fence reset has failed!");
				}
				TargetFence = newfence.Fence_o;
				newfence.is_Used = true;
			}
			unsigned long long duration;
			TURAN_PROFILE_SCOPE_MCS(("CB submission! Submit Count: " + std::to_string(FINALSUBMITs.size()) + " & QueueFeatureCount: " + std::to_string(RENDERGPU->QUEUEFAMS()[QueueIndex].QueueFeatureScore)).c_str(), &duration)
				if (vkQueueSubmit(RENDERGPU->QUEUEFAMS()[QueueIndex].Queue, FINALSUBMITs.size(), FINALSUBMITs.data(), TargetFence) != VK_SUCCESS) {
					printer(result_tgfx_FAIL, "Vulkan Queue Submission has failed!");
					return;
				}
			STOP_PROFILE_PRINTFUL_TAPI()
		}
	}
}
void Send_PresentationCommands() {
	unsigned char CurrentFrameIndex = VKRENDERER->GetCurrentFrameIndex();
	VK_FrameGraph& Current_FrameGraph = VKRENDERER->FrameGraphs[CurrentFrameIndex];

	//Handle additional dear IMGUI windows (docking branch)
	if (VK_IMGUI) {
		VK_IMGUI->Render_AdditionalWindows();
	}

	//Send displays
	for (unsigned char WindowPassIndex = 0; WindowPassIndex < VKRENDERER->WindowPasses.size(); WindowPassIndex++) {
		windowpass_vk* WP = VKRENDERER->WindowPasses[WindowPassIndex];
		if (!WP->WindowCalls[2].size()) {
			continue;
		}

		//Fill swapchain and image indices std::vectors
		std::vector<VkSwapchainKHR> Swapchains;	Swapchains.resize(WP->WindowCalls[2].size());
		std::vector<uint32_t> ImageIndices;		ImageIndices.resize(WP->WindowCalls[2].size());
		for (unsigned char WindowIndex = 0; WindowIndex < WP->WindowCalls[2].size(); WindowIndex++) {
			ImageIndices[WindowIndex] = WP->WindowCalls[2][WindowIndex].Window->CurrentFrameSWPCHNIndex;
			WP->WindowCalls[2][WindowIndex].Window->CurrentFrameSWPCHNIndex = (WP->WindowCalls[2][WindowIndex].Window->CurrentFrameSWPCHNIndex + 1) % 2;
			Swapchains[WindowIndex] = WP->WindowCalls[2][WindowIndex].Window->Window_SwapChain;
		}

		//Find the submit to get the wait semaphores
		std::vector<VkSemaphore> WaitSemaphores;

		for (unsigned char SubmitIndex = 0; SubmitIndex < GetFrameGraphSubmits(VKRENDERER->FrameGraphs[CurrentFrameIndex]).size(); SubmitIndex++) {
			VK_Submit& Submit = SUBMITSYS->GetSubmit_byID(GetFrameGraphSubmits(VKRENDERER->FrameGraphs[CurrentFrameIndex])[SubmitIndex]);

			//Window Pass only submits, contains one branch and the branch is Window Pass only too!
			if (Submit->BranchIndexes[0]) {
				VK_BranchPass& Pass = VKRENDERER->FrameGraphs[CurrentFrameIndex].FrameGraphTree[Submit->BranchIndexes[0] - 1].CorePasses[0];
				if (Pass.Handle == WP) {
					for (unsigned char WaitSemaphoreIndex = 0; WaitSemaphoreIndex < Submit->WaitSemaphoreIDs.size(); WaitSemaphoreIndex++) {
						WaitSemaphores.push_back(VKRENDERER->Semaphores[Submit->WaitSemaphoreIDs[WaitSemaphoreIndex]].SPHandle);
						printer(result_tgfx_SUCCESS, "vkQueuePresentKHR WaitSemaphoreIndex: " + std::to_string(Submit->WaitSemaphoreIDs[WaitSemaphoreIndex]));
					}
				}
			}
		}

		VkPresentInfoKHR SwapchainImage_PresentationInfo = {};
		SwapchainImage_PresentationInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		SwapchainImage_PresentationInfo.pNext = nullptr;
		SwapchainImage_PresentationInfo.swapchainCount = Swapchains.size();
		SwapchainImage_PresentationInfo.pSwapchains = Swapchains.data();
		SwapchainImage_PresentationInfo.pImageIndices = ImageIndices.data();
		SwapchainImage_PresentationInfo.pResults = nullptr;

		SwapchainImage_PresentationInfo.waitSemaphoreCount = WaitSemaphores.size();
		SwapchainImage_PresentationInfo.pWaitSemaphores = WaitSemaphores.data();
		VkQueue DisplayQueue = {};
		for (unsigned char QueueIndex = 0; QueueIndex < RENDERGPU->QUEUEFAMS().size(); QueueIndex++) {
			if (RENDERGPU->QUEUEFAMS()[QueueIndex].SupportFlag.is_PRESENTATIONsupported) {
				DisplayQueue = RENDERGPU->QUEUEFAMS()[QueueIndex].Queue;
			}
		}

		TURAN_PROFILE_SCOPE_MCS("Display sendage!", nullptr)
			if (vkQueuePresentKHR(DisplayQueue, &SwapchainImage_PresentationInfo) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "Submitting Presentation Queue has failed!");
				return;
			}
		STOP_PROFILE_PRINTFUL_TAPI()
	}

}
void RenderGraph_DataShifting() {
	//Shift all WindowCall buffers of all Window Passes!
	//Also shift all of the window semaphores
	for (unsigned char WPIndex = 0; WPIndex < VKRENDERER->WindowPasses.size(); WPIndex++) {
		windowpass_vk* WP = VKRENDERER->WindowPasses[WPIndex];
		WP->WindowCalls[0] = WP->WindowCalls[1];
		WP->WindowCalls[1] = WP->WindowCalls[2];
		WP->WindowCalls[2].clear();
	}
	for (unsigned char WindowIndex = 0; WindowIndex < VKCORE->WINDOWs.size(); WindowIndex++) {
		WINDOW* Window = ((WINDOW*)VKCORE->WINDOWs[WindowIndex]);
		unsigned char b_semaphore = Window->PresentationSemaphores[0];
		Window->PresentationSemaphores[0] = Window->PresentationSemaphores[1];
		Window->PresentationSemaphores[1] = b_semaphore;
	}
}




bool Check_WaitHandles() {
	for (unsigned int DPIndex = 0; DPIndex < VKRENDERER->DrawPasses.size(); DPIndex++) {
		drawpass_vk* DP = VKRENDERER->DrawPasses[DPIndex];
		for (unsigned int WaitIndex = 0; WaitIndex < DP->WAITsCOUNT; WaitIndex++) {
			VK_PassWaitDescription& Wait_desc = DP->WAITs[WaitIndex];
			if ((*Wait_desc.WaitedPass) == nullptr) {
				printer(result_tgfx_FAIL, "You forgot to set wait handle of one of the draw passes!");
				return false;
			}

			//Search through all pass types
			switch ((*Wait_desc.WaitedPass)->TYPE) {
			case PassType::DP:
			{
				bool is_Found = false;
				for (unsigned int CheckedDP = 0; CheckedDP < VKRENDERER->DrawPasses.size(); CheckedDP++) {
					if (*Wait_desc.WaitedPass == VKRENDERER->DrawPasses[CheckedDP]) {
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
			case PassType::TP:
			{
				bool is_Found = false;
				for (unsigned int CheckedTP = 0; CheckedTP < VKRENDERER->TransferPasses.size(); CheckedTP++) {
					if (*Wait_desc.WaitedPass == VKRENDERER->TransferPasses[CheckedTP]) {
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
			case PassType::WP:
			{
				bool is_Found = false;
				for (unsigned int CheckedWP = 0; CheckedWP < VKRENDERER->WindowPasses.size(); CheckedWP++) {
					if (*Wait_desc.WaitedPass == VKRENDERER->WindowPasses[CheckedWP]) {
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
			case PassType::CP:
			{
				bool is_Found = false;
				for (unsigned int CheckedCP = 0; CheckedCP < VKRENDERER->ComputePasses.size(); CheckedCP++) {
					if (*Wait_desc.WaitedPass == VKRENDERER->ComputePasses[CheckedCP]) {
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
			case PassType::ERROR:
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
	for (unsigned int WPIndex = 0; WPIndex < VKRENDERER->WindowPasses.size(); WPIndex++) {
		windowpass_vk* WP = ((windowpass_vk*)VKRENDERER->WindowPasses[WPIndex]);
		for (unsigned int WaitIndex = 0; WaitIndex < WP->WAITsCOUNT; WaitIndex++) {
			VK_PassWaitDescription& Wait_desc = WP->WAITs[WaitIndex];
			if (!(*Wait_desc.WaitedPass)) {
				printer(result_tgfx_FAIL, "You forgot to set wait handle of one of the window passes!");
				return false;
			}

			switch ((*Wait_desc.WaitedPass)->TYPE) {
			case PassType::DP:
			{
				bool is_Found = false;
				for (unsigned int CheckedDP = 0; CheckedDP < VKRENDERER->DrawPasses.size(); CheckedDP++) {
					if (*Wait_desc.WaitedPass == VKRENDERER->DrawPasses[CheckedDP]) {
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
			case PassType::TP:
			{
				transferpass_vk* currentTP = ((transferpass_vk*)*Wait_desc.WaitedPass);
				bool is_Found = false;
				for (unsigned int CheckedTP = 0; CheckedTP < VKRENDERER->TransferPasses.size(); CheckedTP++) {
					if (currentTP == VKRENDERER->TransferPasses[CheckedTP]) {
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
			case PassType::CP:
			{
				bool is_Found = false;
				for (unsigned int CheckedCP = 0; CheckedCP < VKRENDERER->ComputePasses.size(); CheckedCP++) {
					if (*Wait_desc.WaitedPass == VKRENDERER->ComputePasses[CheckedCP]) {
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
			case PassType::WP:
				printer(result_tgfx_FAIL, "A window pass can't wait for another window pass, Check Wait Handle failed!");
				return false;
			case PassType::ERROR:
				printer(result_tgfx_FAIL, "Finding a TPType has failed, so Check Wait Handle too!");
				return false;
			default:
				printer(result_tgfx_FAIL, "TP Type is not supported for now, so Check Wait Handle failed too!");
				return false;
			}
		}
	}
	for (unsigned int TPIndex = 0; TPIndex < VKRENDERER->TransferPasses.size(); TPIndex++) {
		transferpass_vk* TP = VKRENDERER->TransferPasses[TPIndex];
		for (unsigned int WaitIndex = 0; WaitIndex < TP->WAITsCOUNT; WaitIndex++) {
			VK_PassWaitDescription& Wait_desc = TP->WAITs[WaitIndex];
			if (!(*Wait_desc.WaitedPass)) {
				printer(result_tgfx_FAIL, "You forgot to set wait handle of one of the transfer passes!");
				return false;
			}

			switch ((*Wait_desc.WaitedPass)->TYPE) {
			case PassType::DP:
			{
				bool is_Found = false;
				for (unsigned int CheckedDP = 0; CheckedDP < VKRENDERER->DrawPasses.size(); CheckedDP++) {
					if (*Wait_desc.WaitedPass == VKRENDERER->DrawPasses[CheckedDP]) {
						is_Found = true;
						break;
					}
				}
				if (!is_Found) {
					std::cout << TP->NAME << std::endl;
					printer(result_tgfx_FAIL, "One of the transfer passes waits for an draw pass but given pass isn't found!");
					return false;
				}
			}
			break;
			case PassType::TP:
			{
				transferpass_vk* currentTP = ((transferpass_vk*)*Wait_desc.WaitedPass);
				bool is_Found = false;
				for (unsigned int CheckedTP = 0; CheckedTP < VKRENDERER->TransferPasses.size(); CheckedTP++) {
					if (currentTP == VKRENDERER->TransferPasses[CheckedTP]) {
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
			case PassType::WP:
			{
				bool is_Found = false;
				for (unsigned int CheckedWP = 0; CheckedWP < VKRENDERER->WindowPasses.size(); CheckedWP++) {
					if (*Wait_desc.WaitedPass == VKRENDERER->WindowPasses[CheckedWP]) {
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
			case PassType::CP:
			{
				bool is_Found = false;
				for (unsigned int CheckedCP = 0; CheckedCP < VKRENDERER->ComputePasses.size(); CheckedCP++) {
					if (*Wait_desc.WaitedPass == VKRENDERER->ComputePasses[CheckedCP]) {
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
			case PassType::ERROR:
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
	for (unsigned int CPIndex = 0; CPIndex < VKRENDERER->ComputePasses.size(); CPIndex++) {
		computepass_vk* CP = VKRENDERER->ComputePasses[CPIndex];
		for (unsigned int WaitIndex = 0; WaitIndex < CP->WAITsCOUNT; WaitIndex++) {
			VK_PassWaitDescription& Wait_desc = CP->WAITs[WaitIndex];
			if (!(*Wait_desc.WaitedPass)) {
				printer(result_tgfx_FAIL, "You forgot to set wait handle of one of the compute passes!");
				return false;
			}

			switch ((*Wait_desc.WaitedPass)->TYPE) {
			case PassType::DP:
			{
				bool is_Found = false;
				for (unsigned int CheckedDP = 0; CheckedDP < VKRENDERER->DrawPasses.size(); CheckedDP++) {
					if (*Wait_desc.WaitedPass == VKRENDERER->DrawPasses[CheckedDP]) {
						is_Found = true;
						break;
					}
				}
				if (!is_Found) {
					std::cout << CP->NAME << std::endl;
					printer(result_tgfx_FAIL, "One of the compute passes waits for an draw pass but given pass isn't found!");
					return false;
				}
			}
			break;
			case PassType::TP:
			{
				transferpass_vk* currentTP = ((transferpass_vk*)*Wait_desc.WaitedPass);
				bool is_Found = false;
				for (unsigned int CheckedTP = 0; CheckedTP < VKRENDERER->TransferPasses.size(); CheckedTP++) {
					if (currentTP == VKRENDERER->TransferPasses[CheckedTP]) {
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
			case PassType::WP:
			{
				bool is_Found = false;
				for (unsigned int CheckedWP = 0; CheckedWP < VKRENDERER->WindowPasses.size(); CheckedWP++) {
					if (*Wait_desc.WaitedPass == VKRENDERER->WindowPasses[CheckedWP]) {
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
			case PassType::CP:
			{
				bool is_Found = false;
				for (unsigned int CheckedCP = 0; CheckedCP < VKRENDERER->ComputePasses.size(); CheckedCP++) {
					if (*Wait_desc.WaitedPass == VKRENDERER->ComputePasses[CheckedCP]) {
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
			case PassType::ERROR:
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

//Simple function definitions

inline VK_RGBranch::BranchIDType* GetFrameGraphTree(VK_FrameGraph& fg) { return ((VK_RGBranch::BranchIDType*)fg.FrameGraphTree); }
inline const VK_RGBranch::BranchIDType* ReadBranchList(const VK_FrameGraph& fg) { return ((VK_RGBranch::BranchIDType*)fg.FrameGraphTree); }
inline std::vector<SubmitIDType>& GetFrameGraphSubmits(VK_FrameGraph& fg) { return (*(std::vector<SubmitIDType>*)fg.SubmitList); }