#include "renderer.h"
#include "predefinitions_vk.h"
#include <tgfx_structs.h>
#include <tgfx_core.h>
#include <tgfx_renderer.h>
#include "resource.h"
#include "core.h"
#include "extension.h"
#include "gpucontentmanager.h"
#include "synchronization_sys.h"
#include "queue.h"
#include <sstream>
#include <algorithm>
#include <utility>
#include "imgui_vktgfx.h"
#include "memory.h"
#include <functional>


typedef struct renderer_private{
	VkDescriptorPool IMGUIPOOL = VK_NULL_HANDLE;
} renderer_private;
static renderer_private* hidden = nullptr;

			//RENDERGRAPH OPERATIONS

extern void WaitForRenderGraphCommandBuffers();
extern void Start_RenderGraphConstruction();
extern unsigned char Finish_RenderGraphConstruction(subdrawpass_tgfx_handle IMGUI_Subpass);
//This function is defined in the FGAlgorithm.cpp
extern void Destroy_RenderGraph();
//RenderGraph has draw-compute-transfer calls to execute and display calls to display on windows
//So this function handles everything related to these calls
//If returns false, this means nothing is rendered this frame
//Implemented in rendergraph_main.cpp
extern result_tgfx Execute_RenderGraph();
struct renderer_funcs {

	static void Fill_ColorVkAttachmentDescription(VkAttachmentDescription& Desc, const colorslot_vk* Attachment) {
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
		if (Attachment->RT->CHANNELs == texture_channels_tgfx_D32) {
			if (Attachment->RT_OPERATIONTYPE == operationtype_tgfx_READ_ONLY) {
				Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
			}
			else {
				Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			}
		}
		else if (Attachment->RT->CHANNELs == texture_channels_tgfx_D24S8) {
			if (Attachment->RT_OPERATIONTYPE == operationtype_tgfx_READ_ONLY) {
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
	static void Fill_SubpassStructs(irtslotset_vk* slotset, subdrawpassaccess_tgfx WaitedOp, subdrawpassaccess_tgfx ContinueOp, unsigned char SubpassIndex, VkSubpassDescription& descs, VkSubpassDependency& dependencies) {
		VkAttachmentReference* COLOR_ATTACHMENTs = new VkAttachmentReference[slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT];
		VkAttachmentReference* DS_Attach = nullptr;

		for (unsigned int i = 0; i < slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT; i++) {
			operationtype_tgfx RT_OPTYPE = slotset->COLOR_OPTYPEs[i];
			VkAttachmentReference Attachment_Reference = {};
			if (RT_OPTYPE == operationtype_tgfx_UNUSED) {
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
			depthstencilslot_vk* depthslot = slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT;
			DS_Attach = new VkAttachmentReference;
			rendergpu->EXTMANAGER()->Fill_DepthAttachmentReference(*DS_Attach, slotset->BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT,
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

	static result_tgfx Create_DrawPass(subdrawpassdescription_tgfx_listhandle SubDrawPasses, rtslotset_tgfx_handle RTSLOTSET_ID, passwaitdescription_tgfx_listhandle DrawPassWAITs,
		const char* NAME, subdrawpass_tgfx_listhandle* SubDrawPassHandles, drawpass_tgfx_handle* DPHandle) {
		if (renderer->RGSTATUS() != RenderGraphStatus::StartedConstruction) {
			printer(result_tgfx_FAIL, "GFXRENDERER->CreateDrawPass() has failed because you first need to call Start_RenderGraphCreation()!");
			return result_tgfx_FAIL;
		}
		rtslotset_vk* SLOTSET = (rtslotset_vk*)RTSLOTSET_ID;
		for (unsigned int i = 0; i < SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT; i++) {
			colorslot_vk& slot = SLOTSET->PERFRAME_SLOTSETs[0].COLOR_SLOTs[i];
			if (slot.RT_OPERATIONTYPE == operationtype_tgfx_UNUSED) {
				printer(result_tgfx_INVALIDARGUMENT, "Create_DrawPass() has failed because you can't give an unused Color RT Slot to the Draw Pass! You either don't give it, or use it");
				return result_tgfx_INVALIDARGUMENT;
			}
		}
		if (SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
			if (SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->DEPTH_OPTYPE == operationtype_tgfx_UNUSED) {
				printer(result_tgfx_FAIL, "Create_DrawPass() has failed because you can't give an unused Depth RT Slot to the Draw Pass! You either don't give it, or use it");
				return result_tgfx_INVALIDARGUMENT;
			}
		}
		if (!SLOTSET) {
			printer(result_tgfx_FAIL, "Create_DrawPass() has failed because RTSLOTSET_ID is invalid!");
			return result_tgfx_INVALIDARGUMENT;
		}
		subdrawpassdesc_vk** SubDrawPassDescs = (subdrawpassdesc_vk**)SubDrawPasses;
		TGFXLISTCOUNT(core_tgfx_main, SubDrawPassDescs, SubDrawPassesCount);
		for (unsigned int i = 0; i < SubDrawPassesCount; i++) {
			irtslotset_vk* ISET = SubDrawPassDescs[i]->INHERITEDSLOTSET;
			if (!ISET) {
				printer(result_tgfx_FAIL, "GFXRenderer->Create_DrawPass() has failed because one of the inherited slotsets is nullptr!");
				return result_tgfx_INVALIDARGUMENT;
			}
			if (ISET->BASESLOTSET != SLOTSET) {
				printer(result_tgfx_FAIL, "GFXRenderer->Create_DrawPass() has failed because one of the inherited slotsets isn't inherited from the DrawPass' Base Slotset!");
				return result_tgfx_INVALIDARGUMENT;
			}
		}


		TGFXLISTCOUNT(core_tgfx_main, DrawPassWAITs, WAITSCOUNT);
		unsigned int ValidWAITsCount = 0;
		for (unsigned int i = 0; i < WAITSCOUNT; i++) {
			if (DrawPassWAITs[i] == nullptr) {
				continue;
			}
			VK_Pass::WaitDescription* Wait = (VK_Pass::WaitDescription*)DrawPassWAITs[i];
			ValidWAITsCount++;
		}

		subdrawpass_vk* Final_Subpasses = new subdrawpass_vk[SubDrawPassesCount];
		drawpass_vk* VKDrawPass = new drawpass_vk(NAME, ValidWAITsCount);
		VKDrawPass->SLOTSET = SLOTSET;
		VKDrawPass->Subpass_Count = SubDrawPassesCount;
		VKDrawPass->Subpasses = Final_Subpasses;
		if ((VK_Pass*)VKDrawPass != & VKDrawPass->base_data) {
			printer(result_tgfx_FAIL, "DrawPass' first variable should be base_data!");
		}

		for (unsigned int S_i = 0, T_i = 0; S_i < WAITSCOUNT; S_i++) {
			if (DrawPassWAITs[S_i] == nullptr) {
				continue;
			}

			VK_Pass::WaitDescription* Wait = (VK_Pass::WaitDescription*)DrawPassWAITs[S_i];
			VKDrawPass->base_data.WAITs[T_i] = *Wait;
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
		renderer->DrawPasses.push_back(VKDrawPass);



		//SubDrawPass creation
		std::vector<VkSubpassDependency> SubpassDepends(SubDrawPassesCount);
		std::vector<VkSubpassDescription> SubpassDescs(SubDrawPassesCount);
		std::vector<std::vector<VkAttachmentReference>> SubpassAttachments(SubDrawPassesCount);
		(* SubDrawPassHandles) = (subdrawpass_tgfx_listhandle)new subdrawpass_vk *[SubDrawPassesCount + 1]{NULL};
		for (unsigned int i = 0; i < SubDrawPassesCount; i++) {
			const subdrawpassdesc_vk* Subpass_gfxdesc = SubDrawPassDescs[i];
			irtslotset_vk* Subpass_Slotset = Subpass_gfxdesc->INHERITEDSLOTSET;

			Fill_SubpassStructs(Subpass_Slotset, Subpass_gfxdesc->WaitOp, Subpass_gfxdesc->ContinueOp, i, SubpassDescs[i], SubpassDepends[i]);

			Final_Subpasses[i].Binding_Index = i;
			Final_Subpasses[i].SLOTSET = Subpass_Slotset;
			Final_Subpasses[i].DrawPass = VKDrawPass;
			(*SubDrawPassHandles)[i] = (subdrawpass_tgfx_handle)&Final_Subpasses[i];
		}
		(*SubDrawPassHandles)[SubDrawPassesCount] = (subdrawpass_tgfx_handle)core_tgfx_main->INVALIDHANDLE;

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
				rendergpu->EXTMANAGER()->Fill_DepthAttachmentDescription(DepthDesc, VKDrawPass->SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT);
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
			VKDrawPass->SlotSetChanged.store(3);

			if (vkCreateRenderPass(rendergpu->LOGICALDEVICE(), &RenderPass_CreationInfo, nullptr, &VKDrawPass->RenderPassObject) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "VkCreateRenderPass() has failed!");
				delete VKDrawPass;
				return result_tgfx_FAIL;
			}
		}

		*DPHandle = (drawpass_tgfx_handle)VKDrawPass;
		return result_tgfx_SUCCESS;
	}
	static result_tgfx Create_TransferPass(passwaitdescription_tgfx_listhandle WaitDescriptions, transferpasstype_tgfx TP_TYPE, const char* NAME,
		transferpass_tgfx_handle* TPHandle) {
		//Count the valid pass wait infos
		TGFXLISTCOUNT(core_tgfx_main, WaitDescriptions, WAITSCOUNT);
		unsigned int ValidWAITsCount = 0;
		for (unsigned int i = 0; i < WAITSCOUNT; i++) {
			if (WaitDescriptions[i] == nullptr) {
				continue;
			}
			VK_Pass::WaitDescription* Wait = (VK_Pass::WaitDescription*)WaitDescriptions[i];
			ValidWAITsCount++;
		}

		transferpass_vk* TRANSFERPASS = new transferpass_vk(NAME, ValidWAITsCount);
		if (renderer->RG_Status != RenderGraphStatus::StartedConstruction) {
			printer(result_tgfx_FAIL, "You should call Start_RenderGraphConstruction() before RENDERER->Create_TransferPass()!");
			return result_tgfx_FAIL;
		}

		//Fill the pass wait description list
		for (unsigned int S_i = 0, T_i = 0; S_i < WAITSCOUNT; S_i++) {
			if (WaitDescriptions[S_i] == nullptr) {
				continue;
			}

			VK_Pass::WaitDescription* Wait = (VK_Pass::WaitDescription*)WaitDescriptions[S_i];
			TRANSFERPASS->base_data.WAITs[T_i] = *Wait;
			T_i++;
		}

		TRANSFERPASS->TYPE = TP_TYPE;
		switch (TP_TYPE) {
		case transferpasstype_tgfx_BARRIER:
			TRANSFERPASS->TransferDatas = new VK_TPBarrierDatas;
			break;
		case transferpasstype_tgfx_COPY:
			TRANSFERPASS->TransferDatas = new VK_TPCopyDatas;
			break;
		default:
			printer(result_tgfx_FAIL, "VulkanRenderer: Create_TransferPass() has failed because this type of TP creation isn't supported!");
			return result_tgfx_NOTCODED;
		}

		renderer->TransferPasses.push_back(TRANSFERPASS);
		*TPHandle = (transferpass_tgfx_handle)TRANSFERPASS;
		return result_tgfx_SUCCESS;
	}
	static result_tgfx Create_ComputePass(passwaitdescription_tgfx_listhandle WaitDescriptions, unsigned int SubComputePassCount, const char* NAME,
		computepass_tgfx_handle* CPHandle) {
		if (renderer->RG_Status != RenderGraphStatus::StartedConstruction) {
			printer(result_tgfx_FAIL, "You should call Start_RenderGraphConstruction() before RENDERER->Create_ComputePass()!");
			return result_tgfx_FAIL;
		}
		//Count the valid pass wait infos
		TGFXLISTCOUNT(core_tgfx_main, WaitDescriptions, WAITSCOUNT);
		unsigned int ValidWAITsCount = 0;
		for (unsigned int i = 0; i < WAITSCOUNT; i++) {
			if (WaitDescriptions[i] == nullptr) {
				continue;
			}
			VK_Pass::WaitDescription* Wait = (VK_Pass::WaitDescription*)WaitDescriptions[i];
			ValidWAITsCount++;
		}

		computepass_vk* CP = new computepass_vk(NAME, ValidWAITsCount);
		//Fill the pass wait description list
		for (unsigned int S_i = 0, T_i = 0; S_i < WAITSCOUNT; S_i++) {
			if (WaitDescriptions[S_i] == nullptr) {
				continue;
			}

			VK_Pass::WaitDescription* Wait = (VK_Pass::WaitDescription*)WaitDescriptions[S_i];
			CP->base_data.WAITs[T_i] = *Wait;
			T_i++;
		}

		CP->Subpasses.resize(SubComputePassCount);
		renderer->ComputePasses.push_back(CP);
		*CPHandle = (computepass_tgfx_handle)CP;
		return result_tgfx_SUCCESS;
	}
	static result_tgfx Create_WindowPass(passwaitdescription_tgfx_listhandle WaitDescriptions, const char* NAME, windowpass_tgfx_handle* WindowPassHandle) {
		if (renderer->RG_Status != RenderGraphStatus::StartedConstruction) {
			printer(result_tgfx_FAIL, "You should call Start_RenderGraphConstruction() before RENDERER->Create_ComputePass()!");
			return result_tgfx_FAIL;
		}
		//Count the valid pass wait infos
		TGFXLISTCOUNT(core_tgfx_main, WaitDescriptions, WAITSCOUNT);
		unsigned int ValidWAITsCount = 0;
		for (unsigned int i = 0; i < WAITSCOUNT; i++) {
			if (WaitDescriptions[i] == nullptr) {
				continue;
			}
			VK_Pass::WaitDescription* Wait = (VK_Pass::WaitDescription*)WaitDescriptions[i];
			ValidWAITsCount++;
		}
		windowpass_vk* WP = new windowpass_vk(NAME, ValidWAITsCount);

		//Fill the pass wait description list
		for (unsigned int S_i = 0, T_i = 0; S_i < WAITSCOUNT; S_i++) {
			if (WaitDescriptions[S_i] == nullptr) {
				continue;
			}

			VK_Pass::WaitDescription* Wait = (VK_Pass::WaitDescription*)WaitDescriptions[S_i];
			WP->base_data.WAITs[T_i] = *Wait;
			T_i++;
		}
		WP->WindowCalls[0].clear();
		WP->WindowCalls[1].clear();
		WP->WindowCalls[2].clear();

		*WindowPassHandle = (windowpass_tgfx_handle)WP;
		renderer->WindowPasses.push_back(WP);
		return result_tgfx_SUCCESS;
	}
	static void PrepareForNextFrame() {
		renderer->FrameIndex = (renderer->FrameIndex + 1) % 2;

		if(imgui){ imgui->NewFrame(); }
	}

	//Rendering operations
	static void Run() {
		printer(result_tgfx_SUCCESS, "Before cb waiting()!");

		unsigned char frame_i = renderer->Get_FrameIndex(false);

		//Wait for command buffers that are sent to render N-2th frame (N is the current frame)
		//As the name says, this function stops the thread while waiting for render calls (raster-compute-transfer-barrier calls).
		//With this way, we are sure that no command buffer execution will collide so we can change descriptor sets according to that.
		WaitForRenderGraphCommandBuffers();


		//Descriptor Set changes, vkFramebuffer changes etc.
		contentmanager->Apply_ResourceChanges();


		//This function is defined in rendergraph_primitive.cpp
		//Recording command buffers and queue submissions is handled here
		//That means: Sending CBs to queues and presenting swapchain to window happens here.
		Execute_RenderGraph();



		//Shift window semaphores
		const std::vector<window_vk*>& windows = core_vk->GET_WINDOWs();
		for (unsigned char WindowIndex = 0; WindowIndex < windows.size(); WindowIndex++) {
			window_vk* VKWINDOW = (window_vk*)(windows[WindowIndex]);

			semaphore_idtype_vk penultimate_semaphore = VKWINDOW->PresentationSemaphores[0];
			VKWINDOW->PresentationSemaphores[0] = VKWINDOW->PresentationSemaphores[1];
			VKWINDOW->PresentationSemaphores[1] = penultimate_semaphore;

			fence_idtype_vk penultimate_fence = VKWINDOW->PresentationFences[0];
			VKWINDOW->PresentationFences[0] = VKWINDOW->PresentationFences[1];
			VKWINDOW->PresentationFences[1] = penultimate_fence;
		}

		//Current frame has finished, so every call after this call affects to the next frame
		//That means we should make some changes/clean-ups for every system to work fine
		PrepareForNextFrame();
	}
	static void DrawDirect(buffer_tgfx_handle VertexBuffer_ID, buffer_tgfx_handle IndexBuffer_ID, unsigned int Count, unsigned int VertexOffset,
		unsigned int FirstIndex, unsigned int InstanceCount, unsigned int FirstInstance, rasterpipelineinstance_tgfx_handle MaterialInstance_ID, subdrawpass_tgfx_handle SubDrawPass_ID) {
		subdrawpass_vk* SP = (subdrawpass_vk*)SubDrawPass_ID;
		if (IndexBuffer_ID) {
			indexeddrawcall_vk call;
			if (VertexBuffer_ID) {
				FindBufferOBJ_byBufType(VertexBuffer_ID, buffertype_tgfx_VERTEX, call.VBuffer, call.VBOffset);
			}
			else {
				call.VBuffer = VK_NULL_HANDLE;
			}
			FindBufferOBJ_byBufType(IndexBuffer_ID, buffertype_tgfx_INDEX, call.IBuffer, call.IBOffset);
			call.IType = ((indexbuffer_vk*)IndexBuffer_ID)->DATATYPE;
			if (Count) {
				call.IndexCount = Count;
			}
			else {
				call.IndexCount = ((indexbuffer_vk*)IndexBuffer_ID)->IndexCount;
			}
			call.FirstIndex = FirstIndex;
			call.VOffset = VertexOffset;
			graphicspipelineinst_vk* PI = (graphicspipelineinst_vk*)MaterialInstance_ID;
			call.MatTypeObj = PI->PROGRAM->PipelineObject;
			call.MatTypeLayout = PI->PROGRAM->PipelineLayout;
			call.GeneralSet = &PI->PROGRAM->General_DescSet.Set;
			call.PerInstanceSet = &PI->DescSet.Set;
			call.FirstInstance = FirstInstance;
			call.InstanceCount = InstanceCount;
			SP->IndexedDrawCalls.push_back(call);
		}
		else {
			nonindexeddrawcall_vk call;
			if (VertexBuffer_ID) {
				FindBufferOBJ_byBufType(VertexBuffer_ID, buffertype_tgfx_VERTEX, call.VBuffer, call.VOffset);
			}
			else {
				call.VBuffer = VK_NULL_HANDLE;
			}
			if (Count) {
				call.VertexCount = Count;
			}
			else {
				call.VertexCount = static_cast<VkDeviceSize>(((vertexbuffer_vk*)VertexBuffer_ID)->VERTEX_COUNT);
			}
			call.FirstVertex = VertexOffset;
			call.FirstInstance = FirstInstance;
			call.InstanceCount = InstanceCount;
			graphicspipelineinst_vk* PI = (graphicspipelineinst_vk*)MaterialInstance_ID;
			call.MatTypeObj = PI->PROGRAM->PipelineObject;
			call.MatTypeLayout = PI->PROGRAM->PipelineLayout;
			call.GeneralSet = &PI->PROGRAM->General_DescSet.Set;
			call.PerInstanceSet = &PI->DescSet.Set;
			SP->NonIndexedDrawCalls.push_back(call);
		}
	}

	static void SwapBuffers(window_tgfx_handle WindowHandle, windowpass_tgfx_handle WindowPassHandle) {
		window_vk* VKWINDOW = (window_vk*)WindowHandle;
		if (VKWINDOW->isSwapped.load()) {
			return;
		}

		VKWINDOW->isSwapped.store(true);

		uint32_t SwapchainImage_Index;
		semaphore_vk& semaphore = semaphoresys->Create_Semaphore();
		fence_vk& fence = fencesys->CreateFence();
		vkAcquireNextImageKHR(rendergpu->LOGICALDEVICE(), VKWINDOW->Window_SwapChain, UINT64_MAX,
			semaphore.vksemaphore(), fence.Fence_o, &SwapchainImage_Index);
		if (SwapchainImage_Index != VKWINDOW->CurrentFrameSWPCHNIndex) {
			std::stringstream ErrorStream;
			ErrorStream << "Vulkan's reported SwapchainImage_Index: " << SwapchainImage_Index <<
				"\nGFX's stored SwapchainImage_Index: " << unsigned int(VKWINDOW->CurrentFrameSWPCHNIndex) <<
				"\nGFX's SwapchainIndex and Vulkan's SwapchainIndex don't match, there is something missing!";
			printer(result_tgfx_FAIL, ErrorStream.str().c_str());
		}

		VKWINDOW->PresentationSemaphores[1] = semaphore.get_id();
		VKWINDOW->PresentationFences[1] = fence.getID();

		windowpass_vk* wp = (windowpass_vk*)WindowPassHandle;
		windowcall_vk call;
		call.Window = VKWINDOW;
		wp->WindowCalls[2].push_back(call);
	}

	static void FindBufferOBJ_byBufType(buffer_tgfx_handle Handle, buffertype_tgfx TYPE, VkBuffer& TargetBuffer, VkDeviceSize& TargetOffset) {
		switch (TYPE) {
		case buffertype_tgfx_GLOBAL:
		{
			printer(result_tgfx_NOTCODED, "FindBufferOBJ_byBufType() isn't implemented for global buffer");
			/*
			VK_GlobalBuffer* buf = (VK_GlobalBuffer*)Handle;
			TargetBuffer = RENDERGPU->ALLOCS()[buf->Block.MemAllocIndex].Buffer;
			TargetOffset += buf->Block.Offset;*/
		}
		break;
		case buffertype_tgfx_VERTEX:
		{
			vertexbuffer_vk* buf = (vertexbuffer_vk*)Handle;
			TargetBuffer = allocatorsys->get_memorybufferhandle_byID(rendergpu, buf->Block.MemAllocIndex);
			TargetOffset += buf->Block.Offset;
		}
		break;
		case buffertype_tgfx_STAGING:
		{
			memoryblock_vk* Staging = (memoryblock_vk*)Handle;
			TargetBuffer = allocatorsys->get_memorybufferhandle_byID(rendergpu, Staging->MemAllocIndex);
			TargetOffset += Staging->Offset;
		}
		break;
		case buffertype_tgfx_INDEX:
		{
			indexbuffer_vk* IB = (indexbuffer_vk*)Handle;
			TargetBuffer = allocatorsys->get_memorybufferhandle_byID(rendergpu, IB->Block.MemAllocIndex);
			TargetOffset += IB->Block.Offset;
		}
		break;
		default:
			printer(result_tgfx_FAIL, "FindBufferOBJ_byBufType() doesn't support this type of buffer!");
		}
	}

	//Source Buffer should be created with HOSTVISIBLE or FASTHOSTVISIBLE
	//Target Buffer should be created with DEVICELOCAL
	static void CopyBuffer_toBuffer(transferpass_tgfx_handle TransferPassHandle, buffer_tgfx_handle SourceBuffer_Handle, buffertype_tgfx SourceBufferTYPE,
		buffer_tgfx_handle TargetBuffer_Handle, buffertype_tgfx TargetBufferTYPE, unsigned int SourceBuffer_Offset, unsigned int TargetBuffer_Offset, unsigned int Size) {
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
		DATAs->BUFBUFCopies.push_back(finalinfo);
	}

	//Source Buffer should be created with HOSTVISIBLE or FASTHOSTVISIBLE
	static void CopyBuffer_toImage(transferpass_tgfx_handle TransferPassHandle, buffer_tgfx_handle SourceBuffer_Handle, texture_tgfx_handle TextureHandle,
		unsigned int SourceBuffer_offset, boxregion_tgfx TargetTextureRegion, buffertype_tgfx SourceBufferTYPE, unsigned int TargetMipLevel,
		cubeface_tgfx TargetCubeMapFace) {
		texture_vk* TEXTURE = (texture_vk*)TextureHandle;
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
		if (TEXTURE->CHANNELs == texture_channels_tgfx_D24S8) {
			x.BufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else if (TEXTURE->CHANNELs == texture_channels_tgfx_D32) {
			x.BufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else {
			x.BufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		if (TEXTURE->DIMENSION == texture_dimensions_tgfx_2DCUBE) {
			x.BufferImageCopy.imageSubresource.baseArrayLayer = Find_TextureLayer_fromtgfx_cubeface(TargetCubeMapFace);
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
		if (TP->TYPE == transferpasstype_tgfx_BARRIER) {
			printer(result_tgfx_FAIL, "You gave an barrier TP handle to CopyBuffer_toImage(), this is wrong!");
			return;
		}
		VK_TPCopyDatas* DATAs = (VK_TPCopyDatas*)TP->TransferDatas;
		DATAs->BUFIMCopies.push_back(x);
	}

	static void CopyImage_toImage(transferpass_tgfx_handle TransferPassHandle, texture_tgfx_handle SourceTextureHandle, texture_tgfx_handle TargetTextureHandle,
		uvec3_tgfx SourceTextureOffset, uvec3_tgfx CopySize, uvec3_tgfx TargetTextureOffset, unsigned int SourceMipLevel, unsigned int TargetMipLevel,
		cubeface_tgfx SourceCubeMapFace, cubeface_tgfx TargetCubeMapFace);

	static void ImageBarrier(transferpass_tgfx_handle BarrierTPHandle, texture_tgfx_handle TextureHandle, image_access_tgfx LAST_ACCESS,
		image_access_tgfx NEXT_ACCESS, unsigned int TargetMipLevel, cubeface_tgfx TargetCubeMapFace) {
		texture_vk* Texture = (texture_vk*)TextureHandle;
		transferpass_vk* TP = (transferpass_vk*)BarrierTPHandle;
		if (TP->TYPE != transferpasstype_tgfx_BARRIER) {
			printer(result_tgfx_FAIL, "You should give the handle of a Barrier Transfer Pass! Given Transfer Pass' type isn't Barrier.");
			return;
		}
		VK_TPBarrierDatas* TPDatas = (VK_TPBarrierDatas*)TP->TransferDatas;


		VK_ImBarrierInfo im_bi;
		im_bi.Barrier.image = Texture->Image;
		Find_AccessPattern_byIMAGEACCESS(LAST_ACCESS, im_bi.Barrier.srcAccessMask, im_bi.Barrier.oldLayout);
		Find_AccessPattern_byIMAGEACCESS(NEXT_ACCESS, im_bi.Barrier.dstAccessMask, im_bi.Barrier.newLayout);
		if (Texture->CHANNELs == texture_channels_tgfx_D24S8) {
			im_bi.Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else if (Texture->CHANNELs == texture_channels_tgfx_D32) {
			im_bi.Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else {
			im_bi.Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		if (Texture->DIMENSION == texture_dimensions_tgfx_2DCUBE) {
			im_bi.Barrier.subresourceRange.baseArrayLayer = Find_TextureLayer_fromtgfx_cubeface(TargetCubeMapFace);
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


		TPDatas->TextureBarriers.push_back(im_bi);
	}

	static void Dispatch_Compute(computepass_tgfx_handle ComputePassHandle, computeshaderinstance_tgfx_handle CSInstanceHandle,
		unsigned int SubComputePassIndex, uvec3_tgfx DispatchSize) {
		computepass_vk* CP = (computepass_vk*)ComputePassHandle;


		subcomputepass_vk& SP = CP->Subpasses[SubComputePassIndex];
		dispatchcall_vk dc;
		dc.DispatchSize = glm::vec3(DispatchSize.x, DispatchSize.y, DispatchSize.z);
		computeinstance_vk* ci = (computeinstance_vk*)CSInstanceHandle;
		dc.GeneralSet = &ci->PROGRAM->General_DescSet.Set;
		dc.InstanceSet = &ci->DescSet.Set;
		dc.Layout = ci->PROGRAM->PipelineLayout;
		dc.Pipeline = ci->PROGRAM->PipelineObject;
		SP.Dispatches.push_back(dc);
	}


	static void ChangeDrawPass_RTSlotSet(drawpass_tgfx_handle DrawPassHandle, rtslotset_tgfx_handle RTSlotSetHandle);
};

void renderer_public::RendererResource_Finalizations() {
	//Handle RTSlotSet changes by recreating framebuffers of draw passes
	//by "RTSlotSet changes": DrawPass' slotset is changed to different one or slotset's slots is changed.
	for (unsigned int DrawPassIndex = 0; DrawPassIndex < renderer->DrawPasses.size(); DrawPassIndex++) {
		drawpass_vk* DP = renderer->DrawPasses[DrawPassIndex];
		unsigned char ChangeInfo = DP->SlotSetChanged.load();
		if (ChangeInfo == FrameIndex + 1 || ChangeInfo == 3 || DP->SLOTSET->PERFRAME_SLOTSETs[FrameIndex].IsChanged.load()) {
			if(DP->FBs[FrameIndex]){
				//This is safe because this FB is used 2 frames ago and CPU already waits for the frame's GPU process to end
				vkDestroyFramebuffer(rendergpu->LOGICALDEVICE(), DP->FBs[FrameIndex], nullptr);
			}

			VkFramebufferCreateInfo fb_ci = DP->SLOTSET->FB_ci[FrameIndex];
			fb_ci.renderPass = DP->RenderPassObject;
			if (vkCreateFramebuffer(rendergpu->LOGICALDEVICE(), &fb_ci, nullptr, &DP->FBs[FrameIndex]) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "vkCreateFramebuffer() has failed while changing one of the drawpasses' current frame slot's texture! Please report this!");
				return;
			}

			DP->SLOTSET->PERFRAME_SLOTSETs[FrameIndex].IsChanged.store(false);
			for (unsigned int SlotIndex = 0; SlotIndex < DP->SLOTSET->PERFRAME_SLOTSETs[FrameIndex].COLORSLOTs_COUNT; SlotIndex++) {
				DP->SLOTSET->PERFRAME_SLOTSETs[FrameIndex].COLOR_SLOTs[SlotIndex].IsChanged.store(false);
			}

			if (ChangeInfo) {
				DP->SlotSetChanged.store(ChangeInfo - FrameIndex - 1);
			}
		}
	}
}
unsigned char GetCurrentFrameIndex() {return renderer->Get_FrameIndex(false);}
inline void set_rendersysptrs() {
	core_tgfx_main->renderer->Start_RenderGraphConstruction = &Start_RenderGraphConstruction;
	core_tgfx_main->renderer->Finish_RenderGraphConstruction = &Finish_RenderGraphConstruction;
	core_tgfx_main->renderer->Destroy_RenderGraph = &Destroy_RenderGraph;

	core_tgfx_main->renderer->Create_TransferPass = &renderer_funcs::Create_TransferPass;
	core_tgfx_main->renderer->Create_DrawPass = &renderer_funcs::Create_DrawPass;
	core_tgfx_main->renderer->Create_WindowPass = &renderer_funcs::Create_WindowPass;
	core_tgfx_main->renderer->Create_ComputePass = &renderer_funcs::Create_ComputePass;
	core_tgfx_main->renderer->Run = &renderer_funcs::Run;
	core_tgfx_main->renderer->ImageBarrier = &renderer_funcs::ImageBarrier;
	core_tgfx_main->renderer->SwapBuffers = &renderer_funcs::SwapBuffers;
	core_tgfx_main->renderer->CopyBuffer_toBuffer = &renderer_funcs::CopyBuffer_toBuffer;
	core_tgfx_main->renderer->DrawDirect = &renderer_funcs::DrawDirect;
	core_tgfx_main->renderer->CopyBuffer_toImage = &renderer_funcs::CopyBuffer_toImage;
	core_tgfx_main->renderer->GetCurrentFrameIndex = &GetCurrentFrameIndex;
	core_tgfx_main->renderer->Dispatch_Compute = &renderer_funcs::Dispatch_Compute;
}
extern void Create_Renderer() {
	renderer = new renderer_public;
	hidden = new renderer_private;

	set_rendersysptrs();
}

bool transferpass_vk::isWorkloaded() {
	if (!TransferDatas) {
		return false;
	}
	switch (TYPE) {
	case transferpasstype_tgfx_BARRIER:
	{
		bool isThereAny = false;
		VK_TPBarrierDatas* TPB = (VK_TPBarrierDatas*)TransferDatas;
		//Check Buffer Barriers
		{
			std::unique_lock<std::mutex> BufferLocker;
			TPB->BufferBarriers.PauseAllOperations(BufferLocker);
			for (unsigned char ThreadID = 0; ThreadID < threadcount; ThreadID++) {
				if (TPB->BufferBarriers.size(ThreadID)) {
					return true;
				}
			}
		}
		//Check Texture Barriers
		if (!isThereAny) {
			std::unique_lock<std::mutex> TextureLocker;
			TPB->TextureBarriers.PauseAllOperations(TextureLocker);
			for (unsigned char ThreadID = 0; ThreadID < threadcount; ThreadID++) {
				if (TPB->TextureBarriers.size(ThreadID)) {
					return true;
				}
			}
		}
		return false;
	}
	break;
	case transferpasstype_tgfx_COPY:
	{
		bool isThereAny = false;
		VK_TPCopyDatas* DATAs = (VK_TPCopyDatas*)TransferDatas;
		{
			std::unique_lock<std::mutex> BUFBUFLOCKER;
			DATAs->BUFBUFCopies.PauseAllOperations(BUFBUFLOCKER);
			for (unsigned char ThreadIndex = 0; ThreadIndex < threadcount; ThreadIndex++) {
				if (DATAs->BUFBUFCopies.size(ThreadIndex)) {
					return true;
				}
			}
		}
		if (!isThereAny) {
			std::unique_lock<std::mutex> BUFIMLOCKER;
			DATAs->BUFIMCopies.PauseAllOperations(BUFIMLOCKER);
			for (unsigned char ThreadIndex = 0; ThreadIndex < threadcount; ThreadIndex++) {
				if (DATAs->BUFIMCopies.size(ThreadIndex)) {
					return true;
				}
			}
		}
		if (!isThereAny) {
			std::unique_lock<std::mutex> IMIMLOCKER;
			DATAs->IMIMCopies.PauseAllOperations(IMIMLOCKER);
			for (unsigned char ThreadIndex = 0; ThreadIndex < threadcount; ThreadIndex++) {
				if (DATAs->IMIMCopies.size(ThreadIndex)) {
					return true;
				}
			}
		}

		return false;
	}
	break;
	default:
		printer(result_tgfx_FAIL, "transferpass_vk::IsWorkloaded() doesn't support this type of transfer pass type!");
		return false;
	}




	return false;
}
bool computepass_vk::isWorkloaded() {
	for (unsigned char SubpassIndex = 0; SubpassIndex < Subpasses.size(); SubpassIndex++) {
		subcomputepass_vk& SP = Subpasses[SubpassIndex];
		if (SP.isThereWorkload()) {
			return true;
		}
	}
	return false;
}
bool windowpass_vk::isWorkloaded() {
	if (WindowCalls[3].size()) {
		return true;
	}
	return false;
}
bool drawpass_vk::isWorkloaded() {
	for (unsigned char SubpassIndex = 0; SubpassIndex < Subpass_Count; SubpassIndex++) {
		subdrawpass_vk& SP = Subpasses[SubpassIndex];
		if (SP.isThereWorkload()) {
			return true;
		}
	}
	return false;
}
bool subdrawpass_vk::isThereWorkload() {
	if (render_dearIMGUI) {
		return true;
	}
	{
		std::unique_lock<std::mutex> Locker;
		IndexedDrawCalls.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < threadcount; ThreadID++) {
			if (IndexedDrawCalls.size(ThreadID)) {
				return true;
			}
		}
	}
	{
		std::unique_lock<std::mutex> Locker;
		NonIndexedDrawCalls.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < threadcount; ThreadID++) {
			if (NonIndexedDrawCalls.size(ThreadID)) {
				return true;
			}
		}
	}
	return false;
}
bool subcomputepass_vk::isThereWorkload() {
	std::unique_lock<std::mutex> Locker;
	Dispatches.PauseAllOperations(Locker);
	for (unsigned char ThreadID = 0; ThreadID < threadcount; ThreadID++) {
		if (Dispatches.size(ThreadID)) {
			return true;
		}
	}
	return false;
} 