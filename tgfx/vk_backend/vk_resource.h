#pragma once
#include <glm/glm.hpp>
#include <atomic>
#include "predefinitions_vk.h"
#include <tgfx_structs.h>
#include "includes.h"


	//Memory Management

struct memoryblock_vk {
	unsigned int MemAllocIndex = UINT32_MAX;
	VkDeviceSize Offset;
};

	//Classic Memory Resources

struct TEXTURE_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::TEXTURE;
	static uint16_t GET_EXTRAFLAGS(TEXTURE_VKOBJ* obj) { return 0; }
	void operator = (const TEXTURE_VKOBJ& copyFrom) {
		isALIVE.store(true);
		WIDTH = copyFrom.WIDTH; HEIGHT = copyFrom.HEIGHT; DATA_SIZE = copyFrom.DATA_SIZE; MIPCOUNT = copyFrom.MIPCOUNT;
		CHANNELs = copyFrom.CHANNELs; USAGE = copyFrom.USAGE; DIMENSION = copyFrom.DIMENSION; Block = copyFrom.Block;
		Image = copyFrom.Image; ImageView = copyFrom.ImageView;
	}

	unsigned int WIDTH, HEIGHT, DATA_SIZE;
	unsigned char MIPCOUNT;
	texture_channels_tgfx CHANNELs;
	VkImageUsageFlags USAGE;
	texture_dimensions_tgfx DIMENSION;
	memoryblock_vk Block;

	VkImage Image = {};
	VkImageView ImageView = {};
};
struct GLOBALSSBO_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::GLOBALSSBO;
	
	static uint16_t GET_EXTRAFLAGS(GLOBALSSBO_VKOBJ* obj) { return 0; }
	void operator = (const GLOBALSSBO_VKOBJ& copyFrom) {
		isALIVE.store(true);
		DATA_SIZE = copyFrom.DATA_SIZE; Block = copyFrom.Block; }
	VkDeviceSize DATA_SIZE;
	memoryblock_vk Block;
};

	//Framebuffer RT Slot Management

struct colorslot_vk {
	TEXTURE_VKOBJ* RT;
	drawpassload_tgfx LOADSTATE;
	bool IS_USED_LATER;
	operationtype_tgfx RT_OPERATIONTYPE;
	glm::vec4 CLEAR_COLOR;
	std::atomic_bool IsChanged = false;
};
struct depthstencilslot_vk {
	TEXTURE_VKOBJ* RT;
	drawpassload_tgfx DEPTH_LOAD, STENCIL_LOAD;
	bool IS_USED_LATER;
	operationtype_tgfx DEPTH_OPTYPE, STENCIL_OPTYPE;
	glm::vec2 CLEAR_COLOR;
	std::atomic_bool IsChanged = false;
};
struct rtslots_vk {
	colorslot_vk* COLOR_SLOTs = nullptr;
	unsigned char COLORSLOTs_COUNT = 0;
	depthstencilslot_vk* DEPTHSTENCIL_SLOT = nullptr;	//There is one, but there may not be a Depth Slot. So if there is no, then this is nullptr.
	//Unused Depth and NoDepth are different. Unused Depth means RenderPass does have one but current Subpass doesn't use, but NoDepth means RenderPass doesn't have one!
	std::atomic_bool IsChanged = false;
	void operator =(const rtslots_vk& copyFrom) {
		IsChanged.store(copyFrom.IsChanged.load()); DEPTHSTENCIL_SLOT = copyFrom.DEPTHSTENCIL_SLOT;
		COLOR_SLOTs = copyFrom.COLOR_SLOTs; COLORSLOTs_COUNT = copyFrom.COLORSLOTs_COUNT;
	}
};
struct RTSLOTSET_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::RTSLOTSET;
	static uint16_t GET_EXTRAFLAGS(RTSLOTSET_VKOBJ* obj) { return 0; }
	
	void operator = (const RTSLOTSET_VKOBJ& copyFrom) {
		isALIVE.store(true);
		PERFRAME_SLOTSETs[0] = copyFrom.PERFRAME_SLOTSETs[0]; PERFRAME_SLOTSETs[1] = copyFrom.PERFRAME_SLOTSETs[1];
		FB_ci[0] = copyFrom.FB_ci[0]; FB_ci[1] = copyFrom.FB_ci[1];
	}
	rtslots_vk PERFRAME_SLOTSETs[2];
	//You should change this struct's vkRenderPass object pointer as your vkRenderPass object
	VkFramebufferCreateInfo FB_ci[2];
};
struct IRTSLOTSET_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::IRTSLOTSET;
	static uint16_t GET_EXTRAFLAGS(IRTSLOTSET_VKOBJ* obj) { return 0; }
	
	void operator = (const IRTSLOTSET_VKOBJ& copyFrom) {
		isALIVE.store(true);
		BASESLOTSET = copyFrom.BASESLOTSET; COLOR_OPTYPEs = copyFrom.COLOR_OPTYPEs; DEPTH_OPTYPE = copyFrom.DEPTH_OPTYPE; STENCIL_OPTYPE = copyFrom.STENCIL_OPTYPE;
	}
	uint32_t BASESLOTSET;
	operationtype_tgfx* COLOR_OPTYPEs;
	operationtype_tgfx DEPTH_OPTYPE;
	operationtype_tgfx STENCIL_OPTYPE;
};
struct rtslot_create_description_vk {
	TEXTURE_VKOBJ* textures[2];
	operationtype_tgfx optype;
	drawpassload_tgfx loadtype;
	bool isUsedLater;
	vec4_tgfx clear_value;
};
struct rtslot_inheritance_descripton_vk {
	bool IS_DEPTH = false;
	operationtype_tgfx OPTYPE = operationtype_tgfx_UNUSED, OPTYPESTENCIL = operationtype_tgfx_UNUSED;
	drawpassload_tgfx LOADTYPE = drawpassload_tgfx_CLEAR, LOADTYPESTENCIL = drawpassload_tgfx_CLEAR;
};




	//Binding Model and Table Management

//These are used to initialize descriptors
//So backend won't fail because of a NULL descriptor (Although in DEBUG release, it'll complain about it)
//Texture is a 1x1 texture, buffer is 1 byte buffer, sampler is as default as possible
extern VkSampler defaultSampler;
extern VkBuffer defaultBuffer;
extern TEXTURE_VKOBJ* defaultTexture;

struct sampler_descVK {
	VkSampler sampler_obj = defaultSampler;
	std::atomic_uchar isUpdated = 0;
};
struct buffer_descVK {		//Both for BUFFER and EXT_UNIFORMBUFFER
	VkDescriptorBufferInfo info = {};
	std::atomic_uchar isUpdated = 0;
};
struct texture_descVK {		//Both for SAMPLEDTEXTURE and STORAGEIMAGE
	VkDescriptorImageInfo info = {};
	std::atomic_uchar isUpdated = 0;
};
struct DESCSET_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::DESCSET;
	static uint16_t GET_EXTRAFLAGS(DESCSET_VKOBJ* obj) { return Find_VKCONST_DESCSETID_byVkDescType(obj->DESCTYPE); }
	
	DESCSET_VKOBJ(VkDescriptorType type, uint32_t elementCount) : isUpdatedCounter(0), Stages(0), isALIVE(true), Layout(VK_NULL_HANDLE)
		, DESCTYPE(type), bufferDescELEMENTs() {
		if (DESCTYPE == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) { bufferDescELEMENTs.resize(elementCount); }
		if (DESCTYPE == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE || DESCTYPE == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) { textureDescELEMENTs.resize(elementCount); }
		if (DESCTYPE == VK_DESCRIPTOR_TYPE_SAMPLER) { samplerDescELEMENTs.resize(elementCount); }
	}
	void operator = (const DESCSET_VKOBJ& copyFrom) { 
		isALIVE.store(true);
		for (unsigned int i = 0; i < VKCONST_BUFFERING_IN_FLIGHT; i++) {
			Sets[i] = copyFrom.Sets[i];
		}
		Layout = copyFrom.Layout;
		isUpdatedCounter.store(copyFrom.isUpdatedCounter.load());
		Stages = copyFrom.Stages;
		//All of them are identical anyway
		DESCTYPE = copyFrom.DESCTYPE;
		if (DESCTYPE == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) { bufferDescELEMENTs = copyFrom.bufferDescELEMENTs; }
		if (DESCTYPE == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE || DESCTYPE == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) { textureDescELEMENTs = copyFrom.textureDescELEMENTs; }
		if (DESCTYPE == VK_DESCRIPTOR_TYPE_SAMPLER) { samplerDescELEMENTs = copyFrom.samplerDescELEMENTs; }
		bufferDescELEMENTs = copyFrom.bufferDescELEMENTs;
	}
	VkDescriptorSet Sets[VKCONST_BUFFERING_IN_FLIGHT] = {};
	VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
	//There is only one descriptor binding per descriptor set
	VkDescriptorType DESCTYPE = VK_DESCRIPTOR_TYPE_MAX_ENUM;

	union {
		VK_VECTOR_ADDONLY<texture_descVK, 1 << 20> textureDescELEMENTs;
		VK_VECTOR_ADDONLY<buffer_descVK, 1 << 20> bufferDescELEMENTs;
		VK_VECTOR_ADDONLY<sampler_descVK, 1 << 20> samplerDescELEMENTs;
	};



	//This is a atomic counter
	//It should be set 3, when a desc is updated
	//Then every frame decrease it by one (assuming there is no change in any desc of the set)
	//And when it is 0, it means all descriptor sets recieved the changes and there is no need to update
	std::atomic_uchar isUpdatedCounter = 0;
	//GLOBAL and MATTYPE/INST ones are all visible to the pipeline but USERDEFINED ones can be custom
	VkShaderStageFlags Stages = 0;
};
struct SAMPLER_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::SAMPLER;
	static uint16_t GET_EXTRAFLAGS(SAMPLER_VKOBJ* obj) { return obj->FLAGs.load(); }
	
	void operator = (const SAMPLER_VKOBJ& copyFrom) { isALIVE.store(true); Sampler = copyFrom.Sampler; FLAGs.store(copyFrom.FLAGs.load()); }
	VkSampler Sampler = VK_NULL_HANDLE;
	std::atomic<uint16_t> FLAGs = 0;	//YCbCr conversion only flag for now 
};


//Classic Rasterization Pipeline Resources

struct GRAPHICSPIPELINETYPE_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::GRAPHICSPIPELINETYPE;
	static uint16_t GET_EXTRAFLAGS(GRAPHICSPIPELINETYPE_VKOBJ* obj) { return 0; }
	void operator = (const GRAPHICSPIPELINETYPE_VKOBJ& copyFrom) {
		isALIVE.store(true);
		GFX_Subpass = copyFrom.GFX_Subpass; PipelineLayout = copyFrom.PipelineLayout; PipelineObject = copyFrom.PipelineObject;
		for (uint32_t i = 0; i < VKCONST_MAXDESCSET_PERLIST; i++) { TypeSETs[i] = UINT32_MAX; InstSETs_base[i] = UINT32_MAX; }
	}
	subdrawpass_tgfx_handle GFX_Subpass;

	VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
	VkPipeline PipelineObject = VK_NULL_HANDLE;
	uint32_t TypeSETs[VKCONST_MAXDESCSET_PERLIST], InstSETs_base[VKCONST_MAXDESCSET_PERLIST];
};
struct GRAPHICSPIPELINEINST_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::GRAPHICSPIPELINEINST;
	static uint16_t GET_EXTRAFLAGS(GRAPHICSPIPELINEINST_VKOBJ* obj) { return 0; }
	void operator = (const GRAPHICSPIPELINEINST_VKOBJ& copyFrom) {
		isALIVE.store(true);
		PROGRAM = copyFrom.PROGRAM; 
		for (unsigned int i = 0; i < VKCONST_MAXDESCSET_PERLIST; i++) { InstSETs[i] = copyFrom.InstSETs[i]; }
	}
	GRAPHICSPIPELINEINST_VKOBJ() : PROGRAM(UINT32_MAX), isALIVE(false) {
		for (unsigned int i = 0; i < VKCONST_MAXDESCSET_PERLIST; i++) { InstSETs[VKCONST_MAXDESCSET_PERLIST] = UINT32_MAX; }
	}
	uint32_t PROGRAM;
	uint32_t InstSETs[VKCONST_MAXDESCSET_PERLIST];
};
struct depthsettingsdesc_vk {
	VkBool32 ShouldWrite = VK_FALSE;
	VkCompareOp DepthCompareOP = VkCompareOp::VK_COMPARE_OP_MAX_ENUM;
	//DepthBounds Extension
	VkBool32 DepthBoundsEnable = VK_FALSE; float DepthBoundsMin = FLT_MIN, DepthBoundsMax = FLT_MAX;
};
struct stencildesc_vk {
	VkStencilOpState OPSTATE;
};
struct blendinginfo_vk {
	unsigned char COLORSLOT_INDEX = 255;
	glm::vec4 BLENDINGCONSTANTs = glm::vec4(FLT_MAX);
	VkPipelineColorBlendAttachmentState BlendState = {};
};
/* Vertex Attribute Layout Specification:
		All vertex attributes should be interleaved because there is no easy way in Vulkan for de-interleaved path.
		Vertex Attributes are created as seperate objects because this helps debug visualization of the data
		Vertex Attributes are gonna be ordered by VertexAttributeLayout's std::vector elements order and this also defines attribute's in-shader location
		That means if position attribute is second element in "Attributes" std::vector, MaterialType that using this layout uses position attribute at location = 1 instead of 0.
*/
struct VERTEXATTRIBLAYOUT_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::VERTEXATTRIB;
	static uint16_t GET_EXTRAFLAGS(VERTEXATTRIBLAYOUT_VKOBJ* obj) { return 0; }
	
	void operator = (const VERTEXATTRIBLAYOUT_VKOBJ& copyFrom) {
		isALIVE.store(true);
		Attributes = copyFrom.Attributes; Attribute_Number = copyFrom.Attribute_Number; size_perVertex = copyFrom.size_perVertex;
		BindingDesc = copyFrom.BindingDesc; AttribDescs = copyFrom.AttribDescs; AttribDesc_Count = copyFrom.AttribDesc_Count; PrimitiveTopology = copyFrom.PrimitiveTopology;
	}
	datatype_tgfx* Attributes;
	unsigned int Attribute_Number, size_perVertex;
	
	VkVertexInputBindingDescription BindingDesc;	//Currently, only one binding is supported because I didn't understand bindings properly.
	VkVertexInputAttributeDescription* AttribDescs;
	VkPrimitiveTopology PrimitiveTopology;
	unsigned char AttribDesc_Count;
};
struct VERTEXBUFFER_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::VERTEXBUFFER;
	static uint16_t GET_EXTRAFLAGS(VERTEXBUFFER_VKOBJ* obj) { return 0; }
	
	void operator = (const VERTEXBUFFER_VKOBJ& copyFrom) { isALIVE.store(true); VERTEX_COUNT = copyFrom.VERTEX_COUNT; Layout = copyFrom.Layout; Block = copyFrom.Block; }
	unsigned int VERTEX_COUNT;
	uint32_t Layout;
	memoryblock_vk Block;
};
struct INDEXBUFFER_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::INDEXBUFFER;
	static uint16_t GET_EXTRAFLAGS(INDEXBUFFER_VKOBJ* obj) { return 0; }
	void operator = (const INDEXBUFFER_VKOBJ& copyFrom) { isALIVE.store(true); DATATYPE = copyFrom.DATATYPE; IndexCount = copyFrom.IndexCount; Block = copyFrom.Block; }
	VkIndexType DATATYPE;
	VkDeviceSize IndexCount = 0;
	memoryblock_vk Block;
};



	//Compute Resources

struct COMPUTETYPE_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::COMPUTETYPE;
	static uint16_t GET_EXTRAFLAGS(COMPUTETYPE_VKOBJ* obj) { return 0; }
	void operator = (const COMPUTETYPE_VKOBJ& copyFrom) { isALIVE.store(true); 
	PipelineObject = copyFrom.PipelineObject; PipelineLayout = copyFrom.PipelineLayout; 
	for (unsigned int i = 0; i < VKCONST_MAXDESCSET_PERLIST; i++) { TypeSETs[i] = UINT32_MAX; InstSETs[i] = UINT32_MAX; }
	}
	VkPipeline PipelineObject = VK_NULL_HANDLE;
	VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
	uint32_t TypeSETs[VKCONST_MAXDESCSET_PERLIST], InstSETs[VKCONST_MAXDESCSET_PERLIST];
};
struct COMPUTEINST_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::COMPUTEINST;
	static uint16_t GET_EXTRAFLAGS(COMPUTEINST_VKOBJ* obj) { return 0; }
	void operator = (const COMPUTEINST_VKOBJ& copyFrom) {
		isALIVE.store(true);
		PROGRAM = copyFrom.PROGRAM; for (unsigned int i = 0; i < VKCONST_MAXDESCSET_PERLIST; i++) { InstSETs[i] = UINT32_MAX; }	}
	uint32_t PROGRAM = UINT32_MAX;
	uint32_t InstSETs[VKCONST_MAXDESCSET_PERLIST];
};

