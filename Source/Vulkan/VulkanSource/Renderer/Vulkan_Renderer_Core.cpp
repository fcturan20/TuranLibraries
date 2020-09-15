#include "Vulkan_Renderer_Core.h"
#include "VK_GPUContentManager.h"
#define VKContentManager ((Vulkan::GPU_ContentManager*)GFXContentManager)
#define VKGPU ((Vulkan::GPU*)GFX->GPU_TO_RENDER)

namespace Vulkan {
	Renderer::Renderer() {

	}
	void Renderer::Start_RenderGraphCreation() {
		if (Record_RenderGraphCreation) {
			TuranAPI::LOG_CRASHING("GFXRENDERER->Start_RenderGraphCreation() has failed because you called it before!");
			return;
		}
		Record_RenderGraphCreation = true;
	}
	void Renderer::Finish_RenderGraphCreation() {
		if (!Record_RenderGraphCreation) {
			TuranAPI::LOG_CRASHING("GFXRENDERER->Finish_RenderGraphCreation() has failed because you either didn't start it or finished before!");
			return;
		}
		Record_RenderGraphCreation = false;

		//Command Pool Creation
		{
			VkCommandPoolCreateInfo cp_ci = {};
			cp_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cp_ci.queueFamilyIndex = VKGPU->Graphics_QueueFamily;
			cp_ci.flags = 0;
			cp_ci.pNext = nullptr;

			if (vkCreateCommandPool(VKGPU->Logical_Device, &cp_ci, nullptr, &GraphicsCmdPool) != VK_SUCCESS) {
				TuranAPI::LOG_CRASHING("Create_SwapchainDependentData() has failed because of vkCreateCommandPool()!");
				return;
			}
		}
		//RenderGraph resources (Global Buffers etc) creation
		VKContentManager->Create_RenderGraphResources();

		Create_SwapchainDependentData();
	}
	void Renderer::Create_SwapchainDependentData() {
		unsigned char SwapchainCount = ((WINDOW*)GFX->Main_Window)->SwapChain_Images.size();
		uint32_t uint32t_SwapchainCount = static_cast<uint32_t>(SwapchainCount);
		//Command Buffers Creation
		{
			VkCommandBufferAllocateInfo cb_ai = {};
			cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cb_ai.pNext = nullptr;
			cb_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cb_ai.commandPool = GraphicsCmdPool;
			cb_ai.commandBufferCount = uint32t_SwapchainCount;
			PerFrame_CommandBuffers.resize(uint32t_SwapchainCount);

			if (vkAllocateCommandBuffers(VKGPU->Logical_Device, &cb_ai, PerFrame_CommandBuffers.data()) != VK_SUCCESS) {
				TuranAPI::LOG_CRASHING("Create_SwapchainDependentData() has failed in vkAllocateCommandBuffers()!");
				return;
			}
		}
		//Framebuffer Creation
		for (unsigned int i = 0; i < DrawPasses.size(); i++) {
			Create_DrawPassFrameBuffers(SwapchainCount, DrawPasses[i]);
		}
	}
	void Renderer::Create_DrawPassFrameBuffers(unsigned char create_count, GFX_API::DrawPass& DrawPass) {
		VK_DrawPass* vkdp = (VK_DrawPass*)DrawPass.GL_ID;
		for (unsigned char c = 0; c < create_count; c++) {
			vkdp->FramebufferObjects.push_back(VkFramebuffer());
		}

		vector<VkImageView> Attachments;
		for (unsigned int i = 0; i < DrawPass.SLOTSET->SLOT_COUNT; i++) {
			VK_Texture* VKTexture = (VK_Texture*)DrawPass.SLOTSET->SLOTs[i]->RT->GL_ID;
			Attachments.push_back(VKTexture->ImageView);
		}
		VkFramebufferCreateInfo FrameBuffer_ci = {};
		FrameBuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FrameBuffer_ci.renderPass = vkdp->RenderPassObject;
		FrameBuffer_ci.attachmentCount = Attachments.size();
		FrameBuffer_ci.pAttachments = Attachments.data();
		FrameBuffer_ci.width = DrawPass.SLOTSET->SLOTs[0]->RT->WIDTH;
		FrameBuffer_ci.height = DrawPass.SLOTSET->SLOTs[0]->RT->HEIGHT;
		FrameBuffer_ci.layers = 1;
		FrameBuffer_ci.pNext = nullptr;
		FrameBuffer_ci.flags = 0;

		if (vkCreateFramebuffer(VKGPU->Logical_Device, &FrameBuffer_ci, nullptr, vkdp->FramebufferObjects.data() + vkdp->FramebufferObjects.size()) != VK_SUCCESS) {
			TuranAPI::LOG_CRASHING("Renderer::Create_DrawPassFrameBuffers() has failed!");
			return;
		}
	}


	void Fill_VkAttachmentDescription(VkAttachmentDescription& Desc, const GFX_API::RT_SLOT& Attachment) {
		Desc = {};
		Desc.format = Find_VkFormat_byTEXTURECHANNELs(Attachment.RT->CHANNELs);
		Desc.samples = VK_SAMPLE_COUNT_1_BIT;
		Desc.loadOp = Find_VkLoadOp_byAttachmentReadState(Attachment.RT_READTYPE);
		Desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		Desc.stencilLoadOp = Find_VkLoadOp_byAttachmentReadState(Attachment.RT_READTYPE);
		Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		Desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		Desc.finalLayout = Find_VkImageFinalLayout_byTextureType(Attachment.RT->TYPE);
		Desc.flags = 0;
	}
	bool Does_SlotSetsMatch(GFX_API::RT_SLOTSET& primary_slotset, GFX_API::RT_SLOTSET& inherited_slotset) {
		if (primary_slotset.SLOT_COUNT != inherited_slotset.SLOT_COUNT) {
			TuranAPI::LOG_ERROR("SlotSets don't match, both slotsets should have the same amount of slot count!");
			return false;
		}
		unsigned int slotcount = primary_slotset.SLOT_COUNT;
		for (unsigned int i = 0; i < slotcount; i++) {
			GFX_API::RT_SLOT* p_SLOT = primary_slotset.SLOTs[i];
			unsigned int slotindex = p_SLOT->SLOT_Index;
			GFX_API::RT_SLOT* i_SLOT = nullptr;
			for (unsigned int j = 0; j < slotcount; j++) {
				if (inherited_slotset.SLOTs[j]->SLOT_Index == slotindex) {
					i_SLOT = inherited_slotset.SLOTs[j];
				}
			}
			if (!i_SLOT) {
				TuranAPI::LOG_ERROR("SlotSets don't match, because inherited slotset doesn't have slot index: " + to_string(slotindex));
				return false;
			}
			if (p_SLOT->RT != i_SLOT->RT || p_SLOT->ATTACHMENT_TYPE != i_SLOT->ATTACHMENT_TYPE) {
				TuranAPI::LOG_ERROR("Slotsets don't match, because inherited slotset has different properties!");
				return false;
			}
			if ((p_SLOT->RT_OPERATIONTYPE == GFX_API::OPERATION_TYPE::READ_ONLY && i_SLOT->RT_OPERATIONTYPE != GFX_API::OPERATION_TYPE::READ_ONLY)) {
				TuranAPI::LOG_ERROR("Slotsets don't match, because inherited slotset can't write to the slot that primary slot uses as read-only");
			}
		}
	}
	bool Is_SubDrawPassList_Valid(vector<GFX_API::SubDrawPass_Description>& subpass_descs) {
		for (unsigned int renderpass_binding = 0; renderpass_binding < subpass_descs.size(); renderpass_binding++) {
			bool is_found = false;
			for (unsigned int desc_vectorindex = 0; desc_vectorindex < subpass_descs.size(); desc_vectorindex++) {
				if (subpass_descs[desc_vectorindex].SubDrawPass_Index == renderpass_binding) {
					if (!is_found) {
						is_found = true;
					}
					if (is_found) {
						TuranAPI::LOG_ERROR("SubDrawPass index: " + to_string(renderpass_binding) + " is used by more than one of the SubDrawPasses!");
						return false;
					}
				}
			}
			if (!is_found) {
				TuranAPI::LOG_ERROR("SubDrawPass index: " + to_string(renderpass_binding) + " isn't used by any subdrawpass!");
			}
		}
		return true;
	}
	void Fill_SubpassStructs(GFX_API::RT_SLOTSET* slotset, vector<VkSubpassDescription>& descs, vector<vector<VkAttachmentReference>>& attachments, vector<VkSubpassDependency>& dependencies) {
		attachments.push_back(vector<VkAttachmentReference>());
		vector<VkAttachmentReference>& sp_attachs = attachments[attachments.size() - 1];
		for (unsigned int i = 0; i < slotset->SLOT_COUNT; i++) {
			VkAttachmentReference Attachment_Reference = {};
			Attachment_Reference.attachment = 0;
			Attachment_Reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			sp_attachs.push_back(Attachment_Reference);
		}

		VkSubpassDescription Subpassdesc = {};
		Subpassdesc.colorAttachmentCount = sp_attachs.size();
		Subpassdesc.pColorAttachments = sp_attachs.data();
		Subpassdesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpassdesc.flags = 0;
		descs.push_back(Subpassdesc);


		VkSubpassDependency Subpassdepend = {};
		Subpassdepend.dependencyFlags = 0;
		Subpassdepend.srcSubpass = VK_SUBPASS_EXTERNAL;
		Subpassdepend.dstSubpass = 0;
		Subpassdepend.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		Subpassdepend.srcAccessMask = 0;
		Subpassdepend.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		Subpassdepend.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies.push_back(Subpassdepend);
	}
	void Renderer::Create_DrawPass(vector<GFX_API::SubDrawPass_Description>& Subpass_gfxdescs, unsigned int RTSLOTSET_ID) {
		if (!Record_RenderGraphCreation) {
			TuranAPI::LOG_CRASHING("GFXRENDERER->CreateDrawPass() has failed because you first need to call Start_RenderGraphCreation()!");
			return;
		}
		GFX_API::RT_SLOTSET* SLOTSET = VKContentManager->Find_RTSLOTSET_byID(RTSLOTSET_ID);
		if (!SLOTSET) {
			TuranAPI::LOG_ERROR("Create_DrawPass() has failed because RTSLOTSET_ID is invalid!");
			return;
		}
		if (!Is_SubDrawPassList_Valid(Subpass_gfxdescs)) {
			TuranAPI::LOG_ERROR("Create_DrawPass() has failed because SubDrawPasses list has problems about binding indexes!");
			return;
		}

		VK_DrawPass* VKDrawPass = new VK_DrawPass;

		vector<VkSubpassDependency> SubpassDepends;
		vector<VkSubpassDescription> SubpassDescs;
		vector<vector<VkAttachmentReference>> SubpassAttachments;


		for (unsigned int i = 0; i < Subpass_gfxdescs.size(); i++) {
			GFX_API::SubDrawPass_Description& Subpass_gfxdesc = Subpass_gfxdescs[i];
			GFX_API::RT_SLOTSET* Subpass_Slotset = VKContentManager->Find_RTSLOTSET_byID(Subpass_gfxdesc.SLOTSET_ID);
			if (!Subpass_Slotset) {
				TuranAPI::LOG_ERROR("Create_DrawPass() has failed because one of the SubDrawPasses has invalid SlotSet ID!");
				delete VKDrawPass;
				return;
			}
			if (!Does_SlotSetsMatch(*SLOTSET, *Subpass_Slotset)) {
				TuranAPI::LOG_ERROR("Create_DrawPass() has failed because one of the SubDrawPasses' SlotSet doesn't match with the DrawPass' one!");
				delete VKDrawPass;
				return;
			}
			//It is safe to create this subpass!
		}

		DrawPasses.push_back(GFX_API::DrawPass());
		GFX_API::DrawPass& GFXDrawPass = DrawPasses[DrawPasses.size() - 1];
		GFXDrawPass.ID = Create_DrawPassID();
		GFXDrawPass.GL_ID = VKDrawPass;
		GFXDrawPass.SLOTSET = SLOTSET;

		//vkRenderPass generation
		{
			GFX_API::SubDrawPass* SubDrawPasses = new GFX_API::SubDrawPass[Subpass_gfxdescs.size()];
			for (unsigned int i = 0; i < Subpass_gfxdescs.size(); i++) {
				GFX_API::SubDrawPass_Description& Subpass_gfxdesc = Subpass_gfxdescs[i];
				GFX_API::RT_SLOTSET* Subpass_Slotset = VKContentManager->Find_RTSLOTSET_byID(Subpass_gfxdesc.SLOTSET_ID);

				Fill_SubpassStructs(Subpass_Slotset, SubpassDescs, SubpassAttachments, SubpassDepends);

				SubDrawPasses[i].Binding_Index = Subpass_gfxdesc.SubDrawPass_Index;
				SubDrawPasses[i].ID = Create_SubDrawPassID();
				SubDrawPasses[i].SLOTSET = Subpass_Slotset;
			}


			vector<VkAttachmentDescription> AttachmentDescs;
			//Create Attachment Descriptions
			for (unsigned int i = 0; i < SLOTSET->SLOT_COUNT; i++) {
				VkAttachmentDescription Attachmentdesc = {};
				Fill_VkAttachmentDescription(Attachmentdesc, *SLOTSET->SLOTs[i]);
				AttachmentDescs.push_back(Attachmentdesc);
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

			if (vkCreateRenderPass(((GPU*)VKGPU)->Logical_Device, &RenderPass_CreationInfo, nullptr, &VKDrawPass->RenderPassObject) != VK_SUCCESS) {
				TuranAPI::LOG_CRASHING("Render Pass creation has failed!");
			}

			GFXDrawPass.Subpass_Count = Subpass_gfxdescs.size();
			GFXDrawPass.Subpasses = SubDrawPasses;
		}
	}















	//ID OPERATIONs

	GFX_API::DrawPass& Renderer::Find_DrawPass_byID(unsigned int ID) {
		for (unsigned int i = 0; i < DrawPasses.size(); i++) {
			GFX_API::DrawPass& DrawPass = DrawPasses[i];
			if (DrawPass.ID == ID) {
				return DrawPass;
			}
		}
		TuranAPI::LOG_ERROR("Find_DrawPass_byID() has failed, ID: " + to_string(ID));
	}
	GFX_API::SubDrawPass& Renderer::Find_SubDrawPass_byID(unsigned int ID, unsigned int* i_drawpass_id) {
		for (unsigned int i = 0; i < DrawPasses.size(); i++) {
			GFX_API::DrawPass& DrawPass = DrawPasses[i];
			for (unsigned int j = 0; j < DrawPass.Subpass_Count; j++) {
				GFX_API::SubDrawPass& Subpass = DrawPass.Subpasses[i];
				if (Subpass.ID == ID) {
					if (i_drawpass_id) {
						*i_drawpass_id = DrawPass.ID;
					}
					return Subpass;
				}
			}
		}
		TuranAPI::LOG_ERROR("Find_SubDrawPass_byID() has failed, ID: " + to_string(ID));
	}
	void Renderer::Create_PrimaryCommandBuffer() {
		/*
		Call_PreviousFrameDraws();
		Insert_PipelineBarriers();
		Call_NewFrameDraws();*/
	}
}