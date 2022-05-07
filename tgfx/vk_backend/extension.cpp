#include "extension.h"
#include "resource.h"
#include <tgfx_forwarddeclarations.h>
#include <atomic>
#include <glm/glm.hpp>
#include "resource.h"
#include "includes.h"
#include "core.h"

//Seperated Depth Stencil Layouts
void Fill_DepthAttachmentReference_SeperatedDSLayouts(VkAttachmentReference& Ref, unsigned int index, texture_channels_tgfx channels, operationtype_tgfx DEPTHOPTYPE, operationtype_tgfx STENCILOPTYPE);
void Fill_DepthAttachmentReference_NOSeperated(VkAttachmentReference& Ref, unsigned int index, texture_channels_tgfx channels, operationtype_tgfx DEPTHOPTYPE, operationtype_tgfx STENCILOPTYPE);
void Fill_DepthAttachmentDescription_SeperatedDSLayouts(VkAttachmentDescription& Desc, depthstencilslot_vk* DepthSlot);
void Fill_DepthAttachmentDescription_NOSeperated(VkAttachmentDescription& Desc, depthstencilslot_vk* DepthSlot);


extension_manager::extension_manager() {
	SeperatedDepthStencilLayouts = false;
	Fill_DepthAttachmentDescription = Fill_DepthAttachmentDescription_NOSeperated;
	Fill_DepthAttachmentReference = Fill_DepthAttachmentReference_NOSeperated;
}
void extension_manager::SUPPORT_VARIABLEDESCCOUNT() { isVariableDescCountSupported = true; }
void extension_manager::SUPPORT_SWAPCHAINDISPLAY() { SwapchainDisplay = true; }
void extension_manager::SUPPORT_SEPERATEDDEPTHSTENCILLAYOUTS() {
	SeperatedDepthStencilLayouts = true;
	Fill_DepthAttachmentDescription = Fill_DepthAttachmentDescription_SeperatedDSLayouts;
	Fill_DepthAttachmentReference = Fill_DepthAttachmentReference_SeperatedDSLayouts;
}
void extension_manager::SUPPORT_BUFFERDEVICEADDRESS() { isBufferDeviceAddressSupported = true; }
void extension_manager::CheckDeviceExtensionProperties(GPU_VKOBJ* VKGPU) {
	VkPhysicalDeviceDescriptorIndexingProperties descindexinglimits = {};
	VkPhysicalDeviceInlineUniformBlockPropertiesEXT uniformblocklimits = {};
	VkPhysicalDeviceProperties2 devprops2;


	devprops2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	descindexinglimits.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;
	uniformblocklimits.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_PROPERTIES_EXT;
	devprops2.pNext = &descindexinglimits;
	descindexinglimits.pNext = &uniformblocklimits;
	vkGetPhysicalDeviceProperties2(VKGPU->PHYSICALDEVICE(), &devprops2);
	//Check Descriptor Limits
	{
		static_assert(VKCONST_DYNAMICDESCRIPTORTYPESCOUNT == 4, "CheckDeviceExtensionProperties() supports only 4 DynamicDescTypes!");
		if (ISSUPPORTED_DESCINDEXING()) {
			MaxDescCounts[VKCONST_DESCSETID_DYNAMICSAMPLER] = descindexinglimits.maxDescriptorSetUpdateAfterBindSamplers;
			MaxDescCounts[VKCONST_DESCSETID_SAMPLEDTEXTURE] = descindexinglimits.maxDescriptorSetUpdateAfterBindSampledImages;
			MaxDescCounts[VKCONST_DESCSETID_STORAGEIMAGE] = descindexinglimits.maxDescriptorSetUpdateAfterBindStorageImages;
			MaxDescCounts[VKCONST_DESCSETID_BUFFER] = descindexinglimits.maxDescriptorSetUpdateAfterBindStorageBuffers;
			MaxDesc_ALL = descindexinglimits.maxUpdateAfterBindDescriptorsInAllPools;
			MaxAccessibleDesc_PerStage = descindexinglimits.maxPerStageUpdateAfterBindResources;
		}
		else {
			MaxDescCounts[VKCONST_DESCSETID_DYNAMICSAMPLER] = VKGPU->DEVICEPROPERTIES().limits.maxPerStageDescriptorSamplers;
			MaxDescCounts[VKCONST_DESCSETID_SAMPLEDTEXTURE] = VKGPU->DEVICEPROPERTIES().limits.maxPerStageDescriptorSampledImages;
			MaxDescCounts[VKCONST_DESCSETID_STORAGEIMAGE] = VKGPU->DEVICEPROPERTIES().limits.maxPerStageDescriptorStorageImages;
			MaxDescCounts[VKCONST_DESCSETID_BUFFER] = VKGPU->DEVICEPROPERTIES().limits.maxPerStageDescriptorStorageBuffers;
			MaxDesc_ALL = UINT32_MAX;
			MaxAccessibleDesc_PerStage = VKGPU->DEVICEPROPERTIES().limits.maxPerStageResources;
		}
		MaxBoundDescSet = VKGPU->DEVICEPROPERTIES().limits.maxBoundDescriptorSets;
	}
}
void Fill_DepthAttachmentReference_SeperatedDSLayouts(VkAttachmentReference& Ref, unsigned int index, texture_channels_tgfx channels, operationtype_tgfx DEPTHOPTYPE, operationtype_tgfx STENCILOPTYPE) {
	Ref.attachment = index;
	if (channels == texture_channels_tgfx_D32) {
		switch (DEPTHOPTYPE) {
		case operationtype_tgfx_READ_ONLY:
			Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case operationtype_tgfx_READ_AND_WRITE:
		case operationtype_tgfx_WRITE_ONLY:
			Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			break;
		case operationtype_tgfx_UNUSED:
			Ref.attachment = VK_ATTACHMENT_UNUSED;
			Ref.layout = VK_IMAGE_LAYOUT_UNDEFINED;
			break;
		default:
			printer(result_tgfx_INVALIDARGUMENT, "VK::Fill_SubpassStructs() doesn't support this type of Operation Type for DepthBuffer!");
		}
	}
	else if (channels == texture_channels_tgfx_D24S8) {
		switch (STENCILOPTYPE) {
		case operationtype_tgfx_READ_ONLY:
			if (DEPTHOPTYPE == operationtype_tgfx_UNUSED || DEPTHOPTYPE == operationtype_tgfx_READ_ONLY) {
				Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			else if (DEPTHOPTYPE == operationtype_tgfx_READ_AND_WRITE || DEPTHOPTYPE == operationtype_tgfx_WRITE_ONLY) {
				Ref.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
			}
			break;
		case operationtype_tgfx_READ_AND_WRITE:
		case operationtype_tgfx_WRITE_ONLY:
			if (DEPTHOPTYPE == operationtype_tgfx_UNUSED || DEPTHOPTYPE == operationtype_tgfx_READ_ONLY) {
				Ref.layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else if (DEPTHOPTYPE == operationtype_tgfx_READ_AND_WRITE || DEPTHOPTYPE == operationtype_tgfx_WRITE_ONLY) {
				Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			break;
		case operationtype_tgfx_UNUSED:
			if (DEPTHOPTYPE == operationtype_tgfx_UNUSED) {
				Ref.attachment = VK_ATTACHMENT_UNUSED;
				Ref.layout = VK_IMAGE_LAYOUT_UNDEFINED;
			}
			else if (DEPTHOPTYPE == operationtype_tgfx_READ_AND_WRITE || DEPTHOPTYPE == operationtype_tgfx_WRITE_ONLY) {
				Ref.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
			}
			else if (DEPTHOPTYPE == operationtype_tgfx_READ_ONLY) {
				Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			break;
		default:
			printer(result_tgfx_INVALIDARGUMENT, "VK::Fill_SubpassStructs() doesn't support this type of Operation Type for DepthSTENCILBuffer!");
		}
	}
}
void Fill_DepthAttachmentReference_NOSeperated(VkAttachmentReference& Ref, unsigned int index, texture_channels_tgfx channels, operationtype_tgfx DEPTHOPTYPE, operationtype_tgfx STENCILOPTYPE) {
	Ref.attachment = index;
	if (DEPTHOPTYPE == operationtype_tgfx_UNUSED) {
		Ref.attachment = VK_ATTACHMENT_UNUSED;
		Ref.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	}
	else {
		Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}
}
void Fill_DepthAttachmentDescription_SeperatedDSLayouts(VkAttachmentDescription& Desc, depthstencilslot_vk* DepthSlot) {
	Desc = {};
	Desc.format = Find_VkFormat_byTEXTURECHANNELs(DepthSlot->RT->CHANNELs);
	Desc.samples = VK_SAMPLE_COUNT_1_BIT;
	Desc.flags = 0;
	Desc.loadOp = Find_LoadOp_byGFXLoadOp(DepthSlot->DEPTH_LOAD);
	Desc.stencilLoadOp = Find_LoadOp_byGFXLoadOp(DepthSlot->STENCIL_LOAD);
	if (DepthSlot->IS_USED_LATER) {
		Desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	}
	else {
		Desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
	if (DepthSlot->RT->CHANNELs == texture_channels_tgfx_D32) {
		if (DepthSlot->DEPTH_OPTYPE == operationtype_tgfx_READ_ONLY) {
			Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
			Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
		}
		else {
			Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		}
	}
	else if (DepthSlot->RT->CHANNELs == texture_channels_tgfx_D24S8) {
		if (DepthSlot->DEPTH_OPTYPE == operationtype_tgfx_READ_ONLY) {
			if (DepthSlot->STENCIL_OPTYPE != operationtype_tgfx_READ_ONLY &&
				DepthSlot->STENCIL_OPTYPE != operationtype_tgfx_UNUSED) {
				Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
				Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else {
				Desc.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				Desc.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
		}
		else {
			if (DepthSlot->STENCIL_OPTYPE != operationtype_tgfx_READ_ONLY &&
				DepthSlot->STENCIL_OPTYPE != operationtype_tgfx_UNUSED) {
				Desc.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				Desc.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else {
				Desc.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
				Desc.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
			}
		}
	}
}
void Fill_DepthAttachmentDescription_NOSeperated(VkAttachmentDescription& Desc, depthstencilslot_vk* DepthSlot) {
	Desc = {};
	Desc.format = Find_VkFormat_byTEXTURECHANNELs(DepthSlot->RT->CHANNELs);
	Desc.samples = VK_SAMPLE_COUNT_1_BIT;
	Desc.flags = 0;
	Desc.loadOp = Find_LoadOp_byGFXLoadOp(DepthSlot->DEPTH_LOAD);
	Desc.stencilLoadOp = Find_LoadOp_byGFXLoadOp(DepthSlot->STENCIL_LOAD);
	if (DepthSlot->IS_USED_LATER) {
		Desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	}
	else {
		Desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
	Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}