#include "VK_GPUContentManager.h"
#include "Vulkan_Renderer_Core.h"
#include "Vulkan/VulkanSource/Vulkan_Core.h"
#define VKRENDERER ((Vulkan::Renderer*)GFXRENDERER)
#define VKGPU (((Vulkan::Vulkan_Core*)GFX)->VK_States.GPU_TO_RENDER)

#define MAXDESCSETCOUNT_PERPOOL 100
#define MAXUBUFFERCOUNT_PERPOOL 100
#define MAXSBUFFERCOUNT_PERPOOL 100
#define MAXSAMPLERCOUNT_PERPOOL 100
#define MAXIMAGECOUNT_PERPOOL	100

namespace Vulkan {
	VkBuffer Create_VkBuffer(unsigned int size, VkBufferUsageFlags usage) {
		VkBuffer buffer;

		VkBufferCreateInfo ci{};
		ci.usage = usage;
		if (VKGPU->QUEUEs.size() > 1) {
			ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
		}
		else {
			ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		ci.queueFamilyIndexCount = VKGPU->QUEUEs.size();
		ci.pQueueFamilyIndices = VKGPU->AllQueueFamilies;
		ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		ci.size = size;

		if (vkCreateBuffer(VKGPU->Logical_Device, &ci, nullptr, &buffer) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Create_VkBuffer has failed!");
		}
		return buffer;
	}
	GPU_ContentManager::GPU_ContentManager() : MESHBUFFERs(*GFX->JobSys), INDEXBUFFERs(*GFX->JobSys), TEXTUREs(*GFX->JobSys), GLOBALBUFFERs(*GFX->JobSys), SHADERSOURCEs(*GFX->JobSys),
		SHADERPROGRAMs(*GFX->JobSys), SHADERPINSTANCEs(*GFX->JobSys), VERTEXATTRIBLAYOUTs(*GFX->JobSys), RT_SLOTSETs(*GFX->JobSys), STAGINGBUFFERs(*GFX->JobSys),
		DescSets_toCreateUpdate(*GFX->JobSys), DescSets_toCreate(*GFX->JobSys), DescSets_toJustUpdate(*GFX->JobSys), SAMPLERs(*GFX->JobSys), IRT_SLOTSETs(*GFX->JobSys),
		DeleteTextureList(*GFX->JobSys), NextFrameDeleteTextureCalls(*GFX->JobSys), GLOBALTEXTUREs(*GFX->JobSys){

		//Do memory allocations
		for (unsigned int allocindex = 0; allocindex < VKGPU->ALLOCs.size(); allocindex++) {
			VK_MemoryAllocation& ALLOC = VKGPU->ALLOCs[allocindex];
			if (!ALLOC.FullSize) {
				continue;
			}

			VkMemoryRequirements memrequirements;
			VkBufferUsageFlags USAGEFLAGs = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			uint64_t AllocSize = ALLOC.FullSize;
			VkBuffer GPULOCAL_buf = Create_VkBuffer(AllocSize, USAGEFLAGs);
			vkGetBufferMemoryRequirements(VKGPU->Logical_Device, GPULOCAL_buf, &memrequirements);
			if (!(memrequirements.memoryTypeBits & (1 << ALLOC.MemoryTypeIndex))) {
				LOG_CRASHING_TAPI("GPU Local Memory Allocation doesn't support the MemoryType!");
				return;
			}
			VkMemoryAllocateInfo ci{};
			ci.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			ci.allocationSize = memrequirements.size;
			ci.memoryTypeIndex = ALLOC.MemoryTypeIndex;
			VkDeviceMemory allocated_memory;
			if (vkAllocateMemory(VKGPU->Logical_Device, &ci, nullptr, &allocated_memory) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("GPU_ContentManager initialization has failed because vkAllocateMemory GPULocalBuffer has failed!");
			}
			ALLOC.Allocated_Memory = allocated_memory;
			ALLOC.UnusedSize.DirectStore(AllocSize);
			ALLOC.MappedMemory = nullptr;
			ALLOC.Buffer = GPULOCAL_buf;
			if (vkBindBufferMemory(VKGPU->Logical_Device, GPULOCAL_buf, allocated_memory, 0) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Binding buffer to the allocated memory has failed!");
			}

			//If allocation is device local, it is not mappable. So continue.
			if (ALLOC.TYPE == GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL) {
				continue;
			}

			if (vkMapMemory(VKGPU->Logical_Device, allocated_memory, 0, memrequirements.size, 0, &ALLOC.MappedMemory) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Mapping the HOSTVISIBLE memory has failed!");
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
			dp_ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			dp_ci.maxSets = MAXDESCSETCOUNT_PERPOOL;
			dp_ci.pNext = nullptr;
			dp_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			VkDescriptorPoolSize SIZEs[4];
			SIZEs[0].descriptorCount = MAXIMAGECOUNT_PERPOOL;
			SIZEs[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			SIZEs[1].descriptorCount = MAXSBUFFERCOUNT_PERPOOL;
			SIZEs[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			SIZEs[2].descriptorCount = MAXUBUFFERCOUNT_PERPOOL;
			SIZEs[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			SIZEs[3].descriptorCount = MAXSAMPLERCOUNT_PERPOOL;
			SIZEs[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

			dp_ci.poolSizeCount = 4;
			dp_ci.pPoolSizes = SIZEs;
			if (vkCreateDescriptorPool(VKGPU->Logical_Device, &dp_ci, nullptr, &MaterialRelated_DescPool.pool) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Material Related Descriptor Pool Creation has failed! So GFXContentManager system initialization has failed!");
				return;
			}

			MaterialRelated_DescPool.REMAINING_IMAGE.DirectStore(MAXIMAGECOUNT_PERPOOL);
			MaterialRelated_DescPool.REMAINING_SAMPLER.DirectStore(MAXSAMPLERCOUNT_PERPOOL);
			MaterialRelated_DescPool.REMAINING_SBUFFER.DirectStore(MAXSBUFFERCOUNT_PERPOOL);
			MaterialRelated_DescPool.REMAINING_UBUFFER.DirectStore(MAXUBUFFERCOUNT_PERPOOL);
			MaterialRelated_DescPool.REMAINING_SET.DirectStore(MAXDESCSETCOUNT_PERPOOL);
		}
	}
	GPU_ContentManager::~GPU_ContentManager() {
		Destroy_AllResources();
	}
	void GPU_ContentManager::Destroy_RenderGraphRelatedResources() {
		//Destroy Material Types and Instances
		{
			std::unique_lock<std::mutex> Locker;
			SHADERPROGRAMs.PauseAllOperations(Locker);
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int MatTypeIndex = 0; MatTypeIndex < SHADERPROGRAMs.size(ThreadID); MatTypeIndex++) {
					VK_GraphicsPipeline* PSO = GFXHandleConverter(VK_GraphicsPipeline*, SHADERPROGRAMs.get(ThreadID, MatTypeIndex));
					vkDestroyDescriptorSetLayout(VKGPU->Logical_Device, PSO->General_DescSet.Layout, nullptr);
					vkDestroyDescriptorSetLayout(VKGPU->Logical_Device, PSO->Instance_DescSet.Layout, nullptr);
					delete[] PSO->General_DescSet.Descs;
					vkDestroyPipelineLayout(VKGPU->Logical_Device, PSO->PipelineLayout, nullptr);
					vkDestroyPipeline(VKGPU->Logical_Device, PSO->PipelineObject, nullptr);
					delete PSO;
				}
				SHADERPROGRAMs.clear(ThreadID);
			}
			Locker.unlock();

			std::unique_lock<std::mutex> InstanceLocker;
			SHADERPINSTANCEs.PauseAllOperations(InstanceLocker);
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int MatInstIndex = 0; MatInstIndex < SHADERPINSTANCEs.size(ThreadID); MatInstIndex++) {
					VK_PipelineInstance* PSO = GFXHandleConverter(VK_PipelineInstance*, SHADERPINSTANCEs.get(ThreadID, MatInstIndex));
					vkDestroyDescriptorSetLayout(VKGPU->Logical_Device, PSO->DescSet.Layout, nullptr);
					delete[] PSO->DescSet.Descs;
					delete PSO;
				}
				SHADERPINSTANCEs.clear(ThreadID);
			}
			InstanceLocker.unlock();
		}

		//There is no need to free each descriptor set, just destroy pool
		if (MaterialRelated_DescPool.pool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(VKGPU->Logical_Device, MaterialRelated_DescPool.pool, nullptr);
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
	void GPU_ContentManager::Destroy_AllResources() {
		//Destroy Shader Sources
		{
			std::unique_lock<std::mutex> Locker;
			SHADERSOURCEs.PauseAllOperations(Locker);
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int i = 0; i < SHADERSOURCEs.size(ThreadID); i++) {
					VK_ShaderSource* ShaderSource = SHADERSOURCEs.get(ThreadID, i);
					vkDestroyShaderModule(VKGPU->Logical_Device, ShaderSource->Module, nullptr);
					delete ShaderSource;
				}
				SHADERSOURCEs.clear(ThreadID);
			}
		}
		//Destroy Samplers
		{
			std::unique_lock<std::mutex> Locker;
			SAMPLERs.PauseAllOperations(Locker);
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int i = 0; i < SAMPLERs.size(ThreadID); i++) {
					VK_Sampler* Sampler = SAMPLERs.get(ThreadID, i);
					vkDestroySampler(VKGPU->Logical_Device, Sampler->Sampler, nullptr);
					delete Sampler;
				}
				SAMPLERs.clear(ThreadID);
			}
		}
		//Destroy Vertex Attribute Layouts
		{
			std::unique_lock<std::mutex> Locker;
			VERTEXATTRIBLAYOUTs.PauseAllOperations(Locker);
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int i = 0; i < VERTEXATTRIBLAYOUTs.size(ThreadID); i++) {
					VK_VertexAttribLayout* LAYOUT = VERTEXATTRIBLAYOUTs.get(ThreadID, i);
					delete[] LAYOUT->AttribDescs;
					delete[] LAYOUT->Attributes;
					delete LAYOUT;
				}
				VERTEXATTRIBLAYOUTs.clear(ThreadID);
			}
		}
		//Destroy RTSLotSets
		{
			std::unique_lock<std::mutex> Locker;
			RT_SLOTSETs.PauseAllOperations(Locker);
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int i = 0; i < RT_SLOTSETs.size(ThreadID); i++) {
					VK_RTSLOTSET* SlotSet = RT_SLOTSETs.get(ThreadID, i);
					delete[] SlotSet->PERFRAME_SLOTSETs[0].COLOR_SLOTs;
					delete SlotSet->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT;
					delete[] SlotSet->PERFRAME_SLOTSETs[1].COLOR_SLOTs;
					delete SlotSet->PERFRAME_SLOTSETs[1].DEPTHSTENCIL_SLOT;
					delete SlotSet;
				}
				RT_SLOTSETs.clear(ThreadID);
			}
		}
		//Destroy IRTSlotSets
		{
			std::unique_lock<std::mutex> Locker;
			IRT_SLOTSETs.PauseAllOperations(Locker);
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int i = 0; i < IRT_SLOTSETs.size(ThreadID); i++) {
					VK_IRTSLOTSET* ISlotSet = IRT_SLOTSETs.get(ThreadID, i);
					delete[] ISlotSet->COLOR_OPTYPEs;
					delete ISlotSet;
				}
				IRT_SLOTSETs.clear(ThreadID);
			}
		}
		//Destroy Global Buffers
		{
			std::unique_lock<std::mutex> Locker;
			GLOBALBUFFERs.PauseAllOperations(Locker);
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int i = 0; i < GLOBALBUFFERs.size(ThreadID); i++) {
					delete GLOBALBUFFERs.get(ThreadID, i);
				}
				GLOBALBUFFERs.clear(ThreadID);
			}
		}
		//Destroy Mesh Buffers
		{
			std::unique_lock<std::mutex> Locker;
			MESHBUFFERs.PauseAllOperations(Locker);
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int i = 0; i < MESHBUFFERs.size(ThreadID); i++) {
					delete MESHBUFFERs.get(ThreadID, i);
				}
				MESHBUFFERs.clear(ThreadID);
			}
		}
		//Destroy Index Buffers
		{
			std::unique_lock<std::mutex> Locker;
			INDEXBUFFERs.PauseAllOperations(Locker);
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int i = 0; i < INDEXBUFFERs.size(ThreadID); i++) {
					delete INDEXBUFFERs.get(ThreadID, i);
				}
				INDEXBUFFERs.clear(ThreadID);
			}
		}
		//Destroy Staging Buffers
		{
			std::unique_lock<std::mutex> Locker;
			STAGINGBUFFERs.PauseAllOperations(Locker);
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int i = 0; i < STAGINGBUFFERs.size(ThreadID); i++) {
					delete STAGINGBUFFERs.get(ThreadID, i);
				}
				STAGINGBUFFERs.clear(ThreadID);
			}
		}
		//Destroy Textures
		{
			std::unique_lock<std::mutex> Locker;
			TEXTUREs.PauseAllOperations(Locker);
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int i = 0; i < TEXTUREs.size(ThreadID); i++) {
					VK_Texture* Texture = TEXTUREs.get(ThreadID, i);
					vkDestroyImageView(VKGPU->Logical_Device, Texture->ImageView, nullptr);
					vkDestroyImage(VKGPU->Logical_Device, Texture->Image, nullptr);
					delete Texture; 
				}
				TEXTUREs.clear(ThreadID);
			}
		}
		//Destroy Global Buffer related Descriptor Datas
		{
			vkDestroyDescriptorPool(VKGPU->Logical_Device, GlobalBuffers_DescPool, nullptr);
			vkDestroyDescriptorSetLayout(VKGPU->Logical_Device, GlobalShaderInputs_DescSet.Layout, nullptr);
		}
	}

	void CopyDescriptorSets(VK_DescSet& Set, vector<VkCopyDescriptorSet>& CopyVector, vector<VkDescriptorSet*>& CopyTargetSets) {
		for (unsigned int DescIndex = 0; DescIndex < Set.DescCount; DescIndex++) {
			VkCopyDescriptorSet copyinfo;
			VK_Descriptor& SourceDesc = GFXHandleConverter(VK_Descriptor*, Set.Descs)[DescIndex];
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
	void GPU_ContentManager::Apply_ResourceChanges() {
		const unsigned char FrameIndex = VKRENDERER->GetCurrentFrameIndex();

		//Manage Global Descriptor Set
		if (UnboundGlobalDescSet != VK_NULL_HANDLE) {
			vkFreeDescriptorSets(VKGPU->Logical_Device, GlobalBuffers_DescPool, 1, &UnboundGlobalDescSet);
			UnboundGlobalDescSet = VK_NULL_HANDLE;
		}
		if(GlobalShaderInputs_DescSet.ShouldRecreate.load()){
			UnboundGlobalDescSet = GlobalShaderInputs_DescSet.Set;
			GlobalShaderInputs_DescSet.Set = VK_NULL_HANDLE;
			CreateandUpdate_GlobalDescSet();
		}

		//Manage Material Related Descriptor Sets
		{
			//Destroy descriptor sets that are changed last frame
			if (UnboundMaterialDescSetList.size()) {
				VK_DescPool& DP = MaterialRelated_DescPool;
				vkFreeDescriptorSets(VKGPU->Logical_Device, DP.pool, UnboundMaterialDescSetList.size(),
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
				vector<VkDescriptorSet> Sets;
				vector<VkDescriptorSet*> SetPTRs;
				vector<VkDescriptorSetLayout> SetLayouts;
				std::unique_lock<std::mutex> Locker;
				DescSets_toCreate.PauseAllOperations(Locker);
				for (unsigned int ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
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
					vkAllocateDescriptorSets(VKGPU->Logical_Device, &al_in, Sets.data());
					for (unsigned int SetIndex = 0; SetIndex < Sets.size(); SetIndex++) {
						*SetPTRs[SetIndex] = Sets[SetIndex];
					}
				}
			}
			//Create Descriptor Sets for material types/instances that are created before and used recently (last 2 frames), to update their descriptor sets
			{
				vector<VkDescriptorSet> NewSets;
				vector<VkDescriptorSet*> SetPTRs;
				vector<VkDescriptorSetLayout> SetLayouts;

				//Copy descriptor sets exactly, then update with this frame's SetMaterial_xxx calls
				vector<VkCopyDescriptorSet> CopySetInfos;
				vector<VkDescriptorSet*> CopyTargetSets;


				std::unique_lock<std::mutex> Locker;
				DescSets_toCreateUpdate.PauseAllOperations(Locker);
				for (unsigned int ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
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
							LOG_NOTCODED_TAPI("Descriptor Set atomic_uchar isn't supposed to have a value that's 2+! Please check 'Handle Descriptor Sets' in Vulkan Renderer->Run()", true);
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
					vkAllocateDescriptorSets(VKGPU->Logical_Device, &al_in, NewSets.data());
					for (unsigned int SetIndex = 0; SetIndex < NewSets.size(); SetIndex++) {
						*SetPTRs[SetIndex] = NewSets[SetIndex];
					}

					for (unsigned int CopyIndex = 0; CopyIndex < CopySetInfos.size(); CopyIndex++) {
						CopySetInfos[CopyIndex].dstSet = *CopyTargetSets[CopyIndex];
					}
					vkUpdateDescriptorSets(VKGPU->Logical_Device, 0, nullptr, CopySetInfos.size(), CopySetInfos.data());
				}
			}
			//Update descriptor sets
			{
				vector<VkWriteDescriptorSet> UpdateInfos;
				std::unique_lock<std::mutex> Locker1;
				DescSets_toCreateUpdate.PauseAllOperations(Locker1);
				for (unsigned int ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
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
							info.pImageInfo = &GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].info;
							GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
							break;
						case DescType::SAMPLER:
							info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
							info.dstBinding = Call.BindingIndex;
							info.dstArrayElement = Call.ElementIndex;
							info.pImageInfo = &GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].info;
							GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
							break;
						case DescType::UBUFFER:
							info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
							info.dstBinding = Call.BindingIndex;
							info.dstArrayElement = Call.ElementIndex;
							info.pBufferInfo = &GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].Info;
							GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
							break;
						case DescType::SBUFFER:
							info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
							info.dstBinding = Call.BindingIndex;
							info.dstArrayElement = Call.ElementIndex;
							info.pBufferInfo = &GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].Info;
							GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
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
				for (unsigned int ThreadIndex = 0; ThreadIndex < GFX->JobSys->GetThreadCount(); ThreadIndex++) {
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
							info.pImageInfo = &GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].info;
							GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
							break;
						case DescType::SAMPLER:
							info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
							info.dstBinding = Call.BindingIndex;
							info.dstArrayElement = Call.ElementIndex;
							info.pImageInfo = &GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].info;
							GFXHandleConverter(VK_DescImageElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
							break;
						case DescType::UBUFFER:
							info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
							info.dstBinding = Call.BindingIndex;
							info.dstArrayElement = Call.ElementIndex;
							info.pBufferInfo = &GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].Info;
							GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
							break;
						case DescType::SBUFFER:
							info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
							info.dstBinding = Call.BindingIndex;
							info.dstArrayElement = Call.ElementIndex;
							info.pBufferInfo = &GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].Info;
							GFXHandleConverter(VK_DescBufferElement*, Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
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

				vkUpdateDescriptorSets(VKGPU->Logical_Device, UpdateInfos.size(), UpdateInfos.data(), 0, nullptr);
			}
		}

		//Handle RTSlotSet changes by recreating framebuffers of draw passes
		//by "RTSlotSet changes": DrawPass' slotset is changed to different one or slotset's slots is changed.
		for (unsigned int DrawPassIndex = 0; DrawPassIndex < VKRENDERER->DrawPasses.size(); DrawPassIndex++) {
			VK_DrawPass* DP = VKRENDERER->DrawPasses[DrawPassIndex];
			unsigned char ChangeInfo = DP->SlotSetChanged.load();
			if (ChangeInfo == FrameIndex + 1 || ChangeInfo == 3 || DP->SLOTSET->PERFRAME_SLOTSETs[FrameIndex].IsChanged.load()) {
				//This is safe because this FB is used 2 frames ago and CPU already waits for the frame's GPU process to end
				vkDestroyFramebuffer(VKGPU->Logical_Device, DP->FBs[FrameIndex], nullptr);

				VkFramebufferCreateInfo fb_ci = DP->SLOTSET->FB_ci[FrameIndex];
				fb_ci.renderPass = DP->RenderPassObject;
				if (vkCreateFramebuffer(VKGPU->Logical_Device, &fb_ci, nullptr, &DP->FBs[FrameIndex]) != VK_SUCCESS) {
					LOG_CRASHING_TAPI("vkCreateFramebuffer() has failed while changing one of the drawpasses' current frame slot's texture! Please report this!");
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
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int i = 0; i < DeleteTextureList.size(ThreadID); i++) {
					VK_Texture* Texture = DeleteTextureList.get(ThreadID, i);
					if (Texture->Image) {
						vkDestroyImageView(VKGPU->Logical_Device, Texture->ImageView, nullptr);
						vkDestroyImage(VKGPU->Logical_Device, Texture->Image, nullptr);
					}
					if (Texture->Block.MemAllocIndex != UINT32_MAX) {
						VKGPU->ALLOCs[Texture->Block.MemAllocIndex].FreeBlock(Texture->Block.Offset);
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
			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int i = 0; i < NextFrameDeleteTextureCalls.size(ThreadID); i++) {
					DeleteTextureList.push_back(0, NextFrameDeleteTextureCalls.get(ThreadID, i));
				}
				NextFrameDeleteTextureCalls.clear(ThreadID);
			}
		}


	}
	void GPU_ContentManager::CreateandUpdate_GlobalDescSet() {
		//Descriptor Set allocation
		{
			VkDescriptorSetAllocateInfo descset_ai = {};
			descset_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descset_ai.pNext = nullptr;
			descset_ai.descriptorPool = GlobalBuffers_DescPool;
			descset_ai.descriptorSetCount = 1;
			descset_ai.pSetLayouts = &GlobalShaderInputs_DescSet.Layout;
			if (vkAllocateDescriptorSets(VKGPU->Logical_Device, &descset_ai, &GlobalShaderInputs_DescSet.Set) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("CreateandUpdate_GlobalDescSet() has failed at vkAllocateDescriptorSets()!");
			}
		}
		//Update Descriptor Set
		{
			vector<VkWriteDescriptorSet> UpdateInfos;
			vector<VkDescriptorBufferInfo*> DeleteBufferInfoList;
			vector<VkDescriptorImageInfo*> DeleteImageInfoList;

			for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
				for (unsigned int i = 0; i < GLOBALBUFFERs.size(ThreadID); i++) {
					VK_GlobalBuffer* buf = GLOBALBUFFERs.get(ThreadID, i);
					VkWriteDescriptorSet info = {};
					info.descriptorCount = 1;
					if (buf->isUniform) {
						info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					}
					else {
						info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					}
					info.dstArrayElement = 0;
					info.dstBinding = buf->BINDINGPOINT;
					info.dstSet = GlobalShaderInputs_DescSet.Set;
					VkDescriptorBufferInfo* descbufinfo = new VkDescriptorBufferInfo;
					VkBuffer nonobj; VkDeviceSize nonoffset;
					descbufinfo->buffer = VKGPU->ALLOCs[buf->Block.MemAllocIndex].Buffer;
					descbufinfo->offset = buf->Block.Offset;
					descbufinfo->range = static_cast<VkDeviceSize>(buf->DATA_SIZE);
					info.pBufferInfo = descbufinfo;
					info.pNext = nullptr;
					info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					DeleteBufferInfoList.push_back(descbufinfo);
					UpdateInfos.push_back(info);
				}
				for (unsigned int i = 0; i < GLOBALTEXTUREs.size(ThreadID); i++) {
					VK_GlobalTexture* tex = GLOBALTEXTUREs.get(ThreadID, i);
					VkDescriptorType texDescType;
					if (tex->isSampledTexture) {
						texDescType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					}
					else {
						texDescType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					}
					for (unsigned int ElementIndex = 0; ElementIndex < tex->ElementCount; ElementIndex++) {
						VkWriteDescriptorSet info = {};
						info.descriptorCount = 1;
						info.dstArrayElement = ElementIndex;
						info.dstBinding = tex->BINDINGPOINT;
						info.dstSet = GlobalShaderInputs_DescSet.Set;
						VkDescriptorImageInfo* desciminfo = new VkDescriptorImageInfo;
						if (tex->Elements[ElementIndex].Texture) {
							desciminfo->imageView = tex->Elements[ElementIndex].Texture->ImageView;
						}
						else {
							desciminfo->imageView = VK_NULL_HANDLE;
						}
						info.descriptorType = texDescType;
						desciminfo->imageLayout = tex->Elements[ElementIndex].Layout;
						desciminfo->sampler = tex->Elements[ElementIndex].Sampler;
						info.pImageInfo = desciminfo;
						info.pNext = nullptr;
						info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						DeleteImageInfoList.push_back(desciminfo);
						UpdateInfos.push_back(info);
					}
				}
			}

			vkUpdateDescriptorSets(VKGPU->Logical_Device, UpdateInfos.size(), UpdateInfos.data(), 0, nullptr);

			for (unsigned int DeleteIndex = 0; DeleteIndex < DeleteBufferInfoList.size(); DeleteIndex++) {
				delete DeleteBufferInfoList[DeleteIndex];
			}
			for (unsigned int DeleteIndex = 0; DeleteIndex < DeleteImageInfoList.size(); DeleteIndex++) {
				delete DeleteImageInfoList[DeleteIndex];
			}
		}
	}

	void GPU_ContentManager::Resource_Finalizations() {
		//Create Descriptor Pool for Global Shader Inputs and Create Descriptor Set for them
		{
			std::unique_lock<std::mutex> GlobalBufferLocker;
			GLOBALBUFFERs.PauseAllOperations(GlobalBufferLocker);
			std::unique_lock<std::mutex> GlobalTextureLocker;
			GLOBALTEXTUREs.PauseAllOperations(GlobalTextureLocker);
			vector<unsigned int> BINDINDEXes;


			//Descriptor Set Layout creation
			unsigned int UNIFORMBUFFER_COUNT = 0, STORAGEBUFFER_COUNT = 0, SAMPLED_COUNT = 0, IMAGE_COUNT = 0;
			{
				vector<VkDescriptorSetLayoutBinding> bindings;
				for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
					for (unsigned int i = 0; i < GLOBALBUFFERs.size(ThreadID); i++) {
						VK_GlobalBuffer* globbuf = GLOBALBUFFERs.get(ThreadID, i);

						//Check if the binding point is already used
						for (unsigned int SearchIndex = 0; SearchIndex < BINDINDEXes.size(); SearchIndex++) {
							if (BINDINDEXes[SearchIndex] == globbuf->BINDINGPOINT) {
								LOG_CRASHING_TAPI("There are colliding global shader input (global buffer/texture) binding indexes!");
								return;
							}
						}
						BINDINDEXes.push_back(globbuf->BINDINGPOINT);


						bindings.push_back(VkDescriptorSetLayoutBinding());
						bindings[i].binding = globbuf->BINDINGPOINT;
						bindings[i].descriptorCount = 1;
						if (globbuf->isUniform) {
							bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						}
						else {
							bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						}
						bindings[i].pImmutableSamplers = VK_NULL_HANDLE;
						bindings[i].stageFlags = Find_VkShaderStages(globbuf->ACCESSED_STAGEs);
						//Buffer Type Counting
						switch (bindings[i].descriptorType) {
						case VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
						case VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
							STORAGEBUFFER_COUNT++;
							break;
						case VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
						case VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
							UNIFORMBUFFER_COUNT++;
							break;
						}
					}
				}

				for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
					for (unsigned int i = 0; i < GLOBALTEXTUREs.size(ThreadID); i++) {
						VK_GlobalTexture* gt = GLOBALTEXTUREs.get(ThreadID, i);
						
						for (unsigned int SearchIndex = 0; SearchIndex < BINDINDEXes.size(); SearchIndex++) {
							if (BINDINDEXes[SearchIndex] == gt->BINDINGPOINT) {
								LOG_CRASHING_TAPI("There are colliding global shader input (global buffer/texture) binding indexes!");
								return;
							}
						}


						bindings.push_back(VkDescriptorSetLayoutBinding());
						VkDescriptorSetLayoutBinding& targetbinding = bindings[bindings.size() - 1];
						targetbinding.binding = gt->BINDINGPOINT;
						targetbinding.descriptorCount = gt->ElementCount;
						if (gt->isSampledTexture) {
							targetbinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
							SAMPLED_COUNT += gt->ElementCount;
						}
						else {
							targetbinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
							IMAGE_COUNT += gt->ElementCount;
						}
						targetbinding.pImmutableSamplers = VK_NULL_HANDLE;
						targetbinding.stageFlags = Find_VkShaderStages(gt->ACCESSED_STAGEs);
					}
				}

				if (bindings.size()) {
					VkDescriptorSetLayoutCreateInfo DescSetLayout_ci = {};
					DescSetLayout_ci.flags = 0;
					DescSetLayout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					DescSetLayout_ci.pNext = nullptr;
					DescSetLayout_ci.bindingCount = bindings.size();
					DescSetLayout_ci.pBindings = bindings.data();
					if (vkCreateDescriptorSetLayout(VKGPU->Logical_Device, &DescSetLayout_ci, nullptr, &GlobalShaderInputs_DescSet.Layout) != VK_SUCCESS) {
						LOG_CRASHING_TAPI("Create_RenderGraphResources() has failed at vkCreateDescriptorSetLayout()!");
						return;
					}
				}
			}

			//Descriptor Pool and Descriptor Set creation
			//Create a descriptor pool that has 2 times of sum of counts above!
			vector<VkDescriptorPoolSize> poolsizes;
			if (UNIFORMBUFFER_COUNT > 0) {
				VkDescriptorPoolSize UDescCount;
				UDescCount.descriptorCount = UNIFORMBUFFER_COUNT * 2;
				UDescCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				poolsizes.push_back(UDescCount);
			}
			if (STORAGEBUFFER_COUNT > 0) {
				VkDescriptorPoolSize SDescCount;
				SDescCount.descriptorCount = STORAGEBUFFER_COUNT * 2;
				SDescCount.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				poolsizes.push_back(SDescCount);
			}
			if (SAMPLED_COUNT > 0) {
				VkDescriptorPoolSize SampledCount;
				SampledCount.descriptorCount = SAMPLED_COUNT * 2;
				SampledCount.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				poolsizes.push_back(SampledCount);
			}
			if (IMAGE_COUNT > 0) {
				VkDescriptorPoolSize ImageCount;
				ImageCount.descriptorCount = IMAGE_COUNT * 2;
				ImageCount.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				poolsizes.push_back(ImageCount);
			}
			if (poolsizes.size()) {
				VkDescriptorPoolCreateInfo descpool_ci = {};
				descpool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				descpool_ci.maxSets = 2;
				descpool_ci.pPoolSizes = poolsizes.data();
				descpool_ci.poolSizeCount = poolsizes.size();
				descpool_ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
				descpool_ci.pNext = nullptr;
				if (poolsizes.size()) {
					if (vkCreateDescriptorPool(VKGPU->Logical_Device, &descpool_ci, nullptr, &GlobalBuffers_DescPool) != VK_SUCCESS) {
						LOG_CRASHING_TAPI("Create_RenderGraphResources() has failed at vkCreateDescriptorPool()!");
					}
				}
			}
			CreateandUpdate_GlobalDescSet();
		}

	}
	
	TAPIResult GPU_ContentManager::Suballocate_Buffer(VkBuffer BUFFER, VkBufferUsageFlags UsageFlags, MemoryBlock& Block) {
		VkMemoryRequirements bufferreq;
		vkGetBufferMemoryRequirements(VKGPU->Logical_Device, BUFFER, &bufferreq);

		VK_MemoryAllocation* MEMALLOC = &VKGPU->ALLOCs[Block.MemAllocIndex];
		
		if (!(bufferreq.memoryTypeBits & (1u << MEMALLOC->MemoryTypeIndex))) {
			LOG_ERROR_TAPI("Intended buffer doesn't support to be stored in specified memory region!");
			return TAPI_FAIL;
		}

		VkDeviceSize AlignmentOffset_ofGPU = 0;
		if (UsageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
			AlignmentOffset_ofGPU = VKGPU->Device_Properties.limits.minStorageBufferOffsetAlignment;
		}
		if (UsageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
			AlignmentOffset_ofGPU = VKGPU->Device_Properties.limits.minUniformBufferOffsetAlignment;
		}
		if (!MEMALLOC->UnusedSize.LimitedSubtract_weak(bufferreq.size + AlignmentOffset_ofGPU, 0)) {
			LOG_ERROR_TAPI("Buffer doesn't fit the remaining memory allocation! SuballocateBuffer has failed.");
			return TAPI_FAIL;
		}


		Block.Offset = MEMALLOC->FindAvailableOffset(bufferreq.size, AlignmentOffset_ofGPU, bufferreq.alignment);
		return TAPI_SUCCESS;
	}
	VkMemoryPropertyFlags ConvertGFXMemoryType_toVulkan(GFX_API::SUBALLOCATEBUFFERTYPEs type) {
		switch (type) {
		case GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL:
			return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::FASTHOSTVISIBLE:
			return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE:
			return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::READBACK:
			return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		default:
			LOG_NOTCODED_TAPI("ConvertGFXMemoryType_toVulkan() doesn't support this type!", true);
			return 0;
		}
	}
	TAPIResult GPU_ContentManager::Suballocate_Image(VK_Texture& Texture) {
		VkMemoryRequirements req;
		vkGetImageMemoryRequirements(VKGPU->Logical_Device, Texture.Image, &req);
		VkDeviceSize size = req.size;

		VkDeviceSize Offset = 0;
		bool Found_Offset = false;
		VK_MemoryAllocation* MEMALLOC = &VKGPU->ALLOCs[Texture.Block.MemAllocIndex];
		if (!(req.memoryTypeBits & (1u << MEMALLOC->MemoryTypeIndex))) {
			LOG_ERROR_TAPI("Intended texture doesn't support to be stored in the specified memory region!");
			return TAPI_FAIL;
		}
		if (!MEMALLOC->UnusedSize.LimitedSubtract_weak(req.size, 0)) {
			LOG_ERROR_TAPI("Buffer doesn't fit the remaining memory allocation! SuballocateBuffer has failed.");
			return TAPI_FAIL;
		}
		Offset = MEMALLOC->FindAvailableOffset(req.size, 0, req.alignment);

		if (vkBindImageMemory(VKGPU->Logical_Device, Texture.Image, MEMALLOC->Allocated_Memory, Offset) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("VKContentManager->Suballocate_Image() has failed at VkBindImageMemory()!");
			return TAPI_FAIL;
		}
		Texture.Block.Offset = Offset;
		return TAPI_SUCCESS;
	}

	unsigned int GPU_ContentManager::Calculate_sizeofVertexLayout(const GFX_API::DATA_TYPE* ATTRIBUTEs, unsigned int count) {
		unsigned int size = 0;
		for (unsigned int i = 0; i < count; i++) {
			size += GFX_API::Get_UNIFORMTYPEs_SIZEinbytes(ATTRIBUTEs[i]);
		}
		return size;
	}
	TAPIResult GPU_ContentManager::Create_SamplingType(GFX_API::TEXTURE_DIMENSIONs dimension, unsigned int MinimumMipLevel, unsigned int MaximumMipLevel,
		GFX_API::TEXTURE_MIPMAPFILTER MINFILTER, GFX_API::TEXTURE_MIPMAPFILTER MAGFILTER, GFX_API::TEXTURE_WRAPPING WRAPPING_WIDTH,
		GFX_API::TEXTURE_WRAPPING WRAPPING_HEIGHT, GFX_API::TEXTURE_WRAPPING WRAPPING_DEPTH, GFX_API::GFXHandle& SamplingTypeHandle) {
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
		if (vkCreateSampler(VKGPU->Logical_Device, &s_ci, nullptr, &SAMPLER->Sampler) != VK_SUCCESS) {
			delete SAMPLER;
			LOG_ERROR_TAPI("GFXContentManager->Create_SamplingType() has failed at vkCreateSampler!");
			return TAPI_FAIL;
		}
		SamplingTypeHandle = SAMPLER;
		SAMPLERs.push_back(GFX->JobSys->GetThisThreadIndex(), SAMPLER);
		return TAPI_SUCCESS;
	}
	
	bool GPU_ContentManager::Create_DescSet(VK_DescSet* Set) {
		if (!Set->DescCount) {
			return true;
		}
		if(!MaterialRelated_DescPool.REMAINING_IMAGE.LimitedSubtract_weak(Set->DescImagesCount, 0) ||
			!MaterialRelated_DescPool.REMAINING_SAMPLER.LimitedSubtract_weak(Set->DescSamplersCount, 0) ||
			!MaterialRelated_DescPool.REMAINING_SBUFFER.LimitedSubtract_weak(Set->DescSBuffersCount, 0) ||
			!MaterialRelated_DescPool.REMAINING_UBUFFER.LimitedSubtract_weak(Set->DescUBuffersCount, 0) ||
			!MaterialRelated_DescPool.REMAINING_SET.LimitedSubtract_weak(1, 0)) {
			LOG_ERROR_TAPI("Create_DescSets() has failed because descriptor pool doesn't have enough space!");
			return false;
		}
		DescSets_toCreate.push_back(GFX->JobSys->GetThisThreadIndex(), Set);
		return true;
	}

	TAPIResult GPU_ContentManager::Create_VertexAttributeLayout(const vector<GFX_API::DATA_TYPE>& Attributes, GFX_API::VERTEXLIST_TYPEs listtype, GFX_API::GFXHandle& Handle) {
		VK_VertexAttribLayout* Layout = new VK_VertexAttribLayout;
		Layout->Attribute_Number = Attributes.size();
		Layout->Attributes = new GFX_API::DATA_TYPE[Attributes.size()];
		for (unsigned int i = 0; i < Attributes.size(); i++) {
			Layout->Attributes[i] = Attributes[i];
		}
		unsigned int size_pervertex = Calculate_sizeofVertexLayout(Layout->Attributes, Attributes.size());
		Layout->size_perVertex = size_pervertex;
		Layout->BindingDesc.binding = 0;
		Layout->BindingDesc.stride = size_pervertex;
		Layout->BindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		Layout->PrimitiveTopology = Find_PrimitiveTopology_byGFXVertexListType(listtype);

		Layout->AttribDescs = new VkVertexInputAttributeDescription[Attributes.size()];
		Layout->AttribDesc_Count = Attributes.size();
		unsigned int stride_ofcurrentattribute = 0;
		for (unsigned int i = 0; i < Attributes.size(); i++) {
			Layout->AttribDescs[i].binding = 0;
			Layout->AttribDescs[i].location = i;
			Layout->AttribDescs[i].offset = stride_ofcurrentattribute;
			Layout->AttribDescs[i].format = Find_VkFormat_byDataType(Layout->Attributes[i]);
			stride_ofcurrentattribute += GFX_API::Get_UNIFORMTYPEs_SIZEinbytes(Layout->Attributes[i]);
		}
		VERTEXATTRIBLAYOUTs.push_back(GFX->JobSys->GetThisThreadIndex(), Layout);
		Handle = Layout;
		return TAPI_SUCCESS;
	}
	void GPU_ContentManager::Delete_VertexAttributeLayout(GFX_API::GFXHandle Layout_ID) {
		LOG_NOTCODED_TAPI("Delete_VertexAttributeLayout() isn't coded yet!", true);
	}


	TAPIResult GPU_ContentManager::Create_StagingBuffer(unsigned int DATASIZE, unsigned int MemoryTypeIndex, GFX_API::GFXHandle& Handle) {
		if (!DATASIZE) {
			LOG_ERROR_TAPI("Staging Buffer DATASIZE is zero!");
			return TAPI_INVALIDARGUMENT;
		}
		if (VKGPU->ALLOCs[MemoryTypeIndex].TYPE == GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL) {
			LOG_ERROR_TAPI("You can't create a staging buffer in DEVICELOCAL memory!");
			return TAPI_INVALIDARGUMENT;
		}
		VkBufferCreateInfo psuedo_ci = {};
		psuedo_ci.flags = 0;
		psuedo_ci.pNext = nullptr;
		if (VKGPU->QUEUEs.size() > 1) {
			psuedo_ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
		}
		else {
			psuedo_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		psuedo_ci.pQueueFamilyIndices = VKGPU->AllQueueFamilies;
		psuedo_ci.queueFamilyIndexCount = VKGPU->QUEUEs.size();
		psuedo_ci.size = DATASIZE;
		psuedo_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		psuedo_ci.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			| VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		VkBuffer Bufferobj;
		if (vkCreateBuffer(VKGPU->Logical_Device, &psuedo_ci, nullptr, &Bufferobj) != VK_SUCCESS) {
			LOG_ERROR_TAPI("Intended staging buffer's creation failed at vkCreateBuffer()!");
			return TAPI_FAIL;
		}
		

		MemoryBlock* StagingBuffer = new MemoryBlock;
		StagingBuffer->MemAllocIndex = MemoryTypeIndex;
		if (Suballocate_Buffer(Bufferobj, psuedo_ci.usage, *StagingBuffer) != VK_SUCCESS) {
			delete StagingBuffer;
			LOG_ERROR_TAPI("Suballocation has failed, so staging buffer creation too!");
			return TAPI_FAIL;
		}
		vkDestroyBuffer(VKGPU->Logical_Device, Bufferobj, nullptr);
		Handle = StagingBuffer;
		STAGINGBUFFERs.push_back(GFX->JobSys->GetThisThreadIndex(), StagingBuffer);
		return TAPI_SUCCESS;
	}
	TAPIResult GPU_ContentManager::Upload_toBuffer(GFX_API::GFXHandle Handle, GFX_API::BUFFER_TYPE Type, const void* DATA, unsigned int DATA_SIZE, unsigned int OFFSET) {
		VkDeviceSize UploadOFFSET = static_cast<VkDeviceSize>(OFFSET);
		unsigned int MEMALLOCINDEX = UINT32_MAX;
		switch (Type) {
		case GFX_API::BUFFER_TYPE::GLOBAL:
			MEMALLOCINDEX = GFXHandleConverter(VK_GlobalBuffer*, Handle)->Block.MemAllocIndex;
			UploadOFFSET += GFXHandleConverter(VK_GlobalBuffer*, Handle)->Block.Offset;
			break;
		case GFX_API::BUFFER_TYPE::STAGING:
			MEMALLOCINDEX = GFXHandleConverter(MemoryBlock*, Handle)->MemAllocIndex;
			UploadOFFSET += GFXHandleConverter(MemoryBlock*, Handle)->Offset;
			break;
		case GFX_API::BUFFER_TYPE::VERTEX:
			MEMALLOCINDEX = GFXHandleConverter(VK_VertexBuffer*, Handle)->Block.MemAllocIndex;
			UploadOFFSET += GFXHandleConverter(VK_VertexBuffer*, Handle)->Block.Offset;
			break;
		case GFX_API::BUFFER_TYPE::INDEX:
			MEMALLOCINDEX = GFXHandleConverter(VK_IndexBuffer*, Handle)->Block.MemAllocIndex;
			UploadOFFSET += GFXHandleConverter(VK_IndexBuffer*, Handle)->Block.Offset;
		default:
			LOG_NOTCODED_TAPI("Upload_toBuffer() doesn't support this type of buffer for now!", true);
			return TAPI_NOTCODED;
		}

		void* MappedMemory = VKGPU->ALLOCs[MEMALLOCINDEX].MappedMemory;
		if (!MappedMemory) {
			LOG_ERROR_TAPI("Memory is not mapped, so you are either trying to upload to an GPU Local buffer or MemoryTypeIndex is not allocated memory type's index!");
			return TAPI_FAIL;
		}
		memcpy(((char*)MappedMemory) + UploadOFFSET, DATA, DATA_SIZE);
		return TAPI_SUCCESS;
	}
	void GPU_ContentManager::Delete_StagingBuffer(GFX_API::GFXHandle StagingBufferHandle) {
		LOG_NOTCODED_TAPI("Delete_StagingBuffer() is not coded!", true);
		/*
		MemoryBlock* SB = GFXHandleConverter(MemoryBlock*, StagingBufferHandle);
		std::vector<VK_MemoryBlock>* MemoryBlockList = nullptr;
		std::mutex* MutexPTR = nullptr;
		switch (SB->Type) {
		case GFX_API::SUBALLOCATEBUFFERTYPEs::FASTHOSTVISIBLE:
			MutexPTR = &VKGPU->FASTHOSTVISIBLE_ALLOC.Locker;
			MemoryBlockList = &VKGPU->FASTHOSTVISIBLE_ALLOC.Allocated_Blocks;
			break;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE:
			MutexPTR = &VKGPU->HOSTVISIBLE_ALLOC.Locker;
			MemoryBlockList = &VKGPU->HOSTVISIBLE_ALLOC.Allocated_Blocks;
			break;
		case GFX_API::SUBALLOCATEBUFFERTYPEs::READBACK:
			MutexPTR = &VKGPU->READBACK_ALLOC.Locker;
			MemoryBlockList = &VKGPU->READBACK_ALLOC.Allocated_Blocks;
			break;
		default:
			LOG_NOTCODED_TAPI("This type of memory block isn't supported for a staging buffer! Delete_StagingBuffer() has failed!", true);
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
		LOG_ERROR_TAPI("Delete_StagingBuffer() didn't delete any memory!");*/
	}

	TAPIResult GPU_ContentManager::Create_VertexBuffer(GFX_API::GFXHandle AttributeLayout, unsigned int VertexCount, 
		unsigned int MemoryTypeIndex, GFX_API::GFXHandle& VertexBufferHandle) {
		if (!VertexCount) {
			LOG_ERROR_TAPI("GFXContentManager->Create_MeshBuffer() has failed because vertex_count is zero!");
			return TAPI_INVALIDARGUMENT;
		}

		VK_VertexAttribLayout* Layout = GFXHandleConverter(VK_VertexAttribLayout*, AttributeLayout);
		if (!Layout) {
			LOG_ERROR_TAPI("GFXContentManager->Create_MeshBuffer() has failed because Attribute Layout ID is invalid!");
			return TAPI_INVALIDARGUMENT;
		}

		unsigned int TOTALDATA_SIZE = Layout->size_perVertex * VertexCount;

		VK_VertexBuffer* VKMesh = new VK_VertexBuffer;
		VKMesh->Block.MemAllocIndex = MemoryTypeIndex;
		VKMesh->Layout = Layout;
		VKMesh->VERTEX_COUNT = VertexCount;

		VkBufferUsageFlags BufferUsageFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		VkBuffer Buffer = Create_VkBuffer(TOTALDATA_SIZE, BufferUsageFlag);
		if (Suballocate_Buffer(Buffer, BufferUsageFlag, VKMesh->Block) != TAPI_SUCCESS) {
			delete VKMesh;
			LOG_ERROR_TAPI("There is no memory left in specified memory region!");
			vkDestroyBuffer(VKGPU->Logical_Device, Buffer, nullptr);
			return TAPI_FAIL;
		}
		vkDestroyBuffer(VKGPU->Logical_Device, Buffer, nullptr);


		MESHBUFFERs.push_back(GFX->JobSys->GetThisThreadIndex(), VKMesh);
		VertexBufferHandle = VKMesh;
		return TAPI_SUCCESS;
	}
	//When you call this function, Draw Calls that uses this ID may draw another Mesh or crash
	//Also if you have any Point Buffer that uses first vertex attribute of that Mesh Buffer, it may crash or draw any other buffer
	void GPU_ContentManager::Unload_VertexBuffer(GFX_API::GFXHandle MeshBuffer_ID) {
		LOG_NOTCODED_TAPI("VK::Unload_MeshBuffer isn't coded!", true);
	}



	TAPIResult GPU_ContentManager::Create_IndexBuffer(GFX_API::DATA_TYPE DataType, unsigned int IndexCount, unsigned int MemoryTypeIndex, GFX_API::GFXHandle& IndexBufferHandle) {
		VkIndexType IndexType = Find_IndexType_byGFXDATATYPE(DataType);
		if (IndexType == VK_INDEX_TYPE_MAX_ENUM) {
			LOG_ERROR_TAPI("GFXContentManager->Create_IndexBuffer() has failed because DataType isn't supported!");
			return TAPI_INVALIDARGUMENT;
		}
		if (!IndexCount) {
			LOG_ERROR_TAPI("GFXContentManager->Create_IndexBuffer() has failed because IndexCount is zero!");
			return TAPI_INVALIDARGUMENT;
		}
		if (MemoryTypeIndex >= VKGPU->ALLOCs.size()) {
			LOG_ERROR_TAPI("GFXContentManager->Create_IndexBuffer() has failed because MemoryTypeIndex is invalid!");
			return TAPI_INVALIDARGUMENT;
		}
		VK_IndexBuffer* IB = new VK_IndexBuffer;
		IB->DATATYPE = IndexType;

		VkBufferUsageFlags BufferUsageFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		VkBuffer Buffer = Create_VkBuffer(GFX_API::Get_UNIFORMTYPEs_SIZEinbytes(DataType) * IndexCount, BufferUsageFlag);
		IB->Block.MemAllocIndex = MemoryTypeIndex;
		IB->IndexCount = static_cast<VkDeviceSize>(IndexCount);
		if (Suballocate_Buffer(Buffer, BufferUsageFlag, IB->Block) != TAPI_SUCCESS) {
			delete IB;
			LOG_ERROR_TAPI("There is no memory left in specified memory region!");
			vkDestroyBuffer(VKGPU->Logical_Device, Buffer, nullptr);
			return TAPI_FAIL;
		}
		vkDestroyBuffer(VKGPU->Logical_Device, Buffer, nullptr);
		
		IndexBufferHandle = IB;
		INDEXBUFFERs.push_back(GFX->JobSys->GetThisThreadIndex(), IB);
		return TAPI_SUCCESS;
	}
	void GPU_ContentManager::Unload_IndexBuffer(GFX_API::GFXHandle BufferHandle) {
		LOG_NOTCODED_TAPI("Create_IndexBuffer() and nothing IndexBuffer related isn't coded yet!", true);
	}


	TAPIResult GPU_ContentManager::Create_Texture(const GFX_API::Texture_Description& TEXTURE_ASSET, unsigned int MemoryTypeIndex, GFX_API::GFXHandle& TextureHandle) {
		LOG_NOTCODED_TAPI("GFXContentManager->Create_Texture() should support mipmaps!", false);
		VK_Texture* TEXTURE = new VK_Texture;
		TEXTURE->CHANNELs = TEXTURE_ASSET.Properties.CHANNEL_TYPE;
		TEXTURE->HEIGHT = TEXTURE_ASSET.HEIGHT;
		TEXTURE->WIDTH = TEXTURE_ASSET.WIDTH;
		TEXTURE->DATA_SIZE = TEXTURE_ASSET.WIDTH * TEXTURE_ASSET.HEIGHT * GFX_API::GetByteSizeOf_TextureChannels(TEXTURE_ASSET.Properties.CHANNEL_TYPE);
		TEXTURE->USAGE = TEXTURE_ASSET.USAGE;
		TEXTURE->Block.MemAllocIndex = MemoryTypeIndex;


		//Create VkImage
		{
			VkImageCreateInfo im_ci = {};
			im_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			im_ci.extent.width = TEXTURE->WIDTH;
			im_ci.extent.height = TEXTURE->HEIGHT;
			im_ci.extent.depth = 1;
			if (TEXTURE_ASSET.Properties.DIMENSION == GFX_API::TEXTURE_DIMENSIONs::TEXTURE_CUBE) {
				im_ci.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
				im_ci.arrayLayers = 6;
			}
			else {
				im_ci.flags = 0;
				im_ci.arrayLayers = 1;
			}
			im_ci.format = Find_VkFormat_byTEXTURECHANNELs(TEXTURE->CHANNELs);
			im_ci.imageType = Find_VkImageType(TEXTURE_ASSET.Properties.DIMENSION);
			im_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			im_ci.mipLevels = 1;
			im_ci.pNext = nullptr;
			if (VKGPU->QUEUEs.size() > 1) {
				im_ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
			}
			else {
				im_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			}
			im_ci.pQueueFamilyIndices = VKGPU->AllQueueFamilies;
			im_ci.queueFamilyIndexCount = VKGPU->QUEUEs.size();
			im_ci.tiling = Find_VkTiling(TEXTURE_ASSET.Properties.DATAORDER);
			im_ci.usage = Find_VKImageUsage_forVKTexture(*TEXTURE);
			im_ci.samples = VK_SAMPLE_COUNT_1_BIT;

			if (vkCreateImage(VKGPU->Logical_Device, &im_ci, nullptr, &TEXTURE->Image) != VK_SUCCESS) {
				LOG_ERROR_TAPI("GFXContentManager->Create_Texture() has failed in vkCreateImage()!");
				delete TEXTURE;
				return TAPI_FAIL;
			}

			if (Suballocate_Image(*TEXTURE) != TAPI_SUCCESS) {
				LOG_ERROR_TAPI("Suballocation of the texture has failed! Please re-create later.");
				delete TEXTURE;
				return TAPI_FAIL;
			}
		}

		//Create Image View
		{
			VkImageViewCreateInfo ci = {};
			ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ci.flags = 0;
			ci.pNext = nullptr;

			ci.image = TEXTURE->Image;
			if (TEXTURE_ASSET.Properties.DIMENSION == GFX_API::TEXTURE_DIMENSIONs::TEXTURE_CUBE) {
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
			if (TEXTURE->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32){
				ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			else if (TEXTURE->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8) {
				ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			else {
				ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}
			
			Fill_ComponentMapping_byCHANNELs(TEXTURE->CHANNELs, ci.components);

			if (vkCreateImageView(VKGPU->Logical_Device, &ci, nullptr, &TEXTURE->ImageView) != VK_SUCCESS) {
				LOG_ERROR_TAPI("GFXContentManager->Upload_Texture() has failed in vkCreateImageView()!");
				return TAPI_FAIL;
			}
		}

		TEXTUREs.push_back(GFX->JobSys->GetThisThreadIndex(), TEXTURE);
		TextureHandle = TEXTURE;
		return TAPI_SUCCESS;
	}
	TAPIResult GPU_ContentManager::Upload_Texture(GFX_API::GFXHandle TextureHandle, const void* DATA, unsigned int DATA_SIZE, unsigned int TARGETOFFSET) {
		LOG_NOTCODED_TAPI("GFXContentManager->Upload_Texture(): Uploading the data isn't coded yet!", true);
		return TAPI_NOTCODED;
	}
	void GPU_ContentManager::Delete_Texture(GFX_API::GFXHandle TextureHandle, bool isUsedLastFrame) {
		if (!TextureHandle) {
			return;
		}
		VK_Texture* Texture = GFXHandleConverter(VK_Texture*, TextureHandle);

		if (isUsedLastFrame) {
			NextFrameDeleteTextureCalls.push_back(GFX->JobSys->GetThisThreadIndex(), Texture);
		}
		else {
			DeleteTextureList.push_back(GFX->JobSys->GetThisThreadIndex(), Texture);
		}
	}


	TAPIResult GPU_ContentManager::Create_GlobalBuffer(const char* BUFFER_NAME, unsigned int DATA_SIZE, unsigned int BINDINDEX, bool isUniform, 
		GFX_API::SHADERSTAGEs_FLAG AccessableStages, unsigned int MemoryTypeIndex, GFX_API::GFXHandle& GlobalBufferHandle) {
		if (VKRENDERER->Is_ConstructingRenderGraph() || VKRENDERER->FrameGraphs->BranchCount) {
			LOG_ERROR_TAPI("GFX API don't support run-time Global Buffer addition for now because Vulkan needs to recreate PipelineLayouts (so all PSOs)! Please create your global buffers before render graph construction.");
			return TAPI_WRONGTIMING;
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
		if (Suballocate_Buffer(obj, flags, GB->Block) != TAPI_SUCCESS) {
			LOG_ERROR_TAPI("Create_GlobalBuffer has failed at suballocation!");
			return TAPI_FAIL;
		}
		vkDestroyBuffer(VKGPU->Logical_Device, obj, nullptr);
		
		GB->ACCESSED_STAGEs = AccessableStages;
		GB->BINDINGPOINT = BINDINDEX;
		GB->DATA_SIZE = DATA_SIZE;
		GB->isUniform = isUniform;

		GlobalBufferHandle = GB;
		GLOBALBUFFERs.push_back(GFX->JobSys->GetThisThreadIndex(), GB);
		return TAPI_SUCCESS;
	}
	void GPU_ContentManager::Unload_GlobalBuffer(GFX_API::GFXHandle BUFFER_ID) {
		LOG_NOTCODED_TAPI("GFXContentManager->Unload_GlobalBuffer() isn't coded!", true);
	}
	TAPIResult GPU_ContentManager::Create_GlobalTexture(const char* TEXTURE_NAME, bool isSampledTexture, unsigned int BINDINDEX, unsigned int TextureCount, GFX_API::SHADERSTAGEs_FLAG AccessableStages,
		GFX_API::GFXHandle& GlobalTextureHandle) {
		//It is possible to add a synchronization and check BINDINDEX, but it is not necessary for now

		if (!TextureCount) {
			LOG_ERROR_TAPI("Create_GlobalTexture() has failed because TextureCount is zero!");
			return TAPI_FAIL;
		}
		VK_GlobalTexture* Descriptor = new VK_GlobalTexture;
		Descriptor->ACCESSED_STAGEs = AccessableStages;
		Descriptor->BINDINGPOINT = BINDINDEX;
		Descriptor->isSampledTexture = isSampledTexture;
		Descriptor->Elements = new VK_GlobalTextureElement[TextureCount];
		Descriptor->ElementCount = TextureCount;
		GLOBALTEXTUREs.push_back(GFX->JobSys->GetThisThreadIndex(), Descriptor);
		GlobalTextureHandle = Descriptor;
		return TAPI_SUCCESS;
	}
	TAPIResult GPU_ContentManager::SetGlobal_SampledTexture(GFX_API::GFXHandle GlobalTextureHandle, unsigned int ElementIndex, GFX_API::GFXHandle TextureHandle, GFX_API::GFXHandle SamplingType,
		GFX_API::IMAGE_ACCESS access) {
		VK_GlobalTexture* GB = GFXHandleConverter(VK_GlobalTexture*, GlobalTextureHandle);
		if (!GB->isSampledTexture) {
			LOG_ERROR_TAPI("SetGlobal_SampledTexture() has failed because global texture is image texture!");
			return TAPI_FAIL;
		}
		if (ElementIndex >= GB->ElementCount) {
			LOG_ERROR_TAPI("SetGlobal_SampledTexture() has failed because ElementIndex isn't smaller than TextureCount of the GlobalTexture!");
			return TAPI_FAIL;
		}
		GB->Elements[ElementIndex].Texture = GFXHandleConverter(VK_Texture*, TextureHandle);
		GB->Elements[ElementIndex].Sampler = GFXHandleConverter(VK_Sampler*, SamplingType)->Sampler;
		VkAccessFlags unused;
		Find_AccessPattern_byIMAGEACCESS(access, unused, GB->Elements[ElementIndex].Layout);
		GlobalShaderInputs_DescSet.ShouldRecreate.store(true);
		return TAPI_SUCCESS;
	}
	TAPIResult GPU_ContentManager::SetGlobal_ImageTexture(GFX_API::GFXHandle GlobalTextureHandle, unsigned int ElementIndex, GFX_API::GFXHandle TextureHandle, GFX_API::GFXHandle SamplingType,
		GFX_API::IMAGE_ACCESS access) {
		VK_GlobalTexture* GB = GFXHandleConverter(VK_GlobalTexture*, GlobalTextureHandle);
		if (GB->isSampledTexture) {
			LOG_ERROR_TAPI("SetGlobal_ImageTexture() has failed because global texture is sampled texture!");
			return TAPI_FAIL;
		}
		if (ElementIndex >= GB->ElementCount) {
			LOG_ERROR_TAPI("SetGlobal_ImageTexture() has failed because ElementIndex isn't smaller than TextureCount of the GlobalTexture!");
			return TAPI_FAIL;
		}
		GB->Elements[ElementIndex].Texture = GFXHandleConverter(VK_Texture*, TextureHandle);
		GB->Elements[ElementIndex].Sampler = GFXHandleConverter(VK_Sampler*, SamplingType)->Sampler;
		VkAccessFlags unused;
		Find_AccessPattern_byIMAGEACCESS(access, unused, GB->Elements[ElementIndex].Layout);
		GlobalShaderInputs_DescSet.ShouldRecreate.store(true);
		return TAPI_SUCCESS;
	}

	TAPIResult GPU_ContentManager::Compile_ShaderSource(const GFX_API::ShaderSource_Resource* SHADER, GFX_API::GFXHandle& ShaderSourceHandle) {
		//Create Vertex Shader Module
		VkShaderModuleCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ci.flags = 0;
		ci.pNext = nullptr;
		ci.pCode = reinterpret_cast<const uint32_t*>(SHADER->SOURCE_DATA);
		ci.codeSize = static_cast<size_t>(SHADER->DATA_SIZE);

		VkShaderModule Module;
		if (vkCreateShaderModule(((GPU*)VKGPU)->Logical_Device, &ci, 0, &Module) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Shader Source is failed at creation!");
			return TAPI_FAIL;
		}
		
		VK_ShaderSource* SHADERSOURCE = new VK_ShaderSource;
		SHADERSOURCE->Module = Module;
		SHADERSOURCEs.push_back(GFX->JobSys->GetThisThreadIndex(), SHADERSOURCE);
		LOG_STATUS_TAPI("Vertex Shader Module is successfully created!");
		ShaderSourceHandle = SHADERSOURCE;
		return TAPI_SUCCESS;
	}
	void GPU_ContentManager::Delete_ShaderSource(GFX_API::GFXHandle ASSET_ID) {
		LOG_NOTCODED_TAPI("VK::Unload_GlobalBuffer isn't coded!", true);
	}
	TAPIResult GPU_ContentManager::Compile_ComputeShader(GFX_API::ComputeShader_Resource* SHADER, GFX_API::GFXHandle* Handles, unsigned int Count) {
		LOG_NOTCODED_TAPI("VK::Compile_ComputeShader isn't coded!", true);
		return TAPI_NOTCODED;
	}
	void GPU_ContentManager::Delete_ComputeShader(GFX_API::GFXHandle ASSET_ID){
		LOG_NOTCODED_TAPI("VK::Delete_ComputeShader isn't coded!", true);
	}
	void Fill_AttachmentBlendState(const GFX_API::ATTACHMENT_BLENDING& info, VkPipelineColorBlendAttachmentState& state) {
		state.alphaBlendOp = Find_BlendOp_byGFXBlendMode(info.BLENDMODE_ALPHA);
		state.blendEnable = VK_TRUE;
		state.colorBlendOp = Find_BlendOp_byGFXBlendMode(info.BLENDMODE_COLOR);
		state.dstAlphaBlendFactor = Find_BlendFactor_byGFXBlendFactor(info.DISTANCEFACTOR_ALPHA);
		state.dstColorBlendFactor = Find_BlendFactor_byGFXBlendFactor(info.DISTANCEFACTOR_COLOR);
		state.srcAlphaBlendFactor = Find_BlendFactor_byGFXBlendFactor(info.SOURCEFACTOR_ALPHA);
		state.srcColorBlendFactor = Find_BlendFactor_byGFXBlendFactor(info.SOURCEFACTOR_COLOR);
	}
	VkColorComponentFlags Find_ColorWriteMask_byChannels(GFX_API::TEXTURE_CHANNELs channels) {
		switch (channels)
		{
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_BGRA8UB:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_BGRA8UNORM:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA32F:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA32UI:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA32I:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8UB:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8B:
			return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGB32F:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGB32UI:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGB32I:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGB8UB:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGB8B:
			return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RA32F:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RA32UI:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RA32I:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RA8UB:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RA8B:
			return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_R32F:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_R32UI:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_R32I:
			return VK_COLOR_COMPONENT_R_BIT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_R8UB:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_R8B:
			return VK_COLOR_COMPONENT_R_BIT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8:
		default:
			LOG_NOTCODED_TAPI("Find_ColorWriteMask_byChannels() doesn't support this type of RTSlot channel!", true);
			return VK_COLOR_COMPONENT_FLAG_BITS_MAX_ENUM;
		}
	}
	TAPIResult GPU_ContentManager::Link_MaterialType(const GFX_API::Material_Type& MATTYPE_ASSET, GFX_API::GFXHandle& MaterialHandle) {
		if (VKRENDERER->Is_ConstructingRenderGraph()) {
			LOG_ERROR_TAPI("You can't link a Material Type while recording RenderGraph!");
			return TAPI_WRONGTIMING;
		}
		VK_VertexAttribLayout* LAYOUT = nullptr;
		LAYOUT = GFXHandleConverter(VK_VertexAttribLayout*, MATTYPE_ASSET.ATTRIBUTELAYOUT_ID);
		if (!LAYOUT) {
			LOG_ERROR_TAPI("Link_MaterialType() has failed because Material Type has invalid Vertex Attribute Layout!");
			return TAPI_INVALIDARGUMENT;
		}
		VK_SubDrawPass* Subpass = GFXHandleConverter(VK_SubDrawPass*, MATTYPE_ASSET.SubDrawPass_ID);
		VK_DrawPass* MainPass = GFXHandleConverter(VK_DrawPass*, Subpass->DrawPass);

		//Subpass attachment should happen here!
		VK_GraphicsPipeline* VKPipeline = new VK_GraphicsPipeline;

		VkPipelineShaderStageCreateInfo Vertex_ShaderStage = {};
		VkPipelineShaderStageCreateInfo Fragment_ShaderStage = {};
		{
			Vertex_ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			Vertex_ShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
			VkShaderModule* VS_Module = &GFXHandleConverter(VK_ShaderSource*, MATTYPE_ASSET.VERTEXSOURCE_ID)->Module;
			Vertex_ShaderStage.module = *VS_Module;
			Vertex_ShaderStage.pName = "main";

			Fragment_ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			Fragment_ShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			VkShaderModule* FS_Module = &GFXHandleConverter(VK_ShaderSource*, MATTYPE_ASSET.FRAGMENTSOURCE_ID)->Module;
			Fragment_ShaderStage.module = *FS_Module;
			Fragment_ShaderStage.pName = "main";
		}
		VkPipelineShaderStageCreateInfo STAGEs[2] = { Vertex_ShaderStage, Fragment_ShaderStage };


		GPU* Vulkan_GPU = (GPU*)VKGPU;

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
			RasterizationState.polygonMode = Find_PolygonMode_byGFXPolygonMode(MATTYPE_ASSET.polygon);
			RasterizationState.cullMode = Find_CullMode_byGFXCullMode(MATTYPE_ASSET.culling);
			RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			RasterizationState.lineWidth = MATTYPE_ASSET.LINEWIDTH;
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

		//Blending is not supported right now, because it should be complicated
		//Blend settings should be per RT slot
		//Per RT Slot means it's better to handle it according to the drawpass
		vector<VkPipelineColorBlendAttachmentState> States(MainPass->SLOTSET->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT);
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
				for (unsigned int BlendingInfoIndex = 0; BlendingInfoIndex < MATTYPE_ASSET.BLENDINGINFOS.size(); BlendingInfoIndex++) {
					const GFX_API::ATTACHMENT_BLENDING& blendinginfo = MATTYPE_ASSET.BLENDINGINFOS[BlendingInfoIndex];
					if (blendinginfo.COLORSLOT_INDEX == RTSlotIndex) {
						Fill_AttachmentBlendState(blendinginfo, States[RTSlotIndex]);
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
			if (MATTYPE_ASSET.BLENDINGINFOS.size()) {
				Pipeline_ColorBlendState.blendConstants[0] = MATTYPE_ASSET.BLENDINGINFOS[0].CONSTANT.x;
				Pipeline_ColorBlendState.blendConstants[1] = MATTYPE_ASSET.BLENDINGINFOS[0].CONSTANT.y;
				Pipeline_ColorBlendState.blendConstants[2] = MATTYPE_ASSET.BLENDINGINFOS[0].CONSTANT.z;
				Pipeline_ColorBlendState.blendConstants[3] = MATTYPE_ASSET.BLENDINGINFOS[0].CONSTANT.w;
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
		vector<VkDynamicState> DynamicStatesList;
		{
			DynamicStatesList.push_back(VK_DYNAMIC_STATE_VIEWPORT);
			DynamicStatesList.push_back(VK_DYNAMIC_STATE_SCISSOR);

			Dynamic_States.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			Dynamic_States.dynamicStateCount = DynamicStatesList.size();
			Dynamic_States.pDynamicStates = DynamicStatesList.data();
		}

		//Material Data related creations
		{
			//General DescriptorSet Layout Creation
			{
				vector<VkDescriptorSetLayoutBinding> bindings;
				for (unsigned int i = 0; i < MATTYPE_ASSET.MATERIALTYPEDATA.size(); i++) {
					const GFX_API::ShaderInput_Description& gfxdesc = MATTYPE_ASSET.MATERIALTYPEDATA[i];
					if (!(gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::IMAGE_G ||
						gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::SAMPLER_G ||
						gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::SBUFFER_G ||
						gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::UBUFFER_G)) {
						continue;
					}

					unsigned int BP = gfxdesc.BINDINGPOINT;
					for (unsigned int bpsearchindex = 0; bpsearchindex < bindings.size(); bpsearchindex++) {
						if (BP == bindings[bpsearchindex].binding) {
							LOG_ERROR_TAPI("Link_MaterialType() has failed because there are colliding binding points!");
							return TAPI_FAIL;
						}
					}

					if (!gfxdesc.ELEMENTCOUNT) {
						LOG_ERROR_TAPI("Link_MaterialType() has failed because one of the shader inputs have 0 element count!");
						return TAPI_FAIL;
					}

					VkDescriptorSetLayoutBinding bn = {};
					bn.stageFlags = Find_VkShaderStages(gfxdesc.SHADERSTAGEs);
					bn.pImmutableSamplers = VK_NULL_HANDLE;
					bn.descriptorType = Find_VkDescType_byMATDATATYPE(gfxdesc.TYPE);
					bn.descriptorCount = gfxdesc.ELEMENTCOUNT;
					bn.binding = BP;
					bindings.push_back(bn);

					VKPipeline->General_DescSet.DescCount++;
					switch (gfxdesc.TYPE) {
					case GFX_API::SHADERINPUT_TYPE::IMAGE_G:
						VKPipeline->General_DescSet.DescImagesCount += gfxdesc.ELEMENTCOUNT;
					break;
					case GFX_API::SHADERINPUT_TYPE::SAMPLER_G:
						VKPipeline->General_DescSet.DescSamplersCount += gfxdesc.ELEMENTCOUNT;
					break;
					case GFX_API::SHADERINPUT_TYPE::UBUFFER_G:
						VKPipeline->General_DescSet.DescUBuffersCount += gfxdesc.ELEMENTCOUNT;
					break;
					case GFX_API::SHADERINPUT_TYPE::SBUFFER_G:
						VKPipeline->General_DescSet.DescSBuffersCount += gfxdesc.ELEMENTCOUNT;
					break;
					}
				}

				if (VKPipeline->General_DescSet.DescCount) {
					VKPipeline->General_DescSet.Descs = new VK_Descriptor[VKPipeline->General_DescSet.DescCount];
					for (unsigned int i = 0; i < MATTYPE_ASSET.MATERIALTYPEDATA.size(); i++) {
						const GFX_API::ShaderInput_Description& gfxdesc = MATTYPE_ASSET.MATERIALTYPEDATA[i];
						if (!(gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::IMAGE_G ||
							gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::SAMPLER_G ||
							gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::SBUFFER_G ||
							gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::UBUFFER_G)) {
							continue;
						}
						if (gfxdesc.BINDINGPOINT >= VKPipeline->General_DescSet.DescCount) {
							LOG_ERROR_TAPI("One of your Material Data Descriptors (General) uses a binding point that is exceeding the number of Material Data Descriptors (General). You have to use a binding point that's lower than size of the Material Data Descriptors (General)!");
							return TAPI_FAIL;
						}

						VK_Descriptor& vkdesc = VKPipeline->General_DescSet.Descs[gfxdesc.BINDINGPOINT];
						switch (gfxdesc.TYPE) {
						case GFX_API::SHADERINPUT_TYPE::IMAGE_G:
						{
							vkdesc.ElementCount = gfxdesc.ELEMENTCOUNT;
							vkdesc.Elements = new VK_DescImageElement[gfxdesc.ELEMENTCOUNT];
							vkdesc.Type = DescType::IMAGE;
						}
						break;
						case GFX_API::SHADERINPUT_TYPE::SAMPLER_G:
						{
							vkdesc.ElementCount = gfxdesc.ELEMENTCOUNT;
							vkdesc.Elements = new VK_DescImageElement[gfxdesc.ELEMENTCOUNT];
							vkdesc.Type = DescType::SAMPLER;
						}
						break;
						case GFX_API::SHADERINPUT_TYPE::UBUFFER_G:
						{
							vkdesc.ElementCount = gfxdesc.ELEMENTCOUNT;
							vkdesc.Elements = new VK_DescBufferElement[gfxdesc.ELEMENTCOUNT];
							vkdesc.Type = DescType::UBUFFER;
						}
						break;
						case GFX_API::SHADERINPUT_TYPE::SBUFFER_G:
						{
							vkdesc.ElementCount = gfxdesc.ELEMENTCOUNT;
							vkdesc.Elements = new VK_DescBufferElement[gfxdesc.ELEMENTCOUNT];
							vkdesc.Type = DescType::SBUFFER;
						}
						break;
						}
					}
					VKPipeline->General_DescSet.ShouldRecreate.store(0);
				}

				if (bindings.size()) {
					VkDescriptorSetLayoutCreateInfo ci = {};
					ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					ci.pNext = nullptr;
					ci.flags = 0;
					ci.bindingCount = bindings.size();
					ci.pBindings = bindings.data();

					if (vkCreateDescriptorSetLayout(VKGPU->Logical_Device, &ci, nullptr, &VKPipeline->General_DescSet.Layout) != VK_SUCCESS) {
						LOG_ERROR_TAPI("Link_MaterialType() has failed at General DescriptorSetLayout Creation vkCreateDescriptorSetLayout()");
						return TAPI_FAIL;
					}
				}
			}

			//Instance DescriptorSet Layout Creation
			{
				vector<VkDescriptorSetLayoutBinding> bindings;
				for (unsigned int i = 0; i < MATTYPE_ASSET.MATERIALTYPEDATA.size(); i++) {
					const GFX_API::ShaderInput_Description& gfxdesc = MATTYPE_ASSET.MATERIALTYPEDATA[i];

					if (!(gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::IMAGE_PI ||
						gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::SAMPLER_PI ||
						gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::SBUFFER_PI ||
						gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::UBUFFER_PI)) {
						continue;
					}
					unsigned int BP = gfxdesc.BINDINGPOINT;
					for (unsigned int bpsearchindex = 0; bpsearchindex < bindings.size(); bpsearchindex++) {
						if (BP == bindings[bpsearchindex].binding) {
							LOG_ERROR_TAPI("Link_MaterialType() has failed because there are colliding binding points!");
							return TAPI_FAIL;
						}
					}
					VkDescriptorSetLayoutBinding bn = {};
					bn.stageFlags = Find_VkShaderStages(gfxdesc.SHADERSTAGEs);
					bn.pImmutableSamplers = VK_NULL_HANDLE;
					bn.descriptorType = Find_VkDescType_byMATDATATYPE(gfxdesc.TYPE);
					bn.descriptorCount = 1;		//I don't support array descriptors for now!
					bn.binding = BP;
					bindings.push_back(bn);

					VKPipeline->Instance_DescSet.DescCount++;
					switch (gfxdesc.TYPE) {
					case GFX_API::SHADERINPUT_TYPE::IMAGE_PI:
					{
						VKPipeline->Instance_DescSet.DescImagesCount += gfxdesc.ELEMENTCOUNT;
					}
					break;
					case GFX_API::SHADERINPUT_TYPE::SAMPLER_PI:
					{
						VKPipeline->Instance_DescSet.DescSamplersCount += gfxdesc.ELEMENTCOUNT;
					}
					break;
					case GFX_API::SHADERINPUT_TYPE::UBUFFER_PI:
					{
						VKPipeline->Instance_DescSet.DescUBuffersCount += gfxdesc.ELEMENTCOUNT;
					}
					break;
					case GFX_API::SHADERINPUT_TYPE::SBUFFER_PI:
					{
						VKPipeline->Instance_DescSet.DescSBuffersCount += gfxdesc.ELEMENTCOUNT;
					}
					break;
					}
				}

				if (VKPipeline->Instance_DescSet.DescCount) {
					VKPipeline->Instance_DescSet.Descs = new VK_Descriptor[VKPipeline->Instance_DescSet.DescCount];
					for (unsigned int i = 0; i < MATTYPE_ASSET.MATERIALTYPEDATA.size(); i++) {
						const GFX_API::ShaderInput_Description& gfxdesc = MATTYPE_ASSET.MATERIALTYPEDATA[i];
						if (!(gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::IMAGE_PI ||
							gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::SAMPLER_PI ||
							gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::SBUFFER_PI ||
							gfxdesc.TYPE == GFX_API::SHADERINPUT_TYPE::UBUFFER_PI)) {
							continue;
						}

						if (gfxdesc.BINDINGPOINT >= VKPipeline->Instance_DescSet.DescCount) {
							LOG_ERROR_TAPI("One of your Material Data Descriptors (Per Instance) uses a binding point that is exceeding the number of Material Data Descriptors (Per Instance). You have to use a binding point that's lower than size of the Material Data Descriptors (Per Instance)!");
							return TAPI_FAIL;
						}

						//We don't need to create each descriptor's array elements
						VK_Descriptor& vkdesc = VKPipeline->Instance_DescSet.Descs[gfxdesc.BINDINGPOINT];
						vkdesc.ElementCount = gfxdesc.ELEMENTCOUNT;
						switch (gfxdesc.TYPE) {
						case GFX_API::SHADERINPUT_TYPE::IMAGE_PI:
							vkdesc.Type = DescType::IMAGE;
						break;
						case GFX_API::SHADERINPUT_TYPE::SAMPLER_PI:
							vkdesc.Type = DescType::SAMPLER;
						break;
						case GFX_API::SHADERINPUT_TYPE::UBUFFER_PI:
							vkdesc.Type = DescType::UBUFFER;
						break;
						case GFX_API::SHADERINPUT_TYPE::SBUFFER_PI:
							vkdesc.Type = DescType::SBUFFER;
						break;
						}
					}
					VKPipeline->Instance_DescSet.ShouldRecreate.store(0);
				}

				if (bindings.size()) {
					VkDescriptorSetLayoutCreateInfo ci = {};
					ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					ci.pNext = nullptr;
					ci.flags = 0;
					ci.bindingCount = bindings.size();
					ci.pBindings = bindings.data();

					if (vkCreateDescriptorSetLayout(VKGPU->Logical_Device, &ci, nullptr, &VKPipeline->Instance_DescSet.Layout) != VK_SUCCESS) {
						LOG_ERROR_TAPI("Link_MaterialType() has failed at Instance DesciptorSetLayout Creation vkCreateDescriptorSetLayout()");
						return TAPI_FAIL;
					}
				}
			}
			
			//General DescriptorSet Creation
			if (!Create_DescSet(&VKPipeline->General_DescSet)) {
				LOG_ERROR_TAPI("Descriptor pool is full, that means you should expand its size!");
				return TAPI_FAIL;
			}

			//Pipeline Layout Creation
			{
				VkPipelineLayoutCreateInfo pl_ci = {};
				pl_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pl_ci.pNext = nullptr;
				pl_ci.flags = 0;
				
				VkDescriptorSetLayout descsetlays[3] = { GlobalShaderInputs_DescSet.Layout, VK_NULL_HANDLE, VK_NULL_HANDLE };
				pl_ci.setLayoutCount = 1;
				if (VKPipeline->General_DescSet.Layout != VK_NULL_HANDLE) {
					pl_ci.setLayoutCount++;
					descsetlays[1] = VKPipeline->General_DescSet.Layout;
				}
				if (VKPipeline->Instance_DescSet.Layout != VK_NULL_HANDLE) {
					pl_ci.setLayoutCount++;
					descsetlays[pl_ci.setLayoutCount - 1] = VKPipeline->Instance_DescSet.Layout;
				}
				pl_ci.pSetLayouts = descsetlays;
				//Don't support for now!
				pl_ci.pushConstantRangeCount = 0;
				pl_ci.pPushConstantRanges = nullptr;

				if (vkCreatePipelineLayout(Vulkan_GPU->Logical_Device, &pl_ci, nullptr, &VKPipeline->PipelineLayout) != VK_SUCCESS) {
					LOG_ERROR_TAPI("Link_MaterialType() failed at vkCreatePipelineLayout()!");
					return TAPI_FAIL;
				}
			}
		}

		VkPipelineDepthStencilStateCreateInfo depth_state = {};
		if (Subpass->SLOTSET->BASESLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
			depth_state.depthBoundsTestEnable = VK_FALSE;
			depth_state.depthCompareOp = Find_CompareOp_byGFXDepthTest(MATTYPE_ASSET.depthtest);
			Find_DepthMode_byGFXDepthMode(MATTYPE_ASSET.depthmode, depth_state.depthTestEnable, depth_state.depthWriteEnable);
			depth_state.flags = 0;
			depth_state.pNext = nullptr;

			if (MATTYPE_ASSET.backfacedstencil.CompareOperation != GFX_API::STENCIL_COMPARE::OFF ||
				MATTYPE_ASSET.frontfacedstencil.CompareOperation != GFX_API::STENCIL_COMPARE::OFF) {
				depth_state.stencilTestEnable = VK_TRUE;

				depth_state.back.compareMask = MATTYPE_ASSET.backfacedstencil.STENCILCOMPAREMASK;
				depth_state.back.compareOp = Find_CompareOp_byGFXStencilCompare(MATTYPE_ASSET.backfacedstencil.CompareOperation);
				depth_state.back.depthFailOp = Find_StencilOp_byGFXStencilOp(MATTYPE_ASSET.backfacedstencil.DepthFailed);
				depth_state.back.failOp = Find_StencilOp_byGFXStencilOp(MATTYPE_ASSET.backfacedstencil.StencilFailed);
				depth_state.back.passOp = Find_StencilOp_byGFXStencilOp(MATTYPE_ASSET.backfacedstencil.DepthSuccess);
				depth_state.back.reference = MATTYPE_ASSET.backfacedstencil.STENCILVALUE;
				depth_state.back.writeMask = MATTYPE_ASSET.backfacedstencil.STENCILWRITEMASK;

				depth_state.front.compareMask = MATTYPE_ASSET.frontfacedstencil.STENCILCOMPAREMASK;
				depth_state.front.compareOp = Find_CompareOp_byGFXStencilCompare(MATTYPE_ASSET.frontfacedstencil.CompareOperation);
				depth_state.front.depthFailOp = Find_StencilOp_byGFXStencilOp(MATTYPE_ASSET.frontfacedstencil.DepthFailed);
				depth_state.front.failOp = Find_StencilOp_byGFXStencilOp(MATTYPE_ASSET.frontfacedstencil.StencilFailed);
				depth_state.front.passOp = Find_StencilOp_byGFXStencilOp(MATTYPE_ASSET.frontfacedstencil.DepthSuccess);
				depth_state.front.reference = MATTYPE_ASSET.frontfacedstencil.STENCILVALUE;
				depth_state.front.writeMask = MATTYPE_ASSET.frontfacedstencil.STENCILWRITEMASK;

			}
			else {
				depth_state.stencilTestEnable = VK_FALSE;
			}
			depth_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
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
			if (vkCreateGraphicsPipelines(Vulkan_GPU->Logical_Device, nullptr, 1, &GraphicsPipelineCreateInfo, nullptr, &VKPipeline->PipelineObject) != VK_SUCCESS) {
				delete VKPipeline;
				delete STAGEs;
				LOG_ERROR_TAPI("vkCreateGraphicsPipelines has failed!");
				return TAPI_FAIL;
			}
		}

		VKPipeline->GFX_Subpass = Subpass;
		SHADERPROGRAMs.push_back(GFX->JobSys->GetThisThreadIndex(), VKPipeline);


		LOG_STATUS_TAPI("Finished creating Graphics Pipeline");
		MaterialHandle = VKPipeline;
		return TAPI_SUCCESS;
	}
	void GPU_ContentManager::Delete_MaterialType(GFX_API::GFXHandle Asset_ID){
		LOG_NOTCODED_TAPI("VK::Unload_GlobalBuffer isn't coded!", true);
	}
	TAPIResult GPU_ContentManager::Create_MaterialInst(GFX_API::GFXHandle MaterialType, GFX_API::GFXHandle& MaterialInstHandle) {
		VK_GraphicsPipeline* VKPSO = GFXHandleConverter(VK_GraphicsPipeline*, MaterialType);
		if (!VKPSO) {
			LOG_ERROR_TAPI("Create_MaterialInst() has failed because Material Type isn't found!");
			return TAPI_INVALIDARGUMENT;
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

			Create_DescSet(&VKPInstance->DescSet);
		}


		VKPInstance->PROGRAM = VKPSO;
		SHADERPINSTANCEs.push_back(GFX->JobSys->GetThisThreadIndex(), VKPInstance);
		MaterialInstHandle = VKPInstance;
		return TAPI_SUCCESS;
	}
	void GPU_ContentManager::Delete_MaterialInst(GFX_API::GFXHandle Asset_ID) {
		LOG_CRASHING_TAPI("Delete_MaterialInst() isn't coded yet!");
	}
	void FindBufferOBJ_byBufType(const GFX_API::GFXHandle Handle, GFX_API::BUFFER_TYPE TYPE, VkBuffer& TargetBuffer, VkDeviceSize& TargetOffset);
	TAPIResult GPU_ContentManager::SetMaterial_UniformBuffer(GFX_API::GFXHandle MaterialType_orInstance, bool isMaterialType, bool isUsedRecently, unsigned int BINDINDEX,
		GFX_API::GFXHandle TargetBufferHandle, unsigned int ELEMENTINDEX, GFX_API::BUFFER_TYPE BufferType, unsigned int TargetOffset, unsigned int BoundDataSize) {
		
		VK_DescSet* Set = nullptr;
		if (isMaterialType) {
			VK_GraphicsPipeline* PSO = GFXHandleConverter(VK_GraphicsPipeline*, MaterialType_orInstance);
			Set = &PSO->General_DescSet;
		}
		else {
			VK_PipelineInstance* PSO = GFXHandleConverter(VK_PipelineInstance*, MaterialType_orInstance);
			Set = &PSO->DescSet;
		}

		if (!Set->DescCount) {
			LOG_ERROR_TAPI("Given Material Type/Instance doesn't have any shader input to set!");
			return TAPI_FAIL;
		}
		if (BINDINDEX >= Set->DescCount) {
			LOG_ERROR_TAPI("BINDINDEX is exceeding the shader input count in the Material Type/Instance");
			return TAPI_FAIL;
		}
		VK_Descriptor& Descriptor = Set->Descs[BINDINDEX];
		if (Descriptor.Type != DescType::UBUFFER) {
			LOG_ERROR_TAPI("BINDINDEX is pointing to some other type of shader input (isn't pointing to an uniform buffer!)");
			return TAPI_FAIL;
		}
		if (ELEMENTINDEX >= Descriptor.ElementCount) {
			LOG_ERROR_TAPI("You gave SetMaterial_UniformBuffer() an overflowing ELEMENTINDEX!");
			return TAPI_FAIL;
		}
		VK_DescBufferElement& DescElement =	GFXHandleConverter(VK_DescBufferElement*, Descriptor.Elements)[ELEMENTINDEX];


		if (TargetOffset % VKGPU->Device_Properties.limits.minUniformBufferOffsetAlignment) {
			LOG_WARNING_TAPI("This TargetOffset in SetMaterial_UniformBuffer triggers Vulkan Validation Layer, this usage may cause undefined behaviour on this GPU! You should set TargetOffset as a multiple of " + to_string(VKGPU->Device_Properties.limits.minUniformBufferOffsetAlignment));
		}
		bool x = 0, y = 1;
		if (!DescElement.IsUpdated.compare_exchange_strong(x, y)) {
			LOG_ERROR_TAPI("You already set the uniform buffer, you can't change it at the same frame!");
			return TAPI_WRONGTIMING;
		}
		VkDeviceSize FinalOffset = static_cast<VkDeviceSize>(TargetOffset);
		switch (BufferType) {
		case GFX_API::BUFFER_TYPE::STAGING:
		{
			MemoryBlock* StagingBuf = GFXHandleConverter(MemoryBlock*, TargetBufferHandle);
			FindBufferOBJ_byBufType(TargetBufferHandle, BufferType, DescElement.Info.buffer, FinalOffset);
		}
		break;
		default:
			LOG_NOTCODED_TAPI("SetMaterial_UniformBuffer() doesn't support this type of target buffers right now!", true);
			return TAPI_NOTCODED;
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
			DescSets_toCreateUpdate.push_back(GFX->JobSys->GetThisThreadIndex(), call);
		}
		else {
			DescSets_toJustUpdate.push_back(GFX->JobSys->GetThisThreadIndex(), call);
		}
		return TAPI_SUCCESS;
	}
	TAPIResult GPU_ContentManager::SetMaterial_SampledTexture(GFX_API::GFXHandle MaterialType_orInstance, bool isMaterialType, bool isUsedRecently, unsigned int BINDINDEX,
		unsigned int ELEMENTINDEX, GFX_API::GFXHandle TextureHandle, GFX_API::GFXHandle SamplingType, GFX_API::IMAGE_ACCESS usage) {
		VK_DescSet* Set = nullptr;
		if (isMaterialType) {
			VK_GraphicsPipeline* PSO = GFXHandleConverter(VK_GraphicsPipeline*, MaterialType_orInstance);
			Set = &PSO->General_DescSet;
		}
		else {
			VK_PipelineInstance* PSO = GFXHandleConverter(VK_PipelineInstance*, MaterialType_orInstance);
			Set = &PSO->DescSet;
		}

		if (!Set->DescCount) {
			LOG_ERROR_TAPI("Given Material Type/Instance doesn't have any shader input to set!");
			return TAPI_FAIL;
		}
		if (BINDINDEX >= Set->DescCount) {
			LOG_ERROR_TAPI("BINDINDEX is exceeding the shader input count in the Material Type/Instance");
			return TAPI_FAIL;
		}
		VK_Descriptor& Descriptor = Set->Descs[BINDINDEX];
		if (Descriptor.Type != DescType::SAMPLER) {
			LOG_ERROR_TAPI("BINDINDEX is pointing to some other type of shader input (isn't pointing to an sampler texture!)");
			return TAPI_FAIL;
		}
		if (ELEMENTINDEX >= Descriptor.ElementCount) {
			LOG_ERROR_TAPI("You gave SetMaterial_ImageTexture() an overflowing ELEMENTINDEX!");
			return TAPI_FAIL;
		}
		VK_DescImageElement& DescElement = GFXHandleConverter(VK_DescImageElement*, Descriptor.Elements)[ELEMENTINDEX];


		VK_Sampler* Sampler = GFXHandleConverter(VK_Sampler*, SamplingType);
		bool x = false, y = true;
		if (!DescElement.IsUpdated.compare_exchange_strong(x, y)) {
			return TAPI_WRONGTIMING;
		}
		VkAccessFlags unused;
		Find_AccessPattern_byIMAGEACCESS(usage, unused, DescElement.info.imageLayout);
		VK_Texture* TEXTURE = GFXHandleConverter(VK_Texture*, TextureHandle);
		DescElement.info.imageView = TEXTURE->ImageView;
		DescElement.info.sampler = Sampler->Sampler;

		VK_DescSetUpdateCall call;
		call.Set = Set;
		call.BindingIndex = BINDINDEX;
		call.ElementIndex = ELEMENTINDEX;
		if (isUsedRecently) {
			call.Set->ShouldRecreate.store(1);
			DescSets_toCreateUpdate.push_back(GFX->JobSys->GetThisThreadIndex(), call);
		}
		else {
			DescSets_toJustUpdate.push_back(GFX->JobSys->GetThisThreadIndex(), call);
		}
		return TAPI_SUCCESS;
	}
	TAPIResult GPU_ContentManager::SetMaterial_ImageTexture(GFX_API::GFXHandle MaterialType_orInstance, bool isMaterialType, bool isUsedRecently, unsigned int BINDINDEX,
		unsigned int ELEMENTINDEX, GFX_API::GFXHandle TextureHandle, GFX_API::GFXHandle SamplingType, GFX_API::IMAGE_ACCESS usage) {
		VK_DescSet* Set = nullptr;
		if (isMaterialType) {
			VK_GraphicsPipeline* PSO = GFXHandleConverter(VK_GraphicsPipeline*, MaterialType_orInstance);
			Set = &PSO->General_DescSet;
		}
		else {
			VK_PipelineInstance* PSO = GFXHandleConverter(VK_PipelineInstance*, MaterialType_orInstance);
			Set = &PSO->DescSet;
		}

		if (!Set->DescCount) {
			LOG_ERROR_TAPI("Given Material Type/Instance doesn't have any shader input to set!");
			return TAPI_FAIL;
		}
		if (BINDINDEX >= Set->DescCount) {
			LOG_ERROR_TAPI("BINDINDEX is exceeding the shader input count in the Material Type/Instance");
			return TAPI_FAIL;
		}
		VK_Descriptor& Descriptor = Set->Descs[BINDINDEX];
		if (Descriptor.Type != DescType::IMAGE) {
			LOG_ERROR_TAPI("BINDINDEX is pointing to some other type of shader input (isn't pointing to an image texture!)");
			return TAPI_FAIL;
		}
		if (ELEMENTINDEX >= Descriptor.ElementCount) {
			LOG_ERROR_TAPI("You gave SetMaterial_ImageTexture() an overflowing ELEMENTINDEX!");
			return TAPI_FAIL;
		}
		VK_DescImageElement& DescElement = GFXHandleConverter(VK_DescImageElement*, Descriptor.Elements)[ELEMENTINDEX];



		VK_Sampler* Sampler = GFXHandleConverter(VK_Sampler*, SamplingType);
		bool x = false, y = true;
		if (!DescElement.IsUpdated.compare_exchange_strong(x, y)) {
			return TAPI_WRONGTIMING;
		}
		VkAccessFlags unused;
		Find_AccessPattern_byIMAGEACCESS(usage, unused, DescElement.info.imageLayout);
		VK_Texture* TEXTURE = GFXHandleConverter(VK_Texture*, TextureHandle);
		DescElement.info.imageView = TEXTURE->ImageView;
		DescElement.info.sampler = Sampler->Sampler;

		VK_DescSetUpdateCall call;
		call.Set = Set;
		call.BindingIndex = BINDINDEX;
		call.ElementIndex = ELEMENTINDEX;
		if (isUsedRecently) {
			call.Set->ShouldRecreate.store(1);
			DescSets_toCreateUpdate.push_back(GFX->JobSys->GetThisThreadIndex(), call);
		}
		else {
			DescSets_toJustUpdate.push_back(GFX->JobSys->GetThisThreadIndex(), call);
		}
		return TAPI_SUCCESS;
	}

	TAPIResult GPU_ContentManager::Create_RTSlotset(const vector<GFX_API::RTSLOT_Description>& Descriptions, GFX_API::GFXHandle& RTSlotSetHandle) {
		for (unsigned int SlotIndex = 0; SlotIndex < Descriptions.size(); SlotIndex++) {
			const GFX_API::RTSLOT_Description& desc = Descriptions[SlotIndex];
			VK_Texture* FirstHandle = GFXHandleConverter(VK_Texture*, desc.TextureHandles[0]);
			VK_Texture* SecondHandle = GFXHandleConverter(VK_Texture*, desc.TextureHandles[1]);
			if ((FirstHandle->CHANNELs != SecondHandle->CHANNELs) ||
				(FirstHandle->WIDTH != SecondHandle->WIDTH) ||
				(FirstHandle->HEIGHT != SecondHandle->HEIGHT)
				) {
				LOG_ERROR_TAPI("GFXContentManager->Create_RTSlotSet() has failed because one of the slots has texture handles that doesn't match channel type, width or height!");
				return TAPI_INVALIDARGUMENT;
			}
			if (!FirstHandle->USAGE.isRenderableTo || !SecondHandle->USAGE.isRenderableTo) {
				LOG_ERROR_TAPI("GFXContentManager->Create_RTSlotSet() has failed because one of the slots has a handle that doesn't use is_RenderableTo in its USAGEFLAG!");
				return TAPI_INVALIDARGUMENT;
			}
		}
		unsigned int DEPTHSLOT_VECTORINDEX = UINT32_MAX;
		//Validate the list and find Depth Slot if there is any
		for (unsigned int SlotIndex = 0; SlotIndex < Descriptions.size(); SlotIndex++) {
			const GFX_API::RTSLOT_Description& desc = Descriptions[SlotIndex];
			for (unsigned int RTIndex = 0; RTIndex < 2; RTIndex++) {
				VK_Texture* RT = GFXHandleConverter(VK_Texture*, desc.TextureHandles[RTIndex]);
				if (!RT) {
					LOG_ERROR_TAPI("Create_RTSlotSet() has failed because intended RT isn't found!");
					return TAPI_INVALIDARGUMENT;
				}
				if (desc.OPTYPE == GFX_API::OPERATION_TYPE::UNUSED) {
					LOG_ERROR_TAPI("Create_RTSlotSet() has failed because you can't create a Base RT SlotSet that has unused attachment!");
					return TAPI_INVALIDARGUMENT;
				}
				if (RT->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8 || RT->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32) {
					if (DEPTHSLOT_VECTORINDEX != UINT32_MAX && DEPTHSLOT_VECTORINDEX != SlotIndex) {
						LOG_ERROR_TAPI("Create_RTSlotSet() has failed because you can't use two depth buffers at the same slot set!");
						return TAPI_INVALIDARGUMENT;
					}
					DEPTHSLOT_VECTORINDEX = SlotIndex;
					continue;
				}
				if (desc.SLOTINDEX > Descriptions.size() - 1) {
					LOG_ERROR_TAPI("Create_RTSlotSet() has failed because you gave a overflowing SLOTINDEX to a RTSLOT!");
					return TAPI_INVALIDARGUMENT;
				}
			}
		}
		unsigned char COLORRT_COUNT = (DEPTHSLOT_VECTORINDEX != UINT32_MAX) ? Descriptions.size() - 1 : Descriptions.size();

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
				if (Descriptions[i].SLOTINDEX == Descriptions[i + j].SLOTINDEX) {
					LOG_ERROR_TAPI("Create_RTSlotSet() has failed because some SLOTINDEXes are same, but each SLOTINDEX should be unique and lower then COLOR SLOT COUNT!");
					return TAPI_INVALIDARGUMENT;
				}
			}
		}

		unsigned int FBWIDTH = GFXHandleConverter(VK_Texture*, Descriptions[0].TextureHandles[0])->WIDTH;
		unsigned int FBHEIGHT = GFXHandleConverter(VK_Texture*, Descriptions[0].TextureHandles[0])->HEIGHT;
		for (unsigned int SlotIndex = 0; SlotIndex < Descriptions.size(); SlotIndex++) {
			VK_Texture* Texture = GFXHandleConverter(VK_Texture*, Descriptions[SlotIndex].TextureHandles[0]);
			if (Texture->WIDTH != FBWIDTH || Texture->HEIGHT != FBHEIGHT) {
				LOG_ERROR_TAPI("Create_RTSlotSet() has failed because one of your slot's texture has wrong resolution!");
				return TAPI_INVALIDARGUMENT;
			}
		}


		VK_RTSLOTSET* VKSLOTSET = new VK_RTSLOTSET;
		for (unsigned int SlotSetIndex = 0; SlotSetIndex < 2; SlotSetIndex++) {
			VK_RTSLOTs& PF_SLOTSET = VKSLOTSET->PERFRAME_SLOTSETs[SlotSetIndex];

			PF_SLOTSET.COLOR_SLOTs = new VK_COLORRTSLOT[COLORRT_COUNT];
			PF_SLOTSET.COLORSLOTs_COUNT = COLORRT_COUNT;
			if (DEPTHSLOT_VECTORINDEX != Descriptions.size()) {
				PF_SLOTSET.DEPTHSTENCIL_SLOT = new VK_DEPTHSTENCILSLOT;
				VK_DEPTHSTENCILSLOT* slot = PF_SLOTSET.DEPTHSTENCIL_SLOT;
				const GFX_API::RTSLOT_Description& DEPTHDESC = Descriptions[DEPTHSLOT_VECTORINDEX];
				slot->CLEAR_COLOR = vec2(DEPTHDESC.CLEAR_VALUE.x, DEPTHDESC.CLEAR_VALUE.y);
				slot->DEPTH_OPTYPE = DEPTHDESC.OPTYPE;
				slot->RT = GFXHandleConverter(VK_Texture*, DEPTHDESC.TextureHandles[SlotSetIndex]);
				slot->STENCIL_OPTYPE = DEPTHDESC.OPTYPESTENCIL;
				slot->IS_USED_LATER = DEPTHDESC.isUSEDLATER;
				slot->DEPTH_LOAD = DEPTHDESC.LOADOP;
				slot->STENCIL_LOAD = DEPTHDESC.LOADOPSTENCIL;
			}
			for (unsigned int i = 0; i < COLORRT_COUNT; i++) {
				const GFX_API::RTSLOT_Description& desc = Descriptions[i];
				VK_Texture* RT = GFXHandleConverter(VK_Texture*, desc.TextureHandles[SlotSetIndex]);
				VK_COLORRTSLOT& SLOT = PF_SLOTSET.COLOR_SLOTs[desc.SLOTINDEX];
				SLOT.RT_OPERATIONTYPE = desc.OPTYPE;
				SLOT.LOADSTATE = desc.LOADOP;
				SLOT.RT = RT;
				SLOT.IS_USED_LATER = desc.isUSEDLATER;
				SLOT.CLEAR_COLOR = desc.CLEAR_VALUE;
			}


			for (unsigned int i = 0; i < PF_SLOTSET.COLORSLOTs_COUNT; i++) {
				VK_Texture* VKTexture = PF_SLOTSET.COLOR_SLOTs[i].RT;
				if (!VKTexture->ImageView) {
					LOG_CRASHING_TAPI("One of your RTs doesn't have a VkImageView! You can't use such a texture as RT. Generally this case happens when you forgot to specify your swapchain texture's usage (while creating a window).");
					return TAPI_FAIL;
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

		RT_SLOTSETs.push_back(GFX->JobSys->GetThisThreadIndex(), VKSLOTSET);
		RTSlotSetHandle = VKSLOTSET;
		return TAPI_SUCCESS;
	}
	TAPIResult GPU_ContentManager::Change_RTSlotTexture(GFX_API::GFXHandle RTSlotHandle, bool isColorRT, unsigned char SlotIndex, unsigned char FrameIndex, GFX_API::GFXHandle TextureHandle) {
		VK_RTSLOTSET* SlotSet = GFXHandleConverter(VK_RTSLOTSET*, RTSlotHandle);
		VK_Texture* Texture = GFXHandleConverter(VK_Texture*, TextureHandle);
		if (!Texture->USAGE.isRenderableTo) {
			LOG_ERROR_TAPI("You can't change RTSlot's texture because given texture's USAGE flag doesn't activate isRenderableTo!");
			return TAPI_FAIL;
		}

		VkFramebufferCreateInfo& FB_ci = SlotSet->FB_ci[FrameIndex];
		if (Texture->WIDTH != FB_ci.width || Texture->HEIGHT != FB_ci.height) {
			LOG_ERROR_TAPI("You can't change RTSlot's texture because given texture has unmatching resolution!");
			return TAPI_FAIL;
		}

		if (isColorRT) {
			if (SlotSet->PERFRAME_SLOTSETs[FrameIndex].COLORSLOTs_COUNT <= SlotIndex) {
				LOG_ERROR_TAPI("You can't change RTSlot's texture because given SlotIndex is exceeding the number of color slots!");
				return TAPI_FAIL;
			}
			VK_COLORRTSLOT& Slot = SlotSet->PERFRAME_SLOTSETs[FrameIndex].COLOR_SLOTs[SlotIndex];
			if (Slot.RT->CHANNELs != Texture->CHANNELs) {
				LOG_ERROR_TAPI("You can't change RTSlot's texture because given Texture's channel type isn't same with target slot's!");
				return TAPI_FAIL;
			}

			//Race against concurrency (*plays Tokyo Drift)
			bool x = false, y = true;
			if (!Slot.IsChanged.compare_exchange_strong(x, y)) {
				LOG_ERROR_TAPI("You already changed this slot!");
				return TAPI_WRONGTIMING;
			}
			SlotSet->PERFRAME_SLOTSETs[FrameIndex].IsChanged.store(true);

			Slot.RT = Texture;
			SlotSet->ImageViews[FrameIndex][SlotIndex] = Texture->ImageView;
		}
		else {
			VK_DEPTHSTENCILSLOT* Slot = SlotSet->PERFRAME_SLOTSETs[FrameIndex].DEPTHSTENCIL_SLOT;
			if (!Slot) {
				LOG_ERROR_TAPI("You can't change RTSlot's texture because SlotSet doesn't have any Depth-Stencil Slot!");
				return TAPI_FAIL;
			}

			if (Slot->RT->CHANNELs != Texture->CHANNELs) {
				LOG_ERROR_TAPI("You can't change RTSlot's texture because given Texture's channel type isn't same with target slot's!");
				return TAPI_FAIL;
			}

			//Race against concurrency
			bool x = false, y = true;
			if (!Slot->IsChanged.compare_exchange_strong(x, y)) {
				LOG_ERROR_TAPI("You already changed this slot!");
				return TAPI_WRONGTIMING;
			}
			SlotSet->PERFRAME_SLOTSETs[FrameIndex].IsChanged.store(true);

			Slot->RT = Texture;
			SlotSet->ImageViews[FrameIndex][SlotSet->PERFRAME_SLOTSETs[FrameIndex].COLORSLOTs_COUNT] = Texture->ImageView;
		}
		return TAPI_SUCCESS;
	}
	TAPIResult GPU_ContentManager::Inherite_RTSlotSet(const vector<GFX_API::RTSLOTUSAGE_Description>& Descriptions, GFX_API::GFXHandle RTSlotSetHandle, GFX_API::GFXHandle& InheritedSlotSetHandle) {
		if (!RTSlotSetHandle) {
			LOG_ERROR_TAPI("Inherite_RTSlotSet() has failed because Handle is invalid!");
			return TAPI_INVALIDARGUMENT;
		}
		VK_RTSLOTSET* BaseSet = GFXHandleConverter(VK_RTSLOTSET*, RTSlotSetHandle);
		VK_IRTSLOTSET* InheritedSet = new VK_IRTSLOTSET;
		InheritedSet->BASESLOTSET = BaseSet;

		//Find Depth/Stencil Slots and count Color Slots
		bool DEPTH_FOUND = false;
		unsigned char COLORSLOT_COUNT = 0, DEPTHDESC_VECINDEX = 0;
		for (unsigned char i = 0; i < Descriptions.size(); i++) {
			const GFX_API::RTSLOTUSAGE_Description& DESC = Descriptions[i];
			if (DESC.IS_DEPTH) {
				if (DEPTH_FOUND) {
					LOG_ERROR_TAPI("Inherite_RTSlotSet() has failed because there are two depth buffers in the description, which is not supported!");
					delete InheritedSet;
					return TAPI_INVALIDARGUMENT;
				}
				DEPTH_FOUND = true;
				DEPTHDESC_VECINDEX = i;
				if (BaseSet->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::READ_ONLY &&
					(DESC.OPTYPE == GFX_API::OPERATION_TYPE::WRITE_ONLY || DESC.OPTYPE == GFX_API::OPERATION_TYPE::READ_AND_WRITE)
					) 
				{
					LOG_ERROR_TAPI("Inherite_RTSlotSet() has failed because you can't use a Read-Only DepthSlot with Write Access in a Inherited Set!");
					delete InheritedSet;
					return TAPI_INVALIDARGUMENT;
				}
				InheritedSet->DEPTH_OPTYPE = DESC.OPTYPE;
				InheritedSet->STENCIL_OPTYPE = DESC.OPTYPESTENCIL;
			}
			else {
				COLORSLOT_COUNT++;
			}
		}
		if (!DEPTH_FOUND) {
			InheritedSet->DEPTH_OPTYPE = GFX_API::OPERATION_TYPE::UNUSED;
		}
		if (COLORSLOT_COUNT != BaseSet->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT) {
			LOG_ERROR_TAPI("Inherite_RTSlotSet() has failed because BaseSet's Color Slot count doesn't match given Descriptions's one!");
			delete InheritedSet;
			return TAPI_INVALIDARGUMENT;
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
				if (Descriptions[i].SLOTINDEX >= COLORSLOT_COUNT) {
					LOG_ERROR_TAPI("Inherite_RTSlotSet() has failed because some ColorSlots have indexes that are more than or equal to ColorSlotCount! Each slot's index should be unique and lower than ColorSlotCount!");
					delete InheritedSet;
					return TAPI_INVALIDARGUMENT;
				}
				if (Descriptions[i].SLOTINDEX == Descriptions[i + j].SLOTINDEX) {
					LOG_ERROR_TAPI("Inherite_RTSlotSet() has failed because given Descriptions have some ColorSlots that has same indexes! Each slot's index should be unique and lower than ColorSlotCount!");
					delete InheritedSet;
					return TAPI_INVALIDARGUMENT;
				}
			}
		}

		InheritedSet->COLOR_OPTYPEs = new GFX_API::OPERATION_TYPE[COLORSLOT_COUNT];
		//Set OPTYPEs of inherited slot set
		for (unsigned int i = 0; i < COLORSLOT_COUNT; i++) {
			if (i == DEPTHDESC_VECINDEX) {
				continue;
			}
			const GFX_API::OPERATION_TYPE& BSLOT_OPTYPE = BaseSet->PERFRAME_SLOTSETs[0].COLOR_SLOTs[Descriptions[i].SLOTINDEX].RT_OPERATIONTYPE;

			if (BSLOT_OPTYPE == GFX_API::OPERATION_TYPE::READ_ONLY &&
					(Descriptions[i].OPTYPE == GFX_API::OPERATION_TYPE::WRITE_ONLY || Descriptions[i].OPTYPE == GFX_API::OPERATION_TYPE::READ_AND_WRITE)
				) 
			{
				LOG_ERROR_TAPI("Inherite_RTSlotSet() has failed because you can't use a Read-Only ColorSlot with Write Access in a Inherited Set!");
				delete InheritedSet;
				return TAPI_INVALIDARGUMENT;
			}
			InheritedSet->COLOR_OPTYPEs[Descriptions[i].SLOTINDEX] = Descriptions[i].OPTYPE;
		}

		InheritedSlotSetHandle = InheritedSet;
		IRT_SLOTSETs.push_back(GFX->JobSys->GetThisThreadIndex(), InheritedSet);
		return TAPI_SUCCESS;
	}

	void GPU_ContentManager::Delete_RTSlotSet(GFX_API::GFXHandle RTSlotSetHandle) {
		if (!RTSlotSetHandle) {
			return;
		}
		VK_RTSLOTSET* SlotSet = GFXHandleConverter(VK_RTSLOTSET*, RTSlotSetHandle);

		delete[] SlotSet->PERFRAME_SLOTSETs[0].COLOR_SLOTs;
		delete SlotSet->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT;
		delete[] SlotSet->PERFRAME_SLOTSETs[1].COLOR_SLOTs;
		delete SlotSet->PERFRAME_SLOTSETs[1].DEPTHSTENCIL_SLOT;
		delete SlotSet;

		unsigned char ThreadID = 0;
		unsigned int ElementIndex = 0;
		if (RT_SLOTSETs.SearchAllThreads(SlotSet, ThreadID, ElementIndex)) {
			std::unique_lock<std::mutex> Locker;
			RT_SLOTSETs.PauseAllOperations(Locker);
			RT_SLOTSETs.erase(ThreadID, ElementIndex);
		}
	}
}