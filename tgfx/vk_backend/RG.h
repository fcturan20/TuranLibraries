#pragma once
//Include this header for .cpps that will change framegraph
//For example: renderer.cpp will add passes and calls, rg_***.cpp will reconstruct RG or record CBs etc
//So don't include this in contentmanager.cpp or core.cpp etc.
#include "predefinitions_vk.h"
#include <stdint.h>
#include "renderer.h"


//Because RG can be destroyed, it's better be an independent allocation
struct RenderGraph {
	//This is needed for each draw call
	struct drawcallbase_vk {
		VkPipeline MatTypeObj;
		VkPipelineLayout MatTypeLayout;
		uint32_t SETs[VKCONST_MAXDESCSET_PERLIST * 2];
	};
	struct nonindexeddrawcall_vk {
		VkBuffer VBuffer;
		VkDeviceSize VOffset = 0;
		uint32_t FirstVertex = 0, VertexCount = 0, FirstInstance = 0, InstanceCount = 0;

		drawcallbase_vk base;
	};
	struct indexeddrawcall_vk {
		VkBuffer VBuffer, IBuffer;
		VkDeviceSize VBOffset = 0, IBOffset = 0;
		uint32_t VOffset = 0, FirstIndex = 0, IndexCount = 0, FirstInstance = 0, InstanceCount = 0;
		VkIndexType IType;

		drawcallbase_vk base;
	};
	struct DP_VK;
	struct SubDP_VK {
		uint32_t DP_ID : 16;
		uint32_t IRTSLOTSET_ID : 16;
		bool render_dearIMGUI : 1;
		bool is_RENDERPASSRELATED : 1;
		subdrawpassaccess_tgfx waitop : 8, continueop : 8;
		uint16_t VKRP_BINDINDEX, RG_BINDINDEX;	//RG bindindex is the id in all subdps of the same dp, vkrp_bindindex is the id in VkRenderpass object
		//These are to define which subpass is waited for the merged subpass
		uint16_t Waitfor_MergedSubCP_withIndex = 0, WaitFor_MergedSubTP_withIndex = 0;
		//These will be virtual memory allocated structs
		VK_VECTOR_ADDONLY<nonindexeddrawcall_vk, 1 << 16> NonIndexedDrawCalls;
		VK_VECTOR_ADDONLY<indexeddrawcall_vk, 1 << 16> IndexedDrawCalls;
		SubDP_VK() : render_dearIMGUI(false) {}
		DP_VK* getDP();
		bool isThereWorkload();
		VKOBJHANDLE getHANDLE() {
			VKOBJHANDLE handle;
			handle.type = VKHANDLETYPEs::SUBDP;
			handle.OBJ_memoffset = VK_POINTER_TO_MEMOFFSET(this);
			handle.EXTRA_FLAGs = RG_BINDINDEX;
			return handle;
		}
		static SubDP_VK* getPassFromHandle(VKOBJHANDLE handle) {
			if (handle.type != VKHANDLETYPEs::SUBDP) { printer(14); return nullptr; }
			return (SubDP_VK*)VK_MEMOFFSET_TO_POINTER(handle.OBJ_memoffset);
		}
	};
	//SubDPs list will be allocated right after the DP_VK's memory location
	//There are 2 different types of subdps: VkRenderPass related and BarrierOnly related
	//VkRenderPass related: Subpass defines a valid sync point for Tile-Based renderer drivers in Vulkan
	//	so they need to define how they access to rtslotset with irtslotset
	//BarrierOnly related: Subpass doesn't define the thing above but subpass only waits for some operations of other passes
	struct DP_VK {
		VK_PASSTYPE PASSTYPE = VK_PASSTYPE::DP;
		VK_PASS_ID_TYPE PASS_ID = VK_PASS_INVALID_ID;
		
		boxregion_tgfx RenderRegion;
		uint32_t BASESLOTSET_ID;
		uint32_t ALLSubDPsCOUNT, BO_SubDPsCOUNT;
		std::atomic<unsigned char> SlotSetChanged = true;


		VkFramebuffer FBs[VKCONST_BUFFERING_IN_FLIGHT]{ VK_NULL_HANDLE };
		VkRenderPass RenderPassObject;

		union {
			passwaitdescription_tgfx_listhandle passwaits; //This is used while reconstructing
			uint32_t PassWaitsPTR;	//This is used after reconstructing
		};
		SubDP_VK* getSUBDPs() { return (SubDP_VK*)(uintptr_t(this) + sizeof(DP_VK)); }

		bool isWorkloaded();
		VKOBJHANDLE getHANDLE() {
			VKOBJHANDLE handle;
			handle.type = VKHANDLETYPEs::DRAWPASS;
			handle.OBJ_memoffset = VK_POINTER_TO_MEMOFFSET(this);
			handle.EXTRA_FLAGs = PASS_ID;
			return handle;
		}
		static DP_VK* getPassFromHandle(VKOBJHANDLE handle) {
			if (handle.type != VKHANDLETYPEs::DRAWPASS) { printer(14); return nullptr; }
			return (DP_VK*)VK_MEMOFFSET_TO_POINTER(handle.OBJ_memoffset);
		}
	};

	//TRANSFER PASS

	struct VK_BUFtoIMinfo {
		VkImage TargetImage;
		VkBuffer SourceBuffer;
		VkBufferImageCopy BufferImageCopy;
	};
	struct VK_BUFtoBUFinfo {
		VkBuffer SourceBuffer, DistanceBuffer;
		VkBufferCopy info;
	};
	struct VK_IMtoIMinfo {
		VkImage SourceTexture, TargetTexture;
		VkImageCopy info;
	};
	

	struct VK_ImBarrierInfo {
		cubeface_tgfx cubemapface : 3;
		uint32_t targetmip_i: 5, targetmip_c : 5;
		image_access_tgfx lastaccess : 5, nextaccess : 5;
		bool isColorBit : 1, isDepthBit : 1, isStencilBit : 1;
		VkImage image;
	};
	struct VK_BufBarrierInfo {
		uint32_t offset : 27, lastaccess : 5, size : 27, nextaccess : 5;
		VkBuffer buf;
	};

	struct TP_VK {
		VK_PASSTYPE PASS_TYPE = VK_PASSTYPE::TP;
		VK_PASS_ID_TYPE PASS_ID = VK_PASS_INVALID_ID;
		uint16_t SubPassesCount = UINT16_MAX, Merged_DP_ID = UINT16_MAX, Merged_CP_ID = UINT16_MAX;
		uint32_t SubPassesList_SizeInBytes;
		union {
			passwaitdescription_tgfx_listhandle passwaits = nullptr; //This is used while reconstructing
			uint32_t PassWaitsPTR;	//This is used after reconstructing
		};
		VKOBJHANDLE getHANDLE() {
			VKOBJHANDLE handle;
			handle.type = VKHANDLETYPEs::TRANSFERPASS;
			handle.OBJ_memoffset = VK_POINTER_TO_MEMOFFSET(this);
			handle.EXTRA_FLAGs = PASS_ID;
			return handle;
		}
		static TP_VK* getPassFromHandle(VKOBJHANDLE handle) {
			if (handle.type != VKHANDLETYPEs::TRANSFERPASS) { printer(14); return nullptr; }
			return (TP_VK*)VK_MEMOFFSET_TO_POINTER(handle.OBJ_memoffset);
		}
	};
	struct SubTPCOPY_VK {
		VK_VECTOR_ADDONLY<VK_BUFtoIMinfo, 1 << 16> BUFIMCopies;
		VK_VECTOR_ADDONLY<VK_BUFtoBUFinfo, 1 << 16> BUFBUFCopies;
		VK_VECTOR_ADDONLY<VK_IMtoIMinfo, 1 << 16> IMIMCopies;
		uint16_t SubTP_ID = 0;		//Index of this subpass
		uint16_t SubDP_index = UINT16_MAX, SubCP_index = UINT16_MAX;	//Merged Pass
		bool isWorkloaded();
		VKOBJHANDLE getHANDLE() {
			VKOBJHANDLE handle;
			handle.type = VKHANDLETYPEs::SUBTP_COPY;
			handle.OBJ_memoffset = VK_POINTER_TO_MEMOFFSET(this);
			handle.EXTRA_FLAGs = SubTP_ID;
			return handle;
		}
		static SubTPCOPY_VK* getPassFromHandle(VKOBJHANDLE handle) {
			if (handle.type != VKHANDLETYPEs::SUBTP_COPY) { printer(14); return nullptr; }
			return (SubTPCOPY_VK*)VK_MEMOFFSET_TO_POINTER(handle.OBJ_memoffset);
		}
	};
	struct SubTPBARRIER_VK {
		VK_VECTOR_ADDONLY<VK_ImBarrierInfo, 1 << 16> TextureBarriers;
		VK_VECTOR_ADDONLY<VK_BufBarrierInfo, 1 << 16> BufferBarriers;
		uint16_t SubTP_ID = 0;		//Index of this subpass
		uint16_t SubDP_index = UINT16_MAX, SubCP_index = UINT16_MAX;	//Merged Pass
		bool isWorkloaded();
		VKOBJHANDLE getHANDLE() {
			VKOBJHANDLE handle;
			handle.type = VKHANDLETYPEs::SUBTP_BARRIER;
			handle.OBJ_memoffset = VK_POINTER_TO_MEMOFFSET(this);
			handle.EXTRA_FLAGs = SubTP_ID;
			return handle;
		}
		static SubTPBARRIER_VK* getPassFromHandle(VKOBJHANDLE handle) {
			if (handle.type != VKHANDLETYPEs::SUBTP_BARRIER) { printer(14); return nullptr; }
			return (SubTPBARRIER_VK*)VK_MEMOFFSET_TO_POINTER(handle.OBJ_memoffset);
		}
	};

	struct windowcall_vk {
		uint32_t WindowID = UINT32_MAX;
	};
	struct WP_VK {
		VK_PASSTYPE PASSTYPE = VK_PASSTYPE::WP;
		VK_PASS_ID_TYPE PASS_ID = VK_PASS_INVALID_ID;
		VK_VECTOR_ADDONLY<windowcall_vk, 256> WindowCalls;
		union {
			passwaitdescription_tgfx_listhandle passwaits = nullptr; //This is used while reconstructing
			uint32_t PassWaitsPTR;	//This is used after reconstructing
		};

		bool isWorkloaded();
		VKOBJHANDLE getHANDLE() {
			VKOBJHANDLE handle;
			handle.type = VKHANDLETYPEs::WINDOWPASS;
			handle.OBJ_memoffset = VK_POINTER_TO_MEMOFFSET(this);
			handle.EXTRA_FLAGs = PASS_ID;
			return handle;
		}
		static WP_VK* getPassFromHandle(VKOBJHANDLE handle) {
			if (handle.type != VKHANDLETYPEs::WINDOWPASS) { printer(14); return nullptr; }
			return (WP_VK*)VK_MEMOFFSET_TO_POINTER(handle.OBJ_memoffset);
		}
	};
	struct dispatchcall_vk {
		VkPipelineLayout Layout = VK_NULL_HANDLE;
		VkPipeline Pipeline = VK_NULL_HANDLE;
		uint16_t BINDINGTABLEIDs[VKCONST_MAXDESCSET_PERLIST * 2];
		glm::uvec3 DispatchSize = glm::uvec3(0);
		dispatchcall_vk() { for (uint32_t i = 0; i < VKCONST_MAXDESCSET_PERLIST * 2; i++) { BINDINGTABLEIDs[i] = UINT16_MAX; } }
	};
	static constexpr uint32_t sizesdf = sizeof(dispatchcall_vk);

	struct SubCP_VK {
		VK_VECTOR_ADDONLY<dispatchcall_vk, 1 << 10> Dispatches;
		uint16_t SubCP_ID = 0;
		uint16_t SubDP_index = UINT16_MAX, SubTP_index = UINT16_MAX;
		VKOBJHANDLE getHANDLE() {
			VKOBJHANDLE handle;
			handle.type = VKHANDLETYPEs::SUBCP;
			handle.OBJ_memoffset = VK_POINTER_TO_MEMOFFSET(this);
			handle.EXTRA_FLAGs = SubCP_ID;
			return handle;
		}
		static SubCP_VK* getPassFromHandle(VKOBJHANDLE handle) {
			if (handle.type != VKHANDLETYPEs::SUBCP) { printer(14); return nullptr; }
			return (SubCP_VK*)VK_MEMOFFSET_TO_POINTER(handle.OBJ_memoffset);
		}
		bool isThereWorkload();
	};

	struct CP_VK {
		VK_PASSTYPE PASSTYPE;
		VK_PASS_ID_TYPE PASS_ID;
		uint16_t SubPassesCount;

		union {
			passwaitdescription_tgfx_listhandle passwaits; //This is used while reconstructing
			uint32_t PassWaitsPTR;	//This is used after reconstructing
		};

		CP_VK() : PASS_ID(VK_PASS_INVALID_ID), PASSTYPE(VK_PASSTYPE::CP) {}
		SubCP_VK* GetSubpasses() { return (SubCP_VK*)((uintptr_t)this + sizeof(CP_VK)); }
		VKOBJHANDLE getHANDLE() {
			VKOBJHANDLE handle;
			handle.type = VKHANDLETYPEs::COMPUTEPASS;
			handle.OBJ_memoffset = VK_POINTER_TO_MEMOFFSET(this);
			handle.EXTRA_FLAGs = PASS_ID;
			return handle;
		}
		static CP_VK* getPassFromHandle(VKOBJHANDLE handle) {
			if (handle.type != VKHANDLETYPEs::COMPUTEPASS) { printer(14); return nullptr; }
			return (CP_VK*)VK_MEMOFFSET_TO_POINTER(handle.OBJ_memoffset);
		}
		bool isWorkloaded();
	};
	static constexpr uint32_t szizdzf = sizeof(CP_VK);

	//4MB is enough for all pass data, i guess?
	static constexpr unsigned int VKCONST_VIRMEM_RGALLOCATION_PAGECOUNT = 2000;
	//Last 4 pages are given to PASSBASES_PTR
	uint32_t PASSES_PTR = UINT32_MAX;
	static uint32_t GetSize_ofType(VK_PASSTYPE type, void* pass) {
		switch (type) {
		case VK_PASSTYPE::DP:
		{	DP_VK* dp = (DP_VK*)pass; return sizeof(DP_VK) + (dp->ALLSubDPsCOUNT * sizeof(SubDP_VK)); }
		break;
		case VK_PASSTYPE::CP:
		{	CP_VK* cp = (CP_VK*)pass; return sizeof(CP_VK) + (cp->SubPassesCount * sizeof(SubCP_VK)); }
		break;
		case VK_PASSTYPE::TP:
		{	TP_VK* tp = (TP_VK*)pass; return sizeof(TP_VK) + tp->SubPassesList_SizeInBytes; }
		break;
		case VK_PASSTYPE::WP:
		{	WP_VK* wp = (WP_VK*)pass; return sizeof(WP_VK); }
		default:
			printer(result_tgfx_FAIL, "Pass is not supported by the GetSize_ofType(), there is a possible memory bug!");
			return UINT32_MAX;
		}
	}
	void* GetNextPass(void* currentPass, VK_PASSTYPE type) {
		uint32_t current = PASSES_PTR;
		if (currentPass) {
			VK_PASSTYPE type = *(VK_PASSTYPE*)(currentPass);
			current = VK_POINTER_TO_MEMOFFSET(currentPass);
		}
		
#ifdef VULKAN_DEBUGGING
		if (current < PASSES_PTR || current > PASSES_PTR + (VKCONST_VIRMEM_RGALLOCATION_PAGECOUNT * VKCONST_VIRMEMPAGESIZE)) { printer(result_tgfx_FAIL, "Passes current ptr is wrong, please report this!"); }
#endif}
		while (current < PASSES_PTR + (VKCONST_VIRMEM_RGALLOCATION_PAGECOUNT * VKCONST_VIRMEMPAGESIZE)) {
			VK_PASSTYPE* base = (VK_PASSTYPE*)VK_MEMOFFSET_TO_POINTER(current);
			if (*base == type) { return base; }
			current += GetSize_ofType(*base, base);
		}
	}
	//This stores reconstruction status, even though old valid RG isn't changed after new one is started reconstruction
	RGReconstructionStatus STATUS = RGReconstructionStatus::Invalid;
};
template<class T>
bool isWorkloaded(T* pass) {
	return pass->isWorkloaded();
}
template<class T>
VKOBJHANDLE getHandle_ofpass(T* pass) {
	return pass->getHANDLE();
}
template<class T>
T* getPass_fromHandle(VKOBJHANDLE handle) {
	return T::getPassFromHandle(handle);
}
extern RenderGraph* VKGLOBAL_RG;





//RenderGraph has draw-compute-transfer calls to execute and display calls to display on windows
//So this function handles everything related to these calls
//If returns false, this means nothing is rendered this frame 
//Implemented in rendergraph_main.cpp 
result_tgfx Execute_RenderGraph();
void WaitForRenderGraphCommandBuffers();
void take_inputs();
void Create_FrameGraphs();