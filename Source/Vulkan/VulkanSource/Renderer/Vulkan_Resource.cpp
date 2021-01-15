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

	VK_TPUploadDatas::VK_TPUploadDatas() : TextureUploads(GFX->JobSys), BufferUploads(GFX->JobSys) {

	}

	VK_TPBarrierDatas::VK_TPBarrierDatas() : BufferBarriers(GFX->JobSys), TextureBarriers(GFX->JobSys) {

	}
}