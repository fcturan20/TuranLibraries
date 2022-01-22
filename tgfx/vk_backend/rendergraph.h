#include "predefinitions_vk.h"
#include "synchronization_sys.h"
#include "core.h"
#include "queue.h"
#include "resource.h"
#include <vector>
//Don't include this header except rendergraph algorithm source files

					//Forward declarations and ID type definitions

struct passLinkedList_vk;
struct submitwaitinfo_vk;
struct submit_vk;
struct submitsys_vk;
struct branch_vk;
struct branchsys_vk;
struct framegraph_vk;
#ifdef VULKAN_DEBUGGING
typedef unsigned int BranchIDType;
static constexpr BranchIDType INVALID_BranchID = UINT32_MAX;
typedef unsigned int SubmitIDType;
static constexpr SubmitIDType INVALID_SubmitID = UINT32_MAX;
#else
typedef branch_vk* BranchIDType;
static constexpr BranchIDType INVALID_BranchID = nullptr;
typedef submit_vk* SubmitIDType;
static constexpr SubmitIDType INVALID_SubmitID = nullptr;
#endif

					//Enums

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
enum class SubmitWaitType : unsigned char {
	LASTFRAME = 0,
	CURRENTFRAME = 1
};

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




class rendergraph_functions {
};



							//STRUCTS



struct submitwaitinfo_vk {
	SubmitIDType WaitedSubmit;
	VkPipelineStageFlags WaitedStage;
	SubmitWaitType WaitType;
};

struct branch_vk {
	const queueflag_vk& GetNeededQueueFlag() { return supportflag; }
private:
	queueflag_vk supportflag;
};

struct submit_vk {
	//Creates an invalidated submit
	submit_vk();
	void Invalidate();
	void Initialize(commandbuffer_idtype_vk CommandBufferID, BranchIDType FirstBranchID, SubmitIDType SubmitID);
	bool isValid() const;

	inline const SubmitIDType Get_ID();
	void PushBack_WaitInfo(SubmitIDType Submit, VkPipelineStageFlags WaitStage, SubmitWaitType WaitType);
	void PushBack_SignalSemaphore(semaphore_idtype_vk SemaphoreID);
	void PushBack_RGBranch(BranchIDType BranchID);
	const std::vector<semaphore_idtype_vk>& Get_SignalSemaphoreIDs() const;
	const std::vector<BranchIDType>& Get_ActiveBranchList() const;
	void Set_SubmitType(SubmitType type);
	SubmitType Get_SubmitType() const;
	queueflag_vk& Get_QueueFlag();
	queuefam_vk* Get_RunQueue() const;
private:
	std::vector<submitwaitinfo_vk> WaitInfos;
	std::vector<semaphore_idtype_vk> SignalSemaphoreIDs;
	commandbuffer_idtype_vk CB_ID = INVALID_CommandBufferID;
	std::vector<BranchIDType> BranchIDs;
	queuefam_vk* Run_Queue = nullptr;
	SubmitType Type;
#ifdef VULKAN_DEBUGGING
	SubmitIDType ID = INVALID_SubmitID;
#endif // VULKAN_DEBUGGING
};

struct branchsys_vk {

};

struct submitsys_vk {
	inline void SortByID();
	void ClearList();
	submit_vk& CreateSubmit(commandbuffer_idtype_vk CommandBufferID, BranchIDType FirstBranchID);
	inline submit_vk& GetSubmit_byID(SubmitIDType ID);
	//A submit object isn't completely destroyed, this just invalidates the object
	//There is no way to completely destroy one object from memory, you have to destroy submitsystem
	inline void DestroySubmit(SubmitIDType ID);
	inline void System_Destroy();
private:
	std::vector<submit_vk*> Submits;
#ifdef VULKAN_DEBUGGING
	bool isSorted = true;
	static void Sort();
	static submit_vk& GetSubmit(SubmitIDType ID);
#endif
};


struct framegraph_vk {
	branch_vk* FrameGraphTree;
	unsigned char branchcount = 255;
};

struct framegraphsys_vk {
	//SECONDARY FUNCTION DECLARATIONs

	//This function will reconstruct framegraph's static information (and will generate branches for next frame, you only need to duplicate framegraph in next frame)
	bool Check_WaitHandles();
	void Create_FrameGraphs();
	void Create_VkDataofRGBranches(const framegraph_vk& FrameGraph);
	void WaitForLastFrame(framegraph_vk& Last_FrameGraph);
	void UnsignalLastFrameSemaphores(framegraph_vk& Last_FrameGraph);
	void CreateSubmits_Intermediate(framegraph_vk& Current_FrameGraph);
	void DuplicateFrameGraph(framegraph_vk& TargetFrameGraph, const framegraph_vk& SourceFrameGraph);
	void Prepare_forSubmitCreation();
	void CreateSubmits_Fast(framegraph_vk& Current_FrameGraph, framegraph_vk& LastFrameGraph);
	void Send_RenderCommands();
	void Send_PresentationCommands();
	void RenderGraph_DataShifting();
	friend void Start_RenderGraphConstruction();
	friend unsigned char Finish_RenderGraphConstruction(subdrawpass_tgfx_handle IMGUI_Subpass);
	friend void Destroy_RenderGraph();

	framegraph_vk framegraphs[2];
};
extern framegraphsys_vk framegraphsys;