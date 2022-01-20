#include "Vulkan_Renderer_Core.h"
#include "VK_GPUContentManager.h"
#include "Vulkan_Resource.h"
#include "Vulkan_Core.h"
#include <string>
#include <sstream>
#define LOG VKCORE->TGFXCORE.DebugCallback

//DELETE THESE AFTER REFACTORING ENDS
void WaitForRenderGraphCommandBuffers();







void SetRendererFunctionPointers() {
	VKCORE->TGFXCORE.Renderer.ChangeDrawPass_RTSlotSet = Renderer::ChangeDrawPass_RTSlotSet;
	VKCORE->TGFXCORE.Renderer.CopyBuffer_toBuffer = Renderer::CopyBuffer_toBuffer;
	VKCORE->TGFXCORE.Renderer.CopyBuffer_toImage = Renderer::CopyBuffer_toImage;
	VKCORE->TGFXCORE.Renderer.CopyImage_toImage = Renderer::CopyImage_toImage;
	VKCORE->TGFXCORE.Renderer.Create_ComputePass = Renderer::Create_ComputePass;
	VKCORE->TGFXCORE.Renderer.Create_DrawPass = Renderer::Create_DrawPass;
	VKCORE->TGFXCORE.Renderer.Create_TransferPass = Renderer::Create_TransferPass;
	VKCORE->TGFXCORE.Renderer.Create_WindowPass = Renderer::Create_WindowPass;
	VKCORE->TGFXCORE.Renderer.Destroy_RenderGraph = Renderer::Destroy_RenderGraph;
	VKCORE->TGFXCORE.Renderer.Dispatch_Compute = Renderer::Dispatch_Compute;
	VKCORE->TGFXCORE.Renderer.DrawDirect = Renderer::DrawDirect;
	VKCORE->TGFXCORE.Renderer.Finish_RenderGraphConstruction = Renderer::Finish_RenderGraphConstruction;
	VKCORE->TGFXCORE.Renderer.GetCurrentFrameIndex = Renderer::GetCurrentFrameIndex;
	VKCORE->TGFXCORE.Renderer.ImageBarrier = Renderer::ImageBarrier;
	VKCORE->TGFXCORE.Renderer.Run = Renderer::Run;
	VKCORE->TGFXCORE.Renderer.Start_RenderGraphConstruction = Renderer::Start_RenderGraphConstruction;
	VKCORE->TGFXCORE.Renderer.SwapBuffers = Renderer::SwapBuffers;
}

Renderer::Renderer() {
	SetRendererFunctionPointers();

	//Create Command Pool per QUEUE
	for (unsigned int QUEUEIndex = 0; QUEUEIndex < RENDERGPU->QUEUEFAMS().size(); QUEUEIndex++) {
		if (!(RENDERGPU->QUEUEFAMS()[QUEUEIndex].SupportFlag.is_COMPUTEsupported || RENDERGPU->QUEUEFAMS()[QUEUEIndex].SupportFlag.is_GRAPHICSsupported ||
			RENDERGPU->QUEUEFAMS()[QUEUEIndex].SupportFlag.is_TRANSFERsupported)
			) {
			printer(result_tgfx_SUCCESS, "VulkanRenderer:Command pool creation for a queue has failed because one of the VkQueues doesn't support neither Graphics, Compute or Transfer. So GFX API didn't create a command pool for it!");
			continue;
		}
		for (unsigned char i = 0; i < 2; i++) {
			VkCommandPoolCreateInfo cp_ci_g = {};
			cp_ci_g.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cp_ci_g.queueFamilyIndex = RENDERGPU->QUEUEFAMS()[QUEUEIndex].QueueFamilyIndex;
			cp_ci_g.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			cp_ci_g.pNext = nullptr;

			if (vkCreateCommandPool(RENDERGPU->LOGICALDEVICE(), &cp_ci_g, nullptr, &RENDERGPU->QUEUEFAMS()[QUEUEIndex].CommandPools[i].CPHandle) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "VulkanRenderer: Command pool creation for a queue has failed at vkCreateCommandPool()!");
			}
		}
	}
}
Renderer::~Renderer() {
	Destroy_RenderGraph();
}


unsigned char Renderer::Get_FrameIndex(bool is_LastFrame) {
	if (is_LastFrame) {
		return (VKRENDERER->FrameIndex) ? 0 : 1;
	}
	else {
		return VKRENDERER->FrameIndex;
	}
}


void Fill_ColorVkAttachmentDescription(VkAttachmentDescription& Desc, const VK_COLORRTSLOT* Attachment) {
	Desc = {};
	Desc.format = Find_VkFormat_byTEXTURECHANNELs(Attachment->RT->CHANNELs);
	Desc.samples = VK_SAMPLE_COUNT_1_BIT;
	Desc.flags = 0;
	Desc.loadOp = Find_LoadOp_byGFXLoadOp(Attachment->LOADSTATE);
	Desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	if (Attachment->IS_USED_LATER) {
		Desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
	else {
		Desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}

	Desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	Desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	if (Attachment->RT->CHANNELs == tgfx_texture_channels_D32) {
		if (Attachment->RT_OPERATIONTYPE == tgfx_operationtype_READ_ONLY) {
			Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
		}
		else {
			Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		}
	}
	else if (Attachment->RT->CHANNELs == tgfx_texture_channels_D24S8) {
		if (Attachment->RT_OPERATIONTYPE == tgfx_operationtype_READ_ONLY) {
			Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
		}
		else {
			Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
	}
	else {
	}
	Desc.flags = 0;
}
void Fill_SubpassStructs(VK_IRTSLOTSET* slotset, tgfx_subdrawpass_access WaitedOp, tgfx_subdrawpass_access ContinueOp, unsigned char SubpassIndex, VkSubpassDescription& descs, VkSubpassDependency& dependencies) {
	VkAttachmentReference* COLOR_ATTACHMENTs = new VkAttachmentReference[slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT];
	VkAttachmentReference* DS_Attach = nullptr;

	for (unsigned int i = 0; i < slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT; i++) {
		tgfx_operationtype RT_OPTYPE = slotset->COLOR_OPTYPEs[i];
		VkAttachmentReference Attachment_Reference = {};
		if (RT_OPTYPE == tgfx_operationtype_UNUSED) {
			Attachment_Reference.attachment = VK_ATTACHMENT_UNUSED;
			Attachment_Reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		else {
			Attachment_Reference.attachment = i;
			Attachment_Reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		COLOR_ATTACHMENTs[i] = Attachment_Reference;
	}

	//Depth Stencil Attachment
	if (slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
		VK_DEPTHSTENCILSLOT* depthslot = slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT;
		DS_Attach = new VkAttachmentReference;
		RENDERGPU->ExtensionRelatedDatas.Fill_DepthAttachmentReference(*DS_Attach, slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT,
			depthslot->RT->CHANNELs, slotset->DEPTH_OPTYPE, slotset->STENCIL_OPTYPE);
	}

	descs = {};
	descs.colorAttachmentCount = slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT;
	descs.pColorAttachments = COLOR_ATTACHMENTs;
	descs.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	descs.flags = 0;
	descs.pDepthStencilAttachment = DS_Attach;

	dependencies = {};
	dependencies.dependencyFlags = 0;
	if (SubpassIndex) {
		dependencies.srcSubpass = SubpassIndex - 1;
		dependencies.dstSubpass = SubpassIndex;
		Find_SubpassAccessPattern(WaitedOp, true, dependencies.srcStageMask, dependencies.srcAccessMask);
		Find_SubpassAccessPattern(ContinueOp, true, dependencies.dstStageMask, dependencies.dstAccessMask);
	}
	else {
		dependencies.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies.dstSubpass = 0;
		dependencies.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies.srcAccessMask = 0;
		dependencies.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
}
tgfx_result Renderer::Create_DrawPass(tgfx_subdrawpassdescription_list TGFXSubDrawPasses, tgfx_rtslotset RTSLOTSET_ID,
	tgfx_passwaitdescription_list WAITs, const char* NAME, tgfx_subdrawpass* SubDrawPassIDs, tgfx_drawpass* DPHandle) {
	if (VKRENDERER->RG_Status != RenderGraphStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "GFXRENDERER->CreateDrawPass() has failed because you first need to call Start_RenderGraphCreation()!");
		return tgfx_result_FAIL;
	}
	VK_RTSLOTSET* SLOTSET = (VK_RTSLOTSET*)RTSLOTSET_ID;
	for (unsigned int i = 0; i < SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT; i++) {
		VK_COLORRTSLOT& slot = SLOTSET->PERFRAME_SLOTSETs[0].COLOR_SLOTs[i];
		if (slot.RT_OPERATIONTYPE == tgfx_operationtype_UNUSED) {
			printer(result_tgfx_INVALIDARGUMENT, "Create_DrawPass() has failed because you can't give an unused Color RT Slot to the Draw Pass! You either don't give it, or use it");
			return tgfx_result_INVALIDARGUMENT;
		}
	}
	if (SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
		if (SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->DEPTH_OPTYPE == tgfx_operationtype_UNUSED) {
			printer(result_tgfx_FAIL, "Create_DrawPass() has failed because you can't give an unused Depth RT Slot to the Draw Pass! You either don't give it, or use it");
			return tgfx_result_INVALIDARGUMENT;
		}
	}
	if (!SLOTSET) {
		printer(result_tgfx_FAIL, "Create_DrawPass() has failed because RTSLOTSET_ID is invalid!");
		return tgfx_result_INVALIDARGUMENT;
	}
	VK_SubDrawPassDesc** SubDrawPassDescs = (VK_SubDrawPassDesc**)TGFXSubDrawPasses;
	TGFXLISTCOUNT((&VKCORE->TGFXCORE), SubDrawPassDescs, SubDrawPassesCount);
	for (unsigned int i = 0; i < SubDrawPassesCount; i++) {
		VK_IRTSLOTSET* ISET = SubDrawPassDescs[i]->INHERITEDSLOTSET;
		if (!ISET) {
			printer(result_tgfx_FAIL, "GFXRenderer->Create_DrawPass() has failed because one of the inherited slotsets is nullptr!");
			return tgfx_result_INVALIDARGUMENT;
		}
		if (ISET->BASESLOTSET != SLOTSET) {
			printer(result_tgfx_FAIL, "GFXRenderer->Create_DrawPass() has failed because one of the inherited slotsets isn't inherited from the DrawPass' Base Slotset!");
			return tgfx_result_INVALIDARGUMENT;
		}
	}


	TGFXLISTCOUNT((&VKCORE->TGFXCORE), WAITs, WAITSCOUNT);
	unsigned int ValidWAITsCount = 0;
	for (unsigned int i = 0; i < WAITSCOUNT; i++) {
		if (WAITs[i] == nullptr) {
			continue;
		}
		VK_PassWaitDescription* Wait = (VK_PassWaitDescription*)WAITs[i];
#ifdef VULKAN_DEBUGGING
		if (!Wait->isValid()) {
			printer(result_tgfx_WARNING, "You passed a wait description that isn't nullptr but points to somewhere that's not created by Helper::Create_PassWaitDescription() to the function Create_DrawPass(). You shouldn't do this in release mode!");
			continue;
		}
#endif
		ValidWAITsCount++;
	}

	VK_SubDrawPass* Final_Subpasses = new VK_SubDrawPass[SubDrawPassesCount];
	drawpass_vk* VKDrawPass = new drawpass_vk(NAME, ValidWAITsCount);
	VKDrawPass->SLOTSET = SLOTSET;
	VKDrawPass->Subpass_Count = SubDrawPassesCount;
	VKDrawPass->Subpasses = Final_Subpasses;
	
	for (unsigned int S_i = 0, T_i = 0; S_i < WAITSCOUNT; S_i++) {
		if (WAITs[S_i] == nullptr) {
			continue;
		}

		VK_PassWaitDescription* Wait = (VK_PassWaitDescription*)WAITs[S_i];
#ifdef VULKAN_DEBUGGING
		if (!Wait->isValid()) {
			continue;
		}
#endif
		VKDrawPass->WAITs[T_i] = *Wait;
		T_i++;
	}

	VKDrawPass->RenderRegion.XOffset = 0;
	VKDrawPass->RenderRegion.YOffset = 0;
	if (SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT) {
		VKDrawPass->RenderRegion.WIDTH = SLOTSET->PERFRAME_SLOTSETs[0].COLOR_SLOTs[0].RT->WIDTH;
		VKDrawPass->RenderRegion.HEIGHT = SLOTSET->PERFRAME_SLOTSETs[0].COLOR_SLOTs[0].RT->HEIGHT;
	}
	else {
		VKDrawPass->RenderRegion.WIDTH = SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->RT->WIDTH;
		VKDrawPass->RenderRegion.HEIGHT = SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->RT->HEIGHT;
	}
	VKRENDERER->DrawPasses.push_back(VKDrawPass);



	//SubDrawPass creation
	std::vector<VkSubpassDependency> SubpassDepends(SubDrawPassesCount);
	std::vector<VkSubpassDescription> SubpassDescs(SubDrawPassesCount);
	std::vector<std::vector<VkAttachmentReference>> SubpassAttachments(SubDrawPassesCount);
	SubDrawPassIDs = (tgfx_subdrawpass_list)new VK_SubDrawPass * [SubDrawPassesCount + 1]{NULL};
	for (unsigned int i = 0; i < SubDrawPassesCount; i++) {
		const VK_SubDrawPassDesc* Subpass_gfxdesc = SubDrawPassDescs[i];
		VK_IRTSLOTSET* Subpass_Slotset = Subpass_gfxdesc->INHERITEDSLOTSET;

		Fill_SubpassStructs(Subpass_Slotset, Subpass_gfxdesc->WaitOp, Subpass_gfxdesc->ContinueOp, Subpass_gfxdesc->SubDrawPass_Index, SubpassDescs[i], SubpassDepends[i]);

		Final_Subpasses[i].Binding_Index = Subpass_gfxdesc->SubDrawPass_Index;
		Final_Subpasses[i].SLOTSET = Subpass_Slotset;
		Final_Subpasses[i].DrawPass = VKDrawPass;
		SubDrawPassIDs[i] = (tgfx_subdrawpass)&Final_Subpasses[i];
	}

	//vkRenderPass generation
	{
		std::vector<VkAttachmentDescription> AttachmentDescs;
		//Create Attachment Descriptions
		for (unsigned int i = 0; i < VKDrawPass->SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT; i++) {
			VkAttachmentDescription Attachmentdesc = {};
			Fill_ColorVkAttachmentDescription(Attachmentdesc, &VKDrawPass->SLOTSET->PERFRAME_SLOTSETs[0].COLOR_SLOTs[i]);
			AttachmentDescs.push_back(Attachmentdesc);
		}
		if (VKDrawPass->SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
			VkAttachmentDescription DepthDesc;
			RENDERGPU->ExtensionRelatedDatas.Fill_DepthAttachmentDescription(DepthDesc, VKDrawPass->SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT);
			AttachmentDescs.push_back(DepthDesc);
		}


		//RenderPass Creation
		VkRenderPassCreateInfo RenderPass_CreationInfo = {};
		RenderPass_CreationInfo.flags = 0;
		RenderPass_CreationInfo.pNext = nullptr;
		RenderPass_CreationInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		RenderPass_CreationInfo.subpassCount = SubpassDescs.size();
		RenderPass_CreationInfo.pSubpasses = SubpassDescs.data();
		RenderPass_CreationInfo.attachmentCount = AttachmentDescs.size();
		RenderPass_CreationInfo.pAttachments = AttachmentDescs.data();
		RenderPass_CreationInfo.dependencyCount = SubpassDepends.size();
		RenderPass_CreationInfo.pDependencies = SubpassDepends.data();

		if (vkCreateRenderPass(RENDERGPU->LOGICALDEVICE(), &RenderPass_CreationInfo, nullptr, &VKDrawPass->RenderPassObject) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "VkCreateRenderPass() has failed!");
			delete VKDrawPass;
			return tgfx_result_FAIL;
		}
	}

	*DPHandle = (tgfx_drawpass)VKDrawPass;
	return tgfx_result_SUCCESS;
}



tgfx_result Renderer::Create_TransferPass(tgfx_passwaitdescription_list WaitDescriptions,
	tgfx_transferpass_type TP_TYPE, const char* NAME, tgfx_transferpass* TPHandle) {

	//Count the valid pass wait infos
	TGFXLISTCOUNT((&VKCORE->TGFXCORE), WaitDescriptions, WAITSCOUNT);
	unsigned int ValidWAITsCount = 0;
	for (unsigned int i = 0; i < WAITSCOUNT; i++) {
		if (WaitDescriptions[i] == nullptr) {
			continue;
		}
		VK_PassWaitDescription* Wait = (VK_PassWaitDescription*)WaitDescriptions[i];
#ifdef VULKAN_DEBUGGING
		if (!Wait->isValid()) {
			printer(result_tgfx_WARNING, "You passed a wait description that isn't nullptr but points to somewhere that's not created by Helper::Create_PassWaitDescription() to the function Create_DrawPass(). You shouldn't do this in release mode!");
			continue;
		}
#endif
		ValidWAITsCount++;
	}

	transferpass_vk* TRANSFERPASS = new transferpass_vk(NAME, ValidWAITsCount);
	if (VKRENDERER->RG_Status != RenderGraphStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "You should call Start_RenderGraphConstruction() before RENDERER->Create_TransferPass()!");
		return tgfx_result_FAIL;
	}

	//Fill the pass wait description list
	for (unsigned int S_i = 0, T_i = 0; S_i < WAITSCOUNT; S_i++) {
		if (WaitDescriptions[S_i] == nullptr) {
			continue;
		}

		VK_PassWaitDescription* Wait = (VK_PassWaitDescription*)WaitDescriptions[S_i];
#ifdef VULKAN_DEBUGGING
		if (!Wait->isValid()) {
			continue;
		}
#endif
		TRANSFERPASS->WAITs[T_i] = *Wait;
		T_i++;
	}

	TRANSFERPASS->TYPE = TP_TYPE;
	switch (TP_TYPE) {
	case tgfx_transferpass_type_BARRIER:
		TRANSFERPASS->TransferDatas = new VK_TPBarrierDatas;
		break;
	case tgfx_transferpass_type_COPY:
		TRANSFERPASS->TransferDatas = new VK_TPCopyDatas;
		break;
	default:
		printer(result_tgfx_FAIL, "VulkanRenderer: Create_TransferPass() has failed because this type of TP creation isn't supported!");
		return tgfx_result_NOTCODED;
	}

	VKRENDERER->TransferPasses.push_back(TRANSFERPASS);
	*TPHandle = (tgfx_transferpass)TRANSFERPASS;
	return tgfx_result_SUCCESS;
}
tgfx_result Renderer::Create_ComputePass(tgfx_passwaitdescription_list WaitDescriptions, unsigned int SubComputePassCount, 
	const char* NAME, tgfx_computepass* CPHandle) {
	if (VKRENDERER->RG_Status != RenderGraphStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "You should call Start_RenderGraphConstruction() before RENDERER->Create_ComputePass()!");
		return tgfx_result_FAIL;
	}
	//Count the valid pass wait infos
	TGFXLISTCOUNT((&VKCORE->TGFXCORE), WaitDescriptions, WAITSCOUNT);
	unsigned int ValidWAITsCount = 0;
	for (unsigned int i = 0; i < WAITSCOUNT; i++) {
		if (WaitDescriptions[i] == nullptr) {
			continue;
		}
		VK_PassWaitDescription* Wait = (VK_PassWaitDescription*)WaitDescriptions[i];
#ifdef VULKAN_DEBUGGING
		if (!Wait->isValid()) {
			printer(result_tgfx_WARNING, "You passed a wait description that isn't nullptr but points to somewhere that's not created by Helper::Create_PassWaitDescription() to the function Create_DrawPass(). You shouldn't do this in release mode!");
			continue;
		}
#endif
		ValidWAITsCount++;
	}

	computepass_vk* CP = new computepass_vk(NAME, ValidWAITsCount);
	//Fill the pass wait description list
	for (unsigned int S_i = 0, T_i = 0; S_i < WAITSCOUNT; S_i++) {
		if (WaitDescriptions[S_i] == nullptr) {
			continue;
		}

		VK_PassWaitDescription* Wait = (VK_PassWaitDescription*)WaitDescriptions[S_i];
#ifdef VULKAN_DEBUGGING
		if (!Wait->isValid()) {
			continue;
		}
#endif
		CP->WAITs[T_i] = *Wait;
		T_i++;
	}

	CP->Subpasses.resize(SubComputePassCount);
	VKRENDERER->ComputePasses.push_back(CP);
	*CPHandle = (tgfx_computepass)CP;
	return tgfx_result_SUCCESS;
}
tgfx_result Renderer::Create_WindowPass(tgfx_passwaitdescription_list WaitDescriptions, const char* NAME, tgfx_windowpass* WindowPassHandle) {
	if (VKRENDERER->RG_Status != RenderGraphStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "You should call Start_RenderGraphConstruction() before RENDERER->Create_ComputePass()!");
		return tgfx_result_FAIL;
	}
	//Count the valid pass wait infos
	TGFXLISTCOUNT((&VKCORE->TGFXCORE), WaitDescriptions, WAITSCOUNT);
	unsigned int ValidWAITsCount = 0;
	for (unsigned int i = 0; i < WAITSCOUNT; i++) {
		if (WaitDescriptions[i] == nullptr) {
			continue;
		}
		VK_PassWaitDescription* Wait = (VK_PassWaitDescription*)WaitDescriptions[i];
#ifdef VULKAN_DEBUGGING
		if (!Wait->isValid()) {
			printer(result_tgfx_WARNING, "You passed a wait description that isn't nullptr but points to somewhere that's not created by Helper::Create_PassWaitDescription() to the function Create_DrawPass(). You shouldn't do this in release mode!");
			continue;
		}
#endif
		ValidWAITsCount++;
	}
	windowpass_vk* WP = new windowpass_vk(NAME, ValidWAITsCount);

	//Fill the pass wait description list
	for (unsigned int S_i = 0, T_i = 0; S_i < WAITSCOUNT; S_i++) {
		if (WaitDescriptions[S_i] == nullptr) {
			continue;
		}

		VK_PassWaitDescription* Wait = (VK_PassWaitDescription*)WaitDescriptions[S_i];
#ifdef VULKAN_DEBUGGING
		if (!Wait->isValid()) {
			continue;
		}
#endif
		WP->WAITs[T_i] = *Wait;
		T_i++;
	}
	WP->WindowCalls[0].clear();
	WP->WindowCalls[1].clear();
	WP->WindowCalls[2].clear();

	*WindowPassHandle = (tgfx_windowpass)WP;
	VKRENDERER->WindowPasses.push_back(WP);
	return tgfx_result_SUCCESS;
}

void PrepareForNextFrame() {
	VKRENDERER->FrameIndex = (VKRENDERER->FrameIndex + 1) % 2;


	VK_IMGUI->NewFrame();
}

void Renderer::Run() {
	printer(result_tgfx_SUCCESS, "Before cb waiting()!");

	//Wait for command buffers that are sent to render N-2th frame (N is the current frame)
	//As the name says, this function stops the thread while waiting for render calls (raster-compute-transfer-barrier calls).
	//With this way, we are sure that no command buffer execution will collide so we can change descriptor sets according to that.
	WaitForRenderGraphCommandBuffers();

	//Descriptor Set changes, vkFramebuffer changes etc.
	VKContentManager->Apply_ResourceChanges();

	//Ask to Vulkan about the swapchain's status and give it a semaphore to signal when the swapchain's texture is available to change
	//Penultimate window pass waits in current frame's rendergraph passes waits for the semaphore here
	for (unsigned char WindowIndex = 0; WindowIndex < VKCORE->WINDOWs.size(); WindowIndex++) {
		WINDOW* VKWINDOW = (WINDOW*)(VKCORE->WINDOWs[WindowIndex]);
		
		uint32_t SwapchainImage_Index;
		vkAcquireNextImageKHR(RENDERGPU->LOGICALDEVICE(), VKWINDOW->Window_SwapChain, UINT64_MAX,
			SEMAPHORESYS->GetSemaphore_byID(VKWINDOW->PresentationSemaphores[1]).VKSEMAPHORE(), VK_NULL_HANDLE, &SwapchainImage_Index);
		if (SwapchainImage_Index != VKWINDOW->CurrentFrameSWPCHNIndex) {
			std::stringstream ErrorStream;
			ErrorStream << "Vulkan's reported SwapchainImage_Index: " << SwapchainImage_Index <<
				"\nGFX's stored SwapchainImage_Index: " << unsigned int(VKWINDOW->CurrentFrameSWPCHNIndex) <<
				"\nGFX's SwapchainIndex and Vulkan's SwapchainIndex don't match, there is something missing!";
			printer(result_tgfx_FAIL, ErrorStream.str().c_str());
		}
	}


	//This function is defined in FGAlgorithm.cpp
	Execute_RenderGraph();


	//Current frame has finished, so every call after this call affects to the next frame
	//That means we should make some changes/clean-ups for every system to work fine
	PrepareForNextFrame();
}
void FindBufferOBJ_byBufType(tgfx_buffer Handle, tgfx_buffertype TYPE, VkBuffer& TargetBuffer, VkDeviceSize& TargetOffset) {
	switch (TYPE) {
	case tgfx_buffertype::GLOBAL:
	{
		VK_GlobalBuffer* buf = (VK_GlobalBuffer*)Handle;
		TargetBuffer = RENDERGPU->ALLOCS()[buf->Block.MemAllocIndex].Buffer;
		TargetOffset += buf->Block.Offset;
	}
	break;
	case tgfx_buffertype::VERTEX:
	{
		VK_VertexBuffer* buf = (VK_VertexBuffer*)Handle;
		TargetBuffer = RENDERGPU->ALLOCS()[buf->Block.MemAllocIndex].Buffer;
		TargetOffset += buf->Block.Offset;
	}
	break;
	case tgfx_buffertype::STAGING:
	{
		MemoryBlock* Staging = (MemoryBlock*)Handle;
		TargetBuffer = RENDERGPU->ALLOCS()[Staging->MemAllocIndex].Buffer;
		TargetOffset += Staging->Offset;
	}
	break;
	case tgfx_buffertype::INDEX:
	{
		VK_IndexBuffer* IB = (VK_IndexBuffer*)Handle;
		TargetBuffer = RENDERGPU->ALLOCS()[IB->Block.MemAllocIndex].Buffer;
		TargetOffset += IB->Block.Offset;
	}
	break;
	default:
		printer(result_tgfx_FAIL, "FindBufferOBJ_byBufType() doesn't support this type of buffer!");
	}
}
void Renderer::DrawDirect(tgfx_buffer VertexBuffer_ID, tgfx_buffer IndexBuffer_ID, unsigned int Count, unsigned int VertexOffset,
	unsigned int FirstIndex, unsigned int InstanceCount, unsigned int FirstInstance, tgfx_rasterpipelineinstance MaterialInstance_ID, tgfx_subdrawpass SubDrawPass_ID) {
	VK_SubDrawPass* SP = (VK_SubDrawPass*)SubDrawPass_ID;
	if (IndexBuffer_ID) {
		VK_IndexedDrawCall call;
		if (VertexBuffer_ID) {
			FindBufferOBJ_byBufType(VertexBuffer_ID, tgfx_buffertype::VERTEX, call.VBuffer, call.VBOffset);
		}
		else {
			call.VBuffer = VK_NULL_HANDLE;
		}
		FindBufferOBJ_byBufType(IndexBuffer_ID, tgfx_buffertype::INDEX, call.IBuffer, call.IBOffset);
		call.IType = ((VK_IndexBuffer*)IndexBuffer_ID)->DATATYPE;
		if (Count) {
			call.IndexCount = Count;
		}
		else {
			call.IndexCount = ((VK_IndexBuffer*)IndexBuffer_ID)->IndexCount;
		}
		call.FirstIndex = FirstIndex;
		call.VOffset = VertexOffset;
		VK_PipelineInstance* PI = (VK_PipelineInstance*)MaterialInstance_ID;
		call.MatTypeObj = PI->PROGRAM->PipelineObject;
		call.MatTypeLayout = PI->PROGRAM->PipelineLayout;
		call.GeneralSet = &PI->PROGRAM->General_DescSet.Set;
		call.PerInstanceSet = &PI->DescSet.Set;
		call.FirstInstance = FirstInstance;
		call.InstanceCount = InstanceCount;
		SP->IndexedDrawCalls.push_back(tapi_GetThisThreadIndex(JobSys), call);
	}
	else {
		VK_NonIndexedDrawCall call;
		if (VertexBuffer_ID) {
			FindBufferOBJ_byBufType(VertexBuffer_ID, tgfx_buffertype::VERTEX, call.VBuffer, call.VOffset);
		}
		else {
			call.VBuffer = VK_NULL_HANDLE;
		}
		if (Count) {
			call.VertexCount = Count;
		}
		else {
			call.VertexCount = static_cast<VkDeviceSize>(((VK_VertexBuffer*)VertexBuffer_ID)->VERTEX_COUNT);
		}
		call.FirstVertex = VertexOffset;
		call.FirstInstance = FirstInstance;
		call.InstanceCount = InstanceCount;
		VK_PipelineInstance* PI = (VK_PipelineInstance*)MaterialInstance_ID;
		call.MatTypeObj = PI->PROGRAM->PipelineObject;
		call.MatTypeLayout = PI->PROGRAM->PipelineLayout;
		call.GeneralSet = &PI->PROGRAM->General_DescSet.Set;
		call.PerInstanceSet = &PI->DescSet.Set;
		SP->NonIndexedDrawCalls.push_back(tapi_GetThisThreadIndex(JobSys), call);
	}
}
void Renderer::SwapBuffers(tgfx_window WindowHandle, tgfx_windowpass WindowPassHandle) {
	windowpass_vk* WP = (windowpass_vk*)WindowPassHandle;
	WINDOW* Window = (WINDOW*)WindowHandle;
	VK_WindowCall WC;
	WC.Window = Window;
	WP->WindowCalls[2].push_back(WC);
}
void Renderer::CopyBuffer_toBuffer(tgfx_transferpass TransferPassHandle, tgfx_buffer SourceBuffer_Handle, tgfx_buffertype SourceBufferTYPE,
	tgfx_buffer TargetBuffer_Handle, tgfx_buffertype TargetBufferTYPE, unsigned int SourceBuffer_Offset, unsigned int TargetBuffer_Offset, unsigned int Size) {
	VkBuffer SourceBuffer, DistanceBuffer;
	VkDeviceSize SourceOffset = static_cast<VkDeviceSize>(SourceBuffer_Offset), DistanceOffset = static_cast<VkDeviceSize>(TargetBuffer_Offset);
	FindBufferOBJ_byBufType(SourceBuffer_Handle, SourceBufferTYPE, SourceBuffer, SourceOffset);
	FindBufferOBJ_byBufType(TargetBuffer_Handle, TargetBufferTYPE, DistanceBuffer, DistanceOffset);
	VkBufferCopy Copy_i = {};
	Copy_i.dstOffset = DistanceOffset;
	Copy_i.srcOffset = SourceOffset;
	Copy_i.size = static_cast<VkDeviceSize>(Size);
	VK_TPCopyDatas* DATAs = (VK_TPCopyDatas*)((transferpass_vk*)TransferPassHandle)->TransferDatas;
	VK_BUFtoBUFinfo finalinfo;
	finalinfo.DistanceBuffer = DistanceBuffer;
	finalinfo.SourceBuffer = SourceBuffer;
	finalinfo.info = Copy_i;
	DATAs->BUFBUFCopies.push_back(tapi_GetThisThreadIndex(JobSys), finalinfo);
}
unsigned int Find_TextureLayer_fromGFXtgfx_cubeface(tgfx_cubeface cubeface) {
	switch (cubeface)
	{
	case tgfx_cubeface::FRONT:
		return 0;
	case tgfx_cubeface::BACK:
		return 1;
	case tgfx_cubeface::LEFT:
		return 2;
	case tgfx_cubeface::RIGHT:
		return 3;
	case tgfx_cubeface::TOP:
		return 4;
	case tgfx_cubeface::BOTTOM:
		return 5;
	default:
		printer(result_tgfx_FAIL, "Find_TextureLayer_fromGFXtgfx_cubeface() in Vulkan backend doesn't support this type of CubeFace!");
	}
}
void Renderer::CopyBuffer_toImage(tgfx_transferpass TransferPassHandle, tgfx_buffer SourceBuffer_Handle, tgfx_texture TextureHandle, 
	unsigned int SourceBuffer_offset, tgfx_BoxRegion TargetTextureRegion, tgfx_buffertype SourceBufferTYPE, unsigned int TargetMipLevel,
	tgfx_cubeface TargetCubeMapFace) {
	VK_Texture* TEXTURE = (VK_Texture*)TextureHandle;
	VkDeviceSize finaloffset = static_cast<VkDeviceSize>(SourceBuffer_offset);
	VK_BUFtoIMinfo x;
	x.BufferImageCopy.bufferImageHeight = 0;
	x.BufferImageCopy.bufferRowLength = 0;
	x.BufferImageCopy.imageExtent.depth = 1;
	if (TargetTextureRegion.HEIGHT) {
		x.BufferImageCopy.imageExtent.height = TargetTextureRegion.HEIGHT;
	}
	else {
		x.BufferImageCopy.imageExtent.height = std::floor(TEXTURE->HEIGHT / std::pow(2, TargetMipLevel));
	}
	if (TargetTextureRegion.WIDTH) {
		x.BufferImageCopy.imageExtent.width = TargetTextureRegion.WIDTH;
	}
	else {
		x.BufferImageCopy.imageExtent.width = std::floor(TEXTURE->WIDTH / std::pow(2, TargetMipLevel));
	}
	x.BufferImageCopy.imageOffset.x = TargetTextureRegion.XOffset;
	x.BufferImageCopy.imageOffset.y = TargetTextureRegion.YOffset;
	x.BufferImageCopy.imageOffset.z = 0;
	if (TEXTURE->CHANNELs == tgfx_texture_channels_D24S8) {
		x.BufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else if (TEXTURE->CHANNELs == tgfx_texture_channels_D32) {
		x.BufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else {
		x.BufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	if (TEXTURE->DIMENSION == tgfx_texture_dimensions::TEXTURE_CUBE) {
		x.BufferImageCopy.imageSubresource.baseArrayLayer = Find_TextureLayer_fromGFXtgfx_cubeface(TargetCubeMapFace);
	}
	else {
		x.BufferImageCopy.imageSubresource.baseArrayLayer = 0;
	}
	x.BufferImageCopy.imageSubresource.layerCount = 1;
	x.BufferImageCopy.imageSubresource.mipLevel = TargetMipLevel;
	x.TargetImage = TEXTURE->Image;
	FindBufferOBJ_byBufType(SourceBuffer_Handle, SourceBufferTYPE, x.SourceBuffer, finaloffset);
	x.BufferImageCopy.bufferOffset = finaloffset;
	transferpass_vk* TP = (transferpass_vk*)TransferPassHandle;
	if (TP->TYPE == tgfx_transferpass_type_BARRIER) {
		printer(result_tgfx_FAIL, "You gave an barrier TP handle to CopyBuffer_toImage(), this is wrong!");
		return;
	}
	VK_TPCopyDatas* DATAs = (VK_TPCopyDatas*)TP->TransferDatas;
	DATAs->BUFIMCopies.push_back(tapi_GetThisThreadIndex(JobSys), x);
}
void Renderer::CopyImage_toImage(tgfx_transferpass TransferPassHandle, tgfx_texture SourceTextureHandle, tgfx_texture TargetTextureHandle,
	tgfx_uvec3 SourceTextureOffset, tgfx_uvec3 CopySize, tgfx_uvec3 TargetTextureOffset, unsigned int SourceMipLevel, unsigned int TargetMipLevel,
	tgfx_cubeface SourceCubeMapFace, tgfx_cubeface TargetCubeMapFace){
	VK_Texture* SourceIm = (VK_Texture*)SourceTextureHandle;
	VK_Texture* TargetIm = (VK_Texture*)TargetTextureHandle;
	VK_IMtoIMinfo x;

	VkImageCopy copy_i = {};
	copy_i.dstOffset.x = TargetTextureOffset.x;
	copy_i.dstOffset.y = TargetTextureOffset.y;
	copy_i.dstOffset.z = TargetTextureOffset.z;
	copy_i.srcOffset.x = SourceTextureOffset.x;
	copy_i.srcOffset.y = SourceTextureOffset.y;
	copy_i.srcOffset.z = SourceTextureOffset.z;
	if (SourceIm->CHANNELs == tgfx_texture_channels_D32) {
		copy_i.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		copy_i.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if (SourceIm->CHANNELs == tgfx_texture_channels_D24S8) {
		copy_i.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		copy_i.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else {
		copy_i.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy_i.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	if (TargetIm->DIMENSION == tgfx_texture_dimensions::TEXTURE_CUBE) {
		copy_i.dstSubresource.baseArrayLayer = Find_TextureLayer_fromGFXtgfx_cubeface(TargetCubeMapFace);
	}
	else {
		copy_i.dstSubresource.baseArrayLayer = 0;
	}
	if (SourceIm->DIMENSION == tgfx_texture_dimensions::TEXTURE_CUBE) {
		copy_i.srcSubresource.baseArrayLayer = Find_TextureLayer_fromGFXtgfx_cubeface(SourceCubeMapFace);
	}
	else {
		copy_i.srcSubresource.baseArrayLayer = 0;
	}
	copy_i.dstSubresource.layerCount = 1; copy_i.srcSubresource.layerCount = 1;
	copy_i.dstSubresource.mipLevel = TargetMipLevel; copy_i.srcSubresource.mipLevel = SourceMipLevel;

	copy_i.extent.width = CopySize.x;
	copy_i.extent.height = CopySize.y;
	copy_i.extent.depth = CopySize.z;

	x.info = copy_i;
	x.SourceTexture = SourceIm->Image;
	x.TargetTexture = TargetIm->Image;

	VK_TPCopyDatas* DATAs = (VK_TPCopyDatas*)((transferpass_vk*)TransferPassHandle)->TransferDatas;
	DATAs->IMIMCopies.push_back(tapi_GetThisThreadIndex(JobSys), x);
}

void Renderer::ImageBarrier(tgfx_transferpass BarrierTPHandle, tgfx_texture TextureHandle, tgfx_imageaccess LAST_ACCESS, 
	tgfx_imageaccess NEXT_ACCESS, unsigned int TargetMipLevel, tgfx_cubeface TargetCubeMapFace) {
	VK_Texture* Texture = (VK_Texture*)TextureHandle;
	transferpass_vk* TP = (transferpass_vk*)BarrierTPHandle;
	if (TP->TYPE != tgfx_transferpass_type_BARRIER) {
		printer(result_tgfx_FAIL, "You should give the handle of a Barrier Transfer Pass! Given Transfer Pass' type isn't Barrier.");
		return;
	}
	VK_TPBarrierDatas* TPDatas = (VK_TPBarrierDatas*)TP->TransferDatas;


	VK_ImBarrierInfo im_bi;
	im_bi.Barrier.image = Texture->Image;
	Find_AccessPattern_byIMAGEACCESS(LAST_ACCESS, im_bi.Barrier.srcAccessMask, im_bi.Barrier.oldLayout);
	Find_AccessPattern_byIMAGEACCESS(NEXT_ACCESS, im_bi.Barrier.dstAccessMask, im_bi.Barrier.newLayout);
	if (Texture->CHANNELs == tgfx_texture_channels_D24S8) {
		im_bi.Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else if (Texture->CHANNELs == tgfx_texture_channels_D32) {
		im_bi.Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else {
		im_bi.Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	if (Texture->DIMENSION == tgfx_texture_dimensions::TEXTURE_CUBE) {
		im_bi.Barrier.subresourceRange.baseArrayLayer = Find_TextureLayer_fromGFXtgfx_cubeface(TargetCubeMapFace);
	}
	else {
		im_bi.Barrier.subresourceRange.baseArrayLayer = 0;
	}
	im_bi.Barrier.subresourceRange.layerCount = 1;
	im_bi.Barrier.subresourceRange.levelCount = 1;
	if (TargetMipLevel == UINT32_MAX) {
		im_bi.Barrier.subresourceRange.baseMipLevel = 0;
		im_bi.Barrier.subresourceRange.levelCount = static_cast<uint32_t>(Texture->MIPCOUNT);
	}
	else {
		im_bi.Barrier.subresourceRange.baseMipLevel = TargetMipLevel;
		im_bi.Barrier.subresourceRange.levelCount = 1;
	}
	im_bi.Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	im_bi.Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	im_bi.Barrier.pNext = nullptr;
	im_bi.Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;


	TPDatas->TextureBarriers.push_back(tapi_GetThisThreadIndex(JobSys), im_bi);
}
void Renderer::Dispatch_Compute(tgfx_computepass ComputePassHandle, tgfx_computeshaderinstance ComputeInstanceHandle, unsigned int SubComputePassIndex, tgfx_uvec3 DispatchSize) {
	computepass_vk* CP = (computepass_vk*)ComputePassHandle;


	VK_SubComputePass& SP = CP->Subpasses[SubComputePassIndex];
	VK_DispatchCall dc;
	dc.DispatchSize = glm::vec3(DispatchSize.x,DispatchSize.y,DispatchSize.z);
	VK_ComputeInstance* ci = (VK_ComputeInstance*)ComputeInstanceHandle;
	dc.GeneralSet = &ci->PROGRAM->General_DescSet.Set;
	dc.InstanceSet = &ci->DescSet.Set;
	dc.Layout = ci->PROGRAM->PipelineLayout;
	dc.Pipeline = ci->PROGRAM->PipelineObject;
	SP.Dispatches.push_back(tapi_GetThisThreadIndex(JobSys), dc);
}


void Renderer::ChangeDrawPass_RTSlotSet(tgfx_drawpass DrawPassHandle, tgfx_rtslotset RTSlotSetHandle) {
	drawpass_vk* DP = (drawpass_vk*)DrawPassHandle;
	VK_RTSLOTSET* slotset = (VK_RTSLOTSET*)RTSlotSetHandle;

	//Check if slotsets are compatible
	if (slotset->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT == DP->SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT) {
		if ((slotset->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT && !DP->SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) ||
			(!slotset->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT && DP->SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT)) {
			printer(result_tgfx_FAIL, "ChangeDrawPass_RTSlotSet() has failed because slotsets are not compatible in depth slots");
			return;
		}
	}
	else {
		printer(result_tgfx_FAIL, "ChangeDrawPass_RTSlotSet() has failed because slotsets are not compatible in color slots count!");
		return;
	}
	for (unsigned char SlotSetIndex = 0; SlotSetIndex < 2; SlotSetIndex++) {
		for (unsigned int ColorSlotIndex = 0; ColorSlotIndex < slotset->PERFRAME_SLOTSETs[SlotSetIndex].COLORSLOTs_COUNT; ColorSlotIndex++) {
			VK_COLORRTSLOT& targetslot = slotset->PERFRAME_SLOTSETs[SlotSetIndex].COLOR_SLOTs[ColorSlotIndex];
			VK_COLORRTSLOT& referenceslot = DP->SLOTSET->PERFRAME_SLOTSETs[SlotSetIndex].COLOR_SLOTs[ColorSlotIndex];
			if (targetslot.RT->CHANNELs != referenceslot.RT->CHANNELs ||
				targetslot.IS_USED_LATER != referenceslot.IS_USED_LATER ||
				targetslot.LOADSTATE != referenceslot.LOADSTATE ||
				targetslot.RT_OPERATIONTYPE != referenceslot.RT_OPERATIONTYPE ||
				targetslot.CLEAR_COLOR != referenceslot.CLEAR_COLOR) {
				printer(result_tgfx_FAIL, "ChangeDrawPass_RTSlotSet() has failed because one of the color slots is incompatible!");
				return;
			}
		}
	}
	if (slotset->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
		VK_DEPTHSTENCILSLOT* targetslot = slotset->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT;
		VK_DEPTHSTENCILSLOT* referenceslot = DP->SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT;
		if (targetslot->RT->CHANNELs != referenceslot->RT->CHANNELs ||
			targetslot->IS_USED_LATER != referenceslot->IS_USED_LATER ||
			targetslot->DEPTH_LOAD != referenceslot->DEPTH_LOAD ||
			targetslot->DEPTH_OPTYPE != referenceslot->DEPTH_OPTYPE ||
			targetslot->STENCIL_LOAD != referenceslot->STENCIL_LOAD ||
			targetslot->STENCIL_OPTYPE != referenceslot->STENCIL_OPTYPE ||
			targetslot->CLEAR_COLOR != referenceslot->CLEAR_COLOR) {
			printer(result_tgfx_FAIL, "ChangeDrawPass_RTSlotSet() has failed because depth slot is incompatible!");
			return;
		}
	}

	unsigned char raceinfo = DP->SlotSetChanged.load();
	if (raceinfo == 3) {
		printer(result_tgfx_WARNING, "You already changed the slotset this frame, second time isn't allowed!");
		return;
	}
	else if (raceinfo > 3) {
		printer(result_tgfx_FAIL, "There is an issue to fix in vulkan backend!");
		return;
	}
	else if (!DP->SlotSetChanged.compare_exchange_strong(raceinfo, 3)) {
		printer(result_tgfx_WARNING, "You are changing the draw pass' RTSlotSet concurrently between threads, only one of them succeeds!");
		return;
	}
	else {
		DP->RenderRegion.WIDTH = slotset->FB_ci->width;
		DP->RenderRegion.HEIGHT = slotset->FB_ci->height;
		DP->SLOTSET = slotset;
	}
}


unsigned char Renderer::GetCurrentFrameIndex() {
	return VKRENDERER->FrameIndex;
}