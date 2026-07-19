#pragma once
#include <stdio.h>
#include <vulkan/vulkan.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <iostream>
#include <mutex>

#include <TGfxDeclarations.h>
#include <TGfxStructs.h>
#include <TGfxCore.h>

#include "vk_predefinitions.h"

// Some algorithms and data structures to help in C++ (like threadlocalvector)

template <typename T>
class atomic
{
	std::atomic<T> data;

public:
	// Returns the old value
	uint64_t DirectAdd(const uint64_t& add) { return data.fetch_add(add); }
	// Returns the old value
	uint64_t DirectSubtract(const uint64_t& sub) { return data.fetch_sub(sub); }
	void DirectStore(const uint64_t& Store) { data.store(Store); }
	uint64_t DirectLoad() const { return data.load(); }

	// Deep Sleeping: The thread won't be available soon enough and application will fail at some
	// point (or be buggy) because condition's not gonna be met soon enough. By the way, it keeps
	// yielding at that time. This situation is so dangerous because maybe other threads's keep
	// creating jobs that depends on this job. In such a situation, 2 cases possible; Case 1)
	// Developers were not careful enough to do a "WaitForTheJob()" and the following operations was
	// depending on the job's execution so all the following operations are wrong because job is not
	// finished executing because of Deep Sleeping. Case 2) Developers were careful enough but this
	// means developers' design lacks of a concept that covers not-meeting the condition. For example;
	// atomic::LimitedAdd_strong() waits until addition happens. But if addition is not possible for
	// the rest of the application, the thread'll keep yielding until termination. Or addition happens
	// late enough that Case 1 occurs, which means following execution is wrong.

	// If you want to try to do addition but want to check if it is possible in a lock-free way, it is
	// this There are 2 cases this function may return false; 1) Your addition is already exceeds the
	// maxlimit even there isn't any concurrent addition 2) Your addition may not exceed if there
	// wouldn't be any concurrent addition If you think your case is '1', my advice is that you should
	// design your application such that, the function that calls "LimitedAdd_weak()" may fail and
	// user is aware about that You should predict cases like '2' at design level and should change
	// your job scheduling accordingly But if you didn't and need a tricky solution, you can use
	// LimitedAdd_strong(). But be aware of long waits. Also for some possible Deep Sleeping because
	// data won't be small enough to do the addition By design level, I mean that; 1) You should know
	// when there won't be any concurrent operations on the data and read it when it's time 2) You
	// should predict the max 'add' and min value (and their timings) when concurrent operations will
	// occur 3) You should reduce concurrent operations or schedule your concurrent operations such
	// that LimitedAdd_strong()'ll never introduce late awakening (Never awakening) or concurrent
	// operations'll use job waiting instead of this lock-free system
	bool LimitedAdd_weak(const uint64_t& add, const uint64_t& maxlimit)
	{
		uint64_t x = data.load();
		if (x + add > maxlimit || // Addition is bigger
			x + add < x			  // UINT overflow
		)
		{
			return false;
		}
		if (!data.compare_exchange_strong(x, x + add))
		{
			return LimitedSubtract_weak(add, maxlimit);
		}
		return true;
	}
	// You should use this function only for this case, because this is not performant;
	// The value is very close to the limit so you are gonna reduce the value some nanoseconds later
	// from other threads And when the value is reduced, you want addition to happen immediately
	//"Immediately" here means after this_thread::yield()
	// If you pay attention, this function doesn't return bool. Because this will block the thread
	// until addition is possible So
	void LimitedAdd_strong(const uint64_t& add, const uint64_t& maxlimit)
	{
		while (true)
		{
			if (LimitedAdd_weak(add, maxlimit))
			{
				return;
			}
			std::this_thread::yield();
		}
	}
	// Similar but the reverse of the LimitedAdd_weak()
	bool LimitedSubtract_weak(const uint64_t& subtract, const uint64_t& minlimit)
	{
		uint64_t x = data.load();
		if (x - subtract < minlimit || // Subtraction is bigger
			x - subtract > x		   // UINT overflow
		)
		{
			return false;
		}
		if (!data.compare_exchange_strong(x, x - subtract))
		{
			return LimitedSubtract_weak(subtract, minlimit);
		}
		return true;
	}
	// Similar but the reverse of the LimitedAdd_strong()
	void LimitedSubtract_strong(const uint64_t& subtract, const uint64_t& minlimit)
	{
		while (true)
		{
			if (LimitedSubtract_weak(subtract, minlimit))
			{
				return;
			}
			std::this_thread::yield();
		}
	}
};

namespace TGFX
{
namespace Vulkan
{

inline unsigned char GetByteSizeOf_TextureChannels(TGfxTextureChannels channeltype)
{
	switch (channeltype)
	{
	case TGFX_TEXTURE_CHANNELS_R8B:
	case TGFX_TEXTURE_CHANNELS_R8UB: return 1;
	case TGFX_TEXTURE_CHANNELS_RGB8B:
	case TGFX_TEXTURE_CHANNELS_RGB8UB: return 3;
	case TGFX_TEXTURE_CHANNELS_D24S8:
	case TGFX_TEXTURE_CHANNELS_D32:
	case TGFX_TEXTURE_CHANNELS_RGBA8B:
	case TGFX_TEXTURE_CHANNELS_RGBA8UB:
	case TGFX_TEXTURE_CHANNELS_BGRA8UB:
	case TGFX_TEXTURE_CHANNELS_BGRA8UNORM:
	case TGFX_TEXTURE_CHANNELS_R32F:
	case TGFX_TEXTURE_CHANNELS_R32I:
	case TGFX_TEXTURE_CHANNELS_R32UI: return 4;
	case TGFX_TEXTURE_CHANNELS_RA32F:
	case TGFX_TEXTURE_CHANNELS_RA32I:
	case TGFX_TEXTURE_CHANNELS_RA32UI: return 8;
	case TGFX_TEXTURE_CHANNELS_RGB32F:
	case TGFX_TEXTURE_CHANNELS_RGB32I:
	case TGFX_TEXTURE_CHANNELS_RGB32UI: return 12;
	case TGFX_TEXTURE_CHANNELS_RGBA32F:
	case TGFX_TEXTURE_CHANNELS_RGBA32I:
	case TGFX_TEXTURE_CHANNELS_RGBA32UI: return 16;
	default: vkPrint(49); return 0;
	}
}
inline VkFormat findDataType(TGfxDataType datatype)
{
	switch (datatype)
	{
	case TGFX_DATATYPE_FVEC2: return VK_FORMAT_R32G32_SFLOAT;
	case TGFX_DATATYPE_FVEC3: return VK_FORMAT_R32G32B32_SFLOAT;
	case TGFX_DATATYPE_FVEC4: return VK_FORMAT_R32G32B32A32_SFLOAT;
	default: vkPrint(49); return VK_FORMAT_UNDEFINED;
	}
}
inline VkFormat findFormatVk(TGfxTextureChannels channels)
{
	switch (channels)
	{
	case TGFX_TEXTURE_CHANNELS_BGRA8UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
	case TGFX_TEXTURE_CHANNELS_BGRA8UB: return VK_FORMAT_B8G8R8A8_UINT;
	case TGFX_TEXTURE_CHANNELS_R8B: return VK_FORMAT_R8_SINT;
	case TGFX_TEXTURE_CHANNELS_RGBA8UB: return VK_FORMAT_R8G8B8A8_UINT;
	case TGFX_TEXTURE_CHANNELS_RGBA8B: return VK_FORMAT_R8G8B8A8_SINT;
	case TGFX_TEXTURE_CHANNELS_RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
	case TGFX_TEXTURE_CHANNELS_RGBA32I: return VK_FORMAT_R32G32B32A32_SINT;
	case TGFX_TEXTURE_CHANNELS_RGBA32UI: return VK_FORMAT_R32G32B32A32_UINT;
	case TGFX_TEXTURE_CHANNELS_RGB8UB: return VK_FORMAT_R8G8B8_UINT;
	case TGFX_TEXTURE_CHANNELS_D32: return VK_FORMAT_D32_SFLOAT;
	case TGFX_TEXTURE_CHANNELS_D24S8: return VK_FORMAT_D24_UNORM_S8_UINT;
	case TGFX_TEXTURE_CHANNELS_BGRA8SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
	case TGFX_TEXTURE_CHANNELS_RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
	case TGFX_TEXTURE_CHANNELS_A2B10G10R10_UNORM: return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
	case TGFX_TEXTURE_CHANNELS_RGBA8SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
	case TGFX_TEXTURE_CHANNELS_UNDEF: return VK_FORMAT_UNDEFINED;
	case TGFX_TEXTURE_CHANNELS_UNDEF2:
	default: vkPrint(49); return VK_FORMAT_UNDEFINED;
	}
}
inline TGfxTextureChannels findTextureChannelsTgfx(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_B8G8R8A8_UNORM: return TGFX_TEXTURE_CHANNELS_BGRA8UNORM;
	case VK_FORMAT_B8G8R8A8_UINT: return TGFX_TEXTURE_CHANNELS_BGRA8UB;
	case VK_FORMAT_R8G8B8A8_UINT: return TGFX_TEXTURE_CHANNELS_RGBA8UB;
	case VK_FORMAT_R8G8B8A8_SINT: return TGFX_TEXTURE_CHANNELS_RGBA8B;
	case VK_FORMAT_R32G32B32A32_SFLOAT: return TGFX_TEXTURE_CHANNELS_RGBA32F;
	case VK_FORMAT_R32G32B32A32_SINT: return TGFX_TEXTURE_CHANNELS_RGBA32I;
	case VK_FORMAT_R32G32B32A32_UINT: return TGFX_TEXTURE_CHANNELS_RGBA32UI;
	case VK_FORMAT_R8G8B8_UINT: return TGFX_TEXTURE_CHANNELS_RGB8UB;
	case VK_FORMAT_D32_SFLOAT: return TGFX_TEXTURE_CHANNELS_D32;
	case VK_FORMAT_D24_UNORM_S8_UINT: return TGFX_TEXTURE_CHANNELS_D24S8;
	case VK_FORMAT_B8G8R8A8_SRGB: return TGFX_TEXTURE_CHANNELS_BGRA8SRGB;
	case VK_FORMAT_R16G16B16A16_SFLOAT: return TGFX_TEXTURE_CHANNELS_RGBA16F;
	case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return TGFX_TEXTURE_CHANNELS_A2B10G10R10_UNORM;
	case VK_FORMAT_R8G8B8A8_UNORM: return TGFX_TEXTURE_CHANNELS_RGBA8UNORM;
	case VK_FORMAT_R8G8B8A8_SRGB: return TGFX_TEXTURE_CHANNELS_RGBA8SRGB;
	default: vkPrint(49); return TGFX_TEXTURE_CHANNELS_R8B;
	}
}
inline TGfxDataType findTextureDataType(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_D32_SFLOAT:
	case VK_FORMAT_R32G32B32A32_SFLOAT:
	case VK_FORMAT_B8G8R8A8_SRGB:
	case VK_FORMAT_R16G16B16A16_SFLOAT:
	case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
	case VK_FORMAT_B8G8R8A8_UNORM: return TGFX_DATATYPE_F32;

	case VK_FORMAT_B8G8R8A8_UINT:
	case VK_FORMAT_R8G8B8A8_UINT:
	case VK_FORMAT_R8G8B8_UINT:
	case VK_FORMAT_R32G32B32A32_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT: return TGFX_DATATYPE_U32;

	case VK_FORMAT_R8G8B8A8_SINT:
	case VK_FORMAT_R8_SINT:
	case VK_FORMAT_R32G32B32A32_SINT: return TGFX_DATATYPE_I32;
	default: vkPrint(49); return TGFX_DATATYPE_UNDEFINED;
	}
}
inline VkDescriptorType findDescTypeVk(TGfxShaderDescriptorType desc)
{
	switch (desc)
	{
	case TGFX_SHADERDESCRIPTORTYPE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case TGFX_SHADERDESCRIPTORTYPE_SAMPLEDTEXTURE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case TGFX_SHADERDESCRIPTORTYPE_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
	case TGFX_SHADERDESCRIPTORTYPE_STORAGEIMAGE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case TGFX_SHADERDESCRIPTORTYPE_EXT_UNIFORMBUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	default: vkPrint(49); return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}
inline TGfxShaderDescriptorType findDescTypeTgfx(VkDescriptorType desc)
{
	switch (desc)
	{
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return TGFX_SHADERDESCRIPTORTYPE_BUFFER;
	case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return TGFX_SHADERDESCRIPTORTYPE_SAMPLEDTEXTURE;
	case VK_DESCRIPTOR_TYPE_SAMPLER: return TGFX_SHADERDESCRIPTORTYPE_SAMPLER;
	case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return TGFX_SHADERDESCRIPTORTYPE_STORAGEIMAGE;
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return TGFX_SHADERDESCRIPTORTYPE_EXT_UNIFORMBUFFER;
	default: vkPrint(49); return (TGfxShaderDescriptorType)UINT64_MAX;
	}
}
inline VkSamplerAddressMode findAddressModeVk(TGfxTextureWrapping Wrapping)
{
	switch (Wrapping)
	{
	case TGFX_TEXTURE_WRAPPING_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case TGFX_TEXTURE_WRAPPING_MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case TGFX_TEXTURE_WRAPPING_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	default: vkPrint(49); return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
	}
}
inline VkFilter findFilterVk(TGfxTextureMipmapFilter filter)
{
	switch (filter)
	{
	case TGFX_TEXTURE_MIPMAPFILTER_LINEAR_FROM_1MIP:
	case TGFX_TEXTURE_MIPMAPFILTER_LINEAR_FROM_2MIP: return VK_FILTER_LINEAR;
	case TGFX_TEXTURE_MIPMAPFILTER_NEAREST_FROM_1MIP:
	case TGFX_TEXTURE_MIPMAPFILTER_NEAREST_FROM_2MIP: return VK_FILTER_NEAREST;
	default: vkPrint(49); return VK_FILTER_MAX_ENUM;
	}
}
inline VkSamplerMipmapMode findMipmapModeVk(TGfxTextureMipmapFilter filter)
{
	switch (filter)
	{
	case TGFX_TEXTURE_MIPMAPFILTER_LINEAR_FROM_2MIP:
	case TGFX_TEXTURE_MIPMAPFILTER_NEAREST_FROM_2MIP: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	case TGFX_TEXTURE_MIPMAPFILTER_LINEAR_FROM_1MIP:
	case TGFX_TEXTURE_MIPMAPFILTER_NEAREST_FROM_1MIP: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	default: vkPrint(49); return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
	}
}
inline VkCullModeFlags findCullModeVk(TGfxCullMode mode)
{
	switch (mode)
	{
	case TGFX_CULLMODE_OFF: return VK_CULL_MODE_NONE;
	case TGFX_CULLMODE_BACK: return VK_CULL_MODE_BACK_BIT;
	case TGFX_CULLMODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
	default: vkPrint(49); return VK_CULL_MODE_NONE;
	}
}
inline VkPolygonMode findPolygonModeVk(TGfxPolygonMode mode)
{
	switch (mode)
	{
	case TGFX_POLYGONMODE_FILL: return VK_POLYGON_MODE_FILL;
	case TGFX_POLYGONMODE_LINE: return VK_POLYGON_MODE_LINE;
	case TGFX_POLYGONMODE_POINT: return VK_POLYGON_MODE_POINT;
	default: vkPrint(49); return VK_POLYGON_MODE_MAX_ENUM;
	}
}
inline VkPrimitiveTopology Find_PrimitiveTopology_byGFXVertexListType(TGfxVertexListType vertextype)
{
	switch (vertextype)
	{
	case TGFX_VERTEXLISTTYPE_TRIANGLELIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	default: vkPrint(49); return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	}
}
inline VkLogicOp findLogicOpVk()
{
	vkPrint(49);
	return VK_LOGIC_OP_MAX_ENUM;
}
inline VkIndexType Find_IndexType_byGFXDATATYPE(TGfxDataType datatype)
{
	switch (datatype)
	{
	case TGFX_DATATYPE_U32: return VK_INDEX_TYPE_UINT32;
	case TGFX_DATATYPE_U16: return VK_INDEX_TYPE_UINT16;
	default: vkPrint(49); return VK_INDEX_TYPE_MAX_ENUM;
	}
}
inline VkCompareOp findCompareOpVk(TGfxCompare test)
{
	switch (test)
	{
	case TGFX_COMPARE_NEVER: return VK_COMPARE_OP_NEVER;
	case TGFX_COMPARE_ALWAYS: return VK_COMPARE_OP_ALWAYS;
	case TGFX_COMPARE_GEQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case TGFX_COMPARE_GREATER: return VK_COMPARE_OP_GREATER;
	case TGFX_COMPARE_LEQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
	case TGFX_COMPARE_LESS: return VK_COMPARE_OP_LESS;
	default: vkPrint(49); return VK_COMPARE_OP_MAX_ENUM;
	}
}
inline void Find_DepthMode_byGFXDepthMode(TGfxDepthMode mode, VkBool32& ShouldTest, VkBool32& ShouldWrite)
{
	switch (mode)
	{
	case TGFX_DEPTHMODE_READ_WRITE:
		ShouldTest = VK_TRUE;
		ShouldWrite = VK_TRUE;
		break;
	case TGFX_DEPTHMODE_READ_ONLY:
		ShouldTest = VK_TRUE;
		ShouldWrite = VK_FALSE;
		break;
	case TGFX_DEPTHMODE_OFF:
		ShouldTest = VK_FALSE;
		ShouldWrite = VK_FALSE;
		break;
	default: vkPrint(49); break;
	}
}
inline VkAttachmentLoadOp findLoadTypeVk(TGfxRasterPassLoad load)
{
	switch (load)
	{
	case TGFX_RASTERPASSLOAD_CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case TGFX_RASTERPASSLOAD_DISCARD: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	case TGFX_RASTERPASSLOAD_LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD;
	case TGFX_RASTERPASSLOAD_NONE: return VK_ATTACHMENT_LOAD_OP_NONE_EXT;
	default: vkPrint(49); return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
	}
}
inline VkAttachmentStoreOp findStoreTypeVk(TGfxRasterPassStore store)
{
	switch (store)
	{
	case TGFX_RASTERPASSSTORE_STORE: return VK_ATTACHMENT_STORE_OP_STORE;
	case TGFX_RASTERPASSSTORE_DISCARD: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	case TGFX_RASTERPASSSTORE_NONE: return VK_ATTACHMENT_STORE_OP_NONE;
	default: vkPrint(49); return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
	}
}
inline VkStencilOp findStencilOpVk(TGfxStencilOp op)
{
	switch (op)
	{
	case TGFX_STENCILOP_DONT_CHANGE: return VK_STENCIL_OP_KEEP;
	case TGFX_STENCILOP_SET_ZERO: return VK_STENCIL_OP_ZERO;
	case TGFX_STENCILOP_CHANGE: return VK_STENCIL_OP_REPLACE;
	case TGFX_STENCILOP_CLAMPED_INCREMENT: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
	case TGFX_STENCILOP_WRAPPED_INCREMENT: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
	case TGFX_STENCILOP_CLAMPED_DECREMENT: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
	case TGFX_STENCILOP_WRAPPED_DECREMENT: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
	case TGFX_STENCILOP_BITWISE_INVERT: return VK_STENCIL_OP_INVERT;
	default: vkPrint(49); return VK_STENCIL_OP_KEEP;
	}
}
inline VkBlendOp findBlendOpVk(TGfxBlendMode mode)
{
	switch (mode)
	{
	case TGFX_BLENDMODE_ADDITIVE: return VK_BLEND_OP_ADD;
	case TGFX_BLENDMODE_SUBTRACTIVE: return VK_BLEND_OP_SUBTRACT;
	case TGFX_BLENDMODE_SUBTRACTIVE_SWAPPED: return VK_BLEND_OP_REVERSE_SUBTRACT;
	case TGFX_BLENDMODE_MIN: return VK_BLEND_OP_MIN;
	case TGFX_BLENDMODE_MAX: return VK_BLEND_OP_MAX;
	default: vkPrint(49); return VK_BLEND_OP_MAX_ENUM;
	}
}
inline VkBlendFactor findBlendFactorVk(TGfxBlendFactor factor)
{
	switch (factor)
	{
	case TGFX_BLENDFACTOR_ONE: return VK_BLEND_FACTOR_ONE;
	case TGFX_BLENDFACTOR_ZERO: return VK_BLEND_FACTOR_ZERO;
	case TGFX_BLENDFACTOR_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
	case TGFX_BLENDFACTOR_SRC_1MINUSCOLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case TGFX_BLENDFACTOR_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
	case TGFX_BLENDFACTOR_SRC_1MINUSALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case TGFX_BLENDFACTOR_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
	case TGFX_BLENDFACTOR_DST_1MINUSCOLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case TGFX_BLENDFACTOR_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
	case TGFX_BLENDFACTOR_DST_1MINUSALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	case TGFX_BLENDFACTOR_CONST_COLOR: return VK_BLEND_FACTOR_CONSTANT_COLOR;
	case TGFX_BLENDFACTOR_CONST_1MINUSCOLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
	case TGFX_BLENDFACTOR_CONST_ALPHA: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
	case TGFX_BLENDFACTOR_CONST_1MINUSALPHA: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
	default: vkPrint(49); return VK_BLEND_FACTOR_MAX_ENUM;
	}
}
inline void Fill_ComponentMapping_byCHANNELs(TGfxTextureChannels channels, VkComponentMapping& mapping)
{
	switch (channels)
	{
	case TGFX_TEXTURE_CHANNELS_D32:
	case TGFX_TEXTURE_CHANNELS_D24S8:
	case TGFX_TEXTURE_CHANNELS_BGRA8UB:
	case TGFX_TEXTURE_CHANNELS_BGRA8UNORM:
	case TGFX_TEXTURE_CHANNELS_RGBA32F:
	case TGFX_TEXTURE_CHANNELS_RGBA32UI:
	case TGFX_TEXTURE_CHANNELS_RGBA32I:
	case TGFX_TEXTURE_CHANNELS_RGBA8UB:
	case TGFX_TEXTURE_CHANNELS_RGBA8B:
		mapping.r = VK_COMPONENT_SWIZZLE_R;
		mapping.g = VK_COMPONENT_SWIZZLE_G;
		mapping.b = VK_COMPONENT_SWIZZLE_B;
		mapping.a = VK_COMPONENT_SWIZZLE_A;
		return;
	case TGFX_TEXTURE_CHANNELS_RGB32F:
	case TGFX_TEXTURE_CHANNELS_RGB32UI:
	case TGFX_TEXTURE_CHANNELS_RGB32I:
	case TGFX_TEXTURE_CHANNELS_RGB8UB:
	case TGFX_TEXTURE_CHANNELS_RGB8B:
		mapping.r = VK_COMPONENT_SWIZZLE_R;
		mapping.g = VK_COMPONENT_SWIZZLE_G;
		mapping.b = VK_COMPONENT_SWIZZLE_B;
		mapping.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		return;
	case TGFX_TEXTURE_CHANNELS_RA32F:
	case TGFX_TEXTURE_CHANNELS_RA32UI:
	case TGFX_TEXTURE_CHANNELS_RA32I:
	case TGFX_TEXTURE_CHANNELS_RA8UB:
	case TGFX_TEXTURE_CHANNELS_RA8B:
		mapping.r = VK_COMPONENT_SWIZZLE_R;
		mapping.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		mapping.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		mapping.a = VK_COMPONENT_SWIZZLE_A;
		return;
	case TGFX_TEXTURE_CHANNELS_R32F:
	case TGFX_TEXTURE_CHANNELS_R32UI:
	case TGFX_TEXTURE_CHANNELS_R32I:
	case TGFX_TEXTURE_CHANNELS_R8UB:
	case TGFX_TEXTURE_CHANNELS_R8B:
		mapping.r = VK_COMPONENT_SWIZZLE_R;
		mapping.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		mapping.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		mapping.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		return;
	default: break;
	}
}
inline void findSubpassAccessPattern(TGfxSubDrawPassAccess access,
									 bool isSource,
									 VkPipelineStageFlags& stageflag,
									 VkAccessFlags& accessflag)
{
	switch (access)
	{
	case TGFX_SUBDRAWPASSACCESS_ALLCOMMANDS:
		if (isSource)
		{
			stageflag |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}
		else
		{
			stageflag |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
		break;
	case TGFX_SUBDRAWPASSACCESS_INDEX_READ:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		accessflag |= VK_ACCESS_INDEX_READ_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_VERTEXATTRIB_READ:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		accessflag |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_VERTEXUBUFFER_READONLY:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		accessflag |= VK_ACCESS_UNIFORM_READ_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_VERTEXSBUFFER_READONLY:
	case TGFX_SUBDRAWPASSACCESS_VERTEXSAMPLED_READONLY:
	case TGFX_SUBDRAWPASSACCESS_VERTEXIMAGE_READONLY:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_VERTEXSBUFFER_READWRITE:
	case TGFX_SUBDRAWPASSACCESS_VERTEXIMAGE_READWRITE:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_VERTEXIMAGE_WRITEONLY:
	case TGFX_SUBDRAWPASSACCESS_VERTEXSBUFFER_WRITEONLY:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_VERTEXINPUTS_READONLY:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_VERTEXINPUTS_READWRITE:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_VERTEXINPUTS_WRITEONLY:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
		break;

	case TGFX_SUBDRAWPASSACCESS_EARLY_Z_READ:
		stageflag |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_EARLY_Z_READWRITE:
		stageflag |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_EARLY_Z_WRITEONLY:
		stageflag |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_FRAGMENTUBUFFER_READONLY:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_UNIFORM_READ_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_FRAGMENTSBUFFER_READONLY:
	case TGFX_SUBDRAWPASSACCESS_FRAGMENTSAMPLED_READONLY:
	case TGFX_SUBDRAWPASSACCESS_FRAGMENTIMAGE_READONLY:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_FRAGMENTSBUFFER_READWRITE:
	case TGFX_SUBDRAWPASSACCESS_FRAGMENTIMAGE_READWRITE:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_FRAGMENTIMAGE_WRITEONLY:
	case TGFX_SUBDRAWPASSACCESS_FRAGMENTSBUFFER_WRITEONLY:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_FRAGMENTINPUTS_READONLY:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_FRAGMENTINPUTS_READWRITE:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_FRAGMENTINPUTS_WRITEONLY:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_FRAGMENTRT_READONLY:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_LATE_Z_READ:
		stageflag |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_LATE_Z_READWRITE:
		stageflag |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_LATE_Z_WRITEONLY:
		stageflag |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case TGFX_SUBDRAWPASSACCESS_FRAGMENTRT_WRITEONLY:
		stageflag |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		accessflag |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	default:
		stageflag = UINT64_MAX;
		accessflag = UINT64_MAX;
		vkPrint(49);
		break;
	}
}

inline void findImageAccessPattern(const TGfxImageAccess& Access,
								   VkAccessFlags& TargetAccessFlag,
								   VkImageLayout& TargetImageLayout)
{
	switch (Access)
	{
	case TGFX_IMAGE_ACCESS_DEPTHSTENCIL_READONLY:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_DEPTHSTENCIL_READWRITE:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_DEPTHSTENCIL_WRITEONLY:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_DEPTHREADWRITE_STENCILREAD:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_DEPTHREADWRITE_STENCILWRITE:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_DEPTHREAD_STENCILREADWRITE:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_DEPTHREAD_STENCILWRITE:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_DEPTHWRITE_STENCILREAD:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_DEPTHWRITE_STENCILREADWRITE:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_DEPTH_READONLY:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_DEPTH_READWRITE:
	case TGFX_IMAGE_ACCESS_DEPTH_WRITEONLY:
		TargetAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_NO_ACCESS:
		TargetAccessFlag = 0;
		TargetImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		return;
	case TGFX_IMAGE_ACCESS_RTCOLOR_READONLY:
		TargetAccessFlag = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_RTCOLOR_READWRITE:
		TargetAccessFlag = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_RTCOLOR_WRITEONLY:
		TargetAccessFlag = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_SHADER_SAMPLEONLY:
		TargetAccessFlag = VK_ACCESS_SHADER_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_SHADER_SAMPLEWRITE:
		TargetAccessFlag = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_GENERAL;
		return;
	case TGFX_IMAGE_ACCESS_SHADER_WRITEONLY:
		TargetAccessFlag = VK_ACCESS_SHADER_WRITE_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_GENERAL;
		return;
	case TGFX_IMAGE_ACCESS_SWAPCHAIN_DISPLAY:
		TargetAccessFlag = 0;
		TargetImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		return;
	case TGFX_IMAGE_ACCESS_TRANSFER_DIST:
		TargetAccessFlag = VK_ACCESS_TRANSFER_WRITE_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		return;
	case TGFX_IMAGE_ACCESS_TRANSFER_SRC:
		TargetAccessFlag = VK_ACCESS_TRANSFER_READ_BIT;
		TargetImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		return;
	default: vkPrint(49); return;
	}
}
inline VkImageType findImageTypeVk(TGfxTextureDimensions dimensions)
{
	switch (dimensions)
	{
	case TGFX_TEXTURE_DIMENSIONS_2D:
	case TGFX_TEXTURE_DIMENSIONS_2DCUBE: return VK_IMAGE_TYPE_2D;
	case TGFX_TEXTURE_DIMENSIONS_3D: return VK_IMAGE_TYPE_3D;
	default: vkPrint(49); return VkImageType::VK_IMAGE_TYPE_MAX_ENUM;
	}
}
inline VkImageTiling Find_VkTiling(TGfxTextureOrder order)
{
	switch (order)
	{
	case TGFX_TEXTURE_ORDER_SWIZZLE: return VK_IMAGE_TILING_OPTIMAL;
	case TGFX_TEXTURE_ORDER_LINEAR: return VK_IMAGE_TILING_LINEAR;
	default: vkPrint(49); return VkImageTiling::VK_IMAGE_TILING_MAX_ENUM;
	}
}
inline unsigned int Find_TextureLayer_fromtgfx_cubeface(TGfxCubeFace cubeface)
{
	switch (cubeface)
	{
	case TGFX_CUBEFACE_FRONT: return 0;
	case TGFX_CUBEFACE_BACK: return 1;
	case TGFX_CUBEFACE_LEFT: return 2;
	case TGFX_CUBEFACE_RIGHT: return 3;
	case TGFX_CUBEFACE_TOP: return 4;
	case TGFX_CUBEFACE_BOTTOM: return 5;
	default: vkPrint(49); return 0;
	}
}
inline TU4 GetDataTypeSizeInBytes(TGfxDataType data)
{
	switch (data)
	{
	case TGFX_DATATYPE_I8:
	case TGFX_DATATYPE_U8: return 1;
	case TGFX_DATATYPE_I16:
	case TGFX_DATATYPE_U16: return 2;
	case TGFX_DATATYPE_F32:
	case TGFX_DATATYPE_I32:
	case TGFX_DATATYPE_U32: return 4;
	case TGFX_DATATYPE_FVEC2: return 8;
	case TGFX_DATATYPE_FVEC3: return 12;
	case TGFX_DATATYPE_FVEC4: return 16;
	case TGFX_DATATYPE_FMAT4x4: return 64;
	case TGFX_DATATYPE_UNDEFINED:
	default: vkPrint(49); return 0;
	}
}
inline VkPresentModeKHR findPresentModeVk(TGfxWindowPresentation p)
{
	switch (p)
	{
	case TGFX_WINDOWPRESENTATION_FIFO: return VK_PRESENT_MODE_FIFO_KHR;
	case TGFX_WINDOWPRESENTATION_FIFO_RELAXED: return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	case TGFX_WINDOWPRESENTATION_IMMEDIATE: return VK_PRESENT_MODE_IMMEDIATE_KHR;
	case TGFX_WINDOWPRESENTATION_MAILBOX: return VK_PRESENT_MODE_MAILBOX_KHR;
	default: vkPrint(49); return VK_PRESENT_MODE_MAX_ENUM_KHR;
	}
}
inline TGfxWindowPresentation findPresentModeTgfx(VkPresentModeKHR p)
{
	switch (p)
	{
	case VK_PRESENT_MODE_FIFO_KHR: return TGFX_WINDOWPRESENTATION_FIFO;
	case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return TGFX_WINDOWPRESENTATION_FIFO_RELAXED;
	case VK_PRESENT_MODE_IMMEDIATE_KHR: return TGFX_WINDOWPRESENTATION_IMMEDIATE;
	case VK_PRESENT_MODE_MAILBOX_KHR: return TGFX_WINDOWPRESENTATION_MAILBOX;
	default: vkPrint(49); return TGFX_WINDOWPRESENTATION_FIFO;
	}
}
inline VkColorSpaceKHR findColorSpaceVk(TGfxColorSpace cs)
{
	switch (cs)
	{
	case TGFX_COLORSPACE_sRGB_NONLINEAR: return VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	case TGFX_COLORSPACE_HDR10_ST2084: return VK_COLOR_SPACE_HDR10_ST2084_EXT;
	case TGFX_COLORSPACE_EXTENDED_sRGB_LINEAR: return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
	default: vkPrint(49); return VK_COLOR_SPACE_MAX_ENUM_KHR;
	}
}
inline VkColorComponentFlags findColorWriteMask(TGfxTextureChannels chnnls)
{
	switch (chnnls)
	{
	case TGFX_TEXTURE_CHANNELS_BGRA8UB:
	case TGFX_TEXTURE_CHANNELS_BGRA8UNORM:
	case TGFX_TEXTURE_CHANNELS_RGBA32F:
	case TGFX_TEXTURE_CHANNELS_RGBA32UI:
	case TGFX_TEXTURE_CHANNELS_RGBA32I:
	case TGFX_TEXTURE_CHANNELS_RGBA8UB:
	case TGFX_TEXTURE_CHANNELS_RGBA8B:
		return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			   VK_COLOR_COMPONENT_A_BIT;
	case TGFX_TEXTURE_CHANNELS_RGB32F:
	case TGFX_TEXTURE_CHANNELS_RGB32UI:
	case TGFX_TEXTURE_CHANNELS_RGB32I:
	case TGFX_TEXTURE_CHANNELS_RGB8UB:
	case TGFX_TEXTURE_CHANNELS_RGB8B:
		return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
	case TGFX_TEXTURE_CHANNELS_RA32F:
	case TGFX_TEXTURE_CHANNELS_RA32UI:
	case TGFX_TEXTURE_CHANNELS_RA32I:
	case TGFX_TEXTURE_CHANNELS_RA8UB:
	case TGFX_TEXTURE_CHANNELS_RA8B: return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT;
	case TGFX_TEXTURE_CHANNELS_R32F:
	case TGFX_TEXTURE_CHANNELS_R32UI:
	case TGFX_TEXTURE_CHANNELS_R32I: return VK_COLOR_COMPONENT_R_BIT;
	case TGFX_TEXTURE_CHANNELS_R8UB:
	case TGFX_TEXTURE_CHANNELS_R8B: return VK_COLOR_COMPONENT_R_BIT;
	case TGFX_TEXTURE_CHANNELS_D32:
	case TGFX_TEXTURE_CHANNELS_D24S8:
	default: vkPrint(49); return VK_COLOR_COMPONENT_FLAG_BITS_MAX_ENUM;
	}
}
inline VkColorComponentFlags findColorComponentsVk(TGfxTextureComponentMask mask, TGfxTextureChannels format)
{
	if (mask == TGFX_TEXTURECOMPONENTMASK_RGBA && format != TGFX_TEXTURE_CHANNELS_UNDEF)
	{
		return findColorWriteMask(format);
	}
	if (mask == (TGfxTextureComponentMask)0)
	{
		return 0;
	}
	VkColorComponentFlags flag = {};
	flag |= (mask & TGFX_TEXTURECOMPONENTMASK_R) ? VK_COLOR_COMPONENT_R_BIT : 0;
	flag |= (mask & TGFX_TEXTURECOMPONENTMASK_G) ? VK_COLOR_COMPONENT_G_BIT : 0;
	flag |= (mask & TGFX_TEXTURECOMPONENTMASK_B) ? VK_COLOR_COMPONENT_B_BIT : 0;
	flag |= (mask & TGFX_TEXTURECOMPONENTMASK_A) ? VK_COLOR_COMPONENT_A_BIT : 0;
	return flag;
}
inline TGfxColorSpace findColorSpaceTgfx(VkColorSpaceKHR cs)
{
	switch (cs)
	{
	case VK_COLORSPACE_SRGB_NONLINEAR_KHR: return TGFX_COLORSPACE_sRGB_NONLINEAR;
	case VK_COLOR_SPACE_HDR10_ST2084_EXT: return TGFX_COLORSPACE_HDR10_ST2084;
	case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT: return TGFX_COLORSPACE_EXTENDED_sRGB_LINEAR;
	default: vkPrint(49); return (TGfxColorSpace)UINT32_MAX;
	}
}
inline TGfxShaderStage findShaderStageFromMask(unsigned int mask)
{
	TGfxShaderStage flag = (TGfxShaderStage)0;
	flag = (TGfxShaderStage)((mask & TGFX_SHADERSTAGE_VERTEXSHADER) ? TGFX_SHADERSTAGE_VERTEXSHADER : 0);
	flag =
		(TGfxShaderStage)((mask & TGFX_SHADERSTAGE_FRAGMENTSHADER) ? (flag | TGFX_SHADERSTAGE_FRAGMENTSHADER) : flag);
	flag = (TGfxShaderStage)((mask & TGFX_SHADERSTAGE_COMPUTESHADER) ? (flag | TGFX_SHADERSTAGE_COMPUTESHADER) : flag);
	return flag;
}
inline TGfxGpuType findGPUTypeTgfx(VkPhysicalDeviceType t)
{
	switch (t)
	{
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		return (TGfxGpuType)0; // replace with actual TGfxGpuType enum if available
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return (TGfxGpuType)1; // replace with actual enum values
	default: return (TGfxGpuType)UINT32_MAX;
	}
}
inline VkPipelineBindPoint findPipelineBindPoint(TGfxPipelineType type)
{
	switch (type)
	{
	case TGFX_PIPELINETYPE_COMPUTE: return VK_PIPELINE_BIND_POINT_COMPUTE;
	case TGFX_PIPELINETYPE_RASTER: return VK_PIPELINE_BIND_POINT_GRAPHICS;
	case TGFX_PIPELINETYPE_RAYTRACING: return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
	default: vkPrint(49); return VK_PIPELINE_BIND_POINT_MAX_ENUM;
	}
}
inline VkBufferUsageFlags findBufferUsageFlagVk(unsigned int mask)
{
	VkBufferUsageFlags flag = {};
	// The TGfx buffer usage mask constants should be mapped here; placeholder mapping:
	// flag |= (mask & TGFX_BUFFERUSAGE_COPYFROM) ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0;
	// flag |= (mask & TGFX_BUFFERUSAGE_COPYTO) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0;
	// flag |= (mask & TGFX_BUFFERUSAGE_INDEXBUFFER) ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0;
	// flag |= (mask & TGFX_BUFFERUSAGE_INDIRECTBUFFER) ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0;
	// flag |= (mask & TGFX_BUFFERUSAGE_STORAGEBUFFER) ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
	// flag |= (mask & TGFX_BUFFERUSAGE_UNIFORMBUFFER) ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0;
	// flag |= (mask & TGFX_BUFFERUSAGE_VERTEXBUFFER) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0;
	return flag;
}
inline VkImageUsageFlags findTextureUsageFlagVk(unsigned int mask)
{
	VkImageUsageFlags flag = {};
	// Placeholder mapping; replace TGFX_TEXTUREUSAGE_* masks with actual constants when available
	// flag |= (mask & TGFX_TEXTUREUSAGE_COPYFROM) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0;
	// flag |= (mask & TGFX_TEXTUREUSAGE_COPYTO) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
	// flag |= (mask & TGFX_TEXTUREUSAGE_RANDOMACCESS) ? VK_IMAGE_USAGE_STORAGE_BIT : 0;
	// flag |= (mask & TGFX_TEXTUREUSAGE_RASTERSAMPLE) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
	// flag |= (mask & TGFX_TEXTUREUSAGE_RENDERATTACHMENT) ? (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
	// VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) : 0;
	return flag;
}
inline unsigned int findTextureUsageFlagTgfx(VkImageUsageFlags mask)
{
	unsigned int flag = 0;
	// Placeholder mapping back to TGfx texture usage mask
	// flag |= (mask & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) ? TGFX_TEXTUREUSAGE_COPYFROM : 0;
	// flag |= (mask & VK_IMAGE_USAGE_TRANSFER_DST_BIT) ? TGFX_TEXTUREUSAGE_COPYTO : 0;
	// flag |= (mask & VK_IMAGE_USAGE_STORAGE_BIT) ? TGFX_TEXTUREUSAGE_RANDOMACCESS : 0;
	// flag |= (mask & VK_IMAGE_USAGE_SAMPLED_BIT) ? TGFX_TEXTUREUSAGE_RASTERSAMPLE : 0;
	// flag |= (mask & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) ?
	// TGFX_TEXTUREUSAGE_RENDERATTACHMENT : 0;
	return flag;
}
inline VkVertexInputRate findVertexInputRateVk(TGfxVertexBindingInputRate rate)
{
	switch (rate)
	{
	case TGFX_VERTEXBINDINGINPUTRATE_VERTEX: return VK_VERTEX_INPUT_RATE_VERTEX;
	case TGFX_VERTEXBINDINGINPUTRATE_INSTANCE: return VK_VERTEX_INPUT_RATE_INSTANCE;
	default: vkPrint(49); return VK_VERTEX_INPUT_RATE_MAX_ENUM;
	}
}
} // namespace Vulkan
} // namespace TGFX