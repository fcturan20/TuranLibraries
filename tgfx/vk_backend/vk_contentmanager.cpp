#define T_INCLUDE_PLATFORM_LIBS
#include "vk_contentmanager.h"

#include <glslang/SPIRV/GlslangToSpv.h>
#include <tgfx_core.h>
#include <tgfx_gpucontentmanager.h>

#include <mutex>

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
  VkSampler         sampler_obj = defaultSampler;
  std::atomic_uchar isUpdated   = 0;
};

struct buffer_descVK { // Both for BUFFER and EXT_UNIFORMBUFFER
  VkDescriptorBufferInfo info      = {};
  std::atomic_uchar      isUpdated = 0;
};

struct texture_descVK { // Both for SAMPLEDTEXTURE and STORAGEIMAGE
  VkDescriptorImageInfo info      = {};
  std::atomic_uchar     isUpdated = 0;
};

struct SHADERSOURCE_VKOBJ {
  std::atomic_bool               isALIVE    = false;
  static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::SHADERSOURCE;
  static uint16_t                GET_EXTRAFLAGS(SHADERSOURCE_VKOBJ* obj) { return obj->stage; }
  void*                          operator new(size_t size) = delete;

  void operator=(const SHADERSOURCE_VKOBJ& copyFrom) {
    isALIVE.store(true);
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
  VK_LINEAR_OBJARRAY<TEXTURE_VKOBJ, texture_tgfxhnd, 1 << 24>   textures;
  VK_LINEAR_OBJARRAY<PIPELINE_VKOBJ, pipeline_tgfxhnd, 1 << 24> pipelines;
  VK_LINEAR_OBJARRAY<VERTEXATTRIBLAYOUT_VKOBJ, vertexAttributeLayout_tgfxhnd, 1 << 10>
                                                                        vertexattributelayouts;
  VK_LINEAR_OBJARRAY<SHADERSOURCE_VKOBJ, shaderSource_tgfxhnd, 1 << 24> shadersources;
  VK_LINEAR_OBJARRAY<SAMPLER_VKOBJ, sampler_tgfxhnd, 1 << 16>           samplers;
  VK_LINEAR_OBJARRAY<BUFFER_VKOBJ, buffer_tgfxhnd>                      buffers;
  VK_LINEAR_OBJARRAY<BINDINGTABLETYPE_VKOBJ, bindingTableType_tgfxhnd, 1 << 10> bindingtabletypes;
  VK_LINEAR_OBJARRAY<BINDINGTABLEINST_VKOBJ, bindingTable_tgfxhnd, 1 << 16>     bindingtableinsts;
  VK_LINEAR_OBJARRAY<HEAP_VKOBJ, heap_tgfxhnd, 1 << 10>                         heaps;
  VK_LINEAR_OBJARRAY<SUBRASTERPASS_VKOBJ, subRasterpass_tgfxhnd, 1 << 16>       subrasterpasses;

  // These are the textures that will be deleted after waiting for 2 frames ago's command buffer
  VK_VECTOR_ADDONLY<TEXTURE_VKOBJ*, 1 << 16> DeleteTextureList;
  // These are the texture that will be added to the list above after clearing the above list
  VK_VECTOR_ADDONLY<TEXTURE_VKOBJ*, 1 << 16> NextFrameDeleteTextureCalls;

  gpudatamanager_private() {}
};
static gpudatamanager_private* hidden = nullptr;

void vk_destroyAllResources() {}

result_tgfx vk_createSampler(gpu_tgfxhnd gpu, const samplerDescription_tgfx* desc,
                             sampler_tgfxhnd* hnd) {
  GPU_VKOBJ* GPU = core_vk->getGPUs().getOBJfromHANDLE(gpu);

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
    THROW_RETURN_IF_FAIL(vkCreateSampler(GPU->vk_logical, &s_ci, nullptr, &sampler),
                         "GFXContentManager->Create_SamplingType() has failed at vkCreateSampler!",
                         result_tgfx_FAIL);
  }

  SAMPLER_VKOBJ* SAMPLER = hidden->samplers.create_OBJ();
  SAMPLER->vk_sampler    = sampler;
  *hnd                   = hidden->samplers.returnHANDLEfromOBJ(SAMPLER);
  return result_tgfx_SUCCESS;
}

/*Attributes are ordered as the same order of input vector
 * For example: Same attribute ID may have different location/order in another attribute layout
 * So you should gather your vertex buffer data according to that
 */

unsigned int vk_calculateSizeOfVertexLayout(const datatype_tgfx* ATTRIBUTEs, unsigned int count);
result_tgfx  vk_createVertexAttribLayout(const datatype_tgfx*           Attributes,
                                         vertexAttributeLayout_tgfxhnd* hnd) {
   VERTEXATTRIBLAYOUT_VKOBJ* lay             = hidden->vertexattributelayouts.create_OBJ();
   unsigned int              AttributesCount = 0;
   while (Attributes[AttributesCount] != datatype_tgfx_UNDEFINED) {
     AttributesCount++;
  }
   lay->AttribCount = AttributesCount;
   lay->Attribs     = new datatype_tgfx[lay->AttribCount];
   for (unsigned int i = 0; i < lay->AttribCount; i++) {
     lay->Attribs[i] = Attributes[i];
  }
   unsigned int size_pervertex = vk_calculateSizeOfVertexLayout(lay->Attribs, lay->AttribCount);
   lay->size_perVertex         = size_pervertex;
   lay->BindingDesc.binding    = 0;
   lay->BindingDesc.stride     = size_pervertex;
   lay->BindingDesc.inputRate  = VK_VERTEX_INPUT_RATE_VERTEX;

   lay->AttribDescs                       = new VkVertexInputAttributeDescription[lay->AttribCount];
   lay->AttribDesc_Count                  = lay->AttribCount;
   unsigned int stride_ofcurrentattribute = 0;
   for (unsigned int i = 0; i < lay->AttribCount; i++) {
     lay->AttribDescs[i].binding  = 0;
     lay->AttribDescs[i].location = i;
     lay->AttribDescs[i].offset   = stride_ofcurrentattribute;
     lay->AttribDescs[i].format   = Find_VkFormat_byDataType(lay->Attribs[i]);
     stride_ofcurrentattribute += vk_getDataTypeByteSizes(lay->Attribs[i]);
  }

   *hnd = hidden->vertexattributelayouts.returnHANDLEfromOBJ(lay);
   return result_tgfx_SUCCESS;
}
void vk_destroyVertexAttribLayout(vertexAttributeLayout_tgfxhnd VertexAttributeLayoutHandle) {}

result_tgfx vk_createTexture(gpu_tgfxhnd i_gpu, const textureDescription_tgfx* desc,
                             texture_tgfxhnd* TextureHandle) {
  GPU_VKOBJ*        gpu       = core_vk->getGPUs().getOBJfromHANDLE(i_gpu);
  VkImageUsageFlags usageFlag = vk_findTextureUsageFlagVk(desc->usage);
  if (desc->channelType == texture_channels_tgfx_D24S8 ||
      desc->channelType == texture_channels_tgfx_D32) {
    usageFlag &= ~(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
  } else {
    usageFlag &= ~(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
  }

  if (desc->mipCount > std::floor(std::log2(std::max(desc->width, desc->height))) + 1 ||
      !desc->mipCount) {
    printer(result_tgfx_FAIL,
            "GFXContentManager->Create_Texture() has failed "
            "because mip count of the texture is wrong!");
    return result_tgfx_FAIL;
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
    VK_getQueueAndSharingInfos(desc->permittedQueues, nullptr, queueFamIndexList,
                               &im_ci.queueFamilyIndexCount, &im_ci.sharingMode);
    im_ci.pQueueFamilyIndices = queueFamIndexList;

    im_ci.tiling  = Find_VkTiling(desc->dataOrder);
    im_ci.usage   = usageFlag;
    im_ci.samples = VK_SAMPLE_COUNT_1_BIT;

    ThrowIfFailed(vkCreateImage(gpu->vk_logical, &im_ci, nullptr, &vkTextureObj),
                  "GFXContentManager->Create_Texture() has failed in vkCreateImage()!");
  }

  memoryRequirements_vk memReqs = memoryRequirements_vk::GETINVALID();
  vkGetImageMemoryRequirements(gpu->vk_logical, vkTextureObj, &memReqs.vk_memReqs);
  if (memReqs.requiresDedicatedAlloc) {
    printer(result_tgfx_FAIL,
            "Created texture requires a dedicated allocation but you didn't use dedicated "
            "allocation extension. Provide a dedicated allocation info as extension!");
    printer(result_tgfx_NOTCODED,
            "Dedicated allocation extension isn't coded yet, so this resource creation's gonna "
            "fail until the implementation.");
    vkDestroyImage(gpu->vk_logical, vkTextureObj, nullptr);
    return result_tgfx_FAIL;
  }

  TEXTURE_VKOBJ* texture = contentmanager->GETTEXTURES_ARRAY().create_OBJ();
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

  *TextureHandle = contentmanager->GETTEXTURES_ARRAY().returnHANDLEfromOBJ(texture);
  return result_tgfx_SUCCESS;
}

result_tgfx vk_createBuffer(gpu_tgfxhnd i_gpu, const bufferDescription_tgfx* desc,
                            buffer_tgfxhnd* buffer) {
  GPU_VKOBJ* gpu = core_vk->getGPUs().getOBJfromHANDLE(i_gpu);

  // Create VkBuffer object
  VkBuffer           vkBufObj;
  VkBufferCreateInfo ci = {};
  {
    ci.usage = vk_findBufferUsageFlagVk(desc->usageFlag);

    uint32_t queueFamIndexList[VKCONST_MAXQUEUEFAMCOUNT_PERGPU];
    VK_getQueueAndSharingInfos(desc->permittedQueues, nullptr, queueFamIndexList,
                               &ci.queueFamilyIndexCount, &ci.sharingMode);
    ci.pQueueFamilyIndices = queueFamIndexList;
    ci.sType               = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size                = desc->dataSize;

    ThrowIfFailed(vkCreateBuffer(gpu->vk_logical, &ci, nullptr, &vkBufObj),
                  "vkCreateBuffer() has failed!");
  }
  memoryRequirements_vk memReqs;
  vkGetBufferMemoryRequirements(gpu->vk_logical, vkBufObj, &memReqs.vk_memReqs);
  if (memReqs.requiresDedicatedAlloc) {
    printer(result_tgfx_FAIL,
            "Created buffer requires a dedicated allocation but you didn't use dedicated "
            "allocation extension. Provide a dedicated allocation info as extension!");
    printer(result_tgfx_NOTCODED,
            "Dedicated allocation extension isn't coded yet, so this resource creation's gonna "
            "fail until the implementation.");
    vkDestroyBuffer(gpu->vk_logical, vkBufObj, nullptr);
    return result_tgfx_FAIL;
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

  *buffer = hidden->buffers.returnHANDLEfromOBJ(o_buffer);
  return result_tgfx_SUCCESS;
}

vk_uint32c  VKCONST_MAXSTATICSAMPLERCOUNT = 128;
result_tgfx vk_createBindingTableType(gpu_tgfxhnd gpu, const bindingTableDescription_tgfx* desc,
                                      bindingTableType_tgfxhnd* bindingTableHandle) {
#ifdef VULKAN_DEBUGGING
  // Check if pool has enough space for the desc set and if there is, then decrease the amount of
  // available descs in the pool for other checks
  if (!desc->ElementCount && !desc->SttcSmplrs) {
    printer(result_tgfx_FAIL, "You shouldn't create a binding table with ElementCount = 0");
    return result_tgfx_FAIL;
  }
#endif
  GPU_VKOBJ* GPU = core_vk->getGPUs().getOBJfromHANDLE(gpu);

  // Static Immutable Sampler binding
  VkDescriptorSetLayoutBinding bindngs[2] = {};
  VkSampler                    staticSamplers[VKCONST_MAXSTATICSAMPLERCOUNT];
  unsigned int                 dynamicbinding_i = 0;
  TGFXLISTCOUNT(core_tgfx_main, desc->SttcSmplrs, staticsamplerscount);
  if (desc->DescriptorType == shaderdescriptortype_tgfx_SAMPLER && desc->SttcSmplrs) {
    unsigned int i = 0;
    while (desc->SttcSmplrs[i] != core_tgfx_main->INVALIDHANDLE) {
      if (!hidden->samplers.getOBJfromHANDLE(desc->SttcSmplrs[i])) {
        printer(result_tgfx_FAIL,
                "You shouldn't give NULL or invalid handles to StaticSamplers list!");
        return result_tgfx_WARNING;
      }

      i++;
    }
    for (unsigned int i = 0; i < staticsamplerscount; i++) {
      staticSamplers[i] = hidden->samplers.getOBJfromHANDLE(desc->SttcSmplrs[i])->vk_sampler;
    }
    bindngs[0].binding            = dynamicbinding_i;
    bindngs[0].descriptorCount    = staticsamplerscount;
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

  VkDescriptorSetLayout DSL;
  {
    VkDescriptorSetLayoutCreateInfo ci = {};
    ci.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ci.flags                           = 0;
    ci.pNext                           = nullptr;
    ci.bindingCount                    = dynamicbinding_i + 1;
    ci.pBindings                       = bindngs;
    ThrowIfFailed(vkCreateDescriptorSetLayout(GPU->vk_logical, &ci, nullptr, &DSL),
                  "Descriptor Set Layout creation has failed!");
  }

  BINDINGTABLETYPE_VKOBJ* finalobj = hidden->bindingtabletypes.create_OBJ();
  finalobj->vk_descType            = bindngs[dynamicbinding_i].descriptorType;
  finalobj->m_elementCount = bindngs[dynamicbinding_i].descriptorCount + staticsamplerscount;
  finalobj->m_gpu                  = GPU->gpuIndx();
  finalobj->vk_layout              = DSL;
  finalobj->vk_stages              = bindngs[dynamicbinding_i].stageFlags;
  *bindingTableHandle              = hidden->bindingtabletypes.returnHANDLEfromOBJ(finalobj);
  return result_tgfx_SUCCESS;
}

result_tgfx vk_instantiateBindingTable(bindingTableType_tgfxhnd tableType, unsigned char isStatic,
                                       bindingTable_tgfxhnd* table) {
  BINDINGTABLETYPE_VKOBJ* vkTableType = hidden->bindingtabletypes.getOBJfromHANDLE(tableType);

  GPU_VKOBJ* gpu = core_vk->getGPUs()[vkTableType->m_gpu];

  // Create Descriptor Pool
  VkDescriptorPool pool;
  {
    VkDescriptorPoolCreateInfo ci = {};
    ci.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ci.maxSets                    = 1;
    ci.pNext                      = nullptr;
    ci.poolSizeCount              = 1;
    VkDescriptorPoolSize size     = {};
    size.descriptorCount          = vkTableType->m_elementCount;
    size.type                     = vkTableType->vk_descType;
    ci.pPoolSizes                 = &size;
    ci.flags                      = isStatic ? 0 : VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    THROW_RETURN_IF_FAIL(vkCreateDescriptorPool(gpu->vk_logical, &ci, nullptr, &pool),
                         "VkCreateDescriptorPool failed!", result_tgfx_FAIL);
  }

  VkDescriptorSet set;
  // Allocate Descriptor Set
  {
    VkDescriptorSetAllocateInfo ai = {};
    ai.descriptorPool              = pool;
    ai.descriptorSetCount          = 1;
    ai.pNext                       = nullptr;
    ai.pSetLayouts                 = &vkTableType->vk_layout;
    ai.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    if (ThrowIfFailed(vkAllocateDescriptorSets(gpu->vk_logical, &ai, &set),
                      "vkAllocateDescriptorSets() failed!")) {
      vkDestroyDescriptorPool(gpu->vk_logical, pool, nullptr);
      return result_tgfx_FAIL;
    }
  }

  BINDINGTABLEINST_VKOBJ* finalobj = hidden->bindingtableinsts.create_OBJ();
  finalobj->vk_pool                = pool;
  finalobj->vk_set                 = set;
  finalobj->m_isStatic             = isStatic;
  finalobj->m_type                 = tableType;
  switch (vkTableType->vk_descType) {
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      finalobj->m_descs =
        new (VKGLOBAL_VIRMEM_CONTENTMANAGER) texture_descVK[vkTableType->m_elementCount];
      break;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      finalobj->m_descs =
        new (VKGLOBAL_VIRMEM_CONTENTMANAGER) buffer_descVK[vkTableType->m_elementCount];
    case VK_DESCRIPTOR_TYPE_SAMPLER:
      finalobj->m_descs =
        new (VKGLOBAL_VIRMEM_CONTENTMANAGER) sampler_descVK[vkTableType->m_elementCount];
      break;
  }
  *table = hidden->bindingtableinsts.returnHANDLEfromOBJ(finalobj);
  return result_tgfx_SUCCESS;
}

// Don't call this if there is no binding table & call buffer!
bool VKPipelineLayoutCreation(GPU_VKOBJ* GPU, bindingTableType_tgfxhnd* descs,
                              unsigned int descsCount, bool isCallBufferSupported,
                              VkPipelineLayout* layout) {
  VkPipelineLayoutCreateInfo pl_ci = {};
  pl_ci.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pl_ci.pNext                      = nullptr;
  pl_ci.flags                      = 0;

  VkDescriptorSetLayout DESCLAYOUTs[VKCONST_MAXDESCSET_PERLIST] = {};
  for (unsigned int typeSets_i = 0; typeSets_i < descsCount; typeSets_i++) {
    BINDINGTABLETYPE_VKOBJ* type = hidden->bindingtabletypes.getOBJfromHANDLE(descs[typeSets_i]);
    if (type->m_gpu != GPU->gpuIndx()) {
      printer(result_tgfx_FAIL, "Binding table doesn't match with the GPU!");
      return false;
    }
    DESCLAYOUTs[typeSets_i] = type->vk_layout;
  }
  pl_ci.setLayoutCount      = descsCount;
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

  ThrowIfFailed(vkCreatePipelineLayout(GPU->vk_logical, &pl_ci, nullptr, layout),
                "Link_MaterialType() failed at vkCreatePipelineLayout()!");

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
  GPU_VKOBJ*   GPU                   = core_vk->getGPUs().getOBJfromHANDLE(gpu);
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
        printer(result_tgfx_FAIL, ( const char* )binary_spirv_data);
        delete binary_spirv_data;
        return result_tgfx_FAIL;
      }
      break;
    default: printer(result_tgfx_NOTCODED, "Vulkan backend doesn't support this shading language");
  }

  // Create Vertex Shader Module
  VkShaderModuleCreateInfo ci = {};
  ci.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  ci.flags                    = 0;
  ci.pNext                    = nullptr;
  ci.pCode                    = reinterpret_cast<const uint32_t*>(binary_spirv_data);
  ci.codeSize                 = static_cast<size_t>(binary_spirv_datasize);

  VkShaderModule Module;
  ThrowIfFailed(vkCreateShaderModule(GPU->vk_logical, &ci, 0, &Module),
                "Shader Source is failed at creation!");

  SHADERSOURCE_VKOBJ* SHADERSOURCE = hidden->shadersources.create_OBJ();
  SHADERSOURCE->Module             = Module;
  SHADERSOURCE->stage              = shaderstage;
  SHADERSOURCE->m_gpu              = gpu;

  *ShaderSourceHandle = hidden->shadersources.returnHANDLEfromOBJ(SHADERSOURCE);
  return result_tgfx_SUCCESS;
}
void                  vk_destroyShaderSource(shaderSource_tgfxhnd ShaderSourceHandle) {}
VkColorComponentFlags vk_findColorWriteMask(textureChannels_tgfx chnnls);

inline void CountDescSets(bindingTableType_tgfxlsthnd descset, unsigned int* finaldesccount,
                          unsigned int finaldescs[VKCONST_MAXDESCSET_PERLIST]) {
  TGFXLISTCOUNT(core_tgfx_main, descset, listsize);
  unsigned int valid_descset_i = 0;
  for (unsigned int i = 0; i < listsize; i++) {
    bindingTableType_tgfxhnd b_table = descset[i];
    if (!b_table) {
      continue;
    }
    BINDINGTABLETYPE_VKOBJ* descset = hidden->bindingtabletypes.getOBJfromHANDLE(b_table);
    if (!descset) {
      printer(10);
    }
    if (valid_descset_i >= VKCONST_MAXDESCSET_PERLIST) {
      printer(11);
    }
    finaldescs[valid_descset_i++] = hidden->bindingtabletypes.getINDEXbyOBJ(descset);
  }
  *finaldesccount = valid_descset_i;
}

result_tgfx vk_createRasterPipeline(const rasterPipelineDescription_tgfx* desc,
                                    extension_tgfxlsthnd exts, pipeline_tgfxhnd* hnd) {
  GPU_VKOBJ* GPU = nullptr;

  // Detect GPU & create shader stage infos
  VkPipelineShaderStageCreateInfo STAGEs[2] = {};
  {
    // Find vertex & fragment shader sources and detect GPU
    SHADERSOURCE_VKOBJ *vkSource_vertex = nullptr, *vkSource_fragment = nullptr;
    unsigned int        ShaderSourceCount = 0;
    while (ShaderSourceCount < 2 &&
           desc->shaderSourceList[ShaderSourceCount] != core_tgfx_main->INVALIDHANDLE) {
      SHADERSOURCE_VKOBJ* source =
        hidden->shadersources.getOBJfromHANDLE(desc->shaderSourceList[ShaderSourceCount]);
      if (!GPU) {
        GPU = core_vk->getGPUs().getOBJfromHANDLE(source->m_gpu);
      } else if (GPU != core_vk->getGPUs().getOBJfromHANDLE(source->m_gpu)) {
        printer(result_tgfx_FAIL, "Shaders has to be from the same GPU!");
        return result_tgfx_FAIL;
      }
      switch (source->stage) {
        case shaderStage_tgfx_VERTEXSHADER:
          if (vkSource_vertex) {
            printer(result_tgfx_FAIL,
                    "Link_MaterialType() has failed because there 2 vertex shaders in the list!");
            return result_tgfx_FAIL;
          }
          vkSource_vertex = source;
          break;
        case shaderStage_tgfx_FRAGMENTSHADER:
          if (vkSource_fragment) {
            printer(result_tgfx_FAIL,
                    "Link_MaterialType() has failed because there 2 fragment shaders in the list!");
            return result_tgfx_FAIL;
          }
          vkSource_fragment = source;
          break;
        default:
          printer(
            result_tgfx_NOTCODED,
            "Link_MaterialType() has failed because list has unsupported shader source type!");
          return result_tgfx_NOTCODED;
      }
      ShaderSourceCount++;
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

  VkPipelineVertexInputStateCreateInfo vertexInputState = {};
  {
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    if (desc->attribLayout) {
      VERTEXATTRIBLAYOUT_VKOBJ* LAYOUT =
        hidden->vertexattributelayouts.getOBJfromHANDLE(desc->attribLayout);
      vertexInputState.pVertexBindingDescriptions      = &LAYOUT->BindingDesc;
      vertexInputState.vertexBindingDescriptionCount   = 1;
      vertexInputState.pVertexAttributeDescriptions    = LAYOUT->AttribDescs;
      vertexInputState.vertexAttributeDescriptionCount = LAYOUT->AttribDesc_Count;
    }
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

  unsigned int typeSets[VKCONST_MAXDESCSET_PERLIST] = {};
  uint32_t     typesetscount                        = 0;
  CountDescSets(desc->typeTables, &typesetscount, typeSets);
  VkPipelineLayout layout = {};
  if (!VKPipelineLayoutCreation(GPU, desc->typeTables, typesetscount, false, &layout)) {
    printer(result_tgfx_FAIL, "Compile_RasterPipeline() has failed at VKPipelineLayoutCreation!");
    return result_tgfx_FAIL;
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
    vk_fillRasterPipelineStateInfo(GPU, &ci, desc, exts);
    ThrowIfFailed(vkCreateGraphicsPipelines(GPU->vk_logical, nullptr, 1, &ci, nullptr, &pipeline),
                  "vkCreateGraphicsPipelines has failed!");
  }

  PIPELINE_VKOBJ* pipelineObj = hidden->pipelines.create_OBJ();
  pipelineObj->m_gpu          = GPU->gpuIndx();
  memcpy(pipelineObj->m_typeSETs, typeSets, sizeof(pipelineObj->m_typeSETs));
  pipelineObj->vk_layout = layout;
  pipelineObj->vk_object = pipeline;
  pipelineObj->vk_type   = VK_PIPELINE_BIND_POINT_GRAPHICS;
  for (uint32_t i = 0; i < TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT; i++) {
    pipelineObj->vk_colorAttachmentFormats[i] = vk_findFormatVk(desc->colorTextureFormats[i]);
  }
  pipelineObj->vk_depthAttachmentFormat = vk_findFormatVk(desc->depthStencilTextureFormat);
  *hnd                                  = hidden->pipelines.returnHANDLEfromOBJ(pipelineObj);
  return result_tgfx_SUCCESS;
}
/*
result_tgfx vk_copyRasterPipeline(rasterPipeline_tgfxhnd src, extension_tgfxlsthnd exts,
                                  rasterPipeline_tgfxhnd* dst) {
  RASTERPIPELINE_VKOBJ* srcPipeline = hidden->pipelines.getOBJfromHANDLE(src);
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
result_tgfx vk_createComputePipeline(shaderSource_tgfxhnd        Source,
                                     bindingTableType_tgfxlsthnd TypeBindingTables,
                                     unsigned char               isCallBufferSupported,
                                     pipeline_tgfxhnd*           ComputeTypeHandle) {
  VkComputePipelineCreateInfo cp_ci  = {};
  SHADERSOURCE_VKOBJ*         SHADER = hidden->shadersources.getOBJfromHANDLE(Source);
  GPU_VKOBJ*                  GPU    = core_vk->getGPUs().getOBJfromHANDLE(SHADER->m_gpu);

  unsigned int typeSets[VKCONST_MAXDESCSET_PERLIST] = {};
  uint32_t     typesetscount                        = 0;
  CountDescSets(TypeBindingTables, &typesetscount, typeSets);

  VkPipelineLayout layout = {};
  if (!VKPipelineLayoutCreation(GPU, TypeBindingTables, typesetscount, isCallBufferSupported,
                                &layout)) {
    printer(result_tgfx_FAIL,
            "Compile_ComputeType() has failed at VKDescSet_PipelineLayoutCreation!");
    return result_tgfx_FAIL;
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
    ThrowIfFailed(
      vkCreateComputePipelines(GPU->vk_logical, VK_NULL_HANDLE, 1, &cp_ci, nullptr, &pipelineObj),
      "Compile_ComputeShader() has failed at vkCreateComputePipelines!");
  }

  PIPELINE_VKOBJ* obj = hidden->pipelines.create_OBJ();
  obj->m_gpu          = GPU->gpuIndx();
  memcpy(obj->m_typeSETs, typeSets, sizeof(obj->m_typeSETs));
  obj->vk_layout     = layout;
  obj->vk_object     = pipelineObj;
  obj->vk_type       = VK_PIPELINE_BIND_POINT_COMPUTE;
  *ComputeTypeHandle = hidden->pipelines.returnHANDLEfromOBJ(obj);
  return result_tgfx_SUCCESS;
}

result_tgfx vk_copyComputePipeline(pipeline_tgfxhnd src, extension_tgfxlsthnd exts,
                                   pipeline_tgfxhnd* dst) {
  return result_tgfx_NOTCODED;
}

static constexpr uint32_t VKCONST_MAXDESCCHANGE_PERCALL = 1024;
// Set a descriptor of the binding table created with shaderdescriptortype_tgfx_SAMPLER
result_tgfx vk_setBindingTable_Sampler(bindingTable_tgfxhnd bindingtable, unsigned int bindingCount,
                               const unsigned int* bindingIndices, const sampler_tgfxlsthnd samplerHandles) {
  BINDINGTABLEINST_VKOBJ* set     = hidden->bindingtableinsts.getOBJfromHANDLE(bindingtable);
  BINDINGTABLETYPE_VKOBJ* setType = hidden->bindingtabletypes.getOBJfromHANDLE(set->m_type);

  VkWriteDescriptorSet writeInfos[VKCONST_MAXDESCCHANGE_PERCALL] = {};
  VkDescriptorImageInfo imageInfos[VKCONST_MAXDESCCHANGE_PERCALL] = {};
  for (uint32_t bindingIter = 0; bindingIter < bindingCount; bindingIter++) {
    SAMPLER_VKOBJ* sampler = hidden->samplers.getOBJfromHANDLE(samplerHandles[bindingIter]);
#ifdef VULKAN_DEBUGGING
    uint32_t elementIndx = bindingIndices[bindingIter];
    if (!set || !sampler || bindingIndices[bindingIter] >= setType->m_elementCount ||
        setType->vk_descType != VK_DESCRIPTOR_TYPE_SAMPLER) {
      printer(result_tgfx_INVALIDARGUMENT, "vk_setBindingTable_Buffer() has invalid input!");
      return result_tgfx_INVALIDARGUMENT;
    }
    sampler_descVK& samplerDESC         = (( sampler_descVK* )set->m_descs)[elementIndx];
    unsigned char   elementUpdateStatus = 0;
    if (!samplerDESC.isUpdated.compare_exchange_weak(elementUpdateStatus, 1)) {
      printer(
        result_tgfx_FAIL,
        "SetDescriptor_Sampler() failed because you already changed the descriptor in the same "
        "frame!");
    }
#else
    sampler_descVK& samplerDESC = (( sampler_descVK* )set->DescElements)[elementIndex];
    samplerDESC.isUpdated.store(1);
#endif

    samplerDESC.sampler_obj = sampler->vk_sampler;
    imageInfos[bindingIter].sampler = sampler->vk_sampler;

    writeInfos[bindingIter].descriptorCount  = 1;
    writeInfos[bindingIter].descriptorType   = setType->vk_descType;
    writeInfos[bindingIter].dstArrayElement  = elementIndx;
    writeInfos[bindingIter].dstBinding       = 0;
    writeInfos[bindingIter].pTexelBufferView = nullptr;
    writeInfos[bindingIter].pNext            = nullptr;
    writeInfos[bindingIter].dstSet           = set->vk_set;
    writeInfos[bindingIter].pImageInfo       = &imageInfos[bindingIter];
    writeInfos[bindingIter].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  }
  vkUpdateDescriptorSets(core_vk->getGPUs()[setType->m_gpu]->vk_logical, bindingCount, writeInfos,
                         0, nullptr);
  return result_tgfx_SUCCESS;
}

// Set a descriptor of the binding table created with shaderdescriptortype_tgfx_BUFFER
result_tgfx vk_setBindingTable_Buffer(bindingTable_tgfxhnd table, unsigned int bindingCount,
                                      const unsigned int*     bindingIndices,
                                      const buffer_tgfxlsthnd buffers, const unsigned int* offsets,
                                      const unsigned int* sizes, extension_tgfxlsthnd exts) {
  BINDINGTABLEINST_VKOBJ* set = hidden->bindingtableinsts.getOBJfromHANDLE(table);
  if (!set) {
    printer(result_tgfx_INVALIDARGUMENT, "Invalid bindingtable!");
    return result_tgfx_INVALIDARGUMENT;
  }
  BINDINGTABLETYPE_VKOBJ* setType = hidden->bindingtabletypes.getOBJfromHANDLE(set->m_type);
  if (setType->vk_descType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
      setType->vk_descType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
    printer(result_tgfx_INVALIDARGUMENT, "Invalid bindingtable type!");
    return result_tgfx_INVALIDARGUMENT;
  }

  VkWriteDescriptorSet writeInfos[VKCONST_MAXDESCCHANGE_PERCALL] = {};
  for (uint32_t bindingIter = 0; bindingIter < bindingCount; bindingIter++) {
    BUFFER_VKOBJ*  buffer      = hidden->buffers.getOBJfromHANDLE(buffers[bindingIter]);
    uint32_t       elementIndx = bindingIndices[bindingIter];
    buffer_descVK& bufferDESC  = (( buffer_descVK* )set->m_descs)[elementIndx];
#ifdef VULKAN_DEBUGGING
    BINDINGTABLETYPE_VKOBJ* setType = hidden->bindingtabletypes.getOBJfromHANDLE(set->m_type);
    if (!buffer || elementIndx >= setType->m_elementCount) {
      printer(result_tgfx_INVALIDARGUMENT, "SetDescriptor_Buffer() has invalid input!");
      return result_tgfx_FAIL;
    }
    unsigned char elementUpdateStatus = 0;
    if (!bufferDESC.isUpdated.compare_exchange_weak(elementUpdateStatus, 1)) {
      printer(
        result_tgfx_FAIL,
        "SetDescriptor_Buffer() failed because you already changed the descriptor in the same "
        "frame!");
      return result_tgfx_FAIL;
    }
#else
    bufferDESC.isUpdated.store(1);
#endif
    bufferDESC.info.buffer = buffer->vk_buffer;
    bufferDESC.info.offset = (offsets == nullptr) ? (0) : (offsets[bindingIter]);
    bufferDESC.info.range  = (sizes == nullptr) ? (buffer->m_intendedSize) : (sizes[bindingIter]);

    writeInfos[bindingIter].descriptorCount  = 1;
    writeInfos[bindingIter].descriptorType   = setType->vk_descType;
    writeInfos[bindingIter].dstArrayElement  = elementIndx;
    writeInfos[bindingIter].dstBinding       = 0;
    writeInfos[bindingIter].pTexelBufferView = nullptr;
    writeInfos[bindingIter].pNext            = nullptr;
    writeInfos[bindingIter].dstSet           = set->vk_set;
    writeInfos[bindingIter].pBufferInfo      = &bufferDESC.info;
    writeInfos[bindingIter].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  }
  vkUpdateDescriptorSets(core_vk->getGPUs()[setType->m_gpu]->vk_logical, bindingCount, writeInfos,
                         0, nullptr);

  return result_tgfx_SUCCESS;
}
result_tgfx vk_setBindingTable_Texture(bindingTable_tgfxhnd table, unsigned int bindingCount,
                                       const unsigned int*      bindingIndices,
                                       const texture_tgfxlsthnd textures) {
  BINDINGTABLEINST_VKOBJ* set = hidden->bindingtableinsts.getOBJfromHANDLE(table);
  if (!set) {
    printer(result_tgfx_INVALIDARGUMENT, "Invalid bindingtable!");
    return result_tgfx_INVALIDARGUMENT;
  }
  BINDINGTABLETYPE_VKOBJ* setType = hidden->bindingtabletypes.getOBJfromHANDLE(set->m_type);
  if (setType->vk_descType != VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE &&
      setType->vk_descType != VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
    printer(result_tgfx_INVALIDARGUMENT, "Invalid bindingtable type!");
    return result_tgfx_INVALIDARGUMENT;
  }
  if (bindingCount > VKCONST_MAXDESCCHANGE_PERCALL) {
    printer(result_tgfx_FAIL, "You exceeded VKCONST_MAXDESCCHANGE_PERCALL");
    return result_tgfx_FAIL;
  }
  VkWriteDescriptorSet writeInfos[VKCONST_MAXDESCCHANGE_PERCALL] = {};
  for (uint32_t bindingIter = 0; bindingIter < bindingCount; bindingIter++) {
    TEXTURE_VKOBJ*  texture     = hidden->textures.getOBJfromHANDLE(textures[bindingIter]);
    uint32_t        elementIndx = bindingIndices[bindingIter];
    texture_descVK& textureDESC = (( texture_descVK* )set->m_descs)[elementIndx];
#ifdef VULKAN_DEBUGGING
    if (!texture || elementIndx >= setType->m_elementCount) {
      printer(result_tgfx_INVALIDARGUMENT,
              "setBindingTable_Texture() has invalid input! Either texture is invalid or index");
      return result_tgfx_INVALIDARGUMENT;
    }
    unsigned char elementUpdateStatus = 0;
    if (!textureDESC.isUpdated.compare_exchange_weak(elementUpdateStatus, 1)) {
      printer(
        result_tgfx_FAIL,
        "vk_setBindingTable_Texture() failed because you already changed the descriptor in the "
        "same frame!");
      return result_tgfx_FAIL;
    }
#else
    textureDESC.isUpdated.store(1);
#endif
    textureDESC.info.imageLayout = (setType->vk_descType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                                     ? VK_IMAGE_LAYOUT_GENERAL
                                     : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    textureDESC.info.imageView   = texture->vk_imageView;
    textureDESC.info.sampler     = VK_NULL_HANDLE;

    writeInfos[bindingIter].descriptorCount  = 1;
    writeInfos[bindingIter].descriptorType   = setType->vk_descType;
    writeInfos[bindingIter].dstArrayElement  = elementIndx;
    writeInfos[bindingIter].dstBinding       = 0;
    writeInfos[bindingIter].pTexelBufferView = nullptr;
    writeInfos[bindingIter].pNext            = nullptr;
    writeInfos[bindingIter].dstSet           = set->vk_set;
    writeInfos[bindingIter].pImageInfo       = &textureDESC.info;
    writeInfos[bindingIter].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  }
  vkUpdateDescriptorSets(core_vk->getGPUs()[setType->m_gpu]->vk_logical, bindingCount, writeInfos,
                         0, nullptr);
  return result_tgfx_SUCCESS;
}

/////////////////////////////////////////////////////
//								MEMORY
/////////////////////////////////////////////////////

result_tgfx vk_createHeap(gpu_tgfxhnd gpu, unsigned char memoryRegionID,
                          unsigned long long heapSize, extension_tgfxlsthnd exts,
                          heap_tgfxhnd* heapHnd) {
  if (exts) {
    printer(result_tgfx_WARNING, "Extensions're not supported in createHeap");
  }
  VkMemoryAllocateInfo memAlloc;
  memAlloc.allocationSize  = heapSize;
  memAlloc.memoryTypeIndex = memoryRegionID;
  memAlloc.pNext           = nullptr;
  memAlloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  GPU_VKOBJ*     vkGPU     = core_vk->getGPUs().getOBJfromHANDLE(gpu);
  VkDeviceMemory vkDevMem;
  ThrowIfFailed(vkAllocateMemory(vkGPU->vk_logical, &memAlloc, nullptr, &vkDevMem),
                "Heap allocation failed at vkAllocateMemory");

  HEAP_VKOBJ* heap         = hidden->heaps.create_OBJ();
  heap->vk_memoryHandle    = vkDevMem;
  heap->vk_memoryTypeIndex = memoryRegionID;
  heap->m_size             = heapSize;
  heap->m_GPU              = vkGPU->gpuIndx();
  *heapHnd                 = hidden->heaps.returnHANDLEfromOBJ(heap);
  return result_tgfx_SUCCESS;
}
result_tgfx vk_getHeapRequirement_Texture(texture_tgfxhnd i_texture, extension_tgfxlsthnd exts,
                                          heapRequirementsInfo_tgfx* reqs) {
  if (exts) {
    printer(result_tgfx_WARNING, "Extensions not supported");
  }
  TEXTURE_VKOBJ* texture  = hidden->textures.getOBJfromHANDLE(i_texture);
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
result_tgfx vk_getHeapRequirement_Buffer(buffer_tgfxhnd i_buffer, extension_tgfxlsthnd exts,
                                         heapRequirementsInfo_tgfx* reqs) {
  if (exts) {
    printer(result_tgfx_WARNING, "Extensions not supported");
  }
  BUFFER_VKOBJ* buf      = hidden->buffers.getOBJfromHANDLE(i_buffer);
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
                                  extension_tgfxlsthnd exts, unsigned long long* size) {
  return result_tgfx_NOTCODED;
}

result_tgfx vk_bindToHeap_Buffer(heap_tgfxhnd i_heap, unsigned long long offset,
                                 buffer_tgfxhnd i_buffer, extension_tgfxlsthnd exts) {
  BUFFER_VKOBJ* buffer = hidden->buffers.getOBJfromHANDLE(i_buffer);
  HEAP_VKOBJ*   heap   = hidden->heaps.getOBJfromHANDLE(i_heap);
  GPU_VKOBJ*    gpu    = core_vk->getGPUs()[buffer->m_GPU];
  if (buffer->m_memReqs.requiresDedicatedAlloc) {
    printer(result_tgfx_FAIL,
            "Driver requires this buffer to be allocated specially, so you can't bind this to an "
            "heap by hand. TGFX automatically bound it!");
    return result_tgfx_FAIL;
  }
  if (offset % buffer->m_memReqs.vk_memReqs.alignment) {
    printer(result_tgfx_FAIL,
            "Offset should be multiple of the resource's alignment in bindToHeap");
    return result_tgfx_FAIL;
  }
  if (ThrowIfFailed(
        vkBindBufferMemory(gpu->vk_logical, buffer->vk_buffer, heap->vk_memoryHandle, offset),
        "bindToHeap_Buffer() has failed at vkBindBufferMemory!")) {
    return result_tgfx_FAIL;
  }
  return result_tgfx_SUCCESS;
}
result_tgfx vk_bindToHeap_Texture(heap_tgfxhnd i_heap, unsigned long long offset,
                                  texture_tgfxhnd i_texture, extension_tgfxlsthnd exts) {
  TEXTURE_VKOBJ* texture = hidden->textures.getOBJfromHANDLE(i_texture);
  HEAP_VKOBJ*    heap    = hidden->heaps.getOBJfromHANDLE(i_heap);
  GPU_VKOBJ*     gpu     = core_vk->getGPUs()[texture->m_GPU];
  if (texture->m_memReqs.requiresDedicatedAlloc) {
    printer(result_tgfx_FAIL,
            "Driver requires this texture to be allocated specially, so you can't bind this to an "
            "heap by hand. TGFX automatically bound it!");
    return result_tgfx_FAIL;
  }
  if (offset % texture->m_memReqs.vk_memReqs.alignment) {
    printer(result_tgfx_FAIL,
            "Offset should be multiple of the resource's alignment in bindToHeap");
    return result_tgfx_FAIL;
  }
  if (ThrowIfFailed(
        vkBindImageMemory(gpu->vk_logical, texture->vk_image, heap->vk_memoryHandle, offset),
        "bindToHeap_Buffer() has failed at vkBindBufferMemory!")) {
    return result_tgfx_FAIL;
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

    ThrowIfFailed(vkCreateImageView(gpu->vk_logical, &ci, nullptr, &texture->vk_imageView),
                  "bindToHeap_Texture() has failed in vkCreateImageView()!");
  }

  return result_tgfx_SUCCESS;
}

result_tgfx vk_mapHeap(heap_tgfxhnd i_heap, unsigned long long offset, unsigned long long size,
                       extension_tgfxlsthnd exts, void** mappedRegion) {
  HEAP_VKOBJ* heap = hidden->heaps.getOBJfromHANDLE(i_heap);
  GPU_VKOBJ*  gpu  = core_vk->getGPUs()[heap->m_GPU];
  THROW_RETURN_IF_FAIL(
    vkMapMemory(gpu->vk_logical, heap->vk_memoryHandle, offset, size, 0, mappedRegion),
    "VkMapMemory failed!", result_tgfx_FAIL);
  return result_tgfx_SUCCESS;
}

result_tgfx vk_unmapHeap(heap_tgfxhnd i_heap) {
  HEAP_VKOBJ* heap = hidden->heaps.getOBJfromHANDLE(i_heap);
  GPU_VKOBJ*  gpu  = core_vk->getGPUs()[heap->m_GPU];
  vkUnmapMemory(gpu->vk_logical, heap->vk_memoryHandle);
  return result_tgfx_SUCCESS;
}

/////////////////////////////////////////////////////
///				INITIALIZATION PROCEDURE
/////////////////////////////////////////////////////

inline void set_functionpointers() {
  core_tgfx_main->contentmanager->compileShaderSource      = vk_compileShaderSource;
  core_tgfx_main->contentmanager->createBindingTableType   = vk_createBindingTableType;
  core_tgfx_main->contentmanager->instantiateBindingTable  = vk_instantiateBindingTable;
  core_tgfx_main->contentmanager->copyComputePipeline      = vk_copyComputePipeline;
  core_tgfx_main->contentmanager->createComputePipeline    = vk_createComputePipeline;
  core_tgfx_main->contentmanager->createBuffer             = vk_createBuffer;
  core_tgfx_main->contentmanager->createTexture            = vk_createTexture;
  core_tgfx_main->contentmanager->createVertexAttribLayout = vk_createVertexAttribLayout;
  // core_tgfx_main->contentmanager->destroyRasterPipeline      = vk_destroyRasterPipeline;
  core_tgfx_main->contentmanager->deleteShaderSource         = vk_destroyShaderSource;
  core_tgfx_main->contentmanager->destroyVertexAttribLayout  = vk_destroyVertexAttribLayout;
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
  VKGLOBAL_VIRMEM_CONTENTMANAGER =
    vk_virmem::allocate_dynamicmem(sizeof(gpudatamanager_public) + sizeof(gpudatamanager_private));
  contentmanager         = new (VKGLOBAL_VIRMEM_CONTENTMANAGER) gpudatamanager_public;
  contentmanager->hidden = new (VKGLOBAL_VIRMEM_CONTENTMANAGER) gpudatamanager_private;
  hidden                 = contentmanager->hidden;

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
VK_LINEAR_OBJARRAY<BINDINGTABLETYPE_VKOBJ, bindingTableType_tgfxhnd, 1 << 10>&
gpudatamanager_public::GETBINDINGTABLETYPE_ARRAY() {
  return hidden->bindingtabletypes;
}
VK_LINEAR_OBJARRAY<SUBRASTERPASS_VKOBJ, subRasterpass_tgfxhnd, 1 << 16>&
gpudatamanager_public::GETSUBRASTERPASS_ARRAY() {
  return hidden->subrasterpasses;
}

unsigned int vk_calculateSizeOfVertexLayout(const datatype_tgfx* ATTRIBUTEs, unsigned int count) {
  unsigned int size = 0;
  for (unsigned int i = 0; i < count; i++) {
    size += vk_getDataTypeByteSizes(ATTRIBUTEs[i]);
  }
  return size;
}