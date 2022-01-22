#include "renderer.h"
#include "rendergraph.h"
#include "gpucontentmanager.h"
#include "imgui.h"
framegraphsys_vk framegraphsys;

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



void Start_RenderGraphConstruction() {
	//Apply resource changes made by user
	contentmanager->Resource_Finalizations();

	if (renderer->RGSTATUS() == RenderGraphStatus::FinishConstructionCalled || renderer->RGSTATUS() == RenderGraphStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "GFXRENDERER->Start_RenderGraphCreation() has failed because you have wrong function call order!"); return;
	}
	renderer->RG_Status = RenderGraphStatus::StartedConstruction;
}
unsigned char Finish_RenderGraphConstruction(subdrawpass_tgfx_handle IMGUI_Subpass) {
	if (renderer->RG_Status != RenderGraphStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "VulkanRenderer->Finish_RenderGraphCreation() has failed because you didn't call Start_RenderGraphConstruction() before!");
		return false;
	}

	//Checks wait handles of the render nodes, if matching fails return false
	if (!framegraphsys.Check_WaitHandles()) {
		printer(result_tgfx_FAIL, "VulkanRenderer->Finish_RenderGraphConstruction() has failed because some wait handles have issues!");
		renderer->RG_Status = RenderGraphStatus::Invalid;	//User can change only waits of a pass, so all rendergraph falls to invalid in this case
		//It needs to be reconstructed by calling Start_RenderGraphConstruction()
		return false;
	}


	//Create static information
	framegraphsys.Create_FrameGraphs();

	//Static information gives maximum number of command buffers we can need, create them beforehand
	framegraphsys.Create_VkDataofRGBranches(framegraphsys.framegraphs[0]);

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
	unsigned char CurrentFrameIndex = renderer->Get_FrameIndex(false), LastFrameIndex = renderer->Get_FrameIndex(true);
	framegraph_vk& Current_FrameGraph = framegraphsys.framegraphs[CurrentFrameIndex], & Last_FrameGraph = framegraphsys.framegraphs[LastFrameIndex];

	switch (renderer->RGSTATUS())
	{
	case RenderGraphStatus::Invalid:
	case RenderGraphStatus::StartedConstruction:
		printer(result_tgfx_FAIL, "VulkanRenderer:Execute_RenderGraph() has failed because your rendergraph is either invalid or Finish_RenderGraphConstruction() isn't called!");
		return result_tgfx_FAIL;
	case RenderGraphStatus::FinishConstructionCalled:
		framegraphsys.Create_FrameGraphs();

		//Wait for all render operations of the last frame to end
		framegraphsys.WaitForLastFrame(Last_FrameGraph);

		//Unsignal last frame's semaphores to recycle them
		framegraphsys.UnsignalLastFrameSemaphores(Last_FrameGraph);

		framegraphsys.CreateSubmits_Intermediate(Current_FrameGraph);

		renderer->RG_Status = RenderGraphStatus::HalfConstructed;	//When reconstruction finishes, it is half constructed. 
		//If reconstruction isn't called next frame, framegraph should be duplicated next frame
		break;
	case RenderGraphStatus::HalfConstructed:
		framegraphsys.DuplicateFrameGraph(Current_FrameGraph, Last_FrameGraph);

		renderer->RG_Status = RenderGraphStatus::Valid;
		//RG becomes valid, so we can safely act as Valid now. So don't use "break;" here
	case RenderGraphStatus::Valid:
		//Some previously allocated lists needs to be cleared and RenderGraph branches needs to be refreshed
		framegraphsys.Prepare_forSubmitCreation();

		//Rendergraph consists of 2 similar framegraphs (that references each other), so create submits in the fast way
		framegraphsys.CreateSubmits_Fast(Current_FrameGraph, Last_FrameGraph);
		break;
	default:
		printer(result_tgfx_FAIL, "VulkanRenderer:Execute_RenderGraph() has failed because rendergraph's status isn't supported!");
		return result_tgfx_FAIL;
	}


	framegraphsys.Send_RenderCommands();

	framegraphsys.Send_PresentationCommands();

	//There are lots of data structures to handle 2 frames
	//So we need to shift some datas because the way we access them
	framegraphsys.RenderGraph_DataShifting();
}
void Destroy_RenderGraph() {

}