#include "VK_GPUContentManager.h"
#include "Vulkan_Renderer_Core.h"
#include "Vulkan_Core.h"
#include <iostream>
#include <vector>
#include <string>
#define LOG VKCORE->TGFXCORE.DebugCallback


void FindBufferOBJ_byBufType(const tgfx_buffer Handle, tgfx_buffertype TYPE, VkBuffer& TargetBuffer, VkDeviceSize& TargetOffset) {
	LOG(tgfx_result_NOTCODED, "FindBufferOBJ_byBufType() isn't coded yet!");
}
VkBuffer Create_VkBuffer(unsigned int size, VkBufferUsageFlags usage) {
	VkBuffer buffer;

	VkBufferCreateInfo ci{};
	ci.usage = usage;
	if (RENDERGPU->QUEUEFAMS().size() > 1) {
		ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
	}
	else {
		ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	ci.queueFamilyIndexCount = RENDERGPU->QUEUEFAMS().size();
	ci.pQueueFamilyIndices = RENDERGPU->ALLQUEUEFAMILIES();
	ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	ci.size = size;

	if (vkCreateBuffer(RENDERGPU->LOGICALDEVICE(), &ci, nullptr, &buffer) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "Create_VkBuffer has failed!");
	}
	return buffer;
}
unsigned int Find_MeaningfulDescriptorPoolSize(unsigned int needed_descriptorcount, DescType DescType) {
	unsigned int Supported_MaxDescCount = 0;
	switch (DescType) {
	case DescType::IMAGE:
		Supported_MaxDescCount = RENDERGPU->ExtensionRelatedDatas.GETMAXDESC(DescType::IMAGE);
		break;
	case DescType::SAMPLER:
		Supported_MaxDescCount = RENDERGPU->ExtensionRelatedDatas.GETMAXDESC(DescType::SAMPLER);
		break;
	case DescType::UBUFFER:
		Supported_MaxDescCount = RENDERGPU->ExtensionRelatedDatas.GETMAXDESC(DescType::UBUFFER);
		break;
	case DescType::SBUFFER:
		Supported_MaxDescCount = RENDERGPU->ExtensionRelatedDatas.GETMAXDESC(DescType::SBUFFER);
		break;
	default:
		LOG(tgfx_result_FAIL, "Find_MeaningfulDescriptorPoolSize() doesn't support this type of DescType!");
		break;
	}
	const unsigned int FragmentationPreventingDescCount = ((Supported_MaxDescCount / 100) * 10);
	if (needed_descriptorcount == Supported_MaxDescCount) {
		return Supported_MaxDescCount;
	}
	if (needed_descriptorcount > Supported_MaxDescCount) {
		LOG(tgfx_result_FAIL, "You want more shader input than your GPU supports, so maximum count that your GPU supports has returned!");
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

inline void SetContentManagerFunctionPointers() {
	VKCORE->TGFXCORE.ContentManager.Change_RTSlotTexture = vk_gpudatamanager::Change_RTSlotTexture;
	VKCORE->TGFXCORE.ContentManager.Compile_ShaderSource = vk_gpudatamanager::Compile_ShaderSource;
	VKCORE->TGFXCORE.ContentManager.Create_ComputeInstance = vk_gpudatamanager::Create_ComputeInstance;
	VKCORE->TGFXCORE.ContentManager.Create_ComputeType = vk_gpudatamanager::Create_ComputeType;
	VKCORE->TGFXCORE.ContentManager.Create_GlobalBuffer = vk_gpudatamanager::Create_GlobalBuffer;
	VKCORE->TGFXCORE.ContentManager.Create_IndexBuffer = vk_gpudatamanager::Create_IndexBuffer;
	VKCORE->TGFXCORE.ContentManager.Create_MaterialInst = vk_gpudatamanager::Create_MaterialInst;
	VKCORE->TGFXCORE.ContentManager.Create_RTSlotset = vk_gpudatamanager::Create_RTSlotset;
	VKCORE->TGFXCORE.ContentManager.Create_SamplingType = vk_gpudatamanager::Create_SamplingType;
	VKCORE->TGFXCORE.ContentManager.Create_StagingBuffer = vk_gpudatamanager::Create_StagingBuffer;
	VKCORE->TGFXCORE.ContentManager.Create_Texture = vk_gpudatamanager::Create_Texture;
	VKCORE->TGFXCORE.ContentManager.Create_VertexAttributeLayout = vk_gpudatamanager::Create_VertexAttributeLayout;
	VKCORE->TGFXCORE.ContentManager.Create_VertexBuffer = vk_gpudatamanager::Create_VertexBuffer;
	VKCORE->TGFXCORE.ContentManager.Delete_ComputeShaderInstance = vk_gpudatamanager::Delete_ComputeShaderInstance;
	VKCORE->TGFXCORE.ContentManager.Delete_ComputeShaderType = vk_gpudatamanager::Delete_ComputeShaderType;
	VKCORE->TGFXCORE.ContentManager.Delete_InheritedRTSlotSet = vk_gpudatamanager::Delete_InheritedRTSlotSet;
	VKCORE->TGFXCORE.ContentManager.Delete_MaterialInst = vk_gpudatamanager::Delete_MaterialInst;
	VKCORE->TGFXCORE.ContentManager.Delete_MaterialType = vk_gpudatamanager::Delete_MaterialType;
	VKCORE->TGFXCORE.ContentManager.Delete_RTSlotSet = vk_gpudatamanager::Delete_RTSlotSet;
	VKCORE->TGFXCORE.ContentManager.Delete_ShaderSource = vk_gpudatamanager::Delete_ShaderSource;
	VKCORE->TGFXCORE.ContentManager.Delete_StagingBuffer = vk_gpudatamanager::Delete_StagingBuffer;
	VKCORE->TGFXCORE.ContentManager.Delete_Texture = vk_gpudatamanager::Delete_Texture;
	VKCORE->TGFXCORE.ContentManager.Delete_VertexAttributeLayout = vk_gpudatamanager::Delete_VertexAttributeLayout;
	VKCORE->TGFXCORE.ContentManager.Destroy_AllResources = vk_gpudatamanager::Destroy_AllResources;
	VKCORE->TGFXCORE.ContentManager.Inherite_RTSlotSet = vk_gpudatamanager::Inherite_RTSlotSet;
	VKCORE->TGFXCORE.ContentManager.Link_MaterialType = vk_gpudatamanager::Link_MaterialType;
	VKCORE->TGFXCORE.ContentManager.SetComputeInst_Buffer = vk_gpudatamanager::SetComputeInst_Buffer;
	VKCORE->TGFXCORE.ContentManager.SetComputeInst_Texture = vk_gpudatamanager::SetComputeInst_Texture;
	VKCORE->TGFXCORE.ContentManager.SetComputeType_Buffer = vk_gpudatamanager::SetComputeType_Buffer;
	VKCORE->TGFXCORE.ContentManager.SetComputeType_Texture = vk_gpudatamanager::SetComputeType_Texture;
	VKCORE->TGFXCORE.ContentManager.SetGlobalShaderInput_Buffer = vk_gpudatamanager::SetGlobalShaderInput_Buffer;
	VKCORE->TGFXCORE.ContentManager.SetGlobalShaderInput_Texture = vk_gpudatamanager::SetGlobalShaderInput_Texture;
	VKCORE->TGFXCORE.ContentManager.SetMaterialInst_Buffer = vk_gpudatamanager::SetMaterialInst_Buffer;
	VKCORE->TGFXCORE.ContentManager.SetMaterialInst_Texture = vk_gpudatamanager::SetMaterialInst_Texture;
	VKCORE->TGFXCORE.ContentManager.SetMaterialType_Buffer = vk_gpudatamanager::SetMaterialType_Buffer;
	VKCORE->TGFXCORE.ContentManager.SetMaterialType_Texture = vk_gpudatamanager::SetMaterialType_Texture;
	VKCORE->TGFXCORE.ContentManager.Unload_GlobalBuffer = vk_gpudatamanager::Unload_GlobalBuffer;
	VKCORE->TGFXCORE.ContentManager.Unload_IndexBuffer = vk_gpudatamanager::Unload_IndexBuffer;
	VKCORE->TGFXCORE.ContentManager.Unload_VertexBuffer = vk_gpudatamanager::Unload_VertexBuffer;
	VKCORE->TGFXCORE.ContentManager.Upload_Texture = vk_gpudatamanager::Upload_Texture;
	VKCORE->TGFXCORE.ContentManager.Upload_toBuffer = vk_gpudatamanager::Upload_toBuffer;
}

vk_gpudatamanager::vk_gpudatamanager(InitializationSecondStageInfo& info) :
	MESHBUFFERs(JobSys), INDEXBUFFERs(JobSys), TEXTUREs(JobSys), GLOBALBUFFERs(JobSys), SHADERSOURCEs(JobSys),
	SHADERPROGRAMs(JobSys), SHADERPINSTANCEs(JobSys), VERTEXATTRIBLAYOUTs(JobSys), RT_SLOTSETs(JobSys), STAGINGBUFFERs(JobSys),
	DescSets_toCreateUpdate(JobSys), DescSets_toCreate(JobSys), DescSets_toJustUpdate(JobSys), SAMPLERs(JobSys), IRT_SLOTSETs(JobSys),
	DeleteTextureList(JobSys), NextFrameDeleteTextureCalls(JobSys), COMPUTETYPEs(JobSys), COMPUTEINSTANCEs(JobSys) {
	SetContentManagerFunctionPointers();


	//Do memory allocations
	for (unsigned int allocindex = 0; allocindex < RENDERGPU->ALLOCS().size(); allocindex++) {
		VK_MemoryAllocation& ALLOC = RENDERGPU->ALLOCS()[allocindex];
		if (!ALLOC.ALLOCATIONSIZE) {
			continue;
		}

		VkMemoryRequirements memrequirements;
		VkBufferUsageFlags USAGEFLAGs = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		uint64_t AllocSize = ALLOC.ALLOCATIONSIZE;
		VkBuffer GPULOCAL_buf = Create_VkBuffer(AllocSize, USAGEFLAGs);
		vkGetBufferMemoryRequirements(RENDERGPU->LOGICALDEVICE(), GPULOCAL_buf, &memrequirements);
		if (!(memrequirements.memoryTypeBits & (1 << ALLOC.MemoryTypeIndex))) {
			LOG(tgfx_result_FAIL, "GPU Local Memory Allocation doesn't support the MemoryType!");
			return;
		}
		VkMemoryAllocateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		ci.allocationSize = memrequirements.size;
		ci.memoryTypeIndex = ALLOC.MemoryTypeIndex;
		VkDeviceMemory allocated_memory;
		if (vkAllocateMemory(RENDERGPU->LOGICALDEVICE(), &ci, nullptr, &allocated_memory) != VK_SUCCESS) {
			LOG(tgfx_result_FAIL, "vk_gpudatamanager initialization has failed because vkAllocateMemory GPULocalBuffer has failed!");
		}
		ALLOC.Allocated_Memory = allocated_memory;
		ALLOC.UnusedSize.DirectStore(AllocSize);
		ALLOC.MappedMemory = nullptr;
		ALLOC.Buffer = GPULOCAL_buf;
		if (vkBindBufferMemory(RENDERGPU->LOGICALDEVICE(), GPULOCAL_buf, allocated_memory, 0) != VK_SUCCESS) {
			LOG(tgfx_result_FAIL, "Binding buffer to the allocated memory has failed!");
		}

		//If allocation is device local, it is not mappable. So continue.
		if (ALLOC.TYPE == tgfx_memoryallocationtype::DEVICELOCAL) {
			continue;
		}

		if (vkMapMemory(RENDERGPU->LOGICALDEVICE(), allocated_memory, 0, memrequirements.size, 0, &ALLOC.MappedMemory) != VK_SUCCESS) {
			LOG(tgfx_result_FAIL, "Mapping the HOSTVISIBLE memory has failed!");
			return;
		}
	}



	UnboundDescSetImageCount = 0;
	UnboundDescSetSamplerCount = 0;
	UnboundDescSetSBufferCount = 0;
	UnboundDescSetUBufferCount = 0;

	//Material Related Descriptor Pool Creation
	{
		VkDescriptorPoolCreateInfo dp_ci = {};
		VkDescriptorPoolSize SIZEs[4];
		SIZEs[0].descriptorCount = Find_MeaningfulDescriptorPoolSize(info.MaxSumMaterial_ImageTexture, DescType::IMAGE);
		SIZEs[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		SIZEs[1].descriptorCount = Find_MeaningfulDescriptorPoolSize(info.MaxSumMaterial_StorageBuffer, DescType::SBUFFER);
		SIZEs[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		SIZEs[2].descriptorCount = Find_MeaningfulDescriptorPoolSize(info.MaxSumMaterial_UniformBuffer, DescType::UBUFFER);
		SIZEs[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		SIZEs[3].descriptorCount = Find_MeaningfulDescriptorPoolSize(info.MaxSumMaterial_SampledTexture, DescType::SAMPLER);
		SIZEs[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		dp_ci.maxSets = info.MaxMaterialCount;
		dp_ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		//If descriptor indexing is supported, global descriptor set is only updated (not freed)
		if (RENDERGPU->ExtensionRelatedDatas.ISSUPPORTED_DESCINDEXING()) {
			dp_ci.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		}
		dp_ci.pNext = nullptr;
		dp_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

		dp_ci.poolSizeCount = 4;
		dp_ci.pPoolSizes = SIZEs;
		if (vkCreateDescriptorPool(RENDERGPU->LOGICALDEVICE(), &dp_ci, nullptr, &MaterialRelated_DescPool.pool) != VK_SUCCESS) {
			LOG(tgfx_result_FAIL, "Material Related Descriptor Pool Creation has failed! So GFXContentManager system initialization has failed!");
			return;
		}

		MaterialRelated_DescPool.REMAINING_IMAGE.DirectStore(info.MaxSumMaterial_ImageTexture);
		MaterialRelated_DescPool.REMAINING_SAMPLER.DirectStore(info.MaxSumMaterial_SampledTexture);
		MaterialRelated_DescPool.REMAINING_SBUFFER.DirectStore(info.MaxSumMaterial_StorageBuffer);
		MaterialRelated_DescPool.REMAINING_UBUFFER.DirectStore(info.MaxSumMaterial_UniformBuffer);
		MaterialRelated_DescPool.REMAINING_SET.DirectStore(info.MaxMaterialCount);
	}


	//Create Descriptor Pool and Descriptor Set Layout for Global Shader Inputs
	{
		unsigned int UBUFFER_COUNT = info.GlobalShaderInput_UniformBufferCount, SBUFFER_COUNT = info.GlobalShaderInput_StorageBufferCount,
			SAMPLER_COUNT = info.GlobalShaderInput_SampledTextureCount, IMAGE_COUNT = info.GlobalShaderInput_ImageTextureCount;

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
			if (RENDERGPU->ExtensionRelatedDatas.ISSUPPORTED_DESCINDEXING()) {
				descpool_ci.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
			}
			descpool_ci.pPoolSizes = poolsizes.data();
			descpool_ci.poolSizeCount = poolsizes.size();
			descpool_ci.pNext = nullptr;
			if (vkCreateDescriptorPool(RENDERGPU->LOGICALDEVICE(), &descpool_ci, nullptr, &GlobalShaderInputs_DescPool) != VK_SUCCESS) {
				LOG(tgfx_result_FAIL, "Vulkan Global Descriptor Pool Creation has failed!");
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

			if (info.isUniformBuffer_Index1) {
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
			if (RENDERGPU->ExtensionRelatedDatas.ISSUPPORTED_DESCINDEXING()) {
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
			if (vkCreateDescriptorSetLayout(RENDERGPU->LOGICALDEVICE(), &DescSetLayout_ci, nullptr, &GlobalBuffers_DescSet.Layout) != VK_SUCCESS) {
				LOG(tgfx_result_FAIL, "Create_RenderGraphResources() has failed at vkCreateDescriptorSetLayout()!");
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

			if (info.isSampledTexture_Index1) {
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
			if (RENDERGPU->ExtensionRelatedDatas.ISSUPPORTED_DESCINDEXING()) {
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
			if (vkCreateDescriptorSetLayout(RENDERGPU->LOGICALDEVICE(), &DescSetLayout_ci, nullptr, &GlobalTextures_DescSet.Layout) != VK_SUCCESS) {
				LOG(tgfx_result_FAIL, "Create_RenderGraphResources() has failed at vkCreateDescriptorSetLayout()!");
				return;
			}
		}

		//NOTE: Descriptor Sets are created two times because we don't want to change descriptors while they're being used by GPU
		
		//Create Global Buffer Descriptor Sets
		{
			VkDescriptorSetVariableDescriptorCountAllocateInfo variabledescscount_info = {};
			VkDescriptorSetAllocateInfo descset_ai = {};
			uint32_t UnboundMaxBufferDescCount[2] = { BufferBinding[1].descriptorCount, BufferBinding[1].descriptorCount };
			if (RENDERGPU->ExtensionRelatedDatas.ISSUPPORTED_DESCINDEXING()) {
				variabledescscount_info.descriptorSetCount = 2;
				variabledescscount_info.pDescriptorCounts = UnboundMaxBufferDescCount;
				variabledescscount_info.pNext = nullptr;
				variabledescscount_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
				descset_ai.descriptorPool = GlobalShaderInputs_DescPool;
			}
			descset_ai.pNext = &variabledescscount_info;
			descset_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descset_ai.descriptorSetCount = 2;
			VkDescriptorSetLayout SetLayouts[2]{ GlobalBuffers_DescSet.Layout, GlobalBuffers_DescSet.Layout };
			descset_ai.pSetLayouts = SetLayouts;
			VkDescriptorSet Sets[2];
			if (vkAllocateDescriptorSets(RENDERGPU->LOGICALDEVICE(), &descset_ai, Sets) != VK_SUCCESS) {
				LOG(tgfx_result_FAIL, "CreateandUpdate_GlobalDescSet() has failed at vkAllocateDescriptorSets()!");
			}
			UnboundGlobalBufferDescSet = Sets[0];
			GlobalBuffers_DescSet.Set = Sets[1];
		}

		//Create Global Texture Descriptor Sets
		{
			VkDescriptorSetVariableDescriptorCountAllocateInfo variabledescscount_info = {};
			VkDescriptorSetAllocateInfo descset_ai = {};
			uint32_t MaxUnboundTextureDescCount[2]{ TextureBinding[1].descriptorCount, TextureBinding[1].descriptorCount };
			if (RENDERGPU->ExtensionRelatedDatas.ISSUPPORTED_DESCINDEXING()) {
				variabledescscount_info.descriptorSetCount = 2;
				variabledescscount_info.pDescriptorCounts = MaxUnboundTextureDescCount;
				variabledescscount_info.pNext = nullptr;
				variabledescscount_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
				descset_ai.descriptorPool = GlobalShaderInputs_DescPool;
			}
			descset_ai.pNext = &variabledescscount_info;
			descset_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descset_ai.descriptorSetCount = 2;
			VkDescriptorSetLayout SetLayouts[2]{ GlobalTextures_DescSet.Layout, GlobalTextures_DescSet.Layout };
			descset_ai.pSetLayouts = SetLayouts;
			VkDescriptorSet Sets[2];
			if (vkAllocateDescriptorSets(RENDERGPU->LOGICALDEVICE(), &descset_ai, Sets) != VK_SUCCESS) {
				LOG(tgfx_result_FAIL, "CreateandUpdate_GlobalDescSet() has failed at vkAllocateDescriptorSets()!");
			}
			UnboundGlobalTextureDescSet = Sets[0];
			GlobalTextures_DescSet.Set = Sets[1];
		}

		//Fill Global Buffer Descriptor Set
		{
			VK_Descriptor* BufferFinalDescs = new VK_Descriptor[2];
			BufferFinalDescs[0].ElementCount = BufferBinding[0].descriptorCount;
			BufferFinalDescs[0].Elements = new VK_DescBufferElement[BufferFinalDescs[0].ElementCount];
			BufferFinalDescs[0].Type = info.isUniformBuffer_Index1 ? DescType::SBUFFER : DescType::UBUFFER;
			BufferFinalDescs[1].ElementCount = BufferBinding[1].descriptorCount;
			BufferFinalDescs[1].Elements = new VK_DescBufferElement[BufferFinalDescs[1].ElementCount];
			BufferFinalDescs[1].Type = info.isUniformBuffer_Index1 ? DescType::UBUFFER : DescType::SBUFFER;;

			GlobalBuffers_DescSet.DescCount = UBUFFER_COUNT + SBUFFER_COUNT;
			GlobalBuffers_DescSet.DescUBuffersCount = 1;
			GlobalBuffers_DescSet.DescSBuffersCount = 1;
			GlobalBuffers_DescSet.Descs = BufferFinalDescs;
		}

		//Fill Global Texture Descriptor Set
		{
			VK_Descriptor* TextureFinalDescs = new VK_Descriptor[2];
			TextureFinalDescs[0].ElementCount = TextureBinding[0].descriptorCount;
			TextureFinalDescs[0].Elements = new VK_DescImageElement[TextureFinalDescs[0].ElementCount];
			TextureFinalDescs[0].Type = info.isSampledTexture_Index1 ? DescType::IMAGE : DescType::SAMPLER;
			TextureFinalDescs[1].ElementCount = TextureBinding[1].descriptorCount;
			TextureFinalDescs[1].Elements = new VK_DescImageElement[TextureFinalDescs[1].ElementCount];
			TextureFinalDescs[1].Type = info.isSampledTexture_Index1 ? DescType::SAMPLER : DescType::IMAGE;

			GlobalTextures_DescSet.DescCount = IMAGE_COUNT + SAMPLER_COUNT;
			GlobalTextures_DescSet.DescUBuffersCount = 1;
			GlobalTextures_DescSet.DescSBuffersCount = 1;
			GlobalTextures_DescSet.Descs = TextureFinalDescs;
		}
	}
}


vk_gpudatamanager::~vk_gpudatamanager() {
	Destroy_AllResources();
}
void vk_gpudatamanager::Destroy_RenderGraphRelatedResources() {
	//Destroy Material Types and Instances
	{
		std::unique_lock<std::mutex> Locker;
		SHADERPROGRAMs.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int MatTypeIndex = 0; MatTypeIndex < SHADERPROGRAMs.size(ThreadID); MatTypeIndex++) {
				VK_GraphicsPipeline* PSO = (VK_GraphicsPipeline*)SHADERPROGRAMs.get(ThreadID, MatTypeIndex);
				vkDestroyDescriptorSetLayout(RENDERGPU->LOGICALDEVICE(), PSO->General_DescSet.Layout, nullptr);
				vkDestroyDescriptorSetLayout(RENDERGPU->LOGICALDEVICE(), PSO->Instance_DescSet.Layout, nullptr);
				delete[] PSO->General_DescSet.Descs;
				vkDestroyPipelineLayout(RENDERGPU->LOGICALDEVICE(), PSO->PipelineLayout, nullptr);
				vkDestroyPipeline(RENDERGPU->LOGICALDEVICE(), PSO->PipelineObject, nullptr);
				delete PSO;
			}
			SHADERPROGRAMs.clear(ThreadID);
		}
		Locker.unlock();

		std::unique_lock<std::mutex> InstanceLocker;
		SHADERPINSTANCEs.PauseAllOperations(InstanceLocker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int MatInstIndex = 0; MatInstIndex < SHADERPINSTANCEs.size(ThreadID); MatInstIndex++) {
				VK_PipelineInstance* PSO = (VK_PipelineInstance*)SHADERPINSTANCEs.get(ThreadID, MatInstIndex);
				vkDestroyDescriptorSetLayout(RENDERGPU->LOGICALDEVICE(), PSO->DescSet.Layout, nullptr);
				delete[] PSO->DescSet.Descs;
				delete PSO;
			}
			SHADERPINSTANCEs.clear(ThreadID);
		}
		InstanceLocker.unlock();
	}
	//Destroy Compute Types and Instances
	{
		std::unique_lock<std::mutex> Locker;
		COMPUTETYPEs.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int MatTypeIndex = 0; MatTypeIndex < COMPUTETYPEs.size(ThreadID); MatTypeIndex++) {
				VK_ComputePipeline* PSO = (VK_ComputePipeline*)COMPUTETYPEs.get(ThreadID, MatTypeIndex);
				vkDestroyDescriptorSetLayout(RENDERGPU->LOGICALDEVICE(), PSO->General_DescSet.Layout, nullptr);
				vkDestroyDescriptorSetLayout(RENDERGPU->LOGICALDEVICE(), PSO->Instance_DescSet.Layout, nullptr);
				delete[] PSO->General_DescSet.Descs;
				vkDestroyPipelineLayout(RENDERGPU->LOGICALDEVICE(), PSO->PipelineLayout, nullptr);
				vkDestroyPipeline(RENDERGPU->LOGICALDEVICE(), PSO->PipelineObject, nullptr);
				delete PSO;
			}
			COMPUTETYPEs.clear(ThreadID);
		}
		Locker.unlock();

		std::unique_lock<std::mutex> InstanceLocker;
		COMPUTEINSTANCEs.PauseAllOperations(InstanceLocker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int MatInstIndex = 0; MatInstIndex < COMPUTEINSTANCEs.size(ThreadID); MatInstIndex++) {
				VK_ComputeInstance* PSO = (VK_ComputeInstance*)COMPUTEINSTANCEs.get(ThreadID, MatInstIndex);
				vkDestroyDescriptorSetLayout(RENDERGPU->LOGICALDEVICE(), PSO->DescSet.Layout, nullptr);
				delete[] PSO->DescSet.Descs;
				delete PSO;
			}
			COMPUTEINSTANCEs.clear(ThreadID);
		}
		InstanceLocker.unlock();
	}

	//There is no need to free each descriptor set, just destroy pool
	if (MaterialRelated_DescPool.pool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(RENDERGPU->LOGICALDEVICE(), MaterialRelated_DescPool.pool, nullptr);
		MaterialRelated_DescPool.pool = VK_NULL_HANDLE;
		{
			MaterialRelated_DescPool.REMAINING_IMAGE.DirectStore(0);
			MaterialRelated_DescPool.REMAINING_SAMPLER.DirectStore(0);
			MaterialRelated_DescPool.REMAINING_SBUFFER.DirectStore(0);
			MaterialRelated_DescPool.REMAINING_SET.DirectStore(0);
			MaterialRelated_DescPool.REMAINING_UBUFFER.DirectStore(0);
		}
	}
}
void vk_gpudatamanager::Destroy_AllResources() {
	//Destroy Shader Sources
	{
		std::unique_lock<std::mutex> Locker;
		VKContentManager->SHADERSOURCEs.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int i = 0; i < VKContentManager->SHADERSOURCEs.size(ThreadID); i++) {
				VK_ShaderSource* ShaderSource = VKContentManager->SHADERSOURCEs.get(ThreadID, i);
				vkDestroyShaderModule(RENDERGPU->LOGICALDEVICE(), ShaderSource->Module, nullptr);
				delete ShaderSource;
			}
			VKContentManager->SHADERSOURCEs.clear(ThreadID);
		}
	}
	//Destroy Samplers
	{
		std::unique_lock<std::mutex> Locker;
		VKContentManager->SAMPLERs.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int i = 0; i < VKContentManager->SAMPLERs.size(ThreadID); i++) {
				VK_Sampler* Sampler = VKContentManager->SAMPLERs.get(ThreadID, i);
				vkDestroySampler(RENDERGPU->LOGICALDEVICE(), Sampler->Sampler, nullptr);
				delete Sampler;
			}
			VKContentManager->SAMPLERs.clear(ThreadID);
		}
	}
	//Destroy Vertex Attribute Layouts
	{
		std::unique_lock<std::mutex> Locker;
		VKContentManager->VERTEXATTRIBLAYOUTs.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int i = 0; i < VKContentManager->VERTEXATTRIBLAYOUTs.size(ThreadID); i++) {
				VK_VertexAttribLayout* LAYOUT = VKContentManager->VERTEXATTRIBLAYOUTs.get(ThreadID, i);
				delete[] LAYOUT->AttribDescs;
				delete[] LAYOUT->Attributes;
				delete LAYOUT;
			}
			VKContentManager->VERTEXATTRIBLAYOUTs.clear(ThreadID);
		}
	}
	//Destroy RTSLotSets
	{
		std::unique_lock<std::mutex> Locker;
		VKContentManager->RT_SLOTSETs.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int i = 0; i < VKContentManager->RT_SLOTSETs.size(ThreadID); i++) {
				VK_RTSLOTSET* SlotSet = VKContentManager->RT_SLOTSETs.get(ThreadID, i);
				delete[] SlotSet->PERFRAME_SLOTSETs[0].COLOR_SLOTs;
				delete SlotSet->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT;
				delete[] SlotSet->PERFRAME_SLOTSETs[1].COLOR_SLOTs;
				delete SlotSet->PERFRAME_SLOTSETs[1].DEPTHSTENCIL_SLOT;
				delete SlotSet;
			}
			VKContentManager->RT_SLOTSETs.clear(ThreadID);
		}
	}
	//Destroy IRTSlotSets
	{
		std::unique_lock<std::mutex> Locker;
		VKContentManager->IRT_SLOTSETs.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int i = 0; i < VKContentManager->IRT_SLOTSETs.size(ThreadID); i++) {
				VK_IRTSLOTSET* ISlotSet = VKContentManager->IRT_SLOTSETs.get(ThreadID, i);
				delete[] ISlotSet->COLOR_OPTYPEs;
				delete ISlotSet;
			}
			VKContentManager->IRT_SLOTSETs.clear(ThreadID);
		}
	}
	//Destroy Global Buffers
	{
		std::unique_lock<std::mutex> Locker;
		VKContentManager->GLOBALBUFFERs.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int i = 0; i < VKContentManager->GLOBALBUFFERs.size(ThreadID); i++) {
				delete VKContentManager->GLOBALBUFFERs.get(ThreadID, i);
			}
			VKContentManager->GLOBALBUFFERs.clear(ThreadID);
		}
	}
	//Destroy Mesh Buffers
	{
		std::unique_lock<std::mutex> Locker;
		VKContentManager->MESHBUFFERs.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int i = 0; i < VKContentManager->MESHBUFFERs.size(ThreadID); i++) {
				delete VKContentManager->MESHBUFFERs.get(ThreadID, i);
			}
			VKContentManager->MESHBUFFERs.clear(ThreadID);
		}
	}
	//Destroy Index Buffers
	{
		std::unique_lock<std::mutex> Locker;
		VKContentManager->INDEXBUFFERs.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int i = 0; i < VKContentManager->INDEXBUFFERs.size(ThreadID); i++) {
				delete VKContentManager->INDEXBUFFERs.get(ThreadID, i);
			}
			VKContentManager->INDEXBUFFERs.clear(ThreadID);
		}
	}
	//Destroy Staging Buffers
	{
		std::unique_lock<std::mutex> Locker;
		VKContentManager->STAGINGBUFFERs.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int i = 0; i < VKContentManager->STAGINGBUFFERs.size(ThreadID); i++) {
				delete VKContentManager->STAGINGBUFFERs.get(ThreadID, i);
			}
			VKContentManager->STAGINGBUFFERs.clear(ThreadID);
		}
	}
	//Destroy Textures
	{
		std::unique_lock<std::mutex> Locker;
		VKContentManager->TEXTUREs.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int i = 0; i < VKContentManager->TEXTUREs.size(ThreadID); i++) {
				VK_Texture* Texture = VKContentManager->TEXTUREs.get(ThreadID, i);
				vkDestroyImageView(RENDERGPU->LOGICALDEVICE(), Texture->ImageView, nullptr);
				vkDestroyImage(RENDERGPU->LOGICALDEVICE(), Texture->Image, nullptr);
				delete Texture;
			}
			VKContentManager->TEXTUREs.clear(ThreadID);
		}
	}
	//Destroy Global Buffer related Descriptor Datas
	{
		vkDestroyDescriptorPool(RENDERGPU->LOGICALDEVICE(), VKContentManager->GlobalShaderInputs_DescPool, nullptr);
		vkDestroyDescriptorSetLayout(RENDERGPU->LOGICALDEVICE(), VKContentManager->GlobalBuffers_DescSet.Layout, nullptr);
		vkDestroyDescriptorSetLayout(RENDERGPU->LOGICALDEVICE(), VKContentManager->GlobalTextures_DescSet.Layout, nullptr);
	}
}

void Update_GlobalDescSet_DescIndexing(VK_DescSet& Set, VkDescriptorPool Pool) {
	//Update Descriptor Sets
	{
		std::vector<VkWriteDescriptorSet> UpdateInfos;

		for (unsigned int BindingIndex = 0; BindingIndex < 2; BindingIndex++) {
			VK_Descriptor& Desc = Set.Descs[BindingIndex];
			if (Desc.Type == DescType::IMAGE || Desc.Type == DescType::SAMPLER) {
				for (unsigned int DescElementIndex = 0; DescElementIndex < Desc.ElementCount; DescElementIndex++) {
					VK_DescImageElement& im = ((VK_DescImageElement*)Desc.Elements)[DescElementIndex];
					if (im.IsUpdated.load() == 255) {
						continue;
					}
					im.IsUpdated.store(0);

					VkWriteDescriptorSet UpdateInfo = {};
					UpdateInfo.descriptorCount = 1;
					UpdateInfo.descriptorType = Find_VkDescType_byDescTypeCategoryless(Desc.Type);
					UpdateInfo.dstArrayElement = DescElementIndex;
					UpdateInfo.dstBinding = BindingIndex;
					UpdateInfo.dstSet = Set.Set;
					UpdateInfo.pImageInfo = &im.info;
					UpdateInfo.pNext = nullptr;
					UpdateInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					UpdateInfos.push_back(UpdateInfo);
				}
			}
			else {
				for (unsigned int DescElementIndex = 0; DescElementIndex < Desc.ElementCount; DescElementIndex++) {
					VK_DescBufferElement& buf = ((VK_DescBufferElement*)Desc.Elements)[DescElementIndex];
					if (buf.IsUpdated.load() == 255) {
						continue;
					}
					buf.IsUpdated.store(0);

					VkWriteDescriptorSet UpdateInfo = {};
					UpdateInfo.descriptorCount = 1;
					UpdateInfo.descriptorType = Find_VkDescType_byDescTypeCategoryless(Desc.Type);
					UpdateInfo.dstArrayElement = DescElementIndex;
					UpdateInfo.dstBinding = BindingIndex;
					UpdateInfo.dstSet = Set.Set;
					UpdateInfo.pBufferInfo = &buf.Info;
					UpdateInfo.pNext = nullptr;
					UpdateInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					UpdateInfos.push_back(UpdateInfo);
				}
			}
		}

		vkUpdateDescriptorSets(RENDERGPU->LOGICALDEVICE(), UpdateInfos.size(), UpdateInfos.data(), 0, nullptr);
	}
}
void Update_GlobalDescSet_NonDescIndexing(VK_DescSet& Set, VkDescriptorPool Pool) {
	//Update Descriptor Sets
	{
		std::vector<VkWriteDescriptorSet> UpdateInfos;

		for (unsigned int BindingIndex = 0; BindingIndex < 2; BindingIndex++) {
			VK_Descriptor& Desc = Set.Descs[BindingIndex];
			if (Desc.Type == DescType::IMAGE || Desc.Type == DescType::SAMPLER) {
				for (unsigned int DescElementIndex = 0; DescElementIndex < Desc.ElementCount; DescElementIndex++) {
					VK_DescImageElement& im = ((VK_DescImageElement*)Desc.Elements)[DescElementIndex];
					im.IsUpdated.store(0);

					VkWriteDescriptorSet UpdateInfo = {};
					UpdateInfo.descriptorCount = 1;
					UpdateInfo.descriptorType = Find_VkDescType_byDescTypeCategoryless(Desc.Type);
					UpdateInfo.dstArrayElement = DescElementIndex;
					UpdateInfo.dstBinding = BindingIndex;
					UpdateInfo.dstSet = Set.Set;
					UpdateInfo.pImageInfo = &im.info;
					UpdateInfo.pNext = nullptr;
					UpdateInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					UpdateInfos.push_back(UpdateInfo);
				}
			}
			else {
				for (unsigned int DescElementIndex = 0; DescElementIndex < Desc.ElementCount; DescElementIndex++) {
					VK_DescBufferElement& buf = ((VK_DescBufferElement*)Desc.Elements)[DescElementIndex];
					buf.IsUpdated.store(0);

					VkWriteDescriptorSet UpdateInfo = {};
					UpdateInfo.descriptorCount = 1;
					UpdateInfo.descriptorType = Find_VkDescType_byDescTypeCategoryless(Desc.Type);
					UpdateInfo.dstArrayElement = DescElementIndex;
					UpdateInfo.dstBinding = BindingIndex;
					UpdateInfo.dstSet = Set.Set;
					UpdateInfo.pBufferInfo = &buf.Info;
					UpdateInfo.pNext = nullptr;
					UpdateInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					UpdateInfos.push_back(UpdateInfo);
				}
			}
		}

		vkUpdateDescriptorSets(RENDERGPU->LOGICALDEVICE(), UpdateInfos.size(), UpdateInfos.data(), 0, nullptr);
	}
}
void CopyDescriptorSets(VK_DescSet& Set, std::vector<VkCopyDescriptorSet>& CopyVector, std::vector<VkDescriptorSet*>& CopyTargetSets) {
	for (unsigned int DescIndex = 0; DescIndex < Set.DescCount; DescIndex++) {
		VkCopyDescriptorSet copyinfo;
		VK_Descriptor& SourceDesc = ((VK_Descriptor*)Set.Descs)[DescIndex];
		copyinfo.pNext = nullptr;
		copyinfo.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
		copyinfo.descriptorCount = SourceDesc.ElementCount;
		//We will copy all descriptor array's elements
		copyinfo.srcArrayElement = 0;
		copyinfo.dstArrayElement = 0;
		copyinfo.dstBinding = DescIndex;
		copyinfo.srcBinding = DescIndex;
		copyinfo.srcSet = Set.Set;
		//We will fill this DistanceSet after creating it!
		copyinfo.dstSet = VK_NULL_HANDLE;
		CopyVector.push_back(copyinfo);
		CopyTargetSets.push_back(&Set.Set);
	}
}
void vk_gpudatamanager::Apply_ResourceChanges() {
	const unsigned char FrameIndex = VKRENDERER->GetCurrentFrameIndex();


	//Manage Global Descriptor Set
	if (GlobalBuffers_DescSet.ShouldRecreate.load()) {
		VkDescriptorSet backSet = UnboundGlobalBufferDescSet;
		UnboundGlobalBufferDescSet = GlobalBuffers_DescSet.Set;
		GlobalBuffers_DescSet.Set = backSet;
		GlobalBuffers_DescSet.ShouldRecreate.store(false);
	}
	if (RENDERGPU->ExtensionRelatedDatas.ISSUPPORTED_DESCINDEXING()) {
		Update_GlobalDescSet_DescIndexing(GlobalBuffers_DescSet, GlobalShaderInputs_DescPool);
	}
	else {
		Update_GlobalDescSet_NonDescIndexing(GlobalBuffers_DescSet, GlobalShaderInputs_DescPool);
	}
	if (GlobalTextures_DescSet.ShouldRecreate.load()) {
		VkDescriptorSet backSet = UnboundGlobalTextureDescSet;
		UnboundGlobalTextureDescSet = GlobalTextures_DescSet.Set;
		GlobalTextures_DescSet.Set = backSet;
		GlobalTextures_DescSet.ShouldRecreate.store(false);
	}
	if (RENDERGPU->ExtensionRelatedDatas.ISSUPPORTED_DESCINDEXING()) {
		Update_GlobalDescSet_DescIndexing(GlobalTextures_DescSet, GlobalShaderInputs_DescPool);
	}
	else {
		Update_GlobalDescSet_NonDescIndexing(GlobalTextures_DescSet, GlobalShaderInputs_DescPool);
	}

	//Manage Material Related Descriptor Sets
	{
		//Destroy descriptor sets that are changed last frame
		if (UnboundMaterialDescSetList.size()) {
			VK_DescPool& DP = MaterialRelated_DescPool;
			vkFreeDescriptorSets(RENDERGPU->LOGICALDEVICE(), DP.pool, UnboundMaterialDescSetList.size(),
				UnboundMaterialDescSetList.data());
			DP.REMAINING_SET.DirectAdd(UnboundMaterialDescSetList.size());

			DP.REMAINING_IMAGE.DirectAdd(UnboundDescSetImageCount);
			UnboundDescSetImageCount = 0;
			DP.REMAINING_SAMPLER.DirectAdd(UnboundDescSetSamplerCount);
			UnboundDescSetSamplerCount = 0;
			DP.REMAINING_SBUFFER.DirectAdd(UnboundDescSetSBufferCount);
			UnboundDescSetSBufferCount = 0;
			DP.REMAINING_UBUFFER.DirectAdd(UnboundDescSetUBufferCount);
			UnboundDescSetUBufferCount = 0;
			UnboundMaterialDescSetList.clear();
		}
		//Create Descriptor Sets for material types/instances that are created this frame
		{
			std::vector<VkDescriptorSet> Sets;
			std::vector<VkDescriptorSet*> SetPTRs;
			std::vector<VkDescriptorSetLayout> SetLayouts;
			std::unique_lock<std::mutex> Locker;
			DescSets_toCreate.PauseAllOperations(Locker);
			for (unsigned int ThreadIndex = 0; ThreadIndex < tapi_GetThreadCount(JobSys); ThreadIndex++) {
				for (unsigned int SetIndex = 0; SetIndex < DescSets_toCreate.size(ThreadIndex); SetIndex++) {
					VK_DescSet* Set = DescSets_toCreate.get(ThreadIndex, SetIndex);
					Sets.push_back(VkDescriptorSet());
					SetLayouts.push_back(Set->Layout);
					SetPTRs.push_back(&Set->Set);
				}
				DescSets_toCreate.clear(ThreadIndex);
			}
			Locker.unlock();

			if (Sets.size()) {
				VkDescriptorSetAllocateInfo al_in = {};
				al_in.descriptorPool = MaterialRelated_DescPool.pool;
				al_in.descriptorSetCount = Sets.size();
				al_in.pNext = nullptr;
				al_in.pSetLayouts = SetLayouts.data();
				al_in.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				vkAllocateDescriptorSets(RENDERGPU->LOGICALDEVICE(), &al_in, Sets.data());
				for (unsigned int SetIndex = 0; SetIndex < Sets.size(); SetIndex++) {
					*SetPTRs[SetIndex] = Sets[SetIndex];
				}
			}
		}
		//Create Descriptor Sets for material types/instances that are created before and used recently (last 2 frames), to update their descriptor sets
		{
			std::vector<VkDescriptorSet> NewSets;
			std::vector<VkDescriptorSet*> SetPTRs;
			std::vector<VkDescriptorSetLayout> SetLayouts;

			//Copy descriptor sets exactly, then update with this frame's SetMaterial_xxx calls
			std::vector<VkCopyDescriptorSet> CopySetInfos;
			std::vector<VkDescriptorSet*> CopyTargetSets;


			std::unique_lock<std::mutex> Locker;
			DescSets_toCreateUpdate.PauseAllOperations(Locker);
			for (unsigned int ThreadIndex = 0; ThreadIndex < tapi_GetThreadCount(JobSys); ThreadIndex++) {
				for (unsigned int SetIndex = 0; SetIndex < DescSets_toCreateUpdate.size(ThreadIndex); SetIndex++) {
					VK_DescSetUpdateCall& Call = DescSets_toCreateUpdate.get(ThreadIndex, SetIndex);
					VK_DescSet* Set = Call.Set;
					bool SetStatus = Set->ShouldRecreate.load();
					switch (SetStatus) {
					case 0:
						continue;
					case 1:
						NewSets.push_back(VkDescriptorSet());
						SetPTRs.push_back(&Set->Set);
						SetLayouts.push_back(Set->Layout);
						UnboundDescSetImageCount += Set->DescImagesCount;
						UnboundDescSetSamplerCount += Set->DescSamplersCount;
						UnboundDescSetSBufferCount += Set->DescSBuffersCount;
						UnboundDescSetUBufferCount += Set->DescUBuffersCount;
						UnboundMaterialDescSetList.push_back(Set->Set);

						CopyDescriptorSets(*Set, CopySetInfos, CopyTargetSets);
						Set->ShouldRecreate.store(0);
						break;
					default:
						LOG(tgfx_result_NOTCODED, "Descriptor Set atomic_uchar isn't supposed to have a value that's 2+! Please check 'Handle Descriptor Sets' in Vulkan Renderer->Run()");
						break;
					}
				}
			}

			if (NewSets.size()) {
				VkDescriptorSetAllocateInfo al_in = {};
				al_in.descriptorPool = MaterialRelated_DescPool.pool;
				al_in.descriptorSetCount = NewSets.size();
				al_in.pNext = nullptr;
				al_in.pSetLayouts = SetLayouts.data();
				al_in.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				vkAllocateDescriptorSets(RENDERGPU->LOGICALDEVICE(), &al_in, NewSets.data());
				for (unsigned int SetIndex = 0; SetIndex < NewSets.size(); SetIndex++) {
					*SetPTRs[SetIndex] = NewSets[SetIndex];
				}

				for (unsigned int CopyIndex = 0; CopyIndex < CopySetInfos.size(); CopyIndex++) {
					CopySetInfos[CopyIndex].dstSet = *CopyTargetSets[CopyIndex];
				}
				vkUpdateDescriptorSets(RENDERGPU->LOGICALDEVICE(), 0, nullptr, CopySetInfos.size(), CopySetInfos.data());
			}
		}
		//Update descriptor sets
		{
			std::vector<VkWriteDescriptorSet> UpdateInfos;
			std::unique_lock<std::mutex> Locker1;
			DescSets_toCreateUpdate.PauseAllOperations(Locker1);
			for (unsigned int ThreadIndex = 0; ThreadIndex < tapi_GetThreadCount(JobSys); ThreadIndex++) {
				for (unsigned int CallIndex = 0; CallIndex < DescSets_toCreateUpdate.size(ThreadIndex); CallIndex++) {
					VK_DescSetUpdateCall& Call = DescSets_toCreateUpdate.get(ThreadIndex, CallIndex);
					VkWriteDescriptorSet info = {};
					info.descriptorCount = 1;
					VK_Descriptor& Desc = Call.Set->Descs[Call.BindingIndex];
					switch (Desc.Type) {
					case DescType::IMAGE:
						info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pImageInfo = &((VK_DescImageElement*)Desc.Elements)[Call.ElementIndex].info;
						((VK_DescImageElement*) Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case DescType::SAMPLER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pImageInfo = &((VK_DescImageElement*)Desc.Elements)[Call.ElementIndex].info;
						((VK_DescImageElement*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case DescType::UBUFFER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pBufferInfo = &((VK_DescBufferElement*)Desc.Elements)[Call.ElementIndex].Info;
						((VK_DescBufferElement*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case DescType::SBUFFER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pBufferInfo = &((VK_DescBufferElement*)Desc.Elements)[Call.ElementIndex].Info;
						((VK_DescBufferElement*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					}
					info.dstSet = Call.Set->Set;
					info.pNext = nullptr;
					info.pTexelBufferView = nullptr;
					info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					UpdateInfos.push_back(info);
				}
				DescSets_toCreateUpdate.clear(ThreadIndex);
			}
			Locker1.unlock();


			std::unique_lock<std::mutex> Locker2;
			DescSets_toJustUpdate.PauseAllOperations(Locker2);
			for (unsigned int ThreadIndex = 0; ThreadIndex < tapi_GetThreadCount(JobSys); ThreadIndex++) {
				for (unsigned int CallIndex = 0; CallIndex < DescSets_toJustUpdate.size(ThreadIndex); CallIndex++) {
					VK_DescSetUpdateCall& Call = DescSets_toJustUpdate.get(ThreadIndex, CallIndex);
					VkWriteDescriptorSet info = {};
					info.descriptorCount = 1;
					VK_Descriptor& Desc = Call.Set->Descs[Call.BindingIndex];
					switch (Desc.Type) {
					case DescType::IMAGE:
						info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pImageInfo = &((VK_DescImageElement*)Desc.Elements)[Call.ElementIndex].info;
						((VK_DescImageElement*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case DescType::SAMPLER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pImageInfo = &((VK_DescImageElement*)Desc.Elements)[Call.ElementIndex].info;
						((VK_DescImageElement*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case DescType::UBUFFER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pBufferInfo = &((VK_DescBufferElement*)Desc.Elements)[Call.ElementIndex].Info;
						((VK_DescBufferElement*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case DescType::SBUFFER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pBufferInfo = &((VK_DescBufferElement*)Desc.Elements)[Call.ElementIndex].Info;
						((VK_DescBufferElement*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					}
					info.dstSet = Call.Set->Set;
					info.pNext = nullptr;
					info.pTexelBufferView = nullptr;
					info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					UpdateInfos.push_back(info);
				}
				DescSets_toJustUpdate.clear(ThreadIndex);
			}
			Locker2.unlock();

			vkUpdateDescriptorSets(RENDERGPU->LOGICALDEVICE(), UpdateInfos.size(), UpdateInfos.data(), 0, nullptr);
		}
	}

	//Handle RTSlotSet changes by recreating framebuffers of draw passes
	//by "RTSlotSet changes": DrawPass' slotset is changed to different one or slotset's slots is changed.
	for (unsigned int DrawPassIndex = 0; DrawPassIndex < VKRENDERER->DrawPasses.size(); DrawPassIndex++) {
		drawpass_vk* DP = VKRENDERER->DrawPasses[DrawPassIndex];
		unsigned char ChangeInfo = DP->SlotSetChanged.load();
		if (ChangeInfo == FrameIndex + 1 || ChangeInfo == 3 || DP->SLOTSET->PERFRAME_SLOTSETs[FrameIndex].IsChanged.load()) {
			//This is safe because this FB is used 2 frames ago and CPU already waits for the frame's GPU process to end
			vkDestroyFramebuffer(RENDERGPU->LOGICALDEVICE(), DP->FBs[FrameIndex], nullptr);

			VkFramebufferCreateInfo fb_ci = DP->SLOTSET->FB_ci[FrameIndex];
			fb_ci.renderPass = DP->RenderPassObject;
			if (vkCreateFramebuffer(RENDERGPU->LOGICALDEVICE(), &fb_ci, nullptr, &DP->FBs[FrameIndex]) != VK_SUCCESS) {
				LOG(tgfx_result_FAIL, "vkCreateFramebuffer() has failed while changing one of the drawpasses' current frame slot's texture! Please report this!");
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

	//Delete textures
	{
		std::unique_lock<std::mutex> Locker;
		DeleteTextureList.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int i = 0; i < DeleteTextureList.size(ThreadID); i++) {
				VK_Texture* Texture = DeleteTextureList.get(ThreadID, i);
				if (Texture->Image) {
					vkDestroyImageView(RENDERGPU->LOGICALDEVICE(), Texture->ImageView, nullptr);
					vkDestroyImage(RENDERGPU->LOGICALDEVICE(), Texture->Image, nullptr);
				}
				if (Texture->Block.MemAllocIndex != UINT32_MAX) {
					RENDERGPU->ALLOCS()[Texture->Block.MemAllocIndex].FreeBlock(Texture->Block.Offset);
				}
				delete Texture;
			}
			DeleteTextureList.clear(ThreadID);
		}
	}
	//Push next frame delete texture list to the delete textures list
	{
		std::unique_lock<std::mutex> Locker;
		NextFrameDeleteTextureCalls.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
			for (unsigned int i = 0; i < NextFrameDeleteTextureCalls.size(ThreadID); i++) {
				DeleteTextureList.push_back(0, NextFrameDeleteTextureCalls.get(ThreadID, i));
			}
			NextFrameDeleteTextureCalls.clear(ThreadID);
		}
	}


}

void vk_gpudatamanager::Resource_Finalizations() {
	//If Descriptor Indexing isn't supported, check if any global descriptor isn't set before!
	if (!RENDERGPU->ExtensionRelatedDatas.ISSUPPORTED_DESCINDEXING()) {
		for (unsigned char BindingIndex = 0; BindingIndex < 2; BindingIndex++) {
			VK_Descriptor& BufferBinding = GlobalBuffers_DescSet.Descs[BindingIndex];
			for (unsigned int ElementIndex = 0; ElementIndex < BufferBinding.ElementCount; ElementIndex++) {
				VK_DescBufferElement& element = ((VK_DescBufferElement*)BufferBinding.Elements)[ElementIndex];
				if (element.IsUpdated.load() == 255) {
					LOG(tgfx_result_FAIL, ("You have to use SetGlobalBuffer() for element index: " + std::to_string(ElementIndex)).c_str());
					return;
				}
			}
			VK_Descriptor& TextureBinding = GlobalTextures_DescSet.Descs[BindingIndex];
			for (unsigned int ElementIndex = 0; ElementIndex < TextureBinding.ElementCount; ElementIndex++) {
				VK_DescImageElement& element = ((VK_DescImageElement*)TextureBinding.Elements)[ElementIndex];
				if (element.IsUpdated.load() == 255) {
					LOG(tgfx_result_FAIL, ("You have to use SetGlobalTexture() for element index: " + std::to_string(ElementIndex)).c_str());
					return;
				}
			}
		}
	}

	if (RENDERGPU->ExtensionRelatedDatas.ISSUPPORTED_DESCINDEXING()) {
		Update_GlobalDescSet_DescIndexing(GlobalBuffers_DescSet, GlobalShaderInputs_DescPool);
		Update_GlobalDescSet_DescIndexing(GlobalTextures_DescSet, GlobalShaderInputs_DescPool);
	}
	else {
		Update_GlobalDescSet_NonDescIndexing(GlobalBuffers_DescSet, GlobalShaderInputs_DescPool);
		Update_GlobalDescSet_NonDescIndexing(GlobalTextures_DescSet, GlobalShaderInputs_DescPool);
	}
}

tgfx_result vk_gpudatamanager::Suballocate_Buffer(VkBuffer BUFFER, VkBufferUsageFlags UsageFlags, MemoryBlock& Block) {
	VkMemoryRequirements bufferreq;
	vkGetBufferMemoryRequirements(RENDERGPU->LOGICALDEVICE(), BUFFER, &bufferreq);

	VK_MemoryAllocation& MEMALLOC = RENDERGPU->ALLOCS()[Block.MemAllocIndex];

	if (!(bufferreq.memoryTypeBits & (1u << MEMALLOC.MemoryTypeIndex))) {
		LOG(tgfx_result_FAIL, "Intended buffer doesn't support to be stored in specified memory region!");
		return tgfx_result_FAIL;
	}

	VkDeviceSize AlignmentOffset_ofGPU = 0;
	if (UsageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
		AlignmentOffset_ofGPU = RENDERGPU->DEVICEPROPERTIES().limits.minStorageBufferOffsetAlignment;
	}
	if (UsageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
		AlignmentOffset_ofGPU = RENDERGPU->DEVICEPROPERTIES().limits.minUniformBufferOffsetAlignment;
	}
	if (!MEMALLOC.UnusedSize.LimitedSubtract_weak(bufferreq.size + AlignmentOffset_ofGPU, 0)) {
		LOG(tgfx_result_FAIL, "Buffer doesn't fit the remaining memory allocation! SuballocateBuffer has failed.");
		return tgfx_result_FAIL;
	}


	Block.Offset = MEMALLOC.FindAvailableOffset(bufferreq.size, AlignmentOffset_ofGPU, bufferreq.alignment);
	return tgfx_result_SUCCESS;
}
VkMemoryPropertyFlags ConvertGFXMemoryType_toVulkan(tgfx_memoryallocationtype type) {
	switch (type) {
	case tgfx_memoryallocationtype::DEVICELOCAL:
		return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	case tgfx_memoryallocationtype::FASTHOSTVISIBLE:
		return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	case tgfx_memoryallocationtype::HOSTVISIBLE:
		return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	case tgfx_memoryallocationtype::READBACK:
		return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
	default:
		LOG(tgfx_result_NOTCODED, "ConvertGFXMemoryType_toVulkan() doesn't support this type!");
		return 0;
	}
}
tgfx_result vk_gpudatamanager::Suballocate_Image(VK_Texture& Texture) {
	VkMemoryRequirements req;
	vkGetImageMemoryRequirements(RENDERGPU->LOGICALDEVICE(), Texture.Image, &req);
	VkDeviceSize size = req.size;

	VkDeviceSize Offset = 0;
	bool Found_Offset = false;
	VK_MemoryAllocation* MEMALLOC = &RENDERGPU->ALLOCS()[Texture.Block.MemAllocIndex];
	if (!(req.memoryTypeBits & (1u << MEMALLOC->MemoryTypeIndex))) {
		LOG(tgfx_result_FAIL, "Intended texture doesn't support to be stored in the specified memory region!");
		return tgfx_result_FAIL;
	}
	if (!MEMALLOC->UnusedSize.LimitedSubtract_weak(req.size, 0)) {
		LOG(tgfx_result_FAIL, "Buffer doesn't fit the remaining memory allocation! SuballocateBuffer has failed.");
		return tgfx_result_FAIL;
	}
	Offset = MEMALLOC->FindAvailableOffset(req.size, 0, req.alignment);

	if (vkBindImageMemory(RENDERGPU->LOGICALDEVICE(), Texture.Image, MEMALLOC->Allocated_Memory, Offset) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "VKContentManager->Suballocate_Image() has failed at VkBindImageMemory()!");
		return tgfx_result_FAIL;
	}
	Texture.Block.Offset = Offset;
	return tgfx_result_SUCCESS;
}

unsigned int Calculate_sizeofVertexLayout(const tgfx_datatype* ATTRIBUTEs, unsigned int count) {
	unsigned int size = 0;
	for (unsigned int i = 0; i < count; i++) {
		size += tgfx_Get_UNIFORMTYPEs_SIZEinbytes(ATTRIBUTEs[i]);
	}
	return size;
}
tgfx_result vk_gpudatamanager::Create_SamplingType(unsigned int MinimumMipLevel, unsigned int MaximumMipLevel,
	tgfx_texture_mipmapfilter MINFILTER, tgfx_texture_mipmapfilter MAGFILTER, tgfx_texture_wrapping WRAPPING_WIDTH,
	tgfx_texture_wrapping WRAPPING_HEIGHT, tgfx_texture_wrapping WRAPPING_DEPTH, tgfx_samplingtype* SamplingTypeHandle) {
	VkSamplerCreateInfo s_ci = {};
	s_ci.addressModeU = Find_AddressMode_byWRAPPING(WRAPPING_WIDTH);
	s_ci.addressModeV = Find_AddressMode_byWRAPPING(WRAPPING_HEIGHT);
	s_ci.addressModeW = Find_AddressMode_byWRAPPING(WRAPPING_DEPTH);
	s_ci.anisotropyEnable = VK_FALSE;
	s_ci.borderColor = VkBorderColor::VK_BORDER_COLOR_MAX_ENUM;
	s_ci.compareEnable = VK_FALSE;
	s_ci.flags = 0;
	s_ci.magFilter = Find_VkFilter_byGFXFilter(MAGFILTER);
	s_ci.minFilter = Find_VkFilter_byGFXFilter(MINFILTER);
	s_ci.maxLod = static_cast<float>(MaximumMipLevel);
	s_ci.minLod = static_cast<float>(MinimumMipLevel);
	s_ci.mipLodBias = 0.0f;
	s_ci.mipmapMode = Find_MipmapMode_byGFXFilter(MINFILTER);
	s_ci.pNext = nullptr;
	s_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	s_ci.unnormalizedCoordinates = VK_FALSE;

	VK_Sampler* SAMPLER = new VK_Sampler;
	if (vkCreateSampler(RENDERGPU->LOGICALDEVICE(), &s_ci, nullptr, &SAMPLER->Sampler) != VK_SUCCESS) {
		delete SAMPLER;
		LOG(tgfx_result_FAIL, "GFXContentManager->Create_SamplingType() has failed at vkCreateSampler!");
		return tgfx_result_FAIL;
	}
	*SamplingTypeHandle = (tgfx_samplingtype)SAMPLER;
	VKContentManager->SAMPLERs.push_back(tapi_GetThisThreadIndex(JobSys), SAMPLER);
	return tgfx_result_SUCCESS;
}

bool vk_gpudatamanager::Create_DescSet(VK_DescSet* Set) {
	if (!Set->DescCount) {
		return true;
	}
	if (!MaterialRelated_DescPool.REMAINING_IMAGE.LimitedSubtract_weak(Set->DescImagesCount, 0) ||
		!MaterialRelated_DescPool.REMAINING_SAMPLER.LimitedSubtract_weak(Set->DescSamplersCount, 0) ||
		!MaterialRelated_DescPool.REMAINING_SBUFFER.LimitedSubtract_weak(Set->DescSBuffersCount, 0) ||
		!MaterialRelated_DescPool.REMAINING_UBUFFER.LimitedSubtract_weak(Set->DescUBuffersCount, 0) ||
		!MaterialRelated_DescPool.REMAINING_SET.LimitedSubtract_weak(1, 0)) {
		LOG(tgfx_result_FAIL, "Create_DescSets() has failed because descriptor pool doesn't have enough space!");
		return false;
	}
	DescSets_toCreate.push_back(tapi_GetThisThreadIndex(JobSys), Set);
	return true;
}
//General DescriptorSet Layout Creation
bool VKDescSet_PipelineLayoutCreation(const VK_ShaderInputDesc** inputdescs, VK_DescSet* GeneralDescSet, VK_DescSet* InstanceDescSet, VkPipelineLayout* layout) {
	TGFXLISTCOUNT((&VKCORE->TGFXCORE), inputdescs, inputdesccount);
	//General DescSet creation
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		for (unsigned int i = 0; i < inputdesccount; i++) {
			const VK_ShaderInputDesc* desc = inputdescs[i];
			if (inputdescs[i] == VKCORE->TGFXCORE.INVALIDHANDLE) {
				continue;
			}
			if (!(desc->TYPE == tgfx_shaderinput_type_IMAGE_G ||
				desc->TYPE == tgfx_shaderinput_type_SAMPLER_G ||
				desc->TYPE == tgfx_shaderinput_type_SBUFFER_G ||
				desc->TYPE == tgfx_shaderinput_type_UBUFFER_G)) {
				continue;
			}

			unsigned int BP = desc->BINDINDEX;
			for (unsigned int bpsearchindex = 0; bpsearchindex < bindings.size(); bpsearchindex++) {
				if (BP == bindings[bpsearchindex].binding) {
					LOG(tgfx_result_FAIL, "VKDescSet_PipelineLayoutCreation() has failed because there are colliding binding points!");
					return false;
				}
			}

			if (!desc->ELEMENTCOUNT) {
				LOG(tgfx_result_FAIL, "VKDescSet_PipelineLayoutCreation() has failed because one of the shader inputs have 0 element count!");
				return false;
			}

			VkDescriptorSetLayoutBinding bn = {};
			bn.stageFlags = desc->ShaderStages.flag;
			bn.pImmutableSamplers = VK_NULL_HANDLE;
			bn.descriptorType = Find_VkDescType_byMATDATATYPE(desc->TYPE);
			bn.descriptorCount = desc->ELEMENTCOUNT;
			bn.binding = BP;
			bindings.push_back(bn);

			GeneralDescSet->DescCount++;
			switch (desc->TYPE) {
			case tgfx_shaderinput_type_IMAGE_G:
				GeneralDescSet->DescImagesCount += desc->ELEMENTCOUNT;
				break;
			case tgfx_shaderinput_type_SAMPLER_G:
				GeneralDescSet->DescSamplersCount += desc->ELEMENTCOUNT;
				break;
			case tgfx_shaderinput_type_UBUFFER_G:
				GeneralDescSet->DescUBuffersCount += desc->ELEMENTCOUNT;
				break;
			case tgfx_shaderinput_type_SBUFFER_G:
				GeneralDescSet->DescSBuffersCount += desc->ELEMENTCOUNT;
				break;
			}
		}

		if (GeneralDescSet->DescCount) {
			GeneralDescSet->Descs = new VK_Descriptor[GeneralDescSet->DescCount];
			for (unsigned int i = 0; i < inputdesccount; i++) {
				const VK_ShaderInputDesc* desc = inputdescs[i];
				if (!(desc->TYPE == tgfx_shaderinput_type_IMAGE_G ||
					desc->TYPE == tgfx_shaderinput_type_SAMPLER_G ||
					desc->TYPE == tgfx_shaderinput_type_SBUFFER_G ||
					desc->TYPE == tgfx_shaderinput_type_UBUFFER_G)) {
					continue;
				}
				if (desc->BINDINDEX >= GeneralDescSet->DescCount) {
					LOG(tgfx_result_FAIL, "One of your General shaderinputs uses a binding point that is exceeding the number of general shaderinputs. You have to use a binding point that's lower than size of the general shader inputs!");
					return false;
				}

				VK_Descriptor& vkdesc = GeneralDescSet->Descs[desc->BINDINDEX];
				switch (desc->TYPE) {
				case tgfx_shaderinput_type_IMAGE_G:
				{
					vkdesc.ElementCount = desc->ELEMENTCOUNT;
					vkdesc.Elements = new VK_DescImageElement[desc->ELEMENTCOUNT];
					vkdesc.Type = DescType::IMAGE;
				}
				break;
				case tgfx_shaderinput_type_SAMPLER_G:
				{
					vkdesc.ElementCount = desc->ELEMENTCOUNT;
					vkdesc.Elements = new VK_DescImageElement[desc->ELEMENTCOUNT];
					vkdesc.Type = DescType::SAMPLER;
				}
				break;
				case tgfx_shaderinput_type_UBUFFER_G:
				{
					vkdesc.ElementCount = desc->ELEMENTCOUNT;
					vkdesc.Elements = new VK_DescBufferElement[desc->ELEMENTCOUNT];
					vkdesc.Type = DescType::UBUFFER;
				}
				break;
				case tgfx_shaderinput_type_SBUFFER_G:
				{
					vkdesc.ElementCount = desc->ELEMENTCOUNT;
					vkdesc.Elements = new VK_DescBufferElement[desc->ELEMENTCOUNT];
					vkdesc.Type = DescType::SBUFFER;
				}
				break;
				}
			}
			GeneralDescSet->ShouldRecreate.store(0);
		}

		if (bindings.size()) {
			VkDescriptorSetLayoutCreateInfo ci = {};
			ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			ci.pNext = nullptr;
			ci.flags = 0;
			ci.bindingCount = bindings.size();
			ci.pBindings = bindings.data();

			if (vkCreateDescriptorSetLayout(RENDERGPU->LOGICALDEVICE(), &ci, nullptr, &GeneralDescSet->Layout) != VK_SUCCESS) {
				LOG(tgfx_result_FAIL, "VKDescSet_PipelineLayoutCreation() has failed at General DescriptorSetLayout Creation vkCreateDescriptorSetLayout()");
				return false;
			}
		}
	}

	//Instance DescriptorSet Layout Creation
	if (true) {
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		for (unsigned int i = 0; i < inputdesccount; i++) {
			const VK_ShaderInputDesc* desc = inputdescs[i];

			if (!(desc->TYPE == tgfx_shaderinput_type_IMAGE_PI ||
				desc->TYPE == tgfx_shaderinput_type_SAMPLER_PI ||
				desc->TYPE == tgfx_shaderinput_type_SBUFFER_PI ||
				desc->TYPE == tgfx_shaderinput_type_UBUFFER_PI)) {
				continue;
			}
			unsigned int BP = desc->BINDINDEX;
			for (unsigned int bpsearchindex = 0; bpsearchindex < bindings.size(); bpsearchindex++) {
				if (BP == bindings[bpsearchindex].binding) {
					LOG(tgfx_result_FAIL, "Link_MaterialType() has failed because there are colliding binding points!");
					return false;
				}
			}
			VkDescriptorSetLayoutBinding bn = {};
			bn.stageFlags = desc->ShaderStages.flag;
			bn.pImmutableSamplers = VK_NULL_HANDLE;
			bn.descriptorType = Find_VkDescType_byMATDATATYPE(desc->TYPE);
			bn.descriptorCount = 1;		//I don't support array descriptors for now!
			bn.binding = BP;
			bindings.push_back(bn);

			InstanceDescSet->DescCount++;
			switch (desc->TYPE) {
			case tgfx_shaderinput_type_IMAGE_PI:
			{
				InstanceDescSet->DescImagesCount += desc->ELEMENTCOUNT;
			}
			break;
			case tgfx_shaderinput_type_SAMPLER_PI:
			{
				InstanceDescSet->DescSamplersCount += desc->ELEMENTCOUNT;
			}
			break;
			case tgfx_shaderinput_type_UBUFFER_PI:
			{
				InstanceDescSet->DescUBuffersCount += desc->ELEMENTCOUNT;
			}
			break;
			case tgfx_shaderinput_type_SBUFFER_PI:
			{
				InstanceDescSet->DescSBuffersCount += desc->ELEMENTCOUNT;
			}
			break;
			}
		}

		if (InstanceDescSet->DescCount) {
			InstanceDescSet->Descs = new VK_Descriptor[InstanceDescSet->DescCount];
			for (unsigned int i = 0; i < inputdesccount; i++) {
				const VK_ShaderInputDesc* desc = inputdescs[i];
				if (!(desc->TYPE == tgfx_shaderinput_type_IMAGE_PI ||
					desc->TYPE == tgfx_shaderinput_type_SAMPLER_PI ||
					desc->TYPE == tgfx_shaderinput_type_SBUFFER_PI ||
					desc->TYPE == tgfx_shaderinput_type_UBUFFER_PI)) {
					continue;
				}

				if (desc->BINDINDEX >= InstanceDescSet->DescCount) {
					LOG(tgfx_result_FAIL, "One of your Material Data Descriptors (Per Instance) uses a binding point that is exceeding the number of Material Data Descriptors (Per Instance). You have to use a binding point that's lower than size of the Material Data Descriptors (Per Instance)!");
					return false;
				}

				//We don't need to create each descriptor's array elements
				VK_Descriptor& vkdesc = InstanceDescSet->Descs[desc->BINDINDEX];
				vkdesc.ElementCount = desc->ELEMENTCOUNT;
				switch (desc->TYPE) {
				case tgfx_shaderinput_type_IMAGE_PI:
					vkdesc.Type = DescType::IMAGE;
					break;
				case tgfx_shaderinput_type_SAMPLER_PI:
					vkdesc.Type = DescType::SAMPLER;
					break;
				case tgfx_shaderinput_type_UBUFFER_PI:
					vkdesc.Type = DescType::UBUFFER;
					break;
				case tgfx_shaderinput_type_SBUFFER_PI:
					vkdesc.Type = DescType::SBUFFER;
					break;
				}
			}
			InstanceDescSet->ShouldRecreate.store(0);
		}

		if (bindings.size()) {
			VkDescriptorSetLayoutCreateInfo ci = {};
			ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			ci.pNext = nullptr;
			ci.flags = 0;
			ci.bindingCount = bindings.size();
			ci.pBindings = bindings.data();

			if (vkCreateDescriptorSetLayout(RENDERGPU->LOGICALDEVICE(), &ci, nullptr, &InstanceDescSet->Layout) != VK_SUCCESS) {
				LOG(tgfx_result_FAIL, "Link_MaterialType() has failed at Instance DesciptorSetLayout Creation vkCreateDescriptorSetLayout()");
				return false;
			}
		}
	}

	//General DescriptorSet Creation
	if (!VKContentManager->Create_DescSet(GeneralDescSet)) {
		LOG(tgfx_result_FAIL, "Descriptor pool is full, that means you should expand its size!");
		return false;
	}

	//Pipeline Layout Creation
	{
		VkPipelineLayoutCreateInfo pl_ci = {};
		pl_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pl_ci.pNext = nullptr;
		pl_ci.flags = 0;

		VkDescriptorSetLayout descsetlays[4] = { VKContentManager->GlobalBuffers_DescSet.Layout, VKContentManager->GlobalTextures_DescSet.Layout, VK_NULL_HANDLE, VK_NULL_HANDLE };
		pl_ci.setLayoutCount = 2;
		if (GeneralDescSet->Layout != VK_NULL_HANDLE) {
			descsetlays[pl_ci.setLayoutCount] = GeneralDescSet->Layout;
			pl_ci.setLayoutCount++;
		}
		if (InstanceDescSet->Layout != VK_NULL_HANDLE) {
			descsetlays[pl_ci.setLayoutCount] = InstanceDescSet->Layout;
			pl_ci.setLayoutCount++;
		}
		pl_ci.pSetLayouts = descsetlays;
		//Don't support for now!
		pl_ci.pushConstantRangeCount = 0;
		pl_ci.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(RENDERGPU->LOGICALDEVICE(), &pl_ci, nullptr, layout) != VK_SUCCESS) {
			LOG(tgfx_result_FAIL, "Link_MaterialType() failed at vkCreatePipelineLayout()!");
			return false;
		}
	}
	return true;
}

tgfx_result vk_gpudatamanager::Create_VertexAttributeLayout(const tgfx_datatype* Attributes, tgfx_vertexlisttypes listtype, tgfx_vertexattributelayout* Handle) {
	VK_VertexAttribLayout* Layout = new VK_VertexAttribLayout;
	unsigned int AttributesCount = 0;
	while (Attributes[AttributesCount] != tgfx_datatype_UNDEFINED) {
		AttributesCount++;
	}
	Layout->Attribute_Number = AttributesCount;
	Layout->Attributes = new tgfx_datatype[Layout->Attribute_Number];
	for (unsigned int i = 0; i < Layout->Attribute_Number; i++) {
		Layout->Attributes[i] = Attributes[i];
	}
	unsigned int size_pervertex = Calculate_sizeofVertexLayout(Layout->Attributes, Layout->Attribute_Number);
	Layout->size_perVertex = size_pervertex;
	Layout->BindingDesc.binding = 0;
	Layout->BindingDesc.stride = size_pervertex;
	Layout->BindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	Layout->PrimitiveTopology = Find_PrimitiveTopology_byGFXVertexListType(listtype);

	Layout->AttribDescs = new VkVertexInputAttributeDescription[Layout->Attribute_Number];
	Layout->AttribDesc_Count = Layout->Attribute_Number;
	unsigned int stride_ofcurrentattribute = 0;
	for (unsigned int i = 0; i < Layout->Attribute_Number; i++) {
		Layout->AttribDescs[i].binding = 0;
		Layout->AttribDescs[i].location = i;
		Layout->AttribDescs[i].offset = stride_ofcurrentattribute;
		Layout->AttribDescs[i].format = Find_VkFormat_byDataType(Layout->Attributes[i]);
		stride_ofcurrentattribute += tgfx_Get_UNIFORMTYPEs_SIZEinbytes(Layout->Attributes[i]);
	}
	VKContentManager->VERTEXATTRIBLAYOUTs.push_back(tapi_GetThisThreadIndex(JobSys), Layout);
	*Handle = (tgfx_vertexattributelayout)Layout;
	return tgfx_result_SUCCESS;
}
void vk_gpudatamanager::Delete_VertexAttributeLayout(tgfx_vertexattributelayout Layout_ID) {
	LOG(tgfx_result_NOTCODED, "Delete_VertexAttributeLayout() isn't coded yet!");
}


tgfx_result vk_gpudatamanager::Create_StagingBuffer(unsigned int DATASIZE, unsigned int MemoryTypeIndex, tgfx_buffer* Handle) {
	if (!DATASIZE) {
		LOG(tgfx_result_FAIL, "Staging Buffer DATASIZE is zero!");
		return tgfx_result_INVALIDARGUMENT;
	}
	if (RENDERGPU->ALLOCS()[MemoryTypeIndex].TYPE == tgfx_memoryallocationtype::DEVICELOCAL) {
		LOG(tgfx_result_FAIL, "You can't create a staging buffer in DEVICELOCAL memory!");
		return tgfx_result_INVALIDARGUMENT;
	}
	VkBufferCreateInfo psuedo_ci = {};
	psuedo_ci.flags = 0;
	psuedo_ci.pNext = nullptr;
	if (RENDERGPU->QUEUEFAMS().size() > 1) {
		psuedo_ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
	}
	else {
		psuedo_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	psuedo_ci.pQueueFamilyIndices = RENDERGPU->ALLQUEUEFAMILIES();
	psuedo_ci.queueFamilyIndexCount = RENDERGPU->QUEUEFAMS().size();
	psuedo_ci.size = DATASIZE;
	psuedo_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	psuedo_ci.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
		| VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	VkBuffer Bufferobj;
	if (vkCreateBuffer(RENDERGPU->LOGICALDEVICE(), &psuedo_ci, nullptr, &Bufferobj) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "Intended staging buffer's creation failed at vkCreateBuffer()!");
		return tgfx_result_FAIL;
	}


	MemoryBlock* StagingBuffer = new MemoryBlock;
	StagingBuffer->MemAllocIndex = MemoryTypeIndex;
	if (VKContentManager->Suballocate_Buffer(Bufferobj, psuedo_ci.usage, *StagingBuffer) != tgfx_result_SUCCESS) {
		delete StagingBuffer;
		LOG(tgfx_result_FAIL, "Suballocation has failed, so staging buffer creation too!");
		return tgfx_result_FAIL;
	}
	vkDestroyBuffer(RENDERGPU->LOGICALDEVICE(), Bufferobj, nullptr);
	*Handle = (tgfx_buffer)StagingBuffer;
	VKContentManager->STAGINGBUFFERs.push_back(tapi_GetThisThreadIndex(JobSys), StagingBuffer);
	return tgfx_result_SUCCESS;
}
tgfx_result vk_gpudatamanager::Upload_toBuffer(tgfx_buffer Handle, tgfx_buffertype Type, const void* DATA, unsigned int DATA_SIZE, unsigned int OFFSET) {
	VkDeviceSize UploadOFFSET = static_cast<VkDeviceSize>(OFFSET);
	unsigned int MEMALLOCINDEX = UINT32_MAX;
	switch (Type) {
	case tgfx_buffertype::GLOBAL:
		MEMALLOCINDEX = ((VK_GlobalBuffer*)Handle)->Block.MemAllocIndex;
		UploadOFFSET += ((VK_GlobalBuffer*)Handle)->Block.Offset;
		break;
	case tgfx_buffertype::STAGING:
		MEMALLOCINDEX = ((MemoryBlock*)Handle)->MemAllocIndex;
		UploadOFFSET += ((MemoryBlock*)Handle)->Offset;
		break;
	case tgfx_buffertype::VERTEX:
		MEMALLOCINDEX = ((VK_VertexBuffer*)Handle)->Block.MemAllocIndex;
		UploadOFFSET += ((VK_VertexBuffer*)Handle)->Block.Offset;
		break;
	case tgfx_buffertype::INDEX:
		MEMALLOCINDEX = ((VK_IndexBuffer*)Handle)->Block.MemAllocIndex;
		UploadOFFSET += ((VK_IndexBuffer*)Handle)->Block.Offset;
	default:
		LOG(tgfx_result_NOTCODED, "Upload_toBuffer() doesn't support this type of buffer for now!");
		return tgfx_result_NOTCODED;
	}

	void* MappedMemory = RENDERGPU->ALLOCS()[MEMALLOCINDEX].MappedMemory;
	if (!MappedMemory) {
		LOG(tgfx_result_FAIL, "Memory is not mapped, so you are either trying to upload to an GPU Local buffer or MemoryTypeIndex is not allocated memory type's index!");
		return tgfx_result_FAIL;
	}
	memcpy(((char*)MappedMemory) + UploadOFFSET, DATA, DATA_SIZE);
	return tgfx_result_SUCCESS;
}
void vk_gpudatamanager::Delete_StagingBuffer(tgfx_buffer StagingBufferHandle) {
	LOG(tgfx_result_NOTCODED, "Delete_StagingBuffer() is not coded!");
	/*
	MemoryBlock* SB = GFXHandleConverter(MemoryBlock*, StagingBufferHandle);
	std::vector<VK_MemoryBlock>* MemoryBlockList = nullptr;
	std::mutex* MutexPTR = nullptr;
	switch (SB->Type) {
	case tgfx_SUBALLOCATEBUFFERTYPEs::FASTHOSTVISIBLE:
		MutexPTR = &RENDERGPU->FASTHOSTVISIBLE_ALLOC.Locker;
		MemoryBlockList = &RENDERGPU->FASTHOSTVISIBLE_ALLOC.Allocated_Blocks;
		break;
	case tgfx_SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE:
		MutexPTR = &RENDERGPU->HOSTVISIBLE_ALLOC.Locker;
		MemoryBlockList = &RENDERGPU->HOSTVISIBLE_ALLOC.Allocated_Blocks;
		break;
	case tgfx_SUBALLOCATEBUFFERTYPEs::READBACK:
		MutexPTR = &RENDERGPU->READBACK_ALLOC.Locker;
		MemoryBlockList = &RENDERGPU->READBACK_ALLOC.Allocated_Blocks;
		break;
	default:
		LOG(tgfx_result_NOTCODED, "This type of memory block isn't supported for a staging buffer! Delete_StagingBuffer() has failed!");
		return;
	}
	MutexPTR->lock();
	for (unsigned int i = 0; i < MemoryBlockList->size(); i++) {
		if ((*MemoryBlockList)[i].Offset == SB->Offset) {
			(*MemoryBlockList)[i].isEmpty = true;
			MutexPTR->unlock();
			return;
		}
	}
	MutexPTR->unlock();
	LOG(tgfx_result_FAIL, "Delete_StagingBuffer() didn't delete any memory!");*/
}

tgfx_result vk_gpudatamanager::Create_VertexBuffer(tgfx_vertexattributelayout AttributeLayout, unsigned int VertexCount,
	unsigned int MemoryTypeIndex, tgfx_buffer* VertexBufferHandle) {
	if (!VertexCount) {
		LOG(tgfx_result_FAIL, "GFXContentManager->Create_MeshBuffer() has failed because vertex_count is zero!");
		return tgfx_result_INVALIDARGUMENT;
	}

	VK_VertexAttribLayout* Layout = ((VK_VertexAttribLayout*)AttributeLayout);
	if (!Layout) {
		LOG(tgfx_result_FAIL, "GFXContentManager->Create_MeshBuffer() has failed because Attribute Layout ID is invalid!");
		return tgfx_result_INVALIDARGUMENT;
	}

	unsigned int TOTALDATA_SIZE = Layout->size_perVertex * VertexCount;

	VK_VertexBuffer* VKMesh = new VK_VertexBuffer;
	VKMesh->Block.MemAllocIndex = MemoryTypeIndex;
	VKMesh->Layout = Layout;
	VKMesh->VERTEX_COUNT = VertexCount;

	VkBufferUsageFlags BufferUsageFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	VkBuffer Buffer = Create_VkBuffer(TOTALDATA_SIZE, BufferUsageFlag);
	if (VKContentManager->Suballocate_Buffer(Buffer, BufferUsageFlag, VKMesh->Block) != tgfx_result_SUCCESS) {
		delete VKMesh;
		LOG(tgfx_result_FAIL, "There is no memory left in specified memory region!");
		vkDestroyBuffer(RENDERGPU->LOGICALDEVICE(), Buffer, nullptr);
		return tgfx_result_FAIL;
	}
	vkDestroyBuffer(RENDERGPU->LOGICALDEVICE(), Buffer, nullptr);


	VKContentManager->MESHBUFFERs.push_back(tapi_GetThisThreadIndex(JobSys), VKMesh);
	*VertexBufferHandle = (tgfx_buffer)VKMesh;
	return tgfx_result_SUCCESS;
}
//When you call this function, Draw Calls that uses this ID may draw another Mesh or crash
//Also if you have any Point Buffer that uses first vertex attribute of that Mesh Buffer, it may crash or draw any other buffer
void vk_gpudatamanager::Unload_VertexBuffer(tgfx_buffer MeshBuffer_ID) {
	LOG(tgfx_result_NOTCODED, "VK::Unload_MeshBuffer isn't coded!");
}



tgfx_result vk_gpudatamanager::Create_IndexBuffer(tgfx_datatype DataType, unsigned int IndexCount, unsigned int MemoryTypeIndex, tgfx_buffer* IndexBufferHandle) {
	VkIndexType IndexType = Find_IndexType_byGFXDATATYPE(DataType);
	if (IndexType == VK_INDEX_TYPE_MAX_ENUM) {
		LOG(tgfx_result_FAIL, "GFXContentManager->Create_IndexBuffer() has failed because DataType isn't supported!");
		return tgfx_result_INVALIDARGUMENT;
	}
	if (!IndexCount) {
		LOG(tgfx_result_FAIL, "GFXContentManager->Create_IndexBuffer() has failed because IndexCount is zero!");
		return tgfx_result_INVALIDARGUMENT;
	}
	if (MemoryTypeIndex >= RENDERGPU->ALLOCS().size()) {
		LOG(tgfx_result_FAIL, "GFXContentManager->Create_IndexBuffer() has failed because MemoryTypeIndex is invalid!");
		return tgfx_result_INVALIDARGUMENT;
	}
	VK_IndexBuffer* IB = new VK_IndexBuffer;
	IB->DATATYPE = IndexType;

	VkBufferUsageFlags BufferUsageFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	VkBuffer Buffer = Create_VkBuffer(tgfx_Get_UNIFORMTYPEs_SIZEinbytes(DataType) * IndexCount, BufferUsageFlag);
	IB->Block.MemAllocIndex = MemoryTypeIndex;
	IB->IndexCount = static_cast<VkDeviceSize>(IndexCount);
	if (VKContentManager->Suballocate_Buffer(Buffer, BufferUsageFlag, IB->Block) != tgfx_result_SUCCESS) {
		delete IB;
		LOG(tgfx_result_FAIL, "There is no memory left in specified memory region!");
		vkDestroyBuffer(RENDERGPU->LOGICALDEVICE(), Buffer, nullptr);
		return tgfx_result_FAIL;
	}
	vkDestroyBuffer(RENDERGPU->LOGICALDEVICE(), Buffer, nullptr);

	*IndexBufferHandle = (tgfx_buffer)IB;
	VKContentManager->INDEXBUFFERs.push_back(tapi_GetThisThreadIndex(JobSys), IB);
	return tgfx_result_SUCCESS;
}
void vk_gpudatamanager::Unload_IndexBuffer(tgfx_buffer BufferHandle) {
	LOG(tgfx_result_NOTCODED, "Create_IndexBuffer() and nothing IndexBuffer related isn't coded yet!");
}


tgfx_result vk_gpudatamanager::Create_Texture(tgfx_texture_dimensions DIMENSION, unsigned int WIDTH, unsigned int HEIGHT, tgfx_texture_channels CHANNEL_TYPE, 
	unsigned char MIPCOUNT, tgfx_textureusageflag USAGE, tgfx_texture_order DATAORDER, unsigned int MemoryTypeIndex, tgfx_texture* TextureHandle) {
	if (MIPCOUNT > std::floor(std::log2(std::max(WIDTH, HEIGHT))) + 1 || !MIPCOUNT) {
		LOG(tgfx_result_FAIL, "GFXContentManager->Create_Texture() has failed because mip count of the texture is wrong!");
		return tgfx_result_FAIL;
	}
	VK_Texture* TEXTURE = new VK_Texture;
	TEXTURE->CHANNELs = CHANNEL_TYPE;
	TEXTURE->HEIGHT = HEIGHT;
	TEXTURE->WIDTH = WIDTH;
	TEXTURE->DATA_SIZE = WIDTH * HEIGHT * tgfx_GetByteSizeOf_TextureChannels(CHANNEL_TYPE);
	TEXTURE->USAGE = *(VkImageUsageFlags*)USAGE;
	delete (VkImageUsageFlags*)USAGE;
	TEXTURE->Block.MemAllocIndex = MemoryTypeIndex;
	TEXTURE->DIMENSION = DIMENSION;
	TEXTURE->MIPCOUNT = MIPCOUNT;


	//Create VkImage
	{
		VkImageCreateInfo im_ci = {};
		im_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		im_ci.extent.width = TEXTURE->WIDTH;
		im_ci.extent.height = TEXTURE->HEIGHT;
		im_ci.extent.depth = 1;
		if (DIMENSION == tgfx_texture_dimensions::TEXTURE_CUBE) {
			im_ci.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			im_ci.arrayLayers = 6;
		}
		else {
			im_ci.flags = 0;
			im_ci.arrayLayers = 1;
		}
		im_ci.format = Find_VkFormat_byTEXTURECHANNELs(TEXTURE->CHANNELs);
		im_ci.imageType = Find_VkImageType(DIMENSION);
		im_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		im_ci.mipLevels = static_cast<uint32_t>(MIPCOUNT);
		im_ci.pNext = nullptr;
		if (RENDERGPU->QUEUEFAMS().size() > 1) {
			im_ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
		}
		else {
			im_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		im_ci.pQueueFamilyIndices = RENDERGPU->ALLQUEUEFAMILIES();
		im_ci.queueFamilyIndexCount = RENDERGPU->QUEUEFAMS().size();
		im_ci.tiling = Find_VkTiling(DATAORDER);
		im_ci.usage = TEXTURE->USAGE;
		im_ci.samples = VK_SAMPLE_COUNT_1_BIT;

		if (vkCreateImage(RENDERGPU->LOGICALDEVICE(), &im_ci, nullptr, &TEXTURE->Image) != VK_SUCCESS) {
			LOG(tgfx_result_FAIL, "GFXContentManager->Create_Texture() has failed in vkCreateImage()!");
			delete TEXTURE;
			return tgfx_result_FAIL;
		}

		if (VKContentManager->Suballocate_Image(*TEXTURE) != tgfx_result_SUCCESS) {
			LOG(tgfx_result_FAIL, "Suballocation of the texture has failed! Please re-create later.");
			delete TEXTURE;
			return tgfx_result_FAIL;
		}
	}

	//Create Image View
	{
		VkImageViewCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.flags = 0;
		ci.pNext = nullptr;

		ci.image = TEXTURE->Image;
		if (DIMENSION == tgfx_texture_dimensions::TEXTURE_CUBE) {
			ci.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
			ci.subresourceRange.layerCount = 6;
		}
		else {
			ci.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
			ci.subresourceRange.layerCount = 1;
		}
		ci.subresourceRange.baseArrayLayer = 0;
		ci.subresourceRange.baseMipLevel = 0;
		ci.subresourceRange.levelCount = 1;

		ci.format = Find_VkFormat_byTEXTURECHANNELs(TEXTURE->CHANNELs);
		if (TEXTURE->CHANNELs == tgfx_texture_channels_D32) {
			ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if (TEXTURE->CHANNELs == tgfx_texture_channels_D24S8) {
			ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else {
			ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		Fill_ComponentMapping_byCHANNELs(TEXTURE->CHANNELs, ci.components);

		if (vkCreateImageView(RENDERGPU->LOGICALDEVICE(), &ci, nullptr, &TEXTURE->ImageView) != VK_SUCCESS) {
			LOG(tgfx_result_FAIL, "GFXContentManager->Upload_Texture() has failed in vkCreateImageView()!");
			return tgfx_result_FAIL;
		}
	}

	VKContentManager->TEXTUREs.push_back(tapi_GetThisThreadIndex(JobSys), TEXTURE);
	*TextureHandle = (tgfx_texture)TEXTURE;
	return tgfx_result_SUCCESS;
}
tgfx_result vk_gpudatamanager::Upload_Texture(tgfx_texture TextureHandle, const void* DATA, unsigned int DATA_SIZE, unsigned int TARGETOFFSET) {
	LOG(tgfx_result_NOTCODED, "GFXContentManager->Upload_Texture(): Uploading the data isn't coded yet!");
	return tgfx_result_NOTCODED;
}
void vk_gpudatamanager::Delete_Texture(tgfx_texture TextureHandle, unsigned char isUsedLastFrame) {
	if (!TextureHandle) {
		return;
	}
	VK_Texture* Texture = ((VK_Texture*)TextureHandle);

	if (isUsedLastFrame) {
		VKContentManager->NextFrameDeleteTextureCalls.push_back(tapi_GetThisThreadIndex(JobSys), Texture);
	}
	else {
		VKContentManager->DeleteTextureList.push_back(tapi_GetThisThreadIndex(JobSys), Texture);
	}

	std::unique_lock<std::mutex> Locker;
	VKContentManager->TEXTUREs.PauseAllOperations(Locker);
	for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
		for (unsigned int i = 0; i < VKContentManager->TEXTUREs.size(ThreadID); i++) {
			if (Texture == VKContentManager->TEXTUREs.get(ThreadID, i)) {
				VKContentManager->TEXTUREs.erase(ThreadID, i);
			}
		}
	}
}


tgfx_result vk_gpudatamanager::Create_GlobalBuffer(const char* BUFFER_NAME, unsigned int DATA_SIZE, unsigned char isUniform, unsigned int MemoryTypeIndex, 
	tgfx_buffer* GlobalBufferHandle) {
	if (VKRENDERER->RG_Status != RenderGraphStatus::Invalid || VKRENDERER->FrameGraphs->BranchCount) {
		LOG(tgfx_result_FAIL, "GFX API don't support run-time Global Buffer addition for now because Vulkan needs to recreate PipelineLayouts (so all PSOs)! Please create your global buffers before render graph construction.");
		return tgfx_result_WRONGTIMING;
	}

	VkBuffer obj;
	VkBufferUsageFlags flags = 0;
	if (isUniform) {
		flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}
	else {
		flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}
	obj = Create_VkBuffer(DATA_SIZE, flags);


	VK_GlobalBuffer* GB = new VK_GlobalBuffer;
	GB->Block.MemAllocIndex = MemoryTypeIndex;
	if (VKContentManager->Suballocate_Buffer(obj, flags, GB->Block) != tgfx_result_SUCCESS) {
		LOG(tgfx_result_FAIL, "Create_GlobalBuffer has failed at suballocation!");
		return tgfx_result_FAIL;
	}
	vkDestroyBuffer(RENDERGPU->LOGICALDEVICE(), obj, nullptr);

	GB->DATA_SIZE = DATA_SIZE;
	GB->isUniform = isUniform;

	*GlobalBufferHandle = (tgfx_buffer)GB;
	VKContentManager->GLOBALBUFFERs.push_back(tapi_GetThisThreadIndex(JobSys), GB);
	return tgfx_result_SUCCESS;
}
void vk_gpudatamanager::Unload_GlobalBuffer(tgfx_buffer BUFFER_ID) {
	LOG(tgfx_result_NOTCODED, "GFXContentManager->Unload_GlobalBuffer() isn't coded!");
}
tgfx_result vk_gpudatamanager::SetGlobalShaderInput_Buffer(unsigned char isUniformBuffer, unsigned int ElementIndex, unsigned char isUsedLastFrame, tgfx_buffer BufferHandle, 
	unsigned int BufferOffset, unsigned int BoundDataSize) {
	DescType intendedDescType = (isUniformBuffer) ? DescType::UBUFFER : DescType::SBUFFER;
	VK_DescBufferElement* element = nullptr;
	for (unsigned char i = 0; i < 2; i++) {
		if (VKContentManager->GlobalBuffers_DescSet.Descs[i].Type == intendedDescType) {
			if (VKContentManager->GlobalBuffers_DescSet.Descs[i].ElementCount <= ElementIndex) {
				LOG(tgfx_result_FAIL, "SetGlobalBuffer() has failed because ElementIndex isn't smaller than ElementCount of the GlobalBuffer!");
				return tgfx_result_FAIL;
			}
			element = &((VK_DescBufferElement*)VKContentManager->GlobalBuffers_DescSet.Descs[i].Elements)[ElementIndex];
		}
	}

	unsigned char x = 0;
	if (!element->IsUpdated.compare_exchange_strong(x, 1)) {
		if (x != 255) {
			LOG(tgfx_result_FAIL, "You already changed the global buffer this frame, second or concurrent one will fail!");
			return tgfx_result_WRONGTIMING;
		}
		//If value is 255, this means global buffer isn't set before so try to set it again!
		if (!element->IsUpdated.compare_exchange_strong(x, 1)) {
			LOG(tgfx_result_FAIL, "You already changed the global buffer this frame, second or concurrent one will fail!");
			return tgfx_result_WRONGTIMING;
		}
	}
	VkDeviceSize finaloffset = static_cast<VkDeviceSize>(BufferOffset);
	FindBufferOBJ_byBufType(BufferHandle, tgfx_buffertype::GLOBAL, element->Info.buffer, finaloffset);
	element->Info.offset = finaloffset;
	element->Info.range = static_cast<VkDeviceSize>(BoundDataSize);
	if (isUsedLastFrame) {
		VKContentManager->GlobalBuffers_DescSet.ShouldRecreate.store(true);
	}
	return tgfx_result_SUCCESS;
}
tgfx_result vk_gpudatamanager::SetGlobalShaderInput_Texture(unsigned char isSampledTexture, unsigned int ElementIndex, unsigned char isUsedLastFrame, 
	tgfx_texture TextureHandle, tgfx_samplingtype SamplingType, tgfx_imageaccess access) {
	DescType intendedDescType = (isSampledTexture) ? DescType::SAMPLER : DescType::IMAGE;
	VK_DescImageElement* element = nullptr;
	for (unsigned char i = 0; i < 2; i++) {
		if (VKContentManager->GlobalTextures_DescSet.Descs[i].Type == intendedDescType) {
			if (VKContentManager->GlobalTextures_DescSet.Descs[i].ElementCount <= ElementIndex) {
				LOG(tgfx_result_FAIL, "SetGlobalTexture() has failed because ElementIndex isn't smaller than TextureCount of the GlobalTexture!");
				return tgfx_result_FAIL;
			}
			element = &((VK_DescImageElement*)VKContentManager->GlobalTextures_DescSet.Descs[i].Elements)[ElementIndex];
		}
	}

	unsigned char x = 0;
	if (!element->IsUpdated.compare_exchange_strong(x, 1)) {
		if (x != 255) {
			LOG(tgfx_result_FAIL, "You already changed the global texture this frame, second or concurrent one will fail!");
			return tgfx_result_WRONGTIMING;
		}
		//If value is 255, this means global texture isn't set before so try to set it again!
		if (!element->IsUpdated.compare_exchange_strong(x, 1)) {
			LOG(tgfx_result_FAIL, "You already changed the global texture this frame, second or concurrent one will fail!");
			return tgfx_result_WRONGTIMING;
		}
	}
	element->info.imageView = ((VK_Texture*)TextureHandle)->ImageView;
	element->info.sampler = ((VK_Sampler*)SamplingType)->Sampler;
	VkAccessFlags unused;
	Find_AccessPattern_byIMAGEACCESS(access, unused, element->info.imageLayout);
	if (isUsedLastFrame) {
		VKContentManager->GlobalTextures_DescSet.ShouldRecreate.store(true);
	}
	return tgfx_result_SUCCESS;
}

tgfx_result vk_gpudatamanager::Compile_ShaderSource(tgfx_shaderlanguages language, tgfx_shaderstage shaderstage, void* DATA, unsigned int DATA_SIZE,
	tgfx_shadersource* ShaderSourceHandle) {
	//Create Vertex Shader Module
	VkShaderModuleCreateInfo ci = {};
	ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ci.flags = 0;
	ci.pNext = nullptr;
	ci.pCode = reinterpret_cast<const uint32_t*>(DATA);
	ci.codeSize = static_cast<size_t>(DATA_SIZE);

	VkShaderModule Module;
	if (vkCreateShaderModule(RENDERGPU->LOGICALDEVICE(), &ci, 0, &Module) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "Shader Source is failed at creation!");
		return tgfx_result_FAIL;
	}

	VK_ShaderSource* SHADERSOURCE = new VK_ShaderSource;
	SHADERSOURCE->Module = Module;
	VKContentManager->SHADERSOURCEs.push_back(tapi_GetThisThreadIndex(JobSys), SHADERSOURCE);
	LOG(tgfx_result_SUCCESS, "Vertex Shader Module is successfully created!");
	*ShaderSourceHandle = (tgfx_shadersource)SHADERSOURCE;
	return tgfx_result_SUCCESS;
}
void vk_gpudatamanager::Delete_ShaderSource(tgfx_shadersource ASSET_ID) {
	LOG(tgfx_result_NOTCODED, "VK::Unload_GlobalBuffer isn't coded!");
}
tgfx_result vk_gpudatamanager::Create_ComputeType(tgfx_shadersource GFXSHADER, tgfx_shaderinputdescription_list ShaderInputDescs, tgfx_computeshadertype* ComputeTypeHandle) {
	VkComputePipelineCreateInfo cp_ci = {};
	VK_ShaderSource* SHADER = (VK_ShaderSource*)GFXSHADER;

	VkShaderModule shader_module = VK_NULL_HANDLE;
	VkShaderModuleCreateInfo sm_ci = {};
	sm_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	sm_ci.flags = 0;
	sm_ci.pNext = nullptr;
	sm_ci.pCode = reinterpret_cast<const uint32_t*>(SHADER->SOURCE_CODE);
	sm_ci.codeSize = static_cast<size_t>(SHADER->DATA_SIZE);
	if (vkCreateShaderModule(RENDERGPU->LOGICALDEVICE(), &sm_ci, nullptr, &shader_module) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "Compile_ComputeShader() has failed at vkCreateShaderModule!");
		return tgfx_result_FAIL;
	}

	VK_ComputePipeline* VKPipeline = new VK_ComputePipeline;

	if (!VKDescSet_PipelineLayoutCreation((const VK_ShaderInputDesc**)ShaderInputDescs, &VKPipeline->General_DescSet, &VKPipeline->Instance_DescSet, 
		&VKPipeline->PipelineLayout)) {
		LOG(tgfx_result_FAIL, "Compile_ComputeType() has failed at VKDescSet_PipelineLayoutCreation!");
		delete VKPipeline;
		return tgfx_result_FAIL;
	}

	//VkPipeline creation
	{
		cp_ci.stage.flags = 0;
		cp_ci.stage.module = shader_module;
		cp_ci.stage.pName = "main";
		cp_ci.stage.pNext = nullptr;
		cp_ci.stage.pSpecializationInfo = nullptr;
		cp_ci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		cp_ci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		cp_ci.basePipelineHandle = VK_NULL_HANDLE;
		cp_ci.basePipelineIndex = -1;
		cp_ci.flags = 0;
		cp_ci.layout = VKPipeline->PipelineLayout;
		cp_ci.pNext = nullptr;
		cp_ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		if (vkCreateComputePipelines(RENDERGPU->LOGICALDEVICE(), VK_NULL_HANDLE, 1, &cp_ci, nullptr, &VKPipeline->PipelineObject) != VK_SUCCESS) {
			LOG(tgfx_result_FAIL, "Compile_ComputeShader() has failed at vkCreateComputePipelines!");
			delete VKPipeline;
			return tgfx_result_FAIL;
		}
	}
	vkDestroyShaderModule(RENDERGPU->LOGICALDEVICE(), shader_module, nullptr);

	*ComputeTypeHandle = (tgfx_computeshadertype)VKPipeline;
	VKContentManager->COMPUTETYPEs.push_back(tapi_GetThisThreadIndex(JobSys), VKPipeline);
	return tgfx_result_SUCCESS;
}
tgfx_result vk_gpudatamanager::Create_ComputeInstance(tgfx_computeshadertype ComputeType, tgfx_computeshaderinstance* ComputeInstHandle) {
	VK_ComputePipeline* VKPipeline = (VK_ComputePipeline*)ComputeType;

	VK_ComputeInstance* instance = new VK_ComputeInstance;
	instance->PROGRAM = VKPipeline;

	instance->DescSet.Layout = VKPipeline->Instance_DescSet.Layout;
	instance->DescSet.ShouldRecreate.store(0);
	instance->DescSet.DescImagesCount = VKPipeline->Instance_DescSet.DescImagesCount;
	instance->DescSet.DescSamplersCount = VKPipeline->Instance_DescSet.DescSamplersCount;
	instance->DescSet.DescSBuffersCount = VKPipeline->Instance_DescSet.DescSBuffersCount;
	instance->DescSet.DescUBuffersCount = VKPipeline->Instance_DescSet.DescUBuffersCount;
	instance->DescSet.DescCount = VKPipeline->Instance_DescSet.DescCount;

	if (instance->DescSet.DescCount) {
		instance->DescSet.Descs = new VK_Descriptor[instance->DescSet.DescCount];

		for (unsigned int i = 0; i < instance->DescSet.DescCount; i++) {
			VK_Descriptor& desc = instance->DescSet.Descs[i];
			desc.ElementCount = VKPipeline->Instance_DescSet.Descs[i].ElementCount;
			desc.Type = VKPipeline->Instance_DescSet.Descs[i].Type;
			switch (desc.Type)
			{
			case DescType::IMAGE:
			case DescType::SAMPLER:
				desc.Elements = new VK_DescImageElement[desc.ElementCount];
			case DescType::SBUFFER:
			case DescType::UBUFFER:
				desc.Elements = new VK_DescBufferElement[desc.ElementCount];
			}
		}

		if (!VKContentManager->Create_DescSet(&instance->DescSet)) {
			LOG(tgfx_result_FAIL, "You probably exceed one of the limits you specified at GFX initialization process! Create_ComputeInstance() has failed!");
			delete[] instance->DescSet.Descs;
			delete instance;
			return tgfx_result_FAIL;
		}
	}

	*ComputeInstHandle = (tgfx_computeshaderinstance)instance;
	VKContentManager->COMPUTEINSTANCEs.push_back(tapi_GetThisThreadIndex(JobSys), instance);
	return tgfx_result_SUCCESS;
}
void vk_gpudatamanager::Delete_ComputeShaderType(tgfx_computeshadertype ASSET_ID) {
	LOG(tgfx_result_NOTCODED, "VK::Delete_ComputeShader isn't coded!");
}
void vk_gpudatamanager::Delete_ComputeShaderInstance(tgfx_computeshaderinstance ASSET_ID) {
	LOG(tgfx_result_NOTCODED, "VK::Delete_ComputeShader isn't coded!");
}
VkColorComponentFlags Find_ColorWriteMask_byChannels(tgfx_texture_channels channels) {
	switch (channels)
	{
	case tgfx_texture_channels_BGRA8UB:
	case tgfx_texture_channels_BGRA8UNORM:
	case tgfx_texture_channels_RGBA32F:
	case tgfx_texture_channels_RGBA32UI:
	case tgfx_texture_channels_RGBA32I:
	case tgfx_texture_channels_RGBA8UB:
	case tgfx_texture_channels_RGBA8B:
		return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	case tgfx_texture_channels_RGB32F:
	case tgfx_texture_channels_RGB32UI:
	case tgfx_texture_channels_RGB32I:
	case tgfx_texture_channels_RGB8UB:
	case tgfx_texture_channels_RGB8B:
		return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
	case tgfx_texture_channels_RA32F:
	case tgfx_texture_channels_RA32UI:
	case tgfx_texture_channels_RA32I:
	case tgfx_texture_channels_RA8UB:
	case tgfx_texture_channels_RA8B:
		return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT;
	case tgfx_texture_channels_R32F:
	case tgfx_texture_channels_R32UI:
	case tgfx_texture_channels_R32I:
		return VK_COLOR_COMPONENT_R_BIT;
	case tgfx_texture_channels_R8UB:
	case tgfx_texture_channels_R8B:
		return VK_COLOR_COMPONENT_R_BIT;
	case tgfx_texture_channels_D32:
	case tgfx_texture_channels_D24S8:
	default:
		LOG(tgfx_result_NOTCODED, "Find_ColorWriteMask_byChannels() doesn't support this type of RTSlot channel!");
		return VK_COLOR_COMPONENT_FLAG_BITS_MAX_ENUM;
	}
}
tgfx_result vk_gpudatamanager::Link_MaterialType(tgfx_shadersource_list ShaderSources, tgfx_shaderinputdescription_list ShaderInputDescs, 
	tgfx_vertexattributelayout AttributeLayout, tgfx_subdrawpass Subdrawpass, tgfx_cullmode culling, tgfx_polygonmode polygonmode, 
	tgfx_depthsettings depthtest, tgfx_stencilsettings StencilFrontFaced, tgfx_stencilsettings StencilBackFaced, tgfx_blendinginfo_list BLENDINGs, 
	tgfx_rasterpipelinetype* MaterialHandle) {
	if (VKRENDERER->RG_Status == RenderGraphStatus::StartedConstruction) {
		LOG(tgfx_result_FAIL, "You can't link a Material Type while recording RenderGraph!");
		return tgfx_result_WRONGTIMING;
	}
	VK_VertexAttribLayout* LAYOUT = nullptr;
	LAYOUT = (VK_VertexAttribLayout*)AttributeLayout;
	if (!LAYOUT) {
		LOG(tgfx_result_FAIL, "Link_MaterialType() has failed because Material Type has invalid Vertex Attribute Layout!");
		return tgfx_result_INVALIDARGUMENT;
	}
	VK_SubDrawPass* Subpass = (VK_SubDrawPass*)Subdrawpass;
	drawpass_vk* MainPass = (drawpass_vk*)Subpass->DrawPass;

	VK_ShaderSource* VertexSource = nullptr, *FragmentSource = nullptr;
	unsigned int ShaderSourceCount = 0;
	while (ShaderSources[ShaderSourceCount] != NULL) {
		VK_ShaderSource* ShaderSource = ((VK_ShaderSource*)ShaderSources[ShaderSourceCount]);
		switch (ShaderSource->stage) {
		case tgfx_shaderstage_VERTEXSHADER:
			if (VertexSource) {LOG(tgfx_result_FAIL, "Link_MaterialType() has failed because there 2 vertex shaders in the list!");return tgfx_result_FAIL;}
			VertexSource = ShaderSource;	break;
		case tgfx_shaderstage_FRAGMENTSHADER:
			if (FragmentSource) {LOG(tgfx_result_FAIL, "Link_MaterialType() has failed because there 2 fragment shaders in the list!");return tgfx_result_FAIL;}
			FragmentSource = ShaderSource; 	break;
		default:
			LOG(tgfx_result_NOTCODED, "Link_MaterialType() has failed because list has unsupported shader source type!");
			return tgfx_result_NOTCODED;
		}
		ShaderSourceCount++;
	}


	//Subpass attachment should happen here!
	VK_GraphicsPipeline* VKPipeline = new VK_GraphicsPipeline;

	VkPipelineShaderStageCreateInfo Vertex_ShaderStage = {};
	VkPipelineShaderStageCreateInfo Fragment_ShaderStage = {};
	{
		Vertex_ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		Vertex_ShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		VkShaderModule* VS_Module = &VertexSource->Module;
		Vertex_ShaderStage.module = *VS_Module;
		Vertex_ShaderStage.pName = "main";

		Fragment_ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		Fragment_ShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		VkShaderModule* FS_Module = &FragmentSource->Module;
		Fragment_ShaderStage.module = *FS_Module;
		Fragment_ShaderStage.pName = "main";
	}
	VkPipelineShaderStageCreateInfo STAGEs[2] = { Vertex_ShaderStage, Fragment_ShaderStage };


	GPU* Vulkan_GPU = (GPU*)RENDERGPU;

	VkPipelineVertexInputStateCreateInfo VertexInputState_ci = {};
	{
		VertexInputState_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		VertexInputState_ci.pVertexBindingDescriptions = &LAYOUT->BindingDesc;
		VertexInputState_ci.vertexBindingDescriptionCount = 1;
		VertexInputState_ci.pVertexAttributeDescriptions = LAYOUT->AttribDescs;
		VertexInputState_ci.vertexAttributeDescriptionCount = LAYOUT->AttribDesc_Count;
		VertexInputState_ci.flags = 0;
		VertexInputState_ci.pNext = nullptr;
	}
	VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = {};
	{
		InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		InputAssemblyState.topology = LAYOUT->PrimitiveTopology;
		InputAssemblyState.primitiveRestartEnable = false;
		InputAssemblyState.flags = 0;
		InputAssemblyState.pNext = nullptr;
	}
	VkPipelineViewportStateCreateInfo RenderViewportState = {};
	{
		RenderViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		RenderViewportState.scissorCount = 1;
		RenderViewportState.pScissors = nullptr;
		RenderViewportState.viewportCount = 1;
		RenderViewportState.pViewports = nullptr;
		RenderViewportState.pNext = nullptr;
		RenderViewportState.flags = 0;
	}
	VkPipelineRasterizationStateCreateInfo RasterizationState = {};
	{
		RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		RasterizationState.polygonMode = Find_PolygonMode_byGFXPolygonMode(polygonmode);
		RasterizationState.cullMode = Find_CullMode_byGFXCullMode(culling);
		RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		RasterizationState.lineWidth = 1.0f;
		RasterizationState.depthClampEnable = VK_FALSE;
		RasterizationState.rasterizerDiscardEnable = VK_FALSE;

		RasterizationState.depthBiasEnable = VK_FALSE;
		RasterizationState.depthBiasClamp = 0.0f;
		RasterizationState.depthBiasConstantFactor = 0.0f;
		RasterizationState.depthBiasSlopeFactor = 0.0f;
	}
	//Draw pass dependent data but draw passes doesn't support MSAA right now
	VkPipelineMultisampleStateCreateInfo MSAAState = {};
	{
		MSAAState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		MSAAState.sampleShadingEnable = VK_FALSE;
		MSAAState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		MSAAState.minSampleShading = 1.0f;
		MSAAState.pSampleMask = nullptr;
		MSAAState.alphaToCoverageEnable = VK_FALSE;
		MSAAState.alphaToOneEnable = VK_FALSE;
	}


	TGFXLISTCOUNT((&VKCORE->TGFXCORE), BLENDINGs, BlendingsCount);
	VK_BlendingInfo** BLENDINGINFOS = (VK_BlendingInfo**)BLENDINGs;
	std::vector<VkPipelineColorBlendAttachmentState> States(MainPass->SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT);
	VkPipelineColorBlendStateCreateInfo Pipeline_ColorBlendState = {};
	{
		VkPipelineColorBlendAttachmentState NonBlendState = {};
		//Non-blend settings
		NonBlendState.blendEnable = VK_FALSE;
		NonBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		NonBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		NonBlendState.colorBlendOp = VkBlendOp::VK_BLEND_OP_ADD;
		NonBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		NonBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		NonBlendState.alphaBlendOp = VK_BLEND_OP_ADD;

		for (unsigned int RTSlotIndex = 0; RTSlotIndex < States.size(); RTSlotIndex++) {
			bool isFound = false;
			for (unsigned int BlendingInfoIndex = 0; BlendingInfoIndex < BlendingsCount; BlendingInfoIndex++) {
				const VK_BlendingInfo* blendinginfo = BLENDINGINFOS[BlendingInfoIndex];
				if (blendinginfo->COLORSLOT_INDEX == RTSlotIndex) {
					States[RTSlotIndex] = blendinginfo->BlendState;
					States[RTSlotIndex].colorWriteMask = Find_ColorWriteMask_byChannels(MainPass->SLOTSET->PERFRAME_SLOTSETs[0].COLOR_SLOTs[RTSlotIndex].RT->CHANNELs);
					isFound = true;
					break;
				}
			}
			if (!isFound) {
				States[RTSlotIndex] = NonBlendState;
				States[RTSlotIndex].colorWriteMask = Find_ColorWriteMask_byChannels(MainPass->SLOTSET->PERFRAME_SLOTSETs[0].COLOR_SLOTs[RTSlotIndex].RT->CHANNELs);
			}
		}

		Pipeline_ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		Pipeline_ColorBlendState.attachmentCount = States.size();
		Pipeline_ColorBlendState.pAttachments = States.data();
		if (BlendingsCount) {
			Pipeline_ColorBlendState.blendConstants[0] = BLENDINGINFOS[0]->BLENDINGCONSTANTs.x;
			Pipeline_ColorBlendState.blendConstants[1] = BLENDINGINFOS[0]->BLENDINGCONSTANTs.y;
			Pipeline_ColorBlendState.blendConstants[2] = BLENDINGINFOS[0]->BLENDINGCONSTANTs.z;
			Pipeline_ColorBlendState.blendConstants[3] = BLENDINGINFOS[0]->BLENDINGCONSTANTs.w;
		}
		else {
			Pipeline_ColorBlendState.blendConstants[0] = 0.0f;
			Pipeline_ColorBlendState.blendConstants[1] = 0.0f;
			Pipeline_ColorBlendState.blendConstants[2] = 0.0f;
			Pipeline_ColorBlendState.blendConstants[3] = 0.0f;
		}
		//I won't use logical operations
		Pipeline_ColorBlendState.logicOpEnable = VK_FALSE;
		Pipeline_ColorBlendState.logicOp = VK_LOGIC_OP_COPY;
	}

	VkPipelineDynamicStateCreateInfo Dynamic_States = {};
	std::vector<VkDynamicState> DynamicStatesList;
	{
		DynamicStatesList.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		DynamicStatesList.push_back(VK_DYNAMIC_STATE_SCISSOR);

		Dynamic_States.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		Dynamic_States.dynamicStateCount = DynamicStatesList.size();
		Dynamic_States.pDynamicStates = DynamicStatesList.data();
	}

	if (!VKDescSet_PipelineLayoutCreation((const VK_ShaderInputDesc**)ShaderInputDescs, &VKPipeline->General_DescSet, &VKPipeline->Instance_DescSet, &VKPipeline->PipelineLayout)) {
		LOG(tgfx_result_FAIL, "Link_MaterialType() has failed at VKDescSet_PipelineLayoutCreation!");
		delete VKPipeline;
		return tgfx_result_FAIL;
	}

	VkPipelineDepthStencilStateCreateInfo depth_state = {};
	if (Subpass->SLOTSET->BASESLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
		depth_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		if (depthtest) {
			VK_DepthSettingDescription* depthsettings = (VK_DepthSettingDescription*)depthtest;
			depth_state.depthTestEnable = VK_TRUE;
			depth_state.depthCompareOp = depthsettings->DepthCompareOP;
			depth_state.depthWriteEnable = depthsettings->ShouldWrite;
			depth_state.depthBoundsTestEnable = VK_FALSE;
			depth_state.maxDepthBounds = depthsettings->DepthBoundsMax;
			depth_state.minDepthBounds = depthsettings->DepthBoundsMin;
		}
		else {
			depth_state.depthTestEnable = VK_FALSE;
			depth_state.depthBoundsTestEnable = VK_FALSE;
		}
		depth_state.flags = 0;
		depth_state.pNext = nullptr;

		if (StencilFrontFaced || StencilBackFaced) { 
			depth_state.stencilTestEnable = VK_TRUE;
			VK_StencilDescription* frontfacestencil = (VK_StencilDescription*)StencilFrontFaced;
			VK_StencilDescription* backfacestencil = (VK_StencilDescription*)StencilBackFaced;
			if (backfacestencil) { depth_state.back = backfacestencil->OPSTATE; }
			else { depth_state.back = {}; }
			if (frontfacestencil) { depth_state.front = frontfacestencil->OPSTATE; }
			else { depth_state.front = {}; }
		}
		else { depth_state.stencilTestEnable = VK_FALSE;	depth_state.back = {};	depth_state.front = {}; }
	}

	VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = {};
	{
		GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.pColorBlendState = &Pipeline_ColorBlendState;
		if (Subpass->SLOTSET->BASESLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
			GraphicsPipelineCreateInfo.pDepthStencilState = &depth_state;
		}
		else {
			GraphicsPipelineCreateInfo.pDepthStencilState = nullptr;
		}
		GraphicsPipelineCreateInfo.pDynamicState = &Dynamic_States;
		GraphicsPipelineCreateInfo.pInputAssemblyState = &InputAssemblyState;
		GraphicsPipelineCreateInfo.pMultisampleState = &MSAAState;
		GraphicsPipelineCreateInfo.pRasterizationState = &RasterizationState;
		GraphicsPipelineCreateInfo.pVertexInputState = &VertexInputState_ci;
		GraphicsPipelineCreateInfo.pViewportState = &RenderViewportState;
		GraphicsPipelineCreateInfo.layout = VKPipeline->PipelineLayout;
		GraphicsPipelineCreateInfo.renderPass = MainPass->RenderPassObject;
		GraphicsPipelineCreateInfo.subpass = Subpass->Binding_Index;
		GraphicsPipelineCreateInfo.stageCount = 2;
		GraphicsPipelineCreateInfo.pStages = STAGEs;
		GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;		//Optional
		GraphicsPipelineCreateInfo.basePipelineIndex = -1;					//Optional
		GraphicsPipelineCreateInfo.flags = 0;
		GraphicsPipelineCreateInfo.pNext = nullptr;
		if (vkCreateGraphicsPipelines(Vulkan_GPU->LOGICALDEVICE(), nullptr, 1, &GraphicsPipelineCreateInfo, nullptr, &VKPipeline->PipelineObject) != VK_SUCCESS) {
			delete VKPipeline;
			delete STAGEs;
			LOG(tgfx_result_FAIL, "vkCreateGraphicsPipelines has failed!");
			return tgfx_result_FAIL;
		}
	}

	VKPipeline->GFX_Subpass = Subpass;
	VKContentManager->SHADERPROGRAMs.push_back(tapi_GetThisThreadIndex(JobSys), VKPipeline);


	LOG(tgfx_result_SUCCESS, "Finished creating Graphics Pipeline");
	*MaterialHandle = (tgfx_rasterpipelinetype)VKPipeline;
	return tgfx_result_SUCCESS;
}
void vk_gpudatamanager::Delete_MaterialType(tgfx_rasterpipelinetype Asset_ID) {
	LOG(tgfx_result_NOTCODED, "VK::Unload_GlobalBuffer isn't coded!");
}
tgfx_result vk_gpudatamanager::Create_MaterialInst(tgfx_rasterpipelinetype MaterialType, tgfx_rasterpipelineinstance* MaterialInstHandle) {
	VK_GraphicsPipeline* VKPSO = (VK_GraphicsPipeline*)MaterialType;
	if (!VKPSO) {
		LOG(tgfx_result_FAIL, "Create_MaterialInst() has failed because Material Type isn't found!");
		return tgfx_result_INVALIDARGUMENT;
	}

	//Descriptor Set Creation
	VK_PipelineInstance* VKPInstance = new VK_PipelineInstance;

	VKPInstance->DescSet.Layout = VKPSO->Instance_DescSet.Layout;
	VKPInstance->DescSet.ShouldRecreate.store(0);
	VKPInstance->DescSet.DescImagesCount = VKPSO->Instance_DescSet.DescImagesCount;
	VKPInstance->DescSet.DescSamplersCount = VKPSO->Instance_DescSet.DescSamplersCount;
	VKPInstance->DescSet.DescSBuffersCount = VKPSO->Instance_DescSet.DescSBuffersCount;
	VKPInstance->DescSet.DescUBuffersCount = VKPSO->Instance_DescSet.DescUBuffersCount;
	VKPInstance->DescSet.DescCount = VKPSO->Instance_DescSet.DescCount;

	if (VKPInstance->DescSet.DescCount) {
		VKPInstance->DescSet.Descs = new VK_Descriptor[VKPInstance->DescSet.DescCount];

		for (unsigned int i = 0; i < VKPInstance->DescSet.DescCount; i++) {
			VK_Descriptor& desc = VKPInstance->DescSet.Descs[i];
			desc.ElementCount = VKPSO->Instance_DescSet.Descs[i].ElementCount;
			desc.Type = VKPSO->Instance_DescSet.Descs[i].Type;
			switch (desc.Type)
			{
			case DescType::IMAGE:
			case DescType::SAMPLER:
				desc.Elements = new VK_DescImageElement[desc.ElementCount];
			case DescType::SBUFFER:
			case DescType::UBUFFER:
				desc.Elements = new VK_DescBufferElement[desc.ElementCount];
			}
		}

		VKContentManager->Create_DescSet(&VKPInstance->DescSet);
	}


	VKPInstance->PROGRAM = VKPSO;
	VKContentManager->SHADERPINSTANCEs.push_back(tapi_GetThisThreadIndex(JobSys), VKPInstance);
	*MaterialInstHandle = (tgfx_rasterpipelineinstance)VKPInstance;
	return tgfx_result_SUCCESS;
}
void vk_gpudatamanager::Delete_MaterialInst(tgfx_rasterpipelineinstance Asset_ID) {
	LOG(tgfx_result_FAIL, "Delete_MaterialInst() isn't coded yet!");
}

tgfx_result SetDescSet_Buffer(VK_DescSet* Set, unsigned int BINDINDEX, bool isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset,
	tgfx_buffertype BUFTYPE, tgfx_buffer BufferHandle, unsigned int BoundDataSize, bool isUsedRecently) {

	if (!Set->DescCount) {
		LOG(tgfx_result_FAIL, "Given Material Type/Instance doesn't have any shader input to set!");
		return tgfx_result_FAIL;
	}
	if (BINDINDEX >= Set->DescCount) {
		LOG(tgfx_result_FAIL, "BINDINDEX is exceeding the shader input count in the Material Type/Instance");
		return tgfx_result_FAIL;
	}
	VK_Descriptor& Descriptor = Set->Descs[BINDINDEX];
	if ((isUniformBufferShaderInput && Descriptor.Type != DescType::UBUFFER) ||
		(!isUniformBufferShaderInput && Descriptor.Type != DescType::SBUFFER)) {
		LOG(tgfx_result_FAIL, "BINDINDEX is pointing to some other type of shader input!");
		return tgfx_result_FAIL;
	}
	if (ELEMENTINDEX >= Descriptor.ElementCount) {
		LOG(tgfx_result_FAIL, "You gave SetMaterialBuffer() an overflowing ELEMENTINDEX!");
		return tgfx_result_FAIL;
	}

	VK_DescBufferElement& DescElement = ((VK_DescBufferElement*)Descriptor.Elements)[ELEMENTINDEX];

	//Check alignment offset
	
	VkDeviceSize reqalignmentoffset = (isUniformBufferShaderInput) ? RENDERGPU->DEVICEPROPERTIES().limits.minUniformBufferOffsetAlignment : RENDERGPU->DEVICEPROPERTIES().limits.minStorageBufferOffsetAlignment;
	if (TargetOffset % reqalignmentoffset) {
		LOG(tgfx_result_WARNING, ("This TargetOffset in SetMaterialBuffer triggers Vulkan Validation Layer, this usage may cause undefined behaviour on this GPU! You should set TargetOffset as a multiple of " + std::to_string(reqalignmentoffset)).c_str());
	}

	unsigned char x = 0;
	if (!DescElement.IsUpdated.compare_exchange_strong(x, 1)) {
		if (x != 255) {
			LOG(tgfx_result_FAIL, "You already set shader input buffer, you can't change it at the same frame!");
			return tgfx_result_WRONGTIMING;
		}
		//If value is 255, this means this shader input isn't set before. So try again!
		if (!DescElement.IsUpdated.compare_exchange_strong(x, 1)) {
			LOG(tgfx_result_FAIL, "You already set shader input buffer, you can't change it at the same frame!");
			return tgfx_result_WRONGTIMING;
		}
	}
	VkDeviceSize FinalOffset = static_cast<VkDeviceSize>(TargetOffset);
	switch (BUFTYPE) {
	case tgfx_buffertype::STAGING:
	case tgfx_buffertype::GLOBAL:
		FindBufferOBJ_byBufType(BufferHandle, BUFTYPE, DescElement.Info.buffer, FinalOffset);
		break;
	default:
		LOG(tgfx_result_NOTCODED, "SetMaterial_UniformBuffer() doesn't support this type of target buffers!");
		return tgfx_result_NOTCODED;
	}
	DescElement.Info.offset = FinalOffset;
	DescElement.Info.range = static_cast<VkDeviceSize>(BoundDataSize);
	DescElement.IsUpdated.store(1);
	VK_DescSetUpdateCall call;
	call.Set = Set;
	call.BindingIndex = BINDINDEX;
	call.ElementIndex = ELEMENTINDEX;
	if (isUsedRecently) {
		call.Set->ShouldRecreate.store(1);
		VKContentManager->DescSets_toCreateUpdate.push_back(tapi_GetThisThreadIndex(JobSys), call);
	}
	else {
		VKContentManager->DescSets_toJustUpdate.push_back(tapi_GetThisThreadIndex(JobSys), call);
	}
	return tgfx_result_SUCCESS;
}
tgfx_result SetDescSet_Texture(VK_DescSet* Set, unsigned int BINDINDEX, bool isSampledTexture, unsigned int ELEMENTINDEX, VK_Sampler* Sampler, VK_Texture* TextureHandle, 
	tgfx_imageaccess usage, bool isUsedRecently) {
	if (!Set->DescCount) {
		LOG(tgfx_result_FAIL, "Given Material Type/Instance doesn't have any shader input to set!");
		return tgfx_result_FAIL;
	}
	if (BINDINDEX >= Set->DescCount) {
		LOG(tgfx_result_FAIL, "BINDINDEX is exceeding the shader input count in the Material Type/Instance");
		return tgfx_result_FAIL;
	}
	VK_Descriptor& Descriptor = Set->Descs[BINDINDEX];
	if ((isSampledTexture && Descriptor.Type != DescType::SAMPLER) ||
		(!isSampledTexture && Descriptor.Type != DescType::IMAGE)) {
		LOG(tgfx_result_FAIL, "BINDINDEX is pointing to some other type of shader input!");
		return tgfx_result_FAIL;
	}
	if (ELEMENTINDEX >= Descriptor.ElementCount) {
		LOG(tgfx_result_FAIL, "You gave SetMaterialTexture() an overflowing ELEMENTINDEX!");
		return tgfx_result_FAIL;
	}
	VK_DescImageElement& DescElement = ((VK_DescImageElement*)Descriptor.Elements)[ELEMENTINDEX];


	unsigned char x = 0;
	if (!DescElement.IsUpdated.compare_exchange_strong(x, 1)) {
		if (x != 255) {
			LOG(tgfx_result_FAIL, "You already set shader input texture, you can't change it at the same frame!");
			return tgfx_result_WRONGTIMING;
		}
		//If value is 255, this means this shader input isn't set before. So try again!
		if (!DescElement.IsUpdated.compare_exchange_strong(x, 1)) {
			LOG(tgfx_result_FAIL, "You already set shader input texture, you can't change it at the same frame!");
			return tgfx_result_WRONGTIMING;
		}
	}
	VkAccessFlags unused;
	Find_AccessPattern_byIMAGEACCESS(usage, unused, DescElement.info.imageLayout);
	VK_Texture* TEXTURE = ((VK_Texture*)TextureHandle);
	DescElement.info.imageView = TEXTURE->ImageView;
	DescElement.info.sampler = Sampler->Sampler;

	VK_DescSetUpdateCall call;
	call.Set = Set;
	call.BindingIndex = BINDINDEX;
	call.ElementIndex = ELEMENTINDEX;
	if (isUsedRecently) {
		call.Set->ShouldRecreate.store(1);
		VKContentManager->DescSets_toCreateUpdate.push_back(tapi_GetThisThreadIndex(JobSys), call);
	}
	else {
		VKContentManager->DescSets_toJustUpdate.push_back(tapi_GetThisThreadIndex(JobSys), call);
	}
	return tgfx_result_SUCCESS;
}

tgfx_result vk_gpudatamanager::SetMaterialType_Buffer(tgfx_rasterpipelinetype MaterialType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	tgfx_buffer BufferHandle, tgfx_buffertype BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize) {
	VK_GraphicsPipeline* PSO = (VK_GraphicsPipeline*)MaterialType;
	VK_DescSet* Set = &PSO->General_DescSet;

	return SetDescSet_Buffer(Set, BINDINDEX, isUniformBufferShaderInput, ELEMENTINDEX, TargetOffset, BUFTYPE, BufferHandle, BoundDataSize, isUsedRecently);
}

tgfx_result vk_gpudatamanager::SetMaterialType_Texture(tgfx_rasterpipelinetype MaterialType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	tgfx_texture TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, tgfx_samplingtype Sampling, tgfx_imageaccess usage) {
	VK_GraphicsPipeline* PSO = ((VK_GraphicsPipeline*)MaterialType);
	VK_DescSet* Set = &PSO->General_DescSet;

	return SetDescSet_Texture(Set, BINDINDEX, isSampledTexture, ELEMENTINDEX, (VK_Sampler*)Sampling, (VK_Texture*)TextureHandle, usage, isUsedRecently);
}



tgfx_result vk_gpudatamanager::SetMaterialInst_Buffer(tgfx_rasterpipelineinstance MaterialInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	tgfx_buffer BufferHandle, tgfx_buffertype BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize) {
	VK_PipelineInstance* PSO = (VK_PipelineInstance*)MaterialInstance;
	VK_DescSet* Set = &PSO->DescSet;

	return SetDescSet_Buffer(Set, BINDINDEX, isUniformBufferShaderInput, ELEMENTINDEX, TargetOffset, BUFTYPE, BufferHandle, BoundDataSize, isUsedRecently);
}
tgfx_result vk_gpudatamanager::SetMaterialInst_Texture(tgfx_rasterpipelineinstance MaterialInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	tgfx_texture TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, tgfx_samplingtype Sampler, tgfx_imageaccess usage) {
	VK_PipelineInstance* PSO = (VK_PipelineInstance*)MaterialInstance;
	VK_DescSet* Set = &PSO->DescSet;

	return SetDescSet_Texture(Set, BINDINDEX, isSampledTexture, ELEMENTINDEX, (VK_Sampler*)Sampler, (VK_Texture*)TextureHandle, usage, isUsedRecently);
}
//IsUsedRecently means is the material type/instance used in last frame. This is necessary for Vulkan synchronization process.
tgfx_result vk_gpudatamanager::SetComputeType_Buffer(tgfx_computeshadertype ComputeType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	tgfx_buffer TargetBufferHandle, tgfx_buffertype BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize) {
	VK_ComputePipeline* PSO = (VK_ComputePipeline*)ComputeType;
	VK_DescSet* Set = &PSO->General_DescSet;

	return SetDescSet_Buffer(Set, BINDINDEX, isUniformBufferShaderInput, ELEMENTINDEX, TargetOffset, BUFTYPE, TargetBufferHandle, BoundDataSize, isUsedRecently);
}
tgfx_result vk_gpudatamanager::SetComputeType_Texture(tgfx_computeshadertype ComputeType, unsigned char isComputeType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	tgfx_texture TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, tgfx_samplingtype Sampler, tgfx_imageaccess usage) {
	VK_ComputePipeline* PSO = (VK_ComputePipeline*)ComputeType;
	VK_DescSet* Set = &PSO->General_DescSet;

	return SetDescSet_Texture(Set, BINDINDEX, isSampledTexture, ELEMENTINDEX, (VK_Sampler*)Sampler, (VK_Texture*)TextureHandle, usage, isUsedRecently);
}

tgfx_result vk_gpudatamanager::SetComputeInst_Buffer(tgfx_computeshaderinstance ComputeInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	tgfx_buffer TargetBufferHandle, tgfx_buffertype BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize) {
	VK_ComputeInstance* PSO = (VK_ComputeInstance*)ComputeInstance;
	VK_DescSet* Set = &PSO->DescSet;

	return SetDescSet_Buffer(Set, BINDINDEX, isUniformBufferShaderInput, ELEMENTINDEX, TargetOffset, BUFTYPE, TargetBufferHandle, BoundDataSize, isUsedRecently);
}
tgfx_result vk_gpudatamanager::SetComputeInst_Texture(tgfx_computeshaderinstance ComputeInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	tgfx_texture TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, tgfx_samplingtype Sampler, tgfx_imageaccess usage) {
	VK_ComputeInstance* PSO = (VK_ComputeInstance*)ComputeInstance;
	VK_DescSet* Set = &PSO->DescSet;

	return SetDescSet_Texture(Set, BINDINDEX, isSampledTexture, ELEMENTINDEX, (VK_Sampler*)Sampler, (VK_Texture*)TextureHandle, usage, isUsedRecently);
}


tgfx_result vk_gpudatamanager::Create_RTSlotset(tgfx_rtslotdescription_list Descriptions, tgfx_rtslotset* RTSlotSetHandle) {
	TGFXLISTCOUNT((&VKCORE->TGFXCORE), Descriptions, DescriptionsCount);
	for (unsigned int SlotIndex = 0; SlotIndex < DescriptionsCount; SlotIndex++) {
		const VK_RTSlotDesc* desc = (VK_RTSlotDesc*)Descriptions[SlotIndex];
		VK_Texture* FirstHandle = (VK_Texture*)desc->TextureHandles[0];
		VK_Texture* SecondHandle = (VK_Texture*)desc->TextureHandles[1];
		if ((FirstHandle->CHANNELs != SecondHandle->CHANNELs) ||
			(FirstHandle->WIDTH != SecondHandle->WIDTH) ||
			(FirstHandle->HEIGHT != SecondHandle->HEIGHT)
			) {
			LOG(tgfx_result_FAIL, "GFXContentManager->Create_RTSlotSet() has failed because one of the slots has texture handles that doesn't match channel type, width or height!");
			return tgfx_result_INVALIDARGUMENT;
		}
		if (!(FirstHandle->USAGE & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) || !(SecondHandle->USAGE & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))) {
			LOG(tgfx_result_FAIL, "GFXContentManager->Create_RTSlotSet() has failed because one of the slots has a handle that doesn't use is_RenderableTo in its USAGEFLAG!");
			return tgfx_result_INVALIDARGUMENT;
		}
	}
	unsigned int DEPTHSLOT_VECTORINDEX = UINT32_MAX;
	//Validate the list and find Depth Slot if there is any
	for (unsigned int SlotIndex = 0; SlotIndex < DescriptionsCount; SlotIndex++) {
		const VK_RTSlotDesc* desc = (VK_RTSlotDesc*)Descriptions[SlotIndex];
		for (unsigned int RTIndex = 0; RTIndex < 2; RTIndex++) {
			VK_Texture* RT = ((VK_Texture*)desc->TextureHandles[RTIndex]);
			if (!RT) {
				LOG(tgfx_result_FAIL, "Create_RTSlotSet() has failed because intended RT isn't found!");
				return tgfx_result_INVALIDARGUMENT;
			}
			if (desc->OPTYPE == tgfx_operationtype_UNUSED) {
				LOG(tgfx_result_FAIL, "Create_RTSlotSet() has failed because you can't create a Base RT SlotSet that has unused attachment!");
				return tgfx_result_INVALIDARGUMENT;
			}
			if (RT->CHANNELs == tgfx_texture_channels_D24S8 || RT->CHANNELs == tgfx_texture_channels_D32) {
				if (DEPTHSLOT_VECTORINDEX != UINT32_MAX && DEPTHSLOT_VECTORINDEX != SlotIndex) {
					LOG(tgfx_result_FAIL, "Create_RTSlotSet() has failed because you can't use two depth buffers at the same slot set!");
					return tgfx_result_INVALIDARGUMENT;
				}
				DEPTHSLOT_VECTORINDEX = SlotIndex;
				continue;
			}
			if (desc->SLOTINDEX > DescriptionsCount - 1) {
				LOG(tgfx_result_FAIL, "Create_RTSlotSet() has failed because you gave a overflowing SLOTINDEX to a RTSLOT!");
				return tgfx_result_INVALIDARGUMENT;
			}
		}
	}
	unsigned char COLORRT_COUNT = (DEPTHSLOT_VECTORINDEX != UINT32_MAX) ? DescriptionsCount - 1 : DescriptionsCount;

	unsigned int i = 0;
	unsigned int j = 0;
	for (i = 0; i < COLORRT_COUNT; i++) {
		if (i == DEPTHSLOT_VECTORINDEX) {
			continue;
		}
		for (j = 1; i + j < COLORRT_COUNT; j++) {
			if (i + j == DEPTHSLOT_VECTORINDEX) {
				continue;
			}
			if (((VK_RTSlotDesc*)Descriptions[i])->SLOTINDEX == ((VK_RTSlotDesc*)Descriptions[i + j])->SLOTINDEX) {
				LOG(tgfx_result_FAIL, "Create_RTSlotSet() has failed because some SLOTINDEXes are same, but each SLOTINDEX should be unique and lower then COLOR SLOT COUNT!");
				return tgfx_result_INVALIDARGUMENT;
			}
		}
	}

	unsigned int FBWIDTH = ((VK_RTSlotDesc*)Descriptions[0])->TextureHandles[0]->WIDTH;
	unsigned int FBHEIGHT = ((VK_RTSlotDesc*)Descriptions[0])->TextureHandles[0]->HEIGHT;
	for (unsigned int SlotIndex = 0; SlotIndex < DescriptionsCount; SlotIndex++) {
		VK_Texture* Texture = ((VK_RTSlotDesc*)Descriptions[SlotIndex])->TextureHandles[0];
		if (Texture->WIDTH != FBWIDTH || Texture->HEIGHT != FBHEIGHT) {
			LOG(tgfx_result_FAIL, "Create_RTSlotSet() has failed because one of your slot's texture has wrong resolution!");
			return tgfx_result_INVALIDARGUMENT;
		}
	}


	VK_RTSLOTSET* VKSLOTSET = new VK_RTSLOTSET;
	for (unsigned int SlotSetIndex = 0; SlotSetIndex < 2; SlotSetIndex++) {
		VK_RTSLOTs& PF_SLOTSET = VKSLOTSET->PERFRAME_SLOTSETs[SlotSetIndex];

		PF_SLOTSET.COLOR_SLOTs = new VK_COLORRTSLOT[COLORRT_COUNT];
		PF_SLOTSET.COLORSLOTs_COUNT = COLORRT_COUNT;
		if (DEPTHSLOT_VECTORINDEX != DescriptionsCount) {
			PF_SLOTSET.DEPTHSTENCIL_SLOT = new VK_DEPTHSTENCILSLOT;
			VK_DEPTHSTENCILSLOT* slot = PF_SLOTSET.DEPTHSTENCIL_SLOT;
			const VK_RTSlotDesc* DEPTHDESC = (VK_RTSlotDesc*)Descriptions[DEPTHSLOT_VECTORINDEX];
			slot->CLEAR_COLOR = glm::vec2(DEPTHDESC->CLEAR_VALUE.x, DEPTHDESC->CLEAR_VALUE.y);
			slot->DEPTH_OPTYPE = DEPTHDESC->OPTYPE;
			slot->RT = (DEPTHDESC->TextureHandles[SlotSetIndex]);
			slot->STENCIL_OPTYPE = DEPTHDESC->OPTYPESTENCIL;
			slot->IS_USED_LATER = DEPTHDESC->isUSEDLATER;
			slot->DEPTH_LOAD = DEPTHDESC->LOADTYPE;
			slot->STENCIL_LOAD = DEPTHDESC->LOADTYPESTENCIL;
		}
		for (unsigned int i = 0; i < COLORRT_COUNT; i++) {
			const VK_RTSlotDesc* desc = (VK_RTSlotDesc*)Descriptions[i];
			VK_Texture* RT = desc->TextureHandles[SlotSetIndex];
			VK_COLORRTSLOT& SLOT = PF_SLOTSET.COLOR_SLOTs[desc->SLOTINDEX];
			SLOT.RT_OPERATIONTYPE = desc->OPTYPE;
			SLOT.LOADSTATE = desc->LOADTYPE;
			SLOT.RT = RT;
			SLOT.IS_USED_LATER = desc->isUSEDLATER;
			SLOT.CLEAR_COLOR = desc->CLEAR_VALUE;
		}


		for (unsigned int i = 0; i < PF_SLOTSET.COLORSLOTs_COUNT; i++) {
			VK_Texture* VKTexture = PF_SLOTSET.COLOR_SLOTs[i].RT;
			if (!VKTexture->ImageView) {
				LOG(tgfx_result_FAIL, "One of your RTs doesn't have a VkImageView! You can't use such a texture as RT. Generally this case happens when you forgot to specify your swapchain texture's usage (while creating a window).");
				return tgfx_result_FAIL;
			}
			VKSLOTSET->ImageViews[SlotSetIndex].push_back(VKTexture->ImageView);
		}
		if (PF_SLOTSET.DEPTHSTENCIL_SLOT) {
			VKSLOTSET->ImageViews[SlotSetIndex].push_back(PF_SLOTSET.DEPTHSTENCIL_SLOT->RT->ImageView);
		}

		VkFramebufferCreateInfo& fb_ci = VKSLOTSET->FB_ci[SlotSetIndex];
		fb_ci.attachmentCount = VKSLOTSET->ImageViews[SlotSetIndex].size();
		fb_ci.pAttachments = VKSLOTSET->ImageViews[SlotSetIndex].data();
		fb_ci.flags = 0;
		fb_ci.height = FBHEIGHT;
		fb_ci.width = FBWIDTH;
		fb_ci.layers = 1;
		fb_ci.pNext = nullptr;
		fb_ci.renderPass = VK_NULL_HANDLE;
		fb_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	}

	VKContentManager->RT_SLOTSETs.push_back(tapi_GetThisThreadIndex(JobSys), VKSLOTSET);
	*RTSlotSetHandle = (tgfx_rtslotset)VKSLOTSET;
	return tgfx_result_SUCCESS;
}
tgfx_result vk_gpudatamanager::Change_RTSlotTexture(tgfx_rtslotset RTSlotHandle, unsigned char isColorRT, unsigned char SlotIndex, unsigned char FrameIndex, tgfx_texture TextureHandle) {
	VK_RTSLOTSET* SlotSet = (VK_RTSLOTSET*)RTSlotHandle;
	VK_Texture* Texture = (VK_Texture*)TextureHandle;
	if (!(Texture->USAGE & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))) {
		LOG(tgfx_result_FAIL, "You can't change RTSlot's texture because given texture's USAGE flag doesn't activate isRenderableTo!");
		return tgfx_result_FAIL;
	}

	VkFramebufferCreateInfo& FB_ci = SlotSet->FB_ci[FrameIndex];
	if (Texture->WIDTH != FB_ci.width || Texture->HEIGHT != FB_ci.height) {
		LOG(tgfx_result_FAIL, "You can't change RTSlot's texture because given texture has unmatching resolution!");
		return tgfx_result_FAIL;
	}

	if (isColorRT) {
		if (SlotSet->PERFRAME_SLOTSETs[FrameIndex].COLORSLOTs_COUNT <= SlotIndex) {
			LOG(tgfx_result_FAIL, "You can't change RTSlot's texture because given SlotIndex is exceeding the number of color slots!");
			return tgfx_result_FAIL;
		}
		VK_COLORRTSLOT& Slot = SlotSet->PERFRAME_SLOTSETs[FrameIndex].COLOR_SLOTs[SlotIndex];
		if (Slot.RT->CHANNELs != Texture->CHANNELs) {
			LOG(tgfx_result_FAIL, "You can't change RTSlot's texture because given Texture's channel type isn't same with target slot's!");
			return tgfx_result_FAIL;
		}

		//Race against concurrency (*plays Tokyo Drift)
		bool x = false, y = true;
		if (!Slot.IsChanged.compare_exchange_strong(x, y)) {
			LOG(tgfx_result_FAIL, "You already changed this slot!");
			return tgfx_result_WRONGTIMING;
		}
		SlotSet->PERFRAME_SLOTSETs[FrameIndex].IsChanged.store(true);

		Slot.RT = Texture;
		SlotSet->ImageViews[FrameIndex][SlotIndex] = Texture->ImageView;
	}
	else {
		VK_DEPTHSTENCILSLOT* Slot = SlotSet->PERFRAME_SLOTSETs[FrameIndex].DEPTHSTENCIL_SLOT;
		if (!Slot) {
			LOG(tgfx_result_FAIL, "You can't change RTSlot's texture because SlotSet doesn't have any Depth-Stencil Slot!");
			return tgfx_result_FAIL;
		}

		if (Slot->RT->CHANNELs != Texture->CHANNELs) {
			LOG(tgfx_result_FAIL, "You can't change RTSlot's texture because given Texture's channel type isn't same with target slot's!");
			return tgfx_result_FAIL;
		}

		//Race against concurrency
		bool x = false, y = true;
		if (!Slot->IsChanged.compare_exchange_strong(x, y)) {
			LOG(tgfx_result_FAIL, "You already changed this slot!");
			return tgfx_result_WRONGTIMING;
		}
		SlotSet->PERFRAME_SLOTSETs[FrameIndex].IsChanged.store(true);

		Slot->RT = Texture;
		SlotSet->ImageViews[FrameIndex][SlotSet->PERFRAME_SLOTSETs[FrameIndex].COLORSLOTs_COUNT] = Texture->ImageView;
	}
	return tgfx_result_SUCCESS;
}
tgfx_result vk_gpudatamanager::Inherite_RTSlotSet(tgfx_rtslotusage_list DescriptionsGFX, tgfx_rtslotset RTSlotSetHandle, tgfx_inheritedrtslotset* InheritedSlotSetHandle) {
	if (!RTSlotSetHandle) {
		LOG(tgfx_result_FAIL, "Inherite_RTSlotSet() has failed because Handle is invalid!");
		return tgfx_result_INVALIDARGUMENT;
	}
	VK_RTSLOTSET* BaseSet = (VK_RTSLOTSET*)RTSlotSetHandle;
	VK_IRTSLOTSET* InheritedSet = new VK_IRTSLOTSET;
	InheritedSet->BASESLOTSET = BaseSet;
	VK_RTSlotUsage** Descriptions = (VK_RTSlotUsage**)DescriptionsGFX;

	//Find Depth/Stencil Slots and count Color Slots
	bool DEPTH_FOUND = false;
	unsigned char COLORSLOT_COUNT = 0, DEPTHDESC_VECINDEX = 0;
	TGFXLISTCOUNT((&VKCORE->TGFXCORE), Descriptions, DESCCOUNT);
	for (unsigned char i = 0; i < DESCCOUNT; i++) {
		const VK_RTSlotUsage* DESC = Descriptions[i];
		if (DESC->IS_DEPTH) {
			if (DEPTH_FOUND) {
				LOG(tgfx_result_FAIL, "Inherite_RTSlotSet() has failed because there are two depth buffers in the description, which is not supported!");
				delete InheritedSet;
				return tgfx_result_INVALIDARGUMENT;
			}
			DEPTH_FOUND = true;
			DEPTHDESC_VECINDEX = i;
			if (BaseSet->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->DEPTH_OPTYPE == tgfx_operationtype_READ_ONLY &&
				(DESC->OPTYPE == tgfx_operationtype_WRITE_ONLY || DESC->OPTYPE == tgfx_operationtype_READ_AND_WRITE)
				)
			{
				LOG(tgfx_result_FAIL, "Inherite_RTSlotSet() has failed because you can't use a Read-Only DepthSlot with Write Access in a Inherited Set!");
				delete InheritedSet;
				return tgfx_result_INVALIDARGUMENT;
			}
			InheritedSet->DEPTH_OPTYPE = DESC->OPTYPE;
			InheritedSet->STENCIL_OPTYPE = DESC->OPTYPESTENCIL;
		}
		else {
			COLORSLOT_COUNT++;
		}
	}
	if (!DEPTH_FOUND) {
		InheritedSet->DEPTH_OPTYPE = tgfx_operationtype_UNUSED;
	}
	if (COLORSLOT_COUNT != BaseSet->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT) {
		LOG(tgfx_result_FAIL, "Inherite_RTSlotSet() has failed because BaseSet's Color Slot count doesn't match given Descriptions's one!");
		delete InheritedSet;
		return tgfx_result_INVALIDARGUMENT;
	}

	//Check SlotIndexes of Color Slots
	for (unsigned int i = 0; i < COLORSLOT_COUNT; i++) {
		if (i == DEPTHDESC_VECINDEX) {
			continue;
		}
		for (unsigned int j = 1; i + j < COLORSLOT_COUNT; j++) {
			if (i + j == DEPTHDESC_VECINDEX) {
				continue;
			}
			if (Descriptions[i]->SLOTINDEX >= COLORSLOT_COUNT) {
				LOG(tgfx_result_FAIL, "Inherite_RTSlotSet() has failed because some ColorSlots have indexes that are more than or equal to ColorSlotCount! Each slot's index should be unique and lower than ColorSlotCount!");
				delete InheritedSet;
				return tgfx_result_INVALIDARGUMENT;
			}
			if (Descriptions[i]->SLOTINDEX == Descriptions[i + j]->SLOTINDEX) {
				LOG(tgfx_result_FAIL, "Inherite_RTSlotSet() has failed because given Descriptions have some ColorSlots that has same indexes! Each slot's index should be unique and lower than ColorSlotCount!");
				delete InheritedSet;
				return tgfx_result_INVALIDARGUMENT;
			}
		}
	}

	InheritedSet->COLOR_OPTYPEs = new tgfx_operationtype[COLORSLOT_COUNT];
	//Set OPTYPEs of inherited slot set
	for (unsigned int i = 0; i < COLORSLOT_COUNT; i++) {
		if (i == DEPTHDESC_VECINDEX) {
			continue;
		}
		const tgfx_operationtype& BSLOT_OPTYPE = BaseSet->PERFRAME_SLOTSETs[0].COLOR_SLOTs[Descriptions[i]->SLOTINDEX].RT_OPERATIONTYPE;

		if (BSLOT_OPTYPE == tgfx_operationtype_READ_ONLY &&
			(Descriptions[i]->OPTYPE == tgfx_operationtype_WRITE_ONLY || Descriptions[i]->OPTYPE == tgfx_operationtype_READ_AND_WRITE)
			)
		{
			LOG(tgfx_result_FAIL, "Inherite_RTSlotSet() has failed because you can't use a Read-Only ColorSlot with Write Access in a Inherited Set!");
			delete InheritedSet;
			return tgfx_result_INVALIDARGUMENT;
		}
		InheritedSet->COLOR_OPTYPEs[Descriptions[i]->SLOTINDEX] = Descriptions[i]->OPTYPE;
	}

	*InheritedSlotSetHandle = (tgfx_inheritedrtslotset)InheritedSet;
	VKContentManager->IRT_SLOTSETs.push_back(tapi_GetThisThreadIndex(JobSys), InheritedSet);
	return tgfx_result_SUCCESS;
}

void vk_gpudatamanager::Delete_RTSlotSet(tgfx_rtslotset RTSlotSetHandle) {
	if (!RTSlotSetHandle) {
		return;
	}
	VK_RTSLOTSET* SlotSet = (VK_RTSLOTSET*)RTSlotSetHandle;

	delete[] SlotSet->PERFRAME_SLOTSETs[0].COLOR_SLOTs;
	delete SlotSet->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT;
	delete[] SlotSet->PERFRAME_SLOTSETs[1].COLOR_SLOTs;
	delete SlotSet->PERFRAME_SLOTSETs[1].DEPTHSTENCIL_SLOT;
	delete SlotSet;

	unsigned char ThreadID = 0;
	unsigned int ElementIndex = 0;
	if (VKContentManager->RT_SLOTSETs.SearchAllThreads(SlotSet, ThreadID, ElementIndex)) {
		std::unique_lock<std::mutex> Locker;
		VKContentManager->RT_SLOTSETs.PauseAllOperations(Locker);
		VKContentManager->RT_SLOTSETs.erase(ThreadID, ElementIndex);
	}
}

void vk_gpudatamanager::Delete_InheritedRTSlotSet(tgfx_inheritedrtslotset irtslotset) {
	LOG(tgfx_result_NOTCODED, "Delete_InheritedRTSlotSet() isn't coded in TGFX's Vulkan backend!");
}