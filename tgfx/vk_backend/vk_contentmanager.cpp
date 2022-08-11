#include "vk_contentmanager.h"
#include <tgfx_core.h>
#include <tgfx_gpucontentmanager.h>
#include "vk_predefinitions.h"
#include "vk_extension.h"
#include "vk_core.h"
#include "vk_includes.h"
#include "vk_renderer.h"
#include <mutex>
#include "vk_resource.h"
#include "vk_memory.h"
#include "vk_helper.h"
#include <glslang/SPIRV/GlslangToSpv.h>

TBuiltInResource glsl_to_spirv_limitationtable;

struct descpool_vk {
	// Each pool stores a descset of the same descsetlayout (So for each desccsetlayout, there are B.I.F. + 1 descsets across all pools)
	// B.I.F. + 1 pools TGFX handles descriptors before waiting for N-2th frame
	// So we can handle descriptor updates while waiting for N-2th frame's command buffers to end
	VkDescriptorPool POOLs[VKCONST_DESCPOOLCOUNT_PERDESCTYPE] = {};
	std::atomic<uint32_t> REMAINING_DESCs = 0, REMAINING_SETs = 0;
};
struct SHADERSOURCE_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::SHADERSOURCE;
	static uint16_t GET_EXTRAFLAGS(SHADERSOURCE_VKOBJ* obj) { return obj->stage; }
	void* operator new(size_t size) = delete;
	void operator = (const SHADERSOURCE_VKOBJ& copyFrom) {
		isALIVE.store(true);
		Module = copyFrom.Module;  stage = copyFrom.stage;
		SOURCE_CODE = copyFrom.SOURCE_CODE; DATA_SIZE = copyFrom.DATA_SIZE;
	}
	VkShaderModule Module;
	shaderstage_tgfx stage;
	void* SOURCE_CODE = nullptr;
	unsigned int DATA_SIZE = 0;
};


struct gpudatamanager_private {
	VK_LINEAR_OBJARRAY<RTSLOTSET_VKOBJ, 1 << 10> rtslotsets;
	VK_LINEAR_OBJARRAY<TEXTURE_VKOBJ, 1 << 24> textures;
	VK_LINEAR_OBJARRAY<IRTSLOTSET_VKOBJ, 1 << 10> irtslotsets;
	VK_LINEAR_OBJARRAY<GRAPHICSPIPELINETYPE_VKOBJ, 1 << 24> pipelinetypes;
	VK_LINEAR_OBJARRAY<GRAPHICSPIPELINEINST_VKOBJ, 1 << 24> pipelineinstances;
	VK_LINEAR_OBJARRAY<VERTEXATTRIBLAYOUT_VKOBJ, 1 << 10> vertexattributelayouts;
	VK_LINEAR_OBJARRAY<SHADERSOURCE_VKOBJ, 1 << 24> shadersources;
	VK_LINEAR_OBJARRAY<VERTEXBUFFER_VKOBJ, 1 << 24> vertexbuffers;
	VK_LINEAR_OBJARRAY<SAMPLER_VKOBJ, 1 << 16> samplers;
	VK_LINEAR_OBJARRAY<COMPUTETYPE_VKOBJ> computetypes;
	VK_LINEAR_OBJARRAY<COMPUTEINST_VKOBJ> computeinstances;
	VK_LINEAR_OBJARRAY<GLOBALSSBO_VKOBJ> globalbuffers;
	VK_LINEAR_OBJARRAY<INDEXBUFFER_VKOBJ> indexbuffers;
	VK_LINEAR_OBJARRAY<DESCSET_VKOBJ, 1 << 16> descsets;
	

	
	//All descs allocated from these pools
	//Each desc type has its own pool, that's why array is VKCONST_DYNAMICDESCRIPTORTYPESCOUNT size
	descpool_vk ALL_DESCPOOLS[VKCONST_DYNAMICDESCRIPTORTYPESCOUNT];
	

	//These are the textures that will be deleted after waiting for 2 frames ago's command buffer
	VK_VECTOR_ADDONLY<TEXTURE_VKOBJ*, 1 << 16> DeleteTextureList;
	//These are the texture that will be added to the list above after clearing the above list
	VK_VECTOR_ADDONLY<TEXTURE_VKOBJ*, 1 << 16> NextFrameDeleteTextureCalls;

	gpudatamanager_private() {}
};
static gpudatamanager_private* hidden = nullptr;

void Update_DescSet(DESCSET_VKOBJ& set) {
	//If the desc set is a deleted one, pass this
	if (set.Layout == VK_NULL_HANDLE) { return; }
	//DescSets aren't created, so it should be created instead of updating
	if (set.Sets[0] = VK_NULL_HANDLE) { return; }
	//Swap places of DescSets. Moving 0th to 1st, 1st to 2nd, last one to 0th
	VkDescriptorSet last_DescSet = set.Sets[VKCONST_BUFFERING_IN_FLIGHT];
	for (unsigned int i = 0; i < VKCONST_BUFFERING_IN_FLIGHT - 1; i++) {
		set.Sets[i + 1] = set.Sets[i];
	}
	set.Sets[0] = last_DescSet;

	//Update 0th descriptor set
	if(set.isUpdatedCounter.load()) {
		std::vector<VkWriteDescriptorSet> UpdateInfos;

		if 
		(
			set.DESCTYPE == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
			|| set.DESCTYPE == set.DESCTYPE == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
			|| set.DESCTYPE == VK_DESCRIPTOR_TYPE_SAMPLER
		) {
			for 
			(
				unsigned int elementIndx = 0;
				elementIndx < set.textureDescELEMENTs.size();
				elementIndx++
			) {
				texture_descVK& im = set.textureDescELEMENTs[elementIndx];
				if (im.isUpdated.load()) {
					im.isUpdated.store(0);

					VkWriteDescriptorSet UpdateInfo = {};
					UpdateInfo.descriptorCount = 1;
					UpdateInfo.descriptorType = set.DESCTYPE;
					UpdateInfo.dstArrayElement = elementIndx;
					UpdateInfo.dstBinding = 0;
					UpdateInfo.dstSet = set.Sets[0];
					UpdateInfo.pImageInfo = &im.info;
					UpdateInfo.pNext = nullptr;
					UpdateInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					UpdateInfos.push_back(UpdateInfo);
				}
			}
		}
		else {
			for 
			(
				unsigned int DescElementIndex = 0;
				DescElementIndex < set.bufferDescELEMENTs.size();
				DescElementIndex++
			) {
				buffer_descVK& buf = set.bufferDescELEMENTs[DescElementIndex];
				if (buf.isUpdated.load()) {
					buf.isUpdated.store(0);

					VkWriteDescriptorSet UpdateInfo = {};
					UpdateInfo.descriptorCount = 1;
					UpdateInfo.descriptorType = set.DESCTYPE;
					UpdateInfo.dstArrayElement = DescElementIndex;
					UpdateInfo.dstBinding = 0;
					UpdateInfo.dstSet = set.Sets[0];
					UpdateInfo.pBufferInfo = &buf.info;
					UpdateInfo.pNext = nullptr;
					UpdateInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					UpdateInfos.push_back(UpdateInfo);
				}
			}
		}

		vkUpdateDescriptorSets
		(
			rendergpu->LOGICALDEVICE(),
			UpdateInfos.size(),
			UpdateInfos.data(),
			0,
			nullptr
		);
		set.isUpdatedCounter.fetch_sub(1);
	}
}
void gpudatamanager_public::Apply_ResourceChanges() {
	const unsigned char FrameIndex = renderer->Get_FrameIndex(false);

	for (uint32_t i = 0; i < hidden->descsets.size(); i++) {
		Update_DescSet(*hidden->descsets.getOBJbyINDEX(i));
	}
	//Create Desc Sets for binding tables that are created this frame
	for (uint32_t id = 0; id < VKCONST_DYNAMICDESCRIPTORTYPESCOUNT; id++) {
		// TODO JOBSYS: This loop is enough to divide into multiple jobs (so 3 threads will create all necessary descsets)
		for						// Allocate same desc set in each different desc pool
		(
			uint32_t pool_i = 0;
			pool_i < VKCONST_DESCPOOLCOUNT_PERDESCTYPE;
			pool_i++
		) {
			std::vector<VkDescriptorSet> Sets;
			std::vector<VkDescriptorSet*> SetPTRs;
			std::vector<VkDescriptorSetLayout> SetLayouts;


			VkDescriptorSetAllocateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			info.descriptorPool = hidden->ALL_DESCPOOLS[id].POOLs[pool_i];
			info.pNext = nullptr;

			// Put desc set to a vector to batch all allocations to same descpool in one call
			for (unsigned int set_i = 0; set_i < hidden->descsets.size(); set_i++) {			
				DESCSET_VKOBJ* currentSet = hidden->descsets.getOBJbyINDEX(set_i);
				if (DESCSET_VKOBJ::GET_EXTRAFLAGS(currentSet) != id) { continue; }
				Sets.push_back(VkDescriptorSet());
				SetLayouts.push_back(currentSet->Layout);
				SetPTRs.push_back(&currentSet->Sets[pool_i]);
			}

			info.descriptorSetCount = Sets.size();
			info.pSetLayouts = SetLayouts.data();
			if (!Sets.size()) { break; }

			vkAllocateDescriptorSets(rendergpu->LOGICALDEVICE(), &info, Sets.data());
			for (unsigned int SetIndex = 0; SetIndex < Sets.size(); SetIndex++) {
				*SetPTRs[SetIndex] = Sets[SetIndex];
			}
		}
	}


	//Re-create VkFramebuffers etc.
	renderer->RendererResource_Finalizations();

	//Delete textures
	for (unsigned int i = 0; i < hidden->DeleteTextureList.size(); i++) {
		TEXTURE_VKOBJ* Texture = hidden->DeleteTextureList[i];
		if (Texture->Image) {
			vkDestroyImageView
			(
				rendergpu->LOGICALDEVICE(),
				Texture->ImageView,
				nullptr
			);
			vkDestroyImage(rendergpu->LOGICALDEVICE(), Texture->Image, nullptr);
		}
		if (Texture->Block.MemAllocIndex != UINT32_MAX) {
			gpu_allocator->free_memoryblock
			(
				rendergpu->MEMORYTYPE_IDS()[Texture->Block.MemAllocIndex],
				Texture->Block.Offset
			);
		}
		hidden->textures.destroyOBJfromHANDLE
		(
			hidden->textures.returnHANDLEfromOBJ(Texture)
		);
	}
	hidden->DeleteTextureList.clear();
	//Push next frame delete texture list to the delete textures list
	for (uint32_t i = 0; i < hidden->NextFrameDeleteTextureCalls.size(); i++) {
		hidden->DeleteTextureList.push_back(hidden->NextFrameDeleteTextureCalls[i]);
	}
	hidden->NextFrameDeleteTextureCalls.clear();
	
	if(threadcount > 1){
		assert(false && "gpudatamanager_public::Apply_ResourceChanges() should support multi-threading!");
	}
}

void gpudatamanager_public::Resource_Finalizations() {

}


void  Destroy_AllResources (){}


extern void FindBufferOBJ_byBufType
(
	const buffer_tgfxhnd Handle,
	VkBuffer& TargetBuffer,
	VkDeviceSize& TargetOffset
) {
	VKOBJHANDLE handle = *(VKOBJHANDLE*)&Handle;
	switch (handle.type) {
	case VKHANDLETYPEs::GLOBALSSBO:
	{
		GLOBALSSBO_VKOBJ* buffer = (GLOBALSSBO_VKOBJ*)Handle;
		TargetBuffer = gpu_allocator->get_memorybufferhandle_byID
		(
			rendergpu, buffer->Block.MemAllocIndex
		);
		TargetOffset += buffer->Block.Offset;
	}
		break;
	default:
		TargetBuffer = VK_NULL_HANDLE;
		TargetOffset = UINT64_MAX;
		printer(result_tgfx_FAIL, "FindBufferOBJ_byBufType() doesn't support this type!");
		break;
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

	ThrowIfFailed
	(
		vkCreateBuffer(rendergpu->LOGICALDEVICE(), &ci, nullptr, &buffer),
		"Create_VkBuffer has failed!"
	);
	return buffer;
}
result_tgfx Create_SamplingType
(
	unsigned int MinimumMipLevel, unsigned int MaximumMipLevel,
	texture_mipmapfilter_tgfx MINFILTER, texture_mipmapfilter_tgfx MAGFILTER,
	texture_wrapping_tgfx WRAPPING_WIDTH, texture_wrapping_tgfx WRAPPING_HEIGHT,
	texture_wrapping_tgfx WRAPPING_DEPTH, uvec4_tgfx bordercolor,
	samplingType_tgfxhnd* SamplingTypeHandle
) {
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

	VkSampler sampler;
	ThrowIfFailed
	(
		vkCreateSampler(rendergpu->LOGICALDEVICE(), &s_ci, nullptr, &sampler),
		"GFXContentManager->Create_SamplingType() has failed at vkCreateSampler!"
	);
	SAMPLER_VKOBJ* SAMPLER = hidden->samplers.create_OBJ();
	SAMPLER->Sampler = sampler;
	VKOBJHANDLE handle = hidden->samplers.returnHANDLEfromOBJ(SAMPLER);;
	*SamplingTypeHandle = *(samplingType_tgfxhnd*)&handle;
	return result_tgfx_SUCCESS;
}

/*Attributes are ordered as the same order of input vector
* For example: Same attribute ID may have different location/order in another attribute layout
* So you should gather your vertex buffer data according to that
*/

inline unsigned int Calculate_sizeofVertexLayout
(
	const datatype_tgfx* ATTRIBUTEs, unsigned int count
) {
	unsigned int size = 0;
	for (unsigned int i = 0; i < count; i++) {
		size += get_uniformtypes_sizeinbytes(ATTRIBUTEs[i]);
	}
	return size;
}
result_tgfx Create_VertexAttributeLayout
(
	const datatype_tgfx* Attributes, vertexlisttypes_tgfx listtype,
	vertexAttributeLayout_tgfxhnd* VertexAttributeLayoutHandle
){
	VERTEXATTRIBLAYOUT_VKOBJ* lay = hidden->vertexattributelayouts.create_OBJ();
	unsigned int AttributesCount = 0;
	while (Attributes[AttributesCount] != datatype_tgfx_UNDEFINED) {
		AttributesCount++;
	}
	lay->AttribCount = AttributesCount;
	lay->Attribs = new datatype_tgfx[lay->AttribCount];
	for (unsigned int i = 0; i < lay->AttribCount; i++) {
		lay->Attribs[i] = Attributes[i];
	}
	unsigned int size_pervertex = Calculate_sizeofVertexLayout
	(
		lay->Attribs, lay->AttribCount
	);
	lay->size_perVertex = size_pervertex;
	lay->BindingDesc.binding = 0;
	lay->BindingDesc.stride = size_pervertex;
	lay->BindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	lay->PrimitiveTopology = Find_PrimitiveTopology_byGFXVertexListType(listtype);

	lay->AttribDescs = new VkVertexInputAttributeDescription[lay->AttribCount];
	lay->AttribDesc_Count = lay->AttribCount;
	unsigned int stride_ofcurrentattribute = 0;
	for (unsigned int i = 0; i < lay->AttribCount; i++) {
		lay->AttribDescs[i].binding = 0;
		lay->AttribDescs[i].location = i;
		lay->AttribDescs[i].offset = stride_ofcurrentattribute;
		lay->AttribDescs[i].format = Find_VkFormat_byDataType(lay->Attribs[i]);
		stride_ofcurrentattribute += get_uniformtypes_sizeinbytes(lay->Attribs[i]);
	}

	VKOBJHANDLE handle = hidden->vertexattributelayouts.returnHANDLEfromOBJ(lay);
	*VertexAttributeLayoutHandle = *(vertexAttributeLayout_tgfxhnd*)&handle;
	return result_tgfx_SUCCESS;
}
void Delete_VertexAttributeLayout
(vertexAttributeLayout_tgfxhnd VertexAttributeLayoutHandle){}

result_tgfx Upload_toBuffer
(
	buffer_tgfxhnd Handle,
	const void* DATA, unsigned int DATA_SIZE, unsigned int OFFSET
) {
	VkDeviceSize UploadOFFSET = static_cast<VkDeviceSize>(OFFSET);
	unsigned int MEMALLOCINDEX = UINT32_MAX;
	VKOBJHANDLE objhandle = *(VKOBJHANDLE*)&Handle;
	switch (objhandle.type) {
	case VKHANDLETYPEs::VERTEXBUFFER:
	{
		VERTEXBUFFER_VKOBJ* vb = hidden->vertexbuffers.getOBJfromHANDLE(objhandle);
		if (!vb) { return result_tgfx_FAIL; }
		MEMALLOCINDEX = vb->Block.MemAllocIndex;
		UploadOFFSET += vb->Block.Offset; 
	}
		break;
	case VKHANDLETYPEs::INDEXBUFFER:
	{
		INDEXBUFFER_VKOBJ* ib = hidden->indexbuffers.getOBJfromHANDLE(objhandle);
		if (!ib) { return result_tgfx_FAIL; }
		MEMALLOCINDEX = ib->Block.MemAllocIndex;
		UploadOFFSET += ib->Block.Offset;
	}
		break;
	case VKHANDLETYPEs::GLOBALSSBO:
	{
		GLOBALSSBO_VKOBJ* gb = hidden->globalbuffers.getOBJfromHANDLE(objhandle);
		MEMALLOCINDEX = gb->Block.MemAllocIndex;
		UploadOFFSET += gb->Block.Offset;
	}
		break;
	default:
		printer(result_tgfx_FAIL, "This type isn't supported for now by Upload_toBuffer()!");
		break;
	}

	return gpu_allocator->copy_to_mappedmemory
						(rendergpu, MEMALLOCINDEX, DATA, DATA_SIZE, UploadOFFSET);
}

/*
* You should sort your vertex data according to attribute layout, don't forget that
* VertexCount shouldn't be 0
*/
result_tgfx Create_VertexBuffer
(
	vertexAttributeLayout_tgfxhnd vertexAttribLayoutHnd, unsigned int VertexCount,
	memoryAllocator_tgfxlsthnd allocatorList, buffer_tgfxhnd* VertexBufferHandle
) {
	if (!VertexCount) {
		printer(result_tgfx_FAIL, "GFXContentManager->Create_MeshBuffer() has failed because vertex_count is zero!");
		return result_tgfx_INVALIDARGUMENT;
	}

	VERTEXATTRIBLAYOUT_VKOBJ* Layout =
		hidden->vertexattributelayouts.getOBJfromHANDLE
		(*(VKOBJHANDLE*)&vertexAttribLayoutHnd);
	if (!Layout) {
		printer(result_tgfx_FAIL, "GFXContentManager->Create_MeshBuffer() has failed because Attribute Layout ID is invalid!");
		return result_tgfx_INVALIDARGUMENT;
	}

	unsigned int TOTALDATA_SIZE = Layout->size_perVertex * VertexCount;

	VERTEXBUFFER_VKOBJ VKMesh;
	VKMesh.Block.MemAllocIndex = MemoryTypeIndex;
	VKMesh.Layout = hidden->vertexattributelayouts.getINDEXbyOBJ(Layout);
	VKMesh.VERTEX_COUNT = VertexCount;

	VkBufferUsageFlags BufferUsageFlag =
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	VkBuffer Buffer = Create_VkBuffer(TOTALDATA_SIZE, BufferUsageFlag);
	if (gpu_allocator->suballocate_buffer(Buffer, BufferUsageFlag, VKMesh.Block) != result_tgfx_SUCCESS) {
		printer(result_tgfx_FAIL, "There is no memory left in specified memory region!");
		vkDestroyBuffer(rendergpu->LOGICALDEVICE(), Buffer, nullptr);
		return result_tgfx_FAIL;
	}
	vkDestroyBuffer(rendergpu->LOGICALDEVICE(), Buffer, nullptr);

	VERTEXBUFFER_VKOBJ* vb = hidden->vertexbuffers.create_OBJ();
	*vb = VKMesh;
	VKOBJHANDLE handle = hidden->vertexbuffers.returnHANDLEfromOBJ(vb);
	*VertexBufferHandle = *(buffer_tgfxhnd*)&handle;
	return result_tgfx_SUCCESS;
}
void  Unload_VertexBuffer (buffer_tgfxhnd BufferHandle){}

result_tgfx Create_IndexBuffer
(
	datatype_tgfx type, uint32_t IndxCount, memoryAllocator_tgfxlsthnd AllocList,
	buffer_tgfxhnd* IndexBufferHandle
) {
	VkIndexType IndexType = Find_IndexType_byGFXDATATYPE(type);
	if (IndexType == VK_INDEX_TYPE_MAX_ENUM) {
		printer(result_tgfx_FAIL, "GFXContentManager->Create_IndexBuffer() has failed because DataType isn't supported!");
		return result_tgfx_INVALIDARGUMENT;
	}
	if (!IndxCount) {
		printer(result_tgfx_FAIL, "GFXContentManager->Create_IndexBuffer() has failed because IndexCount is zero!");
		return result_tgfx_INVALIDARGUMENT;
	}
	INDEXBUFFER_VKOBJ IB;
	IB.DATATYPE = IndexType;

	VkBufferUsageFlags BufferUsageFlag =
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	VkBuffer Buffer = Create_VkBuffer
	(
		get_uniformtypes_sizeinbytes(type) * IndxCount, BufferUsageFlag
	);
	IB.Block.MemAllocIndex = MemoryTypeIndex;
	IB.IndexCount = static_cast<VkDeviceSize>(IndxCount);
	if 
	(
		gpu_allocator->suballocate_buffer(Buffer, BufferUsageFlag, IB.Block) !=
		result_tgfx_SUCCESS
	) {
		printer(result_tgfx_FAIL, "There is no memory left in specified memory region!");
		vkDestroyBuffer(rendergpu->LOGICALDEVICE(), Buffer, nullptr);
		return result_tgfx_FAIL;
	}
	vkDestroyBuffer(rendergpu->LOGICALDEVICE(), Buffer, nullptr);

	INDEXBUFFER_VKOBJ* finalobj = hidden->indexbuffers.create_OBJ();
	*finalobj = IB;

	VKOBJHANDLE handle = hidden->indexbuffers.returnHANDLEfromOBJ(finalobj);
	*IndexBufferHandle = *(buffer_tgfxhnd*)&handle;
	return result_tgfx_SUCCESS;
}
void  Unload_IndexBuffer (buffer_tgfxhnd BufferHandle){}

result_tgfx Create_Texture
(
	texture_dimensions_tgfx DIMENSION, unsigned int WIDTH, unsigned int HEIGHT,
	textureChannels_tgfx channelType, uint8_t MIPCOUNT,
	textureUsageFlag_tgfxhnd usage, textureOrder_tgfx DATAORDER,
	memoryAllocator_tgfxlsthnd allocList, texture_tgfxhnd* TextureHandle
) {
	if
	(
		MIPCOUNT > std::floor(std::log2(std::max(WIDTH, HEIGHT))) + 1 ||
		!MIPCOUNT
	) {
		printer
		(
			result_tgfx_FAIL, "GFXContentManager->Create_Texture() has failed "
			"because mip count of the texture is wrong!"
		);
		return result_tgfx_FAIL;
	}
	TEXTURE_VKOBJ texture;
	texture.CHANNELs = channelType;
	texture.HEIGHT = HEIGHT;
	texture.WIDTH = WIDTH;
	texture.DATA_SIZE = WIDTH * HEIGHT * GetByteSizeOf_TextureChannels(channelType);
	VkImageUsageFlags flag = 0;
	if (VKCONST_isPointerContainVKFLAG) { flag = *(VkImageUsageFlags*)&usage; }
	else { flag = *(VkImageUsageFlags*)usage; }
	if
	(
		texture.CHANNELs == texture_channels_tgfx_D24S8 ||
		texture.CHANNELs == texture_channels_tgfx_D32
	) { flag &= ~(1UL << 4); }
	else { flag &= ~(1UL << 5); }
	texture.USAGE = flag;
	texture.Block.MemAllocIndex = MemoryTypeIndex;
	texture.DIMENSION = DIMENSION;
	texture.MIPCOUNT = MIPCOUNT;


	//Create VkImage
	{
		VkImageCreateInfo im_ci = {};
		im_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		im_ci.extent.width = texture.WIDTH;
		im_ci.extent.height = texture.HEIGHT;
		im_ci.extent.depth = 1;
		if (DIMENSION == texture_dimensions_tgfx_2DCUBE) {
			im_ci.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			im_ci.arrayLayers = 6;
		}
		else {
			im_ci.flags = 0;
			im_ci.arrayLayers = 1;
		}
		im_ci.format = Find_VkFormat_byTEXTURECHANNELs(texture.CHANNELs);
		im_ci.imageType = Find_VkImageType(DIMENSION);
		im_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		im_ci.mipLevels = static_cast<uint32_t>(MIPCOUNT);
		im_ci.pNext = nullptr;

		if (rendergpu->QUEUEFAMSCOUNT() > 1) 
		{ im_ci.sharingMode = VK_SHARING_MODE_CONCURRENT; }
		else { im_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE; }

		im_ci.pQueueFamilyIndices = rendergpu->ALLQUEUEFAMILIES();
		im_ci.queueFamilyIndexCount = rendergpu->QUEUEFAMSCOUNT();
		im_ci.tiling = Find_VkTiling(DATAORDER);
		im_ci.usage = texture.USAGE;
		im_ci.samples = VK_SAMPLE_COUNT_1_BIT;

		ThrowIfFailed
		(
			vkCreateImage(rendergpu->LOGICALDEVICE(), &im_ci, nullptr, &texture.Image)
			, "GFXContentManager->Create_Texture() has failed in vkCreateImage()!"
		);

		if (gpu_allocator->suballocate_image(&texture) != result_tgfx_SUCCESS) {
			printer(result_tgfx_FAIL, "Suballocation of the texture has failed! Please re-create later.");
			return result_tgfx_FAIL;
		}
	}

	//Create Image View
	{
		VkImageViewCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.flags = 0;
		ci.pNext = nullptr;

		ci.image = texture.Image;
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
		ci.format = Find_VkFormat_byTEXTURECHANNELs(texture.CHANNELs);
		if (texture.CHANNELs == texture_channels_tgfx_D32) {
			ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if (texture.CHANNELs == texture_channels_tgfx_D24S8) {
			ci.subresourceRange.aspectMask =
				VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else {
			ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		Fill_ComponentMapping_byCHANNELs(texture.CHANNELs, ci.components);

		ThrowIfFailed
		(
			vkCreateImageView
			(
				rendergpu->LOGICALDEVICE(), &ci, nullptr, &texture.ImageView
			),
			"GFXContentManager->Upload_Texture() has failed in vkCreateImageView()!"
		);
	}

	TEXTURE_VKOBJ* obj = contentmanager->GETTEXTURES_ARRAY().create_OBJ();
	*obj = texture;
	VKOBJHANDLE handle = contentmanager->GETTEXTURES_ARRAY().returnHANDLEfromOBJ(obj);
	*TextureHandle = *(texture_tgfxhnd*)&handle;
	return result_tgfx_SUCCESS;
}
//TARGET OFFSET is the offset in the texture's buffer to copy to
result_tgfx Upload_Texture
(
	texture_tgfxhnd TextureHandle,
	const void* InputData, unsigned int DataSize, unsigned int TargetOffset
)
{ return result_tgfx_FAIL; }

result_tgfx Create_GlobalBuffer
(
	const char* name, unsigned int size, memoryAllocator_tgfxlsthnd allocatorList,
	extension_tgfxlsthnd exts, buffer_tgfxhnd* GlobalBufferHandle
) {
	VkBuffer obj;
	VkBufferUsageFlags flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	obj = Create_VkBuffer(size, flags);


	GLOBALSSBO_VKOBJ GB;
	GB.Block.MemAllocIndex = MemoryTypeIndex;
	GB.DATA_SIZE = size;
	if (gpu_allocator->suballocate_buffer(obj, flags, GB.Block) != result_tgfx_SUCCESS) {
		printer(result_tgfx_FAIL, "Create_GlobalBuffer has failed at suballocation!");
		return result_tgfx_FAIL;
	}
	vkDestroyBuffer(rendergpu->LOGICALDEVICE(), obj, nullptr);

	GLOBALSSBO_VKOBJ* finalgb = hidden->globalbuffers.create_OBJ();
	*finalgb = GB;
	VKOBJHANDLE handle = hidden->globalbuffers.returnHANDLEfromOBJ(finalgb);
	*GlobalBufferHandle = *(buffer_tgfxhnd*)&handle;
	return result_tgfx_SUCCESS;
}


result_tgfx Create_BindingTable(shaderdescriptortype_tgfx DescriptorType, unsigned int ElementCount,
	shaderStageFlag_tgfxhnd VisbleStags, samplingType_tgfxlsthnd StaticSamplers, bindingTable_tgfxhnd* bindingTableHandle) {
#ifdef VULKAN_DEBUGGING
	//Check if pool has enough space for the desc set and if there is, then decrease the amount of available descs in the pool for other checks
	if (!ElementCount) { printer(result_tgfx_FAIL, "You shouldn't create a binding table with ElementCount = 0"); return result_tgfx_FAIL;}
	unsigned int descsetid = Find_VKCONST_DESCSETID_byTGFXDescType(DescriptorType);
	if (hidden->ALL_DESCPOOLS[descsetid].REMAINING_DESCs.fetch_sub(ElementCount) < ElementCount ||
		!hidden->ALL_DESCPOOLS[descsetid].REMAINING_DESCs.fetch_sub(1)) {
		printer(result_tgfx_FAIL, "Create_DescSets() has failed because descriptor pool doesn't have enough space! You probably exceeded one of the limits you've set before");
		return result_tgfx_FAIL;
	}
	if (StaticSamplers) {
		unsigned int i = 0;
		while (StaticSamplers[i] != core_tgfx_main->INVALIDHANDLE) {
			if (!hidden->samplers.getOBJfromHANDLE(*(VKOBJHANDLE*)&StaticSamplers[i])) { printer(result_tgfx_FAIL, "You shouldn't give NULL or invalid handles to StaticSamplers list!"); return result_tgfx_WARNING; }
			i++;
		}
	}
#endif


	VkDescriptorSetLayoutCreateInfo ci = {};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	VkDescriptorSetLayoutBinding bindngs[2] = {};
	ci.flags = 0;	ci.pNext = nullptr; ci.bindingCount = 1; ci.pBindings = bindngs;
	
	unsigned int dynamicbinding_i = 0;
	std::vector<VkSampler> staticSamplers;
	if (DescriptorType == shaderdescriptortype_tgfx_SAMPLER && StaticSamplers) {
		TGFXLISTCOUNT(core_tgfx_main, StaticSamplers, staticsamplerscount);
		staticSamplers.resize(staticsamplerscount);
		for (unsigned int i = 0; i < staticsamplerscount; i++) {
			staticSamplers[i] =
				hidden->samplers.getOBJfromHANDLE
				(
					*(VKOBJHANDLE*)&StaticSamplers[i]
				)->Sampler;
		}
		bindngs[0].binding = 0;
		bindngs[0].descriptorCount = staticsamplerscount;
		bindngs[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		bindngs[0].pImmutableSamplers = staticSamplers.data();
		bindngs[0].stageFlags = VK_SHADER_STAGE_ALL;

		ci.bindingCount++;
		dynamicbinding_i++;
	}

	bindngs[dynamicbinding_i].binding = dynamicbinding_i;
	bindngs[dynamicbinding_i].descriptorCount = ElementCount;
	bindngs[dynamicbinding_i].descriptorType =
		Find_VkDescType_byVKCONST_DESCSETID(descsetid);
	bindngs[dynamicbinding_i].pImmutableSamplers = nullptr;
	if (VisbleStags) 
	{ bindngs[dynamicbinding_i].stageFlags = *(VkShaderStageFlags*)&VisbleStags; }
	bindngs[dynamicbinding_i].stageFlags =
		GetVkShaderStageFlags_fromTGFXHandle(VisbleStags);

	VkDescriptorSetLayout DSL;
	ThrowIfFailed(
		vkCreateDescriptorSetLayout(rendergpu->LOGICALDEVICE(), &ci, nullptr, &DSL)
		, "Descriptor Set Layout creation has failed!"
	);
	DESCSET_VKOBJ* finalobj = hidden->descsets.create_OBJ();
	*finalobj = DESCSET_VKOBJ
	(
		Find_VkDescType_byVKCONST_DESCSETID(descsetid), ElementCount
	);

	VKOBJHANDLE handle = hidden->descsets.returnHANDLEfromOBJ(finalobj);
	*bindingTableHandle = *(bindingTable_tgfxhnd*)&handle;
	return result_tgfx_SUCCESS;
}
bool VKPipelineLayoutCreation
(
	uint32_t TypeDescs[VKCONST_MAXDESCSET_PERLIST],
	uint32_t InstDescs[VKCONST_MAXDESCSET_PERLIST],
	bool isCallBufferSupported,
	VkPipelineLayout* layout
) {
	VkPipelineLayoutCreateInfo pl_ci = {}; pl_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO; pl_ci.pNext = nullptr;
	pl_ci.flags = 0;

	uint32_t TypeDescsCount = 0, InstDescsCount = 0;
	for (TypeDescsCount; TypeDescsCount < VKCONST_MAXDESCSET_PERLIST; TypeDescsCount++) { if (TypeDescs[TypeDescsCount] == UINT32_MAX) { break; } }
	for (InstDescsCount; InstDescsCount < VKCONST_MAXDESCSET_PERLIST; InstDescsCount++) { if (InstDescs[InstDescsCount] == UINT32_MAX) { break; } }

	if (TypeDescsCount + InstDescsCount == 0) { return true; }
	//It stores 2 lists, so size is twice
	VkDescriptorSetLayout DESCLAYOUTs[VKCONST_MAXDESCSET_PERLIST * 2];
	for (unsigned int typeSets_i = 0; typeSets_i < TypeDescsCount; typeSets_i++) {
		DESCLAYOUTs[typeSets_i] = hidden->descsets.getOBJbyINDEX(TypeDescs[typeSets_i])->Layout;
	}
	for (unsigned int instSets_i = 0; instSets_i < InstDescsCount; instSets_i++) {
		DESCLAYOUTs[TypeDescsCount + instSets_i] = hidden->descsets.getOBJbyINDEX(InstDescs[instSets_i])->Layout;
	}
	pl_ci.setLayoutCount = TypeDescsCount + InstDescsCount;
	pl_ci.pSetLayouts = DESCLAYOUTs;
	VkPushConstantRange range = {};
	//Don't support for now!
	if (isCallBufferSupported) {
		pl_ci.pushConstantRangeCount = 1;
		pl_ci.pPushConstantRanges = &range;
		range.offset = 0;
		range.size = 128;
		range.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
	}
	else { pl_ci.pushConstantRangeCount = 0; pl_ci.pPushConstantRanges = nullptr; }

	ThrowIfFailed
	(
		vkCreatePipelineLayout(rendergpu->LOGICALDEVICE(), &pl_ci, nullptr, layout),
		"Link_MaterialType() failed at vkCreatePipelineLayout()!"
	);

	return true;
}


static EShLanguage Find_EShShaderStage_byTGFXShaderStage(shaderstage_tgfx stage)
{
	switch (stage) {
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
static void* compile_shadersource_withglslang
(
	shaderstage_tgfx tgfxstage,
	void* i_DATA, unsigned int i_DATA_SIZE,
	unsigned int* compiledbinary_datasize
) {
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
		std::string log = std::string("Shader compilation failed!")
			+ shader.getInfoLog() + std::string(shader.getInfoDebugLog());
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
result_tgfx Compile_ShaderSource
(
	shaderlanguages_tgfx language, shaderstage_tgfx shaderstage,
	void* DATA, unsigned int DATA_SIZE,
	shaderSource_tgfxhnd* ShaderSourceHandle
) {
	void* binary_spirv_data = nullptr;
	unsigned int binary_spirv_datasize = 0;
	switch (language) {
	case shaderlanguages_tgfx_SPIRV:
		binary_spirv_data = DATA;
		binary_spirv_datasize = DATA_SIZE;
		break;
	case shaderlanguages_tgfx_GLSL:
		binary_spirv_data =
			compile_shadersource_withglslang
			(
				shaderstage, DATA, DATA_SIZE, &binary_spirv_datasize
			);
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
	ThrowIfFailed
	(
		vkCreateShaderModule(rendergpu->LOGICALDEVICE(), &ci, 0, &Module),
		"Shader Source is failed at creation!"
	);

	SHADERSOURCE_VKOBJ* SHADERSOURCE = hidden->shadersources.create_OBJ();
	SHADERSOURCE->Module = Module;
	SHADERSOURCE->stage = shaderstage;

	VKOBJHANDLE handle = hidden->shadersources.returnHANDLEfromOBJ(SHADERSOURCE);
	*ShaderSourceHandle = *(shaderSource_tgfxhnd*)&handle;
	return result_tgfx_SUCCESS;
}
void  Delete_ShaderSource (shaderSource_tgfxhnd ShaderSourceHandle){}
VkColorComponentFlags Find_ColorWriteMask_byChannels
(textureChannels_tgfx chnnls) {
	switch (chnnls)
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

inline void CountDescSets
(
	bindingTable_tgfxlsthnd descset,
	unsigned int* finaldesccount,
	unsigned int finaldescs[VKCONST_MAXDESCSET_PERLIST]
) {
	TGFXLISTCOUNT(core_tgfx_main, descset, listsize);
	unsigned int valid_descset_i = 0;
	for (unsigned int i = 0; i < listsize; i++) {
		bindingTable_tgfxhnd b_table = descset[i];
		if (!b_table) { continue; }
		VKOBJHANDLE handle = *(VKOBJHANDLE*) & b_table;
		DESCSET_VKOBJ* descset = hidden->descsets.getOBJfromHANDLE(handle);
		if (!descset) { printer(10); }
		if (valid_descset_i >= VKCONST_MAXDESCSET_PERLIST) { printer(11); }
		finaldescs[valid_descset_i++] = hidden->descsets.getINDEXbyOBJ(descset);
	}
	if (valid_descset_i == 0) { printer(12); }
	*finaldesccount = valid_descset_i;
}

bool Get_SlotSets_fromSubDP
(
	renderSubPass_tgfxhnd subdp_handle, 
  RTSLOTSET_VKOBJ** rtslotset,
	IRTSLOTSET_VKOBJ** irtslotset
) {
	/*
	VKOBJHANDLE subdphandle = *(VKOBJHANDLE*)&subdp_handle;
	RenderGraph::SubDP_VK* subdp = getPass_fromHandle<RenderGraph::SubDP_VK>(subdphandle);
	if (!subdp) { printer(13); return false; }
	*rtslotset = contentmanager->GETRTSLOTSET_ARRAY().getOBJbyINDEX(subdp->getDP()->BASESLOTSET_ID);
	*irtslotset = contentmanager->GETIRTSLOTSET_ARRAY().getOBJbyINDEX(subdp->IRTSLOTSET_ID);
	*/
	return true;
}
bool Get_RPOBJ_andSPINDEX_fromSubDP
(
	renderSubPass_tgfxhnd subdp_handle,
	VkRenderPass* rp_obj,
	unsigned int* sp_index
) {
	/*
	VKOBJHANDLE subdphandle = *(VKOBJHANDLE*)&subdp_handle;
	RenderGraph::SubDP_VK* subdp = getPass_fromHandle<RenderGraph::SubDP_VK>(subdphandle);
	if (!subdp) { printer(13); return false; }
	*rp_obj = subdp->getDP()->RenderPassObject;
	*sp_index = subdp->VKRP_BINDINDEX;
	*/
	return true;
}
result_tgfx Link_MaterialType
(
	shadersource_tgfx_listhandle ShaderSourcesList,
	bindingTable_tgfxlsthnd MatTypeBindTables, bindingTable_tgfxlsthnd MatInstBindTables,
	vertexAttributeLayout_tgfxhnd AttributeLayout, renderSubPass_tgfxhnd Subdrawpass, cullmode_tgfx culling,
	polygonmode_tgfx polygonmode, depthsettings_tgfxhnd depthtest, stencilcnfg_tgfxnd StencilFrontFaced,
	stencilcnfg_tgfxnd StencilBackFaced, blendingInfo_tgfxlsthnd BLENDINGs, unsigned char isCallBufferSupported, rasterPipelineType_tgfxhnd* MaterialHandle) {
	if (renderer->RGSTATUS() == RGReconstructionStatus::StartedConstruction) {
		printer(result_tgfx_FAIL, "You can't link a Material Type while recording RenderGraph!");
		return result_tgfx_WRONGTIMING;
	}
	VERTEXATTRIBLAYOUT_VKOBJ* LAYOUT = hidden->vertexattributelayouts.getOBJfromHANDLE(*(VKOBJHANDLE*) & AttributeLayout);
	if (!LAYOUT) {
		printer(result_tgfx_FAIL, "Link_MaterialType() has failed because Material Type has invalid Vertex Attribute Layout!");
		return result_tgfx_INVALIDARGUMENT;
	}

	SHADERSOURCE_VKOBJ* VertexSource = nullptr, *FragmentSource = nullptr;
	unsigned int ShaderSourceCount = 0;
	while (ShaderSourcesList[ShaderSourceCount] != core_tgfx_main->INVALIDHANDLE) {
		SHADERSOURCE_VKOBJ* ShaderSource = hidden->shadersources.getOBJfromHANDLE(*(VKOBJHANDLE*)&ShaderSourcesList[ShaderSourceCount]);
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
	GRAPHICSPIPELINETYPE_VKOBJ VKPipeline;

	VkPipelineShaderStageCreateInfo Vertex_ShaderStage = {};
	VkPipelineShaderStageCreateInfo Fragment_ShaderStage = {};
	{
		Vertex_ShaderStage.sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		Vertex_ShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		VkShaderModule* VS_Module = &VertexSource->Module;
		Vertex_ShaderStage.module = *VS_Module;
		Vertex_ShaderStage.pName = "main";

		Fragment_ShaderStage.sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		Fragment_ShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		VkShaderModule* FS_Module = &FragmentSource->Module;
		Fragment_ShaderStage.module = *FS_Module;
		Fragment_ShaderStage.pName = "main";
	}
	VkPipelineShaderStageCreateInfo STAGEs[2] =
	{ Vertex_ShaderStage, Fragment_ShaderStage };

	VkPipelineVertexInputStateCreateInfo VertexInputState_ci = {};
	{
		VertexInputState_ci.sType =
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		VertexInputState_ci.pVertexBindingDescriptions = &LAYOUT->BindingDesc;
		VertexInputState_ci.vertexBindingDescriptionCount = 1;
		VertexInputState_ci.pVertexAttributeDescriptions = LAYOUT->AttribDescs;
		VertexInputState_ci.vertexAttributeDescriptionCount =
			LAYOUT->AttribDesc_Count;
		VertexInputState_ci.flags = 0;
		VertexInputState_ci.pNext = nullptr;
	}
	VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = {};
	{
		InputAssemblyState.sType =
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		InputAssemblyState.topology = LAYOUT->PrimitiveTopology;
		InputAssemblyState.primitiveRestartEnable = false;
		InputAssemblyState.flags = 0;
		InputAssemblyState.pNext = nullptr;
	}
	VkPipelineViewportStateCreateInfo RenderViewportState = {};
	{
		RenderViewportState.sType =
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		RenderViewportState.scissorCount = 1;
		RenderViewportState.pScissors = nullptr;
		RenderViewportState.viewportCount = 1;
		RenderViewportState.pViewports = nullptr;
		RenderViewportState.pNext = nullptr;
		RenderViewportState.flags = 0;
	}
	VkPipelineRasterizationStateCreateInfo RasterizationState = {};
	{
		RasterizationState.sType =
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		RasterizationState.polygonMode =
			Find_PolygonMode_byGFXPolygonMode(polygonmode);
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

	RTSLOTSET_VKOBJ* baseslotset = nullptr; IRTSLOTSET_VKOBJ* inherited_slotset = nullptr;
	if (!Get_SlotSets_fromSubDP(Subdrawpass, &baseslotset, &inherited_slotset)) { return result_tgfx_FAIL; }
	VkPipelineColorBlendAttachmentState* States =
		(VkPipelineColorBlendAttachmentState*)
		VK_ALLOCATE_AND_GETPTR
		(
			VKGLOBAL_VIRMEM_CURRENTFRAME,
			baseslotset->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT * 
			sizeof(VkPipelineColorBlendAttachmentState)
		);
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

		for 
		(
			uint32_t RTSlotIndex = 0;
			RTSlotIndex < baseslotset->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT;
			RTSlotIndex++
		) {
			bool isFound = false;
			for 
			(
				uint32_t BlendingInfoIndex = 0;
				BlendingInfoIndex < BlendingsCount;
				BlendingInfoIndex++
			) {
				const blendinginfo_vk* blendinginfo = BLENDINGINFOS[BlendingInfoIndex];
				if (blendinginfo->COLORSLOT_INDEX == RTSlotIndex) {
					States[RTSlotIndex] = blendinginfo->BlendState;
					States[RTSlotIndex].colorWriteMask =
						Find_ColorWriteMask_byChannels
						(
							baseslotset->PERFRAME_SLOTSETs[0].COLOR_SLOTs[RTSlotIndex].RT->CHANNELs
						);
					isFound = true;
					break;
				}
			}
			if (!isFound) {
				States[RTSlotIndex] = NonBlendState;
				States[RTSlotIndex].colorWriteMask =
					Find_ColorWriteMask_byChannels
					(
						baseslotset->PERFRAME_SLOTSETs[0].COLOR_SLOTs[RTSlotIndex].RT->CHANNELs
					);
			}
		}

		Pipeline_ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		Pipeline_ColorBlendState.attachmentCount = baseslotset->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT;
		Pipeline_ColorBlendState.pAttachments = States;
		if (BlendingsCount) {
			Pipeline_ColorBlendState.blendConstants[0] =
				BLENDINGINFOS[0]->BLENDINGCONSTANTs.x;
			Pipeline_ColorBlendState.blendConstants[1] =
				BLENDINGINFOS[0]->BLENDINGCONSTANTs.y;
			Pipeline_ColorBlendState.blendConstants[2] =
				BLENDINGINFOS[0]->BLENDINGCONSTANTs.z;
			Pipeline_ColorBlendState.blendConstants[3] =
				BLENDINGINFOS[0]->BLENDINGCONSTANTs.w;
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
	VkDynamicState DynamicStatesList[64]; uint32_t dynamicstatescount = 0;	// 64 is enough, i guess!?
	{
		DynamicStatesList[0] = VK_DYNAMIC_STATE_VIEWPORT; dynamicstatescount++;
		DynamicStatesList[1] = VK_DYNAMIC_STATE_SCISSOR; dynamicstatescount++;

		Dynamic_States.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		Dynamic_States.dynamicStateCount = dynamicstatescount;
		Dynamic_States.pDynamicStates = DynamicStatesList;
	}

	uint32_t typesetscount = 0, instsetscount = 0;
	CountDescSets(MatTypeBindTables, &typesetscount, VKPipeline.TypeSETs);
	CountDescSets(MatInstBindTables, &instsetscount, VKPipeline.InstSETs_base);
	if 
	(	!	VKPipelineLayoutCreation
		(
			VKPipeline.TypeSETs, VKPipeline.InstSETs_base,
			isCallBufferSupported,
			&VKPipeline.PipelineLayout
		)
	) {
		printer(result_tgfx_FAIL, "Link_MaterialType() has failed at VKDescSet_PipelineLayoutCreation!");
		return result_tgfx_FAIL;
	}

	VkPipelineDepthStencilStateCreateInfo depth_state = {};
	if (baseslotset->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT) {
		depth_state.sType =
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
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
		else
		{ 
			depth_state.stencilTestEnable = VK_FALSE;	
			depth_state.back = {};	depth_state.front = {}; 
		}
	}

	VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = {};
	{
		GraphicsPipelineCreateInfo.sType =
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.pColorBlendState = &Pipeline_ColorBlendState;
		if (baseslotset->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT)
		{ GraphicsPipelineCreateInfo.pDepthStencilState = &depth_state; }
		else { GraphicsPipelineCreateInfo.pDepthStencilState = nullptr; }
		GraphicsPipelineCreateInfo.pDynamicState = &Dynamic_States;
		GraphicsPipelineCreateInfo.pInputAssemblyState = &InputAssemblyState;
		GraphicsPipelineCreateInfo.pMultisampleState = &MSAAState;
		GraphicsPipelineCreateInfo.pRasterizationState = &RasterizationState;
		GraphicsPipelineCreateInfo.pVertexInputState = &VertexInputState_ci;
		GraphicsPipelineCreateInfo.pViewportState = &RenderViewportState;
		GraphicsPipelineCreateInfo.layout = VKPipeline.PipelineLayout;
		Get_RPOBJ_andSPINDEX_fromSubDP
		(
			Subdrawpass, &GraphicsPipelineCreateInfo.renderPass,
			&GraphicsPipelineCreateInfo.subpass
		);
		GraphicsPipelineCreateInfo.stageCount = 2;
		GraphicsPipelineCreateInfo.pStages = STAGEs;
		GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;		//Optional
		GraphicsPipelineCreateInfo.basePipelineIndex = -1;								//Optional
		GraphicsPipelineCreateInfo.flags = 0;
		GraphicsPipelineCreateInfo.pNext = nullptr;
		ThrowIfFailed
		(
			vkCreateGraphicsPipelines
			(
				rendergpu->LOGICALDEVICE(),
				nullptr,
				1, &GraphicsPipelineCreateInfo,
				nullptr,
				&VKPipeline.PipelineObject),
			"vkCreateGraphicsPipelines has failed!"
		);
	}

	VKPipeline.GFX_Subpass = Subdrawpass;

	GRAPHICSPIPELINETYPE_VKOBJ* finalobj = hidden->pipelinetypes.create_OBJ();
	*finalobj = VKPipeline;
	VKOBJHANDLE handle = hidden->pipelinetypes.returnHANDLEfromOBJ(finalobj);
	*MaterialHandle = *(rasterPipelineType_tgfxhnd*)&handle;
	return result_tgfx_SUCCESS;
}

void  Delete_MaterialType(rasterPipelineType_tgfxhnd ID)
{ printer(result_tgfx_NOTCODED, "Delete_MaterialType() isn't implemented!"); }
result_tgfx Create_MaterialInst
(
	rasterPipelineType_tgfxhnd MaterialType,
	bindingTable_tgfxlsthnd MatInstBindingTable,
	rasterPipelineInstance_tgfxhnd* MaterialInstHandle
) {
	GRAPHICSPIPELINETYPE_VKOBJ* VKPSO =
		hidden->pipelinetypes.getOBJfromHANDLE(*(VKOBJHANDLE*) & MaterialType);
	unsigned int descSets[VKCONST_MAXDESCSET_PERLIST], descSetCount;
	CountDescSets(MatInstBindingTable, &descSetCount, descSets);

#ifdef VULKAN_DEBUGGING
	if (!VKPSO) {
		printer
		(
			result_tgfx_FAIL,
			"Create_MaterialInst() has failed because Material Type isn't found!"
		);
		return result_tgfx_INVALIDARGUMENT;
	}
	uint32_t base_instancedescsetcount = 0;
	while 
	(
		VKPSO->InstSETs_base[base_instancedescsetcount++] != UINT32_MAX &&
		base_instancedescsetcount < VKCONST_MAXDESCSET_PERLIST
	) {}
	if (descSetCount != base_instancedescsetcount) {
		printer(result_tgfx_INVALIDARGUMENT, "Create_MaterialInst() has failed because given BindingTable doesn't compatible with the one given while linking material type!"); return result_tgfx_INVALIDARGUMENT;
	}
#endif

	//Descriptor Set Creation
	GRAPHICSPIPELINEINST_VKOBJ* VKPInstance =
		hidden->pipelineinstances.create_OBJ();

	VKPInstance->PROGRAM = hidden->pipelinetypes.getINDEXbyOBJ(VKPSO);
	for (unsigned int i = 0; i < descSetCount; i++) {
		VKPInstance->InstSETs[i] = descSets[i];
	}

	VKOBJHANDLE handle =
		hidden->pipelineinstances.returnHANDLEfromOBJ(VKPInstance);
	*MaterialInstHandle = *(rasterPipelineInstance_tgfxhnd*)&handle;
	return result_tgfx_SUCCESS;
}
void  Delete_MaterialInst (rasterPipelineInstance_tgfxhnd ID){}

result_tgfx Create_ComputeType
(
	shaderSource_tgfxhnd Source,
	bindingTable_tgfxlsthnd TypeBindingTables,
	bindingTable_tgfxlsthnd InstBindingTables,
	unsigned char isCallBufferSupported,
	computeShaderType_tgfxhnd* ComputeTypeHandle
) {
	VkComputePipelineCreateInfo cp_ci = {};
	SHADERSOURCE_VKOBJ* SHADER =
		hidden->shadersources.getOBJfromHANDLE(*(VKOBJHANDLE*)&Source);
	VkShaderModule shader_module = SHADER->Module;

	COMPUTETYPE_VKOBJ VKPipeline;

	uint32_t typesetscount = 0, instsetscount = 0;
	CountDescSets(TypeBindingTables, &typesetscount, VKPipeline.TypeSETs);
	CountDescSets(InstBindingTables, &instsetscount, VKPipeline.InstSETs);


	if 
	(
		!VKPipelineLayoutCreation
		(
			VKPipeline.TypeSETs, VKPipeline.InstSETs,
			isCallBufferSupported,
			&VKPipeline.PipelineLayout
		)
	) {
		printer(result_tgfx_FAIL, "Compile_ComputeType() has failed at VKDescSet_PipelineLayoutCreation!");
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
		cp_ci.layout = VKPipeline.PipelineLayout;
		cp_ci.pNext = nullptr;
		cp_ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		ThrowIfFailed
		(
			vkCreateComputePipelines
			(
				rendergpu->LOGICALDEVICE(),
				VK_NULL_HANDLE,
				1, &cp_ci,
				nullptr,
				&VKPipeline.PipelineObject
			),
			"Compile_ComputeShader() has failed at vkCreateComputePipelines!"
		);
	}
	vkDestroyShaderModule(rendergpu->LOGICALDEVICE(), shader_module, nullptr);
	
	COMPUTETYPE_VKOBJ* obj = hidden->computetypes.create_OBJ();
	*obj = VKPipeline;
	VKOBJHANDLE handle = hidden->computetypes.returnHANDLEfromOBJ(obj);
	*ComputeTypeHandle = *(computeShaderType_tgfxhnd*)&handle;
	return result_tgfx_SUCCESS;
}
result_tgfx Create_ComputeInstance
(
	computeShaderType_tgfxhnd ComputeType,
	bindingTable_tgfxlsthnd ComputeInstBindingTable,
	computeShaderInstance_tgfxhnd* ComputeShaderInstanceHandle
) {
	COMPUTETYPE_VKOBJ* VKPipeline =
		hidden->computetypes.getOBJfromHANDLE(*(VKOBJHANDLE*)&ComputeType);
	uint32_t descSets[VKCONST_MAXDESCSET_PERLIST] = {UINT32_MAX}, descSetCount;
	CountDescSets(ComputeInstBindingTable, &descSetCount, descSets);

#ifdef VULKAN_DEBUGGING
	if (!VKPipeline) {
		printer
		(
			result_tgfx_FAIL,
			"Create_MaterialInst() has failed because Material Type isn't found!"
		);
		return result_tgfx_INVALIDARGUMENT;
	}
	uint32_t baseInstSetCount = 0;
	while
	(
		VKPipeline->InstSETs[baseInstSetCount++] != UINT32_MAX
		&& baseInstSetCount < VKCONST_MAXDESCSET_PERLIST
	) {}
	if (descSetCount != baseInstSetCount) {
		printer(result_tgfx_INVALIDARGUMENT, "Create_ComputeInstance() has failed because given BindingTable doesn't compatible with the one given while creating compute type!"); return result_tgfx_INVALIDARGUMENT;
	}
#endif

	COMPUTEINST_VKOBJ* instance = hidden->computeinstances.create_OBJ();
	instance->PROGRAM = hidden->computetypes.getINDEXbyOBJ(VKPipeline);
	for (unsigned int i = 0; i < descSetCount; i++) {
		instance->InstSETs[i] = descSets[i];
	}

	VKOBJHANDLE handle = hidden->computeinstances.returnHANDLEfromOBJ(instance);
	*ComputeShaderInstanceHandle = *(computeShaderInstance_tgfxhnd*)&handle;
	return result_tgfx_SUCCESS;
}

//Set a descriptor of the binding table created with shaderdescriptortype_tgfx_SAMPLER
void SetDescriptor_Sampler
(
	bindingTable_tgfxhnd bindingtable, unsigned int elementIndex,
	samplingType_tgfxhnd samplerHandle
) {
	VKOBJHANDLE bindingtable_objhandle = *(VKOBJHANDLE*)&bindingtable;
	VKOBJHANDLE	sampler_objhandle = *(VKOBJHANDLE*)&samplerHandle;
#ifdef VULKAN_DEBUGGING
	if (bindingtable_objhandle.EXTRA_FLAGs >= VKCONST_DYNAMICDESCRIPTORTYPESCOUNT)
	{ printer(result_tgfx_FAIL, "BindingTable handle is invalid!"); return; }
#endif
	DESCSET_VKOBJ* set = hidden->descsets.getOBJfromHANDLE(bindingtable_objhandle);
	SAMPLER_VKOBJ* sampler = hidden->samplers.getOBJfromHANDLE(sampler_objhandle);

#ifdef VULKAN_DEBUGGING
	if 
	(
		!set || !sampler
		|| elementIndex >= set->samplerDescELEMENTs.size()
		|| set->DESCTYPE != VK_DESCRIPTOR_TYPE_SAMPLER)
	{
		printer(result_tgfx_INVALIDARGUMENT, "SetDescriptor_Sampler() has invalid input!"); return;
	}
	sampler_descVK& samplerDESC = set->samplerDescELEMENTs[elementIndex];
	unsigned char elementUpdateStatus = 0;
	if (!samplerDESC.isUpdated.compare_exchange_weak(elementUpdateStatus, 1)) {
		printer(result_tgfx_FAIL, "SetDescriptor_Sampler() failed because you already changed the descriptor in the same frame!");
	}
#else
	sampler_descVK& samplerDESC = ((sampler_descVK*)set->DescElements)[elementIndex];
	samplerDESC.isUpdated.store(1);
#endif

	samplerDESC.sampler_obj = sampler->Sampler;
	set->isUpdatedCounter.store(3);
}
//Set a descriptor of the binding table created with shaderdescriptortype_tgfx_BUFFER
result_tgfx SetDescriptor_Buffer
(
	bindingTable_tgfxhnd bindingtable, unsigned int elementIndex,
	buffer_tgfxhnd bufferHandle, unsigned int Offset, unsigned int DataSize,
	extension_tgfxlsthnd exts) {
	VKOBJHANDLE bindingtable_objhandle = *(VKOBJHANDLE*)&bindingtable;
#ifdef VULKAN_DEBUGGING
	if (bindingtable_objhandle.EXTRA_FLAGs >= VKCONST_DYNAMICDESCRIPTORTYPESCOUNT)
	{ printer(result_tgfx_FAIL, "BindingTable handle is invalid!"); return result_tgfx_FAIL; }
#endif
	DESCSET_VKOBJ* set = hidden->descsets.getOBJfromHANDLE(bindingtable_objhandle);

	
#ifdef VULKAN_DEBUGGING
	if 
	(
		!set || !bufferHandle
		|| elementIndex >= set->bufferDescELEMENTs.size()
		|| set->DESCTYPE != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	) {
		printer(result_tgfx_INVALIDARGUMENT, "SetDescriptor_Buffer() has invalid input!"); return result_tgfx_FAIL; 
	}
	buffer_descVK& bufferDESC = set->bufferDescELEMENTs[elementIndex];
	unsigned char elementUpdateStatus = 0;
	if (!bufferDESC.isUpdated.compare_exchange_weak(elementUpdateStatus, 1)) {
		printer(result_tgfx_FAIL, "SetDescriptor_Buffer() failed because you already changed the descriptor in the same frame!");
		return result_tgfx_FAIL;
	}
#else
	bufferDESC.isUpdated.store(1);
#endif
	bufferDESC.info.buffer = VK_NULL_HANDLE;

	set->isUpdatedCounter.store(3);
	return result_tgfx_SUCCESS;
}

//Set a descriptor of the binding table created with shaderdescriptortype_tgfx_SAMPLEDTEXTURE
void SetDescriptor_SampledTexture(bindingTable_tgfxhnd bindingtable, unsigned int elementIndex,
	texture_tgfxhnd textureHandle) {
	VKOBJHANDLE bindingtable_objhandle = *(VKOBJHANDLE*)&bindingtable;
#ifdef VULKAN_DEBUGGING
	if (bindingtable_objhandle.EXTRA_FLAGs >= VKCONST_DYNAMICDESCRIPTORTYPESCOUNT)
	{ printer(result_tgfx_FAIL, "BindingTable handle is invalid!"); return; }
#endif
	DESCSET_VKOBJ* set =
		hidden->descsets.getOBJfromHANDLE(bindingtable_objhandle);

	TEXTURE_VKOBJ* texture = (TEXTURE_VKOBJ*)textureHandle;
#ifdef VULKAN_DEBUGGING
	if 
	(
		!set || !textureHandle
		|| elementIndex >= set->textureDescELEMENTs.size()
		|| set->DESCTYPE != VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
	) {
		printer(result_tgfx_INVALIDARGUMENT, "SetDescriptor_SampledTexture() has invalid input!"); return;
	}
	texture_descVK& textureDESC = set->textureDescELEMENTs[elementIndex];
	unsigned char elementUpdateStatus = 0;
	if (!textureDESC.isUpdated.compare_exchange_weak(elementUpdateStatus, 1)) {
		printer(result_tgfx_FAIL, "SetDescriptor_SampledTexture() failed because you already changed the descriptor in the same frame!");
	}
#else
	texture_descVK& textureDESC =
		((texture_descVK*)set->DescElements)[elementIndex];
	textureDESC.isUpdated.store(1);
#endif
	textureDESC.info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	textureDESC.info.imageView = texture->ImageView;
	textureDESC.info.sampler = VK_NULL_HANDLE;

	set->isUpdatedCounter.store(3);
}
//Set a descriptor of the binding table created with shaderdescriptortype_tgfx_STORAGEIMAGE
void SetDescriptor_StorageImage
(
	bindingTable_tgfxhnd bindingtable, unsigned int elementIndex,
	texture_tgfxhnd textureHandle
) {
	VKOBJHANDLE bindingtable_objhandle = *(VKOBJHANDLE*)&bindingtable;
#ifdef VULKAN_DEBUGGING
	if (bindingtable_objhandle.EXTRA_FLAGs >= VKCONST_DYNAMICDESCRIPTORTYPESCOUNT) { printer(result_tgfx_FAIL, "BindingTable handle is invalid!"); return; }
#endif
	DESCSET_VKOBJ* set = hidden->descsets.getOBJfromHANDLE(bindingtable_objhandle);

	TEXTURE_VKOBJ* texture = (TEXTURE_VKOBJ*)textureHandle;
#ifdef VULKAN_DEBUGGING
	if (!set || !textureHandle || elementIndex >= set->textureDescELEMENTs.size() || set->DESCTYPE != VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
		printer(result_tgfx_INVALIDARGUMENT, "SetDescriptor_StorageImage() has invalid input!"); return;
	}
	texture_descVK& textureDESC = set->textureDescELEMENTs[elementIndex];
	unsigned char elementUpdateStatus = 0;
	if (!textureDESC.isUpdated.compare_exchange_weak(elementUpdateStatus, 1)) {
		printer(result_tgfx_FAIL, "SetDescriptor_StorageImage() failed because you already changed the descriptor in the same frame!");
}
#else
	texture_descVK& textureDESC = ((texture_descVK*)set->DescElements)[elementIndex];
	textureDESC.isUpdated.store(1);
#endif
	textureDESC.info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	textureDESC.info.imageView = texture->ImageView;
	textureDESC.info.sampler = VK_NULL_HANDLE;

	set->isUpdatedCounter.store(3);
}


result_tgfx Create_RTSlotset
(
	RTSlotDescription_tgfxlsthnd Descriptions,
	RTSlotset_tgfxhnd* RTSlotSetHandle
) { 
	TGFXLISTCOUNT(core_tgfx_main, Descriptions, DescriptionsCount);
	for (unsigned int SlotIndex = 0; SlotIndex < DescriptionsCount; SlotIndex++) {
		const rtslot_create_description_vk* desc = (rtslot_create_description_vk*)Descriptions[SlotIndex];
		TEXTURE_VKOBJ* FirstHandle = desc->textures[0];
		TEXTURE_VKOBJ* SecondHandle = desc->textures[1];
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
		const rtslot_create_description_vk* desc = (rtslot_create_description_vk*)Descriptions[SlotIndex];
		for (unsigned int RTIndex = 0; RTIndex < 2; RTIndex++) {
			TEXTURE_VKOBJ* RT = (desc->textures[RTIndex]);
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

	unsigned int FBWIDTH = ((rtslot_create_description_vk*)Descriptions[0])->textures[0]->WIDTH;
	unsigned int FBHEIGHT = ((rtslot_create_description_vk*)Descriptions[0])->textures[0]->HEIGHT;
	for (unsigned int SlotIndex = 0; SlotIndex < DescriptionsCount; SlotIndex++) {
		TEXTURE_VKOBJ* Texture = ((rtslot_create_description_vk*)Descriptions[SlotIndex])->textures[0];
		if (Texture->WIDTH != FBWIDTH || Texture->HEIGHT != FBHEIGHT) {
			printer(result_tgfx_FAIL, "Create_RTSlotSet() has failed because one of your slot's texture has wrong resolution!");
			return result_tgfx_INVALIDARGUMENT;
		}
	}
	

	RTSLOTSET_VKOBJ* VKSLOTSET = hidden->rtslotsets.create_OBJ();
	for (unsigned int SlotSetIndex = 0; SlotSetIndex < 2; SlotSetIndex++) {
		rtslots_vk& PF_SLOTSET = VKSLOTSET->PERFRAME_SLOTSETs[SlotSetIndex];

		PF_SLOTSET.COLOR_SLOTs = new colorslot_vk[COLORRT_COUNT];
		PF_SLOTSET.COLORSLOTs_COUNT = COLORRT_COUNT;
		if (DEPTHSLOT_VECTORINDEX != UINT32_MAX) {
			PF_SLOTSET.DEPTHSTENCIL_SLOT = new depthstencilslot_vk;
			depthstencilslot_vk* slot = PF_SLOTSET.DEPTHSTENCIL_SLOT;
			const rtslot_create_description_vk* DEPTHDESC = (rtslot_create_description_vk*)Descriptions[DEPTHSLOT_VECTORINDEX];
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
			const rtslot_create_description_vk* desc = (rtslot_create_description_vk*)Descriptions[i];
			TEXTURE_VKOBJ* RT = desc->textures[SlotSetIndex];
			colorslot_vk& SLOT = PF_SLOTSET.COLOR_SLOTs[slotindex];
			SLOT.RT_OPERATIONTYPE = desc->optype;
			SLOT.LOADSTATE = desc->loadtype;
			SLOT.RT = RT;
			SLOT.IS_USED_LATER = desc->isUsedLater;
			SLOT.CLEAR_COLOR = glm::vec4(desc->clear_value.x, desc->clear_value.y, desc->clear_value.z, desc->clear_value.w);
		}

		VkImageView* imageviews = (VkImageView*)VK_ALLOCATE_AND_GETPTR(VKGLOBAL_VIRMEM_CURRENTFRAME, (PF_SLOTSET.COLORSLOTs_COUNT + 1) * sizeof(VkImageView));
		for (unsigned int i = 0; i < PF_SLOTSET.COLORSLOTs_COUNT; i++) {
			TEXTURE_VKOBJ* VKTexture = PF_SLOTSET.COLOR_SLOTs[i].RT;
			imageviews[i] = VKTexture->ImageView;
		}
		if (PF_SLOTSET.DEPTHSTENCIL_SLOT) {
			imageviews[PF_SLOTSET.COLORSLOTs_COUNT] = PF_SLOTSET.DEPTHSTENCIL_SLOT->RT->ImageView;
		}
		

		VkFramebufferCreateInfo& fb_ci = VKSLOTSET->FB_ci[SlotSetIndex];
		fb_ci.attachmentCount = PF_SLOTSET.COLORSLOTs_COUNT + ((PF_SLOTSET.DEPTHSTENCIL_SLOT) ? 1 : 0);
		fb_ci.pAttachments = imageviews;
		fb_ci.flags = 0;
		fb_ci.height = FBHEIGHT;
		fb_ci.width = FBWIDTH;
		fb_ci.layers = 1;
		fb_ci.pNext = nullptr;
		fb_ci.renderPass = VK_NULL_HANDLE;
		fb_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	}

	VKOBJHANDLE handle = hidden->rtslotsets.returnHANDLEfromOBJ(VKSLOTSET);
	*RTSlotSetHandle = *(RTSlotset_tgfxhnd*)&handle;
	return result_tgfx_SUCCESS;
}
void  Delete_RTSlotSet (RTSlotset_tgfxhnd RTSlotSetHandle){}
//Changes on RTSlots only happens at the frame slot is gonna be used
//For example; if you change next frame's slot, necessary API calls are gonna be called next frame
//For example; if you change slot but related slotset isn't used by drawpass, it doesn't happen until it is used
result_tgfx Change_RTSlotTexture
(
	RTSlotset_tgfxhnd RTSlotHandle,
	unsigned char isColorRT,
	unsigned char SlotIndex,
	unsigned char FrameIndex,
	texture_tgfxhnd TextureHandle
) { return result_tgfx_FAIL; }
result_tgfx Inherite_RTSlotSet
(
	RTSlotUsage_tgfxlsthnd DescriptionsGFX,
	RTSlotset_tgfxhnd RTSlotSetHandle,
	inheritedRTSlotset_tgfxhnd* InheritedSlotSetHandle
) {
	if (!RTSlotSetHandle) {
		printer(result_tgfx_FAIL, "Inherite_RTSlotSet() has failed because Handle is invalid!");
		return result_tgfx_INVALIDARGUMENT;
	}
	RTSLOTSET_VKOBJ* BaseSet = hidden->rtslotsets.getOBJfromHANDLE(*(VKOBJHANDLE*)&RTSlotSetHandle);
	IRTSLOTSET_VKOBJ InheritedSet;
	InheritedSet.BASESLOTSET = hidden->rtslotsets.getINDEXbyOBJ(BaseSet);
	rtslot_inheritance_descripton_vk** Descriptions = (rtslot_inheritance_descripton_vk**)DescriptionsGFX;

	//Find Depth/Stencil Slots and count Color Slots
	bool DEPTH_FOUND = false;
	unsigned char COLORSLOT_COUNT = 0, DEPTHDESC_VECINDEX = 0;
	TGFXLISTCOUNT(core_tgfx_main, Descriptions, DESCCOUNT);
	for (unsigned char i = 0; i < DESCCOUNT; i++) {
		const rtslot_inheritance_descripton_vk* DESC = Descriptions[i];
		if (DESC->IS_DEPTH) {
			if (DEPTH_FOUND) {
				printer(result_tgfx_FAIL, "Inherite_RTSlotSet() has failed because there are two depth buffers in the description, which is not supported!");
				return result_tgfx_INVALIDARGUMENT;
			}
			DEPTH_FOUND = true;
			DEPTHDESC_VECINDEX = i;
			if
			(
				BaseSet->PERFRAME_SLOTSETs[0].DEPTHSTENCIL_SLOT->DEPTH_OPTYPE == operationtype_tgfx_READ_ONLY &&
				(
					DESC->OPTYPE == operationtype_tgfx_WRITE_ONLY
					|| DESC->OPTYPE == operationtype_tgfx_READ_AND_WRITE
				)
			)
			{
				printer(result_tgfx_FAIL, "Inherite_RTSlotSet() has failed because you can't use a Read-Only DepthSlot with Write Access in a Inherited Set!");
				return result_tgfx_INVALIDARGUMENT;
			}
			InheritedSet.DEPTH_OPTYPE = DESC->OPTYPE;
			InheritedSet.STENCIL_OPTYPE = DESC->OPTYPESTENCIL;
		}
		else {
			COLORSLOT_COUNT++;
		}
	}
	if (!DEPTH_FOUND) {
		InheritedSet.DEPTH_OPTYPE = operationtype_tgfx_UNUSED;
	}
	if (COLORSLOT_COUNT != BaseSet->PERFRAME_SLOTSETs[0].COLORSLOTs_COUNT) {
		printer(result_tgfx_FAIL, "Inherite_RTSlotSet() has failed because BaseSet's Color Slot count doesn't match given Descriptions's one!");
		return result_tgfx_INVALIDARGUMENT;
	}


	InheritedSet.COLOR_OPTYPEs = new operationtype_tgfx[COLORSLOT_COUNT];
	//Set OPTYPEs of inherited slotset
	for (unsigned int i = 0; i < COLORSLOT_COUNT; i++) {
		if (i == DEPTHDESC_VECINDEX) {
			continue;
		}
		unsigned char slotindex = ((i > DEPTHDESC_VECINDEX) ? (i - 1) : i);
		
		//FIX: LoadType isn't supported natively while changing subpass, it may be supported by adding a VkCmdPipelineBarrier but don't want to bother with it for now!
		InheritedSet.COLOR_OPTYPEs[slotindex] = Descriptions[i]->OPTYPE;
	}

	IRTSLOTSET_VKOBJ* finalobj = hidden->irtslotsets.create_OBJ();
	*finalobj = InheritedSet;
	VKOBJHANDLE handle = hidden->irtslotsets.returnHANDLEfromOBJ(finalobj);
	*InheritedSlotSetHandle = *(inheritedRTSlotset_tgfxhnd*)&handle;
	return result_tgfx_SUCCESS;
}
void  Delete_InheritedRTSlotSet
(inheritedRTSlotset_tgfxhnd InheritedRTSlotSetHandle){}

VK_LINEAR_OBJARRAY<INDEXBUFFER_VKOBJ>& gpudatamanager_public::
GETIB_ARRAY() { return hidden->indexbuffers; }
VK_LINEAR_OBJARRAY<VERTEXBUFFER_VKOBJ, 1 << 24>& gpudatamanager_public::
GETVB_ARRAY() { return hidden->vertexbuffers; }
VK_LINEAR_OBJARRAY<RTSLOTSET_VKOBJ, 1024>& gpudatamanager_public::
GETRTSLOTSET_ARRAY() { return hidden->rtslotsets; }
VK_LINEAR_OBJARRAY<IRTSLOTSET_VKOBJ, 1024>& gpudatamanager_public::
GETIRTSLOTSET_ARRAY() { return hidden->irtslotsets; }
VK_LINEAR_OBJARRAY<COMPUTETYPE_VKOBJ>& gpudatamanager_public::
GETCOMPUTETYPE_ARRAY() { return hidden->computetypes; }
VK_LINEAR_OBJARRAY<COMPUTEINST_VKOBJ>& gpudatamanager_public::
GETCOMPUTEINST_ARRAY() { return hidden->computeinstances; }
VK_LINEAR_OBJARRAY<TEXTURE_VKOBJ, 1 << 24>& gpudatamanager_public::
GETTEXTURES_ARRAY() { return hidden->textures; }
VK_LINEAR_OBJARRAY<GRAPHICSPIPELINETYPE_VKOBJ, 1 << 24>& gpudatamanager_public::
GETGRAPHICSPIPETYPE_ARRAY() { return hidden->pipelinetypes; }
VK_LINEAR_OBJARRAY<GRAPHICSPIPELINEINST_VKOBJ, 1 << 24>& gpudatamanager_public::
GETGRAPHICSPIPEINST_ARRAY() { return hidden->pipelineinstances; }
VK_LINEAR_OBJARRAY<GLOBALSSBO_VKOBJ>& gpudatamanager_public::
GETGLOBALSSBO_ARRAY() { return hidden->globalbuffers; }
VK_LINEAR_OBJARRAY<DESCSET_VKOBJ, 1 << 16>& gpudatamanager_public::
GETDESCSET_ARRAY() { return hidden->descsets; }
inline void set_functionpointers() {
	core_tgfx_main->contentmanager->Change_RTSlotTexture = Change_RTSlotTexture;
	core_tgfx_main->contentmanager->Compile_ShaderSource = Compile_ShaderSource;
	core_tgfx_main->contentmanager->Create_BindingTable = Create_BindingTable;
	core_tgfx_main->contentmanager->Create_ComputeInstance = Create_ComputeInstance;
	core_tgfx_main->contentmanager->Create_ComputeType = Create_ComputeType;
	core_tgfx_main->contentmanager->Create_GlobalBuffer = Create_GlobalBuffer;
	core_tgfx_main->contentmanager->Create_IndexBuffer = Create_IndexBuffer;
	core_tgfx_main->contentmanager->Create_MaterialInst = Create_MaterialInst;
	core_tgfx_main->contentmanager->Create_RTSlotset = Create_RTSlotset;
	core_tgfx_main->contentmanager->Create_Sampler = Create_SamplingType;
	core_tgfx_main->contentmanager->Create_Texture = Create_Texture;
	core_tgfx_main->contentmanager->Create_VertexAttributeLayout = Create_VertexAttributeLayout;
	core_tgfx_main->contentmanager->Create_VertexBuffer = Create_VertexBuffer;
	core_tgfx_main->contentmanager->Delete_InheritedRTSlotSet = Delete_InheritedRTSlotSet;
	core_tgfx_main->contentmanager->Delete_MaterialInst = Delete_MaterialInst;
	core_tgfx_main->contentmanager->Delete_MaterialType = Delete_MaterialType;
	core_tgfx_main->contentmanager->Delete_RTSlotSet = Delete_RTSlotSet;
	core_tgfx_main->contentmanager->Delete_ShaderSource = Delete_ShaderSource;
	core_tgfx_main->contentmanager->Delete_VertexAttributeLayout = Delete_VertexAttributeLayout;
	core_tgfx_main->contentmanager->Destroy_AllResources = Destroy_AllResources;
	core_tgfx_main->contentmanager->Inherite_RTSlotSet = Inherite_RTSlotSet;
	core_tgfx_main->contentmanager->Link_MaterialType = Link_MaterialType;
	core_tgfx_main->contentmanager->Unload_IndexBuffer = Unload_IndexBuffer;
	core_tgfx_main->contentmanager->Unload_VertexBuffer = Unload_VertexBuffer;
	core_tgfx_main->contentmanager->Upload_Texture = Upload_Texture;
	core_tgfx_main->contentmanager->Upload_toBuffer = Upload_toBuffer;
	core_tgfx_main->contentmanager->SetBindingTable_Buffer = SetDescriptor_Buffer;
}
void Create_GPUContentManager(initialization_secondstageinfo* info) {
	contentmanager = new gpudatamanager_public;
	contentmanager->hidden = new gpudatamanager_private;
	hidden = contentmanager->hidden;

	set_functionpointers();


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



	//Create Descriptor Pools
	for (unsigned int DescTypeID = 0; DescTypeID < VKCONST_DYNAMICDESCRIPTORTYPESCOUNT; DescTypeID++) {
		VkDescriptorPoolSize size;
		size.descriptorCount = info->MAXDESCCOUNTs[DescTypeID];
		size.type = Find_VkDescType_byVKCONST_DESCSETID(DescTypeID);

		if (!size.descriptorCount) { break; }

		VkDescriptorPoolCreateInfo descpool_ci = {};
		descpool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descpool_ci.maxSets = info->MAXDESCSETCOUNT;
		descpool_ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		descpool_ci.pPoolSizes = &size;
		descpool_ci.poolSizeCount = 1;
		descpool_ci.pNext = nullptr;

		for(unsigned int PoolID = 0; PoolID < VKCONST_DESCPOOLCOUNT_PERDESCTYPE; PoolID++)
		{
			if (vkCreateDescriptorPool(rendergpu->LOGICALDEVICE(), &descpool_ci, nullptr, &hidden->ALL_DESCPOOLS[DescTypeID].POOLs[PoolID]) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "Vulkan Global Descriptor Pool creation has failed! You possibly exceeded GPU limits.");
			}
		}
		hidden->ALL_DESCPOOLS[DescTypeID].REMAINING_DESCs.store(info->MAXDESCCOUNTs[DescTypeID]);
		hidden->ALL_DESCPOOLS[DescTypeID].REMAINING_SETs.store(info->MAXDESCSETCOUNT);
	}
}