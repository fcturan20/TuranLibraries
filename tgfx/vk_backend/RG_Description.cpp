#include "predefinitions_vk.h"
#include <tgfx_renderer.h>
#include <tgfx_helper.h>
#include "renderer.h"
#include "core.h"
#include "RG.h"
#include "gpucontentmanager.h"
#include "imgui_vktgfx.h"
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include "queue.h"

RenderGraph* VKGLOBAL_RG = nullptr;
RGReconstructionStatus renderer_public::RGSTATUS() {
	return VKGLOBAL_RG->STATUS;
}
RenderGraph::DP_VK* RenderGraph::SubDP_VK::getDP() {
	DP_VK* dp = (DP_VK*)(uintptr_t(this) - (sizeof(SubDP_VK) * RG_BINDINDEX) - sizeof(DP_VK));
	if (dp->PASS_ID != DP_ID) { printer(result_tgfx_FAIL, "SubDP-MainDP doesn't match!"); }
	return dp;
}

struct RG_RECDATA {
	static std::atomic<VK_PASS_ID_TYPE> ID_GENERATOR;
	static vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_RGRECDYNAMICMEM;
	enum class WaitDescTYPE : unsigned char {
		subDP = 0,
		subCP = 1,
		subTP = 2
	};
	struct WaitDesc_SubDP  {
		bool isWaitingForLastFrame : 1;
		uint32_t SubPassIndex : 31;
		drawpass_tgfx_handle* passhandle;
		VKDATAHANDLE getHandle_ofDesc() {
			VKDATAHANDLE handle;
			handle.type = VKHANDLETYPEs::WAITDESC_SUBDP;
			handle.DATA_memoffset = VK_POINTER_TO_MEMOFFSET(this);
			return handle;
		};
		static WaitDesc_SubDP* getDesc_fromHandle(VKDATAHANDLE handle) {
			if (handle.type != VKHANDLETYPEs::WAITDESC_SUBDP) { return nullptr; }
			return (WaitDesc_SubDP*)VK_MEMOFFSET_TO_POINTER(handle.DATA_memoffset);
		};
	};
	struct WaitDesc_SubCP {
		bool isWaitingForLastFrame : 1;
		uint32_t SubPassIndex : 31;
		computepass_tgfx_handle* passhandle;
		VKDATAHANDLE getHandle_ofDesc() {
			VKDATAHANDLE handle;
			handle.type = VKHANDLETYPEs::WAITDESC_SUBCP;
			handle.DATA_memoffset = VK_POINTER_TO_MEMOFFSET(this);
			return handle;
		};
		static WaitDesc_SubCP* getDesc_fromHandle(VKDATAHANDLE handle) {
			if (handle.type != VKHANDLETYPEs::WAITDESC_SUBCP) { return nullptr; }
			return (WaitDesc_SubCP*)VK_MEMOFFSET_TO_POINTER(handle.DATA_memoffset);
		};
	};
	struct WaitDesc_SubTP {
		bool isWaitingForLastFrame : 1;
		uint32_t SubPassIndex : 31;
		transferpass_tgfx_handle* passhandle;
		VKDATAHANDLE getHandle_ofDesc() {
			VKDATAHANDLE handle;
			handle.type = VKHANDLETYPEs::WAITDESC_SUBTP;
			handle.DATA_memoffset = VK_POINTER_TO_MEMOFFSET(this);
			return handle;
		};
		static WaitDesc_SubTP* getDesc_fromHandle(VKDATAHANDLE handle) {
			if (handle.type != VKHANDLETYPEs::WAITDESC_SUBTP) { return nullptr; }
			return (WaitDesc_SubTP*)VK_MEMOFFSET_TO_POINTER(handle.DATA_memoffset);
		};
	};
	struct SubDPDESC {
		uint32_t IRTSLOTSET_ID = UINT32_MAX;
		subdrawpassaccess_tgfx WAITOP : 8, CONTINUEOP : 8;
		passwaitdescription_tgfx_listhandle waits;
		VKDATAHANDLE getHandle_ofDesc() {
			VKDATAHANDLE handle;
			handle.type = VKHANDLETYPEs::RGRECDATA_SUBDPDESC;
			handle.DATA_memoffset = VK_POINTER_TO_MEMOFFSET(this);
			return handle;
		};
		static SubDPDESC* getDesc_fromHandle(VKDATAHANDLE handle) {
			if (handle.type != VKHANDLETYPEs::RGRECDATA_SUBDPDESC) { return nullptr; }
			return (SubDPDESC*)VK_MEMOFFSET_TO_POINTER(handle.DATA_memoffset);
		};
	};
	struct SubTPDESC {
		transferpasstype_tgfx type;
		passwaitdescription_tgfx_listhandle waits;
		VKDATAHANDLE getHandle_ofDesc() {
			VKDATAHANDLE handle;
			handle.type = VKHANDLETYPEs::RGRECDATA_SUBTPDESC;
			handle.DATA_memoffset = VK_POINTER_TO_MEMOFFSET(this);
			return handle;
		};
		static SubTPDESC* getDesc_fromHandle(VKDATAHANDLE handle) {
			if (handle.type != VKHANDLETYPEs::RGRECDATA_SUBTPDESC) { return nullptr; }
			return (SubTPDESC*)VK_MEMOFFSET_TO_POINTER(handle.DATA_memoffset);
		};
	};
	struct SubCPDESC {
		passwaitdescription_tgfx_listhandle waits;
		VKDATAHANDLE getHandle_ofDesc() {
			VKDATAHANDLE handle;
			handle.type = VKHANDLETYPEs::RGRECDATA_SUBCPDESC;
			handle.DATA_memoffset = VK_POINTER_TO_MEMOFFSET(this);
			return handle;
		};
		static WaitDesc_SubCP* getDesc_fromHandle(VKDATAHANDLE handle) {
			if (handle.type != VKHANDLETYPEs::RGRECDATA_SUBCPDESC) { return nullptr; }
			return (WaitDesc_SubCP*)VK_MEMOFFSET_TO_POINTER(handle.DATA_memoffset);
		};
	};
	template<typename T> static VKDATAHANDLE getHandle_ofDesc(T* obj) { return obj->getHandle_ofDesc(); }
	template<typename T> static T* getDesc_fromHandle(VKDATAHANDLE handle) { return (T*)T::getDesc_fromHandle(handle); }

	static RenderGraph* RG_wip;
};
RenderGraph* RG_RECDATA::RG_wip = nullptr;
std::atomic<VK_PASS_ID_TYPE> RG_RECDATA::ID_GENERATOR = 0;
vk_virmem::dynamicmem* RG_RECDATA::VKGLOBAL_VIRMEM_RGRECDYNAMICMEM = nullptr;

void Start_RenderGraphConstruction() {
	//Apply resource changes made by user
	contentmanager->Resource_Finalizations();

	if (renderer->RGSTATUS() == RGReconstructionStatus::FinishConstructionCalled || renderer->RGSTATUS() == RGReconstructionStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "GFXRENDERER->Start_RenderGraphCreation() has failed because you have wrong function call order!"); return;
	}
	VKGLOBAL_RG->STATUS = RGReconstructionStatus::StartedConstruction;

	uint32_t dynamicmemoffset = 0;
	RG_RECDATA::VKGLOBAL_VIRMEM_RGRECDYNAMICMEM = vk_virmem::allocate_dynamicmem(RenderGraph::VKCONST_VIRMEM_RGALLOCATION_PAGECOUNT, &dynamicmemoffset);
	RG_RECDATA::RG_wip = (RenderGraph*)VK_MEMOFFSET_TO_POINTER(dynamicmemoffset);
	RG_RECDATA::RG_wip->PASSES_PTR = dynamicmemoffset;
}

bool Get_SlotSets_fromSubDP(subdrawpass_tgfx_handle subdp_handle, RTSLOTSET_VKOBJ** rtslotset, IRTSLOTSET_VKOBJ** irtslotset) {
	VKOBJHANDLE subdphandle = *(VKOBJHANDLE*)&subdp_handle;
	RenderGraph::SubDP_VK* subdp = getPass_fromHandle<RenderGraph::SubDP_VK>(subdphandle);
	if (!subdp) { printer(13); return false; }
	*rtslotset = contentmanager->GETRTSLOTSET_ARRAY().getOBJbyINDEX(subdp->getDP()->BASESLOTSET_ID);
	*irtslotset = contentmanager->GETIRTSLOTSET_ARRAY().getOBJbyINDEX(subdp->IRTSLOTSET_ID);
	return true;
}
bool Get_RPOBJ_andSPINDEX_fromSubDP(subdrawpass_tgfx_handle subdp_handle, VkRenderPass* rp_obj, unsigned int* sp_index) {
	VKOBJHANDLE subdphandle = *(VKOBJHANDLE*)&subdp_handle;
	RenderGraph::SubDP_VK* subdp = getPass_fromHandle<RenderGraph::SubDP_VK>(subdphandle);
	if (!subdp) { printer(13); return false; }
	*rp_obj = subdp->getDP()->RenderPassObject;
	*sp_index = subdp->VKRP_BINDINDEX;
	return true;
}

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
		printer(result_tgfx_NOTCODED, "This type of texture channel isn't supported by Fill_ColorVkAttachmentDescription");
	}
	Desc.flags = 0;
}
static void Fill_SubpassStructs(IRTSLOTSET_VKOBJ* slotset, subdrawpassaccess_tgfx WaitedOp, subdrawpassaccess_tgfx ContinueOp, unsigned char SubpassIndex, VkSubpassDescription& descs, VkSubpassDependency& dependencies) {
	RTSLOTSET_VKOBJ* BASESLOTSET = contentmanager->GETRTSLOTSET_ARRAY().getOBJbyINDEX(slotset->BASESLOTSET);
	VkAttachmentReference* COLOR_ATTACHMENTs = (VkAttachmentReference*)VK_ALLOCATE_AND_GETPTR(RG_RECDATA::VKGLOBAL_VIRMEM_RGRECDYNAMICMEM, sizeof(VkAttachmentReference) * BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT);
	VkAttachmentReference* DS_Attach = nullptr;

	for (unsigned int i = 0; i < BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT; i++) {
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
	if (BASESLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
		depthstencilslot_vk* depthslot = BASESLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT;
		DS_Attach = (VkAttachmentReference*)VK_ALLOCATE_AND_GETPTR(RG_RECDATA::VKGLOBAL_VIRMEM_RGRECDYNAMICMEM, sizeof(VkAttachmentReference));
		rendergpu->EXTMANAGER()->Fill_DepthAttachmentReference(*DS_Attach, BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT,
			depthslot->RT->CHANNELs, slotset->DEPTH_OPTYPE, slotset->STENCIL_OPTYPE);
	}

	descs = {};
	descs.colorAttachmentCount = BASESLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT;
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

//For second order subpasses, check waitdescs
//Check against if 2 different mainpasses of the same type are waited for
result_tgfx check_waitdescs_of_subpass(passwaitdescription_tgfx_listhandle waits, drawpass_tgfx_handle** dphandle, computepass_tgfx_handle** cphandle, transferpass_tgfx_handle** tphandle) {
	TGFXLISTCOUNT(core_tgfx_main, waits, waitlistsize);
	for (unsigned int wait_i = 0; wait_i < waitlistsize; wait_i++) {
		if (!waits[wait_i]) { continue; }
		VKDATAHANDLE handle = *(VKDATAHANDLE*)&waits[wait_i];
		switch (handle.type) {
		case VKHANDLETYPEs::WAITDESC_SUBDP:
		{
			RG_RECDATA::WaitDesc_SubDP* waitdesc = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::WaitDesc_SubDP>(handle);
			if (*dphandle) { if (waitdesc->passhandle != *dphandle) { return printer(1); } }
			else { *dphandle = waitdesc->passhandle; }
		}
		break;
		case VKHANDLETYPEs::WAITDESC_SUBCP:
		{
			RG_RECDATA::WaitDesc_SubCP* waitdesc = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::WaitDesc_SubCP>(handle);
			if (*cphandle) { if (waitdesc->passhandle != *cphandle) { return printer(1); } }
			else { *cphandle = waitdesc->passhandle; }
		}
		break;
		case VKHANDLETYPEs::WAITDESC_SUBTP:
		{
			RG_RECDATA::WaitDesc_SubTP* waitdesc = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::WaitDesc_SubTP>(handle);
			if (*tphandle) { if (waitdesc->passhandle != *tphandle) { return printer(1); } }
			else { *tphandle = waitdesc->passhandle; }
		}
		break;
		default:
			return printer(2);
		}
	}
	return result_tgfx_SUCCESS;
}
static result_tgfx Create_DrawPass(subdrawpassdescription_tgfx_listhandle subDPdescs, rtslotset_tgfx_handle RTSLOTSET_ID,
	subdrawpass_tgfx_listhandle SubDrawPassHandles, drawpass_tgfx_handle* DPHandle) {
	if (renderer->RGSTATUS() != RGReconstructionStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "GFXRENDERER->CreateDrawPass() has failed because you first need to call Start_RenderGraphCreation()!");
		return result_tgfx_FAIL;
	}
	RTSLOTSET_VKOBJ* SLOTSET = contentmanager->GETRTSLOTSET_ARRAY().getOBJfromHANDLE(*(VKOBJHANDLE*)&RTSLOTSET_ID);
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
	TGFXLISTCOUNT(core_tgfx_main, subDPdescs, SubDPsLISTSIZE);
	//Count every kind of subDPs
	unsigned int RenderPass_subDPs_COUNT = 0, BarrierOnly_subDPs_COUNT = 0;
#ifdef VULKAN_DEBUGGING
	//Check first subdrawpass
	{
		RG_RECDATA::SubDPDESC* dpdesc = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::SubDPDESC>(*(VKDATAHANDLE*)&subDPdescs[0]);
		if (dpdesc->IRTSLOTSET_ID == UINT32_MAX) {
			printer(result_tgfx_INVALIDARGUMENT, "First subdrawpass should have valid irtslotset!");
			return result_tgfx_INVALIDARGUMENT; }
		IRTSLOTSET_VKOBJ* ISET = contentmanager->GETIRTSLOTSET_ARRAY().getOBJbyINDEX(dpdesc->IRTSLOTSET_ID);
		if (!ISET) {
			printer(result_tgfx_FAIL, "GFXRenderer->Create_DrawPass() has failed because first inherited slotset isn't valid!");
			return result_tgfx_INVALIDARGUMENT;
		}
		if (contentmanager->GETRTSLOTSET_ARRAY().getOBJbyINDEX(ISET->BASESLOTSET) != SLOTSET) {
			printer(result_tgfx_FAIL, "GFXRenderer->Create_DrawPass() has failed because first inherited slotset isn't inherited from the DrawPass' Base Slotset!");
			return result_tgfx_INVALIDARGUMENT;
		}
		TGFXLISTCOUNT(core_tgfx_main, dpdesc->waits, waitlist_size);
		for (unsigned int wait_i = 0; wait_i < waitlist_size; wait_i++) {
			if (!dpdesc->waits[wait_i]) { continue; }
			VKDATAHANDLE handle = *(VKDATAHANDLE*)&dpdesc->waits[wait_i];
			if (handle.type == VKHANDLETYPEs::WAITDESC_SUBDP) {
				RG_RECDATA::WaitDesc_SubDP* waitdesc = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::WaitDesc_SubDP>(handle);
				if (waitdesc->passhandle == DPHandle) { printer(result_tgfx_INVALIDARGUMENT, "First subdrawpass can't wait for any subpass of the same drawpass"); return result_tgfx_INVALIDARGUMENT; }
			}
		}
	}
#endif
	//Check other subdrawpasses
	computepass_tgfx_handle* merged_computepass = nullptr;
	transferpass_tgfx_handle* merged_transferpass = nullptr;
	for (unsigned int subdp_i = 1; subdp_i < SubDPsLISTSIZE; subdp_i++) {
		RG_RECDATA::SubDPDESC* dpdesc = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::SubDPDESC>(*(VKDATAHANDLE*)&subDPdescs[0]);
		if (dpdesc->IRTSLOTSET_ID == UINT32_MAX) { BarrierOnly_subDPs_COUNT++; }
		else { RenderPass_subDPs_COUNT++; }
#ifdef VULKAN_DEBUGGING
		IRTSLOTSET_VKOBJ* ISET = contentmanager->GETIRTSLOTSET_ARRAY().getOBJbyINDEX(dpdesc->IRTSLOTSET_ID);
		if (!ISET) {
			printer(4);
			return result_tgfx_INVALIDARGUMENT;
		}
		if (contentmanager->GETRTSLOTSET_ARRAY().getOBJbyINDEX(ISET->BASESLOTSET) != SLOTSET) {
			printer(5);
			return result_tgfx_INVALIDARGUMENT;
		}
		//Check which passes are merged with this draw pass
		check_waitdescs_of_subpass(dpdesc->waits, &DPHandle, &merged_computepass, &merged_transferpass);
#endif
	}

	RenderGraph::DP_VK VKDrawPass;
	
	VKDrawPass.BASESLOTSET_ID = contentmanager->GETRTSLOTSET_ARRAY().getINDEXbyOBJ(SLOTSET);
	VKDrawPass.ALLSubDPsCOUNT = RenderPass_subDPs_COUNT + BarrierOnly_subDPs_COUNT;

	VKDrawPass.RenderRegion.XOffset = 0;
	VKDrawPass.RenderRegion.YOffset = 0;
	if (SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT) {
		VKDrawPass.RenderRegion.WIDTH = SLOTSET->PERFRAME_SLOTSETs[0].COLOR_SLOTs[0].RT->WIDTH;
		VKDrawPass.RenderRegion.HEIGHT = SLOTSET->PERFRAME_SLOTSETs[0].COLOR_SLOTs[0].RT->HEIGHT;
	}
	else {
		VKDrawPass.RenderRegion.WIDTH = SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->RT->WIDTH;
		VKDrawPass.RenderRegion.HEIGHT = SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->RT->HEIGHT;
	}


	//SubDrawPass creation
	VkSubpassDependency* SubpassDepends = (VkSubpassDependency*)VK_ALLOCATE_AND_GETPTR(RG_RECDATA::VKGLOBAL_VIRMEM_RGRECDYNAMICMEM, sizeof(VkSubpassDependency) * RenderPass_subDPs_COUNT);
	VkSubpassDescription* SubpassDescs = (VkSubpassDescription*)VK_ALLOCATE_AND_GETPTR(RG_RECDATA::VKGLOBAL_VIRMEM_RGRECDYNAMICMEM, sizeof(VkSubpassDescription) * RenderPass_subDPs_COUNT);

	for (unsigned int i = 0; i < SubDPsLISTSIZE; i++) {
		RG_RECDATA::SubDPDESC* Subpass_gfxdesc = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::SubDPDESC>(*(VKDATAHANDLE*)&subDPdescs[i]);
		if (!Subpass_gfxdesc) { continue; }
		IRTSLOTSET_VKOBJ* Subpass_Slotset = contentmanager->GETIRTSLOTSET_ARRAY().getOBJbyINDEX(Subpass_gfxdesc->IRTSLOTSET_ID);
		Fill_SubpassStructs(Subpass_Slotset, Subpass_gfxdesc->WAITOP, Subpass_gfxdesc->CONTINUEOP, i, SubpassDescs[i], SubpassDepends[i]);
	}

	//vkRenderPass generation
	{
		VkAttachmentDescription* AttachmentDescs = (VkAttachmentDescription*)VK_ALLOCATE_AND_GETPTR(RG_RECDATA::VKGLOBAL_VIRMEM_RGRECDYNAMICMEM, sizeof(VkAttachmentDescription) * 
			(SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT + 1));
		uint8_t attachmentdescs_count = 0;
		//Create Attachment Descriptions
		for (unsigned int i = 0; i < SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT; i++) {
			Fill_ColorVkAttachmentDescription(AttachmentDescs[attachmentdescs_count++], &SLOTSET->PERFRAME_SLOTSETs[0].COLOR_SLOTs[i]);
		}
		if (SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
			rendergpu->EXTMANAGER()->Fill_DepthAttachmentDescription(AttachmentDescs[attachmentdescs_count++], SLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT);
		}


		//RenderPass Creation
		VkRenderPassCreateInfo RenderPass_CreationInfo = {};
		RenderPass_CreationInfo.flags = 0;
		RenderPass_CreationInfo.pNext = nullptr;
		RenderPass_CreationInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		RenderPass_CreationInfo.subpassCount = SubDPsLISTSIZE;
		RenderPass_CreationInfo.pSubpasses = SubpassDescs;
		RenderPass_CreationInfo.attachmentCount = attachmentdescs_count;
		RenderPass_CreationInfo.pAttachments = AttachmentDescs;
		RenderPass_CreationInfo.dependencyCount = SubDPsLISTSIZE;
		RenderPass_CreationInfo.pDependencies = SubpassDepends;
		VKDrawPass.SlotSetChanged.store(3);

		if (vkCreateRenderPass(rendergpu->LOGICALDEVICE(), &RenderPass_CreationInfo, nullptr, &VKDrawPass.RenderPassObject) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "VkCreateRenderPass() has failed!");
			return result_tgfx_FAIL;
		}
	}

	//Create DP struct in reconstructed RenderGraph memory block
	uint32_t dp_malloc_size = sizeof(RenderGraph::DP_VK) + (SubDPsLISTSIZE * sizeof(RenderGraph::SubDP_VK));
	RenderGraph::DP_VK* DP_final = (RenderGraph::DP_VK*)VK_ALLOCATE_AND_GETPTR(RG_RECDATA::VKGLOBAL_VIRMEM_RGRECDYNAMICMEM, dp_malloc_size);
	DP_final->PASSTYPE = VK_PASSTYPE::DP;
	DP_final->BASESLOTSET_ID = VKDrawPass.BASESLOTSET_ID;
	for (unsigned int i = 0; i < VKCONST_BUFFERING_IN_FLIGHT; i++) {
		DP_final->FBs[i] = VKDrawPass.FBs[i];
	}
	DP_final->RenderPassObject = VKDrawPass.RenderPassObject;
	DP_final->RenderRegion = VKDrawPass.RenderRegion;
	DP_final->SlotSetChanged.store(VKDrawPass.SlotSetChanged.load());
	DP_final->ALLSubDPsCOUNT = VKDrawPass.ALLSubDPsCOUNT;

	//Create new element in the BasePass list
	DP_final->PASS_ID = RG_RECDATA::ID_GENERATOR.fetch_add(1);


	//Create subpass structures too
	RenderGraph::SubDP_VK* Final_Subpasses = (RenderGraph::SubDP_VK*)(uintptr_t(DP_final) + sizeof(RenderGraph::DP_VK));
	for (unsigned int i = 0, s = 0, rp_bi = 0; i < DP_final->ALLSubDPsCOUNT; i++) {
		//Get next valid desc
		RG_RECDATA::SubDPDESC* desc = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::SubDPDESC>(*(VKDATAHANDLE*)&subDPdescs[s++]);
		while (!desc) { desc = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::SubDPDESC>(*(VKDATAHANDLE*)&subDPdescs[s++]); }

		//Fill subdp
		RenderGraph::SubDP_VK& subdp = Final_Subpasses[i];
		subdp.RG_BINDINDEX = i;
		subdp.continueop = desc->CONTINUEOP;
		subdp.DP_ID = DP_final->PASS_ID;
		if (desc->IRTSLOTSET_ID == UINT32_MAX) {
			subdp.IRTSLOTSET_ID = UINT32_MAX;
			subdp.is_RENDERPASSRELATED = false;
			subdp.VKRP_BINDINDEX = rp_bi;
		}
		else {
			subdp.IRTSLOTSET_ID = desc->IRTSLOTSET_ID;
			subdp.is_RENDERPASSRELATED = true;
			subdp.VKRP_BINDINDEX = rp_bi++;
		}
		subdp.render_dearIMGUI = false;		//This will be set when you finish RG reconstruction
		subdp.waitop = desc->WAITOP;
		if (i == 0) {
			DP_final->passwaits = desc->waits;
		}
		VKOBJHANDLE subdp_finalhandle = getHandle_ofpass(&Final_Subpasses[i]);
		SubDrawPassHandles[i] = *(subdrawpass_tgfx_handle*)&subdp_finalhandle;
	}

	VKOBJHANDLE finalhandle = getHandle_ofpass(DP_final);
	*DPHandle = *(drawpass_tgfx_handle*)&finalhandle;
	return result_tgfx_SUCCESS;
}
static result_tgfx Create_TransferPass(subtransferpassdescription_tgfx_listhandle subTPdescs,
	subtransferpass_tgfx_listhandle subTPHandles, transferpass_tgfx_handle* TPHandle) {
	if (renderer->RGSTATUS() != RGReconstructionStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "You should call Start_RenderGraphConstruction() before RENDERER->Create_TransferPass()!");
		return result_tgfx_FAIL;
	}

	TGFXLISTCOUNT(core_tgfx_main, subTPdescs, subtplistsize);
	unsigned int barrier_subtpcount = 0, copy_subtpcount = 0;
	computepass_tgfx_handle* merged_cp = nullptr;
	drawpass_tgfx_handle* merged_dp = nullptr;
	for (unsigned int subpass_i = 0; subpass_i < subtplistsize; subpass_i++) {
		if (!subTPdescs[subpass_i]) { continue; }
		RG_RECDATA::SubTPDESC* desc = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::SubTPDESC>(*(VKDATAHANDLE*)&subTPdescs[subpass_i]);
		if (desc->type == transferpasstype_tgfx_BARRIER) { barrier_subtpcount++;}
		if(desc->type == transferpasstype_tgfx_COPY){copy_subtpcount++;}
#ifdef VULKAN_DEBUGGING
		else { return printer(3);}
		check_waitdescs_of_subpass(desc->waits, &merged_dp, &merged_cp, &TPHandle);
#endif
	}
	
	uint32_t size_of_subtps = (sizeof(RenderGraph::SubTPCOPY_VK) * copy_subtpcount) + (sizeof(RenderGraph::SubTPBARRIER_VK) * barrier_subtpcount);
	uint32_t sizeof_malloc = sizeof(RenderGraph::TP_VK) + size_of_subtps;
	RenderGraph::TP_VK* tp = (RenderGraph::TP_VK*)VK_ALLOCATE_AND_GETPTR(RG_RECDATA::VKGLOBAL_VIRMEM_RGRECDYNAMICMEM, sizeof_malloc);
	*tp = RenderGraph::TP_VK();
	tp->PASS_ID = RG_RECDATA::ID_GENERATOR.fetch_add(1);
	tp->SubPassesCount = barrier_subtpcount + copy_subtpcount;
	tp->SubPassesList_SizeInBytes = size_of_subtps;
	//Fill Subpass structs
	uintptr_t last_ptr = uintptr_t(tp) + sizeof(RenderGraph::TP_VK);
	for (unsigned int subpass_i = 0; subpass_i < barrier_subtpcount + copy_subtpcount; subpass_i++) {
		if (!subTPdescs[subpass_i]) { continue; }
		RG_RECDATA::SubTPDESC* desc = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::SubTPDESC>(*(VKDATAHANDLE*)&subTPdescs[subpass_i]);
		if (desc->type == transferpasstype_tgfx_BARRIER) { 
			RenderGraph::SubTPBARRIER_VK* tpbar = (RenderGraph::SubTPBARRIER_VK*)last_ptr;
			tpbar->BufferBarriers = VK_VECTOR_ADDONLY<RenderGraph::VK_BufBarrierInfo, 1 << 16>();
			tpbar->TextureBarriers = VK_VECTOR_ADDONLY<RenderGraph::VK_ImBarrierInfo, 1 << 16>();

			VKOBJHANDLE subdp_finalhandle = getHandle_ofpass(tpbar);
			subTPHandles[subpass_i] = *(subtransferpass_tgfx_handle*)&subdp_finalhandle;

			last_ptr += sizeof(RenderGraph::SubTPBARRIER_VK);
		}
		if (desc->type == transferpasstype_tgfx_COPY) {
			RenderGraph::SubTPCOPY_VK* tpcopy = (RenderGraph::SubTPCOPY_VK*)last_ptr;
			tpcopy->BUFBUFCopies = VK_VECTOR_ADDONLY<RenderGraph::VK_BUFtoBUFinfo, 1 << 16>();
			tpcopy->BUFIMCopies = VK_VECTOR_ADDONLY<RenderGraph::VK_BUFtoIMinfo, 1 << 16>();
			tpcopy->IMIMCopies = VK_VECTOR_ADDONLY<RenderGraph::VK_IMtoIMinfo, 1 << 16>();

			VKOBJHANDLE subdp_finalhandle = getHandle_ofpass(tpcopy);
			subTPHandles[subpass_i] = *(subtransferpass_tgfx_handle*)&subdp_finalhandle;

			last_ptr += sizeof(RenderGraph::SubTPCOPY_VK);
		}

		if (subpass_i == 0) {
			tp->passwaits = desc->waits;
		}
	}

	VKOBJHANDLE handle = getHandle_ofpass(tp);
	*TPHandle = *(transferpass_tgfx_handle*)&handle;
	return result_tgfx_SUCCESS;
}
static result_tgfx Create_ComputePass(subcomputepassdescription_tgfx_listhandle subCPdescs,
	subcomputepass_tgfx_listhandle subCPHandles, computepass_tgfx_handle* CPHandle) {
	if (renderer->RGSTATUS() != RGReconstructionStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "You should call Start_RenderGraphConstruction() before RENDERER->Create_ComputePass()!");
		return result_tgfx_FAIL;
	}


	TGFXLISTCOUNT(core_tgfx_main, subCPdescs, subcplistsize);
	unsigned int subcpcount = 0;
	drawpass_tgfx_handle* merged_dp = nullptr;
	transferpass_tgfx_handle* merged_tp = nullptr;
	for (unsigned int subpass_i = 0; subpass_i < subcplistsize; subpass_i++) {
		if (!subCPdescs[subpass_i]) { continue; }
		RG_RECDATA::SubCPDESC* desc = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::SubCPDESC>(*(VKDATAHANDLE*)&subCPdescs[subpass_i]);
#ifdef VULKAN_DEBUGGING
		check_waitdescs_of_subpass(desc->waits, &merged_dp, &CPHandle, &merged_tp);
#endif
		subcpcount++;
	}

	uint32_t sizeof_malloc = sizeof(RenderGraph::CP_VK) + (sizeof(RenderGraph::SubCP_VK*) * subcpcount);
	RenderGraph::CP_VK* CP = (RenderGraph::CP_VK*)VK_ALLOCATE_AND_GETPTR(RG_RECDATA::VKGLOBAL_VIRMEM_RGRECDYNAMICMEM, sizeof_malloc);
	CP->PASS_ID = RG_RECDATA::ID_GENERATOR.fetch_add(1);
	CP->SubPassesCount = subcpcount;
	RenderGraph::SubCP_VK* final_subcps = (RenderGraph::SubCP_VK*)(uintptr_t(CP) + sizeof(RenderGraph::CP_VK));
	//Fill the pass wait description list
	for (unsigned int subpass_i = 0, list_i = 0; list_i < subcplistsize; list_i++) {
		if (subCPdescs[list_i] == nullptr) {
			continue;
		}

		RG_RECDATA::SubCPDESC* desc = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::SubCPDESC>(*(VKDATAHANDLE*)&subCPdescs[subpass_i]);
		RenderGraph::SubCP_VK* subcp = &final_subcps[subpass_i];
		if (subpass_i == 0) {
			CP->passwaits = desc->waits;
		}
		*subcp = RenderGraph::SubCP_VK();

		VKOBJHANDLE handle = getHandle_ofpass(subcp);
		subCPHandles[subpass_i] = *(subcomputepass_tgfx_handle*)&handle;

		subpass_i++;
	}
	
	VKOBJHANDLE handle = getHandle_ofpass(CP);
	*CPHandle = *(computepass_tgfx_handle*)&handle;
	return result_tgfx_SUCCESS;
}
static result_tgfx Create_WindowPass(passwaitdescription_tgfx_listhandle WaitDescriptions, windowpass_tgfx_handle* WindowPassHandle) {
	if (renderer->RGSTATUS() != RGReconstructionStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "You should call Start_RenderGraphConstruction() before RENDERER->Create_ComputePass()!");
		return result_tgfx_FAIL;
	}


	RenderGraph::WP_VK* WP = (RenderGraph::WP_VK*)VK_ALLOCATE_AND_GETPTR(RG_RECDATA::VKGLOBAL_VIRMEM_RGRECDYNAMICMEM, sizeof(RenderGraph::WP_VK));
	WP->WindowCalls = VK_VECTOR_ADDONLY<RenderGraph::windowcall_vk, 256>();
	WP->PASS_ID = RG_RECDATA::ID_GENERATOR.fetch_add(1);
	WP->passwaits = WaitDescriptions;

	
	VKOBJHANDLE handle = getHandle_ofpass(WP);
	*WindowPassHandle = *(windowpass_tgfx_handle*)&handle;
	return result_tgfx_SUCCESS;
}

inline static bool Check_WaitHandles() {
	uint16_t current_PASS_ID = 0;
	uintptr_t last_ptr = (uintptr_t)VKCONST_VIRMEMSPACE_BEGIN + RG_RECDATA::RG_wip->PASSES_PTR;
	while (current_PASS_ID < RG_RECDATA::ID_GENERATOR.load() && 
		last_ptr < uintptr_t(VKCONST_VIRMEMSPACE_BEGIN) + (RG_RECDATA::RG_wip->VKCONST_VIRMEM_RGALLOCATION_PAGECOUNT * VKCONST_VIRMEMPAGESIZE)) {
		VK_PASSTYPE type = *(VK_PASSTYPE*)last_ptr;
		passwaitdescription_tgfx_listhandle waitlist = nullptr;
		switch (type) {
		case VK_PASSTYPE::DP:
			if (current_PASS_ID != ((RenderGraph::DP_VK*)last_ptr)->PASS_ID) { printer(9); return false; }
			waitlist = ((RenderGraph::DP_VK*)last_ptr)->passwaits;
			break;
		case VK_PASSTYPE::CP:
			waitlist = ((RenderGraph::CP_VK*)last_ptr)->passwaits;
			break;
		case VK_PASSTYPE::TP:
			waitlist = ((RenderGraph::TP_VK*)last_ptr)->passwaits;
			break;
		case VK_PASSTYPE::WP:
			waitlist = ((RenderGraph::WP_VK*)last_ptr)->passwaits;
		default:
			printer(6);
			return false;
		}

		TGFXLISTCOUNT(core_tgfx_main, waitlist, waitlistsize);
		for (unsigned int wait_i = 0; wait_i < waitlistsize; wait_i++) {
			VKDATAHANDLE passwaitdesc_handle = *(VKDATAHANDLE*)&(waitlist[wait_i]);
			switch (passwaitdesc_handle.type) {
			case VKHANDLETYPEs::WAITDESC_SUBDP:
			{
				RG_RECDATA::WaitDesc_SubDP* wait_subdp = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::WaitDesc_SubDP>(passwaitdesc_handle);
				VKOBJHANDLE passhandle = *(VKOBJHANDLE*)wait_subdp->passhandle;
				if (passhandle.type != VKHANDLETYPEs::DRAWPASS) { printer(7); }
				RenderGraph::DP_VK* dp = getPass_fromHandle<RenderGraph::DP_VK>(passhandle);
				if (!dp) { printer(7); return false; }
				if (wait_subdp->SubPassIndex >= dp->ALLSubDPsCOUNT) { printer(8); return false; }
			}
			break;
			case VKHANDLETYPEs::WAITDESC_SUBCP:
			{
				RG_RECDATA::WaitDesc_SubCP* wait_subcp = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::WaitDesc_SubCP>(passwaitdesc_handle);
				VKOBJHANDLE passhandle = *(VKOBJHANDLE*)wait_subcp->passhandle;
				if (passhandle.type != VKHANDLETYPEs::COMPUTEPASS) { printer(7); }
				RenderGraph::CP_VK* cp = getPass_fromHandle<RenderGraph::CP_VK>(passhandle);
				if (!cp) { printer(7); return false; }
				if (wait_subcp->SubPassIndex >= cp->SubPassesCount) { printer(8); return false; }
			}
			break;
			case VKHANDLETYPEs::WAITDESC_SUBTP:
			{
				RG_RECDATA::WaitDesc_SubTP* wait_subtp = RG_RECDATA::getDesc_fromHandle<RG_RECDATA::WaitDesc_SubTP>(passwaitdesc_handle);
				VKOBJHANDLE passhandle = *(VKOBJHANDLE*)wait_subtp->passhandle;
				if (passhandle.type != VKHANDLETYPEs::TRANSFERPASS) { printer(7); }
				RenderGraph::TP_VK* tp = getPass_fromHandle<RenderGraph::TP_VK>(passhandle);
				if (!tp) { printer(7); return false; }
				if (wait_subtp->SubPassIndex >= tp->SubPassesCount) { printer(8); return false; }
			}
			break;
			default:
				printer(6);
				break;
			}
		}

		current_PASS_ID++;
		last_ptr += RenderGraph::GetSize_ofType(type, (void*)last_ptr);
	}

	return true;
}
extern void CheckIMGUIVKResults(VkResult result);
unsigned char Finish_RenderGraphConstruction(subdrawpass_tgfx_handle IMGUI_Subpass) {
	if (VKGLOBAL_RG->STATUS != RGReconstructionStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "VulkanRenderer->Finish_RenderGraphCreation() has failed because you didn't call Start_RenderGraphConstruction() before!");
		return false;
	}

	//Checks wait handles of the render nodes, if matching fails return false
	if (!Check_WaitHandles()) {
		printer(result_tgfx_NOTCODED, "VulkanRenderer->Finish_RenderGraphConstruction() has failed but should return to old RenderGraph. This is not the case for now! Fixes: Return back to old rendergraph, destroy all vk objects created for deleted passes");
		VKGLOBAL_RG->STATUS = RGReconstructionStatus::Invalid;	//User can change only waits of a pass, so all rendergraph falls to invalid in this case
		//It needs to be reconstructed by calling Start_RenderGraphConstruction()
		return false;
	}


	//Create static information
	Create_FrameGraphs();


#ifndef NO_IMGUI
	//If dear Imgui is created, apply subpass changes and render UI
	if (imgui) {
		//Initialize IMGUI if it isn't initialized
		if (IMGUI_Subpass) {
			VKOBJHANDLE handle = *(VKOBJHANDLE*)&IMGUI_Subpass;
			RenderGraph::SubDP_VK* subdp = getPass_fromHandle<RenderGraph::SubDP_VK>(handle);
			RenderGraph::DP_VK* maindp = subdp->getDP();
			RTSLOTSET_VKOBJ* SLOTSET = contentmanager->GETRTSLOTSET_ARRAY().getOBJbyINDEX(maindp->BASESLOTSET_ID);
			//If subdrawpass has more than one color slot, return false
			if (SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT != 1) {
				printer(result_tgfx_FAIL, "The Drawpass that's gonna render dear IMGUI should only have one color slot!");
				VKGLOBAL_RG->STATUS = RGReconstructionStatus::Invalid;	//User can delete a draw pass, dear Imgui fails in this case.
				//To avoid this, it needs to be reconstructed by calling Start_RenderGraphConstruction()
				return false;
			}

			if (imgui->Get_IMGUIStatus() == imgui_vk::IMGUI_STATUS::UNINITIALIZED) {
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
				if (vkCreateDescriptorPool(rendergpu->LOGICALDEVICE(), &descpool_ci, nullptr, &imgui->descpool) != VK_SUCCESS) {
					printer(result_tgfx_FAIL, "Creating a descriptor pool for dear IMGUI has failed!");
					return result_tgfx_FAIL;
				}

				if (!core_vk->GETWINDOWs().size()) {
					printer(result_tgfx_FAIL, "Initialization of dear IMGUI has failed because you didn't create a window!");
					return result_tgfx_FAIL;
				}
				ImGui_ImplGlfw_InitForVulkan(core_vk->GETWINDOWs()[0]->GLFW_WINDOW, true);
				ImGui_ImplVulkan_InitInfo init_info = {};
				init_info.Instance = Vulkan_Instance;
				init_info.PhysicalDevice = rendergpu->PHYSICALDEVICE();
				init_info.Device = rendergpu->LOGICALDEVICE();
				init_info.Queue = queuesys->get_queue(rendergpu, core_vk->GETWINDOWs()[0]->presentationqueue);
				init_info.QueueFamily = queuesys->get_queuefam_index(rendergpu, core_vk->GETWINDOWs()[0]->presentationqueue);
				init_info.PipelineCache = VK_NULL_HANDLE;
				init_info.DescriptorPool = imgui->descpool;
				init_info.Allocator = nullptr;
				init_info.MinImageCount = 2;
				init_info.ImageCount = 2;
				init_info.CheckVkResultFn = CheckIMGUIVKResults;
				init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;


				
				init_info.Subpass = static_cast<uint32_t>(subdp->VKRP_BINDINDEX);
				subdp->render_dearIMGUI = true;
				ImGui_ImplVulkan_Init(&init_info, maindp->RenderPassObject);

				imgui->UploadFontTextures();
			}
			//If there was a rendergraph before, just change the subdrawpass of the imgui
			else {
				subdp->render_dearIMGUI = true;
				imgui->Change_DrawPass(IMGUI_Subpass);
			}

			imgui->NewFrame();
		}
	}
#endif

	VKGLOBAL_RG->STATUS = RGReconstructionStatus::FinishConstructionCalled;
	RG_RECDATA::ID_GENERATOR.store(0);
	vk_virmem::free_dynamicmem(RG_RECDATA::VKGLOBAL_VIRMEM_RGRECDYNAMICMEM);
	return true;
}
//This function is defined in the FGAlgorithm.cpp
void Destroy_RenderGraph() {

}


//------------------------------------
// 
//		RENDERNODE HELPERS
// 
//------------------------------------


//WaitInfos is a pointer because function expects a list of waits (Color attachment output and also VertexShader-UniformReads etc)
static passwaitdescription_tgfx_handle CreatePassWait_SubDrawPass(drawpass_tgfx_handle* PassHandle, unsigned int SubDPIndex,
	waitsignaldescription_tgfx_handle* WaitInfos, unsigned char isLastFrame) {
	if (!PassHandle) { printer(result_tgfx_INVALIDARGUMENT, "CreatePassWait_SubDrawPass()'s first argument can't be nullptr!"); return nullptr; }
	TGFXLISTCOUNT(core_tgfx_main, WaitInfos, waitlistsize);
	if (waitlistsize) { printer(result_tgfx_NOTCODED, "waitsignaldescription isn't supported for now!"); }
	RG_RECDATA::WaitDesc_SubDP* waitdesc = (RG_RECDATA::WaitDesc_SubDP*)VK_ALLOCATE_AND_GETPTR(VKGLOBAL_VIRMEM_CURRENTFRAME, sizeof(RG_RECDATA::WaitDesc_SubDP));
	waitdesc->passhandle = PassHandle;
	waitdesc->SubPassIndex = SubDPIndex;
	waitdesc->isWaitingForLastFrame = isLastFrame;


	VKDATAHANDLE handle;
	handle.DATA_memoffset = VK_POINTER_TO_MEMOFFSET(waitdesc);
	handle.type = VKHANDLETYPEs::WAITDESC_SUBDP;
	return *(passwaitdescription_tgfx_handle*)&handle;
}
//WaitInfo is single, because function expects only one wait and it should be created with CreateWaitSignal_ComputeShader()
static passwaitdescription_tgfx_handle CreatePassWait_SubComputePass(computepass_tgfx_handle* PassHandle, unsigned int SubCPIndex,
	waitsignaldescription_tgfx_handle WaitInfo, unsigned char isLastFrame) {
	if (!PassHandle) { printer(result_tgfx_INVALIDARGUMENT, "CreatePassWait_SubComputePass()'s first argument can't be nullptr!"); return nullptr; }
	RG_RECDATA::WaitDesc_SubCP* waitdesc = (RG_RECDATA::WaitDesc_SubCP*)VK_ALLOCATE_AND_GETPTR(VKGLOBAL_VIRMEM_CURRENTFRAME, sizeof(RG_RECDATA::WaitDesc_SubCP));
	waitdesc->passhandle = PassHandle;
	waitdesc->SubPassIndex = SubCPIndex;
	waitdesc->isWaitingForLastFrame = isLastFrame;

	VKDATAHANDLE handle;
	handle.DATA_memoffset = VK_POINTER_TO_MEMOFFSET(waitdesc);
	handle.type = VKHANDLETYPEs::WAITDESC_SUBCP;
	return *(passwaitdescription_tgfx_handle*)&handle;
}
//WaitInfo is single, because function expects only one wait and it should be created with CreateWaitSignal_Transfer()
static passwaitdescription_tgfx_handle CreatePassWait_SubTransferPass(transferpass_tgfx_handle* PassHandle, unsigned int SubTPIndex,
	waitsignaldescription_tgfx_handle WaitInfo, unsigned char isLastFrame) {
	if (!PassHandle) { printer(result_tgfx_INVALIDARGUMENT, "CreatePassWait_SubTransferPass()'s first argument can't be nullptr!"); return nullptr; }
	RG_RECDATA::WaitDesc_SubTP* waitdesc = (RG_RECDATA::WaitDesc_SubTP*)VK_ALLOCATE_AND_GETPTR(VKGLOBAL_VIRMEM_CURRENTFRAME, sizeof(RG_RECDATA::WaitDesc_SubTP));
	waitdesc->passhandle = PassHandle;
	waitdesc->SubPassIndex = SubTPIndex;
	waitdesc->isWaitingForLastFrame = isLastFrame;
	
	VKDATAHANDLE handle;
	handle.DATA_memoffset = VK_POINTER_TO_MEMOFFSET(waitdesc);
	handle.type = VKHANDLETYPEs::WAITDESC_SUBTP;
	return *(passwaitdescription_tgfx_handle*)&handle;
}
//There is no option because you can only wait for a penultimate window pass
//I'd like to support last frame wait too but it confuses the users and it doesn't have much use
static passwaitdescription_tgfx_handle CreatePassWait_WindowPass(windowpass_tgfx_handle* PassHandle) {
	printer(result_tgfx_FAIL, "You can not wait for a window pass for now!");
	return nullptr;
}

static subdrawpassdescription_tgfx_handle CreateSubDrawPassDescription(passwaitdescription_tgfx_listhandle waitdescs, inheritedrtslotset_tgfx_handle irtslotset, subdrawpassaccess_tgfx WaitOP, subdrawpassaccess_tgfx ContinueOP) {
	RG_RECDATA::SubDPDESC* desc = (RG_RECDATA::SubDPDESC*)VK_ALLOCATE_AND_GETPTR(VKGLOBAL_VIRMEM_CURRENTFRAME, sizeof(RG_RECDATA::SubDPDESC));
	if (irtslotset) {
		desc->IRTSLOTSET_ID = contentmanager->GETIRTSLOTSET_ARRAY().getINDEXbyOBJ(contentmanager->GETIRTSLOTSET_ARRAY().getOBJfromHANDLE(*(VKOBJHANDLE*)&irtslotset));
	}
	else { desc->IRTSLOTSET_ID = UINT32_MAX; }
	desc->WAITOP = WaitOP;
	desc->CONTINUEOP = ContinueOP;
	desc->waits = waitdescs;

	VKDATAHANDLE handle = RG_RECDATA::getHandle_ofDesc(desc);
	return *(subdrawpassdescription_tgfx_handle*)&handle;
}
static subcomputepassdescription_tgfx_handle CreateSubComputePassDescription(passwaitdescription_tgfx_listhandle waitdescs) {
	RG_RECDATA::SubDPDESC* desc = (RG_RECDATA::SubDPDESC*)VK_ALLOCATE_AND_GETPTR(VKGLOBAL_VIRMEM_CURRENTFRAME, sizeof(RG_RECDATA::SubCPDESC));
	desc->waits = waitdescs;

	VKDATAHANDLE handle = RG_RECDATA::getHandle_ofDesc(desc);
	return *(subcomputepassdescription_tgfx_handle*)&handle;
}

void set_RGCreation_funcptrs() {
	core_tgfx_main->renderer->Start_RenderGraphConstruction = &Start_RenderGraphConstruction;
	core_tgfx_main->renderer->Finish_RenderGraphConstruction = &Finish_RenderGraphConstruction;
	core_tgfx_main->renderer->Destroy_RenderGraph = &Destroy_RenderGraph;

	core_tgfx_main->renderer->Create_TransferPass = &Create_TransferPass;
	core_tgfx_main->renderer->Create_DrawPass = &Create_DrawPass;
	core_tgfx_main->renderer->Create_WindowPass = &Create_WindowPass;
	core_tgfx_main->renderer->Create_ComputePass = &Create_ComputePass;

	core_tgfx_main->helpers->CreatePassWait_SubDrawPass = &CreatePassWait_SubDrawPass;
	core_tgfx_main->helpers->CreatePassWait_SubComputePass = &CreatePassWait_SubComputePass;
	core_tgfx_main->helpers->CreatePassWait_SubTransferPass = &CreatePassWait_SubTransferPass;
	core_tgfx_main->helpers->CreatePassWait_WindowPass = &CreatePassWait_WindowPass;
	core_tgfx_main->helpers->CreateSubDrawPassDescription = &CreateSubDrawPassDescription;
}