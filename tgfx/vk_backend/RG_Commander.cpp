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
#include "RG.h"


typedef struct renderer_private{
	RGReconstructionStatus RG_Status = RGReconstructionStatus::Invalid;
	VkDescriptorPool IMGUIPOOL = VK_NULL_HANDLE;
} renderer_private;
static renderer_private* hidden = nullptr;


			//RENDERGRAPH OPERATIONS


struct renderer_funcs {
	static void PrepareForNextFrame() {
		VKGLOBAL_FRAMEINDEX = (VKGLOBAL_FRAMEINDEX + 1) % VKCONST_BUFFERING_IN_FLIGHT;
		take_inputs();
		if(imgui){ imgui->NewFrame(); }
	}

	//Rendering operations
	static void Run() {
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
		VK_STATICVECTOR<WINDOW_VKOBJ, 50>& windows = core_vk->GETWINDOWs();
		for (unsigned char WindowIndex = 0; WindowIndex < windows.size(); WindowIndex++) {
			WINDOW_VKOBJ* VKWINDOW = windows[WindowIndex];

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
		unsigned int FirstIndex, unsigned int InstanceCount, unsigned int FirstInstance, rasterpipelineinstance_tgfx_handle MaterialInstance_ID,
		unsigned int DrawCallIndex, void* CallBufferSourceData, unsigned char CallBufferSourceCopySize, unsigned char CallBufferTargetOffset, subdrawpass_tgfx_handle SubDrawPass_ID) {
		VKOBJHANDLE sphandle = *(VKOBJHANDLE*)SubDrawPass_ID;
		RenderGraph::SubDP_VK* SP = getPass_fromHandle<RenderGraph::SubDP_VK>(sphandle);
		VKOBJHANDLE ib_handle = *(VKOBJHANDLE*)&IndexBuffer_ID, vb_handle = *(VKOBJHANDLE*)&VertexBuffer_ID;
		INDEXBUFFER_VKOBJ* IB = contentmanager->GETIB_ARRAY().getOBJfromHANDLE(ib_handle);
		VERTEXBUFFER_VKOBJ* VB = contentmanager->GETVB_ARRAY().getOBJfromHANDLE(vb_handle);
		GRAPHICSPIPELINEINST_VKOBJ* PI = (GRAPHICSPIPELINEINST_VKOBJ*)MaterialInstance_ID;
		GRAPHICSPIPELINETYPE_VKOBJ* MATTYPE = contentmanager->GETGRAPHICSPIPETYPE_ARRAY().getOBJbyINDEX(PI->PROGRAM);

		if (IB) {
			RenderGraph::indexeddrawcall_vk call;
			if (VB) {
				call.VBOffset = VB->Block.Offset;
				call.VBuffer = gpu_allocator->get_memorybufferhandle_byID(rendergpu, VB->Block.MemAllocIndex);
			} 
			else {
				call.VBuffer = VK_NULL_HANDLE;
			}
			call.IBuffer = gpu_allocator->get_memorybufferhandle_byID(rendergpu, IB->Block.MemAllocIndex);
			call.IBOffset = IB->Block.Offset;
			call.IType = ((INDEXBUFFER_VKOBJ*)IndexBuffer_ID)->DATATYPE;
			if (Count) {
				call.IndexCount = Count;
			}
			else {
				call.IndexCount = ((INDEXBUFFER_VKOBJ*)IndexBuffer_ID)->IndexCount;
			}
			call.FirstIndex = FirstIndex;
			call.VOffset = VertexOffset;
			call.base.MatTypeObj = MATTYPE->PipelineObject;
			call.base.MatTypeLayout = MATTYPE->PipelineLayout;
			for (uint32_t i = 0; i < VKCONST_MAXDESCSET_PERLIST; i++) {
				call.base.TypeSETs[i] = MATTYPE->TypeSETs[i];
				call.base.InstanceSETs[i] = PI->InstSETs[i];
			}
			call.FirstInstance = FirstInstance;
			call.InstanceCount = InstanceCount;
			SP->IndexedDrawCalls.push_back(call);
		}
		else {
			RenderGraph::nonindexeddrawcall_vk call;
			if (VertexBuffer_ID) {
				FindBufferOBJ_byBufType(VertexBuffer_ID, call.VBuffer, call.VOffset);
			}
			else {
				call.VBuffer = VK_NULL_HANDLE;
			}
			if (Count) {
				call.VertexCount = Count;
			}
			else {
				call.VertexCount = static_cast<VkDeviceSize>(VB->VERTEX_COUNT);
			}
			call.FirstVertex = VertexOffset;
			call.FirstInstance = FirstInstance;
			call.InstanceCount = InstanceCount;
			call.base.MatTypeObj = MATTYPE->PipelineObject;
			call.base.MatTypeLayout = MATTYPE->PipelineLayout;
			SP->NonIndexedDrawCalls.push_back(call);
		}
	}

	static void SwapBuffers(window_tgfx_handle WindowHandle, windowpass_tgfx_handle WindowPassHandle) {
		WINDOW_VKOBJ* VKWINDOW = (WINDOW_VKOBJ*)WindowHandle;
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

		VKOBJHANDLE wphandle = *(VKOBJHANDLE*) & WindowPassHandle;
		RenderGraph::WP_VK* wp = getPass_fromHandle<RenderGraph::WP_VK>(wphandle);
		RenderGraph::windowcall_vk call;
		call.WindowID = core_vk->GETWINDOWs().getINDEX_byOBJ(VKWINDOW);
		wp->WindowCalls.push_back(call);
	}
	
	static void FindBufferOBJ_byBufType(buffer_tgfx_handle Handle, VkBuffer& TargetBuffer, VkDeviceSize& TargetOffset) {
		VKOBJHANDLE objhandle = *(VKOBJHANDLE*)&Handle;
		switch (objhandle.type) {
		case VKHANDLETYPEs::GLOBALSSBO:
		{
			GLOBALSSBO_VKOBJ* obj = contentmanager->GETGLOBALSSBO_ARRAY().getOBJfromHANDLE(objhandle);
			TargetBuffer = gpu_allocator->get_memorybufferhandle_byID(rendergpu, obj->Block.MemAllocIndex);
			TargetOffset += obj->Block.Offset;
		}
		break;
		case VKHANDLETYPEs::VERTEXBUFFER:
		{
			VERTEXBUFFER_VKOBJ* obj = contentmanager->GETVB_ARRAY().getOBJfromHANDLE(objhandle);
			TargetBuffer = gpu_allocator->get_memorybufferhandle_byID(rendergpu, obj->Block.MemAllocIndex);
			TargetOffset += obj->Block.Offset;
		}
		break;
		case VKHANDLETYPEs::INDEXBUFFER:
		{
			INDEXBUFFER_VKOBJ* IB = contentmanager->GETIB_ARRAY().getOBJfromHANDLE(objhandle);
			TargetBuffer = gpu_allocator->get_memorybufferhandle_byID(rendergpu, IB->Block.MemAllocIndex);
			TargetOffset += IB->Block.Offset;
		}
		break;
		default:
			printer(result_tgfx_FAIL, "FindBufferOBJ_byBufType() doesn't support this type of buffer!");
		}
	}

	//Source Buffer should be created with HOSTVISIBLE or FASTHOSTVISIBLE
	//Target Buffer should be created with DEVICELOCAL
	static void CopyBuffer_toBuffer(subtransferpass_tgfx_handle TransferPassHandle, buffer_tgfx_handle SourceBuffer_Handle,
		buffer_tgfx_handle TargetBuffer_Handle, unsigned int SourceBuffer_Offset, unsigned int TargetBuffer_Offset, unsigned int Size) {
		VkBuffer SourceBuffer, DistanceBuffer;
		VkDeviceSize SourceOffset = static_cast<VkDeviceSize>(SourceBuffer_Offset), DistanceOffset = static_cast<VkDeviceSize>(TargetBuffer_Offset);
		FindBufferOBJ_byBufType(SourceBuffer_Handle, SourceBuffer, SourceOffset);
		FindBufferOBJ_byBufType(TargetBuffer_Handle, DistanceBuffer, DistanceOffset);
		VkBufferCopy Copy_i = {};
		Copy_i.dstOffset = DistanceOffset;
		Copy_i.srcOffset = SourceOffset;
		Copy_i.size = static_cast<VkDeviceSize>(Size);

		VKOBJHANDLE subtphandle = *(VKOBJHANDLE*)&TransferPassHandle;
		RenderGraph::SubTPCOPY_VK* subtp = getPass_fromHandle<RenderGraph::SubTPCOPY_VK>(subtphandle);
		RenderGraph::VK_BUFtoBUFinfo finalinfo;
		finalinfo.DistanceBuffer = DistanceBuffer;
		finalinfo.SourceBuffer = SourceBuffer;
		finalinfo.info = Copy_i;
		subtp->BUFBUFCopies.push_back(finalinfo);
	}

	//Source Buffer should be created with HOSTVISIBLE or FASTHOSTVISIBLE
	static void CopyBuffer_toImage(subtransferpass_tgfx_handle TransferPassHandle, buffer_tgfx_handle SourceBuffer_Handle, texture_tgfx_handle TextureHandle,
		unsigned int SourceBuffer_offset, boxregion_tgfx TargetTextureRegion, unsigned int TargetMipLevel,
		cubeface_tgfx TargetCubeMapFace) {
		TEXTURE_VKOBJ* TEXTURE = (TEXTURE_VKOBJ*)TextureHandle;
		VkDeviceSize finaloffset = static_cast<VkDeviceSize>(SourceBuffer_offset);
		RenderGraph::VK_BUFtoIMinfo x;
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
		FindBufferOBJ_byBufType(SourceBuffer_Handle, x.SourceBuffer, finaloffset);
		x.BufferImageCopy.bufferOffset = finaloffset;

		VKOBJHANDLE subTPhandle = *(VKOBJHANDLE*)&TransferPassHandle;
		RenderGraph::SubTPCOPY_VK* subTP = getPass_fromHandle<RenderGraph::SubTPCOPY_VK>(subTPhandle);
		subTP->BUFIMCopies.push_back(x);
	}

	static void CopyImage_toImage(subtransferpass_tgfx_handle TransferPassHandle, texture_tgfx_handle SourceTextureHandle, texture_tgfx_handle TargetTextureHandle,
		uvec3_tgfx SourceTextureOffset, uvec3_tgfx CopySize, uvec3_tgfx TargetTextureOffset, unsigned int SourceMipLevel, unsigned int TargetMipLevel,
		cubeface_tgfx SourceCubeMapFace, cubeface_tgfx TargetCubeMapFace);

	static void ImageBarrier(subtransferpass_tgfx_handle BarrierTPHandle, texture_tgfx_handle TextureHandle, image_access_tgfx LAST_ACCESS,
		image_access_tgfx NEXT_ACCESS, unsigned int TargetMipLevel, cubeface_tgfx TargetCubeMapFace) {
		TEXTURE_VKOBJ* Texture = (TEXTURE_VKOBJ*)TextureHandle;
		VKOBJHANDLE subTPhandle = *(VKOBJHANDLE*)&BarrierTPHandle;
		RenderGraph::SubTPBARRIER_VK* tp = getPass_fromHandle<RenderGraph::SubTPBARRIER_VK>(subTPhandle);


		RenderGraph::VK_ImBarrierInfo im_bi;
		im_bi.image = Texture->Image;
		im_bi.lastaccess = LAST_ACCESS;
		im_bi.nextaccess = NEXT_ACCESS;
		if (Texture->CHANNELs == texture_channels_tgfx_D24S8) {
			im_bi.isDepthBit = true; im_bi.isStencilBit = true;
		}
		else if (Texture->CHANNELs == texture_channels_tgfx_D32) {
			im_bi.isDepthBit = true;
		}
		else {
			im_bi.isColorBit = true;
		}
		if (Texture->DIMENSION == texture_dimensions_tgfx_2DCUBE) {
			im_bi.cubemapface = TargetCubeMapFace;
		}
		else {
			im_bi.cubemapface = cubeface_tgfx_FRONT;
		}
		if (TargetMipLevel == UINT32_MAX) {
			im_bi.targetmip_c = Texture->MIPCOUNT;
			im_bi.targetmip_i = 0;
		}
		else {
			im_bi.targetmip_i = TargetMipLevel;
			im_bi.targetmip_c = 1;
		}


		tp->TextureBarriers.push_back(im_bi);
	}

	static void Dispatch_Compute(subcomputepass_tgfx_handle ComputePassHandle, computeshaderinstance_tgfx_handle CSInstanceHandle, uvec3_tgfx DispatchSize) {
		VKOBJHANDLE CPhandle = *(VKOBJHANDLE*)&ComputePassHandle;
		RenderGraph::SubCP_VK* subCP = getPass_fromHandle<RenderGraph::SubCP_VK>(CPhandle);

		RenderGraph::dispatchcall_vk dc;
		dc.DispatchSize = glm::vec3(DispatchSize.x, DispatchSize.y, DispatchSize.z);
		COMPUTEINST_VKOBJ* ci = contentmanager->GETCOMPUTEINST_ARRAY().getOBJfromHANDLE(*(VKOBJHANDLE*)CSInstanceHandle);
		COMPUTETYPE_VKOBJ* pso = contentmanager->GETCOMPUTETYPE_ARRAY().getOBJbyINDEX(ci->PROGRAM);
		uint32_t current_i = 0;
		for (uint32_t i = 0; i < VKCONST_MAXDESCSET_PERLIST && ci->InstSETs[i] != UINT32_MAX; i++) { dc.BINDINGTABLEIDs[current_i++] = ci->InstSETs[i]; }
		for (uint32_t j = 0; j < VKCONST_MAXDESCSET_PERLIST && ci->InstSETs[j] != UINT32_MAX; j++) { dc.BINDINGTABLEIDs[current_i++] = ci->InstSETs[j]; }
		dc.Layout = pso->PipelineLayout;
		dc.Pipeline = pso->PipelineObject;
		subCP->Dispatches.push_back(dc);
	}
};

void renderer_public::RendererResource_Finalizations() {
	//Handle RTSlotSet changes by recreating framebuffers of draw passes
	//by "RTSlotSet changes": DrawPass' slotset is changed to different one or slotset's slots is changed.
	RenderGraph::DP_VK* DP = (RenderGraph::DP_VK*)VKGLOBAL_RG->GetNextPass(nullptr, VK_PASSTYPE::DP);
	while (DP) {
		unsigned char ChangeInfo = DP->SlotSetChanged.load();
		RTSLOTSET_VKOBJ* slotset = contentmanager->GETRTSLOTSET_ARRAY().getOBJbyINDEX(DP->BASESLOTSET_ID);
		if (ChangeInfo == VKGLOBAL_FRAMEINDEX + 1 || ChangeInfo == 3 || slotset->PERFRAME_SLOTSETs[VKGLOBAL_FRAMEINDEX].IsChanged.load()) {
			if (DP->FBs[VKGLOBAL_FRAMEINDEX]) {
				//This is safe because this FB is used 2 frames ago and CPU already waits for the frame's GPU process to end
				vkDestroyFramebuffer(rendergpu->LOGICALDEVICE(), DP->FBs[VKGLOBAL_FRAMEINDEX], nullptr);
			}

			VkFramebufferCreateInfo fb_ci = slotset->FB_ci[VKGLOBAL_FRAMEINDEX];
			fb_ci.renderPass = DP->RenderPassObject;
			if (vkCreateFramebuffer(rendergpu->LOGICALDEVICE(), &fb_ci, nullptr, &DP->FBs[VKGLOBAL_FRAMEINDEX]) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "vkCreateFramebuffer() has failed while changing one of the drawpasses' current frame slot's texture! Please report this!");
				return;
			}

			slotset->PERFRAME_SLOTSETs[VKGLOBAL_FRAMEINDEX].IsChanged.store(false);
			for (unsigned int SlotIndex = 0; SlotIndex < slotset->PERFRAME_SLOTSETs[VKGLOBAL_FRAMEINDEX].COLORSLOTs_COUNT; SlotIndex++) {
				slotset->PERFRAME_SLOTSETs[VKGLOBAL_FRAMEINDEX].COLOR_SLOTs[SlotIndex].IsChanged.store(false);
			}

			if (ChangeInfo) {
				DP->SlotSetChanged.store(ChangeInfo - VKGLOBAL_FRAMEINDEX - 1);
			}
		}
	}
}
unsigned char GetCurrentFrameIndex() {return renderer->Get_FrameIndex(false);}
extern void set_RGCreation_funcptrs();
void set_rendersysptrs() {
	core_tgfx_main->renderer->Run = &renderer_funcs::Run;
	core_tgfx_main->renderer->ImageBarrier = &renderer_funcs::ImageBarrier;
	core_tgfx_main->renderer->SwapBuffers = &renderer_funcs::SwapBuffers;
	core_tgfx_main->renderer->CopyBuffer_toBuffer = &renderer_funcs::CopyBuffer_toBuffer;
	core_tgfx_main->renderer->DrawDirect = &renderer_funcs::DrawDirect;
	core_tgfx_main->renderer->CopyBuffer_toImage = &renderer_funcs::CopyBuffer_toImage;
	core_tgfx_main->renderer->GetCurrentFrameIndex = &GetCurrentFrameIndex;
	core_tgfx_main->renderer->Dispatch_Compute = &renderer_funcs::Dispatch_Compute;
}

static constexpr unsigned char VKCONST_VIRMEM_RENDERERALLOCATION_PAGECOUNT = 1;
extern void Create_Renderer() {
	//Allocate a page for renderer's data
	uintptr_t allocation_ptr = uintptr_t(VKCONST_VIRMEMSPACE_BEGIN) + vk_virmem::allocate_page(1);
	renderer = (renderer_public*)allocation_ptr;
	hidden = (renderer_private*)(allocation_ptr + sizeof(renderer_public));

	set_rendersysptrs();
}

bool RenderGraph::SubTPCOPY_VK::isWorkloaded() {
	if (!BUFBUFCopies.size() && !BUFIMCopies.size() && !IMIMCopies.size()) { return false; }
	return true;
}
bool RenderGraph::SubTPBARRIER_VK::isWorkloaded() {
	if (!this->BufferBarriers.size() && !this->TextureBarriers.size()) { return false; }
	return true;
}
bool RenderGraph::CP_VK::isWorkloaded() {
	SubCP_VK* subpasses = (SubCP_VK*)(uintptr_t(this) + sizeof(CP_VK));
	for (unsigned char SubpassIndex = 0; SubpassIndex < SubPassesCount; SubpassIndex++) {
		SubCP_VK& SP = subpasses[SubpassIndex];
		if (SP.isThereWorkload()) {
			return true;
		}
	}
	return false;
}
bool RenderGraph::WP_VK::isWorkloaded() {
	if (WindowCalls.size() != UINT32_MAX) { return true; }
	return false;
}
bool RenderGraph::DP_VK::isWorkloaded() {
	SubDP_VK* subpasses = (SubDP_VK*)(uintptr_t(this) + sizeof(DP_VK));
	for (unsigned char SubpassIndex = 0; SubpassIndex < ALLSubDPsCOUNT; SubpassIndex++) {
		SubDP_VK& SP = subpasses[SubpassIndex];
		if (SP.isThereWorkload()) {
			return true;
		}
	}
	return false;
}
bool RenderGraph::SubDP_VK::isThereWorkload() {
	if (render_dearIMGUI) { return true; }
	if (!IndexedDrawCalls.size() && !NonIndexedDrawCalls.size()) { return false; }
	return true;
}
bool RenderGraph::SubCP_VK::isThereWorkload() {
	if (!Dispatches.size()) { return false; }
	return true;
}