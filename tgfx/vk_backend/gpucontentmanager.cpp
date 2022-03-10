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
#include "includes.h"
#include <glslang/SPIRV/GlslangToSpv.h>

TBuiltInResource glsl_to_spirv_limitationtable;

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
struct descset_updatecall_vk {
	descset_vk* Set = nullptr;
	unsigned int BindingIndex = UINT32_MAX, ElementIndex = UINT32_MAX;
};
struct descpool_vk {
	VkDescriptorPool pool;
	std::atomic<uint32_t> REMAINING_SET, REMAINING_UBUFFER, REMAINING_SBUFFER, REMAINING_SAMPLER, REMAINING_IMAGE;
};
struct shadersource_vk {
	VkShaderModule Module;
	shaderstage_tgfx stage;
	void* SOURCE_CODE = nullptr;
	unsigned int DATA_SIZE = 0;
};

struct gpudatamanager_private {
	threadlocal_vector<rtslotset_vk*> rtslotsets;
	threadlocal_vector<texture_vk*> textures;
	threadlocal_vector<irtslotset_vk*> irtslotsets;
	threadlocal_vector<graphicspipelinetype_vk*> graphicspipelinetypes;
	threadlocal_vector<graphicspipelineinst_vk*> graphicspipelineinstances;
	threadlocal_vector<vertexattriblayout_vk*> vertexattributelayouts;
	threadlocal_vector<shadersource_vk*> shadersources;
	threadlocal_vector<vertexbuffer_vk*> vertexbuffers;
	threadlocal_vector<memoryblock_vk*> stagingbuffers;
	threadlocal_vector<sampler_vk*> samplers;
	threadlocal_vector<computetype_vk*> computetypes;
	threadlocal_vector<computeinstance_vk*> computeinstances;
	threadlocal_vector<globalbuffer_vk*> globalbuffers;


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

	gpudatamanager_private() : DescSets_toCreate(1024), DescSets_toCreateUpdate(1024), DescSets_toJustUpdate(1024), DeleteTextureList(1024), NextFrameDeleteTextureCalls(1024), rtslotsets(10), textures(100), globalbuffers(1024),
	irtslotsets(100), graphicspipelineinstances(1024), graphicspipelinetypes(1024), vertexattributelayouts(1024), shadersources(1024), vertexbuffers(1024), stagingbuffers(100), samplers(100), computetypes(1024), computeinstances(1024) {}
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

	//Re-create VkFramebuffers etc.
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
//General DescriptorSet Layout Creation
bool Create_DescSet(descset_vk* Set) {
	if (!Set->DescCount) {
		return true;
	}
	if (!hidden->MaterialRelated_DescPool.REMAINING_IMAGE.fetch_sub(Set->DescImagesCount) ||
		!hidden->MaterialRelated_DescPool.REMAINING_SAMPLER.fetch_sub(Set->DescSamplersCount) ||
		!hidden->MaterialRelated_DescPool.REMAINING_SBUFFER.fetch_sub(Set->DescSBuffersCount) ||
		!hidden->MaterialRelated_DescPool.REMAINING_UBUFFER.fetch_sub(Set->DescUBuffersCount) ||
		!hidden->MaterialRelated_DescPool.REMAINING_SET.fetch_sub(1)) {
		printer(result_tgfx_FAIL, "Create_DescSets() has failed because descriptor pool doesn't have enough space!");
		return false;
	}
	hidden->DescSets_toCreate.push_back(Set);
	return true;
}
bool VKDescSet_PipelineLayoutCreation(const shaderinputdesc_vk** inputdescs, descset_vk* GeneralDescSet, descset_vk* InstanceDescSet, VkPipelineLayout* layout) {
	TGFXLISTCOUNT(core_tgfx_main, inputdescs, inputdesccount);
	//General DescSet creation
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		for (unsigned int i = 0; i < inputdesccount; i++) {
			const shaderinputdesc_vk* desc = inputdescs[i];
			if (!inputdescs[i]) {
				continue;
			}
			
			if (!(desc->TYPE == shaderinputtype_tgfx_IMAGE_G ||
				desc->TYPE == shaderinputtype_tgfx_SAMPLER_G ||
				desc->TYPE == shaderinputtype_tgfx_SBUFFER_G ||
				desc->TYPE == shaderinputtype_tgfx_UBUFFER_G)) {
				continue;
			}

			unsigned int BP = desc->BINDINDEX;
			for (unsigned int bpsearchindex = 0; bpsearchindex < bindings.size(); bpsearchindex++) {
				if (BP == bindings[bpsearchindex].binding) {
					printer(result_tgfx_FAIL, "VKDescSet_PipelineLayoutCreation() has failed because there are colliding binding points!");
					return false;
				}
			}

			if (!desc->ELEMENTCOUNT) {
				printer(result_tgfx_FAIL, "VKDescSet_PipelineLayoutCreation() has failed because one of the shader inputs have 0 element count!");
				return false;
			}

			VkDescriptorSetLayoutBinding bn = {};
			bn.stageFlags = desc->ShaderStages;
			bn.pImmutableSamplers = VK_NULL_HANDLE;
			bn.descriptorType = Find_VkDescType_byMATDATATYPE(desc->TYPE);
			bn.descriptorCount = desc->ELEMENTCOUNT;
			bn.binding = BP;
			bindings.push_back(bn);

			GeneralDescSet->DescCount++;
			switch (desc->TYPE) {
			case shaderinputtype_tgfx_IMAGE_G:
				GeneralDescSet->DescImagesCount += desc->ELEMENTCOUNT;
				break;
			case shaderinputtype_tgfx_SAMPLER_G:
				GeneralDescSet->DescSamplersCount += desc->ELEMENTCOUNT;
				break;
			case shaderinputtype_tgfx_UBUFFER_G:
				GeneralDescSet->DescUBuffersCount += desc->ELEMENTCOUNT;
				break;
			case shaderinputtype_tgfx_SBUFFER_G:
				GeneralDescSet->DescSBuffersCount += desc->ELEMENTCOUNT;
				break;
			}
		}

		if (GeneralDescSet->DescCount) {
			GeneralDescSet->Descs = new descriptor_vk[GeneralDescSet->DescCount];
			for (unsigned int i = 0; i < inputdesccount; i++) {
				const shaderinputdesc_vk* desc = inputdescs[i];
				if (!(desc->TYPE == shaderinputtype_tgfx_IMAGE_G ||
					desc->TYPE == shaderinputtype_tgfx_SAMPLER_G ||
					desc->TYPE == shaderinputtype_tgfx_SBUFFER_G ||
					desc->TYPE == shaderinputtype_tgfx_UBUFFER_G)) {
					continue;
				}
				if (desc->BINDINDEX >= GeneralDescSet->DescCount) {
					printer(result_tgfx_FAIL, "One of your General shaderinputs uses a binding point that is exceeding the number of general shaderinputs. You have to use a binding point that's lower than size of the general shader inputs!");
					return false;
				}

				descriptor_vk& vkdesc = GeneralDescSet->Descs[desc->BINDINDEX];
				switch (desc->TYPE) {	
				case shaderinputtype_tgfx_IMAGE_G:
				{
					vkdesc.ElementCount = desc->ELEMENTCOUNT;
					vkdesc.Elements = new descelement_image_vk[desc->ELEMENTCOUNT];
					vkdesc.Type = desctype_vk::IMAGE;
				}
				break;
				case shaderinputtype_tgfx_SAMPLER_G:
				{
					vkdesc.ElementCount = desc->ELEMENTCOUNT;
					vkdesc.Elements = new descelement_image_vk[desc->ELEMENTCOUNT];
					vkdesc.Type = desctype_vk::SAMPLER;
				}
				break;
				case shaderinputtype_tgfx_UBUFFER_G:
				{
					vkdesc.ElementCount = desc->ELEMENTCOUNT;
					vkdesc.Elements = new descelement_buffer_vk[desc->ELEMENTCOUNT];
					vkdesc.Type = desctype_vk::UBUFFER;
				}
				break;
				case shaderinputtype_tgfx_SBUFFER_G:
				{
					vkdesc.ElementCount = desc->ELEMENTCOUNT;
					vkdesc.Elements = new descelement_buffer_vk[desc->ELEMENTCOUNT];
					vkdesc.Type = desctype_vk::SBUFFER;
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
			

			if (vkCreateDescriptorSetLayout(rendergpu->LOGICALDEVICE(), &ci, nullptr, &GeneralDescSet->Layout) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "VKDescSet_PipelineLayoutCreation() has failed at General DescriptorSetLayout Creation vkCreateDescriptorSetLayout()");
				return false;
			}
		}
	}

	//Instance DescriptorSet Layout Creation
	if (true) {
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		for (unsigned int i = 0; i < inputdesccount; i++) {
			const shaderinputdesc_vk* desc = inputdescs[i];

			if (!(desc->TYPE == shaderinputtype_tgfx_IMAGE_PI ||
				desc->TYPE == shaderinputtype_tgfx_SAMPLER_PI ||
				desc->TYPE == shaderinputtype_tgfx_SBUFFER_PI ||
				desc->TYPE == shaderinputtype_tgfx_UBUFFER_PI)) {
				continue;
			}
			unsigned int BP = desc->BINDINDEX;
			for (unsigned int bpsearchindex = 0; bpsearchindex < bindings.size(); bpsearchindex++) {
				if (BP == bindings[bpsearchindex].binding) {
					printer(result_tgfx_FAIL, "Link_MaterialType() has failed because there are colliding binding points!");
					return false;
				}
			}
			VkDescriptorSetLayoutBinding bn = {};
			bn.stageFlags = desc->ShaderStages;
			bn.pImmutableSamplers = VK_NULL_HANDLE;
			bn.descriptorType = Find_VkDescType_byMATDATATYPE(desc->TYPE);
			bn.descriptorCount = 1;		//I don't support array descriptors for now!
			bn.binding = BP;
			bindings.push_back(bn);

			InstanceDescSet->DescCount++;
			switch (desc->TYPE) {
			case shaderinputtype_tgfx_IMAGE_PI:
			{
				InstanceDescSet->DescImagesCount += desc->ELEMENTCOUNT;
			}
			break;
			case shaderinputtype_tgfx_SAMPLER_PI:
			{
				InstanceDescSet->DescSamplersCount += desc->ELEMENTCOUNT;
			}
			break;
			case shaderinputtype_tgfx_UBUFFER_PI:
			{
				InstanceDescSet->DescUBuffersCount += desc->ELEMENTCOUNT;
			}
			break;
			case shaderinputtype_tgfx_SBUFFER_PI:
			{
				InstanceDescSet->DescSBuffersCount += desc->ELEMENTCOUNT;
			}
			break;
			}
		}

		if (InstanceDescSet->DescCount) {
			InstanceDescSet->Descs = new descriptor_vk[InstanceDescSet->DescCount];
			for (unsigned int i = 0; i < inputdesccount; i++) {
				const shaderinputdesc_vk* desc = inputdescs[i];
				if (!(desc->TYPE == shaderinputtype_tgfx_IMAGE_PI ||
					desc->TYPE == shaderinputtype_tgfx_SAMPLER_PI ||
					desc->TYPE == shaderinputtype_tgfx_SBUFFER_PI ||
					desc->TYPE == shaderinputtype_tgfx_UBUFFER_PI)) {
					continue;
				}

				if (desc->BINDINDEX >= InstanceDescSet->DescCount) {
					printer(result_tgfx_FAIL, "One of your Material Data Descriptors (Per Instance) uses a binding point that is exceeding the number of Material Data Descriptors (Per Instance). You have to use a binding point that's lower than size of the Material Data Descriptors (Per Instance)!");
					return false;
				}

				//We don't need to create each descriptor's array elements
				descriptor_vk& vkdesc = InstanceDescSet->Descs[desc->BINDINDEX];
				vkdesc.ElementCount = desc->ELEMENTCOUNT;
				switch (desc->TYPE) {
				case shaderinputtype_tgfx_IMAGE_PI:
					vkdesc.Type = desctype_vk::IMAGE;
					break;
				case shaderinputtype_tgfx_SAMPLER_PI:
					vkdesc.Type = desctype_vk::SAMPLER;
					break;
				case shaderinputtype_tgfx_UBUFFER_PI:
					vkdesc.Type = desctype_vk::UBUFFER;
					break;
				case shaderinputtype_tgfx_SBUFFER_PI:
					vkdesc.Type = desctype_vk::SBUFFER;
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

			if (vkCreateDescriptorSetLayout(rendergpu->LOGICALDEVICE(), &ci, nullptr, &InstanceDescSet->Layout) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "Link_MaterialType() has failed at Instance DesciptorSetLayout Creation vkCreateDescriptorSetLayout()");
				return false;
			}
		}
	}

	//General DescriptorSet Creation
	if (!Create_DescSet(GeneralDescSet)) {
		printer(result_tgfx_FAIL, "Descriptor pool is full, that means you should expand its size!");
		return false;
	}

	//Pipeline Layout Creation
	{
		VkPipelineLayoutCreateInfo pl_ci = {};
		pl_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pl_ci.pNext = nullptr;
		pl_ci.flags = 0;

		VkDescriptorSetLayout descsetlays[4] = { hidden->GlobalBuffers_DescSet.Layout, hidden->GlobalTextures_DescSet.Layout, VK_NULL_HANDLE, VK_NULL_HANDLE };
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

		if (vkCreatePipelineLayout(rendergpu->LOGICALDEVICE(), &pl_ci, nullptr, layout) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "Link_MaterialType() failed at vkCreatePipelineLayout()!");
			return false;
		}
	}
	return true;
}


VkDescriptorSet gpudatamanager_public::get_GlobalBuffersDescSet() { return hidden->GlobalBuffers_DescSet.Set; }
VkDescriptorSet gpudatamanager_public::get_GlobalTexturesDescsSet() {return hidden->GlobalTextures_DescSet.Set;}
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
	if (TYPE == buffertype_tgfx_GLOBAL) {
		globalbuffer_vk* buffer = (globalbuffer_vk*)Handle;
		TargetBuffer = allocatorsys->get_memorybufferhandle_byID(rendergpu, buffer->Block.MemAllocIndex);
		TargetOffset = buffer->Block.Offset;
	}
	else {
		printer(result_tgfx_FAIL, "FindBufferOBJ_byBufType() doesn't support this type!");
	}
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

	sampler_vk* SAMPLER = new sampler_vk;
	if (vkCreateSampler(rendergpu->LOGICALDEVICE(), &s_ci, nullptr, &SAMPLER->Sampler) != VK_SUCCESS) {
		delete SAMPLER;
		printer(result_tgfx_FAIL, "GFXContentManager->Create_SamplingType() has failed at vkCreateSampler!");
		return result_tgfx_FAIL;
	}
	*SamplingTypeHandle = (samplingtype_tgfx_handle)SAMPLER;
	hidden->samplers.push_back(SAMPLER);
	return result_tgfx_SUCCESS;
}

/*Attributes are ordered as the same order of input vector
* For example: Same attribute ID may have different location/order in another attribute layout
* So you should gather your vertex buffer data according to that
*/

inline unsigned int Calculate_sizeofVertexLayout(const datatype_tgfx* ATTRIBUTEs, unsigned int count) {
	unsigned int size = 0;
	for (unsigned int i = 0; i < count; i++) {
		size += get_uniformtypes_sizeinbytes(ATTRIBUTEs[i]);
	}
	return size;
}
result_tgfx Create_VertexAttributeLayout (const datatype_tgfx* Attributes, vertexlisttypes_tgfx listtype,
	vertexattributelayout_tgfx_handle* VertexAttributeLayoutHandle){
	vertexattriblayout_vk* Layout = new vertexattriblayout_vk;
	unsigned int AttributesCount = 0;
	while (Attributes[AttributesCount] != datatype_tgfx_UNDEFINED) {
		AttributesCount++;
	}
	Layout->Attribute_Number = AttributesCount;
	Layout->Attributes = new datatype_tgfx[Layout->Attribute_Number];
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
		stride_ofcurrentattribute += get_uniformtypes_sizeinbytes(Layout->Attributes[i]);
	}
	hidden->vertexattributelayouts.push_back(Layout);
	*VertexAttributeLayoutHandle = (vertexattributelayout_tgfx_handle)Layout;
	return result_tgfx_SUCCESS;
}
void  Delete_VertexAttributeLayout (vertexattributelayout_tgfx_handle VertexAttributeLayoutHandle){}

result_tgfx Upload_toBuffer (buffer_tgfx_handle Handle, buffertype_tgfx Type, const void* DATA, unsigned int DATA_SIZE, unsigned int OFFSET){
	VkDeviceSize UploadOFFSET = static_cast<VkDeviceSize>(OFFSET);
	unsigned int MEMALLOCINDEX = UINT32_MAX;
	switch (Type) {
	case buffertype_tgfx_STAGING:
		MEMALLOCINDEX = ((memoryblock_vk*)Handle)->MemAllocIndex;
		UploadOFFSET += ((memoryblock_vk*)Handle)->Offset;
		break;
	case buffertype_tgfx_VERTEX:
		MEMALLOCINDEX = ((vertexbuffer_vk*)Handle)->Block.MemAllocIndex;
		UploadOFFSET += ((vertexbuffer_vk*)Handle)->Block.Offset;
		break;
	case buffertype_tgfx_GLOBAL:
		MEMALLOCINDEX = ((globalbuffer_vk*)Handle)->Block.MemAllocIndex;
		UploadOFFSET += ((globalbuffer_vk*)Handle)->Block.Offset;
		break;
	case buffertype_tgfx_INDEX:
	default:
		printer(result_tgfx_NOTCODED, "Upload_toBuffer() doesn't support this type for now!");
		return result_tgfx_NOTCODED;
	}

	return allocatorsys->copy_to_mappedmemory(rendergpu, MEMALLOCINDEX, DATA, DATA_SIZE, UploadOFFSET);
}

result_tgfx Create_StagingBuffer (unsigned int DATASIZE, unsigned int MemoryTypeIndex, buffer_tgfx_handle* Handle){
	if (!DATASIZE) {
		printer(result_tgfx_FAIL, "Staging Buffer DATASIZE is zero!");
		return result_tgfx_INVALIDARGUMENT;
	}
	if (allocatorsys->get_memorytype_byID(rendergpu, MemoryTypeIndex) == memoryallocationtype_DEVICELOCAL) {
		printer(result_tgfx_FAIL, "You can't create a staging buffer in DEVICELOCAL memory!");
		return result_tgfx_INVALIDARGUMENT;
	}
	VkBufferCreateInfo psuedo_ci = {};
	psuedo_ci.flags = 0;
	psuedo_ci.pNext = nullptr;
	if (rendergpu->QUEUEFAMSCOUNT() > 1) {
		psuedo_ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
	}
	else {
		psuedo_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	psuedo_ci.pQueueFamilyIndices = rendergpu->ALLQUEUEFAMILIES();
	psuedo_ci.queueFamilyIndexCount = rendergpu->QUEUEFAMSCOUNT();
	psuedo_ci.size = DATASIZE;
	psuedo_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	psuedo_ci.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
		| VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	VkBuffer Bufferobj;
	if (vkCreateBuffer(rendergpu->LOGICALDEVICE(), &psuedo_ci, nullptr, &Bufferobj) != VK_SUCCESS) {
		printer(result_tgfx_FAIL, "Intended staging buffer's creation failed at vkCreateBuffer()!");
		return result_tgfx_FAIL;
	}


	memoryblock_vk* StagingBuffer = new memoryblock_vk;
	StagingBuffer->MemAllocIndex = MemoryTypeIndex;
	if (allocatorsys->suballocate_buffer(Bufferobj, psuedo_ci.usage, *StagingBuffer) != result_tgfx_SUCCESS) {
		delete StagingBuffer;
		printer(result_tgfx_FAIL, "Suballocation has failed, so staging buffer creation too!");
		return result_tgfx_FAIL;
	}
	vkDestroyBuffer(rendergpu->LOGICALDEVICE(), Bufferobj, nullptr);
	*Handle = (buffer_tgfx_handle)StagingBuffer;
	hidden->stagingbuffers.push_back(StagingBuffer);
	return result_tgfx_SUCCESS;
}
void  Delete_StagingBuffer (buffer_tgfx_handle StagingBufferHandle){}
/*
* You should sort your vertex data according to attribute layout, don't forget that
* VertexCount shouldn't be 0
*/
result_tgfx Create_VertexBuffer (vertexattributelayout_tgfx_handle VertexAttributeLayoutHandle, unsigned int VertexCount,
	unsigned int MemoryTypeIndex, buffer_tgfx_handle* VertexBufferHandle){
	if (!VertexCount) {
		printer(result_tgfx_FAIL, "GFXContentManager->Create_MeshBuffer() has failed because vertex_count is zero!");
		return result_tgfx_INVALIDARGUMENT;
	}

	vertexattriblayout_vk* Layout = ((vertexattriblayout_vk*)VertexAttributeLayoutHandle);
	if (!Layout) {
		printer(result_tgfx_FAIL, "GFXContentManager->Create_MeshBuffer() has failed because Attribute Layout ID is invalid!");
		return result_tgfx_INVALIDARGUMENT;
	}

	unsigned int TOTALDATA_SIZE = Layout->size_perVertex * VertexCount;

	vertexbuffer_vk* VKMesh = new vertexbuffer_vk;
	VKMesh->Block.MemAllocIndex = MemoryTypeIndex;
	VKMesh->Layout = Layout;
	VKMesh->VERTEX_COUNT = VertexCount;

	VkBufferUsageFlags BufferUsageFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	VkBuffer Buffer = Create_VkBuffer(TOTALDATA_SIZE, BufferUsageFlag);
	if (allocatorsys->suballocate_buffer(Buffer, BufferUsageFlag, VKMesh->Block) != result_tgfx_SUCCESS) {
		delete VKMesh;
		printer(result_tgfx_FAIL, "There is no memory left in specified memory region!");
		vkDestroyBuffer(rendergpu->LOGICALDEVICE(), Buffer, nullptr);
		return result_tgfx_FAIL;
	}
	vkDestroyBuffer(rendergpu->LOGICALDEVICE(), Buffer, nullptr);


	hidden->vertexbuffers.push_back(VKMesh);
	*VertexBufferHandle = (buffer_tgfx_handle)VKMesh;
	return result_tgfx_SUCCESS;
}
void  Unload_VertexBuffer (buffer_tgfx_handle BufferHandle){}

result_tgfx Create_IndexBuffer (datatype_tgfx DataType, unsigned int IndexCount, unsigned int MemoryTypeIndex, buffer_tgfx_handle* IndexBufferHandle){ return result_tgfx_FAIL; }
void  Unload_IndexBuffer (buffer_tgfx_handle BufferHandle){}

result_tgfx Create_Texture (texture_dimensions_tgfx DIMENSION, unsigned int WIDTH, unsigned int HEIGHT, texture_channels_tgfx CHANNEL_TYPE,
	unsigned char MIPCOUNT, textureusageflag_tgfx_handle USAGE, texture_order_tgfx DATAORDER, unsigned int MemoryTypeIndex, texture_tgfx_handle* TextureHandle){
	if (MIPCOUNT > std::floor(std::log2(std::max(WIDTH, HEIGHT))) + 1 || !MIPCOUNT) {
		printer(result_tgfx_FAIL, "GFXContentManager->Create_Texture() has failed because mip count of the texture is wrong!");
		return result_tgfx_FAIL;
	}
	texture_vk* texture = new texture_vk;
	texture->CHANNELs = CHANNEL_TYPE;
	texture->HEIGHT = HEIGHT;
	texture->WIDTH = WIDTH;
	texture->DATA_SIZE = WIDTH * HEIGHT * GetByteSizeOf_TextureChannels(CHANNEL_TYPE);
	VkImageUsageFlags flag = *(VkImageUsageFlags*)USAGE;
	if (texture->CHANNELs == texture_channels_tgfx_D24S8 || texture->CHANNELs == texture_channels_tgfx_D32) { flag &= ~(1UL << 4); }
	else { flag &= ~(1UL << 5); }
	texture->USAGE = flag;
	texture->Block.MemAllocIndex = MemoryTypeIndex;
	texture->DIMENSION = DIMENSION;
	texture->MIPCOUNT = MIPCOUNT;


	//Create VkImage
	{
		VkImageCreateInfo im_ci = {};
		im_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		im_ci.extent.width = texture->WIDTH;
		im_ci.extent.height = texture->HEIGHT;
		im_ci.extent.depth = 1;
		if (DIMENSION == texture_dimensions_tgfx_2DCUBE) {
			im_ci.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			im_ci.arrayLayers = 6;
		}
		else {
			im_ci.flags = 0;
			im_ci.arrayLayers = 1;
		}
		im_ci.format = Find_VkFormat_byTEXTURECHANNELs(texture->CHANNELs);
		im_ci.imageType = Find_VkImageType(DIMENSION);
		im_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		im_ci.mipLevels = static_cast<uint32_t>(MIPCOUNT);
		im_ci.pNext = nullptr;
		if (rendergpu->QUEUEFAMSCOUNT() > 1) { im_ci.sharingMode = VK_SHARING_MODE_CONCURRENT; }
		else { im_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE; }
		im_ci.pQueueFamilyIndices = rendergpu->ALLQUEUEFAMILIES();
		im_ci.queueFamilyIndexCount = rendergpu->QUEUEFAMSCOUNT();
		im_ci.tiling = Find_VkTiling(DATAORDER);
		im_ci.usage = texture->USAGE;
		im_ci.samples = VK_SAMPLE_COUNT_1_BIT;

		if (vkCreateImage(rendergpu->LOGICALDEVICE(), &im_ci, nullptr, &texture->Image) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "GFXContentManager->Create_Texture() has failed in vkCreateImage()!");
			delete texture;
			return result_tgfx_FAIL;
		}

		if (allocatorsys->suballocate_image(texture) != result_tgfx_SUCCESS) {
			printer(result_tgfx_FAIL, "Suballocation of the texture has failed! Please re-create later.");
			delete texture;
			return result_tgfx_FAIL;
		}
	}

	//Create Image View
	{
		VkImageViewCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.flags = 0;
		ci.pNext = nullptr;

		ci.image = texture->Image;
		if (DIMENSION == texture_dimensions_tgfx_2DCUBE) {
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
		ci.format = Find_VkFormat_byTEXTURECHANNELs(texture->CHANNELs);
		if (texture->CHANNELs == texture_channels_tgfx_D32) {
			ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if (texture->CHANNELs == texture_channels_tgfx_D24S8) {
			ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else {
			ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		Fill_ComponentMapping_byCHANNELs(texture->CHANNELs, ci.components);

		if (vkCreateImageView(rendergpu->LOGICALDEVICE(), &ci, nullptr, &texture->ImageView) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "GFXContentManager->Upload_Texture() has failed in vkCreateImageView()!");
			return result_tgfx_FAIL;
		}
	}

	hidden->textures.push_back(texture);
	*TextureHandle = (texture_tgfx_handle)texture;
	return result_tgfx_SUCCESS;
}
//TARGET OFFSET is the offset in the texture's buffer to copy to
result_tgfx Upload_Texture (texture_tgfx_handle TextureHandle, const void* InputData, unsigned int DataSize, unsigned int TargetOffset){ return result_tgfx_FAIL; }
void  Delete_Texture (texture_tgfx_handle textureHANDLE, unsigned char isUsedLastFrame){}

result_tgfx Create_GlobalBuffer (const char* BUFFER_NAME, unsigned int DATA_SIZE, unsigned char isUniform,
	unsigned int MemoryTypeIndex, buffer_tgfx_handle* GlobalBufferHandle) {
	if (renderer->RGSTATUS() != RenderGraphStatus::Invalid) {
		printer(result_tgfx_FAIL, "GFX API don't support run-time Global Buffer addition for now because Vulkan needs to recreate PipelineLayouts (so all PSOs)! Please create your global buffers before render graph construction.");
		return result_tgfx_WRONGTIMING;
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


	globalbuffer_vk* GB = new globalbuffer_vk;
	GB->Block.MemAllocIndex = MemoryTypeIndex;
	if (allocatorsys->suballocate_buffer(obj, flags, GB->Block) != result_tgfx_SUCCESS) {
		printer(result_tgfx_FAIL, "Create_GlobalBuffer has failed at suballocation!");
		return result_tgfx_FAIL;
	}
	vkDestroyBuffer(rendergpu->LOGICALDEVICE(), obj, nullptr);

	GB->DATA_SIZE = DATA_SIZE;
	GB->isUniform = isUniform;

	*GlobalBufferHandle = (buffer_tgfx_handle)GB;
	hidden->globalbuffers.push_back(GB);
	return result_tgfx_SUCCESS;
}
void  Unload_GlobalBuffer (buffer_tgfx_handle BUFFER_ID){}
result_tgfx SetGlobalShaderInput_Buffer (unsigned char isUniformBuffer, unsigned int ElementIndex, unsigned char isUsedLastFrame,
	buffer_tgfx_handle BufferHandle, unsigned int BufferOffset, unsigned int BoundDataSize) {
	return result_tgfx_FAIL;
}
result_tgfx SetGlobalShaderInput_Texture (unsigned char isSampledTexture, unsigned int ElementIndex, unsigned char isUsedLastFrame,
	texture_tgfx_handle TextureHandle, samplingtype_tgfx_handle sampler, image_access_tgfx access) {
	desctype_vk intendedDescType = (isSampledTexture) ? desctype_vk::SAMPLER : desctype_vk::IMAGE;
	descelement_image_vk* element = nullptr;
	for (unsigned char i = 0; i < 2; i++) {
		if (hidden->GlobalTextures_DescSet.Descs[i].Type == intendedDescType) {
			if (hidden->GlobalTextures_DescSet.Descs[i].ElementCount <= ElementIndex) {
				printer(result_tgfx_FAIL, "SetGlobalTexture() has failed because ElementIndex isn't smaller than TextureCount of the GlobalTexture!");
				return result_tgfx_FAIL;
			}
			element = &((descelement_image_vk*)hidden->GlobalTextures_DescSet.Descs[i].Elements)[ElementIndex];
		}
	}

	unsigned char x = 0;
	if (!element->IsUpdated.compare_exchange_strong(x, 1)) {
		if (x != 255) {
			printer(result_tgfx_FAIL, "You already changed the global texture this frame, second or concurrent one will fail!");
			return result_tgfx_WRONGTIMING;
		}
		//If value is 255, this means global texture isn't set before so try to set it again!
		if (!element->IsUpdated.compare_exchange_strong(x, 1)) {
			printer(result_tgfx_FAIL, "You already changed the global texture this frame, second or concurrent one will fail!");
			return result_tgfx_WRONGTIMING;
		}
	}
	element->info.imageView = ((texture_vk*)TextureHandle)->ImageView;
	element->info.sampler = ((sampler_vk*)sampler)->Sampler;
	VkAccessFlags unused;
	Find_AccessPattern_byIMAGEACCESS(access, unused, element->info.imageLayout);
	if (isUsedLastFrame) {
		hidden->GlobalTextures_DescSet.ShouldRecreate.store(true);
	}
	return result_tgfx_SUCCESS;
}


static EShLanguage Find_EShShaderStage_byTGFXShaderStage(shaderstage_tgfx shaderstage) {
	switch (shaderstage) {
	case shaderstage_tgfx_VERTEXSHADER:
		return EShLangVertex;
	case shaderstage_tgfx_FRAGMENTSHADER:
		return EShLangFragment;
	case shaderstage_tgfx_COMPUTESHADER:
		return EShLangCompute;
	default:
		printer(result_tgfx_NOTCODED, "Find_EShShaderStage_byTGFXShaderStage() doesn't support this type of stage!");
		return EShLangVertex;
	}
}
static void* compile_shadersource_withglslang(shaderstage_tgfx tgfxstage, void* i_DATA, unsigned int i_DATA_SIZE, unsigned int* compiledbinary_datasize) {
	EShLanguage stage = Find_EShShaderStage_byTGFXShaderStage(tgfxstage);
	glslang::TShader shader(stage);
	glslang::TProgram program;

	// Enable SPIR-V and Vulkan rules when parsing GLSL
	EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

	const char* strings[1] = { (const char*)i_DATA };
	shader.setStrings(strings, 1);

	if (!shader.parse(&glsl_to_spirv_limitationtable, 100, false, messages)) {
		puts(shader.getInfoLog());
		puts(shader.getInfoDebugLog());
		return false;  // something didn't work
	}

	program.addShader(&shader);

	//
	// Program-level processing...
	//

	if (!program.link(messages)) {
		std::string log = std::string("Shader compilation failed!") + shader.getInfoLog() + std::string(shader.getInfoDebugLog());
		printer(result_tgfx_FAIL, log.c_str());
		return false;
	}
	std::vector<unsigned int> binarydata;
	glslang::GlslangToSpv(*program.getIntermediate(stage), binarydata);
	if (binarydata.size()) {
		unsigned int* outbinary = new unsigned int[binarydata.size()];
		*compiledbinary_datasize = binarydata.size() * 4;
		memcpy(outbinary, binarydata.data(), binarydata.size() * 4);
		return outbinary;
	}
	printer(result_tgfx_FAIL, "glslang couldn't compile the shader!");
	return nullptr;
}
result_tgfx Compile_ShaderSource (shaderlanguages_tgfx language, shaderstage_tgfx shaderstage, void* DATA, unsigned int DATA_SIZE,
	shadersource_tgfx_handle* ShaderSourceHandle) {
	void* binary_spirv_data = nullptr;
	unsigned int binary_spirv_datasize = 0;
	switch (language) {
	case shaderlanguages_tgfx_SPIRV:
		binary_spirv_data = DATA;
		binary_spirv_datasize = DATA_SIZE;
		break;
	case shaderlanguages_tgfx_GLSL:
		binary_spirv_data = compile_shadersource_withglslang(shaderstage, DATA, DATA_SIZE, &binary_spirv_datasize);
		break;
	default:
		printer(result_tgfx_NOTCODED, "Vulkan backend doesn't support this shading language");
	}


	//Create Vertex Shader Module
	VkShaderModuleCreateInfo ci = {};
	ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ci.flags = 0;
	ci.pNext = nullptr;
	ci.pCode = reinterpret_cast<const uint32_t*>(binary_spirv_data);
	ci.codeSize = static_cast<size_t>(binary_spirv_datasize);

	VkShaderModule Module;
	if (vkCreateShaderModule(rendergpu->LOGICALDEVICE(), &ci, 0, &Module) != VK_SUCCESS) {
		printer(result_tgfx_FAIL, "Shader Source is failed at creation!");
		return result_tgfx_FAIL;
	}

	shadersource_vk* SHADERSOURCE = new shadersource_vk;
	SHADERSOURCE->Module = Module;
	SHADERSOURCE->stage = shaderstage;
	hidden->shadersources.push_back(SHADERSOURCE);
	printer(result_tgfx_SUCCESS, "Shader Module is successfully created!");
	*ShaderSourceHandle = (shadersource_tgfx_handle)SHADERSOURCE;
	return result_tgfx_SUCCESS;
}
void  Delete_ShaderSource (shadersource_tgfx_handle ShaderSourceHandle){}
VkColorComponentFlags Find_ColorWriteMask_byChannels(texture_channels_tgfx channels) {
	switch (channels)
	{
	case texture_channels_tgfx_BGRA8UB:
	case texture_channels_tgfx_BGRA8UNORM:
	case texture_channels_tgfx_RGBA32F:
	case texture_channels_tgfx_RGBA32UI:
	case texture_channels_tgfx_RGBA32I:
	case texture_channels_tgfx_RGBA8UB:
	case texture_channels_tgfx_RGBA8B:
		return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	case texture_channels_tgfx_RGB32F:
	case texture_channels_tgfx_RGB32UI:
	case texture_channels_tgfx_RGB32I:
	case texture_channels_tgfx_RGB8UB:
	case texture_channels_tgfx_RGB8B:
		return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
	case texture_channels_tgfx_RA32F:
	case texture_channels_tgfx_RA32UI:
	case texture_channels_tgfx_RA32I:
	case texture_channels_tgfx_RA8UB:
	case texture_channels_tgfx_RA8B:
		return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT;
	case texture_channels_tgfx_R32F:
	case texture_channels_tgfx_R32UI:
	case texture_channels_tgfx_R32I:
		return VK_COLOR_COMPONENT_R_BIT;
	case texture_channels_tgfx_R8UB:
	case texture_channels_tgfx_R8B:
		return VK_COLOR_COMPONENT_R_BIT;
	case texture_channels_tgfx_D32:
	case texture_channels_tgfx_D24S8:
	default:
		printer(result_tgfx_NOTCODED, "Find_ColorWriteMask_byChannels() doesn't support this type of RTSlot channel!");
		return VK_COLOR_COMPONENT_FLAG_BITS_MAX_ENUM;
	}
}

result_tgfx Link_MaterialType(shadersource_tgfx_listhandle ShaderSourcesList, shaderinputdescription_tgfx_listhandle ShaderInputDescs,
	vertexattributelayout_tgfx_handle AttributeLayout, subdrawpass_tgfx_handle Subdrawpass, cullmode_tgfx culling,
	polygonmode_tgfx polygonmode, depthsettings_tgfx_handle depthtest, stencilsettings_tgfx_handle StencilFrontFaced,
	stencilsettings_tgfx_handle StencilBackFaced, blendinginfo_tgfx_listhandle BLENDINGs, rasterpipelinetype_tgfx_handle* MaterialHandle) {
	if (renderer->RGSTATUS() == RenderGraphStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "You can't link a Material Type while recording RenderGraph!");
		return result_tgfx_WRONGTIMING;
	}
	vertexattriblayout_vk* LAYOUT = (vertexattriblayout_vk*)AttributeLayout;
	if (!LAYOUT) {
		printer(result_tgfx_FAIL, "Link_MaterialType() has failed because Material Type has invalid Vertex Attribute Layout!");
		return result_tgfx_INVALIDARGUMENT;
	}
	subdrawpass_vk* Subpass = (subdrawpass_vk*)Subdrawpass;
	drawpass_vk* MainPass = (drawpass_vk*)Subpass->DrawPass;

	shadersource_vk* VertexSource = nullptr, *FragmentSource = nullptr;
	unsigned int ShaderSourceCount = 0;
	while (ShaderSourcesList[ShaderSourceCount] != core_tgfx_main->INVALIDHANDLE) {
		shadersource_vk* ShaderSource = ((shadersource_vk*)ShaderSourcesList[ShaderSourceCount]);
		switch (ShaderSource->stage) {
		case shaderstage_tgfx_VERTEXSHADER:
			if (VertexSource) { printer(result_tgfx_FAIL, "Link_MaterialType() has failed because there 2 vertex shaders in the list!"); return result_tgfx_FAIL; }
			VertexSource = ShaderSource;	break;
		case shaderstage_tgfx_FRAGMENTSHADER:
			if (FragmentSource) { printer(result_tgfx_FAIL, "Link_MaterialType() has failed because there 2 fragment shaders in the list!"); return result_tgfx_FAIL; }
			FragmentSource = ShaderSource; 	break;
		default:
			printer(result_tgfx_NOTCODED, "Link_MaterialType() has failed because list has unsupported shader source type!");
			return result_tgfx_NOTCODED;
		}
		ShaderSourceCount++;
	}


	//Subpass attachment should happen here!
	graphicspipelinetype_vk* VKPipeline = new graphicspipelinetype_vk;

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

	
	TGFXLISTCOUNT(core_tgfx_main, BLENDINGs, BlendingsCount);
	blendinginfo_vk** BLENDINGINFOS = (blendinginfo_vk**)BLENDINGs;
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
				const blendinginfo_vk* blendinginfo = BLENDINGINFOS[BlendingInfoIndex];
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

	if (!VKDescSet_PipelineLayoutCreation((const shaderinputdesc_vk**)ShaderInputDescs, &VKPipeline->General_DescSet, &VKPipeline->Instance_DescSet, &VKPipeline->PipelineLayout)) {
		printer(result_tgfx_FAIL, "Link_MaterialType() has failed at VKDescSet_PipelineLayoutCreation!");
		delete VKPipeline;
		return result_tgfx_FAIL;
	}

	VkPipelineDepthStencilStateCreateInfo depth_state = {};
	if (Subpass->SLOTSET->BASESLOTSET->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
		depth_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		if (depthtest) {
			depthsettingsdesc_vk* depthsettings = (depthsettingsdesc_vk*)depthtest;
			depth_state.depthTestEnable = VK_TRUE;
			depth_state.depthCompareOp = depthsettings->DepthCompareOP;
			depth_state.depthWriteEnable = depthsettings->ShouldWrite;
			depth_state.depthBoundsTestEnable = depthsettings->DepthBoundsEnable;
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
			stencildesc_vk* frontfacestencil = (stencildesc_vk*)StencilFrontFaced;
			stencildesc_vk* backfacestencil = (stencildesc_vk*)StencilBackFaced;
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
		if (vkCreateGraphicsPipelines(rendergpu->LOGICALDEVICE(), nullptr, 1, &GraphicsPipelineCreateInfo, nullptr, &VKPipeline->PipelineObject) != VK_SUCCESS) {
			delete VKPipeline;
			delete STAGEs;
			printer(result_tgfx_FAIL, "vkCreateGraphicsPipelines has failed!");
			return result_tgfx_FAIL;
		}
	}

	VKPipeline->GFX_Subpass = Subpass;
	hidden->graphicspipelinetypes.push_back(VKPipeline);


	printer(result_tgfx_SUCCESS, "Finished creating Graphics Pipeline");
	*MaterialHandle = (rasterpipelinetype_tgfx_handle)VKPipeline;
	return result_tgfx_SUCCESS;
}

void  Delete_MaterialType(rasterpipelinetype_tgfx_handle ID) { printer(result_tgfx_NOTCODED, "Delete_MaterialType() isn't implemented!"); }
result_tgfx Create_MaterialInst (rasterpipelinetype_tgfx_handle MaterialType, rasterpipelineinstance_tgfx_handle* MaterialInstHandle){
	graphicspipelinetype_vk* VKPSO = (graphicspipelinetype_vk*)MaterialType;
#ifdef VULKAN_DEBUGGING
	if (!VKPSO) {
		printer(result_tgfx_FAIL, "Create_MaterialInst() has failed because Material Type isn't found!");
		return result_tgfx_INVALIDARGUMENT;
	}
#endif

	//Descriptor Set Creation
	graphicspipelineinst_vk* VKPInstance = new graphicspipelineinst_vk;

	VKPInstance->DescSet.Layout = VKPSO->Instance_DescSet.Layout;
	VKPInstance->DescSet.ShouldRecreate.store(0);
	VKPInstance->DescSet.DescImagesCount = VKPSO->Instance_DescSet.DescImagesCount;
	VKPInstance->DescSet.DescSamplersCount = VKPSO->Instance_DescSet.DescSamplersCount;
	VKPInstance->DescSet.DescSBuffersCount = VKPSO->Instance_DescSet.DescSBuffersCount;
	VKPInstance->DescSet.DescUBuffersCount = VKPSO->Instance_DescSet.DescUBuffersCount;
	VKPInstance->DescSet.DescCount = VKPSO->Instance_DescSet.DescCount;

	if (VKPInstance->DescSet.DescCount) {
		VKPInstance->DescSet.Descs = new descriptor_vk[VKPInstance->DescSet.DescCount];

		for (unsigned int i = 0; i < VKPInstance->DescSet.DescCount; i++) {
			descriptor_vk& desc = VKPInstance->DescSet.Descs[i];
			desc.ElementCount = VKPSO->Instance_DescSet.Descs[i].ElementCount;
			desc.Type = VKPSO->Instance_DescSet.Descs[i].Type;
			switch (desc.Type)
			{
			case desctype_vk::IMAGE:
			case desctype_vk::SAMPLER:
				desc.Elements = new descelement_image_vk[desc.ElementCount];
			case desctype_vk::SBUFFER:
			case desctype_vk::UBUFFER:
				desc.Elements = new descelement_buffer_vk[desc.ElementCount];
			}
		}

		Create_DescSet(&VKPInstance->DescSet);
	}


	VKPInstance->PROGRAM = VKPSO;
	hidden->graphicspipelineinstances.push_back(VKPInstance);
	*MaterialInstHandle = (rasterpipelineinstance_tgfx_handle)VKPInstance;
	return result_tgfx_SUCCESS;
}
void  Delete_MaterialInst (rasterpipelineinstance_tgfx_handle ID){}

result_tgfx Create_ComputeType (shadersource_tgfx_handle Source, shaderinputdescription_tgfx_listhandle ShaderInputDescs, computeshadertype_tgfx_handle* ComputeTypeHandle) {
	VkComputePipelineCreateInfo cp_ci = {};
	shadersource_vk* SHADER = (shadersource_vk*)Source;
	VkShaderModule shader_module = SHADER->Module;

	computetype_vk* VKPipeline = new computetype_vk;

	if (!VKDescSet_PipelineLayoutCreation((const shaderinputdesc_vk**)ShaderInputDescs, &VKPipeline->General_DescSet, &VKPipeline->Instance_DescSet,
		&VKPipeline->PipelineLayout)) {
		printer(result_tgfx_FAIL, "Compile_ComputeType() has failed at VKDescSet_PipelineLayoutCreation!");
		delete VKPipeline;
		return result_tgfx_FAIL;
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
		if (vkCreateComputePipelines(rendergpu->LOGICALDEVICE(), VK_NULL_HANDLE, 1, &cp_ci, nullptr, &VKPipeline->PipelineObject) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "Compile_ComputeShader() has failed at vkCreateComputePipelines!");
			delete VKPipeline;
			return result_tgfx_FAIL;
		}
	}
	vkDestroyShaderModule(rendergpu->LOGICALDEVICE(), shader_module, nullptr);

	*ComputeTypeHandle = (computeshadertype_tgfx_handle)VKPipeline;
	hidden->computetypes.push_back(VKPipeline);
	return result_tgfx_SUCCESS;
}
result_tgfx Create_ComputeInstance(computeshadertype_tgfx_handle ComputeType, computeshaderinstance_tgfx_handle* ComputeShaderInstanceHandle) {
	computetype_vk* VKPipeline = (computetype_vk*)ComputeType;

	computeinstance_vk* instance = new computeinstance_vk;
	instance->PROGRAM = VKPipeline;

	instance->DescSet.Layout = VKPipeline->Instance_DescSet.Layout;
	instance->DescSet.ShouldRecreate.store(0);
	instance->DescSet.DescImagesCount = VKPipeline->Instance_DescSet.DescImagesCount;
	instance->DescSet.DescSamplersCount = VKPipeline->Instance_DescSet.DescSamplersCount;
	instance->DescSet.DescSBuffersCount = VKPipeline->Instance_DescSet.DescSBuffersCount;
	instance->DescSet.DescUBuffersCount = VKPipeline->Instance_DescSet.DescUBuffersCount;
	instance->DescSet.DescCount = VKPipeline->Instance_DescSet.DescCount;

	if (instance->DescSet.DescCount) {
		instance->DescSet.Descs = new descriptor_vk[instance->DescSet.DescCount];

		for (unsigned int i = 0; i < instance->DescSet.DescCount; i++) {
			descriptor_vk& desc = instance->DescSet.Descs[i];
			desc.ElementCount = VKPipeline->Instance_DescSet.Descs[i].ElementCount;
			desc.Type = VKPipeline->Instance_DescSet.Descs[i].Type;
			switch (desc.Type)
			{
			case desctype_vk::IMAGE:
			case desctype_vk::SAMPLER:
				desc.Elements = new descelement_image_vk[desc.ElementCount];
			case desctype_vk::SBUFFER:
			case desctype_vk::UBUFFER:
				desc.Elements = new descelement_buffer_vk[desc.ElementCount];
			}
		}

		if (!Create_DescSet(&instance->DescSet)) {
			printer(result_tgfx_FAIL, "You probably exceed one of the limits you specified at GFX initialization process! Create_ComputeInstance() has failed!");
			delete[] instance->DescSet.Descs;
			delete instance;
			return result_tgfx_FAIL;
		}
	}

	*ComputeShaderInstanceHandle = (computeshaderinstance_tgfx_handle)instance;
	hidden->computeinstances.push_back(instance);
	return result_tgfx_SUCCESS;
}
void  Delete_ComputeShaderType (computeshadertype_tgfx_handle ID){}
void  Delete_ComputeShaderInstance (computeshaderinstance_tgfx_handle ID){}


result_tgfx SetDescSet_Buffer(descset_vk* Set, unsigned int BINDINDEX, bool isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset,
	buffertype_tgfx BUFTYPE, buffer_tgfx_handle BufferHandle, unsigned int BoundDataSize, bool isUsedRecently) {

	if (!Set->DescCount) {
		printer(result_tgfx_FAIL, "Given Material Type/Instance doesn't have any shader input to set!");
		return result_tgfx_FAIL;
	}
	if (BINDINDEX >= Set->DescCount) {
		printer(result_tgfx_FAIL, "BINDINDEX is exceeding the shader input count in the Material Type/Instance");
		return result_tgfx_FAIL;
	}
	descriptor_vk& Descriptor = Set->Descs[BINDINDEX];
	if ((isUniformBufferShaderInput && Descriptor.Type != desctype_vk::UBUFFER) ||
		(!isUniformBufferShaderInput && Descriptor.Type != desctype_vk::SBUFFER)) {
		printer(result_tgfx_FAIL, "BINDINDEX is pointing to some other type of shader input!");
		return result_tgfx_FAIL;
	}
	if (ELEMENTINDEX >= Descriptor.ElementCount) {
		printer(result_tgfx_FAIL, "You gave SetMaterialBuffer() an overflowing ELEMENTINDEX!");
		return result_tgfx_FAIL;
	}

	descelement_buffer_vk& DescElement = ((descelement_buffer_vk*)Descriptor.Elements)[ELEMENTINDEX];

	//Check alignment offset

	VkDeviceSize reqalignmentoffset = (isUniformBufferShaderInput) ? rendergpu->DEVICEPROPERTIES().limits.minUniformBufferOffsetAlignment : rendergpu->DEVICEPROPERTIES().limits.minStorageBufferOffsetAlignment;
	if (TargetOffset % reqalignmentoffset) {
		printer(result_tgfx_WARNING, ("This TargetOffset in SetMaterialBuffer triggers Vulkan Validation Layer, this usage may cause undefined behaviour on this GPU! You should set TargetOffset as a multiple of " + std::to_string(reqalignmentoffset)).c_str());
	}


	unsigned char x = 0;
	if (!DescElement.IsUpdated.compare_exchange_strong(x, 1)) {
		if (x != 255) {
			printer(result_tgfx_FAIL, "You already set shader input buffer, you can't change it at the same frame!");
			return result_tgfx_WRONGTIMING;
		}
		//If value is 255, this means this shader input isn't set before. So try again!
		if (!DescElement.IsUpdated.compare_exchange_strong(x, 1)) {
			printer(result_tgfx_FAIL, "You already set shader input buffer, you can't change it at the same frame!");
			return result_tgfx_WRONGTIMING;
		}
	}
	VkDeviceSize FinalOffset = static_cast<VkDeviceSize>(TargetOffset);
	switch (BUFTYPE) {
	case buffertype_tgfx_STAGING:
	case buffertype_tgfx_GLOBAL:
		FindBufferOBJ_byBufType(BufferHandle, BUFTYPE, DescElement.Info.buffer, FinalOffset);
		break;
	default:
		printer(result_tgfx_NOTCODED, "SetMaterial_UniformBuffer() doesn't support this type of target buffers!");
		return result_tgfx_NOTCODED;
	}
	DescElement.Info.offset = FinalOffset;
	DescElement.Info.range = static_cast<VkDeviceSize>(BoundDataSize);
	DescElement.IsUpdated.store(1);
	descset_updatecall_vk call;
	call.Set = Set;
	call.BindingIndex = BINDINDEX;
	call.ElementIndex = ELEMENTINDEX;
	if (isUsedRecently) {
		call.Set->ShouldRecreate.store(1);
		hidden->DescSets_toCreateUpdate.push_back(call);
	}
	else {
		hidden->DescSets_toJustUpdate.push_back(call);
	}
	return result_tgfx_SUCCESS;
}
result_tgfx SetDescSet_Texture(descset_vk* Set, unsigned int BINDINDEX, bool isSampledTexture, unsigned int ELEMENTINDEX, sampler_vk* Sampler, texture_vk* TextureHandle,
	image_access_tgfx usage, bool isUsedRecently) {
	if (!Set->DescCount) {
		printer(result_tgfx_FAIL, "Given Material Type/Instance doesn't have any shader input to set!");
		return result_tgfx_FAIL;
	}
	if (BINDINDEX >= Set->DescCount) {
		printer(result_tgfx_FAIL, "BINDINDEX is exceeding the shader input count in the Material Type/Instance");
		return result_tgfx_FAIL;
	}
	descriptor_vk& Descriptor = Set->Descs[BINDINDEX];
	if ((isSampledTexture && Descriptor.Type != desctype_vk::SAMPLER) ||
		(!isSampledTexture && Descriptor.Type != desctype_vk::IMAGE)) {
		printer(result_tgfx_FAIL, "BINDINDEX is pointing to some other type of shader input!");
		return result_tgfx_FAIL;
	}
	if (ELEMENTINDEX >= Descriptor.ElementCount) {
		printer(result_tgfx_FAIL, "You gave SetMaterialTexture() an overflowing ELEMENTINDEX!");
		return result_tgfx_FAIL;
	}
	descelement_image_vk& DescElement = ((descelement_image_vk*)Descriptor.Elements)[ELEMENTINDEX];


	unsigned char x = 0;
	if (!DescElement.IsUpdated.compare_exchange_strong(x, 1)) {
		if (x != 255) {
			printer(result_tgfx_FAIL, "You already set shader input texture, you can't change it at the same frame!");
			return result_tgfx_WRONGTIMING;
		}
		//If value is 255, this means this shader input isn't set before. So try again!
		if (!DescElement.IsUpdated.compare_exchange_strong(x, 1)) {
			printer(result_tgfx_FAIL, "You already set shader input texture, you can't change it at the same frame!");
			return result_tgfx_WRONGTIMING;
		}
	}
	VkAccessFlags unused;
	Find_AccessPattern_byIMAGEACCESS(usage, unused, DescElement.info.imageLayout);
	texture_vk* TEXTURE = ((texture_vk*)TextureHandle);
	DescElement.info.imageView = TEXTURE->ImageView;
	DescElement.info.sampler = ((isSampledTexture) ? (Sampler->Sampler) : (nullptr));

	descset_updatecall_vk call;
	call.Set = Set;
	call.BindingIndex = BINDINDEX;
	call.ElementIndex = ELEMENTINDEX;
	if (isUsedRecently) {
		call.Set->ShouldRecreate.store(1);
		hidden->DescSets_toCreateUpdate.push_back(call);
	}
	else {
		hidden->DescSets_toJustUpdate.push_back(call);
	}
	return result_tgfx_SUCCESS;
}



//IsUsedRecently means is the material type/instance used in last frame. This is necessary for Vulkan synchronization process.
result_tgfx SetMaterialType_Buffer (rasterpipelinetype_tgfx_handle MaterialType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	buffer_tgfx_handle BufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize){
	graphicspipelinetype_vk* PSO = (graphicspipelinetype_vk*)MaterialType;
	descset_vk* Set = &PSO->General_DescSet;

	return SetDescSet_Buffer(Set, BINDINDEX, isUniformBufferShaderInput, ELEMENTINDEX, TargetOffset, BUFTYPE, BufferHandle, BoundDataSize, isUsedRecently);
}
result_tgfx SetMaterialType_Texture (rasterpipelinetype_tgfx_handle MaterialType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage){
	graphicspipelinetype_vk* PSO = ((graphicspipelinetype_vk*)MaterialType);
	descset_vk* Set = &PSO->General_DescSet;

	return SetDescSet_Texture(Set, BINDINDEX, isSampledTexture, ELEMENTINDEX, (sampler_vk*)Sampler, (texture_vk*)TextureHandle, usage, isUsedRecently);
}

result_tgfx SetMaterialInst_Buffer (rasterpipelineinstance_tgfx_handle MaterialInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	buffer_tgfx_handle BufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize){
	graphicspipelineinst_vk* PSO = (graphicspipelineinst_vk*)MaterialInstance;
	descset_vk* Set = &PSO->DescSet;

	return SetDescSet_Buffer(Set, BINDINDEX, isUniformBufferShaderInput, ELEMENTINDEX, TargetOffset, BUFTYPE, BufferHandle, BoundDataSize, isUsedRecently);
}
result_tgfx SetMaterialInst_Texture (rasterpipelineinstance_tgfx_handle MaterialInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage){
	graphicspipelineinst_vk* PSO = (graphicspipelineinst_vk*)MaterialInstance;
	descset_vk* Set = &PSO->DescSet;

	return SetDescSet_Texture(Set, BINDINDEX, isSampledTexture, ELEMENTINDEX, (sampler_vk*)Sampler, (texture_vk*)TextureHandle, usage, isUsedRecently);
}
//IsUsedRecently means is the material type/instance used in last frame. This is necessary for Vulkan synchronization process.
result_tgfx SetComputeType_Buffer (computeshadertype_tgfx_handle ComputeType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	buffer_tgfx_handle TargetBufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize){
	computetype_vk* PSO = (computetype_vk*)ComputeType;
	descset_vk* Set = &PSO->General_DescSet;

	return SetDescSet_Buffer(Set, BINDINDEX, isUniformBufferShaderInput, ELEMENTINDEX, TargetOffset, BUFTYPE, TargetBufferHandle, BoundDataSize, isUsedRecently);
}
result_tgfx SetComputeType_Texture (computeshadertype_tgfx_handle ComputeType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage) {
	computetype_vk* PSO = (computetype_vk*)ComputeType;
	descset_vk* Set = &PSO->General_DescSet;

	return SetDescSet_Texture(Set, BINDINDEX, isSampledTexture, ELEMENTINDEX, (sampler_vk*)Sampler, (texture_vk*)TextureHandle, usage, isUsedRecently);
}

result_tgfx SetComputeInst_Buffer (computeshaderinstance_tgfx_handle ComputeInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	buffer_tgfx_handle TargetBufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize){
	computeinstance_vk* PSO = (computeinstance_vk*)ComputeInstance;
	descset_vk* Set = &PSO->DescSet;

	return SetDescSet_Buffer(Set, BINDINDEX, isUniformBufferShaderInput, ELEMENTINDEX, TargetOffset, BUFTYPE, TargetBufferHandle, BoundDataSize, isUsedRecently);
}
result_tgfx SetComputeInst_Texture (computeshaderinstance_tgfx_handle ComputeInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage){
	computeinstance_vk* PSO = (computeinstance_vk*)ComputeInstance;
	descset_vk* Set = &PSO->DescSet;

	return SetDescSet_Texture(Set, BINDINDEX, isSampledTexture, ELEMENTINDEX, (sampler_vk*)Sampler, (texture_vk*)TextureHandle, usage, isUsedRecently);
}


result_tgfx Create_RTSlotset (rtslotdescription_tgfx_listhandle Descriptions, rtslotset_tgfx_handle* RTSlotSetHandle){ 
	TGFXLISTCOUNT(core_tgfx_main, Descriptions, DescriptionsCount);
	for (unsigned int SlotIndex = 0; SlotIndex < DescriptionsCount; SlotIndex++) {
		const rtslotdesc_vk* desc = (rtslotdesc_vk*)Descriptions[SlotIndex];
		texture_vk* FirstHandle = desc->textures[0];
		texture_vk* SecondHandle = desc->textures[1];
		if ((FirstHandle->CHANNELs != SecondHandle->CHANNELs) ||
			(FirstHandle->WIDTH != SecondHandle->WIDTH) ||
			(FirstHandle->HEIGHT != SecondHandle->HEIGHT)
			) {
			printer(result_tgfx_FAIL, "GFXContentManager->Create_RTSlotSet() has failed because one of the slots has texture handles that doesn't match channel type, width or height!");
			return result_tgfx_INVALIDARGUMENT;
		}
		if (!(FirstHandle->USAGE & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) || !(SecondHandle->USAGE & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))) {
			printer(result_tgfx_FAIL, "GFXContentManager->Create_RTSlotSet() has failed because one of the slots has a handle that doesn't use is_RenderableTo in its USAGEFLAG!");
			return result_tgfx_INVALIDARGUMENT;
		}
	}
	unsigned int DEPTHSLOT_VECTORINDEX = UINT32_MAX;
	//Validate the list and find Depth Slot if there is any
	for (unsigned int SlotIndex = 0; SlotIndex < DescriptionsCount; SlotIndex++) {
		const rtslotdesc_vk* desc = (rtslotdesc_vk*)Descriptions[SlotIndex];
		for (unsigned int RTIndex = 0; RTIndex < 2; RTIndex++) {
			texture_vk* RT = (desc->textures[RTIndex]);
			if (!RT) {
				printer(result_tgfx_FAIL, "Create_RTSlotSet() has failed because intended RT isn't found!");
				return result_tgfx_INVALIDARGUMENT;
			}
			if (desc->optype == operationtype_tgfx_UNUSED) {
				printer(result_tgfx_FAIL, "Create_RTSlotSet() has failed because you can't create a Base RT SlotSet that has unused attachment!");
				return result_tgfx_INVALIDARGUMENT;
			}
			if (RT->CHANNELs == texture_channels_tgfx_D24S8 || RT->CHANNELs == texture_channels_tgfx_D32) {
				if (DEPTHSLOT_VECTORINDEX != UINT32_MAX && DEPTHSLOT_VECTORINDEX != SlotIndex) {
					printer(result_tgfx_FAIL, "Create_RTSlotSet() has failed because you can't use two depth buffers at the same slot set!");
					return result_tgfx_INVALIDARGUMENT;
				}
				DEPTHSLOT_VECTORINDEX = SlotIndex;
				continue;
			}
		}
	}
	unsigned char COLORRT_COUNT = (DEPTHSLOT_VECTORINDEX != UINT32_MAX) ? DescriptionsCount - 1 : DescriptionsCount;

	unsigned int FBWIDTH = ((rtslotdesc_vk*)Descriptions[0])->textures[0]->WIDTH;
	unsigned int FBHEIGHT = ((rtslotdesc_vk*)Descriptions[0])->textures[0]->HEIGHT;
	for (unsigned int SlotIndex = 0; SlotIndex < DescriptionsCount; SlotIndex++) {
		texture_vk* Texture = ((rtslotdesc_vk*)Descriptions[SlotIndex])->textures[0];
		if (Texture->WIDTH != FBWIDTH || Texture->HEIGHT != FBHEIGHT) {
			printer(result_tgfx_FAIL, "Create_RTSlotSet() has failed because one of your slot's texture has wrong resolution!");
			return result_tgfx_INVALIDARGUMENT;
		}
	}
	

	rtslotset_vk* VKSLOTSET = new rtslotset_vk;
	for (unsigned int SlotSetIndex = 0; SlotSetIndex < 2; SlotSetIndex++) {
		rtslots_vk& PF_SLOTSET = VKSLOTSET->PERFRAME_SLOTSETs[SlotSetIndex];

		PF_SLOTSET.COLOR_SLOTs = new colorslot_vk[COLORRT_COUNT];
		PF_SLOTSET.COLORSLOTs_COUNT = COLORRT_COUNT;
		if (DEPTHSLOT_VECTORINDEX != UINT32_MAX) {
			PF_SLOTSET.DEPTHSTENCIL_SLOT = new depthstencilslot_vk;
			depthstencilslot_vk* slot = PF_SLOTSET.DEPTHSTENCIL_SLOT;
			const rtslotdesc_vk* DEPTHDESC = (rtslotdesc_vk*)Descriptions[DEPTHSLOT_VECTORINDEX];
			slot->CLEAR_COLOR = glm::vec2(DEPTHDESC->clear_value.x, DEPTHDESC->clear_value.y);
			slot->DEPTH_OPTYPE = DEPTHDESC->optype;
			slot->RT = (DEPTHDESC->textures[SlotSetIndex]);
			slot->STENCIL_OPTYPE = DEPTHDESC->optype;
			slot->IS_USED_LATER = DEPTHDESC->isUsedLater;
			slot->DEPTH_LOAD = DEPTHDESC->loadtype;
			slot->STENCIL_LOAD = DEPTHDESC->loadtype;
		}
		for (unsigned int i = 0; i < DescriptionsCount; i++) {
			if (i == DEPTHSLOT_VECTORINDEX) { continue; }
			unsigned int slotindex = ((i > DEPTHSLOT_VECTORINDEX) ? (i - 1) : (i));
			const rtslotdesc_vk* desc = (rtslotdesc_vk*)Descriptions[i];
			texture_vk* RT = desc->textures[SlotSetIndex];
			colorslot_vk& SLOT = PF_SLOTSET.COLOR_SLOTs[slotindex];
			SLOT.RT_OPERATIONTYPE = desc->optype;
			SLOT.LOADSTATE = desc->loadtype;
			SLOT.RT = RT;
			SLOT.IS_USED_LATER = desc->isUsedLater;
			SLOT.CLEAR_COLOR = glm::vec4(desc->clear_value.x, desc->clear_value.y, desc->clear_value.z, desc->clear_value.w);
		}


		for (unsigned int i = 0; i < PF_SLOTSET.COLORSLOTs_COUNT; i++) {
			texture_vk* VKTexture = PF_SLOTSET.COLOR_SLOTs[i].RT;
			if (!VKTexture->ImageView) {
				printer(result_tgfx_FAIL, "One of your RTs doesn't have a VkImageView! You can't use such a texture as RT. Generally this case happens when you forgot to specify your swapchain texture's usage (while creating a window).");
				return result_tgfx_FAIL;
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

	hidden->rtslotsets.push_back(VKSLOTSET);
	*RTSlotSetHandle = (rtslotset_tgfx_handle)VKSLOTSET;
	return result_tgfx_SUCCESS;
}
void  Delete_RTSlotSet (rtslotset_tgfx_handle RTSlotSetHandle){}
//Changes on RTSlots only happens at the frame slot is gonna be used
//For example; if you change next frame's slot, necessary API calls are gonna be called next frame
//For example; if you change slot but related slotset isn't used by drawpass, it doesn't happen until it is used
result_tgfx Change_RTSlotTexture (rtslotset_tgfx_handle RTSlotHandle, unsigned char isColorRT, unsigned char SlotIndex, unsigned char FrameIndex, texture_tgfx_handle TextureHandle){ return result_tgfx_FAIL; }
result_tgfx Inherite_RTSlotSet (rtslotusage_tgfx_listhandle DescriptionsGFX, rtslotset_tgfx_handle RTSlotSetHandle, inheritedrtslotset_tgfx_handle* InheritedSlotSetHandle){
	if (!RTSlotSetHandle) {
		printer(result_tgfx_FAIL, "Inherite_RTSlotSet() has failed because Handle is invalid!");
		return result_tgfx_INVALIDARGUMENT;
	}
	rtslotset_vk* BaseSet = (rtslotset_vk*)RTSlotSetHandle;
	irtslotset_vk* InheritedSet = new irtslotset_vk;
	InheritedSet->BASESLOTSET = BaseSet;
	rtslotusage_vk** Descriptions = (rtslotusage_vk**)DescriptionsGFX;

	//Find Depth/Stencil Slots and count Color Slots
	bool DEPTH_FOUND = false;
	unsigned char COLORSLOT_COUNT = 0, DEPTHDESC_VECINDEX = 0;
	TGFXLISTCOUNT(core_tgfx_main, Descriptions, DESCCOUNT);
	for (unsigned char i = 0; i < DESCCOUNT; i++) {
		const rtslotusage_vk* DESC = Descriptions[i];
		if (DESC->IS_DEPTH) {
			if (DEPTH_FOUND) {
				printer(result_tgfx_FAIL, "Inherite_RTSlotSet() has failed because there are two depth buffers in the description, which is not supported!");
				delete InheritedSet;
				return result_tgfx_INVALIDARGUMENT;
			}
			DEPTH_FOUND = true;
			DEPTHDESC_VECINDEX = i;
			if (BaseSet->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->DEPTH_OPTYPE == operationtype_tgfx_READ_ONLY &&
				(DESC->OPTYPE == operationtype_tgfx_WRITE_ONLY || DESC->OPTYPE == operationtype_tgfx_READ_AND_WRITE)
				)
			{
				printer(result_tgfx_FAIL, "Inherite_RTSlotSet() has failed because you can't use a Read-Only DepthSlot with Write Access in a Inherited Set!");
				delete InheritedSet;
				return result_tgfx_INVALIDARGUMENT;
			}
			InheritedSet->DEPTH_OPTYPE = DESC->OPTYPE;
			InheritedSet->STENCIL_OPTYPE = DESC->OPTYPESTENCIL;
		}
		else {
			COLORSLOT_COUNT++;
		}
	}
	if (!DEPTH_FOUND) {
		InheritedSet->DEPTH_OPTYPE = operationtype_tgfx_UNUSED;
	}
	if (COLORSLOT_COUNT != BaseSet->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT) {
		printer(result_tgfx_FAIL, "Inherite_RTSlotSet() has failed because BaseSet's Color Slot count doesn't match given Descriptions's one!");
		delete InheritedSet;
		return result_tgfx_INVALIDARGUMENT;
	}


	InheritedSet->COLOR_OPTYPEs = new operationtype_tgfx[COLORSLOT_COUNT];
	//Set OPTYPEs of inherited slotset
	for (unsigned int i = 0; i < COLORSLOT_COUNT; i++) {
		if (i == DEPTHDESC_VECINDEX) {
			continue;
		}
		unsigned char slotindex = ((i > DEPTHDESC_VECINDEX) ? (i - 1) : i);
		
		//FIX: LoadType isn't supported natively while changing subpass, it may be supported by adding a VkCmdPipelineBarrier but don't want to bother with it for now!
		InheritedSet->COLOR_OPTYPEs[slotindex] = Descriptions[i]->OPTYPE;
	}

	*InheritedSlotSetHandle = (inheritedrtslotset_tgfx_handle)InheritedSet;
	hidden->irtslotsets.push_back(InheritedSet);
	return result_tgfx_SUCCESS;
}
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


	//Start glslang
	{
		glslang::InitializeProcess();

		//Initialize limitation table
		//from Eric's Blog "Translate GLSL to SPIRV for Vulkan at Runtime" post: https://lxjk.github.io/2020/03/10/Translate-GLSL-to-SPIRV-for-Vulkan-at-Runtime.html
		glsl_to_spirv_limitationtable.maxLights = 32;
		glsl_to_spirv_limitationtable.maxClipPlanes = 6;
		glsl_to_spirv_limitationtable.maxTextureUnits = 32;
		glsl_to_spirv_limitationtable.maxTextureCoords = 32;
		glsl_to_spirv_limitationtable.maxVertexAttribs = 64;
		glsl_to_spirv_limitationtable.maxVertexUniformComponents = 4096;
		glsl_to_spirv_limitationtable.maxVaryingFloats = 64;
		glsl_to_spirv_limitationtable.maxVertexTextureImageUnits = 32;
		glsl_to_spirv_limitationtable.maxCombinedTextureImageUnits = 80;
		glsl_to_spirv_limitationtable.maxTextureImageUnits = 32;
		glsl_to_spirv_limitationtable.maxFragmentUniformComponents = 4096;
		glsl_to_spirv_limitationtable.maxDrawBuffers = 32;
		glsl_to_spirv_limitationtable.maxVertexUniformVectors = 128;
		glsl_to_spirv_limitationtable.maxVaryingVectors = 8;
		glsl_to_spirv_limitationtable.maxFragmentUniformVectors = 16;
		glsl_to_spirv_limitationtable.maxVertexOutputVectors = 16;
		glsl_to_spirv_limitationtable.maxFragmentInputVectors = 15;
		glsl_to_spirv_limitationtable.minProgramTexelOffset = -8;
		glsl_to_spirv_limitationtable.maxProgramTexelOffset = 7;
		glsl_to_spirv_limitationtable.maxClipDistances = 8;
		glsl_to_spirv_limitationtable.maxComputeWorkGroupCountX = 65535;
		glsl_to_spirv_limitationtable.maxComputeWorkGroupCountY = 65535;
		glsl_to_spirv_limitationtable.maxComputeWorkGroupCountZ = 65535;
		glsl_to_spirv_limitationtable.maxComputeWorkGroupSizeX = 1024;
		glsl_to_spirv_limitationtable.maxComputeWorkGroupSizeY = 1024;
		glsl_to_spirv_limitationtable.maxComputeWorkGroupSizeZ = 64;
		glsl_to_spirv_limitationtable.maxComputeUniformComponents = 1024;
		glsl_to_spirv_limitationtable.maxComputeTextureImageUnits = 16;
		glsl_to_spirv_limitationtable.maxComputeImageUniforms = 8;
		glsl_to_spirv_limitationtable.maxComputeAtomicCounters = 8;
		glsl_to_spirv_limitationtable.maxComputeAtomicCounterBuffers = 1;
		glsl_to_spirv_limitationtable.maxVaryingComponents = 60;
		glsl_to_spirv_limitationtable.maxVertexOutputComponents = 64;
		glsl_to_spirv_limitationtable.maxGeometryInputComponents = 64;
		glsl_to_spirv_limitationtable.maxGeometryOutputComponents = 128;
		glsl_to_spirv_limitationtable.maxFragmentInputComponents = 128;
		glsl_to_spirv_limitationtable.maxImageUnits = 8;
		glsl_to_spirv_limitationtable.maxCombinedImageUnitsAndFragmentOutputs = 8;
		glsl_to_spirv_limitationtable.maxCombinedShaderOutputResources = 8;
		glsl_to_spirv_limitationtable.maxImageSamples = 0;
		glsl_to_spirv_limitationtable.maxVertexImageUniforms = 0;
		glsl_to_spirv_limitationtable.maxTessControlImageUniforms = 0;
		glsl_to_spirv_limitationtable.maxTessEvaluationImageUniforms = 0;
		glsl_to_spirv_limitationtable.maxGeometryImageUniforms = 0;
		glsl_to_spirv_limitationtable.maxFragmentImageUniforms = 8;
		glsl_to_spirv_limitationtable.maxCombinedImageUniforms = 8;
		glsl_to_spirv_limitationtable.maxGeometryTextureImageUnits = 16;
		glsl_to_spirv_limitationtable.maxGeometryOutputVertices = 256;
		glsl_to_spirv_limitationtable.maxGeometryTotalOutputComponents = 1024;
		glsl_to_spirv_limitationtable.maxGeometryUniformComponents = 1024;
		glsl_to_spirv_limitationtable.maxGeometryVaryingComponents = 64;
		glsl_to_spirv_limitationtable.maxTessControlInputComponents = 128;
		glsl_to_spirv_limitationtable.maxTessControlOutputComponents = 128;
		glsl_to_spirv_limitationtable.maxTessControlTextureImageUnits = 16;
		glsl_to_spirv_limitationtable.maxTessControlUniformComponents = 1024;
		glsl_to_spirv_limitationtable.maxTessControlTotalOutputComponents = 4096;
		glsl_to_spirv_limitationtable.maxTessEvaluationInputComponents = 128;
		glsl_to_spirv_limitationtable.maxTessEvaluationOutputComponents = 128;
		glsl_to_spirv_limitationtable.maxTessEvaluationTextureImageUnits = 16;
		glsl_to_spirv_limitationtable.maxTessEvaluationUniformComponents = 1024;
		glsl_to_spirv_limitationtable.maxTessPatchComponents = 120;
		glsl_to_spirv_limitationtable.maxPatchVertices = 32;
		glsl_to_spirv_limitationtable.maxTessGenLevel = 64;
		glsl_to_spirv_limitationtable.maxViewports = 16;
		glsl_to_spirv_limitationtable.maxVertexAtomicCounters = 0;
		glsl_to_spirv_limitationtable.maxTessControlAtomicCounters = 0;
		glsl_to_spirv_limitationtable.maxTessEvaluationAtomicCounters = 0;
		glsl_to_spirv_limitationtable.maxGeometryAtomicCounters = 0;
		glsl_to_spirv_limitationtable.maxFragmentAtomicCounters = 8;
		glsl_to_spirv_limitationtable.maxCombinedAtomicCounters = 8;
		glsl_to_spirv_limitationtable.maxAtomicCounterBindings = 1;
		glsl_to_spirv_limitationtable.maxVertexAtomicCounterBuffers = 0;
		glsl_to_spirv_limitationtable.maxTessControlAtomicCounterBuffers = 0;
		glsl_to_spirv_limitationtable.maxTessEvaluationAtomicCounterBuffers = 0;
		glsl_to_spirv_limitationtable.maxGeometryAtomicCounterBuffers = 0;
		glsl_to_spirv_limitationtable.maxFragmentAtomicCounterBuffers = 1;
		glsl_to_spirv_limitationtable.maxCombinedAtomicCounterBuffers = 1;
		glsl_to_spirv_limitationtable.maxAtomicCounterBufferSize = 16384;
		glsl_to_spirv_limitationtable.maxTransformFeedbackBuffers = 4;
		glsl_to_spirv_limitationtable.maxTransformFeedbackInterleavedComponents = 64;
		glsl_to_spirv_limitationtable.maxCullDistances = 8;
		glsl_to_spirv_limitationtable.maxCombinedClipAndCullDistances = 8;
		glsl_to_spirv_limitationtable.maxSamples = 4;
		glsl_to_spirv_limitationtable.maxMeshOutputVerticesNV = 256;
		glsl_to_spirv_limitationtable.maxMeshOutputPrimitivesNV = 512;
		glsl_to_spirv_limitationtable.maxMeshWorkGroupSizeX_NV = 32;
		glsl_to_spirv_limitationtable.maxMeshWorkGroupSizeY_NV = 1;
		glsl_to_spirv_limitationtable.maxMeshWorkGroupSizeZ_NV = 1;
		glsl_to_spirv_limitationtable.maxTaskWorkGroupSizeX_NV = 32;
		glsl_to_spirv_limitationtable.maxTaskWorkGroupSizeY_NV = 1;
		glsl_to_spirv_limitationtable.maxTaskWorkGroupSizeZ_NV = 1;
		glsl_to_spirv_limitationtable.maxMeshViewCountNV = 4;
		glsl_to_spirv_limitationtable.limits.nonInductiveForLoops = 1;
		glsl_to_spirv_limitationtable.limits.whileLoops = 1;
		glsl_to_spirv_limitationtable.limits.doWhileLoops = 1;
		glsl_to_spirv_limitationtable.limits.generalUniformIndexing = 1;
		glsl_to_spirv_limitationtable.limits.generalAttributeMatrixVectorIndexing = 1;
		glsl_to_spirv_limitationtable.limits.generalVaryingIndexing = 1;
		glsl_to_spirv_limitationtable.limits.generalSamplerIndexing = 1;
		glsl_to_spirv_limitationtable.limits.generalVariableIndexing = 1;
		glsl_to_spirv_limitationtable.limits.generalConstantMatrixVectorIndexing = 1;

	}


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
				TextureBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			}
			else {
				TextureBinding[0].descriptorCount = SAMPLER_COUNT;
				TextureBinding[1].descriptorCount = IMAGE_COUNT;
				TextureBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
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