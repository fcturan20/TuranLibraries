#include "Vulkan_Resource.h"
#define VKGPU ((Vulkan::GPU*)GFX->GPU_TO_RENDER)

namespace Vulkan {
	VkImageUsageFlags Find_VKImageUsage_forVKTexture(VK_Texture& TEXTURE) {
		VkImageUsageFlags FLAG = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if (TEXTURE.USAGE.isRandomlyWrittenTo) {
			FLAG = FLAG | VK_IMAGE_USAGE_STORAGE_BIT;
		}
		if (TEXTURE.USAGE.isCopiableFrom) {
			FLAG = FLAG | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
		if (TEXTURE.USAGE.isCopiableTo) {
			FLAG = FLAG | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		if (TEXTURE.USAGE.isRenderableTo) {
			if (TEXTURE.CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8 || TEXTURE.CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32) {
				FLAG = FLAG | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			}
			else {
				FLAG = FLAG | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			}
		}
		if (TEXTURE.USAGE.isSampledReadOnly) {
			FLAG = FLAG | VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		return FLAG;
	}
	VkImageUsageFlags Find_VKImageUsage_forGFXTextureDesc(GFX_API::TEXTUREUSAGEFLAG USAGEFLAG, GFX_API::TEXTURE_CHANNELs channels) {
		VkImageUsageFlags FLAG = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if (USAGEFLAG.isRandomlyWrittenTo) {
			FLAG = FLAG | VK_IMAGE_USAGE_STORAGE_BIT;
		}
		if (USAGEFLAG.isCopiableFrom) {
			FLAG = FLAG | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
		if (USAGEFLAG.isCopiableTo) {
			FLAG = FLAG | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		if (USAGEFLAG.isRenderableTo) {
			if (channels == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8 || channels == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32) {
				FLAG = FLAG | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			}
			else {
				FLAG = FLAG | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			}
		}
		if (USAGEFLAG.isSampledReadOnly) {
			FLAG = FLAG | VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		return FLAG;
	}
	VK_API void Find_AccessPattern_byIMAGEACCESS(const GFX_API::IMAGE_ACCESS& Access, VkAccessFlags& TargetAccessFlag, VkImageLayout& TargetImageLayout) {
		switch (Access)
		{
		case GFX_API::IMAGE_ACCESS::DEPTHSTENCIL_READONLY:
			TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::DEPTHSTENCIL_READWRITE:
			TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::DEPTHSTENCIL_WRITEONLY:
			TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::DEPTHREADWRITE_STENCILREAD:
			TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::DEPTHREADWRITE_STENCILWRITE:
			TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::DEPTHREAD_STENCILREADWRITE:
			TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::DEPTHREAD_STENCILWRITE:
			TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::DEPTHWRITE_STENCILREAD:
			TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::DEPTHWRITE_STENCILREADWRITE:
			TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::DEPTH_READONLY:
			TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::DEPTH_READWRITE:
		case GFX_API::IMAGE_ACCESS::DEPTH_WRITEONLY:
			TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::NO_ACCESS:
			TargetAccessFlag = 0;
			TargetImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			return;
		case GFX_API::IMAGE_ACCESS::RTCOLOR_READONLY:
			TargetAccessFlag = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE:
			TargetAccessFlag = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::RTCOLOR_WRITEONLY:
			TargetAccessFlag = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY:
			TargetAccessFlag = VK_ACCESS_SHADER_READ_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::SHADER_SAMPLEWRITE:
			TargetAccessFlag = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_GENERAL;
			return;
		case GFX_API::IMAGE_ACCESS::SHADER_WRITEONLY:
			TargetAccessFlag = VK_ACCESS_SHADER_WRITE_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_GENERAL;
			return;
		case GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY:
			TargetAccessFlag = 0;
			TargetImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			return;
		case GFX_API::IMAGE_ACCESS::TRANSFER_DIST:
			TargetAccessFlag = VK_ACCESS_TRANSFER_WRITE_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			return;
		case GFX_API::IMAGE_ACCESS::TRANSFER_SRC:
			TargetAccessFlag = VK_ACCESS_TRANSFER_READ_BIT;
			TargetImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			return;
		default:
			LOG_NOTCODED_TAPI("Find_AccessPattern_byIMAGEACCESS() doesn't support this access type!", true);
			return;
		}
	}

	VK_API VkImageTiling Find_VkTiling(GFX_API::TEXTURE_ORDER order) {
		switch (order) {
		case GFX_API::TEXTURE_ORDER::SWIZZLE:
			return VK_IMAGE_TILING_OPTIMAL;
		case GFX_API::TEXTURE_ORDER::LINEAR:
			return VK_IMAGE_TILING_LINEAR;
		default:
			LOG_NOTCODED_TAPI("Find_VkTiling() doesn't support this order!", true);
			return VkImageTiling::VK_IMAGE_TILING_MAX_ENUM;
		}
	}
	VK_API VkImageType Find_VkImageType(GFX_API::TEXTURE_DIMENSIONs dimensions) {
		switch (dimensions) {
		case GFX_API::TEXTURE_DIMENSIONs::TEXTURE_2D:
		case GFX_API::TEXTURE_DIMENSIONs::TEXTURE_CUBE:
			return VK_IMAGE_TYPE_2D;
		case GFX_API::TEXTURE_DIMENSIONs::TEXTURE_3D:
			return VK_IMAGE_TYPE_3D;
		default:
			LOG_NOTCODED_TAPI("Find_VkImageType() doesn't support this dimension!", true);
			return VkImageType::VK_IMAGE_TYPE_MAX_ENUM;
		}
	}



	VK_TPCopyDatas::VK_TPCopyDatas() : BUFBUFCopies(*GFX->JobSys), BUFIMCopies(*GFX->JobSys), IMIMCopies(*GFX->JobSys) {

	}

	VK_TPBarrierDatas::VK_TPBarrierDatas() : BufferBarriers(*GFX->JobSys), TextureBarriers(*GFX->JobSys) {

	}

	VK_TPBarrierDatas::VK_TPBarrierDatas(const VK_TPBarrierDatas& copyfrom) : BufferBarriers(copyfrom.BufferBarriers), TextureBarriers(copyfrom.TextureBarriers){

	}

	VK_SubComputePass::VK_SubComputePass() : Dispatches(*GFX->JobSys) {

	}
	bool VK_SubComputePass::isThereWorkload() {
		std::unique_lock<std::mutex> Locker;
		Dispatches.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
			if (Dispatches.size(ThreadID)) {
				return true;
			}
		}
		return false;
	}


	VK_SubDrawPass::VK_SubDrawPass() : NonIndexedDrawCalls(*GFX->JobSys), IndexedDrawCalls(*GFX->JobSys) {

	}
	bool VK_SubDrawPass::isThereWorkload() {
		if (render_dearIMGUI) {
			return true;
		}
		{
			std::unique_lock<std::mutex> Locker;
			IndexedDrawCalls.PauseAllOperations(Locker);
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				if (IndexedDrawCalls.size(ThreadID)) {
					return true;
				}
			}
		}
		{
			std::unique_lock<std::mutex> Locker;
			NonIndexedDrawCalls.PauseAllOperations(Locker);
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
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
	VK_DescImageElement::VK_DescImageElement(const VK_DescImageElement& copydesc) : IsUpdated(copydesc.IsUpdated.load()), info(copydesc.info){

	}
	VK_DescBufferElement::VK_DescBufferElement() {
		Info.buffer = VK_NULL_HANDLE;
		Info.offset = UINT64_MAX;
		Info.range = UINT64_MAX;
	}
	VK_DescBufferElement::VK_DescBufferElement(const VK_DescBufferElement& copyBuf) : IsUpdated(copyBuf.IsUpdated.load()), Info(copyBuf.Info){

	}
}