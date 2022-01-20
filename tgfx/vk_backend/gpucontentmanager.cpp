#include "gpucontentmanager.h"
#include <tgfx_core.h>
#include <tgfx_gpucontentmanager.h>
#include "predefinitions_vk.h"
#include "extension.h"
#include "core.h"
#include "includes.h"


struct descelement_buffer_vk {
	VkDescriptorBufferInfo Info = {};
	std::atomic<unsigned char> IsUpdated = 255;
	descelement_buffer_vk(){}
	descelement_buffer_vk(const descelement_buffer_vk& copyDesc){}
};
struct descelement_image_vk {
	VkDescriptorImageInfo info = {};
	std::atomic<unsigned char> IsUpdated = 255;
	descelement_image_vk(){}
	descelement_image_vk(const descelement_image_vk& copyDesc){}
};
struct descriptor_vk {
	desctype_vk Type;
	void* Elements = nullptr;
	unsigned int ElementCount = 0;
};
struct descset_vk {
	VkDescriptorSet Set = VK_NULL_HANDLE;
	VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
	descriptor_vk* Descs = nullptr;
	//DescCount is size of the "Descs" pointer above, others are including element counts of each descriptor
	unsigned int DescCount = 0, DescUBuffersCount = 0, DescSBuffersCount = 0, DescSamplersCount = 0, DescImagesCount = 0;
	std::atomic_bool ShouldRecreate = false;
};
struct descset_updatecall_vk {
	descset_vk* Set = nullptr;
	unsigned int BindingIndex = UINT32_MAX, ElementIndex = UINT32_MAX;
};
struct descpool_vk {
	VkDescriptorPool pool;
	std::atomic<uint32_t> REMAINING_SET, REMAINING_UBUFFER, REMAINING_SBUFFER, REMAINING_SAMPLER, REMAINING_IMAGE;
};

struct gpudatamanager_private {
	//This vector contains descriptor sets that are for material types/instances that are created this frame
	threadlocal_vector<descset_vk*> DescSets_toCreate;
	//This vector contains descriptor sets that needs update
	//These desc sets are created before and is used in last 2 frames (So new descriptor set creation and updating it is necessary)
	threadlocal_vector<descset_updatecall_vk> DescSets_toCreateUpdate;
	//These desc sets are not used recently in draw calls. So don't go to create-update-delete process, just update.
	threadlocal_vector<descset_updatecall_vk> DescSets_toJustUpdate;
	//These are the desc sets that should be destroyed next frame!
	std::vector<VkDescriptorSet> UnboundMaterialDescSetList;
	unsigned int UnboundDescSetImageCount, UnboundDescSetSamplerCount, UnboundDescSetUBufferCount, UnboundDescSetSBufferCount;
	descpool_vk MaterialRelated_DescPool;

	VkDescriptorPool GlobalShaderInputs_DescPool;
	VkDescriptorSet UnboundGlobalBufferDescSet, UnboundGlobalTextureDescSet; //These won't be deleted, just as a back buffer
	descset_vk GlobalBuffers_DescSet, GlobalTextures_DescSet;
	gpudatamanager_private() : DescSets_toCreate(1024, nullptr), DescSets_toCreateUpdate(1024, descset_updatecall_vk()), DescSets_toJustUpdate(1024, descset_updatecall_vk()) {}
};
static gpudatamanager_private* hidden = nullptr;


void  Destroy_AllResources (){}


extern void FindBufferOBJ_byBufType(const buffer_tgfx_handle Handle, buffertype_tgfx TYPE, VkBuffer& TargetBuffer, VkDeviceSize& TargetOffset) {
	printer(result_tgfx_NOTCODED, "FindBufferOBJ_byBufType() isn't coded yet!");
}
extern VkBuffer Create_VkBuffer(unsigned int size, VkBufferUsageFlags usage) {
	VkBuffer buffer;

	VkBufferCreateInfo ci{};
	ci.usage = usage;
	if (rendergpu->QUEUEFAMSCOUNT() > 1) {
		ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
	}
	else {
		ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	ci.queueFamilyIndexCount = rendergpu->QUEUEFAMSCOUNT();
	ci.pQueueFamilyIndices = rendergpu->ALLQUEUEFAMILIES();
	ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	ci.size = size;

	if (vkCreateBuffer(rendergpu->LOGICALDEVICE(), &ci, nullptr, &buffer) != VK_SUCCESS) {
		printer(result_tgfx_FAIL, "Create_VkBuffer has failed!");
	}
	return buffer;
}
unsigned int Find_MeaningfulDescriptorPoolSize(unsigned int needed_descriptorcount, desctype_vk DescType) {
	unsigned int Supported_MaxDescCount = 0;
	switch (DescType) {
	case desctype_vk::IMAGE:
		Supported_MaxDescCount = rendergpu->EXTMANAGER()->GETMAXDESC(desctype_vk::IMAGE);
		break;
	case desctype_vk::SAMPLER:
		Supported_MaxDescCount = rendergpu->EXTMANAGER()->GETMAXDESC(desctype_vk::SAMPLER);
		break;
	case desctype_vk::UBUFFER:
		Supported_MaxDescCount = rendergpu->EXTMANAGER()->GETMAXDESC(desctype_vk::UBUFFER);
		break;
	case desctype_vk::SBUFFER:
		Supported_MaxDescCount = rendergpu->EXTMANAGER()->GETMAXDESC(desctype_vk::SBUFFER);
		break;
	default:
		printer(result_tgfx_FAIL, "Find_MeaningfulDescriptorPoolSize() doesn't support this type of DescType!");
		break;
	}
	const unsigned int FragmentationPreventingDescCount = ((Supported_MaxDescCount / 100) * 10);
	if (needed_descriptorcount == Supported_MaxDescCount) {
		return Supported_MaxDescCount;
	}
	if (needed_descriptorcount > Supported_MaxDescCount) {
		printer(result_tgfx_FAIL, "You want more shader input than your GPU supports, so maximum count that your GPU supports has returned!");
		return Supported_MaxDescCount;
	}
	if (needed_descriptorcount > (Supported_MaxDescCount * 2) / 5) {
		if (needed_descriptorcount < Supported_MaxDescCount / 2) {
			return needed_descriptorcount * 2;
		}
		if (needed_descriptorcount < (Supported_MaxDescCount * 4) / 5) {
			return needed_descriptorcount + FragmentationPreventingDescCount;
		}
		return needed_descriptorcount;
	}
	return needed_descriptorcount * 2 + FragmentationPreventingDescCount;
}
result_tgfx Create_SamplingType (unsigned int MinimumMipLevel, unsigned int MaximumMipLevel,
	texture_mipmapfilter_tgfx MINFILTER, texture_mipmapfilter_tgfx MAGFILTER, texture_wrapping_tgfx WRAPPING_WIDTH,
	texture_wrapping_tgfx WRAPPING_HEIGHT, texture_wrapping_tgfx WRAPPING_DEPTH, samplingtype_tgfx_handle* SamplingTypeHandle){
	return result_tgfx_FAIL;
}

/*Attributes are ordered as the same order of input vector
* For example: Same attribute ID may have different location/order in another attribute layout
* So you should gather your vertex buffer data according to that
*/

result_tgfx Create_VertexAttributeLayout (const datatype_tgfx* Attributes, vertexlisttypes_tgfx listtype,
	vertexattributelayout_tgfx_handle* VertexAttributeLayoutHandle){
	return result_tgfx_FAIL;
}
void  Delete_VertexAttributeLayout (vertexattributelayout_tgfx_handle VertexAttributeLayoutHandle){}

result_tgfx Upload_toBuffer (buffer_tgfx_handle Handle, buffertype_tgfx Type, const void* DATA, unsigned int DATA_SIZE, unsigned int OFFSET){ return result_tgfx_FAIL; }

result_tgfx Create_StagingBuffer (unsigned int DATASIZE, unsigned int MemoryTypeIndex, buffer_tgfx_handle* Handle){ return result_tgfx_FAIL; }
void  Delete_StagingBuffer (buffer_tgfx_handle StagingBufferHandle){}
/*
* You should sort your vertex data according to attribute layout, don't forget that
* VertexCount shouldn't be 0
*/
result_tgfx Create_VertexBuffer (vertexattributelayout_tgfx_handle VertexAttributeLayoutHandle, unsigned int VertexCount,
	unsigned int MemoryTypeIndex, buffer_tgfx_handle* VertexBufferHandle){
	return result_tgfx_FAIL;
}
void  Unload_VertexBuffer (buffer_tgfx_handle BufferHandle){}

result_tgfx Create_IndexBuffer (datatype_tgfx DataType, unsigned int IndexCount, unsigned int MemoryTypeIndex, buffer_tgfx_handle* IndexBufferHandle){ return result_tgfx_FAIL; }
void  Unload_IndexBuffer (buffer_tgfx_handle BufferHandle){}

result_tgfx Create_Texture (texture_dimensions_tgfx DIMENSION, unsigned int WIDTH, unsigned int HEIGHT, texture_channels_tgfx CHANNEL_TYPE,
	unsigned char MIPCOUNT, textureusageflag_tgfx_handle USAGE, texture_order_tgfx DATAORDER, unsigned int MemoryTypeIndex, texture_tgfx_handle* TextureHandle){
	return result_tgfx_FAIL;
}
//TARGET OFFSET is the offset in the texture's buffer to copy to
result_tgfx Upload_Texture (texture_tgfx_handle TextureHandle, const void* InputData, unsigned int DataSize, unsigned int TargetOffset){ return result_tgfx_FAIL; }
void  Delete_Texture (texture_tgfx_handle TEXTUREHANDLE, unsigned char isUsedLastFrame){}

result_tgfx Create_GlobalBuffer (const char* BUFFER_NAME, unsigned int DATA_SIZE, unsigned char isUniform,
	unsigned int MemoryTypeIndex, buffer_tgfx_handle* GlobalBufferHandle) {
	return result_tgfx_FAIL;
}
void  Unload_GlobalBuffer (buffer_tgfx_handle BUFFER_ID){}
result_tgfx SetGlobalShaderInput_Buffer (unsigned char isUniformBuffer, unsigned int ElementIndex, unsigned char isUsedLastFrame,
	buffer_tgfx_handle BufferHandle, unsigned int BufferOffset, unsigned int BoundDataSize) {
	return result_tgfx_FAIL;
}
result_tgfx SetGlobalShaderInput_Texture (unsigned char isSampledTexture, unsigned int ElementIndex, unsigned char isUsedLastFrame,
	texture_tgfx_handle TextureHandle, samplingtype_tgfx_handle sampler, image_access_tgfx access) { return result_tgfx_FAIL;}


result_tgfx Compile_ShaderSource (shaderlanguages_tgfx language, shaderstage_tgfx shaderstage, void* DATA, unsigned int DATA_SIZE,
	shadersource_tgfx_handle* ShaderSourceHandle) { return result_tgfx_FAIL;}
void  Delete_ShaderSource (shadersource_tgfx_handle ShaderSourceHandle){}
result_tgfx Link_MaterialType (shadersource_tgfx_listhandle ShaderSourcesList, shaderinputdescription_tgfx_listhandle ShaderInputDescs,
	vertexattributelayout_tgfx_handle AttributeLayout, subdrawpass_tgfx_handle Subdrawpass, cullmode_tgfx culling,
	polygonmode_tgfx polygonmode, depthsettings_tgfx_handle depthtest, stencilsettings_tgfx_handle StencilFrontFaced,
	stencilsettings_tgfx_handle StencilBackFaced, blendinginfo_tgfx_listhandle BLENDINGs, rasterpipelinetype_tgfx_handle* MaterialHandle){ return result_tgfx_FAIL;}
void  Delete_MaterialType (rasterpipelinetype_tgfx_handle ID){}
result_tgfx Create_MaterialInst (rasterpipelinetype_tgfx_handle MaterialType, rasterpipelineinstance_tgfx_handle* MaterialInstHandle){ return result_tgfx_FAIL; }
void  Delete_MaterialInst (rasterpipelineinstance_tgfx_handle ID){}

result_tgfx Create_ComputeType (shadersource_tgfx_handle Source, shaderinputdescription_tgfx_listhandle ShaderInputDescs, computeshadertype_tgfx_handle* ComputeTypeHandle){ return result_tgfx_FAIL; }
result_tgfx Create_ComputeInstance(computeshadertype_tgfx_handle ComputeType, computeshaderinstance_tgfx_handle* ComputeShaderInstanceHandle) { return result_tgfx_FAIL; }
void  Delete_ComputeShaderType (computeshadertype_tgfx_handle ID){}
void  Delete_ComputeShaderInstance (computeshaderinstance_tgfx_handle ID){}

//IsUsedRecently means is the material type/instance used in last frame. This is necessary for Vulkan synchronization process.
result_tgfx SetMaterialType_Buffer (rasterpipelinetype_tgfx_handle MaterialType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	buffer_tgfx_handle BufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize){
	return result_tgfx_FAIL;
}
result_tgfx SetMaterialType_Texture (rasterpipelinetype_tgfx_handle MaterialType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage){
	return result_tgfx_FAIL;
}

result_tgfx SetMaterialInst_Buffer (rasterpipelineinstance_tgfx_handle MaterialInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	buffer_tgfx_handle BufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize){
	return result_tgfx_FAIL;
}
result_tgfx SetMaterialInst_Texture (rasterpipelineinstance_tgfx_handle MaterialInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage){
	return result_tgfx_FAIL;
}
//IsUsedRecently means is the material type/instance used in last frame. This is necessary for Vulkan synchronization process.
result_tgfx SetComputeType_Buffer (computeshadertype_tgfx_handle ComputeType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	buffer_tgfx_handle TargetBufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize){
	return result_tgfx_FAIL;
}
result_tgfx SetComputeType_Texture (computeshadertype_tgfx_handle ComputeType, unsigned char isComputeType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage){
	return result_tgfx_FAIL;
}

result_tgfx SetComputeInst_Buffer (computeshaderinstance_tgfx_handle ComputeInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	buffer_tgfx_handle TargetBufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize){
	return result_tgfx_FAIL;
}
result_tgfx SetComputeInst_Texture (computeshaderinstance_tgfx_handle ComputeInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage){
	return result_tgfx_FAIL;
}


result_tgfx Create_RTSlotset (rtslotdescription_tgfx_listhandle Descriptions, rtslotset_tgfx_handle* RTSlotSetHandle){ return result_tgfx_FAIL; }
void  Delete_RTSlotSet (rtslotset_tgfx_handle RTSlotSetHandle){}
//Changes on RTSlots only happens at the frame slot is gonna be used
//For example; if you change next frame's slot, necessary API calls are gonna be called next frame
//For example; if you change slot but related slotset isn't used by drawpass, it doesn't happen until it is used
result_tgfx Change_RTSlotTexture (rtslotset_tgfx_handle RTSlotHandle, unsigned char isColorRT, unsigned char SlotIndex, unsigned char FrameIndex, texture_tgfx_handle TextureHandle){ return result_tgfx_FAIL; }
result_tgfx Inherite_RTSlotSet (rtslotusage_tgfx_listhandle Descriptions, rtslotset_tgfx_handle RTSlotSetHandle, inheritedrtslotset_tgfx_handle* InheritedSlotSetHandle){ return result_tgfx_FAIL; }
void  Delete_InheritedRTSlotSet (inheritedrtslotset_tgfx_handle InheritedRTSlotSetHandle){}

inline void set_functionpointers() {
	core_tgfx_main->contentmanager->Change_RTSlotTexture = Change_RTSlotTexture;
	core_tgfx_main->contentmanager->Compile_ShaderSource = Compile_ShaderSource;
	core_tgfx_main->contentmanager->Create_ComputeInstance = Create_ComputeInstance;
	core_tgfx_main->contentmanager->Create_ComputeType = Create_ComputeType;
	core_tgfx_main->contentmanager->Create_GlobalBuffer = Create_GlobalBuffer;
	core_tgfx_main->contentmanager->Create_IndexBuffer = Create_IndexBuffer;
	core_tgfx_main->contentmanager->Create_MaterialInst = Create_MaterialInst;
	core_tgfx_main->contentmanager->Create_RTSlotset = Create_RTSlotset;
	core_tgfx_main->contentmanager->Create_SamplingType = Create_SamplingType;
	core_tgfx_main->contentmanager->Create_StagingBuffer = Create_StagingBuffer;
	core_tgfx_main->contentmanager->Create_Texture = Create_Texture;
	core_tgfx_main->contentmanager->Create_VertexAttributeLayout = Create_VertexAttributeLayout;
	core_tgfx_main->contentmanager->Create_VertexBuffer = Create_VertexBuffer;
	core_tgfx_main->contentmanager->Delete_ComputeShaderInstance = Delete_ComputeShaderInstance;
	core_tgfx_main->contentmanager->Delete_ComputeShaderType = Delete_ComputeShaderType;
	core_tgfx_main->contentmanager->Delete_InheritedRTSlotSet = Delete_InheritedRTSlotSet;
	core_tgfx_main->contentmanager->Delete_MaterialInst = Delete_MaterialInst;
	core_tgfx_main->contentmanager->Delete_MaterialType = Delete_MaterialType;
	core_tgfx_main->contentmanager->Delete_RTSlotSet = Delete_RTSlotSet;
	core_tgfx_main->contentmanager->Delete_ShaderSource = Delete_ShaderSource;
	core_tgfx_main->contentmanager->Delete_StagingBuffer = Delete_StagingBuffer;
	core_tgfx_main->contentmanager->Delete_Texture = Delete_Texture;
	core_tgfx_main->contentmanager->Delete_VertexAttributeLayout = Delete_VertexAttributeLayout;
	core_tgfx_main->contentmanager->Destroy_AllResources = Destroy_AllResources;
	core_tgfx_main->contentmanager->Inherite_RTSlotSet = Inherite_RTSlotSet;
	core_tgfx_main->contentmanager->Link_MaterialType = Link_MaterialType;
	core_tgfx_main->contentmanager->SetComputeInst_Buffer = SetComputeInst_Buffer;
	core_tgfx_main->contentmanager->SetComputeInst_Texture = SetComputeInst_Texture;
	core_tgfx_main->contentmanager->SetComputeType_Buffer = SetComputeType_Buffer;
	core_tgfx_main->contentmanager->SetComputeType_Texture = SetComputeType_Texture;
	core_tgfx_main->contentmanager->SetGlobalShaderInput_Buffer = SetGlobalShaderInput_Buffer;
	core_tgfx_main->contentmanager->SetGlobalShaderInput_Texture = SetGlobalShaderInput_Texture;
	core_tgfx_main->contentmanager->SetMaterialInst_Buffer = SetMaterialInst_Buffer;
	core_tgfx_main->contentmanager->SetMaterialInst_Texture = SetMaterialInst_Texture;
	core_tgfx_main->contentmanager->SetMaterialType_Buffer = SetMaterialType_Buffer;
	core_tgfx_main->contentmanager->SetMaterialType_Texture = SetMaterialType_Texture;
	core_tgfx_main->contentmanager->Unload_GlobalBuffer = Unload_GlobalBuffer;
	core_tgfx_main->contentmanager->Unload_IndexBuffer = Unload_IndexBuffer;
	core_tgfx_main->contentmanager->Unload_VertexBuffer = Unload_VertexBuffer;
	core_tgfx_main->contentmanager->Upload_Texture = Upload_Texture;
	core_tgfx_main->contentmanager->Upload_toBuffer = Upload_toBuffer;
}
extern void Create_GPUContentManager(initialization_secondstageinfo* info) {
	contentmanager = new gpudatamanager_public;
	contentmanager->hidden = new gpudatamanager_private;
	hidden = contentmanager->hidden;

	set_functionpointers();



	hidden->UnboundDescSetImageCount = 0;
	hidden->UnboundDescSetSamplerCount = 0;
	hidden->UnboundDescSetSBufferCount = 0;
	hidden->UnboundDescSetUBufferCount = 0;

	//Material Related Descriptor Pool Creation
	{
		VkDescriptorPoolCreateInfo dp_ci = {};
		VkDescriptorPoolSize SIZEs[4];
		SIZEs[0].descriptorCount = Find_MeaningfulDescriptorPoolSize(info->MaxSumMaterial_ImageTexture, desctype_vk::IMAGE);
		SIZEs[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		SIZEs[1].descriptorCount = Find_MeaningfulDescriptorPoolSize(info->MaxSumMaterial_StorageBuffer, desctype_vk::SBUFFER);
		SIZEs[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		SIZEs[2].descriptorCount = Find_MeaningfulDescriptorPoolSize(info->MaxSumMaterial_UniformBuffer, desctype_vk::UBUFFER);
		SIZEs[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		SIZEs[3].descriptorCount = Find_MeaningfulDescriptorPoolSize(info->MaxSumMaterial_SampledTexture, desctype_vk::SAMPLER);
		SIZEs[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		dp_ci.maxSets = info->MaxMaterialCount;
		dp_ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		//If descriptor indexing is supported, global descriptor set is only updated (not freed)
		if (rendergpu->EXTMANAGER()->ISSUPPORTED_DESCINDEXING()) {
			dp_ci.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		}
		dp_ci.pNext = nullptr;
		dp_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

		dp_ci.poolSizeCount = 4;
		dp_ci.pPoolSizes = SIZEs;
		if (vkCreateDescriptorPool(rendergpu->LOGICALDEVICE(), &dp_ci, nullptr, &hidden->MaterialRelated_DescPool.pool) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "Material Related Descriptor Pool Creation has failed! So GFXContentManager system initialization has failed!");
			return;
		}

		hidden->MaterialRelated_DescPool.REMAINING_IMAGE.store(info->MaxSumMaterial_ImageTexture);
		hidden->MaterialRelated_DescPool.REMAINING_SAMPLER.store(info->MaxSumMaterial_SampledTexture);
		hidden->MaterialRelated_DescPool.REMAINING_SBUFFER.store(info->MaxSumMaterial_StorageBuffer);
		hidden->MaterialRelated_DescPool.REMAINING_UBUFFER.store(info->MaxSumMaterial_UniformBuffer);
		hidden->MaterialRelated_DescPool.REMAINING_SET.store(info->MaxMaterialCount);
	}


	//Create Descriptor Pool and Descriptor Set Layout for Global Shader Inputs
	{
		unsigned int UBUFFER_COUNT = info->GlobalShaderInput_UniformBufferCount, SBUFFER_COUNT = info->GlobalShaderInput_StorageBufferCount,
			SAMPLER_COUNT = info->GlobalShaderInput_SampledTextureCount, IMAGE_COUNT = info->GlobalShaderInput_ImageTextureCount;

		//Create Descriptor Pool
		{
			//Descriptor Pool and Descriptor Set creation
			//Create a descriptor pool that has 2 times of sum of counts above!
			std::vector<VkDescriptorPoolSize> poolsizes;
			if (UBUFFER_COUNT > 0) {
				VkDescriptorPoolSize UDescCount;
				UDescCount.descriptorCount = UBUFFER_COUNT * 2;
				UDescCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				poolsizes.push_back(UDescCount);
			}
			if (SBUFFER_COUNT > 0) {
				VkDescriptorPoolSize SDescCount;
				SDescCount.descriptorCount = SBUFFER_COUNT * 2;
				SDescCount.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				poolsizes.push_back(SDescCount);
			}
			if (SAMPLER_COUNT > 0) {
				VkDescriptorPoolSize SampledCount;
				SampledCount.descriptorCount = SAMPLER_COUNT * 2;
				SampledCount.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				poolsizes.push_back(SampledCount);
			}
			if (IMAGE_COUNT > 0) {
				VkDescriptorPoolSize ImageCount;
				ImageCount.descriptorCount = IMAGE_COUNT * 2;
				ImageCount.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				poolsizes.push_back(ImageCount);
			}

			if (!poolsizes.size()) {
				VkDescriptorPoolSize defaultsamplersize;
				defaultsamplersize.descriptorCount = 1;
				defaultsamplersize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				poolsizes.push_back(defaultsamplersize);

				VkDescriptorPoolSize defaultubuffersize;
				defaultubuffersize.descriptorCount = 1;
				defaultubuffersize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				poolsizes.push_back(defaultubuffersize);
			}

			VkDescriptorPoolCreateInfo descpool_ci = {};
			descpool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descpool_ci.maxSets = 4;
			descpool_ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			if (rendergpu->EXTMANAGER()->ISSUPPORTED_DESCINDEXING()) {
				descpool_ci.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
			}
			descpool_ci.pPoolSizes = poolsizes.data();
			descpool_ci.poolSizeCount = poolsizes.size();
			descpool_ci.pNext = nullptr;
			if (vkCreateDescriptorPool(rendergpu->LOGICALDEVICE(), &descpool_ci, nullptr, &hidden->GlobalShaderInputs_DescPool) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "Vulkan Global Descriptor Pool Creation has failed!");
			}
		}


		//Create Global Buffer Descriptor Set Layout
		VkDescriptorSetLayoutBinding BufferBinding[2];
		{
			BufferBinding[0].binding = 0;
			BufferBinding[0].pImmutableSamplers = nullptr;
			BufferBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			BufferBinding[1].binding = 1;
			BufferBinding[1].pImmutableSamplers = nullptr;
			BufferBinding[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			if (info->isUniformBuffer_Index1) {
				BufferBinding[0].descriptorCount = SBUFFER_COUNT;
				BufferBinding[1].descriptorCount = UBUFFER_COUNT;
				BufferBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				BufferBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			}
			else {
				BufferBinding[0].descriptorCount = UBUFFER_COUNT;
				BufferBinding[1].descriptorCount = SBUFFER_COUNT;
				BufferBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				BufferBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			}

			VkDescriptorSetLayoutCreateInfo DescSetLayout_ci = {};
			VkDescriptorSetLayoutBindingFlagsCreateInfo descindexing_ci = {};
			VkDescriptorBindingFlags ext_flags[2] = { VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT ,
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT };
			if (rendergpu->EXTMANAGER()->ISSUPPORTED_DESCINDEXING()) {
				descindexing_ci.bindingCount = 2;
				descindexing_ci.pBindingFlags = ext_flags;
				descindexing_ci.pNext = nullptr;
				descindexing_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
				DescSetLayout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
				DescSetLayout_ci.pNext = &descindexing_ci;
			}
			DescSetLayout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			DescSetLayout_ci.bindingCount = 2;
			DescSetLayout_ci.pBindings = BufferBinding;
			if (vkCreateDescriptorSetLayout(rendergpu->LOGICALDEVICE(), &DescSetLayout_ci, nullptr, &hidden->GlobalBuffers_DescSet.Layout) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "Create_RenderGraphResources() has failed at vkCreateDescriptorSetLayout()!");
				return;
			}
		}

		//Create Global Texture Descriptor Set Layout
		VkDescriptorSetLayoutBinding TextureBinding[2];
		{
			TextureBinding[0].binding = 0;
			TextureBinding[0].pImmutableSamplers = nullptr;
			TextureBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			TextureBinding[1].binding = 1;
			TextureBinding[1].pImmutableSamplers = nullptr;
			TextureBinding[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			if (info->isSampledTexture_Index1) {
				TextureBinding[0].descriptorCount = IMAGE_COUNT;
				TextureBinding[1].descriptorCount = SAMPLER_COUNT;
				TextureBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				TextureBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			}
			else {
				TextureBinding[0].descriptorCount = SAMPLER_COUNT;
				TextureBinding[1].descriptorCount = IMAGE_COUNT;
				TextureBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				TextureBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			}


			VkDescriptorSetLayoutCreateInfo DescSetLayout_ci = {};
			VkDescriptorSetLayoutBindingFlagsCreateInfo descindexing_ci = {};
			VkDescriptorBindingFlags ext_flags[2] = { VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT ,
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT };
			if (rendergpu->EXTMANAGER()->ISSUPPORTED_DESCINDEXING()) {
				descindexing_ci.bindingCount = 2;
				descindexing_ci.pBindingFlags = ext_flags;
				descindexing_ci.pNext = nullptr;
				descindexing_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
				DescSetLayout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
				DescSetLayout_ci.pNext = &descindexing_ci;
			}
			DescSetLayout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			DescSetLayout_ci.bindingCount = 2;
			DescSetLayout_ci.pBindings = TextureBinding;
			if (vkCreateDescriptorSetLayout(rendergpu->LOGICALDEVICE(), &DescSetLayout_ci, nullptr, &hidden->GlobalTextures_DescSet.Layout) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "Create_RenderGraphResources() has failed at vkCreateDescriptorSetLayout()!");
				return;
			}
		}

		//NOTE: Descriptor Sets are created two times because we don't want to change descriptors while they're being used by GPU

		//Create Global Buffer Descriptor Sets
		{
			VkDescriptorSetVariableDescriptorCountAllocateInfo variabledescscount_info = {};
			VkDescriptorSetAllocateInfo descset_ai = {};
			uint32_t UnboundMaxBufferDescCount[2] = { BufferBinding[1].descriptorCount, BufferBinding[1].descriptorCount };
			if (rendergpu->EXTMANAGER()->ISSUPPORTED_DESCINDEXING()) {
				variabledescscount_info.descriptorSetCount = 2;
				variabledescscount_info.pDescriptorCounts = UnboundMaxBufferDescCount;
				variabledescscount_info.pNext = nullptr;
				variabledescscount_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
				descset_ai.descriptorPool = hidden->GlobalShaderInputs_DescPool;
			}
			descset_ai.pNext = &variabledescscount_info;
			descset_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descset_ai.descriptorSetCount = 2;
			VkDescriptorSetLayout SetLayouts[2]{ hidden->GlobalBuffers_DescSet.Layout, hidden->GlobalBuffers_DescSet.Layout };
			descset_ai.pSetLayouts = SetLayouts;
			VkDescriptorSet Sets[2];
			if (vkAllocateDescriptorSets(rendergpu->LOGICALDEVICE(), &descset_ai, Sets) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "CreateandUpdate_GlobalDescSet() has failed at vkAllocateDescriptorSets()!");
			}
			hidden->UnboundGlobalBufferDescSet = Sets[0];
			hidden->GlobalBuffers_DescSet.Set = Sets[1];
		}

		//Create Global Texture Descriptor Sets
		{
			VkDescriptorSetVariableDescriptorCountAllocateInfo variabledescscount_info = {};
			VkDescriptorSetAllocateInfo descset_ai = {};
			uint32_t MaxUnboundTextureDescCount[2]{ TextureBinding[1].descriptorCount, TextureBinding[1].descriptorCount };
			if (rendergpu->EXTMANAGER()->ISSUPPORTED_DESCINDEXING()) {
				variabledescscount_info.descriptorSetCount = 2;
				variabledescscount_info.pDescriptorCounts = MaxUnboundTextureDescCount;
				variabledescscount_info.pNext = nullptr;
				variabledescscount_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
				descset_ai.descriptorPool = hidden->GlobalShaderInputs_DescPool;
			}
			descset_ai.pNext = &variabledescscount_info;
			descset_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descset_ai.descriptorSetCount = 2;
			VkDescriptorSetLayout SetLayouts[2]{ hidden->GlobalTextures_DescSet.Layout, hidden->GlobalTextures_DescSet.Layout };
			descset_ai.pSetLayouts = SetLayouts;
			VkDescriptorSet Sets[2];
			if (vkAllocateDescriptorSets(rendergpu->LOGICALDEVICE(), &descset_ai, Sets) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "CreateandUpdate_GlobalDescSet() has failed at vkAllocateDescriptorSets()!");
			}
			hidden->UnboundGlobalTextureDescSet = Sets[0];
			hidden->GlobalTextures_DescSet.Set = Sets[1];
		}

		//Fill Global Buffer Descriptor Set
		{
			descriptor_vk* BufferFinalDescs = new descriptor_vk[2];
			BufferFinalDescs[0].ElementCount = BufferBinding[0].descriptorCount;
			BufferFinalDescs[0].Elements = new descelement_buffer_vk[BufferFinalDescs[0].ElementCount];
			BufferFinalDescs[0].Type = info->isUniformBuffer_Index1 ? desctype_vk::SBUFFER : desctype_vk::UBUFFER;
			BufferFinalDescs[1].ElementCount = BufferBinding[1].descriptorCount;
			BufferFinalDescs[1].Elements = new descelement_buffer_vk[BufferFinalDescs[1].ElementCount];
			BufferFinalDescs[1].Type = info->isUniformBuffer_Index1 ? desctype_vk::UBUFFER : desctype_vk::SBUFFER;;

			hidden->GlobalBuffers_DescSet.DescCount = UBUFFER_COUNT + SBUFFER_COUNT;
			hidden->GlobalBuffers_DescSet.DescUBuffersCount = 1;
			hidden->GlobalBuffers_DescSet.DescSBuffersCount = 1;
			hidden->GlobalBuffers_DescSet.Descs = BufferFinalDescs;
		}

		//Fill Global Texture Descriptor Set
		{
			descriptor_vk* TextureFinalDescs = new descriptor_vk[2];
			TextureFinalDescs[0].ElementCount = TextureBinding[0].descriptorCount;
			TextureFinalDescs[0].Elements = new descelement_image_vk[TextureFinalDescs[0].ElementCount];
			TextureFinalDescs[0].Type = info->isSampledTexture_Index1 ? desctype_vk::IMAGE : desctype_vk::SAMPLER;
			TextureFinalDescs[1].ElementCount = TextureBinding[1].descriptorCount;
			TextureFinalDescs[1].Elements = new descelement_image_vk[TextureFinalDescs[1].ElementCount];
			TextureFinalDescs[1].Type = info->isSampledTexture_Index1 ? desctype_vk::SAMPLER : desctype_vk::IMAGE;

			hidden->GlobalTextures_DescSet.DescCount = IMAGE_COUNT + SAMPLER_COUNT;
			hidden->GlobalTextures_DescSet.DescUBuffersCount = 1;
			hidden->GlobalTextures_DescSet.DescSBuffersCount = 1;
			hidden->GlobalTextures_DescSet.Descs = TextureFinalDescs;
		}
	}
}