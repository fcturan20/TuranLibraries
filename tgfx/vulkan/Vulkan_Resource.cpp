#include "Vulkan_Resource.h"
#include "Vulkan_Renderer_Core.h"
#include "Vulkan_Core.h"
#define LOG VKCORE->TGFXCORE.DebugCallback

VK_Pass::VK_Pass(const std::string& name, PassType type, unsigned int WAITSCOUNT) : NAME(name), TYPE(type), WAITsCOUNT(WAITSCOUNT), WAITs(new VK_PassWaitDescription[WAITSCOUNT]) {

}
VK_DrawPass::VK_DrawPass(const std::string& name, unsigned int WAITSCOUNT) : VK_Pass(name, PassType::DP, WAITSCOUNT) {

}
VK_ComputePass::VK_ComputePass(const std::string& name, unsigned int WAITSCOUNT) : VK_Pass(name, PassType::CP, WAITSCOUNT) {

}
VK_WindowPass::VK_WindowPass(const std::string& name, unsigned int WAITSCOUNT) : VK_Pass(name, PassType::WP, WAITSCOUNT) {

}
VK_TransferPass::VK_TransferPass(const char* name, unsigned int WAITSCOUNT) : VK_Pass(name, PassType::TP, WAITSCOUNT) {

}


void Find_AccessPattern_byIMAGEACCESS(const tgfx_imageaccess& Access, VkAccessFlags& TargetAccessFlag, VkImageLayout& TargetImageLayout) {
	switch (Access)
	{
	case tgfx_imageaccess_DEPTHSTENCIL_READONLY:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
		return;
	case tgfx_imageaccess_DEPTHSTENCIL_READWRITE:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		return;
	case tgfx_imageaccess_DEPTHSTENCIL_WRITEONLY:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		return;
	case tgfx_imageaccess_DEPTHREADWRITE_STENCILREAD:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
		return;
	case tgfx_imageaccess_DEPTHREADWRITE_STENCILWRITE:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		return;
	case tgfx_imageaccess_DEPTHREAD_STENCILREADWRITE:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
		return;
	case tgfx_imageaccess_DEPTHREAD_STENCILWRITE:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
		return;
	case tgfx_imageaccess_DEPTHWRITE_STENCILREAD:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
		return;
	case tgfx_imageaccess_DEPTHWRITE_STENCILREADWRITE:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		return;
	case tgfx_imageaccess_DEPTH_READONLY:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
		return;
	case tgfx_imageaccess_DEPTH_READWRITE:
	case tgfx_imageaccess_DEPTH_WRITEONLY:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		return;
	case tgfx_imageaccess_NO_ACCESS:
		TargetAccessFlag = 0;
		TargetImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		return;
	case tgfx_imageaccess_RTCOLOR_READONLY:
		TargetAccessFlag = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		return;
	case tgfx_imageaccess_RTCOLOR_READWRITE:
		TargetAccessFlag = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		return;
	case tgfx_imageaccess_RTCOLOR_WRITEONLY:
		TargetAccessFlag = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		return;
	case tgfx_imageaccess_SHADER_SAMPLEONLY:
		TargetAccessFlag = VK_ACCESS_SHADER_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		return;
	case tgfx_imageaccess_SHADER_SAMPLEWRITE:
		TargetAccessFlag = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_GENERAL;
		return;
	case tgfx_imageaccess_SHADER_WRITEONLY:
		TargetAccessFlag = VK_ACCESS_SHADER_WRITE_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_GENERAL;
		return;
	case tgfx_imageaccess_SWAPCHAIN_DISPLAY:
		TargetAccessFlag = 0;
		TargetImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		return;
	case tgfx_imageaccess_TRANSFER_DIST:
		TargetAccessFlag = VK_ACCESS_TRANSFER_WRITE_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		return;
	case tgfx_imageaccess_TRANSFER_SRC:
		TargetAccessFlag = VK_ACCESS_TRANSFER_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		return;
	default:
		LOG(tgfx_result_NOTCODED, "Find_AccessPattern_byIMAGEACCESS() doesn't support this access type!");
		return;
	}
}

VkImageTiling Find_VkTiling(tgfx_texture_order order) {
	switch (order) {
	case tgfx_texture_order_SWIZZLE:
		return VK_IMAGE_TILING_OPTIMAL;
	case tgfx_texture_order_LINEAR:
		return VK_IMAGE_TILING_LINEAR;
	default:
		LOG(tgfx_result_NOTCODED, "Find_VkTiling() doesn't support this order!");
		return VkImageTiling::VK_IMAGE_TILING_MAX_ENUM;
	}
}
VkImageType Find_VkImageType(tgfx_texture_dimensions dimensions) {
	switch (dimensions) {
	case tgfx_texture_dimensions::TEXTURE_2D:
	case tgfx_texture_dimensions::TEXTURE_CUBE:
		return VK_IMAGE_TYPE_2D;
	case tgfx_texture_dimensions::TEXTURE_3D:
		return VK_IMAGE_TYPE_3D;
	default:
		LOG(tgfx_result_NOTCODED, "Find_VkImageType() doesn't support this dimension!");
		return VkImageType::VK_IMAGE_TYPE_MAX_ENUM;
	}
}



VK_TPCopyDatas::VK_TPCopyDatas() : BUFBUFCopies(JobSys), BUFIMCopies(JobSys), IMIMCopies(JobSys) {

}

VK_TPBarrierDatas::VK_TPBarrierDatas() : BufferBarriers(JobSys), TextureBarriers(JobSys) {

}

VK_TPBarrierDatas::VK_TPBarrierDatas(const VK_TPBarrierDatas& copyfrom) : BufferBarriers(copyfrom.BufferBarriers), TextureBarriers(copyfrom.TextureBarriers) {

}

VK_SubComputePass::VK_SubComputePass() : Dispatches(JobSys) {

}
bool VK_SubComputePass::isThereWorkload() {
	std::unique_lock<std::mutex> Locker;
	Dispatches.PauseAllOperations(Locker);
	for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
		if (Dispatches.size(ThreadID)) {
			return true;
		}
	}
	return false;
}


VK_SubDrawPass::VK_SubDrawPass() : NonIndexedDrawCalls(JobSys), IndexedDrawCalls(JobSys) {

}
bool VK_SubDrawPass::isThereWorkload() {
	if (render_dearIMGUI) {
		return true;
	}
	{
		std::unique_lock<std::mutex> Locker;
		IndexedDrawCalls.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			if (IndexedDrawCalls.size(ThreadID)) {
				return true;
			}
		}
	}
	{
		std::unique_lock<std::mutex> Locker;
		NonIndexedDrawCalls.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			if (NonIndexedDrawCalls.size(ThreadID)) {
				return true;
			}
		}
	}
	return false;
}

VK_DescImageElement::VK_DescImageElement() {
	info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.imageView = VK_NULL_HANDLE;
	info.sampler = VK_NULL_HANDLE;
}
VK_DescImageElement::VK_DescImageElement(const VK_DescImageElement& copydesc) : IsUpdated(copydesc.IsUpdated.load()), info(copydesc.info) {

}
VK_DescBufferElement::VK_DescBufferElement() {
	Info.buffer = VK_NULL_HANDLE;
	Info.offset = UINT64_MAX;
	Info.range = UINT64_MAX;
}
VK_DescBufferElement::VK_DescBufferElement(const VK_DescBufferElement& copyBuf) : IsUpdated(copyBuf.IsUpdated.load()), Info(copyBuf.Info) {

}
bool VK_TransferPass::IsWorkloaded() {
	if (!TransferDatas) {
		return false;
	}
	switch (TYPE) {
	case tgfx_transferpass_type_BARRIER:
	{
		bool isThereAny = false;
		VK_TPBarrierDatas* TPB = (VK_TPBarrierDatas*)TransferDatas;
		//Check Buffer Barriers
		{
			std::unique_lock<std::mutex> BufferLocker;
			TPB->BufferBarriers.PauseAllOperations(BufferLocker);
			for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
				if (TPB->BufferBarriers.size(ThreadID)) {
					return true;
				}
			}
		}
		//Check Texture Barriers
		if (!isThereAny) {
			std::unique_lock<std::mutex> TextureLocker;
			TPB->TextureBarriers.PauseAllOperations(TextureLocker);
			for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
				if (TPB->TextureBarriers.size(ThreadID)) {
					return true;
				}
			}
		}
		return false;
	}
	break;
	case tgfx_transferpass_type_COPY:
	{
		bool isThereAny = false;
		VK_TPCopyDatas* DATAs = (VK_TPCopyDatas*)TransferDatas;
		{
			std::unique_lock<std::mutex> BUFBUFLOCKER;
			DATAs->BUFBUFCopies.PauseAllOperations(BUFBUFLOCKER);
			for (unsigned char ThreadIndex = 0; ThreadIndex < tapi_GetThreadCount(JobSys); ThreadIndex++) {
				if (DATAs->BUFBUFCopies.size(ThreadIndex)) {
					return true;
				}
			}
		}
		if (!isThereAny) {
			std::unique_lock<std::mutex> BUFIMLOCKER;
			DATAs->BUFIMCopies.PauseAllOperations(BUFIMLOCKER);
			for (unsigned char ThreadIndex = 0; ThreadIndex < tapi_GetThreadCount(JobSys); ThreadIndex++) {
				if (DATAs->BUFIMCopies.size(ThreadIndex)) {
					return true;
				}
			}
		}
		if (!isThereAny) {
			std::unique_lock<std::mutex> IMIMLOCKER;
			DATAs->IMIMCopies.PauseAllOperations(IMIMLOCKER);
			for (unsigned char ThreadIndex = 0; ThreadIndex < tapi_GetThreadCount(JobSys); ThreadIndex++) {
				if (DATAs->IMIMCopies.size(ThreadIndex)) {
					return true;
				}
			}
		}

		return false;
	}
	break;
	default:
		LOG(tgfx_result_FAIL, "VK_TransferPass::IsWorkloaded() doesn't support this type of transfer pass type!");
		return false;
	}




	return false;
}
bool VK_ComputePass::IsWorkloaded() {
	for (unsigned char SubpassIndex = 0; SubpassIndex < Subpasses.size(); SubpassIndex++) {
		VK_SubComputePass& SP = Subpasses[SubpassIndex];
		if (SP.isThereWorkload()) {
			return true;
		}
	}
	return false;
}
bool VK_WindowPass::IsWorkloaded() {
	if (WindowCalls[3].size()) {
		return true;
	}
	return false;
}
bool VK_DrawPass::IsWorkloaded() {
	for (unsigned char SubpassIndex = 0; SubpassIndex < Subpass_Count; SubpassIndex++) {
		VK_SubDrawPass& SP = Subpasses[SubpassIndex];
		if (SP.isThereWorkload()) {
			return true;
		}
	}
	return false;
}