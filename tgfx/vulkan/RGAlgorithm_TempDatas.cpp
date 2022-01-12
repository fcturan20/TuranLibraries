#include "RGAlgorithm_TempDatas.h"
#define LOG VKCORE->TGFXCORE.DebugCallback

#define SEMAPHORESYS ((VK_SemaphoreSystem*)VKRENDERER->Semaphores)
#define BRANCHSYS ((VK_RGBranchSystem*)VKRENDERER->Branches)
#define SUBMITSYS ((VK_SubmitSystem*)VKRENDERER->Submits)

VK_Submit::VK_Submit() {
	Invalidate();
}
void VK_Submit::Invalidate() {
	Run_Queue = nullptr;
	CB_ID = VK_CommandBuffer::INVALID_ID;
	WaitInfos.clear();
	SignalSemaphoreIDs.clear();
	BranchIDs.clear();

	Type = SubmitType::UNDEFINED;
}
void VK_Submit::Initialize(VK_CommandBuffer::CommandBufferIDType CommandBufferID, VK_RGBranch::BranchIDType FirstBranchID, SubmitIDType SubmitID) {
#ifdef VULKAN_DEBUGGING
	if (CommandBufferID == VK_CommandBuffer::INVALID_ID) {
		LOG(tgfx_result_FAIL, "Please don't initialize submits with invalid command buffer id, this is not recommended!");
	}
	if (FirstBranchID == VK_RGBranch::INVALID_BRANCHID) {
		LOG(tgfx_result_FAIL, "Please don't initialize submits with invalid branch id, this is not recommended!");
	}
	ID = SubmitID;
#endif // VULKAN_DEBUGGING

	BranchIDs.push_back(FirstBranchID);
	CB_ID = CommandBufferID;
}


void VK_Submit::PushBack_WaitInfo(SubmitIDType WaitSubmit, VkPipelineStageFlags WaitStage, SubmitWaitType WaitType) {
	WaitInfos.push_back(VK_SubmitWaitInfo());
	VK_SubmitWaitInfo& WaitInfo = WaitInfos[WaitInfos.size() - 1];
	WaitInfo.WaitedStage = WaitStage;
	WaitInfo.WaitedSubmit = WaitSubmit;
	WaitInfo.WaitType = WaitType;
}
void VK_Submit::PushBack_RGBranch(VK_RGBranch::BranchIDType BranchID) { BranchIDs.push_back(BranchID); }
void VK_Submit::PushBack_SignalSemaphore(VK_Semaphore::SemaphoreIDType SemaphoreID) { SignalSemaphoreIDs.push_back(SemaphoreID); }
const std::vector<VK_Semaphore::SemaphoreIDType>& VK_Submit::Get_SignalSemaphoreIDs() const { return SignalSemaphoreIDs; }
const std::vector<VK_RGBranch::BranchIDType>& VK_Submit::Get_ActiveBranchList() const { return BranchIDs; }
void VK_Submit::Set_SubmitType(SubmitType type) { Type = type; }
SubmitType VK_Submit::Get_SubmitType() const { return Type; }
VK_QUEUEFAM* VK_Submit::Get_RunQueue() const { return Run_Queue; }

bool VK_RGBranch::PrepareForNewFrame() {
	ClearDynamicDatas();
	if (!CreateWorkloadedPassList_oftheBranch()) {
		return false;
	}
	//If the branch is workloaded
	if (!CalculateBranchType()) {
		LOG(tgfx_result_FAIL, "VK_RGBranch::PrepareForNewFrame() has failed at CalculateBranchType()!");
		return false;
	}
	if (!CalculateNeededQueueFlag()) {
		LOG(tgfx_result_FAIL, "VK_RGBranch::PrepareForNewFrame() has failed at CalculateNeededQueueFlag()!");
		return false;
	}
	return true;
}
inline void VK_RGBranch::FindProcessibleType() {
#ifdef VULKAN_DEBUGGING
	if (CurrentType == BranchType::UNDEFINED) {
		LOG(tgfx_result_FAIL, "CurrentBranchType shouldn't be UNDEFINED, there maybe a problem.");
		return;
	}
#endif
	if (CurrentType == BranchType::WindowBranch) {
		return;
	}
	if (CFDynamicDependents.size() == 0) {
		CurrentType = BranchType::RootBranch;
		return;
	}
	if (DynamicLaterExecutes.size() == 0 && CFDynamicDependents.size() == 1 && LFDynamicDependents.size() == 0) {
		CurrentType = BranchType::NoDynLaterExec;
		return;
	}
	if (DynamicLaterExecutes.size() && CFDynamicDependents.size() == 1 && LFDynamicDependents.size() == 0) {
		CurrentType = BranchType::DynLaterExecs;
		return;
	}
	if (CFDynamicDependents.size() > 1 || LFDynamicDependents.size()) {
		CurrentType = BranchType::SeperateBranch;
		return;
	}
	CurrentType = BranchType::UNDEFINED;
}
BranchType VK_RGBranch::GetBranchType() const {
	return CurrentType;
}
inline bool VK_RGBranch::IsWorkloaded() const {
	return CurrentFramePassesIndexes[0] == PASSINDEXTYPE_INVALID;
}
VK_QUEUEFLAG VK_RGBranch::GetNeededQueueFlag() const {
	return CFNeeded_QueueSpecs;
}

void VK_RGBranch::Invalidate() {
	//Clear previously allocated lists
	PenultimateSwapchainBranchCount = 0;
	if (PenultimateSwapchainBranches) {
		delete[] PenultimateSwapchainBranches;
		PenultimateSwapchainBranches = nullptr;
	}
	LaterExecutedBranchCount = 0;
	if (LaterExecutedBranches) {
		delete[] LaterExecutedBranches;
		LaterExecutedBranches = nullptr;
	}
	CFDependentBranchCount = 0;
	if (CFDependentBranches) {
		delete[] CFDependentBranches;
		CFDependentBranches = nullptr;
	}
	LaterExecutedBranchCount = 0;
	if (LaterExecutedBranches) {
		delete[] LaterExecutedBranches;
		LaterExecutedBranches = nullptr;
	}
	LFDynamicDependents.clear();
	CFDynamicDependents.clear();
	DynamicLaterExecutes.clear();
	PsuedoCFDynDeps.clear();
	PsuedoLFDynDeps.clear();

	CFNeeded_QueueSpecs = VK_QUEUEFLAG::CreateInvalidNullFlag();
	CurrentType = BranchType::UNDEFINED;
	AttachedSubmit = INVALID_SubmitID;
	PassCount = 0;
	if (CorePasses) {
		delete[] CorePasses;
		CorePasses = nullptr;
	}
	if (CurrentFramePassesIndexes) {
		delete[] CurrentFramePassesIndexes;
		CurrentFramePassesIndexes = nullptr;
	}

#ifdef VULKAN_DEBUGGING
	ID = INVALID_BRANCHID;
#endif
}
VK_RGBranch::VK_RGBranch() {
	Invalidate();
}
void VK_RGBranch::Initialize(VK_RGBranch::PassIndexType passcount) {
	PassCount = passcount;
	CorePasses = new VK_Pass * [passcount] {nullptr};
	CurrentFramePassesIndexes = new PassIndexType[passcount]{ PASSINDEXTYPE_INVALID };
}
void VK_RGBranch::CreateVulkanObjects() {
	if (CorePasses[0]->TYPE == PassType::WP) {
		if (PassCount > 1) {
			LOG(tgfx_result_FAIL, "If a branch has a window pass, it can't have any other pass!");
		}
		return;
	}
}
bool VK_RGBranch::IsValid() {
	for (BranchIDListType i = 0; i < PenultimateSwapchainBranchCount; i++) {
		if (PenultimateSwapchainBranches[i] == INVALID_BRANCHID) { LOG(tgfx_result_WARNING,  "Problem is in penultimate branches!");	return false; }
	}
	for (BranchIDListType i = 0; i < LaterExecutedBranchCount; i++) {
		if (LaterExecutedBranches[i] == INVALID_BRANCHID) { LOG(tgfx_result_WARNING,  "Problem is in current frame later executed branches!");	return false; }
	}
	for (BranchIDListType i = 0; i < LFDependentBranchCount; i++) {
		if (LFDependentBranches[i] == INVALID_BRANCHID) { LOG(tgfx_result_WARNING,  "Problem is in last frame dependent branches!");	return false; }
	}
	for (BranchIDListType i = 0; i < CFDependentBranchCount; i++) {
		if (CFDependentBranches[i] == INVALID_BRANCHID) { LOG(tgfx_result_WARNING,  "Problem is in current frame dependent branches!");	return false; }
	}
#ifdef VULKAN_DEBUGGING
	if (ID == INVALID_BRANCHID) { LOG(tgfx_result_WARNING,  "Problem is in branch id!");	return false; }
#endif
	if (SiblingBranchID == INVALID_BRANCHID) { LOG(tgfx_result_WARNING,  "Problem is in sibling branch id!"); return false; }

	if (!CurrentFramePassesIndexes) { LOG(tgfx_result_WARNING,  "Problem is in CurrentFramePassesIndexes array!"); return false; };

	if (!CorePasses) { LOG(tgfx_result_WARNING,  "Problem is in CorePasses array!"); return false; };
	if (!PassCount || PassCount == PASSINDEXTYPE_INVALID) { LOG(tgfx_result_WARNING,  "Problem is in PassCount value!"); return false; };

	return true;
}





void VK_RGBranch::PushBack_CorePass(VK_Pass* PassHandle, PassType Type) {
	for (PassIndexType i = 0; i < PassCount; i++) {
		if (CorePasses[i]->TYPE == PassType::ERROR) {
			CorePasses[i] = PassHandle;
			CorePasses[i]->TYPE = Type;
			return;
		}
	}
	LOG(tgfx_result_FAIL, "VK_RGBranch::PushBack_CorePass() has failed! You either called it before initialization or full-filled the list!");
}
void VK_RGBranch::SetListSizes(VK_RGBranch::BranchIDListType PenultimateDependentBranchCount_i, VK_RGBranch::BranchIDListType LFDependentBranchCount_i,
	VK_RGBranch::BranchIDListType CFDependentBranchCount_i, VK_RGBranch::BranchIDListType CFLaterExecutedBranchCount_i) {
	if (PenultimateDependentBranchCount_i == MAXBranchCount + 1 || PenultimateDependentBranchCount_i == 0) {
		PenultimateSwapchainBranchCount = 0;
	}
	else {
		PenultimateSwapchainBranchCount = PenultimateDependentBranchCount_i;
		PenultimateSwapchainBranches = new BranchIDType[PenultimateSwapchainBranchCount];
	}

	if (LFDependentBranchCount_i == MAXBranchCount + 1 || LFDependentBranchCount_i == 0) {
		LFDependentBranchCount = 0;
	}
	else {
		LFDependentBranchCount = LFDependentBranchCount_i;
		LFDependentBranches = new BranchIDType[LFDependentBranchCount];
	}

	if (CFDependentBranchCount_i == MAXBranchCount + 1 || CFDependentBranchCount_i == 0) {
		CFDependentBranchCount = 0;
	}
	else {
		CFDependentBranchCount = CFDependentBranchCount_i;
		CFDependentBranches = new BranchIDType[CFDependentBranchCount];
	}

	if (CFLaterExecutedBranchCount_i == MAXBranchCount + 1 || CFLaterExecutedBranchCount_i == 0) {
		LaterExecutedBranchCount = 0;
	}
	else {
		LaterExecutedBranchCount = CFLaterExecutedBranchCount_i;
		LaterExecutedBranches = new BranchIDType[LaterExecutedBranchCount];
	}
}
void VK_RGBranch::PushBack_PenultimateDependentBranch(BranchIDType BranchID) {
	for (BranchIDListType i = 0; i < PenultimateSwapchainBranchCount; i++) {
		if (PenultimateSwapchainBranches[i] == INVALID_BRANCHID) {
			PenultimateSwapchainBranches[i] = BranchID;
			return;
		}
	}
	LOG(tgfx_result_FAIL, "VK_RGBranch::PushBack_PenultimateDependentBranch() has failed! You either called it before SetListSizes or full-filled the list!");
	return;
}
void VK_RGBranch::PushBack_LFDependentBranch(BranchIDType BranchID) {
	for (BranchIDListType i = 0; i < LFDependentBranchCount; i++) {
		if (LFDependentBranches[i] == INVALID_BRANCHID) {
			LFDependentBranches[i] = BranchID;
			return;
		}
	}
	LOG(tgfx_result_FAIL, "VK_RGBranch::PushBack_LFDependentBranch() has failed! You either called it before SetListSizes or full-filled the list!");
	return;
}
void VK_RGBranch::PushBack_CFDependentBranch(BranchIDType BranchID) {
	for (BranchIDListType i = 0; i < CFDependentBranchCount; i++) {
		if (CFDependentBranches[i] == INVALID_BRANCHID) {
			CFDependentBranches[i] = BranchID;
			return;
		}
	}
	LOG(tgfx_result_FAIL, "VK_RGBranch::PushBack_CFDependentBranch() has failed! You either called it before SetListSizes or full-filled the list!");
	return;
}
void VK_RGBranch::PushBack_CFLaterExecutedBranch(BranchIDType BranchID) {
	for (BranchIDListType i = 0; i < LaterExecutedBranchCount; i++) {
		if (LaterExecutedBranches[i] == INVALID_BRANCHID) {
			LaterExecutedBranches[i] = BranchID;
			return;
		}
	}
	LOG(tgfx_result_FAIL, "VK_RGBranch::PushBack_CFLaterExecutedBranch() has failed! You either called it before SetListSizes or full-filled the list!");
	return;
}


const VK_RGBranch::BranchIDType VK_RGBranch::GetID() {
#ifdef VULKAN_DEBUGGING
	return ID;
#else
	return this;
#endif
}
const VK_RGBranch::BranchIDType VK_RGBranch::GetSiblingID() {
	return SiblingBranchID;
}
void VK_RGBranch::SetSiblingID(BranchIDType sourceid) {
	SiblingBranchID = sourceid;
}
inline void VK_RGBranch::SetAttachedSubmit(SubmitIDType SubmitID) { AttachedSubmit = SubmitID; }
std::string VK_RGBranch::ConvertID_tostring(const VK_RGBranch::BranchIDType& ID) {
#ifdef VULKAN_DEBUGGING
	return std::to_string(ID);
#else
	std::ostd::stringstream address;
	address << (void const*)ID;
	return address.str();
#endif
}
VK_RGBranch::PassIndexType VK_RGBranch::GetPassCount() const {
	return PassCount;
}
std::vector<VK_RGBranch::BranchIDType>& VK_RGBranch::GetLastFDynamicDBranches() { return LFDynamicDependents; }
const std::vector<VK_RGBranch::BranchIDType>& VK_RGBranch::ReadLastFDynamicDBranches() const { return LFDynamicDependents; }
std::vector<VK_RGBranch::BranchIDType>& VK_RGBranch::GetCFDynamicDependentBranches() { return CFDynamicDependents; }
const std::vector<VK_RGBranch::BranchIDType>& VK_RGBranch::ReadCFDynamicDependentBranches() const { return CFDynamicDependents; }
std::vector<VK_RGBranch::BranchIDType>& VK_RGBranch::GetDynamicLaterExecuteBranches() { return DynamicLaterExecutes; }
const std::vector<VK_RGBranch::BranchIDType>& VK_RGBranch::ReadDynamicLaterExecuteBranches() const { return DynamicLaterExecutes; }
VK_Pass* VK_RGBranch::GetCorePass(PassIndexType CorePassIndex) const { return CorePasses[CorePassIndex]; }
VK_RGBranch::BranchIDListType VK_RGBranch::Get_PenultimateDependentBranchCount() const { return PenultimateSwapchainBranchCount; }
VK_RGBranch::BranchIDListType VK_RGBranch::Get_LFDependentBranchCount() const { return LFDependentBranchCount; }
VK_RGBranch::BranchIDListType VK_RGBranch::Get_CFDependentBranchCount() const { return CFDependentBranchCount; }
VK_RGBranch::BranchIDListType VK_RGBranch::Get_CFLaterExecutedBranchCount() const { return LaterExecutedBranchCount; }
VK_Submit& VK_RGBranch::Get_AttachedSubmit() const { return SUBMITSYS->GetSubmit_byID(AttachedSubmit); }
bool VK_RGBranch::is_AttachedSubmitValid() const {
	if (AttachedSubmit == INVALID_SubmitID) {
		return false;
	}
	return SUBMITSYS->GetSubmit_byID(AttachedSubmit).isValid();
}
VK_RGBranch::BranchIDType VK_RGBranch::Get_PenultimateDependentBranch_byIndex(BranchIDListType elementindex) const { return PenultimateSwapchainBranches[elementindex]; }
VK_RGBranch::BranchIDType VK_RGBranch::Get_LFDependentBranch_byIndex(BranchIDListType elementindex) const { return LFDependentBranches[elementindex]; }
VK_RGBranch::BranchIDType VK_RGBranch::Get_CFDependentBranch_byIndex(BranchIDListType elementindex) const { return CFDependentBranches[elementindex]; }
VK_RGBranch::BranchIDType VK_RGBranch::Get_CFLaterExecutedBranch_byIndex(BranchIDListType elementindex) const { return LaterExecutedBranches[elementindex]; }


void VK_RGBranch::ClearDynamicDatas() {
	ClearCFPassIndexes();
	CurrentType = BranchType::UNDEFINED;


	AttachedSubmit = INVALID_SubmitID;
	CFDynamicDependents.clear();
	LFDynamicDependents.clear();
	DynamicLaterExecutes.clear();
	PsuedoCFDynDeps.clear();
	PsuedoLFDynDeps.clear();

	VK_QUEUEFLAG empty;
	CFNeeded_QueueSpecs = empty;
}


//If there is no workload or some type of error occurs while deciding the type
//This function returns false and BranchType of the Branch becomes UNDEFINED
bool VK_RGBranch::CalculateBranchType() {
	CurrentType = BranchType::UNDEFINED;
	if (isCFPassIndexValid(0)) {
		return false;
	}
	VK_Pass* FirstWorkloadedPass = GetBranchPass_byIndex(CurrentFramePassesIndexes[0]);

	if (FirstWorkloadedPass->TYPE == PassType::WP) {
		//Check if there is anything wrong
		if (PassCount > 1) {	//Even this shouldn't happen!
			std::string crash_text = "A window branch can't have more than one pass!";
			if (isCFPassIndexValid(1)) {
				crash_text.append(" And the other pass has workload too! Please report this issue!");
			}
			LOG(tgfx_result_FAIL, crash_text.c_str());
			CurrentType = BranchType::UNDEFINED;
			return false;
		}
		CurrentType = BranchType::WindowBranch;
		return true;
	}
	//It is either barrier TP only, BusyStart_RenderBranch or BarrierStart_RenderBranch. Of course UNDEFINED is possible (I won't mention that any further).
	if (FirstWorkloadedPass->TYPE == PassType::TP) {
		VK_TransferPass* TP = ((VK_TransferPass*)FirstWorkloadedPass);
		switch (TP->TYPE)
		{
			//It is either barrier TP only or BarrierStart_RenderBranch.
		case tgfx_transferpass_type_BARRIER:
		{
			//If branch has only 1 pass, it is barrier tp only
			if (PassCount == 1) {
				CurrentType = BranchType::BarrierTPOnlyBranch;
				return true;
			}


			//If there is no other workloaded pass, it is barrier tp only
			if (!isCFPassIndexValid(1)) {
				CurrentType = BranchType::BarrierTPOnlyBranch;
				return true;
			}
			//If there are other workloaded passes. 
			else {
				//Check if all of them are barrier tp. 
				//If they are, it is barrier TP only. Otherwise, it is BarrierStart_RenderBranch.
				bool isBarrierTpOnly = true;
				for (unsigned char WorkloadedPassElement = 1; WorkloadedPassElement < PassCount && isBarrierTpOnly; WorkloadedPassElement++) {
					if (!isCFPassIndexValid(WorkloadedPassElement)) { continue; }
					VK_Pass* CurrentWorkloadedPass = GetBranchPass_byIndex(WorkloadedPassElement);

					//Check if it is Compute or Draw Pass
					if (CurrentWorkloadedPass->TYPE == PassType::CP ||
						CurrentWorkloadedPass->TYPE == PassType::DP)
					{
						isBarrierTpOnly = false;
					}
					//Check if it is Copy TP
					else if (CurrentWorkloadedPass->TYPE == PassType::TP) {
						if (((VK_TransferPass*)CurrentWorkloadedPass)->TYPE != tgfx_transferpass_type_BARRIER) {
							isBarrierTpOnly = false;
						}
					}
					//If it is ERROR or WindowPass, throw crashing. If there is new type of Pass, it hits here too!
					else {
						LOG(tgfx_result_FAIL, "There is a unsupported pass type in the branch! This shouldn't happen!");
						return false;
					}
				}

				if (isBarrierTpOnly) {
					CurrentType = BranchType::BarrierTPOnlyBranch;
				}
				else {
					CurrentType = BranchType::RenderBranch;
				}
				return true;
			}
		}
		break;
		//Only BusyStart_RenderBranch is possible!
		case tgfx_transferpass_type_COPY:
		{
			//Check if there is anything wrong in the following passes
			for (unsigned char PassElement = 1; PassElement < PassCount; PassElement++) {
				if (!CurrentFramePassesIndexes[PassElement]) {
					continue;
				}

				PassType passtype = GetBranchPass_byIndex(CurrentFramePassesIndexes[PassElement])->TYPE;
				if (passtype == PassType::WP || passtype == PassType::ERROR) {
					LOG(tgfx_result_FAIL, "Current branch is a BusyStart_RenderBranch but it has either a WindowPass or a ERROR pass, which is undefined!");
					return false;
				}
			}

			CurrentType = BranchType::RenderBranch;
		}
		break;
		default:
			LOG(tgfx_result_FAIL, "This TP type isn't allowed in CalculateBranchType()!");
			return false;
		}

	}
	if (FirstWorkloadedPass->TYPE == PassType::ERROR) {
		LOG(tgfx_result_FAIL, "First workloaded pass is a ERROR pass, this is undefined!");
		return false;
	}
	if (FirstWorkloadedPass->TYPE == PassType::CP ||
		FirstWorkloadedPass->TYPE == PassType::DP) {
		//Check if there is anything wrong in the following passes
		for (unsigned char PassElement = 1; PassElement < PassCount; PassElement++) {
			if (!isCFPassIndexValid(PassElement)) {
				continue;
			}
			VK_Pass* CurrentPass = GetBranchPass_byIndex(PassElement);
			if (CurrentPass->TYPE == PassType::WP ||
				CurrentPass->TYPE == PassType::ERROR) {
				LOG(tgfx_result_FAIL, "Current branch is a BusyStart_RenderBranch but it has either a WindowPass or a ERROR pass, which is undefined!");
				return false;
			}
		}

		CurrentType = BranchType::RenderBranch;
		return true;
	}
}

//////// Each frame, passes that has workload should be listed.

//Returns false if there is no job
bool VK_RGBranch::CreateWorkloadedPassList_oftheBranch() {

	bool Workloaded = false;
	//Check each dynamic pass if there is any work! If the CurrentFramePassesIndexes says its zero, there is no 
	for (unsigned char PassElement = 0; PassElement < PassCount; PassElement++) {
		VK_Pass* CurrentPass = CorePasses[PassElement];
		if (CurrentPass->IsWorkloaded()) {
			Workloaded = true;
			PushBack_toCFPassIndexList(PassElement);
		}
	}

	return Workloaded;
}
bool VK_RGBranch::isCFPassIndexValid(PassIndexType index) const {
	if (CurrentFramePassesIndexes[index] == PASSINDEXTYPE_INVALID) {
		return false;
	}
	return true;
}
void VK_RGBranch::ClearCFPassIndexes() {
	for (PassIndexType PassIndex = 0; PassIndex < PassCount; PassIndex++) {
		CurrentFramePassesIndexes[PassIndex] = PASSINDEXTYPE_INVALID;
	}
}
VK_Pass* VK_RGBranch::GetBranchPass_byIndex(PassIndexType index) {
	return CorePasses[CurrentFramePassesIndexes[index]];
}
//This is not a push back, gives index to the proper element in the CurrentFramePassesIndexes list
void VK_RGBranch::PushBack_toCFPassIndexList(PassIndexType CorePassIndex) {
	for (PassIndexType PassIndex = 0; PassIndex < PassCount; PassIndex++) {
		if (CurrentFramePassesIndexes[PassIndex] == PASSINDEXTYPE_INVALID) {
			CurrentFramePassesIndexes[PassIndex] = CorePassIndex;
			return;
		}
	}
}



// We need to create a flag to help decide which queue the branch should be run by
bool VK_RGBranch::CalculateNeededQueueFlag() {
	if (CurrentType == BranchType::UNDEFINED) {
		LOG(tgfx_result_FAIL, "VK_RGBranch::CalculateNeededQueueFlag() has failed because BranchType is UNDEFINED!");
		return false;
	}
	if (CurrentType == BranchType::WindowBranch || CurrentType == BranchType::BarrierTPOnlyBranch) {
		CFNeeded_QueueSpecs.doesntNeedAnything = true;

		CFNeeded_QueueSpecs.is_COMPUTEsupported = false;
		CFNeeded_QueueSpecs.is_GRAPHICSsupported = false;
		CFNeeded_QueueSpecs.is_PRESENTATIONsupported = false;
	}
	return true;
}


#ifdef VULKAN_DEBUGGING
const SubmitIDType VK_Submit::Get_ID() {
	return ID;
}

void VK_RGBranchSystem::SortByID() { if (BRANCHSYS->isSorted)		Sort(); }
void VK_SubmitSystem::SortByID() { if (SUBMITSYS->isSorted)	Sort(); }
VK_RGBranch& VK_RGBranchSystem::GetBranch_byID(VK_RGBranch::BranchIDType ID) { return GetBranch(ID); }
VK_RGBranch& VK_RGBranchSystem::GetBranch(VK_RGBranch::BranchIDType ID) {
	for (VK_RGBranch::BranchIDType i = 0; i < BRANCHSYS->Branches.size(); i++) {
		if (BRANCHSYS->Branches[i]->GetID() == ID) {
			return *BRANCHSYS->Branches[i];
		}
	}
	LOG(tgfx_result_FAIL, "There is no branch that has this ID!");
}
VK_Submit& VK_SubmitSystem::GetSubmit_byID(SubmitIDType ID) { return GetSubmit(ID); }
VK_Submit& VK_SubmitSystem::GetSubmit(SubmitIDType ID) {
	for (SubmitIDType i = 0; i < SUBMITSYS->Submits.size(); i++) {
		if (SUBMITSYS->Submits[i]->Get_ID() == ID) {
			return *SUBMITSYS->Submits[i];
		}
	}
	LOG(tgfx_result_FAIL, "There is no submit that has this ID!");
}
#else
const SubmitIDType VK_Submit::Get_ID() {
	return this;
}
VK_Submit& VK_SubmitSystem::CreateSubmit(VK_CommandBuffer::CommandBufferIDType CommandBufferID, VK_RGBranch::BranchIDType FirstBranchID) {
	VK_Submit s;
	LOG(tgfx_result_NOTCODED, "CreateSubmit isn't coded!", true);
	return s;
}

void VK_RGBranchSystem::SortByID() {}
void VK_SubmitSystem::SortByID() {}
VK_RGBranch& VK_RGBranchSystem::GetBranch_byID(VK_RGBranch::BranchIDType ID) { return *((VK_RGBranch*)ID); }
VK_Submit& VK_SubmitSystem::GetSubmit_byID(SubmitIDType ID) { LOG(tgfx_result_FAIL, "GetSubmit_byID()'nin pointer d�nd�rmesi saglanmali, hatta diger bircok get'in de. Cunku validation yapamiyorum."); return *((VK_Submit*)ID); }
#endif // VULKAN_DEBUGGING


VK_RGBranch& VK_RGBranchSystem::CreateBranch(VK_RGBranch::PassIndexType PassCount) {
	VK_RGBranch* Branch = new VK_RGBranch();
	Branch->Initialize(PassCount);

#ifdef VULKAN_DEBUGGING
	Branch->ID = BRANCHSYS->CreateID();
#endif

	BRANCHSYS->Branches.push_back(Branch);
}