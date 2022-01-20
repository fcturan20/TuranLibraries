#pragma once
#include "Vulkan_Renderer_Core.h"
#include "VK_GPUContentManager.h"
#include "Vulkan_Resource.h"
#include "Vulkan_Core.h"

////////////	Generalization of pass info and linked list for faster construction of framegraph's branch info!

struct VK_PassLinkedList {
	VK_Pass* CurrentGP = nullptr;
	VK_PassLinkedList* NextElement = nullptr;
};
VK_PassLinkedList* Create_PLinkedList() {
	return new VK_PassLinkedList;
}
void PushBack_ToPLinkedList(VK_Pass* Pass, VK_PassLinkedList* GPLL) {
	//Go to the last element in the list!
	while (GPLL->NextElement != nullptr) {
		GPLL = GPLL->NextElement;
	}
	//If element points to a GP, create new element
	if (GPLL->CurrentGP != nullptr) {
		VK_PassLinkedList* newelement = Create_PLinkedList();
		GPLL->NextElement = newelement;
		newelement->CurrentGP = Pass;
	}
	//Otherwise, make element point to the GP
	else {
		GPLL->CurrentGP = Pass;
	}
}
//Return the new header if the header changes!
//If header isn't found, given header is returned
//If header is the last element and should be deleted, then nullptr is returned
//If header isn't the last element but should be deleted, then next element is returned
VK_PassLinkedList* DeleteP_FromPLinkedList(VK_PassLinkedList* Header, VK_Pass* ElementToDelete) {
	VK_PassLinkedList* CurrentCheckItem = Header;
	VK_PassLinkedList* LastChecked_Item = nullptr;
	while (CurrentCheckItem->CurrentGP != ElementToDelete) {
		if (!CurrentCheckItem->NextElement) {
			//printer(result_tgfx_SUCCESS, "VulkanRenderer: DeleteGP_FromGPLinkedList() couldn't find the GP in the list!");
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





//Branch type is a way to mark a branch to reduce amount of times to classify the branch
//Static types means they can not be changed, not even become UNDEFINED when there is no workload
//Dynamic types means their type depends on the workload and pass list, becomes UNDEFINED when there is no workload
//Processible types means the branch's dynamic dependencies are found. The type depends on workload, pass list and the type of dynamic dependencies.
enum class BranchType : unsigned char {
	WindowBranch = 0,						//Static type. A Window Pass has its own branch always.

	BarrierTPOnlyBranch = 1,				//Dynamic Type. There are only Barrier TP active passes.
	RenderBranch,							//Dynamic Type. There are active passes either Copy, Draw or Compute. There may be Barrier passes too.

	RootBranch,								//Processible Type. This branch can't be attached to any submit so create a submit.
	//Because it doesn't have any CFDynamicDependent. They may have LFDynamicDependent(s) or DynamicLaterExecute(s), they aren't important.

	NoDynLaterExec,							//Processible Type. This branch can be attached to a submit or create a submit.
	//It doesn't have any DynamicLaterExecute. It has only one CFDynamicDependent. There is not any LFDynamicDependent.

	DynLaterExecs,							//Processible Type. This branch can be attached to a submit or creates a submit.
	//It has DynamicLaterExecute(s). It has only one CFDynamicDependent. There is not any LFDynamicDependent.
	//If it has more than one DynamicLaterExecutes, it ends attached the submit.

	SeperateBranch,							//Processible Type. This type of branches can't be attached to any submit, so have to create a submit.
	//It has LFDynamicDependent(s) or more than one CFDynamicDependents. DynamicLaterExecute(s) are not important.

	UNDEFINED
};


//Forward declarations and ID type definitions
#ifdef VULKAN_DEBUGGING
typedef unsigned int SubmitIDType;
static constexpr SubmitIDType INVALID_SubmitID = UINT32_MAX;
#else
typedef VK_Submit* SubmitIDType;
static constexpr SubmitIDType INVALID_SubmitID = nullptr;
#endif
struct VK_Submit;
/*
1) VK_RGBranch consists of one or more passes that should be executed serially without executing any other passes
Note: You can think a VK_RGBranch as a VkSubmitInfo with one command buffer if its all passes are active
2) VK_RGBranch will be called RB for this spec
3) a RB start after one or more RBs and end before one or more RBs
4) Render Graph starts with Root Passes that doesn't wait for any of the current passes
Note: Don't forget that a Pass may be a Rendering Path too (It doesn't matter if it is a root pass or not)
5) Passes that waits for the same passes will branch and create their own RBs
6) If a pass waits for passes that are in different RBs, then this pass will connect them and starts a new RB
Note: Of course if all of the wait passes are in same RB, algorithm finds the order of execution.
7) For maximum workload, each RGBranch stores a signal semaphore and list of wait semaphores that point to the signal semaphores of other RGBranches
But GFX API doesn't have to use the listed semaphores, because there may not be any workload of this branch or any of the dependent branches
8) ID is only used for now and same branch in different frames have the same ID (Because this ID is generated in current frame construction proccess)
9) CorePasses is a linked list of all passes that created at RenderGraph_Construction process
10) CFPasses is a linked list of all passes of the CorePasses that has some workload and CFNeed_QueueSpecs is the queue support flag of these passes
11) GFX API will analyze the workload of the all passes in the Branch each frame and create CFPasses and CFNeed_QueueSpecs
*/

struct VK_RGBranch {
	//Static Datas

	//PassIndexType is unsigned char for now, because 255+ branch is very theoric
	//This is to change system much more easier when I want more
	typedef unsigned char PassIndexType;
	static constexpr PassIndexType MAXPASSCOUNT = 255, PASSINDEXTYPE_INVALID = 255;
	//I want to support dynamic rendergraph reconstruction, so pointers become problematic
	//ID is a better approach and unsigned int seems a better range than unsigned char

#ifdef VULKAN_DEBUGGING
	typedef unsigned int BranchIDType;
	static constexpr BranchIDType INVALID_BRANCHID = UINT32_MAX;
#else
	typedef VK_RGBranch* BranchIDType;
	static constexpr BranchIDType INVALID_BRANCHID = nullptr;
#endif // VULKAN_DEBUGGING
	//This type should be unsigned and between 1-8 bytes
	//This is only used for loops (because there are lots of loops, performance may change between 1-8 bytes)
	typedef unsigned int BranchIDListType;
	static constexpr BranchIDListType MAXBranchCount = UINT32_MAX - 1;

	//Dynamic Datas

	//If there is no job, returns false
	//If an error occurs, it calls LOG_CRASHING and then returns false
	inline bool PrepareForNewFrame();
	//Use this funtion carefully, this changes branch type
	inline void FindProcessibleType();
	inline BranchType GetBranchType() const;
	inline VK_QUEUEFLAG GetNeededQueueFlag() const;
	inline bool IsWorkloaded() const;
	//To reset a all informations and re-initialize branch data
	inline void Invalidate();
	VK_RGBranch();				//Creates an invalidated branch
	inline void Initialize(PassIndexType PassCount);
	bool IsValid();
	//This function will generate necessary objects for each CorePass
	//For example: if branch is a WindowPassOnlyBranch, command buffer isn't generated
	void CreateVulkanObjects();

	void PushBack_CorePass(VK_Pass* PassHandle, PassType Type);	//This is not a vector::push_back, it just locates core pass to the next unused element in the list
	//Call this function before calling PushBack functions below
	void SetListSizes(BranchIDListType PenultimateDependentBranchCount, BranchIDListType LFDependentBranchCount = MAXBranchCount + 1,
		BranchIDListType CFDependentBranchCount = MAXBranchCount + 1, BranchIDListType CFLaterExecutedBranchCount = MAXBranchCount + 1);
	void PushBack_PenultimateDependentBranch(BranchIDType BranchID);
	void PushBack_LFDependentBranch(BranchIDType BranchID);
	void PushBack_CFDependentBranch(BranchIDType BranchID);
	void PushBack_CFLaterExecutedBranch(BranchIDType BranchID);
	inline void SetSiblingID(BranchIDType SiblingID);
	inline void SetAttachedSubmit(SubmitIDType SubmitID);




	inline const BranchIDType GetID(); inline const BranchIDType GetSiblingID();
	inline static std::string ConvertID_tostring(const BranchIDType& ID);
	inline PassIndexType GetPassCount() const;
	inline std::vector<BranchIDType>& GetLastFDynamicDBranches();
	inline const std::vector<BranchIDType>& ReadLastFDynamicDBranches() const;
	inline std::vector<BranchIDType>& GetCFDynamicDependentBranches();
	inline const std::vector<BranchIDType>& ReadCFDynamicDependentBranches() const;
	inline std::vector<BranchIDType>& GetDynamicLaterExecuteBranches();
	inline const std::vector<BranchIDType>& ReadDynamicLaterExecuteBranches() const;
	inline std::vector<BranchIDType>& GetPsuedoCFDynDBranches();
	inline std::vector<BranchIDType>& GetPsuedoLFDynDBranches();

	inline BranchIDListType Get_PenultimateDependentBranchCount() const;
	inline BranchIDListType Get_LFDependentBranchCount() const;
	inline BranchIDListType Get_CFDependentBranchCount() const;
	inline BranchIDListType Get_CFLaterExecutedBranchCount() const;
	//Call this function only if you're sure the submit is valid
	//If you are not, then call is_AttachedSubmitValid() first!
	inline VK_Submit& Get_AttachedSubmit() const;
	//Call this function if you don't know if submit is valid or not
	bool is_AttachedSubmitValid() const;

	inline BranchIDType Get_PenultimateDependentBranch_byIndex(BranchIDListType elementindex) const;
	inline BranchIDType Get_LFDependentBranch_byIndex(BranchIDListType elementindex) const;
	inline BranchIDType Get_CFDependentBranch_byIndex(BranchIDListType elementindex) const;
	inline BranchIDType Get_CFLaterExecutedBranch_byIndex(BranchIDListType elementindex) const;
	inline VK_Pass* GetCorePass(PassIndexType CorePassIndex) const;
private:
#ifdef VULKAN_DEBUGGING
	BranchIDType ID;
#endif // VULKAN_DEBUGGING
	//Defined ID here references the branch that has same passes as current branch but is in a different framegraph
	//Referencing it here makes duplicating a framegraph easier but it can help in other ways too (I'm not concerned about them)
	BranchIDType SiblingBranchID = INVALID_BRANCHID;

	VK_Pass** CorePasses = nullptr;
	PassIndexType PassCount;
	BranchIDType* PenultimateSwapchainBranches = nullptr, * LFDependentBranches = nullptr, * CFDependentBranches = nullptr, * LaterExecutedBranches = nullptr;
	BranchIDListType PenultimateSwapchainBranchCount, LFDependentBranchCount, CFDependentBranchCount = 0, LaterExecutedBranchCount;


	SubmitIDType AttachedSubmit;
	std::vector<BranchIDType> LFDynamicDependents, CFDynamicDependents, DynamicLaterExecutes, PsuedoCFDynDeps, PsuedoLFDynDeps;


	VK_QUEUEFLAG CFNeeded_QueueSpecs;
	//This list is used to store which passes in CorePasses list has workload
	//You shouldn't touch this list; you should use either isCFPassIndexValid(), ClearCFPassIndexes(), GetBranchPass_byIndex() or PushBack_toCFPassIndexList()
	PassIndexType* CurrentFramePassesIndexes = nullptr;


	//BranchType is important to decide how to connect this branch to others, so store it
	BranchType CurrentType = BranchType::UNDEFINED;


	void ClearDynamicDatas();


	//If there is no workload or some type of error occurs while deciding the type
	//This function returns false and BranchType of the Branch becomes UNDEFINED
	bool CalculateBranchType();

	//////// Each frame, passes that has workload should be listed.

	//Returns false if there is no job
	bool CreateWorkloadedPassList_oftheBranch();
	bool isCFPassIndexValid(PassIndexType index) const;
	void ClearCFPassIndexes() {
		for (PassIndexType PassIndex = 0; PassIndex < PassCount; PassIndex++) {
			CurrentFramePassesIndexes[PassIndex] = PASSINDEXTYPE_INVALID;
		}
	}
	VK_Pass* GetBranchPass_byIndex(PassIndexType index);
	//This is not a push back, gives index to the proper element in the CurrentFramePassesIndexes list
	void PushBack_toCFPassIndexList(PassIndexType CorePassIndex);

	// We need to create a flag to help decide which queue the branch should be run by
	bool CalculateNeededQueueFlag();

	friend class VK_RGBranchSystem;
};

struct VK_RGBranchSystem {
	inline static void SortByID();
	static void ClearList();
	static VK_RGBranch& CreateBranch(VK_RGBranch::PassIndexType PassCount);
	//Don't call this function in release build if there is no branch that has this ID
	//But debug build catches this type of issues
	inline static VK_RGBranch& GetBranch_byID(VK_RGBranch::BranchIDType ID);
private:
	//This branch is to return as invalid
	VK_RGBranch INVALIDBRANCH;
	std::vector<VK_RGBranch*> Branches;
#ifdef VULKAN_DEBUGGING
	VK_RGBranch::BranchIDType CreateID();
	bool isSorted = true;
	static VK_RGBranch& GetBranch(VK_RGBranch::BranchIDType ID);
	static void Sort();
#endif // VULKAN_DEBUGGING

};








enum class SubmitWaitType : unsigned char {
	LASTFRAME = 0,
	CURRENTFRAME = 1
};
struct VK_SubmitWaitInfo {
	SubmitIDType WaitedSubmit;
	VkPipelineStageFlags WaitedStage;
	SubmitWaitType WaitType;
};


//There are 2 categories of SubmitTypes: Preparation Types and Final Types
//Preparation Types are used while attaching branches
//Final Types are used while sending the submit
enum class SubmitType : unsigned char {
	//THESE ARE PREPARATION TYPES
	UNDEFINED = 0,
	NoMoreBranch = 1,

	/// <summary>
	/// These are Final Types that are used while sending the submit
	/// EndSubmit means there is no other submit in the same frame that waits for this submit
	/// </summary>

	EmptySubmit,
	Root_NonEndSubmit,
	NonRoot_NonEndSubmit,
	NonRoot_EndSubmit,
	Root_EndSubmit
};

struct VK_Submit {
	//Creates an invalidated submit
	VK_Submit();
	void Invalidate();
	void Initialize(VK_CommandBuffer::CommandBufferIDType CommandBufferID, VK_RGBranch::BranchIDType FirstBranchID, SubmitIDType SubmitID);
	bool isValid() const;

	inline const SubmitIDType Get_ID();
	void PushBack_WaitInfo(SubmitIDType Submit, VkPipelineStageFlags WaitStage, SubmitWaitType WaitType);
	void PushBack_SignalSemaphore(VK_Semaphore::SemaphoreIDType SemaphoreID);
	void PushBack_RGBranch(VK_RGBranch::BranchIDType BranchID);
	const std::vector<VK_Semaphore::SemaphoreIDType>& Get_SignalSemaphoreIDs() const;
	const std::vector<VK_RGBranch::BranchIDType>& Get_ActiveBranchList() const;
	void Set_SubmitType(SubmitType type);
	SubmitType Get_SubmitType() const;
	VK_QUEUEFLAG& Get_QueueFlag();
	VK_QUEUEFAM* Get_RunQueue() const;
private:
	std::vector<VK_SubmitWaitInfo> WaitInfos;
	std::vector<VK_Semaphore::SemaphoreIDType> SignalSemaphoreIDs;
	VK_CommandBuffer::CommandBufferIDType CB_ID = VK_CommandBuffer::INVALID_ID;
	std::vector<VK_RGBranch::BranchIDType> BranchIDs;
	VK_QUEUEFAM* Run_Queue = nullptr;
	SubmitType Type;
#ifdef VULKAN_DEBUGGING
	SubmitIDType ID = INVALID_SubmitID;
#endif // VULKAN_DEBUGGING

};

struct VK_SubmitSystem {
	inline void SortByID();
	void ClearList();
	VK_Submit& CreateSubmit(VK_CommandBuffer::CommandBufferIDType CommandBufferID, VK_RGBranch::BranchIDType FirstBranchID);
	inline VK_Submit& GetSubmit_byID(SubmitIDType ID);
	//A submit object isn't completely destroyed, this just invalidates the object
	//There is no way to completely destroy one object from memory, you have to destroy submitsystem
	inline void DestroySubmit(SubmitIDType ID);
	inline void System_Destroy();
private:
	std::vector<VK_Submit*> Submits;
#ifdef VULKAN_DEBUGGING
	bool isSorted = true;
	static void Sort();
	static VK_Submit& GetSubmit(SubmitIDType ID);
#endif
};