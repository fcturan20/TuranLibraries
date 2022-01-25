#include "gpucontentmanager.h"
#include <tgfx_core.h>
#include <tgfx_gpucontentmanager.h>
#include "predefinitions_vk.h"
#include "extension.h"
#include "core.h"
#include "includes.h"
#include "renderer.h"
#include <mutex>
#include "resource.h"
#include "memory.h"


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

	//These are the textures that will be deleted after waiting for 2 frames ago's command buffer
	threadlocal_vector<texture_vk*> DeleteTextureList;
	//These are the texture that will be added to the list above after clearing the above list
	threadlocal_vector<texture_vk*> NextFrameDeleteTextureCalls;

	gpudatamanager_private() : DescSets_toCreate(1024, nullptr), DescSets_toCreateUpdate(1024, descset_updatecall_vk()), DescSets_toJustUpdate(1024, descset_updatecall_vk()), DeleteTextureList(1024, nullptr), NextFrameDeleteTextureCalls(1024, nullptr) {}
};
static gpudatamanager_private* hidden = nullptr;

void Update_GlobalDescSet_DescIndexing(descset_vk& Set, VkDescriptorPool Pool) {
	//Update Descriptor Sets
	{
		std::vector<VkWriteDescriptorSet> UpdateInfos;

		for (unsigned int BindingIndex = 0; BindingIndex < 2; BindingIndex++) {
			descriptor_vk& Desc = Set.Descs[BindingIndex];
			if (Desc.Type == desctype_vk::IMAGE || Desc.Type == desctype_vk::SAMPLER) {
				for (unsigned int DescElementIndex = 0; DescElementIndex < Desc.ElementCount; DescElementIndex++) {
					descelement_image_vk& im = ((descelement_image_vk*)Desc.Elements)[DescElementIndex];
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
					descelement_buffer_vk& buf = ((descelement_buffer_vk*)Desc.Elements)[DescElementIndex];
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

		vkUpdateDescriptorSets(rendergpu->LOGICALDEVICE(), UpdateInfos.size(), UpdateInfos.data(), 0, nullptr);
	}
}
void Update_GlobalDescSet_NonDescIndexing(descset_vk& Set, VkDescriptorPool Pool) {
	//Update Descriptor Sets
	{
		std::vector<VkWriteDescriptorSet> UpdateInfos;

		for (unsigned int BindingIndex = 0; BindingIndex < 2; BindingIndex++) {
			descriptor_vk& Desc = Set.Descs[BindingIndex];
			if (Desc.Type == desctype_vk::IMAGE || Desc.Type == desctype_vk::SAMPLER) {
				for (unsigned int DescElementIndex = 0; DescElementIndex < Desc.ElementCount; DescElementIndex++) {
					descelement_image_vk& im = ((descelement_image_vk*)Desc.Elements)[DescElementIndex];
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
					descelement_buffer_vk& buf = ((descelement_buffer_vk*)Desc.Elements)[DescElementIndex];
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

		vkUpdateDescriptorSets(rendergpu->LOGICALDEVICE(), UpdateInfos.size(), UpdateInfos.data(), 0, nullptr);
	}
}
void CopyDescriptorSets(descset_vk& Set, std::vector<VkCopyDescriptorSet>& CopyVector, std::vector<VkDescriptorSet*>& CopyTargetSets) {
	for (unsigned int DescIndex = 0; DescIndex < Set.DescCount; DescIndex++) {
		VkCopyDescriptorSet copyinfo;
		descriptor_vk& SourceDesc = ((descriptor_vk*)Set.Descs)[DescIndex];
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
void gpudatamanager_public::Apply_ResourceChanges() {
	const unsigned char FrameIndex = renderer->Get_FrameIndex(false);


	//Manage Global Descriptor Set
	if (hidden->GlobalBuffers_DescSet.ShouldRecreate.load()) {
		VkDescriptorSet backSet = hidden->UnboundGlobalBufferDescSet;
		hidden->UnboundGlobalBufferDescSet = hidden->GlobalBuffers_DescSet.Set;
		hidden->GlobalBuffers_DescSet.Set = backSet;
		hidden->GlobalBuffers_DescSet.ShouldRecreate.store(false);
	}
	if (rendergpu->EXTMANAGER()->ISSUPPORTED_DESCINDEXING()) {
		Update_GlobalDescSet_DescIndexing(hidden->GlobalBuffers_DescSet, hidden->GlobalShaderInputs_DescPool);
	}
	else {
		Update_GlobalDescSet_NonDescIndexing(hidden->GlobalBuffers_DescSet, hidden->GlobalShaderInputs_DescPool);
	}
	if (hidden->GlobalTextures_DescSet.ShouldRecreate.load()) {
		VkDescriptorSet backSet = hidden->UnboundGlobalTextureDescSet;
		hidden->UnboundGlobalTextureDescSet = hidden->GlobalTextures_DescSet.Set;
		hidden->GlobalTextures_DescSet.Set = backSet;
		hidden->GlobalTextures_DescSet.ShouldRecreate.store(false);
	}
	if (rendergpu->EXTMANAGER()->ISSUPPORTED_DESCINDEXING()) {
		Update_GlobalDescSet_DescIndexing(hidden->GlobalTextures_DescSet, hidden->GlobalShaderInputs_DescPool);
	}
	else {
		Update_GlobalDescSet_NonDescIndexing(hidden->GlobalTextures_DescSet, hidden->GlobalShaderInputs_DescPool);
	}

	//Manage Material Related Descriptor Sets
	{
		//Destroy descriptor sets that are changed last frame
		if (hidden->UnboundMaterialDescSetList.size()) {
			descpool_vk& DP = hidden->MaterialRelated_DescPool;
			vkFreeDescriptorSets(rendergpu->LOGICALDEVICE(), DP.pool, hidden->UnboundMaterialDescSetList.size(),
				hidden->UnboundMaterialDescSetList.data());
			DP.REMAINING_SET.fetch_add(hidden->UnboundMaterialDescSetList.size());

			DP.REMAINING_IMAGE.fetch_add(hidden->UnboundDescSetImageCount);
			hidden->UnboundDescSetImageCount = 0;
			DP.REMAINING_SAMPLER.fetch_add(hidden->UnboundDescSetSamplerCount);
			hidden->UnboundDescSetSamplerCount = 0;
			DP.REMAINING_SBUFFER.fetch_add(hidden->UnboundDescSetSBufferCount);
			hidden->UnboundDescSetSBufferCount = 0;
			DP.REMAINING_UBUFFER.fetch_add(hidden->UnboundDescSetUBufferCount);
			hidden->UnboundDescSetUBufferCount = 0;
			hidden->UnboundMaterialDescSetList.clear();
		}
		//Create Descriptor Sets for material types/instances that are created this frame
		{
			std::vector<VkDescriptorSet> Sets;
			std::vector<VkDescriptorSet*> SetPTRs;
			std::vector<VkDescriptorSetLayout> SetLayouts;
			std::unique_lock<std::mutex> Locker;
			hidden->DescSets_toCreate.PauseAllOperations(Locker);
			for (unsigned int ThreadIndex = 0; ThreadIndex < threadcount; ThreadIndex++) {
				for (unsigned int SetIndex = 0; SetIndex < hidden->DescSets_toCreate.size(ThreadIndex); SetIndex++) {
					descset_vk* Set = hidden->DescSets_toCreate.get(ThreadIndex, SetIndex);
					Sets.push_back(VkDescriptorSet());
					SetLayouts.push_back(Set->Layout);
					SetPTRs.push_back(&Set->Set);
				}
				hidden->DescSets_toCreate.clear(ThreadIndex);
			}
			Locker.unlock();

			if (Sets.size()) {
				VkDescriptorSetAllocateInfo al_in = {};
				al_in.descriptorPool = hidden->MaterialRelated_DescPool.pool;
				al_in.descriptorSetCount = Sets.size();
				al_in.pNext = nullptr;
				al_in.pSetLayouts = SetLayouts.data();
				al_in.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				vkAllocateDescriptorSets(rendergpu->LOGICALDEVICE(), &al_in, Sets.data());
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
			hidden->DescSets_toCreateUpdate.PauseAllOperations(Locker);
			for (unsigned int ThreadIndex = 0; ThreadIndex < threadcount; ThreadIndex++) {
				for (unsigned int SetIndex = 0; SetIndex < hidden->DescSets_toCreateUpdate.size(ThreadIndex); SetIndex++) {
					descset_updatecall_vk& Call = hidden->DescSets_toCreateUpdate.get(ThreadIndex, SetIndex);
					descset_vk* Set = Call.Set;
					bool SetStatus = Set->ShouldRecreate.load();
					switch (SetStatus) {
					case 0:
						continue;
					case 1:
						NewSets.push_back(VkDescriptorSet());
						SetPTRs.push_back(&Set->Set);
						SetLayouts.push_back(Set->Layout);
						hidden->UnboundDescSetImageCount += Set->DescImagesCount;
						hidden->UnboundDescSetSamplerCount += Set->DescSamplersCount;
						hidden->UnboundDescSetSBufferCount += Set->DescSBuffersCount;
						hidden->UnboundDescSetUBufferCount += Set->DescUBuffersCount;
						hidden->UnboundMaterialDescSetList.push_back(Set->Set);

						CopyDescriptorSets(*Set, CopySetInfos, CopyTargetSets);
						Set->ShouldRecreate.store(0);
						break;
					default:
						printer(result_tgfx_NOTCODED, "Descriptor Set atomic_uchar isn't supposed to have a value that's 2+! Please check 'Handle Descriptor Sets' in Vulkan Renderer->Run()");
						break;
					}
				}
			}

			if (NewSets.size()) {
				VkDescriptorSetAllocateInfo al_in = {};
				al_in.descriptorPool = hidden->MaterialRelated_DescPool.pool;
				al_in.descriptorSetCount = NewSets.size();
				al_in.pNext = nullptr;
				al_in.pSetLayouts = SetLayouts.data();
				al_in.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				vkAllocateDescriptorSets(rendergpu->LOGICALDEVICE(), &al_in, NewSets.data());
				for (unsigned int SetIndex = 0; SetIndex < NewSets.size(); SetIndex++) {
					*SetPTRs[SetIndex] = NewSets[SetIndex];
				}

				for (unsigned int CopyIndex = 0; CopyIndex < CopySetInfos.size(); CopyIndex++) {
					CopySetInfos[CopyIndex].dstSet = *CopyTargetSets[CopyIndex];
				}
				vkUpdateDescriptorSets(rendergpu->LOGICALDEVICE(), 0, nullptr, CopySetInfos.size(), CopySetInfos.data());
			}
		}
		//Update descriptor sets
		{
			std::vector<VkWriteDescriptorSet> UpdateInfos;
			std::unique_lock<std::mutex> Locker1;
			hidden->DescSets_toCreateUpdate.PauseAllOperations(Locker1);
			for (unsigned int ThreadIndex = 0; ThreadIndex < threadcount; ThreadIndex++) {
				for (unsigned int CallIndex = 0; CallIndex < hidden->DescSets_toCreateUpdate.size(ThreadIndex); CallIndex++) {
					descset_updatecall_vk& Call = hidden->DescSets_toCreateUpdate.get(ThreadIndex, CallIndex);
					VkWriteDescriptorSet info = {};
					info.descriptorCount = 1;
					descriptor_vk& Desc = Call.Set->Descs[Call.BindingIndex];
					switch (Desc.Type) {
					case desctype_vk::IMAGE:
						info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pImageInfo = &((descelement_image_vk*)Desc.Elements)[Call.ElementIndex].info;
						((descelement_image_vk*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case desctype_vk::SAMPLER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pImageInfo = &((descelement_image_vk*)Desc.Elements)[Call.ElementIndex].info;
						((descelement_image_vk*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case desctype_vk::UBUFFER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pBufferInfo = &((descelement_buffer_vk*)Desc.Elements)[Call.ElementIndex].Info;
						((descelement_buffer_vk*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case desctype_vk::SBUFFER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pBufferInfo = &((descelement_buffer_vk*)Desc.Elements)[Call.ElementIndex].Info;
						((descelement_buffer_vk*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					}
					info.dstSet = Call.Set->Set;
					info.pNext = nullptr;
					info.pTexelBufferView = nullptr;
					info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					UpdateInfos.push_back(info);
				}
				hidden->DescSets_toCreateUpdate.clear(ThreadIndex);
			}
			Locker1.unlock();


			std::unique_lock<std::mutex> Locker2;
			hidden->DescSets_toJustUpdate.PauseAllOperations(Locker2);
			for (unsigned int ThreadIndex = 0; ThreadIndex < threadcount; ThreadIndex++) {
				for (unsigned int CallIndex = 0; CallIndex < hidden->DescSets_toJustUpdate.size(ThreadIndex); CallIndex++) {
					descset_updatecall_vk& Call = hidden->DescSets_toJustUpdate.get(ThreadIndex, CallIndex);
					VkWriteDescriptorSet info = {};
					info.descriptorCount = 1;
					descriptor_vk& Desc = Call.Set->Descs[Call.BindingIndex];
					switch (Desc.Type) {
					case desctype_vk::IMAGE:
						info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pImageInfo = &((descelement_image_vk*)Desc.Elements)[Call.ElementIndex].info;
						((descelement_image_vk*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case desctype_vk::SAMPLER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pImageInfo = &((descelement_image_vk*)Desc.Elements)[Call.ElementIndex].info;
						((descelement_image_vk*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case desctype_vk::UBUFFER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pBufferInfo = &((descelement_buffer_vk*)Desc.Elements)[Call.ElementIndex].Info;
						((descelement_buffer_vk*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					case desctype_vk::SBUFFER:
						info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						info.dstBinding = Call.BindingIndex;
						info.dstArrayElement = Call.ElementIndex;
						info.pBufferInfo = &((descelement_buffer_vk*)Desc.Elements)[Call.ElementIndex].Info;
						((descelement_buffer_vk*)Desc.Elements)[Call.ElementIndex].IsUpdated.store(0);
						break;
					}
					info.dstSet = Call.Set->Set;
					info.pNext = nullptr;
					info.pTexelBufferView = nullptr;
					info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					UpdateInfos.push_back(info);
				}
				hidden->DescSets_toJustUpdate.clear(ThreadIndex);
			}
			Locker2.unlock();

			vkUpdateDescriptorSets(rendergpu->LOGICALDEVICE(), UpdateInfos.size(), UpdateInfos.data(), 0, nullptr);
		}
	}
	renderer->RendererResource_Finalizations();
	
	//Delete textures
	{
		std::unique_lock<std::mutex> Locker;
		hidden->DeleteTextureList.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < threadcount; ThreadID++) {
			for (unsigned int i = 0; i < hidden->DeleteTextureList.size(ThreadID); i++) {
				texture_vk* Texture = hidden->DeleteTextureList.get(ThreadID, i);
				if (Texture->Image) {
					vkDestroyImageView(rendergpu->LOGICALDEVICE(), Texture->ImageView, nullptr);
					vkDestroyImage(rendergpu->LOGICALDEVICE(), Texture->Image, nullptr);
				}
				if (Texture->Block.MemAllocIndex != UINT32_MAX) {
					allocatorsys->free_memoryblock(rendergpu->MEMORYTYPE_IDS()[Texture->Block.MemAllocIndex], Texture->Block.Offset);
				}
				delete Texture;
			}
			hidden->DeleteTextureList.clear(ThreadID);
		}
	}
	//Push next frame delete texture list to the delete textures list
	{
		std::unique_lock<std::mutex> Locker;
		hidden->NextFrameDeleteTextureCalls.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < threadcount; ThreadID++) {
			for (unsigned int i = 0; i < hidden->NextFrameDeleteTextureCalls.size(ThreadID); i++) {
				hidden->DeleteTextureList.push_back(hidden->NextFrameDeleteTextureCalls.get(ThreadID, i));
			}
			hidden->NextFrameDeleteTextureCalls.clear();
		}
	}
	

}

void gpudatamanager_public::Resource_Finalizations() {
	//If Descriptor Indexing isn't supported, check if any global descriptor isn't set before!
	if (!rendergpu->EXTMANAGER()->ISSUPPORTED_DESCINDEXING()) {
		for (unsigned char BindingIndex = 0; BindingIndex < 2; BindingIndex++) {
			descriptor_vk& BufferBinding = hidden->GlobalBuffers_DescSet.Descs[BindingIndex];
			for (unsigned int ElementIndex = 0; ElementIndex < BufferBinding.ElementCount; ElementIndex++) {
				descelement_buffer_vk& element = ((descelement_buffer_vk*)BufferBinding.Elements)[ElementIndex];
				if (element.IsUpdated.load() == 255) {
					printer(result_tgfx_FAIL, ("You have to use SetGlobalBuffer() for element index: " + std::to_string(ElementIndex)).c_str());
					return;
				}
			}
			descriptor_vk& TextureBinding = hidden->GlobalTextures_DescSet.Descs[BindingIndex];
			for (unsigned int ElementIndex = 0; ElementIndex < TextureBinding.ElementCount; ElementIndex++) {
				descelement_image_vk& element = ((descelement_image_vk*)TextureBinding.Elements)[ElementIndex];
				if (element.IsUpdated.load() == 255) {
					printer(result_tgfx_FAIL, ("You have to use SetGlobalTexture() for element index: " + std::to_string(ElementIndex)).c_str());
					return;
				}
			}
		}
	}

	
	if (rendergpu->EXTMANAGER()->ISSUPPORTED_DESCINDEXING()) {
		Update_GlobalDescSet_DescIndexing(hidden->GlobalBuffers_DescSet, hidden->GlobalShaderInputs_DescPool);
		Update_GlobalDescSet_DescIndexing(hidden->GlobalTextures_DescSet, hidden->GlobalShaderInputs_DescPool);
	}
	else {
		Update_GlobalDescSet_NonDescIndexing(hidden->GlobalBuffers_DescSet, hidden->GlobalShaderInputs_DescPool);
		Update_GlobalDescSet_NonDescIndexing(hidden->GlobalTextures_DescSet, hidden->GlobalShaderInputs_DescPool);
	}
}


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