#pragma once
#include <stdio.h>
#include <threadingsys_tapi.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <glm/glm.hpp>
#include <iostream>
#include <mutex>

#include "vk_predefinitions.h"

// Some algorithms and data structures to help in C++ (like threadlocalvector)

template <typename T>
class vk_atomic {
  std::atomic<T> data;

 public:
  // Returns the old value
  uint64_t DirectAdd(const uint64_t& add) { return data.fetch_add(add); }
  // Returns the old value
  uint64_t DirectSubtract(const uint64_t& sub) { return data.fetch_sub(sub); }
  void     DirectStore(const uint64_t& Store) { data.store(Store); }
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
  bool LimitedAdd_weak(const uint64_t& add, const uint64_t& maxlimit) {
    uint64_t x = data.load();
    if (x + add > maxlimit || // Addition is bigger
        x + add < x           // UINT overflow
    ) {
      return false;
    }
    if (!data.compare_exchange_strong(x, x + add)) {
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
  void LimitedAdd_strong(const uint64_t& add, const uint64_t& maxlimit) {
    while (true) {
      if (LimitedAdd_weak(add, maxlimit)) {
        return;
      }
      std::this_thread::yield();
    }
  }
  // Similar but the reverse of the LimitedAdd_weak()
  bool LimitedSubtract_weak(const uint64_t& subtract, const uint64_t& minlimit) {
    uint64_t x = data.load();
    if (x - subtract < minlimit || // Subtraction is bigger
        x - subtract > x           // UINT overflow
    ) {
      return false;
    }
    if (!data.compare_exchange_strong(x, x - subtract)) {
      return LimitedSubtract_weak(subtract, minlimit);
    }
    return true;
  }
  // Similar but the reverse of the LimitedAdd_strong()
  void LimitedSubtract_strong(const uint64_t& subtract, const uint64_t& minlimit) {
    while (true) {
      if (LimitedSubtract_weak(subtract, minlimit)) {
        return;
      }
      std::this_thread::yield();
    }
  }
};

inline unsigned char GetByteSizeOf_TextureChannels(textureChannels_tgfx channeltype) {
  switch (channeltype) {
    case texture_channels_tgfx_R8B:
    case texture_channels_tgfx_R8UB: return 1;
    case texture_channels_tgfx_RGB8B:
    case texture_channels_tgfx_RGB8UB: return 3;
    case texture_channels_tgfx_D24S8:
    case texture_channels_tgfx_D32:
    case texture_channels_tgfx_RGBA8B:
    case texture_channels_tgfx_RGBA8UB:
    case texture_channels_tgfx_BGRA8UB:
    case texture_channels_tgfx_BGRA8UNORM:
    case texture_channels_tgfx_R32F:
    case texture_channels_tgfx_R32I:
    case texture_channels_tgfx_R32UI: return 4;
    case texture_channels_tgfx_RA32F:
    case texture_channels_tgfx_RA32I:
    case texture_channels_tgfx_RA32UI: return 8;
    case texture_channels_tgfx_RGB32F:
    case texture_channels_tgfx_RGB32I:
    case texture_channels_tgfx_RGB32UI: return 12;
    case texture_channels_tgfx_RGBA32F:
    case texture_channels_tgfx_RGBA32I:
    case texture_channels_tgfx_RGBA32UI: return 16;
    default: assert_vk(0 && "GetSizeOf_TextureChannels() doesn't support this type!"); break;
  }
}
inline VkFormat Find_VkFormat_byDataType(datatype_tgfx datatype) {
  switch (datatype) {
    case datatype_tgfx_VAR_VEC2: return VK_FORMAT_R32G32_SFLOAT;
    case datatype_tgfx_VAR_VEC3: return VK_FORMAT_R32G32B32_SFLOAT;
    case datatype_tgfx_VAR_VEC4: return VK_FORMAT_R32G32B32A32_SFLOAT;
    default:
      printer(result_tgfx_FAIL,
              "(Find_VkFormat_byDataType() doesn't support this data type! UNDEFINED");
      return VK_FORMAT_UNDEFINED;
  }
}
inline VkFormat vk_findFormatVk(textureChannels_tgfx channels) {
  switch (channels) {
    case texture_channels_tgfx_BGRA8UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
    case texture_channels_tgfx_BGRA8UB: return VK_FORMAT_B8G8R8A8_UINT;
    case texture_channels_tgfx_R8B: return VK_FORMAT_R8_SINT;
    case texture_channels_tgfx_RGBA8UB: return VK_FORMAT_R8G8B8A8_UINT;
    case texture_channels_tgfx_RGBA8B: return VK_FORMAT_R8G8B8A8_SINT;
    case texture_channels_tgfx_RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case texture_channels_tgfx_RGBA32I: return VK_FORMAT_R32G32B32A32_SINT;
    case texture_channels_tgfx_RGBA32UI: return VK_FORMAT_R32G32B32A32_UINT;
    case texture_channels_tgfx_RGB8UB: return VK_FORMAT_R8G8B8_UINT;
    case texture_channels_tgfx_D32: return VK_FORMAT_D32_SFLOAT;
    case texture_channels_tgfx_D24S8: return VK_FORMAT_D24_UNORM_S8_UINT;
    case texture_channels_tgfx_BGRA8SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
    case texture_channels_tgfx_RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
    case texture_channels_tgfx_A2B10G10R10_UNORM: return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    case texture_channels_tgfx_RGBA8SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
    case texture_channels_tgfx_UNDEF: return VK_FORMAT_UNDEFINED;
    case texture_channels_tgfx_UNDEF2:
    default:
      printer(result_tgfx_FAIL,
              "(Find_VkFormat_byTEXTURECHANNELs doesn't support this type of channel!");
      return VK_FORMAT_UNDEFINED;
  }
}
inline textureChannels_tgfx vk_findTextureChannelsTgfx(VkFormat format) {
  switch (format) {
    case VK_FORMAT_B8G8R8A8_UNORM: return texture_channels_tgfx_BGRA8UNORM;
    case VK_FORMAT_B8G8R8A8_UINT: return texture_channels_tgfx_BGRA8UB;
    case VK_FORMAT_R8G8B8A8_UINT: return texture_channels_tgfx_RGBA8UB;
    case VK_FORMAT_R8G8B8A8_SINT: return texture_channels_tgfx_RGBA8B;
    case VK_FORMAT_R32G32B32A32_SFLOAT: return texture_channels_tgfx_RGBA32F;
    case VK_FORMAT_R32G32B32A32_SINT: return texture_channels_tgfx_RGBA32I;
    case VK_FORMAT_R32G32B32A32_UINT: return texture_channels_tgfx_RGBA32UI;
    case VK_FORMAT_R8G8B8_UINT: return texture_channels_tgfx_RGB8UB;
    case VK_FORMAT_D32_SFLOAT: return texture_channels_tgfx_D32;
    case VK_FORMAT_D24_UNORM_S8_UINT: return texture_channels_tgfx_D24S8;
    case VK_FORMAT_B8G8R8A8_SRGB: return texture_channels_tgfx_BGRA8SRGB;
    case VK_FORMAT_R16G16B16A16_SFLOAT: return texture_channels_tgfx_RGBA16F;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return texture_channels_tgfx_A2B10G10R10_UNORM;
    case VK_FORMAT_R8G8B8A8_UNORM: return texture_channels_tgfx_RGBA8UNORM;
    case VK_FORMAT_R8G8B8A8_SRGB: return texture_channels_tgfx_RGBA8SRGB;
    default:
      printer(result_tgfx_FAIL, "(Find_TEXTURECHANNELs_byVkFormat doesn't support this VkFormat!");
      return texture_channels_tgfx_R8B;
  }
}
inline datatype_tgfx vk_findTextureDataType(VkFormat format) {
  switch (format) {
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
    case VK_FORMAT_B8G8R8A8_UNORM: return datatype_tgfx_VAR_FLOAT32;

    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT: return datatype_tgfx_VAR_UINT32;

    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R32G32B32A32_SINT: return datatype_tgfx_VAR_INT32;
    default:
      printer(result_tgfx_FAIL, "vk_findTextureDataType() has failed!");
      return datatype_tgfx_UNDEFINED;
  }
}
inline VkDescriptorType vk_findDescTypeVk(shaderdescriptortype_tgfx desc) {
  switch (desc) {
    case shaderdescriptortype_tgfx_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case shaderdescriptortype_tgfx_SAMPLEDTEXTURE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case shaderdescriptortype_tgfx_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
    case shaderdescriptortype_tgfx_STORAGEIMAGE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case shaderdescriptortype_tgfx_EXT_UNIFORMBUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    default:
      printer(result_tgfx_FAIL, "vk_findDescTypeVk couldn't find descriptor type!");
      return VK_DESCRIPTOR_TYPE_MAX_ENUM;
  }
}
inline shaderdescriptortype_tgfx vk_findDescTypeTgfx(VkDescriptorType desc) {
  switch (desc) {
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return shaderdescriptortype_tgfx_BUFFER;
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return shaderdescriptortype_tgfx_SAMPLEDTEXTURE;
    case VK_DESCRIPTOR_TYPE_SAMPLER: return shaderdescriptortype_tgfx_SAMPLER;
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return shaderdescriptortype_tgfx_STORAGEIMAGE;
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return shaderdescriptortype_tgfx_EXT_UNIFORMBUFFER;
    default:
      printer(result_tgfx_FAIL, "vk_findDescTypeVk couldn't find descriptor type!");
      return ( shaderdescriptortype_tgfx )UINT64_MAX;
  }
}
inline VkSamplerAddressMode vk_findAddressModeVk(texture_wrapping_tgfx Wrapping) {
  switch (Wrapping) {
    case texture_wrapping_tgfx_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case texture_wrapping_tgfx_MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case texture_wrapping_tgfx_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    default:
      printer(result_tgfx_INVALIDARGUMENT,
              "Find_AddressMode_byWRAPPING() doesn't support this wrapping type!");
      return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
  }
}
inline VkFilter vk_findFilterVk(texture_mipmapfilter_tgfx filter) {
  switch (filter) {
    case texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP:
    case texture_mipmapfilter_tgfx_LINEAR_FROM_2MIP: return VK_FILTER_LINEAR;
    case texture_mipmapfilter_tgfx_NEAREST_FROM_1MIP:
    case texture_mipmapfilter_tgfx_NEAREST_FROM_2MIP: return VK_FILTER_NEAREST;
    default:
      printer(result_tgfx_INVALIDARGUMENT,
              "Find_VkFilter_byGFXFilter() doesn't support this filter type!");
      return VK_FILTER_MAX_ENUM;
  }
}
inline VkSamplerMipmapMode vk_findMipmapModeVk(texture_mipmapfilter_tgfx filter) {
  switch (filter) {
    case texture_mipmapfilter_tgfx_LINEAR_FROM_2MIP:
    case texture_mipmapfilter_tgfx_NEAREST_FROM_2MIP: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    case texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP:
    case texture_mipmapfilter_tgfx_NEAREST_FROM_1MIP: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
  }
}
inline VkCullModeFlags vk_findCullModeVk(cullmode_tgfx mode) {
  switch (mode) {
    case cullmode_tgfx_OFF: return VK_CULL_MODE_NONE; break;
    case cullmode_tgfx_BACK: return VK_CULL_MODE_BACK_BIT; break;
    case cullmode_tgfx_FRONT: return VK_CULL_MODE_FRONT_BIT; break;
    default:
      printer(result_tgfx_INVALIDARGUMENT,
              "This culling type isn't supported by Find_CullMode_byGFXCullMode()!");
      return VK_CULL_MODE_NONE;
      break;
  }
}
inline VkPolygonMode vk_findPolygonModeVk(polygonmode_tgfx mode) {
  switch (mode) {
    case polygonmode_tgfx_FILL: return VK_POLYGON_MODE_FILL; break;
    case polygonmode_tgfx_LINE: return VK_POLYGON_MODE_LINE; break;
    case polygonmode_tgfx_POINT: return VK_POLYGON_MODE_POINT; break;
    default:
      printer(result_tgfx_INVALIDARGUMENT,
              "This polygon mode isn't support by Find_PolygonMode_byGFXPolygonMode()");
      break;
  }
}
inline VkPrimitiveTopology Find_PrimitiveTopology_byGFXVertexListType(
  vertexlisttypes_tgfx vertextype) {
  switch (vertextype) {
    case vertexlisttypes_tgfx_TRIANGLELIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    default:
      printer(result_tgfx_INVALIDARGUMENT,
              "This type of vertex list is not supported by "
              "Find_PrimitiveTopology_byGFXVertexListType()");
      return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
      break;
  }
}
inline VkLogicOp   vk_findLogicOpVk() {}
inline VkIndexType Find_IndexType_byGFXDATATYPE(datatype_tgfx datatype) {
  switch (datatype) {
    case datatype_tgfx_VAR_UINT32: return VK_INDEX_TYPE_UINT32;
    case datatype_tgfx_VAR_UINT16: return VK_INDEX_TYPE_UINT16;
    default:
      printer(result_tgfx_INVALIDARGUMENT,
              "This type of data isn't supported by Find_IndexType_byGFXDATATYPE()");
      return VK_INDEX_TYPE_MAX_ENUM;
  }
}
inline VkCompareOp vk_findCompareOpVk(compare_tgfx test) {
  switch (test) {
    case compare_tgfx_NEVER: return VK_COMPARE_OP_NEVER;
    case compare_tgfx_ALWAYS: return VK_COMPARE_OP_ALWAYS;
    case compare_tgfx_GEQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case compare_tgfx_GREATER: return VK_COMPARE_OP_GREATER;
    case compare_tgfx_LEQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
    case compare_tgfx_LESS: return VK_COMPARE_OP_LESS;
    default:
      printer(result_tgfx_INVALIDARGUMENT,
              "vk_findCompareOpVk() doesn't support this type of test!");
      return VK_COMPARE_OP_MAX_ENUM;
  }
}
inline void Find_DepthMode_byGFXDepthMode(depthmode_tgfx mode, VkBool32& ShouldTest,
                                          VkBool32& ShouldWrite) {
  switch (mode) {
    case depthmode_tgfx_READ_WRITE:
      ShouldTest  = VK_TRUE;
      ShouldWrite = VK_TRUE;
      break;
    case depthmode_tgfx_READ_ONLY:
      ShouldTest  = VK_TRUE;
      ShouldWrite = VK_FALSE;
      break;
    case depthmode_tgfx_OFF:
      ShouldTest  = VK_FALSE;
      ShouldWrite = VK_FALSE;
      break;
    default:
      printer(result_tgfx_INVALIDARGUMENT,
              "Find_DepthMode_byGFXDepthMode() doesn't support this type of depth mode!");
      break;
  }
}
inline VkAttachmentLoadOp vk_findLoadTypeVk(rasterpassLoad_tgfx load) {
  switch (load) {
    case rasterpassLoad_tgfx_CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case rasterpassLoad_tgfx_DISCARD: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    case rasterpassLoad_tgfx_LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD;
    case rasterpassLoad_tgfx_NONE: return VK_ATTACHMENT_LOAD_OP_NONE_EXT;
    default:
      printer(result_tgfx_INVALIDARGUMENT,
              "vk_findLoadTypeVk() doesn't support this type of load!");
      return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
  }
}
inline VkAttachmentStoreOp vk_findStoreTypeVk(rasterpassStore_tgfx store) {
  switch (store) {
    case rasterpassStore_tgfx_STORE: return VK_ATTACHMENT_STORE_OP_STORE;
    case rasterpassStore_tgfx_DISCARD: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    case rasterpassLoad_tgfx_NONE: return VK_ATTACHMENT_STORE_OP_NONE;
    default:
      printer(result_tgfx_INVALIDARGUMENT,
              "vk_findLoadTypeVk() doesn't support this type of load!");
      return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
  }
}
inline VkStencilOp vk_findStencilOpVk(stencilop_tgfx op) {
  switch (op) {
    case stencilop_tgfx_DONT_CHANGE: return VK_STENCIL_OP_KEEP;
    case stencilop_tgfx_SET_ZERO: return VK_STENCIL_OP_ZERO;
    case stencilop_tgfx_CHANGE: return VK_STENCIL_OP_REPLACE;
    case stencilop_tgfx_CLAMPED_INCREMENT: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case stencilop_tgfx_WRAPPED_INCREMENT: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    case stencilop_tgfx_CLAMPED_DECREMENT: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    case stencilop_tgfx_WRAPPED_DECREMENT: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    case stencilop_tgfx_BITWISE_INVERT: return VK_STENCIL_OP_INVERT;
    default:
      printer(result_tgfx_INVALIDARGUMENT,
              "Find_StencilOp_byGFXStencilOp() doesn't support this type of stencil operation!");
      return VK_STENCIL_OP_KEEP;
  }
}
inline VkBlendOp Find_BlendOp_byGFXBlendMode(blendmode_tgfx mode) {
  switch (mode) {
    case blendmode_tgfx_ADDITIVE: return VK_BLEND_OP_ADD;
    case blendmode_tgfx_SUBTRACTIVE: return VK_BLEND_OP_SUBTRACT;
    case blendmode_tgfx_SUBTRACTIVE_SWAPPED: return VK_BLEND_OP_REVERSE_SUBTRACT;
    case blendmode_tgfx_MIN: return VK_BLEND_OP_MIN;
    case blendmode_tgfx_MAX: return VK_BLEND_OP_MAX;
    default:
      printer(result_tgfx_INVALIDARGUMENT,
              "Find_BlendOp_byGFXBlendMode() doesn't support this type of blend mode!");
      return VK_BLEND_OP_MAX_ENUM;
  }
}
inline VkBlendFactor Find_BlendFactor_byGFXBlendFactor(blendfactor_tgfx factor) {
  switch (factor) {
    case blendfactor_tgfx_ONE: return VK_BLEND_FACTOR_ONE;
    case blendfactor_tgfx_ZERO: return VK_BLEND_FACTOR_ZERO;
    case blendfactor_tgfx_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
    case blendfactor_tgfx_SRC_1MINUSCOLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case blendfactor_tgfx_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
    case blendfactor_tgfx_SRC_1MINUSALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case blendfactor_tgfx_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
    case blendfactor_tgfx_DST_1MINUSCOLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case blendfactor_tgfx_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
    case blendfactor_tgfx_DST_1MINUSALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case blendfactor_tgfx_CONST_COLOR: return VK_BLEND_FACTOR_CONSTANT_COLOR;
    case blendfactor_tgfx_CONST_1MINUSCOLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    case blendfactor_tgfx_CONST_ALPHA: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
    case blendfactor_tgfx_CONST_1MINUSALPHA: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
    default:
      printer(result_tgfx_INVALIDARGUMENT,
              "Find_BlendFactor_byGFXBlendFactor() doesn't support this type of blend factor!");
      return VK_BLEND_FACTOR_MAX_ENUM;
  }
}
inline void Fill_ComponentMapping_byCHANNELs(textureChannels_tgfx channels,
                                             VkComponentMapping&  mapping) {
  switch (channels) {
    case texture_channels_tgfx_D32:
    case texture_channels_tgfx_D24S8:
    case texture_channels_tgfx_BGRA8UB:
    case texture_channels_tgfx_BGRA8UNORM:
    case texture_channels_tgfx_RGBA32F:
    case texture_channels_tgfx_RGBA32UI:
    case texture_channels_tgfx_RGBA32I:
    case texture_channels_tgfx_RGBA8UB:
    case texture_channels_tgfx_RGBA8B:
      mapping.r = VK_COMPONENT_SWIZZLE_R;
      mapping.g = VK_COMPONENT_SWIZZLE_G;
      mapping.b = VK_COMPONENT_SWIZZLE_B;
      mapping.a = VK_COMPONENT_SWIZZLE_A;
      return;
    case texture_channels_tgfx_RGB32F:
    case texture_channels_tgfx_RGB32UI:
    case texture_channels_tgfx_RGB32I:
    case texture_channels_tgfx_RGB8UB:
    case texture_channels_tgfx_RGB8B:
      mapping.r = VK_COMPONENT_SWIZZLE_R;
      mapping.g = VK_COMPONENT_SWIZZLE_G;
      mapping.b = VK_COMPONENT_SWIZZLE_B;
      mapping.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      return;
    case texture_channels_tgfx_RA32F:
    case texture_channels_tgfx_RA32UI:
    case texture_channels_tgfx_RA32I:
    case texture_channels_tgfx_RA8UB:
    case texture_channels_tgfx_RA8B:
      mapping.r = VK_COMPONENT_SWIZZLE_R;
      mapping.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      mapping.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      mapping.a = VK_COMPONENT_SWIZZLE_A;
      return;
    case texture_channels_tgfx_R32F:
    case texture_channels_tgfx_R32UI:
    case texture_channels_tgfx_R32I:
    case texture_channels_tgfx_R8UB:
    case texture_channels_tgfx_R8B:
      mapping.r = VK_COMPONENT_SWIZZLE_R;
      mapping.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      mapping.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      mapping.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      return;
    default: break;
  }
}
inline void vk_findSubpassAccessPattern(subdrawpassaccess_tgfx access, bool isSource,
                                        VkPipelineStageFlags& stageflag,
                                        VkAccessFlags&        accessflag) {
  switch (access) {
    case subdrawpassaccess_tgfx_ALLCOMMANDS:
      if (isSource) {
        stageflag |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      } else {
        stageflag |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      }
      break;
    case subdrawpassaccess_tgfx_INDEX_READ:
      stageflag |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
      accessflag |= VK_ACCESS_INDEX_READ_BIT;
      break;
    case subdrawpassaccess_tgfx_VERTEXATTRIB_READ:
      stageflag |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
      accessflag |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
      break;
    case subdrawpassaccess_tgfx_VERTEXUBUFFER_READONLY:
      stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
      accessflag |= VK_ACCESS_UNIFORM_READ_BIT;
      break;
    case subdrawpassaccess_tgfx_VERTEXSBUFFER_READONLY:
    case subdrawpassaccess_tgfx_VERTEXSAMPLED_READONLY:
    case subdrawpassaccess_tgfx_VERTEXIMAGE_READONLY:
      stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
      accessflag |= VK_ACCESS_SHADER_READ_BIT;
      break;
    case subdrawpassaccess_tgfx_VERTEXSBUFFER_READWRITE:
    case subdrawpassaccess_tgfx_VERTEXIMAGE_READWRITE:
      stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
      accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      break;
    case subdrawpassaccess_tgfx_VERTEXIMAGE_WRITEONLY:
    case subdrawpassaccess_tgfx_VERTEXSBUFFER_WRITEONLY:
      stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
      accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
      break;
    case subdrawpassaccess_tgfx_VERTEXINPUTS_READONLY:
      stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
      accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
      break;
    case subdrawpassaccess_tgfx_VERTEXINPUTS_READWRITE:
      stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
      accessflag |=
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      break;
    case subdrawpassaccess_tgfx_VERTEXINPUTS_WRITEONLY:
      stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
      accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
      break;

    case subdrawpassaccess_tgfx_EARLY_Z_READ:
      stageflag |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      break;
    case subdrawpassaccess_tgfx_EARLY_Z_READWRITE:
      stageflag |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      accessflag |=
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;
    case subdrawpassaccess_tgfx_EARLY_Z_WRITEONLY:
      stageflag |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;
    case subdrawpassaccess_tgfx_FRAGMENTUBUFFER_READONLY:
      stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      accessflag |= VK_ACCESS_UNIFORM_READ_BIT;
      break;
    case subdrawpassaccess_tgfx_FRAGMENTSBUFFER_READONLY:
    case subdrawpassaccess_tgfx_FRAGMENTSAMPLED_READONLY:
    case subdrawpassaccess_tgfx_FRAGMENTIMAGE_READONLY:
      stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      accessflag |= VK_ACCESS_SHADER_READ_BIT;
      break;
    case subdrawpassaccess_tgfx_FRAGMENTSBUFFER_READWRITE:
    case subdrawpassaccess_tgfx_FRAGMENTIMAGE_READWRITE:
      stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      break;
    case subdrawpassaccess_tgfx_FRAGMENTIMAGE_WRITEONLY:
    case subdrawpassaccess_tgfx_FRAGMENTSBUFFER_WRITEONLY:
      stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
      break;
    case subdrawpassaccess_tgfx_FRAGMENTINPUTS_READONLY:
      stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
      break;
    case subdrawpassaccess_tgfx_FRAGMENTINPUTS_READWRITE:
      stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      accessflag |=
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      break;
    case subdrawpassaccess_tgfx_FRAGMENTINPUTS_WRITEONLY:
      stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
      break;
    case subdrawpassaccess_tgfx_FRAGMENTRT_READONLY:
      stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      accessflag |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
      break;
    case subdrawpassaccess_tgfx_LATE_Z_READ:
      stageflag |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      break;
    case subdrawpassaccess_tgfx_LATE_Z_READWRITE:
      stageflag |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      accessflag |=
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;
    case subdrawpassaccess_tgfx_LATE_Z_WRITEONLY:
      stageflag |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;
    case subdrawpassaccess_tgfx_FRAGMENTRT_WRITEONLY:
      stageflag |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      accessflag |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      break;
    default:
      stageflag  = UINT64_MAX;
      accessflag = UINT64_MAX;
      break;
  }
}

inline void vk_findImageAccessPattern(const image_access_tgfx& Access,
                                      VkAccessFlags&           TargetAccessFlag,
                                      VkImageLayout&           TargetImageLayout) {
  switch (Access) {
    case image_access_tgfx_DEPTHSTENCIL_READONLY:
      TargetAccessFlag  = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
      return;
    case image_access_tgfx_DEPTHSTENCIL_READWRITE:
      TargetAccessFlag =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      return;
    case image_access_tgfx_DEPTHSTENCIL_WRITEONLY:
      TargetAccessFlag  = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      return;
    case image_access_tgfx_DEPTHREADWRITE_STENCILREAD:
      TargetAccessFlag =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
      return;
    case image_access_tgfx_DEPTHREADWRITE_STENCILWRITE:
      TargetAccessFlag =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      return;
    case image_access_tgfx_DEPTHREAD_STENCILREADWRITE:
      TargetAccessFlag =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
      return;
    case image_access_tgfx_DEPTHREAD_STENCILWRITE:
      TargetAccessFlag =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
      return;
    case image_access_tgfx_DEPTHWRITE_STENCILREAD:
      TargetAccessFlag =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
      return;
    case image_access_tgfx_DEPTHWRITE_STENCILREADWRITE:
      TargetAccessFlag =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      return;
    case image_access_tgfx_DEPTH_READONLY:
      TargetAccessFlag  = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
      return;
    case image_access_tgfx_DEPTH_READWRITE:
    case image_access_tgfx_DEPTH_WRITEONLY:
      TargetAccessFlag =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
      return;
    case image_access_tgfx_NO_ACCESS:
      TargetAccessFlag  = 0;
      TargetImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      return;
    case image_access_tgfx_RTCOLOR_READONLY:
      TargetAccessFlag  = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      return;
    case image_access_tgfx_RTCOLOR_READWRITE:
      TargetAccessFlag = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      return;
    case image_access_tgfx_RTCOLOR_WRITEONLY:
      TargetAccessFlag  = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      return;
    case image_access_tgfx_SHADER_SAMPLEONLY:
      TargetAccessFlag  = VK_ACCESS_SHADER_READ_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      return;
    case image_access_tgfx_SHADER_SAMPLEWRITE:
      TargetAccessFlag  = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_GENERAL;
      return;
    case image_access_tgfx_SHADER_WRITEONLY:
      TargetAccessFlag  = VK_ACCESS_SHADER_WRITE_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_GENERAL;
      return;
    case image_access_tgfx_SWAPCHAIN_DISPLAY:
      TargetAccessFlag  = 0;
      TargetImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      return;
    case image_access_tgfx_TRANSFER_DIST:
      TargetAccessFlag  = VK_ACCESS_TRANSFER_WRITE_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      return;
    case image_access_tgfx_TRANSFER_SRC:
      TargetAccessFlag  = VK_ACCESS_TRANSFER_READ_BIT;
      TargetImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      return;
    default:
      printer(result_tgfx_NOTCODED,
              "Find_AccessPattern_byIMAGEACCESS() doesn't support this access type!");
      return;
  }
}
inline VkImageType vk_findImageTypeVk(texture_dimensions_tgfx dimensions) {
  switch (dimensions) {
    case texture_dimensions_tgfx_2D:
    case texture_dimensions_tgfx_2DCUBE: return VK_IMAGE_TYPE_2D;
    case texture_dimensions_tgfx_3D: return VK_IMAGE_TYPE_3D;
    default:
      printer(result_tgfx_NOTCODED, "Find_VkImageType() doesn't support this dimension!");
      return VkImageType::VK_IMAGE_TYPE_MAX_ENUM;
  }
}
inline VkImageTiling Find_VkTiling(textureOrder_tgfx order) {
  switch (order) {
    case textureOrder_tgfx_SWIZZLE: return VK_IMAGE_TILING_OPTIMAL;
    case textureOrder_tgfx_LINEAR: return VK_IMAGE_TILING_LINEAR;
    default:
      printer(result_tgfx_NOTCODED, "Find_VkTiling() doesn't support this order!");
      return VkImageTiling::VK_IMAGE_TILING_MAX_ENUM;
  }
}
inline unsigned int Find_TextureLayer_fromtgfx_cubeface(cubeface_tgfx cubeface) {
  switch (cubeface) {
    case cubeface_tgfx_FRONT: return 0;
    case cubeface_tgfx_BACK: return 1;
    case cubeface_tgfx_LEFT: return 2;
    case cubeface_tgfx_RIGHT: return 3;
    case cubeface_tgfx_TOP: return 4;
    case cubeface_tgfx_BOTTOM: return 5;
    default:
      printer(result_tgfx_FAIL,
              "Find_TextureLayer_fromGFXtgfx_cubeface() in Vulkan backend doesn't support this "
              "type of CubeFace!");
  }
}
inline unsigned int vk_getDataTypeByteSizes(datatype_tgfx data) {
  switch (data) {
    case datatype_tgfx_VAR_BYTE8:
    case datatype_tgfx_VAR_UBYTE8: return 1;
    case datatype_tgfx_VAR_INT16:
    case datatype_tgfx_VAR_UINT16: return 2;
    case datatype_tgfx_VAR_FLOAT32:
    case datatype_tgfx_VAR_INT32:
    case datatype_tgfx_VAR_UINT32: return 4;
    case datatype_tgfx_VAR_VEC2: return 8;
    case datatype_tgfx_VAR_VEC3: return 12;
    case datatype_tgfx_VAR_VEC4: return 16;
    case datatype_tgfx_VAR_MAT4x4: return 64;
    case datatype_tgfx_UNDEFINED:
    default:
      printer(result_tgfx_INVALIDARGUMENT,
              "get_uniformtypes_sizeinbytes() has failed because input isn't supported!");
      return 0;
  }
}
inline VkPresentModeKHR vk_findPresentModeVk(windowpresentation_tgfx p) {
  switch (p) {
    case windowpresentation_tgfx_FIFO: return VK_PRESENT_MODE_FIFO_KHR;
    case windowpresentation_tgfx_FIFO_RELAXED: return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    case windowpresentation_tgfx_IMMEDIATE: return VK_PRESENT_MODE_IMMEDIATE_KHR;
    case windowpresentation_tgfx_MAILBOX: return VK_PRESENT_MODE_MAILBOX_KHR;
    default:
      printer(result_tgfx_NOTCODED, "This presentation mode isn't supported by the VK backend!");
      return VK_PRESENT_MODE_MAX_ENUM_KHR;
  }
}
inline windowpresentation_tgfx vk_findPresentModeTgfx(VkPresentModeKHR p) {
  switch (p) {
    case VK_PRESENT_MODE_FIFO_KHR: return windowpresentation_tgfx_FIFO;
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return windowpresentation_tgfx_FIFO_RELAXED;
    case VK_PRESENT_MODE_IMMEDIATE_KHR: return windowpresentation_tgfx_IMMEDIATE;
    case VK_PRESENT_MODE_MAILBOX_KHR: return windowpresentation_tgfx_MAILBOX;
    default:
      printer(result_tgfx_NOTCODED, "This presentation mode isn't supported by the VK backend!");
      return windowpresentation_tgfx_FIFO;
  }
}
inline VkColorSpaceKHR vk_findColorSpaceVk(colorspace_tgfx cs) {
  switch (cs) {
    case colorspace_tgfx_sRGB_NONLINEAR: return VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    case colorspace_tgfx_HDR10_ST2084: return VK_COLOR_SPACE_HDR10_ST2084_EXT;
    case colorspace_tgfx_EXTENDED_sRGB_LINEAR: return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
    default:
      printer(result_tgfx_NOTCODED, "ColorSpace isn't supported by Find_VkColorSpace_byTGFX");
  }
  return VK_COLOR_SPACE_MAX_ENUM_KHR;
}
inline VkColorComponentFlags vk_findColorWriteMask(textureChannels_tgfx chnnls) {
  switch (chnnls) {
    case texture_channels_tgfx_BGRA8UB:
    case texture_channels_tgfx_BGRA8UNORM:
    case texture_channels_tgfx_RGBA32F:
    case texture_channels_tgfx_RGBA32UI:
    case texture_channels_tgfx_RGBA32I:
    case texture_channels_tgfx_RGBA8UB:
    case texture_channels_tgfx_RGBA8B:
      return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
             VK_COLOR_COMPONENT_A_BIT;
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
    case texture_channels_tgfx_RA8B: return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT;
    case texture_channels_tgfx_R32F:
    case texture_channels_tgfx_R32UI:
    case texture_channels_tgfx_R32I: return VK_COLOR_COMPONENT_R_BIT;
    case texture_channels_tgfx_R8UB:
    case texture_channels_tgfx_R8B: return VK_COLOR_COMPONENT_R_BIT;
    case texture_channels_tgfx_D32:
    case texture_channels_tgfx_D24S8:
    default:
      printer(result_tgfx_NOTCODED,
              "Find_ColorWriteMask_byChannels() doesn't support this type of RTSlot channel!");
      return VK_COLOR_COMPONENT_FLAG_BITS_MAX_ENUM;
  }
}
inline VkColorComponentFlags vk_findColorComponentsVk(textureComponentMask_tgfx mask,
                                                      textureChannels_tgfx format) {
  if (mask == textureComponentMask_tgfx_ALL && format != texture_channels_tgfx_UNDEF) {
    return vk_findColorWriteMask(format);
  }
  if (mask == textureComponentMask_tgfx_NONE) {
    return 0;
  }
  VkColorComponentFlags flag = {};
  flag |= (mask & textureComponentMask_tgfx_R) ? VK_COLOR_COMPONENT_R_BIT : 0;
  flag |= (mask & textureComponentMask_tgfx_G) ? VK_COLOR_COMPONENT_G_BIT : 0;
  flag |= (mask & textureComponentMask_tgfx_B) ? VK_COLOR_COMPONENT_B_BIT : 0;
  flag |= (mask & textureComponentMask_tgfx_A) ? VK_COLOR_COMPONENT_A_BIT : 0;
  return flag;
}
inline colorspace_tgfx vk_findColorSpaceTgfx(VkColorSpaceKHR cs) {
  switch (cs) {
    case VK_COLORSPACE_SRGB_NONLINEAR_KHR: return colorspace_tgfx_sRGB_NONLINEAR;
    case VK_COLOR_SPACE_HDR10_ST2084_EXT: return colorspace_tgfx_HDR10_ST2084;
    case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT: return colorspace_tgfx_EXTENDED_sRGB_LINEAR;
    default:
      printer(result_tgfx_NOTCODED, "ColorSpace isn't supported by Find_TGFXColorSpace_byVk");
      return ( colorspace_tgfx )UINT32_MAX;
  }
}
inline gpu_type_tgfx vk_findGPUTypeTgfx(VkPhysicalDeviceType t) {
  switch (t) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return gpu_type_tgfx_DISCRETE_GPU;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return gpu_type_tgfx_INTEGRATED_GPU;
    default: return ( gpu_type_tgfx )UINT32_MAX;
  }
}
inline VkPipelineBindPoint vk_findPipelineBindPoint(pipelineType_tgfx type) {
  switch (type) {
    case pipelineType_tgfx_COMPUTE: return VK_PIPELINE_BIND_POINT_COMPUTE;
    case pipelineType_tgfx_RASTER: return VK_PIPELINE_BIND_POINT_GRAPHICS;
    case pipelineType_tgfx_RAYTRACING: return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
    default:
      printer(result_tgfx_FAIL, "vk_findPipelineBindPoint() doesn't support this pipeline!");
      return VK_PIPELINE_BIND_POINT_MAX_ENUM;
  }
}