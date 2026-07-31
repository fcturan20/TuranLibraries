#pragma once
#include <atomic>
#include <glm/glm.hpp>
#include "vk_core.h"
#include "vk_includes.h"

namespace TGFX
{
namespace Vulkan
{

// Represents how it's bind to a heap
struct VMemoryBlock
{
	TGfxHeap m_heap;
	VkDeviceSize vk_offset;
	static const VMemoryBlock& GETINVALID()
	{
		VMemoryBlock invalid;
		invalid.m_heap = nullptr;
		invalid.vk_offset = UINT64_MAX;
		return invalid;
	}
};
//
struct VMemoryRequirements
{
	VkMemoryRequirements vk_memReqs;
	bool prefersDedicatedAlloc : 1, requiresDedicatedAlloc : 1;
	static const VMemoryRequirements& GETINVALID()
	{
		VMemoryRequirements invalid;
		invalid.prefersDedicatedAlloc = false;
		invalid.requiresDedicatedAlloc = false;
		invalid.vk_memReqs.memoryTypeBits = 0;
		invalid.vk_memReqs.size = 0;
		invalid.vk_memReqs.alignment = UINT64_MAX;
		return invalid;
	}
};

// Classic Memory Resources

struct Texture : public VkObjectBase<Texture, TGfxTexture, VkObjTypes::TEXTURE>, public GpuObject
{
	Texture(GPU* gpu) : vk_image(gpu->ReferenceManager), vk_imageView(gpu->ReferenceManager), GpuObject(gpu) {}
	uint16_t GetExtraFlags(Texture* obj) { return 0; }

	TGfxUVec2 Size;
	TU1 MipCount;
	TGfxTextureChannels m_channels;
	TGfxTextureDimensions m_dim;
	VMemoryBlock m_memBlock = VMemoryBlock::GETINVALID();
	VMemoryRequirements m_memReqs = VMemoryRequirements::GETINVALID();

	VkImageHnd vk_image;
	VkImageViewHnd vk_imageView;
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(Texture, Vk)

struct Buffer : public VkObjectBase<Buffer, TGfxBuffer, VkObjTypes::BUFFER>, public GpuObject
{
	Buffer(GPU* gpu) : GpuObject(gpu), vk_buffer(gpu->ReferenceManager) {}
	uint16_t GetExtraFlags() { return 0; }

	VMemoryBlock m_memBlock;
	VMemoryRequirements m_memReqs;
	uint64_t m_intendedSize = UINT64_MAX;

	VkBufferHnd vk_buffer;
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(Buffer, Vk)

// Framebuffer RT Slot Management

struct colorslot_vk
{
	Texture* RT;
	TGfxRasterPassLoad LOADSTATE;
	bool IS_USED_LATER;
	TGfxOperationType RT_OPERATIONTYPE;
	glm::vec4 CLEAR_COLOR;
	std::atomic_bool IsChanged = false;
};
struct depthstencilslot_vk
{
	Texture* RT;
	TGfxRasterPassLoad DEPTH_LOAD, STENCIL_LOAD;
	bool IS_USED_LATER;
	TGfxOperationType DEPTH_OPTYPE, STENCIL_OPTYPE;
	glm::vec2 CLEAR_COLOR;
	std::atomic_bool IsChanged = false;
};
struct rtslots_vk
{
	colorslot_vk* COLOR_SLOTs = nullptr;
	unsigned char COLORSLOTs_COUNT = 0;
	depthstencilslot_vk* DEPTHSTENCIL_SLOT = nullptr; // There is one, but there may not be a Depth Slot. So if there is
													  // no, then this is nullptr.
	// Unused Depth and NoDepth are different. Unused Depth means RenderPass does have one but current
	// Subpass doesn't use, but NoDepth means RenderPass doesn't have one!
	std::atomic_bool IsChanged = false;

	void operator=(const rtslots_vk& copyFrom)
	{
		IsChanged.store(copyFrom.IsChanged.load());
		DEPTHSTENCIL_SLOT = copyFrom.DEPTHSTENCIL_SLOT;
		COLOR_SLOTs = copyFrom.COLOR_SLOTs;
		COLORSLOTs_COUNT = copyFrom.COLORSLOTs_COUNT;
	}
};

struct SubRasterpass : public VkObjectBase<SubRasterpass, TGfxSubRasterpass, VkObjTypes::SUBRASTERPASS>,
					   public GpuObject
{
	SubRasterpass(GPU* gpu) : GpuObject(gpu), vk_renderPass(gpu->ReferenceManager) {}
	uint16_t GetExtraFlags() { return 0; }

	uint32_t m_subpassIdx;
	VkRenderPassHnd vk_renderPass; // It's same across all subpasses
	bool isDepthAttachment = false;
	// Extra information to check raster pipeline compilations without relying on validation layer
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(SubRasterpass, Vk)

struct Sampler : public VkObjectBase<Sampler, TGfxSampler, VkObjTypes::SAMPLER>, public GpuObject
{
	Sampler(GPU* gpu) : GpuObject(gpu), vk_sampler(gpu->ReferenceManager) {}
	uint16_t GetExtraFlags() { return Flags; }

	VkSamplerHnd vk_sampler;
	TU1 Flags = 0; // YCbCr conversion only flag for now
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(Sampler, Vk)

/////////////////////////////////////////////
//				PIPELINE RESOURCES
/////////////////////////////////////////////

struct Pipeline : public VkObjectBase<Pipeline, TGfxPipeline, VkObjTypes::PIPELINE>, public GpuObject
{
	Pipeline(GPU* gpu) : GpuObject(gpu), vk_layout(gpu->ReferenceManager), vk_object(gpu->ReferenceManager) {}
	uint16_t GetExtraFlags() { return vk_type; }

	VkPipelineLayoutHnd vk_layout;
	VkPipelineHnd vk_object;
	VkPipelineBindPoint vk_type = VK_PIPELINE_BIND_POINT_MAX_ENUM;

	VkFormat vk_colorAttachmentFormats[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT] = {};
	VkFormat vk_depthAttachmentFormat = {};
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(Pipeline, Vk)

struct ShaderSource : VkObjectBase<ShaderSource, TGfxShaderSource, VkObjTypes::SHADERSOURCE>, public GpuObject
{
	ShaderSource(GPU* gpu) : GpuObject(gpu), Module(gpu->ReferenceManager) {}
	uint16_t GetExtraFlags() { return Stage; }

	VkShaderModuleHnd Module;
	TGfxShaderStage Stage;
	void* SourceCode = nullptr;
	unsigned int SourceCodeSize = 0;
	TGfxGpu Gpu;
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(ShaderSource, Vk)

struct depthsettingsdesc_vk
{
	VkBool32 ShouldWrite = VK_FALSE;
	VkCompareOp DepthCompareOP = VkCompareOp::VK_COMPARE_OP_MAX_ENUM;
	// DepthBounds Extension
	VkBool32 DepthBoundsEnable = VK_FALSE;
	float DepthBoundsMin = FLT_MIN, DepthBoundsMax = FLT_MAX;
};
struct stencildesc_vk
{
	VkStencilOpState OPSTATE;
};
struct blendinginfo_vk
{
	unsigned char COLORSLOT_INDEX = 255;
	glm::vec4 BLENDINGCONSTANTs = glm::vec4(FLT_MAX);
	VkPipelineColorBlendAttachmentState BlendState = {};
};

TCORE_DEFINE_HANDLE(VkVertexAttrib);
VkConstU4 VKCONST_MAXVERTEXATTRIBCOUNT = 16, VKCONST_MAXVERTEXBINDINGCOUNT = 4;
struct VertexAttributeLayout : public VkObjectBase<VertexAttributeLayout, VkVertexAttrib, VkObjTypes::VERTEXATTRIB>
{
	uint16_t GetExtraFlags() { return 0; }

	VkVertexInputBindingDescription bindingDescs[VKCONST_MAXVERTEXBINDINGCOUNT];
	VkVertexInputAttributeDescription attribDescs[VKCONST_MAXVERTEXATTRIBCOUNT];
	unsigned char attribDescsCount, bindingDescsCount;
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(VertexAttributeLayout, Vk)

struct Heap : public VkObjectBase<Heap, TGfxHeap, VkObjTypes::HEAP>, public GpuObject
{
	Heap(GPU* gpu) : GpuObject(gpu), vk_memoryHnd(gpu->ReferenceManager) {}
	uint16_t GetExtraFlags()
	{
		// assert(0 && "GPU index & memTypeIndex should be passed as extra flag");
		return 0;
	}
	unsigned long long m_size;

	VkDeviceMemoryHnd vk_memoryHnd;
	unsigned int vk_memoryTypeIndex;
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(Heap, Vk)

struct Fence : VkObjectBase<Fence, TGfxFence, VkObjTypes::FENCE>, public GpuObject
{
	Fence(GPU* gpu) : GpuObject(gpu), FenceHnd(gpu->ReferenceManager), SemaphoreHnd(gpu->ReferenceManager) {}
	VkFenceHnd FenceHnd;
	VkSemaphoreHnd SemaphoreHnd;
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(Fence, Vk)

struct Framebuffer : public GpuObject
{
	Framebuffer(GPU* gpu) : GpuObject(gpu), vk_framebuffer(gpu->ReferenceManager) {}
	uint16_t GetExtraFlags(Framebuffer* obj) { return 0; }

	TGfxTexture m_textures[kMaxRtSlotCount];
	VkFramebufferHnd vk_framebuffer;
};

} // namespace Vulkan
} // namespace TGFX