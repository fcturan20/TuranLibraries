#include "renderer.h"
#include "rendergraph.h"
#include "gpucontentmanager.h"


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
extern bool Check_WaitHandles();
extern void Create_FrameGraphs();
extern void Create_VkDataofRGBranches(const framegraph_vk& FrameGraph);
extern void WaitForLastFrame(framegraph_vk& Last_FrameGraph);
extern void UnsignalLastFrameSemaphores(framegraph_vk& Last_FrameGraph);
extern void CreateSubmits_Intermediate(framegraph_vk& Current_FrameGraph);
extern void DuplicateFrameGraph(framegraph_vk& TargetFrameGraph, const framegraph_vk& SourceFrameGraph);
extern void Prepare_forSubmitCreation();
extern inline void CreateSubmits_Fast(framegraph_vk& Current_FrameGraph, framegraph_vk& LastFrameGraph);
extern void Send_RenderCommands();
extern void Send_PresentationCommands();
extern void RenderGraph_DataShifting();


extern unsigned char GetCurrentFrameIndex();
extern void Start_RenderGraphConstruction() {
	//Apply resource changes made by user
	contentmanager->Resource_Finalizations();
	/*
	if (renderer->RG_Status == RenderGraphStatus::FinishConstructionCalled || VKRENDERER->RG_Status == RenderGraphStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "GFXRENDERER->Start_RenderGraphCreation() has failed because you have wrong function call order!"); return;
	}
	VKRENDERER->RG_Status = RenderGraphStatus::StartedConstruction;*/
}
extern unsigned char Finish_RenderGraphConstruction(subdrawpass_tgfx_handle IMGUI_Subpass) {
	return 0;
}
extern void Destroy_RenderGraph() {

}