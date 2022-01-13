#include "RGAlgorithm_TempDatas.h"
#define LOG VKCORE->TGFXCORE.DebugCallback
#define SEMAPHORESYS ((VK_SemaphoreSystem*)VKRENDERER->Semaphores)
#define BRANCHSYS ((VK_RGBranchSystem*)VKRENDERER->Branches)
#define SUBMITSYS ((VK_SubmitSystem*)VKRENDERER->Submits)



/**	INTRODUCTION
* This .cpp is core of the framegraph algorithm. Reconstruction, Execution and Destruction lies here and each of them is a Section.
* Start_RenderGraphConstruction() and Finish_RenderGraphConstruction() are the main functions of Reconstruction Section.
* Execute_RenderGraph() is the main function of Execution Section.
* Destroy_RenderGraph() is the main function of Destruction Section.
* Start by reading Reconstruction and Executing Sections to learn about the algorithm
* Similar datas are called different in specific sections of the algorithm, so be aware of the section/context you are at
* RenderGraph = FrameGraph in general, otherwise it is specified in the context
* RenderGraph Algorithm is divided into 2 main steps; Static, Dynamic. 
* Static doesn't consider workload of the passes, only considers manually defined relations.
* Dynamic considers workload of the passes and run (last, current and next frame) passes
* Static information is needed to speed up dynamic data creation and only changes in Reconstruction Section, so only calculated at Reconstruction Section
* Dynamic information is calculated every frame (because pass workloads are dynamic), so it's in Execution Section.
* For now; you can match Reconstruction Section -> Static Step, Execution -> Dynamic Step. But I can't say anything about future
*/

	//SECONDARY FUNCTION DECLARATIONs

//This function will reconstruct framegraph's static information (and will generate branches for next frame, you only need to duplicate framegraph in next frame)
bool Check_WaitHandles();
void Create_FrameGraphs();
void Create_VkDataofRGBranches(const VK_FrameGraph& FrameGraph);
void WaitForLastFrame(VK_FrameGraph& Last_FrameGraph);
void UnsignalLastFrameSemaphores(VK_FrameGraph& Last_FrameGraph);
void CreateSubmits_Intermediate(VK_FrameGraph& Current_FrameGraph);
void DuplicateFrameGraph(VK_FrameGraph& TargetFrameGraph, const VK_FrameGraph& SourceFrameGraph);
void Prepare_forSubmitCreation();
inline void CreateSubmits_Fast(VK_FrameGraph& Current_FrameGraph, VK_FrameGraph& LastFrameGraph);
void Send_RenderCommands();
void Send_PresentationCommands();
void RenderGraph_DataShifting();


//If user wants to create/change rendergaph, this function should be called first.
//Changes rendergraph status (which allow other rendergraph modifier functions to be used)
void Renderer::Start_RenderGraphConstruction() {
	//Apply resource changes made by user
	VKContentManager->Resource_Finalizations();

	if (VKRENDERER->RG_Status == RenderGraphStatus::FinishConstructionCalled || VKRENDERER->RG_Status == RenderGraphStatus::StartedConstruction) {
		LOG(tgfx_result_FAIL, "GFXRENDERER->Start_RenderGraphCreation() has failed because you have wrong function call order!"); return;
	}
	VKRENDERER->RG_Status = RenderGraphStatus::StartedConstruction;
}


/**
* If user called Start_RenderGraphConstruction() before, apply changes
* Returns true if construction is successful
* Inputs:
* IMGUI_Subpass: is used to change the subpass dear imgui will use to render UI. If you don't want to change, set as nullptr
*/
unsigned char Renderer::Finish_RenderGraphConstruction(tgfx_subdrawpass IMGUI_Subpass) {
	if (VKRENDERER->RG_Status != RenderGraphStatus::StartedConstruction) {
		LOG(tgfx_result_FAIL, "VulkanRenderer->Finish_RenderGraphCreation() has failed because you didn't call Start_RenderGraphConstruction() before!");
		return false;
	}

	//Checks wait handles of the render nodes, if matching fails return false
	{
		unsigned long long duration = 0;
		TURAN_PROFILE_SCOPE_MCS("Checking Wait Handles of RenderNodes", &duration)
			if (!Check_WaitHandles()) {
				LOG(tgfx_result_FAIL, "VulkanRenderer->Finish_RenderGraphConstruction() has failed because some wait handles have issues!");
				VKRENDERER->RG_Status = RenderGraphStatus::Invalid;	//User can change only waits of a pass, so all rendergraph falls to invalid in this case
				//It needs to be reconstructed by calling Start_RenderGraphConstruction()
				return false;
			}
		STOP_PROFILE_PRINTFUL_TAPI()
	}


	//Create static information
	Create_FrameGraphs();

	//Static information gives maximum number of command buffers we can need, create them beforehand
	Create_VkDataofRGBranches(VKRENDERER->FrameGraphs[0]);


	//If dear Imgui is created, apply subpass changes and render UI
	if (VK_IMGUI) {
		//Initialize IMGUI if it isn't initialized
		if (IMGUI_Subpass) {
			//If subdrawpass has only one color slot, return false
			if (((drawpass_vk*)((VK_SubDrawPass*)IMGUI_Subpass)->DrawPass)->SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT != 1) {
				LOG(tgfx_result_FAIL, "The Drawpass that's gonna render dear IMGUI should only have one color slot!");
				VKRENDERER->RG_Status = RenderGraphStatus::Invalid;	//User can delete a draw pass, dear Imgui fails in this case.
				//To avoid this, it needs to be reconstructed by calling Start_RenderGraphConstruction()
				return false;
			}

			if (VK_IMGUI->Get_IMGUIStatus() == IMGUI_VK::IMGUI_STATUS::UNINITIALIZED) {
				//Create a special Descriptor Pool for IMGUI
				VkDescriptorPoolSize pool_sizes[] =
				{
					{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
				};

				VkDescriptorPoolCreateInfo descpool_ci;
				descpool_ci.flags = 0;
				descpool_ci.maxSets = 1;
				descpool_ci.pNext = nullptr;
				descpool_ci.poolSizeCount = 1;
				descpool_ci.pPoolSizes = pool_sizes;
				descpool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				if (vkCreateDescriptorPool(RENDERGPU->LOGICALDEVICE(), &descpool_ci, nullptr, &VKRENDERER->IMGUIPOOL) != VK_SUCCESS) {
					LOG(tgfx_result_FAIL, "Creating a descriptor pool for dear IMGUI has failed!");
					return false;
				}

				VK_IMGUI->Initialize(IMGUI_Subpass, VKRENDERER->IMGUIPOOL);
				VK_IMGUI->UploadFontTextures();
			}
			//If given subpass is different from the previous one, change subpass
			else if(((VK_SubDrawPass*)VK_IMGUI->Get_SubDrawPassHandle()) != ((VK_SubDrawPass*)IMGUI_Subpass)) {
				((VK_SubDrawPass*)VK_IMGUI->Get_SubDrawPassHandle())->render_dearIMGUI = false;
				((VK_SubDrawPass*)IMGUI_Subpass)->render_dearIMGUI = true;
				VK_IMGUI->Change_DrawPass(IMGUI_Subpass);
			}
		}

		VK_IMGUI->NewFrame();
	}

	VKRENDERER->RG_Status = RenderGraphStatus::FinishConstructionCalled;
	return true;
}

bool Renderer::Execute_RenderGraph() {
	TURAN_PROFILE_SCOPE_MCS("Run_CurrentFramegraph()", nullptr)
		unsigned char CurrentFrameIndex = VKRENDERER->Get_FrameIndex(false), LastFrameIndex = VKRENDERER->Get_FrameIndex(true);
	VK_FrameGraph& Current_FrameGraph = VKRENDERER->FrameGraphs[CurrentFrameIndex], & Last_FrameGraph = VKRENDERER->FrameGraphs[LastFrameIndex];

	switch (VKRENDERER->RG_Status)
	{
	case RenderGraphStatus::Invalid:
	case RenderGraphStatus::StartedConstruction:
		LOG(tgfx_result_FAIL, "VulkanRenderer:Execute_RenderGraph() has failed because your rendergraph is either invalid or Finish_RenderGraphConstruction() isn't called!");
		return false;
	case RenderGraphStatus::FinishConstructionCalled:
		Create_FrameGraphs();

		//Wait for all render operations of the last frame to end
		WaitForLastFrame(Last_FrameGraph);

		//Unsignal last frame's semaphores to recycle them
		UnsignalLastFrameSemaphores(Last_FrameGraph);

		CreateSubmits_Intermediate(Current_FrameGraph);

		VKRENDERER->RG_Status = RenderGraphStatus::HalfConstructed;	//When reconstruction finishes, it is half constructed. 
		//If reconstruction isn't called next frame, framegraph should be duplicated next frame
		break;
	case RenderGraphStatus::HalfConstructed:
		DuplicateFrameGraph(Current_FrameGraph, Last_FrameGraph);

		VKRENDERER->RG_Status = RenderGraphStatus::Valid;
		//RG becomes valid, so we can safely act as Valid now. So don't use "break;" here
	case RenderGraphStatus::Valid:
		//Some previously allocated lists needs to be cleared and RenderGraph branches needs to be refreshed
		Prepare_forSubmitCreation();

		//Rendergraph consists of 2 similar framegraphs (that references each other), so create submits in the fast way
		CreateSubmits_Fast(Current_FrameGraph, Last_FrameGraph);
		break;
	default:
		LOG(tgfx_result_FAIL, "VulkanRenderer:Execute_RenderGraph() has failed because rendergraph's status isn't supported!");
		return false;
	}


	Send_RenderCommands();

	Send_PresentationCommands();

	//There are lots of data structures to handle 2 frames
	//So we need to shift some datas because the way we access them
	RenderGraph_DataShifting();
	STOP_PROFILE_PRINTFUL_TAPI()
}

void Renderer::Destroy_RenderGraph() {
	vkDeviceWaitIdle(RENDERGPU->LOGICALDEVICE());
	VKContentManager->Destroy_RenderGraphRelatedResources();

	delete SEMAPHORESYS;
	for (unsigned int DrawPassIndex = 0; DrawPassIndex < VKRENDERER->DrawPasses.size(); DrawPassIndex++) {
		drawpass_vk* DP = VKRENDERER->DrawPasses[DrawPassIndex];
		vkDestroyFramebuffer(RENDERGPU->LOGICALDEVICE(), DP->FBs[0], nullptr);
		vkDestroyFramebuffer(RENDERGPU->LOGICALDEVICE(), DP->FBs[1], nullptr);
		vkDestroyRenderPass(RENDERGPU->LOGICALDEVICE(), DP->RenderPassObject, nullptr);
		delete DP;
	}
	VKRENDERER->DrawPasses.clear();
	for (unsigned int TransferPassIndex = 0; TransferPassIndex < VKRENDERER->TransferPasses.size(); TransferPassIndex++) {
		delete VKRENDERER->TransferPasses[TransferPassIndex];
	}
	VKRENDERER->TransferPasses.clear();
	for (unsigned int WindowPassIndex = 0; WindowPassIndex < VKRENDERER->WindowPasses.size(); WindowPassIndex++) {
		delete VKRENDERER->WindowPasses[WindowPassIndex];
	}
	VKRENDERER->WindowPasses.clear();

	SUBMITSYS->System_Destroy();
	for (unsigned char i = 0; i < 2; i++) {
		VK_FrameGraph& FG = VKRENDERER->FrameGraphs[i];


		for (unsigned int BranchIndex = 0; BranchIndex < FG.BranchCount; BranchIndex++) {
			VK_RGBranch& Branch = FG.FrameGraphTree[BranchIndex];

			if (Branch.CFDependentBranches) {
				delete[] Branch.CFDependentBranches;
			}

			delete[] Branch.CorePasses;
			if (Branch.LaterExecutedBranches) {
				delete[] Branch.LaterExecutedBranches;
			}
			if (Branch.LFDependentBranches) {
				delete[] Branch.LFDependentBranches;
			}
			if (Branch.PenultimateSwapchainBranches) {
				delete[] Branch.PenultimateSwapchainBranches;
			}
		}
		delete[] FG.FrameGraphTree;
		FG.FrameGraphTree = nullptr;
	}

	//Delete Command Buffer related data from the renderer GPU
	for (unsigned int QueueIndex = 0; QueueIndex < RENDERGPU->QUEUEFAMS().size(); QueueIndex++) {
		VK_QUEUEFAM& Queue = RENDERGPU->QUEUEFAMS()[QueueIndex];
		Queue.ActiveSubmits.clear();
		vkResetCommandPool(RENDERGPU->LOGICALDEVICE(), Queue.CommandPools[0].CPHandle, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
		vkResetCommandPool(RENDERGPU->LOGICALDEVICE(), Queue.CommandPools[1].CPHandle, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
	}
}