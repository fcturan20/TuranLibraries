#define T_INCLUDE_PLATFORM_LIBS
#include "vk_contentmanager.h"

#include <glslang/SPIRV/GlslangToSpv.h>
#include <tgfx_core.h>
#include <tgfx_gpucontentmanager.h>

#include <mutex>

#include "profiler_tapi.h"
#include "vk_core.h"
#include "vk_extension.h"
#include "vk_helper.h"
#include "vk_includes.h"
#include "vk_predefinitions.h"
#include "vk_queue.h"
#include "vk_renderer.h"
#include "vk_resource.h"
#include "vkext_dynamicStates.h"

vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_CONTENTMANAGER = nullptr;

// Binding Model and Table Management

// These are used to initialize descriptors
// So backend won't fail because of a NULL descriptor
//     -but in DEBUG release, it'll complain about it-
// Texture is a 1x1 texture, buffer is 1 byte buffer, sampler is as default as possible
VkSampler      defaultSampler;
VkBuffer       defaultBuffer;
TEXTURE_VKOBJ* defaultTexture;

struct sampler_descVK {
  VkSampler sampler_obj = defaultSampler;
};

struct buffer_descVK { // Both for BUFFER and EXT_UNIFORMBUFFER
  VkDescriptorBufferInfo info = {};
};

struct texture_descVK { // Both for SAMPLEDTEXTURE and STORAGEIMAGE
  VkDescriptorImageInfo info = {};
};

struct SHADERSOURCE_VKOBJ {
  bool                           isALIVE    = false;
  static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::SHADERSOURCE;
  static uint16_t                GET_EXTRAFLAGS(SHADERSOURCE_VKOBJ* obj) { return obj->stage; }

  void operator=(const SHADERSOURCE_VKOBJ& copyFrom) {
    isALIVE     = true;
    Module      = copyFrom.Module;
    stage       = copyFrom.stage;
    SOURCE_CODE = copyFrom.SOURCE_CODE;
    DATA_SIZE   = copyFrom.DATA_SIZE;
  }
  VkShaderModule   Module;
  shaderStage_tgfx stage;
  void*            SOURCE_CODE = nullptr;
  unsigned int     DATA_SIZE   = 0;
  gpu_tgfxhnd      m_gpu;
};

struct gpudatamanager_private {
  VK_LINEAR_OBJARRAY<TEXTURE_VKOBJ, texture_tgfxhnd, 1 << 24>               textures;
  VK_LINEAR_OBJARRAY<PIPELINE_VKOBJ, pipeline_tgfxhnd, 1 << 24>             pipelines;
  VK_LINEAR_OBJARRAY<SHADERSOURCE_VKOBJ, shaderSource_tgfxhnd, 1 << 24>     shadersources;
  VK_LINEAR_OBJARRAY<SAMPLER_VKOBJ, sampler_tgfxhnd, 1 << 16>               samplers;
  VK_LINEAR_OBJARRAY<BUFFER_VKOBJ, buffer_tgfxhnd>                          buffers;
  VK_LINEAR_OBJARRAY<BINDINGTABLEINST_VKOBJ, bindingTable_tgfxhnd, 1 << 16> bindingtableinsts;
  VK_LINEAR_OBJARRAY<HEAP_VKOBJ, heap_tgfxhnd, 1 << 10>                     heaps;

  gpudatamanager_private() {}
};
static gpudatamanager_private* hidden = nullptr;

void vk_destroyAllResources() {}

result_tgfx vk_createSampler(gpu_tgfxhnd gpu, const samplerDescription_tgfx* desc,
                             sampler_tgfxhnd* hnd) {
  GPU_VKOBJ* GPU = getOBJ<GPU_VKOBJ>(gpu);

  VkSampler sampler;
  {
    VkSamplerCreateInfo s_ci     = {};
    s_ci.addressModeU            = vk_findAddressModeVk(desc->wrapWidth);
    s_ci.addressModeV            = vk_findAddressModeVk(desc->wrapHeight);
    s_ci.addressModeW            = vk_findAddressModeVk(desc->wrapDepth);
    s_ci.anisotropyEnable        = VK_FALSE;
    s_ci.borderColor             = VkBorderColor::VK_BORDER_COLOR_MAX_ENUM;
    s_ci.compareEnable           = VK_FALSE;
    s_ci.flags                   = 0;
    s_ci.magFilter               = vk_findFilterVk(desc->magFilter);
    s_ci.minFilter               = vk_findFilterVk(desc->minFilter);
    s_ci.maxLod                  = static_cast<float>(desc->MaxMipLevel);
    s_ci.minLod                  = static_cast<float>(desc->MinMipLevel);
    s_ci.mipLodBias              = 0.0f;
    s_ci.mipmapMode              = vk_findMipmapModeVk(desc->minFilter);
    s_ci.pNext                   = nullptr;
    s_ci.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    s_ci.unnormalizedCoordinates = VK_FALSE;
    if (vkCreateSampler(GPU->vk_logical, &s_ci, nullptr, &sampler) != VK_SUCCESS) {
      return vkPrint(26);
    }
  }

  SAMPLER_VKOBJ* SAMPLER = hidden->samplers.create_OBJ();
  SAMPLER->vk_sampler    = sampler;
  SAMPLER->m_gpu         = GPU->gpuIndx();
  *hnd                   = getHANDLE<sampler_tgfxhnd>(SAMPLER);
  return result_tgfx_SUCCESS;
}
void vk_destroySampler(sampler_tgfxhnd sampler) {
  SAMPLER_VKOBJ* vkSampler = getOBJ<SAMPLER_VKOBJ>(sampler);
#ifdef VULKAN_DEBUGGING
  assert(vkSampler && "Invalid sampler!");
#endif // VULKAN_DEBUGGING
  vkDestroySampler(core_vk->getGPU(vkSampler->m_gpu)->vk_logical, vkSampler->vk_sampler, nullptr);
}

/*Attributes are ordered as the same order of input vector
 * For example: Same attribute ID may have different location/order in another attribute layout
 * So you should gather your vertex buffer data according to that
 */

unsigned int vk_calculateSizeOfVertexLayout(const datatype_tgfx* ATTRIBUTEs, unsigned int count);

result_tgfx vk_createTexture(gpu_tgfxhnd i_gpu, const textureDescription_tgfx* desc,
                             texture_tgfxhnd* TextureHandle) {
  GPU_VKOBJ*        gpu       = getOBJ<GPU_VKOBJ>(i_gpu);
  VkImageUsageFlags usageFlag = vk_findTextureUsageFlagVk(desc->usage);
  if (desc->channelType == texture_channels_tgfx_D24S8 ||
      desc->channelType == texture_channels_tgfx_D32) {
    usageFlag &= ~(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
  } else {
    usageFlag &= ~(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
  }

  if (desc->mipCount > std::floor(std::log2(std::max(desc->width, desc->height))) + 1 ||
      !desc->mipCount) {
    vkPrint(26);
    return core_tgfx_main->getLogMessage(26, nullptr);
  }

  // Create VkImage
  VkImage           vkTextureObj;
  VkImageCreateInfo im_ci = {};
  {
    im_ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    im_ci.extent.width  = desc->width;
    im_ci.extent.height = desc->height;
    im_ci.extent.depth  = 1;
    if (desc->dimension == texture_dimensions_tgfx_2DCUBE) {
      im_ci.flags       = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
      im_ci.arrayLayers = 6;
    } else {
      im_ci.flags       = 0;
      im_ci.arrayLayers = 1;
    }
    im_ci.format        = vk_findFormatVk(desc->channelType);
    im_ci.imageType     = vk_findImageTypeVk(desc->dimension);
    im_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    im_ci.mipLevels     = static_cast<uint32_t>(desc->mipCount);
    im_ci.pNext         = nullptr;

    uint32_t queueFamIndexList[VKCONST_MAXQUEUEFAMCOUNT_PERGPU];
    VK_getQueueAndSharingInfos(desc->permittedQueueCount, desc->permittedQueues, 0, nullptr,
                               queueFamIndexList, &im_ci.queueFamilyIndexCount, &im_ci.sharingMode);
    im_ci.pQueueFamilyIndices = queueFamIndexList;

    im_ci.tiling  = Find_VkTiling(desc->dataOrder);
    im_ci.usage   = usageFlag;
    im_ci.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(gpu->vk_logical, &im_ci, nullptr, &vkTextureObj) != VK_SUCCESS) {
      return vkPrint(24);
    }
  }

  memoryRequirements_vk memReqs = memoryRequirements_vk::GETINVALID();
  vkGetImageMemoryRequirements(gpu->vk_logical, vkTextureObj, &memReqs.vk_memReqs);
  if (memReqs.requiresDedicatedAlloc) {
    vkPrint(25);
    vkDestroyImage(gpu->vk_logical, vkTextureObj, nullptr);
    return result_tgfx_FAIL;
  }

  TEXTURE_VKOBJ* texture = contentManager->GETTEXTURES_ARRAY().create_OBJ();
  texture->m_channels    = desc->channelType;
  texture->m_height      = desc->height;
  texture->m_width       = desc->width;
  texture->vk_imageUsage = usageFlag;
  texture->m_dim         = desc->dimension;
  texture->m_mips        = desc->mipCount;
  texture->vk_image      = vkTextureObj;
  texture->vk_imageView  = VK_NULL_HANDLE;
  texture->m_GPU         = gpu->gpuIndx();
  texture->vk_imageUsage = im_ci.usage;
  texture->m_memReqs     = memReqs;

  *TextureHandle = getHANDLE<texture_tgfxhnd>(texture);
  return result_tgfx_SUCCESS;
}
void vk_destroyTexture(texture_tgfxhnd texture) {
  TEXTURE_VKOBJ* vkTexture = getOBJ<TEXTURE_VKOBJ>(texture);
  assert(vkTexture && "Invalid texture");
  vkDestroyImage(core_vk->getGPU(vkTexture->m_GPU)->vk_logical, vkTexture->vk_image, nullptr);
  vkDestroyImageView(core_vk->getGPU(vkTexture->m_GPU)->vk_logical, vkTexture->vk_imageView,
                     nullptr);
  hidden->textures.destroyObj(hidden->textures.getINDEXbyOBJ(vkTexture));
}

result_tgfx vk_createBuffer(gpu_tgfxhnd i_gpu, const bufferDescription_tgfx* desc,
                            buffer_tgfxhnd* buffer) {
  GPU_VKOBJ* gpu = getOBJ<GPU_VKOBJ>(i_gpu);

  // Create VkBuffer object
  VkBuffer           vkBufObj;
  VkBufferCreateInfo ci = {};
  {
    ci.usage = vk_findBufferUsageFlagVk(desc->usageFlag);

    uint32_t queueFamIndexList[VKCONST_MAXQUEUEFAMCOUNT_PERGPU];
    VK_getQueueAndSharingInfos(desc->permittedQueueCount, desc->permittedQueues, 0, nullptr,
                               queueFamIndexList, &ci.queueFamilyIndexCount, &ci.sharingMode);
    ci.pQueueFamilyIndices = queueFamIndexList;
    ci.sType               = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size                = desc->dataSize;

    if (vkCreateBuffer(gpu->vk_logical, &ci, nullptr, &vkBufObj) != VK_SUCCESS) {
      return vkPrint(27);
    }
  }
  memoryRequirements_vk memReqs;
  vkGetBufferMemoryRequirements(gpu->vk_logical, vkBufObj, &memReqs.vk_memReqs);
  if (memReqs.requiresDedicatedAlloc) {
    vkPrint(25);
    vkDestroyBuffer(gpu->vk_logical, vkBufObj, nullptr);
    return core_tgfx_main->getLogMessage(25, nullptr);
  }

  // Get buffer requirements and fill BUFFER_VKOBJ
  BUFFER_VKOBJ* o_buffer = hidden->buffers.create_OBJ();
  {
    o_buffer->vk_buffer      = vkBufObj;
    o_buffer->m_GPU          = gpu->gpuIndx();
    o_buffer->vk_usage       = ci.usage;
    o_buffer->m_intendedSize = desc->dataSize;
    o_buffer->m_memReqs      = memReqs;
  }

  *buffer = getHANDLE<buffer_tgfxhnd>(o_buffer);
  return result_tgfx_SUCCESS;
}
void vk_destroyBuffer(buffer_tgfxhnd buffer) {
  BUFFER_VKOBJ* vkBuffer = getOBJ<BUFFER_VKOBJ>(buffer);
  assert(vkBuffer && "Invalid texture");
  vkDestroyBuffer(core_vk->getGPU(vkBuffer->m_GPU)->vk_logical, vkBuffer->vk_buffer, nullptr);
  hidden->buffers.destroyObj(hidden->buffers.getINDEXbyOBJ(vkBuffer));
}

vk_uint32c VKCONST_MAXSTATICSAMPLERCOUNT = 128;

void* getInvalidResourceByType(GPU_VKOBJ* gpu, VkDescriptorType descType) {
  switch (descType) {
    case VK_DESCRIPTOR_TYPE_SAMPLER: return gpu->m_invalidSampler;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return gpu->m_invalidBuffer;
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return gpu->m_invalidShaderReadTexture;
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return gpu->m_invalidStorageTexture;
    default: assert(0 && "Invalid descriptor type"); return nullptr;
  }
}
VkDescriptorSetLayout createDescriptorSetLayout(
  GPU_VKOBJ* gpu, const bindingTableDescription_tgfx const* const desc) {
  // Static Immutable Sampler binding
  VkDescriptorSetLayoutBinding bindngs[2] = {};
  VkSampler                    staticSamplers[VKCONST_MAXSTATICSAMPLERCOUNT];
  unsigned int                 dynamicbinding_i = 0;
  if (desc->DescriptorType == shaderdescriptortype_tgfx_SAMPLER && desc->staticSamplerCount) {
    for (uint32_t i = 0; i < desc->staticSamplerCount; i++) {
      if (!getOBJ<SAMPLER_VKOBJ>(desc->staticSamplers[i])) {
        vkPrint(10);
        return nullptr;
      }

      i++;
    }
    for (unsigned int i = 0; i < desc->staticSamplerCount; i++) {
      staticSamplers[i] = getOBJ<SAMPLER_VKOBJ>(desc->staticSamplers[i])->vk_sampler;
    }
    bindngs[0].binding            = dynamicbinding_i;
    bindngs[0].descriptorCount    = desc->staticSamplerCount;
    bindngs[0].descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLER;
    bindngs[0].pImmutableSamplers = staticSamplers;
    bindngs[0].stageFlags         = VK_SHADER_STAGE_ALL;

    dynamicbinding_i++;
  }

  // Main binding
  {
    bindngs[dynamicbinding_i].binding            = dynamicbinding_i;
    bindngs[dynamicbinding_i].descriptorCount    = desc->ElementCount;
    bindngs[dynamicbinding_i].descriptorType     = vk_findDescTypeVk(desc->DescriptorType);
    bindngs[dynamicbinding_i].pImmutableSamplers = nullptr;
    bindngs[dynamicbinding_i].stageFlags         = vk_findShaderStageVk(desc->visibleStagesMask);
  }

  VkDescriptorSetLayout dsl = {};
  {
    VkDescriptorSetLayoutBindingFlagsCreateInfo dynCi = {};
    if (desc->isDynamic) {
      dynCi.bindingCount            = 1;
      VkDescriptorBindingFlags flag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
                                      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
                                      VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT |
                                      VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;
      dynCi.pBindingFlags = &flag;
      dynCi.pNext         = nullptr;
      dynCi.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    }
    VkDescriptorSetLayoutCreateInfo ci = {};
    ci.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ci.flags = desc->isDynamic ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT : 0;
    ci.pNext = desc->isDynamic ? &dynCi : nullptr;
    ci.bindingCount = dynamicbinding_i + 1;
    ci.pBindings    = bindngs;
    if (vkCreateDescriptorSetLayout(gpu->vk_logical, &ci, nullptr, &dsl) != VK_SUCCESS) {
      vkPrint(28);
      return nullptr;
    }
  }
  return dsl;
}
result_tgfx vk_createBindingTable(gpu_tgfxhnd                                     i_gpu,
                                  const bindingTableDescription_tgfx const* const desc,
                                  bindingTable_tgfxhnd*                           table) {
#ifdef VULKAN_DEBUGGING
  // Check if pool has enough space for the desc set and if there is, then decrease the amount of
  // available descs in the pool for other checks
  if (!desc->ElementCount && !desc->staticSamplerCount) {
    return vkPrint(29);
  }
  if (desc->staticSamplerCount && desc->DescriptorType != shaderdescriptortype_tgfx_SAMPLER) {
    return vkPrint(30);
  }
#endif
  GPU_VKOBJ* gpu = getOBJ<GPU_VKOBJ>(i_gpu);

  // Create Descriptor Pool
  VkDescriptorPool pool;
  {
    VkDescriptorPoolCreateInfo ci = {};
    ci.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ci.maxSets                    = 1;
    ci.pNext                      = nullptr;
    ci.poolSizeCount              = 1;
    VkDescriptorPoolSize size     = {};
    size.descriptorCount          = desc->ElementCount + desc->staticSamplerCount;
    size.type                     = vk_findDescTypeVk(desc->DescriptorType);
    ci.pPoolSizes                 = &size;
    ci.flags = desc->isDynamic ? VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT : 0;
    if (vkCreateDescriptorPool(gpu->vk_logical, &ci, nullptr, &pool) != VK_SUCCESS) {
      return vkPrint(28);
    }
  }

  // Create DescriptorSet Layout
  VkDescriptorSetLayout DSL = createDescriptorSetLayout(gpu, desc);

  VkDescriptorSet set;
  // Allocate Descriptor Set
  {
    VkDescriptorSetVariableDescriptorCountAllocateInfo descIndexing = {};
    {
      descIndexing.descriptorSetCount = 1;
      descIndexing.pDescriptorCounts  = &desc->ElementCount;
      descIndexing.pNext              = nullptr;
      descIndexing.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
    }
    VkDescriptorSetAllocateInfo ai = {};
    ai.descriptorPool              = pool;
    ai.descriptorSetCount          = 1;
    ai.pNext                       = desc->isDynamic ? &descIndexing : nullptr;
    ai.pSetLayouts                 = &DSL;
    ai.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    if (vkAllocateDescriptorSets(gpu->vk_logical, &ai, &set) != VK_SUCCESS) {
      vkDestroyDescriptorPool(gpu->vk_logical, pool, nullptr);
      return vkPrint(31);
    }
  }

  BINDINGTABLEINST_VKOBJ* finalobj = hidden->bindingtableinsts.create_OBJ();
  finalobj->vk_pool                = pool;
  finalobj->vk_set                 = set;
  finalobj->m_isStatic             = !desc->isDynamic;
  finalobj->vk_layout              = DSL;
  finalobj->vk_descType            = vk_findDescTypeVk(desc->DescriptorType);
  finalobj->vk_stages              = vk_findShaderStageVk(desc->visibleStagesMask);
  finalobj->m_gpu                  = gpu->gpuIndx();
  finalobj->m_elementCount         = desc->ElementCount;
  switch (finalobj->vk_descType) {
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      finalobj->m_descs = new (VKGLOBAL_VIRMEM_CONTENTMANAGER) texture_descVK[desc->ElementCount];
      break;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      finalobj->m_descs = new (VKGLOBAL_VIRMEM_CONTENTMANAGER) buffer_descVK[desc->ElementCount];
      break;
    case VK_DESCRIPTOR_TYPE_SAMPLER:
      finalobj->m_descs = new (VKGLOBAL_VIRMEM_CONTENTMANAGER) sampler_descVK[desc->ElementCount];
      break;
  }
  *table = getHANDLE<bindingTable_tgfxhnd>(finalobj);
  /*
  // If descriptor set is static, zero initialize all of the descs
  if (isStatic) {
    static constexpr uint32_t batchSize  = 1 << 12;
    static const uint32_t     batchCount = (vkTableType->m_elementCount / batchSize) + 1;
    for (uint32_t batchIndx = 0; batchIndx < batchCount; batchIndx++) {
      void*          resourceHnd[batchSize]    = {};
      uint32_t       bindingIndices[batchSize] = {};
      const uint32_t bindingCount =
        (batchIndx == batchCount - 1) ? (batchSize) : (vkTableType->m_elementCount % batchSize);
      if (bindingCount == 0) {
        continue;
      }

      void* invalidResource = getInvalidResourceByType(gpu, vkTableType->vk_descType);
      for (uint32_t iterIndx = 0; iterIndx < bindingCount; iterIndx++) {
        bindingIndices[iterIndx] = (batchSize * batchIndx) + iterIndx;
        resourceHnd[iterIndx]    = invalidResource;
      }

      switch (vkTableType->vk_descType) {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
          core_tgfx_main->contentmanager->setBindingTable_Sampler(
            *table, bindingCount, bindingIndices, ( sampler_tgfxlsthnd )resourceHnd);
          break;
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
          core_tgfx_main->contentmanager->setBindingTable_Buffer(
            *table, bindingCount, bindingIndices, ( buffer_tgfxlsthnd )resourceHnd, {}, {}, {});
          break;
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
          core_tgfx_main->contentmanager->setBindingTable_Texture(
            *table, bindingCount, bindingIndices, ( texture_tgfxlsthnd )resourceHnd);
          break;
      }
    }
  }*/

  return result_tgfx_SUCCESS;
}
void vk_destroyBindingTable(bindingTable_tgfxhnd bindingTable) {
  BINDINGTABLEINST_VKOBJ* vkDescSet = getOBJ<BINDINGTABLEINST_VKOBJ>(bindingTable);
  assert(vkDescSet && "Invalid binding table");
  vkDestroyDescriptorPool(core_vk->getGPU(vkDescSet->m_gpu)->vk_logical, vkDescSet->vk_pool,
                          nullptr);
  hidden->bindingtableinsts.destroyObj(hidden->bindingtableinsts.getINDEXbyOBJ(vkDescSet));
}

// Don't call this if there is no binding table & call buffer!
bool VKPipelineLayoutCreation(GPU_VKOBJ* GPU, unsigned int descSetCount,
                              const bindingTableDescription_tgfx const* const descSets,
                              bool isCallBufferSupported, VkPipelineLayout* layout) {
  VkPipelineLayoutCreateInfo pl_ci = {};
  pl_ci.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pl_ci.pNext                      = nullptr;
  pl_ci.flags                      = 0;

  VkDescriptorSetLayout DESCLAYOUTs[VKCONST_MAXDESCSET_PERLIST] = {};
  for (unsigned int setIndx = 0; setIndx < descSetCount; setIndx++) {
    DESCLAYOUTs[setIndx] = createDescriptorSetLayout(GPU, &descSets[setIndx]);
  }
  pl_ci.setLayoutCount      = descSetCount;
  pl_ci.pSetLayouts         = DESCLAYOUTs;
  VkPushConstantRange range = {};
  // Don't support for now!
  if (isCallBufferSupported) {
    pl_ci.pushConstantRangeCount = 1;
    pl_ci.pPushConstantRanges    = &range;
    range.offset                 = 0;
    range.size                   = 128;
    range.stageFlags             = VK_SHADER_STAGE_ALL;
  } else {
    pl_ci.pushConstantRangeCount = 0;
    pl_ci.pPushConstantRanges    = nullptr;
  }

  if (vkCreatePipelineLayout(GPU->vk_logical, &pl_ci, nullptr, layout) != VK_SUCCESS) {
    vkPrint(16, L"Failed at vkCreatePipelineLayout()");
  }

  for (uint32_t i = 0; i < descSetCount; i++) {
    vkDestroyDescriptorSetLayout(GPU->vk_logical, DESCLAYOUTs[i], nullptr);
  }

  return true;
}

typedef const void* (*vk_glslangCompileFnc)(shaderStage_tgfx tgfxstage, const void* i_DATA,
                                            unsigned int  i_DATA_SIZE,
                                            unsigned int* compiledbinary_datasize);
vk_glslangCompileFnc VKCONST_GLSLANG_COMPILE_FNC;

result_tgfx vk_compileShaderSource(gpu_tgfxhnd gpu, shaderlanguages_tgfx language,
                                   shaderStage_tgfx shaderstage, const void* DATA,
                                   unsigned int          DATA_SIZE,
                                   shaderSource_tgfxhnd* ShaderSourceHandle) {
  GPU_VKOBJ*   GPU                   = getOBJ<GPU_VKOBJ>(gpu);
  const void*  binary_spirv_data     = nullptr;
  unsigned int binary_spirv_datasize = 0;
  switch (language) {
    case shaderlanguages_tgfx_SPIRV:
      binary_spirv_data     = DATA;
      binary_spirv_datasize = DATA_SIZE;
      break;
    case shaderlanguages_tgfx_GLSL:
      binary_spirv_data =
        VKCONST_GLSLANG_COMPILE_FNC(shaderstage, DATA, DATA_SIZE, &binary_spirv_datasize);
      if (!binary_spirv_datasize) {
        delete binary_spirv_data;
        return vkPrint(35);
      }
      break;
    default: vkPrint(16, L"Backend doesn't support this shading language");
  }

  // Create Vertex Shader Module
  VkShaderModuleCreateInfo ci = {};
  ci.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  ci.flags                    = 0;
  ci.pNext                    = nullptr;
  ci.pCode                    = reinterpret_cast<const uint32_t*>(binary_spirv_data);
  ci.codeSize                 = static_cast<size_t>(binary_spirv_datasize);

  VkShaderModule Module;
  if (vkCreateShaderModule(GPU->vk_logical, &ci, 0, &Module) != VK_SUCCESS) {
    return vkPrint(36);
  }

  SHADERSOURCE_VKOBJ* SHADERSOURCE = hidden->shadersources.create_OBJ();
  SHADERSOURCE->Module             = Module;
  SHADERSOURCE->stage              = shaderstage;
  SHADERSOURCE->m_gpu              = gpu;

  *ShaderSourceHandle = getHANDLE<shaderSource_tgfxhnd>(SHADERSOURCE);
  return result_tgfx_SUCCESS;
}
void                  vk_destroyShaderSource(shaderSource_tgfxhnd ShaderSourceHandle) {}
VkColorComponentFlags vk_findColorWriteMask(textureChannels_tgfx chnnls);

result_tgfx vk_createRasterPipeline(const rasterPipelineDescription_tgfx* desc,
                                    pipeline_tgfxhnd*                     hnd) {
  GPU_VKOBJ* GPU = nullptr;

  // Detect GPU & create shader stage infos
  VkPipelineShaderStageCreateInfo STAGEs[2] = {};
  {
    // Find vertex & fragment shader sources and detect GPU
    SHADERSOURCE_VKOBJ *vkSource_vertex = nullptr, *vkSource_fragment = nullptr;
    for (uint32_t shaderIndx = 0; shaderIndx < desc->shaderCount; shaderIndx++) {
      SHADERSOURCE_VKOBJ* source = getOBJ<SHADERSOURCE_VKOBJ>(desc->shaders[shaderIndx]);
      if (!GPU) {
        GPU = getOBJ<GPU_VKOBJ>(source->m_gpu);
      } else if (GPU != getOBJ<GPU_VKOBJ>(source->m_gpu)) {
        vkPrint(37);
        return result_tgfx_FAIL;
      }
      switch (source->stage) {
        case shaderStage_tgfx_VERTEXSHADER:
          if (vkSource_vertex) {
            return vkPrint(38);
          }
          vkSource_vertex = source;
          break;
        case shaderStage_tgfx_FRAGMENTSHADER:
          if (vkSource_fragment) {
            return vkPrint(38);
          }
          vkSource_fragment = source;
          break;
        default: return vkPrint(39);
      }
    }

    VkPipelineShaderStageCreateInfo& vertexStage_ci   = STAGEs[0];
    VkPipelineShaderStageCreateInfo& fragmentStage_ci = STAGEs[1];
    vertexStage_ci.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStage_ci.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStage_ci.module = vkSource_vertex->Module;
    vertexStage_ci.pName  = "main";

    fragmentStage_ci.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStage_ci.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStage_ci.module = vkSource_fragment->Module;
    fragmentStage_ci.pName  = "main";
  }

  VkPipelineVertexInputStateCreateInfo vertexInputState                        = {};
  VkVertexInputAttributeDescription    attribs[VKCONST_MAXVERTEXATTRIBCOUNT]   = {};
  VkVertexInputBindingDescription      bindings[VKCONST_MAXVERTEXBINDINGCOUNT] = {};
  {
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    if (desc->attribLayout.attribCount > VKCONST_MAXVERTEXATTRIBCOUNT ||
        desc->attribLayout.attribCount >
          GPU->vk_propsDev.properties.limits.maxVertexInputAttributes) {
      return vkPrint(40);
    }
    if (desc->attribLayout.attribCount > VKCONST_MAXVERTEXBINDINGCOUNT ||
        desc->attribLayout.attribCount >
          GPU->vk_propsDev.properties.limits.maxVertexInputBindings) {
      return vkPrint(40);
    }

    for (uint32_t i = 0; i < desc->attribLayout.attribCount; i++) {
      const vertexAttributeDescription_tgfx& attr     = desc->attribLayout.i_attributes[i];
      uint32_t                               attrIndx = attr.attributeIndx;
      if (attrIndx >= desc->attribLayout.attribCount) {
        return vkPrint(41);
      }
      if (attr.offset > GPU->vk_propsDev.properties.limits.maxVertexInputAttributeOffset) {
        return vkPrint(42);
      }

      attribs[attrIndx].binding  = attr.bindingIndx;
      attribs[attrIndx].offset   = attr.offset;
      attribs[attrIndx].location = attrIndx;
      attribs[attrIndx].format   = vk_findDataType(attr.dataType);
    }

    for (uint32_t i = 0; i < desc->attribLayout.bindingCount; i++) {
      const vertexBindingDescription_tgfx& binding     = desc->attribLayout.i_bindings[i];
      uint32_t                             bindingIndx = binding.bindingIndx;
      if (bindingIndx >= desc->attribLayout.bindingCount) {
        return vkPrint(41);
      }
      if (binding.stride > GPU->vk_propsDev.properties.limits.maxVertexInputBindingStride) {
        return vkPrint(42);
      }

      bindings[bindingIndx].binding   = bindingIndx;
      bindings[bindingIndx].stride    = binding.stride;
      bindings[bindingIndx].inputRate = vk_findVertexInputRateVk(binding.inputRate);
    }

    vertexInputState.pVertexAttributeDescriptions    = attribs;
    vertexInputState.vertexAttributeDescriptionCount = desc->attribLayout.attribCount;
    vertexInputState.pVertexBindingDescriptions      = bindings;
    vertexInputState.vertexBindingDescriptionCount   = desc->attribLayout.bindingCount;
  }

  VkPipelineInputAssemblyStateCreateInfo IAState = {};
  {
    IAState.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    IAState.topology = Find_PrimitiveTopology_byGFXVertexListType(desc->mainStates->topology);
    IAState.primitiveRestartEnable = false;
    IAState.flags                  = 0;
    IAState.pNext                  = nullptr;
  }

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  {
    VkRect2D   vk_scissors[VKCONST_MAXVIEWPORTCOUNT];
    VkViewport vk_viewports[VKCONST_MAXVIEWPORTCOUNT];
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;
  }
  VkPipelineRasterizationStateCreateInfo rasterState = {};
  {
    rasterState.sType            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterState.polygonMode      = vk_findPolygonModeVk(desc->mainStates->polygonmode);
    rasterState.cullMode         = vk_findCullModeVk(desc->mainStates->culling);
    rasterState.frontFace        = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterState.lineWidth        = 1.0f;
    rasterState.depthClampEnable = VK_FALSE;
    rasterState.rasterizerDiscardEnable = VK_FALSE;

    rasterState.depthBiasEnable         = VK_FALSE;
    rasterState.depthBiasClamp          = 0.0f;
    rasterState.depthBiasConstantFactor = 0.0f;
    rasterState.depthBiasSlopeFactor    = 0.0f;
  }

  // MSAA isn't supported for now
  VkPipelineMultisampleStateCreateInfo MSAAState = {};
  {
    MSAAState.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MSAAState.sampleShadingEnable   = VK_FALSE;
    MSAAState.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    MSAAState.minSampleShading      = 1.0f;
    MSAAState.pSampleMask           = nullptr;
    MSAAState.alphaToCoverageEnable = VK_FALSE;
    MSAAState.alphaToOneEnable      = VK_FALSE;
  }

  // Blending isn't supported for now
  VkPipelineColorBlendStateCreateInfo blendState                                      = {};
  VkPipelineColorBlendAttachmentState states[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT] = {};
  {
    blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    while (blendState.attachmentCount <= TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT &&
           desc->colorTextureFormats[blendState.attachmentCount] != texture_channels_tgfx_UNDEF &&
           desc->colorTextureFormats[blendState.attachmentCount] != texture_channels_tgfx_UNDEF2) {
      blendState.attachmentCount++;
    }
    for (uint32_t i = 0; i < TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT; i++) {
      const blendState_tgfx& blendState = desc->mainStates->blendStates[i];
      states[i].blendEnable             = blendState.blendEnabled;
      states[i].colorWriteMask =
        vk_findColorComponentsVk(blendState.blendComponents, desc->colorTextureFormats[i]);
      states[i].alphaBlendOp        = vk_findBlendOpVk(blendState.alphaMode);
      states[i].colorBlendOp        = vk_findBlendOpVk(blendState.colorMode);
      states[i].dstAlphaBlendFactor = vk_findBlendFactorVk(blendState.dstAlphaFactor);
      states[i].dstColorBlendFactor = vk_findBlendFactorVk(blendState.dstColorFactor);
      states[i].srcAlphaBlendFactor = vk_findBlendFactorVk(blendState.srcAlphaFactor);
      states[i].srcColorBlendFactor = vk_findBlendFactorVk(blendState.srcColorFactor);
    }
    blendState.pAttachments  = states;
    blendState.logicOpEnable = VK_FALSE;
    blendState.logicOp       = VK_LOGIC_OP_COPY;
  }

  VkPipelineDynamicStateCreateInfo dynamicStates = {};
  VkDynamicState                   dynamicStatesList[64];
  uint32_t                         dynamicStatesCount = 0;
  {
    dynamicStatesList[dynamicStatesCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStatesList[dynamicStatesCount++] = VK_DYNAMIC_STATE_SCISSOR;
    dynamicStatesList[dynamicStatesCount++] = VK_DYNAMIC_STATE_DEPTH_BOUNDS;

    dynamicStates.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStates.dynamicStateCount = dynamicStatesCount;
    dynamicStates.pDynamicStates    = dynamicStatesList;
  }

  VkPipelineLayout layout = {};
  if (!VKPipelineLayoutCreation(GPU, desc->tableCount, desc->tables, false, &layout)) {
    return vkPrint(43, L"raster pipeline at VKPipelineLayoutCreation()");
  }

  VkPipelineDepthStencilStateCreateInfo depthState = {};
  {
    depthState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthState.flags = 0;
    depthState.pNext = nullptr;
    const depthStencilState_tgfx& state = desc->mainStates->depthStencilState;

    // Depth states
    {
      depthState.depthTestEnable       = state.depthTestEnabled;
      depthState.depthCompareOp        = vk_findCompareOpVk(state.depthCompare);
      depthState.depthWriteEnable      = state.depthWriteEnabled;
      depthState.depthBoundsTestEnable = state.depthTestEnabled;
      depthState.maxDepthBounds        = 1.0;
      depthState.minDepthBounds        = 0.0;
    }

    // Stencil States
    {
      depthState.stencilTestEnable = state.stencilTestEnabled;
      for (uint32_t i = 0; i < 2; i++) {
        VkStencilOpState&         vkState   = (i == 0) ? depthState.front : depthState.back;
        const tgfx_stencil_state& tgfxState = (i == 0) ? state.front : state.back;
        vkState.compareMask                 = tgfxState.compareMask;
        vkState.compareOp                   = vk_findCompareOpVk(tgfxState.compareOp);
        vkState.depthFailOp                 = vk_findStencilOpVk(tgfxState.depthFail);
        vkState.failOp                      = vk_findStencilOpVk(tgfxState.stencilFail);
        vkState.passOp                      = vk_findStencilOpVk(tgfxState.pass);
        vkState.reference                   = tgfxState.reference;
        vkState.writeMask                   = tgfxState.writeMask;
      }
    }
  }

  VkPipeline pipeline;
  {
    VkPipelineRenderingCreateInfoKHR dynCi = {};
    VkGraphicsPipelineCreateInfo     ci    = {};
    dynCi.sType                            = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    VkFormat VKCOLORATTACHMENTFORMATS[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT] = {};
    while (dynCi.colorAttachmentCount <= TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT &&
           desc->colorTextureFormats[dynCi.colorAttachmentCount] != texture_channels_tgfx_UNDEF &&
           desc->colorTextureFormats[blendState.attachmentCount] != texture_channels_tgfx_UNDEF2) {
      VKCOLORATTACHMENTFORMATS[dynCi.colorAttachmentCount] =
        vk_findFormatVk(desc->colorTextureFormats[dynCi.colorAttachmentCount]);
      dynCi.colorAttachmentCount++;
    }
    dynCi.pColorAttachmentFormats = VKCOLORATTACHMENTFORMATS;
    dynCi.viewMask                = 0;
    dynCi.pNext                   = {};

    ci.sType            = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ci.pColorBlendState = &blendState;
    if (desc->depthStencilTextureFormat != texture_channels_tgfx_UNDEF &&
        desc->depthStencilTextureFormat != texture_channels_tgfx_UNDEF2) {
      dynCi.depthAttachmentFormat   = vk_findFormatVk(desc->depthStencilTextureFormat);
      dynCi.stencilAttachmentFormat = vk_findFormatVk(desc->depthStencilTextureFormat);
    } else {
      ci.pDepthStencilState = nullptr;
    }
    ci.pDynamicState       = &dynamicStates;
    ci.pInputAssemblyState = &IAState;
    ci.pMultisampleState   = &MSAAState;
    ci.pRasterizationState = &rasterState;
    ci.pVertexInputState   = &vertexInputState;
    ci.pViewportState      = &viewportState;
    ci.pDepthStencilState  = &depthState;
    ci.layout              = layout;
    ci.stageCount          = 2;
    ci.pStages             = STAGEs;
    ci.flags               = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    ci.pNext               = &dynCi;
    vk_fillRasterPipelineStateInfo(GPU, &ci, desc, 0, nullptr);
    if (vkCreateGraphicsPipelines(GPU->vk_logical, nullptr, 1, &ci, nullptr, &pipeline) !=
        VK_SUCCESS) {
      vkPrint(43, L"at vkCreateGraphicsPipelines()");
    }
  }

  PIPELINE_VKOBJ* pipelineObj = hidden->pipelines.create_OBJ();
  pipelineObj->m_gpu          = GPU->gpuIndx();
  pipelineObj->vk_layout      = layout;
  pipelineObj->vk_object      = pipeline;
  pipelineObj->vk_type        = VK_PIPELINE_BIND_POINT_GRAPHICS;
  for (uint32_t i = 0; i < TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT; i++) {
    pipelineObj->vk_colorAttachmentFormats[i] = vk_findFormatVk(desc->colorTextureFormats[i]);
  }
  pipelineObj->vk_depthAttachmentFormat = vk_findFormatVk(desc->depthStencilTextureFormat);
  *hnd                                  = getHANDLE<pipeline_tgfxhnd>(pipelineObj);
  return result_tgfx_SUCCESS;
}
/*
result_tgfx vk_copyRasterPipeline(rasterPipeline_tgfxhnd src, extension_tgfxlsthnd exts,
                                  rasterPipeline_tgfxhnd* dst) {
  RASTERPIPELINE_VKOBJ* srcPipeline = getOBJ<PIPELINE_VKOBJ>(src);
#ifdef VULKAN_DEBUGGING
  if (!srcPipeline) {
    printer(result_tgfx_FAIL,
            "vk_copyRasterPipeline() has failed because source raster pipeline isn't found!");
    return result_tgfx_INVALIDARGUMENT;
  }
#endif

  // Descriptor Set Creation
  RASTERPIPELINE_VKOBJ* dstPipeline = hidden->pipelines.create_OBJ();

  *dst = hidden->pipelines.returnHANDLEfromOBJ(dstPipeline);
  return result_tgfx_SUCCESS;
}
void vk_destroyRasterPipeline(rasterPipeline_tgfxhnd hnd) {
  printer(result_tgfx_NOTCODED, "Delete_MaterialType() isn't implemented!");
}
*/
result_tgfx vk_createComputePipeline(
  shaderSource_tgfxhnd Source, unsigned int bindingTableCount,
  const bindingTableDescription_tgfx const* const bindingTableDescs,
  unsigned char isCallBufferSupported, pipeline_tgfxhnd* hnd) {
  VkComputePipelineCreateInfo cp_ci  = {};
  SHADERSOURCE_VKOBJ*         SHADER = getOBJ<SHADERSOURCE_VKOBJ>(Source);
  GPU_VKOBJ*                  GPU    = getOBJ<GPU_VKOBJ>(SHADER->m_gpu);

  VkPipelineLayout layout = {};
  if (!VKPipelineLayoutCreation(GPU, bindingTableCount, bindingTableDescs, isCallBufferSupported,
                                &layout)) {
    return vkPrint(43, L"compute pipeline at VKPipelineLayoutCreation()!");
  }

  // VkPipeline creation
  VkPipeline pipelineObj = {};
  {
    cp_ci.stage.flags               = 0;
    cp_ci.stage.module              = SHADER->Module;
    cp_ci.stage.pName               = "main";
    cp_ci.stage.pNext               = nullptr;
    cp_ci.stage.pSpecializationInfo = nullptr;
    cp_ci.stage.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
    cp_ci.stage.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    cp_ci.basePipelineHandle        = VK_NULL_HANDLE;
    cp_ci.basePipelineIndex         = -1;
    cp_ci.flags                     = 0;
    cp_ci.layout                    = layout;
    cp_ci.pNext                     = nullptr;
    cp_ci.sType                     = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    if (vkCreateComputePipelines(GPU->vk_logical, VK_NULL_HANDLE, 1, &cp_ci, nullptr,
                                 &pipelineObj) != VK_SUCCESS) {
      return vkPrint(43, L"at vkCreateComputePipelines()");
    }
  }

  PIPELINE_VKOBJ* obj = hidden->pipelines.create_OBJ();
  obj->m_gpu          = GPU->gpuIndx();
  obj->vk_layout      = layout;
  obj->vk_object      = pipelineObj;
  obj->vk_type        = VK_PIPELINE_BIND_POINT_COMPUTE;
  *hnd                = getHANDLE<pipeline_tgfxhnd>(obj);
  return result_tgfx_SUCCESS;
}

result_tgfx vk_copyComputePipeline(pipeline_tgfxhnd src, const extension_tgfxhnd* exts,
                                   pipeline_tgfxhnd* dst) {
  return result_tgfx_NOTCODED;
}
void vk_destroyPipeline(pipeline_tgfxhnd pipe) {
  PIPELINE_VKOBJ* vkPipe = getOBJ<PIPELINE_VKOBJ>(pipe);
  assert(vkPipe && "Invalid pipeline!");
  vkDestroyPipelineLayout(core_vk->getGPU(vkPipe->m_gpu)->vk_logical, vkPipe->vk_layout, nullptr);
  vkDestroyPipeline(core_vk->getGPU(vkPipe->m_gpu)->vk_logical, vkPipe->vk_object, nullptr);
  hidden->pipelines.destroyObj(hidden->pipelines.getINDEXbyOBJ(vkPipe));
}

static constexpr uint32_t VKCONST_MAXDESCCHANGE_PERCALL = 1024;
static constexpr wchar_t* VKCONST_MAXDESCCHANGE_TEXT =
  L"Exceeded max number of changable binding, please report this";
// Set a descriptor of the binding table created with shaderdescriptortype_tgfx_SAMPLER
result_tgfx vk_setBindingTable_Sampler(bindingTable_tgfxhnd bindingtable, unsigned int bindingCount,
                                       const unsigned int*    bindingIndices,
                                       const sampler_tgfxhnd* samplerHandles) {
  BINDINGTABLEINST_VKOBJ*   set      = getOBJ<BINDINGTABLEINST_VKOBJ>(bindingtable);
  static constexpr wchar_t* funcName = L"at setBindingTable_sampler()";
  if (!set) {
    return vkPrint(11, funcName);
  }
  if (bindingCount > VKCONST_MAXDESCCHANGE_PERCALL) {
    return vkPrint(16, VKCONST_MAXDESCCHANGE_TEXT);
  }

  VkWriteDescriptorSet  writeInfos[VKCONST_MAXDESCCHANGE_PERCALL] = {};
  VkDescriptorImageInfo imageInfos[VKCONST_MAXDESCCHANGE_PERCALL] = {};
  for (uint32_t bindingIter = 0; bindingIter < bindingCount; bindingIter++) {
    SAMPLER_VKOBJ* sampler     = getOBJ<SAMPLER_VKOBJ>(samplerHandles[bindingIter]);
    uint32_t       elementIndx = bindingIndices[bindingIter];
    if (!set || !sampler || bindingIndices[bindingIter] >= set->m_elementCount ||
        set->vk_descType != VK_DESCRIPTOR_TYPE_SAMPLER) {
      return vkPrint(11, funcName);
    }
    sampler_descVK& samplerDESC = (( sampler_descVK* )set->m_descs)[elementIndx];

    samplerDESC.sampler_obj         = sampler->vk_sampler;
    imageInfos[bindingIter].sampler = sampler->vk_sampler;

    writeInfos[bindingIter].descriptorCount  = 1;
    writeInfos[bindingIter].descriptorType   = set->vk_descType;
    writeInfos[bindingIter].dstArrayElement  = elementIndx;
    writeInfos[bindingIter].dstBinding       = 0;
    writeInfos[bindingIter].pTexelBufferView = nullptr;
    writeInfos[bindingIter].pNext            = nullptr;
    writeInfos[bindingIter].dstSet           = set->vk_set;
    writeInfos[bindingIter].pImageInfo       = &imageInfos[bindingIter];
    writeInfos[bindingIter].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  }
  vkUpdateDescriptorSets(core_vk->getGPU(set->m_gpu)->vk_logical, bindingCount, writeInfos, 0,
                         nullptr);
  return result_tgfx_SUCCESS;
}

// Set a descriptor of the binding table created with shaderdescriptortype_tgfx_BUFFER
result_tgfx vk_setBindingTable_Buffer(bindingTable_tgfxhnd table, unsigned int bindingCount,
                                      const unsigned int*   bindingIndices,
                                      const buffer_tgfxhnd* buffers, const unsigned int* offsets,
                                      const unsigned int* sizes, unsigned int extCount,
                                      const extension_tgfxhnd* exts) {
  BINDINGTABLEINST_VKOBJ*   set      = getOBJ<BINDINGTABLEINST_VKOBJ>(table);
  static constexpr wchar_t* funcName = L"at setBindingTable_buffer()";
  if (!set) {
    return vkPrint(11, funcName);
  }
  if (set->vk_descType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
      set->vk_descType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
    return vkPrint(11, funcName);
  }
  if (bindingCount > VKCONST_MAXDESCCHANGE_PERCALL) {
    return vkPrint(16, VKCONST_MAXDESCCHANGE_TEXT);
  }

  VkWriteDescriptorSet writeInfos[VKCONST_MAXDESCCHANGE_PERCALL] = {};
  for (uint32_t bindingIter = 0; bindingIter < bindingCount; bindingIter++) {
    BUFFER_VKOBJ*  buffer      = getOBJ<BUFFER_VKOBJ>(buffers[bindingIter]);
    uint32_t       elementIndx = bindingIndices[bindingIter];
    buffer_descVK& bufferDESC  = (( buffer_descVK* )set->m_descs)[elementIndx];

    if (!buffer || elementIndx >= set->m_elementCount) {
      return vkPrint(11, funcName);
    }

    bufferDESC.info.buffer = buffer->vk_buffer;
    bufferDESC.info.offset = (offsets == nullptr) ? (0) : (offsets[bindingIter]);
    bufferDESC.info.range  = (sizes == nullptr) ? (buffer->m_intendedSize) : (sizes[bindingIter]);

    writeInfos[bindingIter].descriptorCount  = 1;
    writeInfos[bindingIter].descriptorType   = set->vk_descType;
    writeInfos[bindingIter].dstArrayElement  = elementIndx;
    writeInfos[bindingIter].dstBinding       = 0;
    writeInfos[bindingIter].pTexelBufferView = nullptr;
    writeInfos[bindingIter].pNext            = nullptr;
    writeInfos[bindingIter].dstSet           = set->vk_set;
    writeInfos[bindingIter].pBufferInfo      = &bufferDESC.info;
    writeInfos[bindingIter].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  }
  uint64_t duration = 0;
  vkUpdateDescriptorSets(core_vk->getGPU(set->m_gpu)->vk_logical, bindingCount, writeInfos, 0,
                         nullptr);

  return result_tgfx_SUCCESS;
}
result_tgfx vk_setBindingTable_Texture(bindingTable_tgfxhnd table, unsigned int bindingCount,
                                       const unsigned int*    bindingIndices,
                                       const texture_tgfxhnd* textures) {
  BINDINGTABLEINST_VKOBJ*   set      = getOBJ<BINDINGTABLEINST_VKOBJ>(table);
  static constexpr wchar_t* funcName = L"at setBindingTable_texture()";
  if (!set) {
    return vkPrint(11, funcName);
  }
  if (set->vk_descType != VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE &&
      set->vk_descType != VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
    return vkPrint(11, funcName);
  }
  if (bindingCount > VKCONST_MAXDESCCHANGE_PERCALL) {
    return vkPrint(16, VKCONST_MAXDESCCHANGE_TEXT);
  }
  VkWriteDescriptorSet writeInfos[VKCONST_MAXDESCCHANGE_PERCALL] = {};
  for (uint32_t bindingIter = 0; bindingIter < bindingCount; bindingIter++) {
    TEXTURE_VKOBJ*  texture     = getOBJ<TEXTURE_VKOBJ>(textures[bindingIter]);
    uint32_t        elementIndx = bindingIndices[bindingIter];
    texture_descVK& textureDESC = (( texture_descVK* )set->m_descs)[elementIndx];
    if (!texture || elementIndx >= set->m_elementCount) {
      return vkPrint(21, funcName);
    }
    textureDESC.info.imageLayout = (set->vk_descType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                                     ? VK_IMAGE_LAYOUT_GENERAL
                                     : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    textureDESC.info.imageView   = texture->vk_imageView;
    textureDESC.info.sampler     = VK_NULL_HANDLE;

    writeInfos[bindingIter].descriptorCount  = 1;
    writeInfos[bindingIter].descriptorType   = set->vk_descType;
    writeInfos[bindingIter].dstArrayElement  = elementIndx;
    writeInfos[bindingIter].dstBinding       = 0;
    writeInfos[bindingIter].pTexelBufferView = nullptr;
    writeInfos[bindingIter].pNext            = nullptr;
    writeInfos[bindingIter].dstSet           = set->vk_set;
    writeInfos[bindingIter].pImageInfo       = &textureDESC.info;
    writeInfos[bindingIter].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  }
  vkUpdateDescriptorSets(core_vk->getGPU(set->m_gpu)->vk_logical, bindingCount, writeInfos, 0,
                         nullptr);
  return result_tgfx_SUCCESS;
}

/////////////////////////////////////////////////////
//								MEMORY
/////////////////////////////////////////////////////

static constexpr wchar_t* VKCONST_HEAP_EXTENSIONS_NOT_SUPPORTED_TEXT =
  L"Extensions're not supported in heap functions for now";
result_tgfx vk_createHeap(gpu_tgfxhnd gpu, unsigned char memoryRegionID,
                          unsigned long long heapSize, unsigned int extCount,
                          const extension_tgfxhnd* exts, heap_tgfxhnd* heapHnd) {
  if (exts) {
    return vkPrint(16, VKCONST_HEAP_EXTENSIONS_NOT_SUPPORTED_TEXT);
  }
  VkMemoryAllocateInfo memAlloc;
  memAlloc.allocationSize  = heapSize;
  memAlloc.memoryTypeIndex = memoryRegionID;
  memAlloc.pNext           = nullptr;
  memAlloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  GPU_VKOBJ*     vkGPU     = getOBJ<GPU_VKOBJ>(gpu);
  VkDeviceMemory vkDevMem;
  if (vkAllocateMemory(vkGPU->vk_logical, &memAlloc, nullptr, &vkDevMem) != VK_SUCCESS) {
    return vkPrint(44, L"at vkAllocateMemory()");
  }

  HEAP_VKOBJ* heap         = hidden->heaps.create_OBJ();
  heap->vk_memoryHandle    = vkDevMem;
  heap->vk_memoryTypeIndex = memoryRegionID;
  heap->m_size             = heapSize;
  heap->m_GPU              = vkGPU->gpuIndx();
  *heapHnd                 = getHANDLE<heap_tgfxhnd>(heap);
  return result_tgfx_SUCCESS;
}
result_tgfx vk_getHeapRequirement_Texture(texture_tgfxhnd i_texture, unsigned int extCount,
                                          const extension_tgfxhnd*   exts,
                                          heapRequirementsInfo_tgfx* reqs) {
  if (exts) {
    return vkPrint(16, VKCONST_HEAP_EXTENSIONS_NOT_SUPPORTED_TEXT);
  }
  TEXTURE_VKOBJ* texture  = getOBJ<TEXTURE_VKOBJ>(i_texture);
  uint8_t        listSize = 0;
  for (uint8_t memTypeIndx = 0; memTypeIndx < 32; memTypeIndx++) {
    if (texture->m_memReqs.vk_memReqs.memoryTypeBits & (1u << memTypeIndx)) {
      reqs->memoryRegionIDs[listSize++] = memTypeIndx;
    }
  }
  reqs->memoryRegionIDs[listSize] = 255;
  reqs->offsetAlignment           = texture->m_memReqs.vk_memReqs.alignment;
  reqs->size                      = texture->m_memReqs.vk_memReqs.size;
  return result_tgfx_SUCCESS;
}
result_tgfx vk_getHeapRequirement_Buffer(buffer_tgfxhnd i_buffer, unsigned int extCount,
                                         const extension_tgfxhnd*   exts,
                                         heapRequirementsInfo_tgfx* reqs) {
  if (exts) {
    return vkPrint(16, VKCONST_HEAP_EXTENSIONS_NOT_SUPPORTED_TEXT);
  }
  BUFFER_VKOBJ* buf      = getOBJ<BUFFER_VKOBJ>(i_buffer);
  uint8_t       listSize = 0;
  for (uint8_t memTypeIndx = 0; memTypeIndx < 32; memTypeIndx++) {
    if (buf->m_memReqs.vk_memReqs.memoryTypeBits & (1u << memTypeIndx)) {
      reqs->memoryRegionIDs[listSize++] = memTypeIndx;
    }
  }
  reqs->memoryRegionIDs[listSize] = 255;
  reqs->offsetAlignment           = buf->m_memReqs.vk_memReqs.alignment;
  reqs->size                      = buf->m_memReqs.vk_memReqs.size;
  return result_tgfx_SUCCESS;
}
// @return FAIL if this feature isn't supported
result_tgfx vk_getRemainingMemory(gpu_tgfxhnd GPU, unsigned char memoryRegionID,
                                  unsigned int extCount, const extension_tgfxhnd* exts,
                                  unsigned long long* size) {
  return result_tgfx_NOTCODED;
}

result_tgfx vk_bindToHeap_Buffer(heap_tgfxhnd i_heap, unsigned long long offset,
                                 buffer_tgfxhnd i_buffer, unsigned int extCount,
                                 const extension_tgfxhnd* exts) {
  if (exts) {
    return vkPrint(16, VKCONST_HEAP_EXTENSIONS_NOT_SUPPORTED_TEXT);
  }
  BUFFER_VKOBJ* buffer = getOBJ<BUFFER_VKOBJ>(i_buffer);
  HEAP_VKOBJ*   heap   = getOBJ<HEAP_VKOBJ>(i_heap);
  GPU_VKOBJ*    gpu    = core_vk->getGPU(buffer->m_GPU);
  if (buffer->m_memReqs.requiresDedicatedAlloc) {
    return vkPrint(45);
  }
  if (offset % buffer->m_memReqs.vk_memReqs.alignment) {
    return vkPrint(46);
  }
  if (vkBindBufferMemory(gpu->vk_logical, buffer->vk_buffer, heap->vk_memoryHandle, offset) !=
      VK_SUCCESS) {
    return vkPrint(47, L"at vkBindBufferMemory()");
  }
  return result_tgfx_SUCCESS;
}
result_tgfx vk_bindToHeap_Texture(heap_tgfxhnd i_heap, unsigned long long offset,
                                  texture_tgfxhnd i_texture, unsigned int extCount,
                                  const extension_tgfxhnd* exts) {
  TEXTURE_VKOBJ* texture = getOBJ<TEXTURE_VKOBJ>(i_texture);
  HEAP_VKOBJ*    heap    = getOBJ<HEAP_VKOBJ>(i_heap);
  GPU_VKOBJ*     gpu     = core_vk->getGPU(texture->m_GPU);
  if (texture->m_memReqs.requiresDedicatedAlloc) {
    return vkPrint(45);
  }
  if (offset % texture->m_memReqs.vk_memReqs.alignment) {
    return vkPrint(46);
  }
  if (vkBindImageMemory(gpu->vk_logical, texture->vk_image, heap->vk_memoryHandle, offset) !=
      VK_SUCCESS) {
    vkPrint(47, L"at vkBindImageMemory()");
  }

  if (texture->vk_imageView) {
    vkDestroyImageView(gpu->vk_logical, texture->vk_imageView, nullptr);
  }

  // Create VkImageView
  VkImageView VkTextureViewObj;
  {
    VkImageViewCreateInfo ci = {};
    ci.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ci.flags                 = 0;
    ci.pNext                 = nullptr;

    ci.image = texture->vk_image;
    if (texture->m_dim == texture_dimensions_tgfx_2DCUBE) {
      ci.viewType                    = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
      ci.subresourceRange.layerCount = 6;
    } else {
      ci.viewType                    = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
      ci.subresourceRange.layerCount = 1;
    }
    ci.subresourceRange.baseArrayLayer = 0;
    ci.subresourceRange.baseMipLevel   = 0;
    ci.subresourceRange.levelCount     = 1;
    ci.format                          = vk_findFormatVk(texture->m_channels);
    if (texture->m_channels == texture_channels_tgfx_D32) {
      ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else if (texture->m_channels == texture_channels_tgfx_D24S8) {
      ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    } else {
      ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    Fill_ComponentMapping_byCHANNELs(texture->m_channels, ci.components);

    if (vkCreateImageView(gpu->vk_logical, &ci, nullptr, &texture->vk_imageView) != VK_SUCCESS) {
      return vkPrint(47, L"at vkCreateImageView()");
    }
  }

  return result_tgfx_SUCCESS;
}

result_tgfx vk_mapHeap(heap_tgfxhnd i_heap, unsigned long long offset, unsigned long long size,
                       unsigned int extCount, const extension_tgfxhnd* exts, void** mappedRegion) {
  HEAP_VKOBJ* heap = getOBJ<HEAP_VKOBJ>(i_heap);
  GPU_VKOBJ*  gpu  = core_vk->getGPU(heap->m_GPU);
  if (vkMapMemory(gpu->vk_logical, heap->vk_memoryHandle, offset, size, 0, mappedRegion) !=
      VK_SUCCESS) {
    return vkPrint(48);
  }
  return result_tgfx_SUCCESS;
}

result_tgfx vk_unmapHeap(heap_tgfxhnd i_heap) {
  HEAP_VKOBJ* heap = getOBJ<HEAP_VKOBJ>(i_heap);
  GPU_VKOBJ*  gpu  = core_vk->getGPU(heap->m_GPU);
  vkUnmapMemory(gpu->vk_logical, heap->vk_memoryHandle);
  return result_tgfx_SUCCESS;
}

/////////////////////////////////////////////////////
///				INITIALIZATION PROCEDURE
/////////////////////////////////////////////////////

inline void set_functionpointers() {
  core_tgfx_main->contentmanager->compileShaderSource   = vk_compileShaderSource;
  core_tgfx_main->contentmanager->createBindingTable    = vk_createBindingTable;
  core_tgfx_main->contentmanager->copyComputePipeline   = vk_copyComputePipeline;
  core_tgfx_main->contentmanager->createComputePipeline = vk_createComputePipeline;
  core_tgfx_main->contentmanager->createBuffer          = vk_createBuffer;
  core_tgfx_main->contentmanager->createTexture         = vk_createTexture;
  // core_tgfx_main->contentmanager->destroyRasterPipeline      = vk_destroyRasterPipeline;
  core_tgfx_main->contentmanager->destroyShaderSource        = vk_destroyShaderSource;
  core_tgfx_main->contentmanager->destroyAllResources        = vk_destroyAllResources;
  core_tgfx_main->contentmanager->createRasterPipeline       = vk_createRasterPipeline;
  core_tgfx_main->contentmanager->setBindingTable_Buffer     = vk_setBindingTable_Buffer;
  core_tgfx_main->contentmanager->setBindingTable_Texture    = vk_setBindingTable_Texture;
  core_tgfx_main->contentmanager->createHeap                 = vk_createHeap;
  core_tgfx_main->contentmanager->getHeapRequirement_Buffer  = vk_getHeapRequirement_Buffer;
  core_tgfx_main->contentmanager->getHeapRequirement_Texture = vk_getHeapRequirement_Texture;
  core_tgfx_main->contentmanager->getRemainingMemory         = vk_getRemainingMemory;
  core_tgfx_main->contentmanager->bindToHeap_Buffer          = vk_bindToHeap_Buffer;
  core_tgfx_main->contentmanager->bindToHeap_Texture         = vk_bindToHeap_Texture;
  core_tgfx_main->contentmanager->mapHeap                    = vk_mapHeap;
  core_tgfx_main->contentmanager->unmapHeap                  = vk_unmapHeap;
  core_tgfx_main->contentmanager->createSampler              = vk_createSampler;
  core_tgfx_main->contentmanager->setBindingTable_Sampler    = vk_setBindingTable_Sampler;
  core_tgfx_main->contentmanager->destroyBuffer              = vk_destroyBuffer;
  core_tgfx_main->contentmanager->destroyTexture             = vk_destroyTexture;
  core_tgfx_main->contentmanager->destroyShaderSource        = vk_destroyShaderSource;
  core_tgfx_main->contentmanager->destroySampler             = vk_destroySampler;
  core_tgfx_main->contentmanager->destroyBindingTable        = vk_destroyBindingTable;
  core_tgfx_main->contentmanager->destroyPipeline            = vk_destroyPipeline;
}

void initGlslang() {
  // Load dll and func pointers
  auto* dllHandle          = DLIB_LOAD_TAPI("TGFXVulkanGlslang.dll");
  void (*initGlslangFnc)() = ( void (*)() )DLIB_FUNC_LOAD_TAPI(dllHandle, "startGlslang");
  VKCONST_GLSLANG_COMPILE_FNC =
    ( vk_glslangCompileFnc )DLIB_FUNC_LOAD_TAPI(dllHandle, "glslangCompile");
  // Init glslang
  initGlslangFnc();
}
void vk_createContentManager() {
  VKGLOBAL_VIRMEM_CONTENTMANAGER = vk_virmem::allocate_dynamicmem(
    sizeof(gpudatamanager_public) + sizeof(gpudatamanager_private) + (10ull << 20));
  contentManager         = new (VKGLOBAL_VIRMEM_CONTENTMANAGER) gpudatamanager_public;
  contentManager->hidden = new (VKGLOBAL_VIRMEM_CONTENTMANAGER) gpudatamanager_private;
  hidden                 = contentManager->hidden;

  set_functionpointers();

  initGlslang();
}

// Helper funcs

VK_LINEAR_OBJARRAY<TEXTURE_VKOBJ, texture_tgfxhnd, 1 << 24>&
gpudatamanager_public::GETTEXTURES_ARRAY() {
  return hidden->textures;
}
VK_LINEAR_OBJARRAY<PIPELINE_VKOBJ, pipeline_tgfxhnd, 1 << 24>&
gpudatamanager_public::GETPIPELINE_ARRAY() {
  return hidden->pipelines;
}
VK_LINEAR_OBJARRAY<BUFFER_VKOBJ, buffer_tgfxhnd>& gpudatamanager_public::GETBUFFER_ARRAY() {
  return hidden->buffers;
}
VK_LINEAR_OBJARRAY<BINDINGTABLEINST_VKOBJ, bindingTable_tgfxhnd, 1 << 16>&
gpudatamanager_public::GETBINDINGTABLE_ARRAY() {
  return hidden->bindingtableinsts;
}

unsigned int vk_calculateSizeOfVertexLayout(const datatype_tgfx* ATTRIBUTEs, unsigned int count) {
  unsigned int size = 0;
  for (unsigned int i = 0; i < count; i++) {
    size += vk_getDataTypeByteSizes(ATTRIBUTEs[i]);
  }
  return size;
}