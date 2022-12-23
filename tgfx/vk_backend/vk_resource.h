#pragma once
#include <tgfx_structs.h>

#include <atomic>
#include <glm/glm.hpp>

#include "vk_includes.h"
#include "vk_predefinitions.h"

// Memory Management

// Represents how it's bind to a heap
struct memoryBlock_vk {
  heap_tgfxhnd                 m_heap;
  VkDeviceSize                 vk_offset;
  static const memoryBlock_vk& GETINVALID() {
    memoryBlock_vk invalid;
    invalid.m_heap    = nullptr;
    invalid.vk_offset = UINT64_MAX;
    return invalid;
  }
};
//
struct memoryRequirements_vk {
  VkMemoryRequirements                vk_memReqs;
  bool                                prefersDedicatedAlloc : 1, requiresDedicatedAlloc : 1;
  static const memoryRequirements_vk& GETINVALID() {
    memoryRequirements_vk invalid;
    invalid.prefersDedicatedAlloc     = false;
    invalid.requiresDedicatedAlloc    = false;
    invalid.vk_memReqs.memoryTypeBits = 0;
    invalid.vk_memReqs.size           = 0;
    invalid.vk_memReqs.alignment      = UINT64_MAX;
    return invalid;
  }
};

// Classic Memory Resources

struct TEXTURE_VKOBJ {
  std::atomic_bool isALIVE    = false;
  vk_handleType    HANDLETYPE = VKHANDLETYPEs::TEXTURE;
  static uint16_t  GET_EXTRAFLAGS(TEXTURE_VKOBJ* obj) { return 0; }

  void operator=(const TEXTURE_VKOBJ& src) {
    isALIVE.store(true);
    m_width       = src.m_width;
    m_height      = src.m_height;
    m_mips        = src.m_mips;
    m_channels    = src.m_channels;
    vk_imageUsage = src.vk_imageUsage;
    m_dim         = src.m_dim;
    m_memBlock    = src.m_memBlock;
    vk_image      = src.vk_image;
    vk_imageView  = src.vk_imageView;
    m_memReqs     = src.m_memReqs;
    m_GPU         = src.m_GPU;
  }

  unsigned int            m_width, m_height;
  unsigned char           m_mips;
  textureChannels_tgfx    m_channels;
  texture_dimensions_tgfx m_dim;
  memoryBlock_vk          m_memBlock = memoryBlock_vk::GETINVALID();
  memoryRequirements_vk   m_memReqs  = memoryRequirements_vk::GETINVALID();
  uint8_t                 m_GPU;

  VkImage           vk_image      = {};
  VkImageView       vk_imageView  = {};
  VkImageUsageFlags vk_imageUsage = {};
};
struct BUFFER_VKOBJ {
  std::atomic_bool isALIVE    = false;
  vk_handleType    HANDLETYPE = VKHANDLETYPEs::BUFFER;
  static uint16_t  GET_EXTRAFLAGS(BUFFER_VKOBJ* obj) { return 0; }

  void operator=(const BUFFER_VKOBJ& src) {
    isALIVE.store(true);
    vk_usage   = src.vk_usage;
    vk_buffer  = src.vk_buffer;
    m_memBlock = src.m_memBlock;
    m_GPU      = src.m_GPU;
    m_memReqs  = src.m_memReqs;
  }

  memoryBlock_vk        m_memBlock;
  memoryRequirements_vk m_memReqs;
  uint8_t               m_GPU;
  uint64_t              m_intendedSize = UINT64_MAX;

  VkBuffer           vk_buffer;
  VkBufferUsageFlags vk_usage;
};

// Framebuffer RT Slot Management

struct colorslot_vk {
  TEXTURE_VKOBJ*      RT;
  rasterpassLoad_tgfx LOADSTATE;
  bool                IS_USED_LATER;
  operationtype_tgfx  RT_OPERATIONTYPE;
  glm::vec4           CLEAR_COLOR;
  std::atomic_bool    IsChanged = false;
};
struct depthstencilslot_vk {
  TEXTURE_VKOBJ*      RT;
  rasterpassLoad_tgfx DEPTH_LOAD, STENCIL_LOAD;
  bool                IS_USED_LATER;
  operationtype_tgfx  DEPTH_OPTYPE, STENCIL_OPTYPE;
  glm::vec2           CLEAR_COLOR;
  std::atomic_bool    IsChanged = false;
};
struct rtslots_vk {
  colorslot_vk*        COLOR_SLOTs      = nullptr;
  unsigned char        COLORSLOTs_COUNT = 0;
  depthstencilslot_vk* DEPTHSTENCIL_SLOT =
    nullptr; // There is one, but there may not be a Depth Slot. So if there is no, then this is
             // nullptr.
  // Unused Depth and NoDepth are different. Unused Depth means RenderPass does have one but current
  // Subpass doesn't use, but NoDepth means RenderPass doesn't have one!
  std::atomic_bool IsChanged = false;

  void operator=(const rtslots_vk& copyFrom) {
    IsChanged.store(copyFrom.IsChanged.load());
    DEPTHSTENCIL_SLOT = copyFrom.DEPTHSTENCIL_SLOT;
    COLOR_SLOTs       = copyFrom.COLOR_SLOTs;
    COLORSLOTs_COUNT  = copyFrom.COLORSLOTs_COUNT;
  }
};

/*
struct RTSLOTSET_VKOBJ {
  std::atomic_bool isALIVE    = false;
  vk_handleType    HANDLETYPE = VKHANDLETYPEs::RTSLOTSET;
  static uint16_t  GET_EXTRAFLAGS(RTSLOTSET_VKOBJ* obj) { return 0; }

  void operator=(const RTSLOTSET_VKOBJ& copyFrom) {
    isALIVE.store(true);
    PERFRAME_SLOTSETs[0] = copyFrom.PERFRAME_SLOTSETs[0];
    PERFRAME_SLOTSETs[1] = copyFrom.PERFRAME_SLOTSETs[1];
    FB_ci[0]             = copyFrom.FB_ci[0];
    FB_ci[1]             = copyFrom.FB_ci[1];
  }
  rtslots_vk PERFRAME_SLOTSETs[2];
  // You should change this struct's vkRenderPass object pointer as your vkRenderPass object
  VkFramebufferCreateInfo FB_ci[2];
};
struct IRTSLOTSET_VKOBJ {
  std::atomic_bool isALIVE    = false;
  vk_handleType    HANDLETYPE = VKHANDLETYPEs::IRTSLOTSET;
  static uint16_t  GET_EXTRAFLAGS(IRTSLOTSET_VKOBJ* obj) { return 0; }

  void operator=(const IRTSLOTSET_VKOBJ& copyFrom) {
    isALIVE.store(true);
    BASESLOTSET    = copyFrom.BASESLOTSET;
    COLOR_OPTYPEs  = copyFrom.COLOR_OPTYPEs;
    DEPTH_OPTYPE   = copyFrom.DEPTH_OPTYPE;
    STENCIL_OPTYPE = copyFrom.STENCIL_OPTYPE;
  }
  uint32_t            BASESLOTSET;
  operationtype_tgfx* COLOR_OPTYPEs;
  operationtype_tgfx  DEPTH_OPTYPE;
  operationtype_tgfx  STENCIL_OPTYPE;
};
struct rtslot_create_description_vk {
  TEXTURE_VKOBJ*     textures[2];
  operationtype_tgfx optype;
  drawpassload_tgfx  loadtype;
  bool               isUsedLater;
  vec4_tgfx          clear_value;
};
struct rtslot_inheritance_descripton_vk {
  bool               IS_DEPTH = false;
  operationtype_tgfx OPTYPE = operationtype_tgfx_UNUSED, OPTYPESTENCIL = operationtype_tgfx_UNUSED;
  drawpassload_tgfx  LOADTYPE = drawpassload_tgfx_CLEAR, LOADTYPESTENCIL = drawpassload_tgfx_CLEAR;
};*/

struct SUBRASTERPASS_VKOBJ {
  std::atomic_bool               isALIVE    = false;
  static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::SUBRASTERPASS;
  static uint16_t                GET_EXTRAFLAGS(SUBRASTERPASS_VKOBJ* obj) { return 0; }

  uint32_t     m_subpassIndx;
  gpu_tgfxhnd  m_gpu;
  VkRenderPass vk_renderPass; // It's same across all subpasses
  bool         isDepthAttachment = false;
  // Extra information to check raster pipeline compilations without relying on validation layer
};
struct SAMPLER_VKOBJ {
  std::atomic_bool isALIVE    = false;
  vk_handleType    HANDLETYPE = VKHANDLETYPEs::SAMPLER;
  static uint16_t  GET_EXTRAFLAGS(SAMPLER_VKOBJ* obj) { return obj->m_flags.load(); }

  VkSampler            vk_sampler = VK_NULL_HANDLE;
  std::atomic_uint16_t m_flags    = 0; // YCbCr conversion only flag for now
  uint8_t              m_gpu;
};

/////////////////////////////////////////////
//				PIPELINE RESOURCES
/////////////////////////////////////////////

struct PIPELINE_VKOBJ {
  std::atomic_bool isALIVE    = false;
  vk_handleType    HANDLETYPE = VKHANDLETYPEs::PIPELINE;
  static uint16_t  GET_EXTRAFLAGS(PIPELINE_VKOBJ* obj) { return obj->vk_type; }

  uint8_t  m_gpu;

  VkPipelineLayout    vk_layout = VK_NULL_HANDLE;
  VkPipeline          vk_object = VK_NULL_HANDLE;
  VkPipelineBindPoint vk_type   = VK_PIPELINE_BIND_POINT_MAX_ENUM;

  VkFormat vk_colorAttachmentFormats[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT] = {};
  VkFormat vk_depthAttachmentFormat                                           = {};
};
struct depthsettingsdesc_vk {
  VkBool32    ShouldWrite    = VK_FALSE;
  VkCompareOp DepthCompareOP = VkCompareOp::VK_COMPARE_OP_MAX_ENUM;
  // DepthBounds Extension
  VkBool32 DepthBoundsEnable = VK_FALSE;
  float    DepthBoundsMin = FLT_MIN, DepthBoundsMax = FLT_MAX;
};
struct stencildesc_vk {
  VkStencilOpState OPSTATE;
};
struct blendinginfo_vk {
  unsigned char                       COLORSLOT_INDEX   = 255;
  glm::vec4                           BLENDINGCONSTANTs = glm::vec4(FLT_MAX);
  VkPipelineColorBlendAttachmentState BlendState        = {};
};

vk_uint32c VKCONST_MAXVERTEXATTRIBCOUNT = 16, VKCONST_MAXVERTEXBINDINGCOUNT = 4;
struct VERTEXATTRIBLAYOUT_VKOBJ {
  std::atomic_bool isALIVE    = false;
  vk_handleType    HANDLETYPE = VKHANDLETYPEs::VERTEXATTRIB;
  static uint16_t  GET_EXTRAFLAGS(VERTEXATTRIBLAYOUT_VKOBJ* obj) { return 0; }

  VkVertexInputBindingDescription   bindingDescs[VKCONST_MAXVERTEXBINDINGCOUNT];
  VkVertexInputAttributeDescription attribDescs[VKCONST_MAXVERTEXATTRIBCOUNT];
  unsigned char                     attribDescsCount, bindingDescsCount;
};

struct HEAP_VKOBJ {
  std::atomic_bool isALIVE    = false;
  vk_handleType    HANDLETYPE = VKHANDLETYPEs::HEAP;

  static uint16_t GET_EXTRAFLAGS(HEAP_VKOBJ* obj) {
    // assert(0 && "GPU index & memTypeIndex should be passed as extra flag");
    return 0;
  }
  void operator=(const HEAP_VKOBJ& src) {
    isALIVE.store(true);
    vk_memoryHandle    = src.vk_memoryHandle;
    vk_memoryTypeIndex = src.vk_memoryTypeIndex;
    m_size             = src.m_size;
  }

  uint8_t            m_GPU;
  unsigned long long m_size;

  VkDeviceMemory vk_memoryHandle;
  unsigned int   vk_memoryTypeIndex;
};

struct cmdPool_vk;
struct cmdBundleRef_vk {
  cmdPool_vk*           m_cmdPool;
  commandBundle_tgfxhnd m_cmdBundle;
  VkCommandBuffer       vk_cmdBuffer;
};

struct FRAMEBUFFER_VKOBJ {
  std::atomic_bool isALIVE    = false;
  vk_handleType    HANDLETYPE = VKHANDLETYPEs::INTERNAL;
  static uint16_t  GET_EXTRAFLAGS(FRAMEBUFFER_VKOBJ* obj) { return 0; }

  texture_tgfxhnd m_textures[VKCONST_MAXRTSLOTCOUNT];
  // Command bundles records a command buffer for each framebuffer
  // For now, proper tracking mechanism isn't implemented.
  static constexpr uint32_t MAXCMDBUNDLECOUNT                  = 32;
  uint32_t                  m_cmdBundleCount                   = 0;
  cmdBundleRef_vk           m_cmdBundleRefs[MAXCMDBUNDLECOUNT] = {};
  VkFramebuffer             vk_framebuffer                     = {};
};