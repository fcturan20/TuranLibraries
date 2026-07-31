#define NOMINMAX
#include "vk_contentmanager.h"

#include <TGfxCore.h>
#include <TGfxGpuContentManager.h>

#include <mutex>

#include <Profiler.h>
#include "vk_core.h"
#include "vk_includes.h"
#include "vk_predefinitions.h"
#include "vk_queue.h"
#include "vk_renderer.h"
#include "vk_resource.h"
#include "vkGlslang.h"

namespace TGFX
{
namespace Vulkan
{
// Binding Model and Table Management

// These are used to initialize descriptors
// So backend won't fail because of a NULL descriptor
//     -but in DEBUG release, it'll complain about it-
// Texture is a 1x1 texture, buffer is 1 byte buffer, sampler is as default as possible
VkSampler defaultSampler;
VkBuffer defaultBuffer;
Texture* defaultTexture;

struct sampler_descVK
{
	VkSampler sampler_obj = defaultSampler;
};

struct buffer_descVK
{ // Both for BUFFER and EXT_UNIFORMBUFFER
	VkDescriptorBufferInfo info = {};
};

struct texture_descVK
{ // Both for SAMPLEDTEXTURE and STORAGEIMAGE
	VkDescriptorImageInfo info = {};
};

void destroyAllResources() {}

TCResult createSampler(TGfxGpu gpu, const TGfxSamplerDescription* desc, TGfxSampler* hnd)
{
	GPU* GPU = GetVkObject(gpu);

	VkSampler sampler;
	{
		VkSamplerCreateInfo s_ci = {};
		s_ci.addressModeU = GetVkEnum(desc->WrapWidth);
		s_ci.addressModeV = GetVkEnum(desc->WrapHeight);
		s_ci.addressModeW = GetVkEnum(desc->WrapDepth);
		s_ci.anisotropyEnable = VK_FALSE;
		s_ci.borderColor = VkBorderColor::VK_BORDER_COLOR_MAX_ENUM;
		s_ci.compareEnable = VK_FALSE;
		s_ci.flags = 0;
		s_ci.magFilter = GetFilter(desc->MagFilter);
		s_ci.minFilter = GetFilter(desc->MinFilter);
		s_ci.maxLod = static_cast<float>(desc->MaxMipLevel);
		s_ci.minLod = static_cast<float>(desc->MinMipLevel);
		s_ci.mipLodBias = 0.0f;
		s_ci.mipmapMode = GetMipmapMode(desc->MinFilter);
		s_ci.pNext = nullptr;
		s_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		s_ci.unnormalizedCoordinates = VK_FALSE;
		if (vkCreateSampler(GPU->vk_logical, &s_ci, nullptr, &sampler) != VK_SUCCESS)
		{
			return vkPrint(26);
		}
	}

	Sampler* SAMPLER = GContentManagerContext->Samplers.CreateObject(GPU);
	SAMPLER->vk_sampler.Set(sampler);
	*hnd = GetOpaqueHandle(SAMPLER);
	return {TC_RESULTSTATE_SUCCESS, 0};
}
void vk_destroySampler(TGfxSampler sampler)
{
	Sampler* vkSampler = GetVkObject(sampler);
#ifdef VULKAN_DEBUGGING
	assert(vkSampler && "Invalid sampler!");
#endif // VULKAN_DEBUGGING
	vkDestroySampler((vkSampler->GetGpu())->vk_logical, vkSampler->vk_sampler, nullptr);
}

/*Attributes are ordered as the same order of input vector
 * For example: Same attribute ID may have different location/order in another attribute layout
 * So you should gather your vertex buffer data according to that
 */
TU4 CalculateSizeOfVertexLayout(const TGfxDataType* ATTRIBUTEs, TU4 count)
{
	TU4 size = 0;
	for (TU4 i = 0; i < count; i++)
		size += GetDataTypeSizeInBytes(ATTRIBUTEs[i]);
	return size;
}

TCResult vk_createTexture(TGfxGpu i_gpu, const TGfxTextureDescription* desc, TGfxTexture* TextureHnd)
{
	GPU* gpu = GetVkObject(i_gpu);
	VkImageUsageFlags usageFlag = findTextureUsageFlagVk(desc->usage);
	if (desc->ChannelType == TGFX_TEXTURE_CHANNELS_D24S8 || desc->ChannelType == TGFX_TEXTURE_CHANNELS_D32)
		usageFlag &= ~(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	else
		usageFlag &= ~(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	if (desc->MipCount > std::floor(std::log2(std::max(desc->Resolution.x, desc->Resolution.y))) + 1 || !desc->MipCount)
		return vkPrint(26);

	// Create VkImage
	VkImage vkTextureObj;
	VkImageCreateInfo im_ci = {};
	{
		im_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		im_ci.extent.width = desc->Resolution.x;
		im_ci.extent.height = desc->Resolution.y;
		im_ci.extent.depth = 1;
		if (desc->Dimension == TGFX_TEXTURE_DIMENSIONS_2DCUBE)
		{
			im_ci.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			im_ci.arrayLayers = 6;
		}
		else
		{
			im_ci.flags = 0;
			im_ci.arrayLayers = 1;
		}
		im_ci.format = GetVkEnum(desc->ChannelType);
		im_ci.imageType = GetVkEnum(desc->Dimension);
		im_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		im_ci.mipLevels = static_cast<uint32_t>(desc->MipCount);
		im_ci.pNext = nullptr;
		im_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		im_ci.tiling = GetVkEnum(desc->DataOrder);
		im_ci.usage = usageFlag;
		im_ci.samples = VK_SAMPLE_COUNT_1_BIT;

		if (vkCreateImage(gpu->vk_logical, &im_ci, nullptr, &vkTextureObj) != VK_SUCCESS)
		{
			return vkPrint(24);
		}
	}

	VMemoryRequirements memReqs = VMemoryRequirements::GETINVALID();
	vkGetImageMemoryRequirements(gpu->vk_logical, vkTextureObj, &memReqs.vk_memReqs);
	if (memReqs.requiresDedicatedAlloc)
	{
		vkPrint(25);
		vkDestroyImage(gpu->vk_logical, vkTextureObj, nullptr);
		return {TC_RESULTSTATE_FAILURE, 0};
	}

	Texture* texture = GContentManagerContext->Textures.CreateObject(gpu);
	texture->m_channels = desc->ChannelType;
	texture->Size = TGfxUVec2{.x = desc->Resolution.x, .y = desc->Resolution.y};
	texture->vk_imageUsage = usageFlag;
	texture->m_dim = desc->Dimension;
	texture->MipCount = desc->MipCount;
	texture->vk_image.Set(vkTextureObj);
	texture->vk_imageView.Set(VK_NULL_HANDLE);
	texture->m_memReqs = memReqs;

	*TextureHnd = GetOpaqueHandle(texture);
	return {TC_RESULTSTATE_SUCCESS, 0};
}
void vk_destroyTexture(TGfxTexture texture)
{
	Texture* vkTexture = GetVkObject(texture);
	assert(vkTexture && "Invalid texture");
	vkDestroyImage((vkTexture->GetGpu())->vk_logical, vkTexture->vk_image, nullptr);
	vkDestroyImageView((vkTexture->GetGpu())->vk_logical, vkTexture->vk_imageView, nullptr);
	GContentManagerContext->Textures.DestroyObject(vkTexture);
}

TCResult vk_createBuffer(TGfxGpu i_gpu, const TGfxBufferDescription* desc, TGfxBuffer* buffer)
{
	GPU* gpu = GetVkObject(i_gpu);

	// Create VkBuffer object
	VkBuffer vkBufObj;
	VkBufferCreateInfo ci = {};
	{
		ci.usage = findBufferUsageFlagVk(desc->usageFlag);
		ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		ci.size = desc->Size;

		if (vkCreateBuffer(gpu->vk_logical, &ci, nullptr, &vkBufObj) != VK_SUCCESS)
		{
			return vkPrint(27);
		}
	}
	VMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(gpu->vk_logical, vkBufObj, &memReqs.vk_memReqs);
	if (memReqs.requiresDedicatedAlloc)
	{
		auto res = vkPrint(25);
		vkDestroyBuffer(gpu->vk_logical, vkBufObj, nullptr);
		return res;
	}

	// Get buffer requirements and fill Buffer
	Buffer* o_buffer = GContentManagerContext->Buffers.CreateObject(gpu);
	{
		o_buffer->vk_buffer.Set(vkBufObj);
		o_buffer->vk_usage = ci.usage;
		o_buffer->m_intendedSize = desc->Size;
		o_buffer->m_memReqs = memReqs;
	}

	*buffer = GetOpaqueHandle(o_buffer);
	return {TC_RESULTSTATE_SUCCESS, 0};
}
void vk_destroyBuffer(TGfxBuffer buffer)
{
	Buffer* vkBuffer = GetVkObject(buffer);
	assert(vkBuffer && "Invalid texture");
	vkDestroyBuffer((vkBuffer->GetGpu())->vk_logical, vkBuffer->vk_buffer, nullptr);
	GContentManagerContext->Buffers.DestroyObject(vkBuffer);
}

VkConstU4 VKCONST_MAXSTATICSAMPLERCOUNT = 128;

void* getInvalidResourceByType(GPU* gpu, VkDescriptorType descType)
{
	switch (descType)
	{
	case VK_DESCRIPTOR_TYPE_SAMPLER: return gpu->m_invalidSampler;
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return gpu->m_invalidBuffer;
	case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return gpu->m_invalidShaderReadTexture;
	case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return gpu->m_invalidStorageTexture;
	default: assert(0 && "Invalid descriptor type"); return nullptr;
	}
}
VkDescriptorSetLayout CreateDescSetLayout(GPU* gpu, const TGfxBindingTableDescription const* const desc)
{
	// Static Immutable Sampler binding
	VkDescriptorSetLayoutBinding bindngs[2] = {};
	unsigned int dynamicbinding_i = 0;

	// Main binding
	{
		bindngs[dynamicbinding_i].binding = dynamicbinding_i;
		bindngs[dynamicbinding_i].descriptorCount = desc->ElementCount;
		bindngs[dynamicbinding_i].descriptorType = GetVkEnum(desc->DescriptorType);
		bindngs[dynamicbinding_i].pImmutableSamplers = nullptr;
		bindngs[dynamicbinding_i].stageFlags = VK_SHADER_STAGE_ALL;
	}

	VkDescriptorSetLayout dsl = {};
	{
		VkDescriptorSetLayoutBindingFlagsCreateInfo dynCi = {};
		if (desc->IsDynamic)
		{
			dynCi.bindingCount = 1;
			VkDescriptorBindingFlags flag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
											VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
											VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT |
											VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;
			dynCi.pBindingFlags = &flag;
			dynCi.pNext = nullptr;
			dynCi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		}
		VkDescriptorSetLayoutCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		ci.flags = desc->IsDynamic ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT : 0;
		ci.pNext = desc->IsDynamic ? &dynCi : nullptr;
		ci.bindingCount = dynamicbinding_i + 1;
		ci.pBindings = bindngs;
		if (vkCreateDescriptorSetLayout(gpu->vk_logical, &ci, nullptr, &dsl) != VK_SUCCESS)
		{
			vkPrint(28);
			return nullptr;
		}
	}
	return dsl;
}
TCResult CreateBindingTable(TGfxGpu i_gpu, const TGfxBindingTableDescription const* const desc, TGfxBindingTable* table)
{
#ifndef NDEBUG
	// Check if pool has enough space for the desc set and if there is, then decrease the amount of
	// available descs in the pool for other checks
	if (!desc->ElementCount)
		return vkPrint(29);
#endif
	GPU* gpu = GetVkObject(i_gpu);

	// Create Descriptor Pool
	VkDescriptorPool pool;
	{
		VkDescriptorPoolCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		ci.maxSets = 1;
		ci.pNext = nullptr;
		ci.poolSizeCount = 1;
		VkDescriptorPoolSize size = {};
		size.descriptorCount = desc->ElementCount;
		size.type = GetVkEnum(desc->DescriptorType);
		ci.pPoolSizes = &size;
		ci.flags = desc->IsDynamic ? VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT : 0;
		if (vkCreateDescriptorPool(gpu->vk_logical, &ci, nullptr, &pool) != VK_SUCCESS)
			return vkPrint(28);
	}

	// Create DescriptorSet Layout
	VkDescriptorSetLayout DSL = CreateDescSetLayout(gpu, desc);

	VkDescriptorSet set;
	// Allocate Descriptor Set
	{
		VkDescriptorSetVariableDescriptorCountAllocateInfo descIndexing = {};
		{
			descIndexing.descriptorSetCount = 1;
			descIndexing.pDescriptorCounts = &desc->ElementCount;
			descIndexing.pNext = nullptr;
			descIndexing.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
		}
		VkDescriptorSetAllocateInfo ai = {};
		ai.descriptorPool = pool;
		ai.descriptorSetCount = 1;
		ai.pNext = desc->IsDynamic ? &descIndexing : nullptr;
		ai.pSetLayouts = &DSL;
		ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		if (vkAllocateDescriptorSets(gpu->vk_logical, &ai, &set) != VK_SUCCESS)
		{
			vkDestroyDescriptorPool(gpu->vk_logical, pool, nullptr);
			return vkPrint(31);
		}
	}

	BindingTableInstance* finalobj = GContentManagerContext->BindingTableInsts.CreateObject(gpu);
	finalobj->vk_pool.Set(pool);
	finalobj->vk_set.Set(set);
	finalobj->m_isStatic = !desc->IsDynamic;
	finalobj->vk_layout.Set(DSL);
	finalobj->vk_descType = GetVkEnum(desc->DescriptorType);
	finalobj->vk_stages = VK_SHADER_STAGE_ALL;
	finalobj->m_elementCount = desc->ElementCount;
	switch (finalobj->vk_descType)
	{
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
	*table = GetOpaqueHandle(finalobj);
	return {TC_RESULTSTATE_SUCCESS, 0};
}
void vk_destroyBindingTable(TGfxBindingTable bindingTable)
{
	BindingTableInstance* vkDescSet = GetVkObject(bindingTable);
	assert(vkDescSet && "Invalid binding table");
	vkDestroyDescriptorPool((vkDescSet->GetGpu())->vk_logical, vkDescSet->vk_pool, nullptr);
	GContentManagerContext->BindingTableInsts.DestroyObject(vkDescSet);
}

// Don't call this if there is no binding table & call buffer!
bool VKPipelineLayoutCreation(GPU* GPU,
							  unsigned int descSetCount,
							  const TGfxBindingTableDescription const* const descSets,
							  unsigned char pushConstantOffset,
							  unsigned char pushConstantSize,
							  VkPipelineLayout* layout)
{
	VkPipelineLayoutCreateInfo pl_ci = {};
	pl_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pl_ci.pNext = nullptr;
	pl_ci.flags = 0;

	VkDescriptorSetLayout DESCLAYOUTs[kMaxDescSetPerList] = {};
	for (unsigned int setIdx = 0; setIdx < descSetCount; setIdx++)
		DESCLAYOUTs[setIdx] = CreateDescSetLayout(GPU, &descSets[setIdx]);
	pl_ci.setLayoutCount = descSetCount;
	pl_ci.pSetLayouts = DESCLAYOUTs;
	VkPushConstantRange range = {};
	// Don't support for now!
	if (pushConstantSize)
	{
		pl_ci.pushConstantRangeCount = 1;
		pl_ci.pPushConstantRanges = &range;
		range.offset = pushConstantOffset;
		range.size = pushConstantSize;
		range.stageFlags = VK_SHADER_STAGE_ALL;
	}
	else
	{
		pl_ci.pushConstantRangeCount = 0;
		pl_ci.pPushConstantRanges = nullptr;
	}

	if (vkCreatePipelineLayout(GPU->vk_logical, &pl_ci, nullptr, layout) != VK_SUCCESS)
	{
		vkPrint(16, "Failed at vkCreatePipelineLayout()");
	}

	for (uint32_t i = 0; i < descSetCount; i++)
	{
		vkDestroyDescriptorSetLayout(GPU->vk_logical, DESCLAYOUTs[i], nullptr);
	}

	return true;
}

TCResult CompileShaderSource(TGfxGpu gpu,
							 TGfxShaderLanguage language,
							 TGfxShaderStage shaderstage,
							 const void* DATA,
							 unsigned int DATA_SIZE,
							 TGfxShaderSource* ShaderSourceHnd)
{
	GPU* GPU = GetVkObject(gpu);
	const void* binary_spirv_data = nullptr;
	unsigned int binary_spirv_datasize = 0;
	switch (language)
	{
	case TGFX_SHADERLANGUAGE_SPIRV:
		binary_spirv_data = DATA;
		binary_spirv_datasize = DATA_SIZE;
		break;
	case TGFX_SHADERLANGUAGE_GLSL:
		binary_spirv_data = GLSLang::Compile(shaderstage, DATA, DATA_SIZE, &binary_spirv_datasize);
		if (!binary_spirv_datasize)
		{
			auto res = vkPrint(35, (const char*)binary_spirv_data);
			delete binary_spirv_data;
			return res;
		}
		break;
	default: vkPrint(16, "Backend doesn't support this shading language");
	}

	// Create Vertex Shader Module
	VkShaderModuleCreateInfo ci = {};
	ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ci.flags = 0;
	ci.pNext = nullptr;
	ci.pCode = reinterpret_cast<const uint32_t*>(binary_spirv_data);
	ci.codeSize = static_cast<size_t>(binary_spirv_datasize);

	VkShaderModule Module;
	if (vkCreateShaderModule(GPU->vk_logical, &ci, 0, &Module) != VK_SUCCESS)
	{
		return vkPrint(36);
	}

	ShaderSource* SHADERSOURCE = GContentManagerContext->ShaderSources.CreateObject(GPU);
	SHADERSOURCE->Module.Set(Module);
	SHADERSOURCE->Stage = shaderstage;
	SHADERSOURCE->Gpu = gpu;

	*ShaderSourceHnd = GetOpaqueHandle(SHADERSOURCE);
	return {TC_RESULTSTATE_SUCCESS, 0};
}
void vk_destroyShaderSource(TGfxShaderSource ShaderSourceHnd) {}
VkColorComponentFlags findColorWriteMask(TGfxTextureChannels chnnls);

TCResult vk_createRasterPipeline(const TGfxRasterPipelineDescription* desc, TGfxPipeline* hnd)
{
	GPU* GPU = nullptr;

	// Detect GPU & create shader stage infos
	VkPipelineShaderStageCreateInfo STAGEs[2] = {};
	{
		// Find vertex & fragment shader sources and detect GPU
		ShaderSource *vkSource_vertex = nullptr, *vkSource_fragment = nullptr;
		for (uint32_t shaderIdx = 0; shaderIdx < desc->ShaderCount; shaderIdx++)
		{
			ShaderSource* source = GetVkObject(desc->Shaders[shaderIdx]);
			if (!GPU)
			{
				GPU = GetVkObject(source->Gpu);
			}
			else if (GPU != GetVkObject(source->Gpu))
			{
				vkPrint(37);
				return {TC_RESULTSTATE_FAILURE, 0};
			}
			switch (source->Stage)
			{
			case TGFX_SHADERSTAGE_VERTEXSHADER:
				if (vkSource_vertex)
				{
					return vkPrint(38);
				}
				vkSource_vertex = source;
				break;
			case TGFX_SHADERSTAGE_FRAGMENTSHADER:
				if (vkSource_fragment)
				{
					return vkPrint(38);
				}
				vkSource_fragment = source;
				break;
			default: return vkPrint(39);
			}
		}

		VkPipelineShaderStageCreateInfo& vertexStage_ci = STAGEs[0];
		VkPipelineShaderStageCreateInfo& fragmentStage_ci = STAGEs[1];
		vertexStage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexStage_ci.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexStage_ci.module = vkSource_vertex->Module;
		vertexStage_ci.pName = "main";

		fragmentStage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentStage_ci.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentStage_ci.module = vkSource_fragment->Module;
		fragmentStage_ci.pName = "main";
	}

	VkPipelineVertexInputStateCreateInfo vertexInputState = {};
	VkVertexInputAttributeDescription attribs[VKCONST_MAXVERTEXATTRIBCOUNT] = {};
	VkVertexInputBindingDescription bindings[VKCONST_MAXVERTEXBINDINGCOUNT] = {};
	{
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		if (desc->AttributeLayout.AttribCount > VKCONST_MAXVERTEXATTRIBCOUNT ||
			desc->AttributeLayout.AttribCount > GPU->vk_propsDev.properties.limits.maxVertexInputAttributes)
		{
			return vkPrint(40);
		}
		if (desc->AttributeLayout.AttribCount > VKCONST_MAXVERTEXBINDINGCOUNT ||
			desc->AttributeLayout.AttribCount > GPU->vk_propsDev.properties.limits.maxVertexInputBindings)
		{
			return vkPrint(40);
		}

		for (uint32_t i = 0; i < desc->AttributeLayout.AttribCount; i++)
		{
			const auto& attr = desc->AttributeLayout.Attributes[i];
			uint32_t attrIdx = attr.AttributeIdx;
			if (attrIdx >= desc->AttributeLayout.AttribCount)
			{
				return vkPrint(41);
			}
			if (attr.Offset > GPU->vk_propsDev.properties.limits.maxVertexInputAttributeOffset)
			{
				return vkPrint(42);
			}

			attribs[attrIdx].binding = attr.BindingIdx;
			attribs[attrIdx].offset = attr.Offset;
			attribs[attrIdx].location = attrIdx;
			attribs[attrIdx].format = GetVkFormatFromTGfxDataType(attr.DataType);
		}

		for (uint32_t i = 0; i < desc->AttributeLayout.BindingCount; i++)
		{
			const auto& binding = desc->AttributeLayout.Bindings[i];
			uint32_t bindingIdx = binding.BindingIdx;
			if (bindingIdx >= desc->AttributeLayout.BindingCount)
				return vkPrint(41);
			if (binding.Stride > GPU->vk_propsDev.properties.limits.maxVertexInputBindingStride)
				return vkPrint(42);

			bindings[bindingIdx].binding = bindingIdx;
			bindings[bindingIdx].stride = binding.Stride;
			bindings[bindingIdx].inputRate = GetVkEnum(binding.InputRate);
		}

		vertexInputState.pVertexAttributeDescriptions = attribs;
		vertexInputState.vertexAttributeDescriptionCount = desc->AttributeLayout.AttribCount;
		vertexInputState.pVertexBindingDescriptions = bindings;
		vertexInputState.vertexBindingDescriptionCount = desc->AttributeLayout.BindingCount;
	}

	VkPipelineInputAssemblyStateCreateInfo IAState = {};
	{
		IAState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		IAState.topology = GetVkEnum(desc->MainStates->Topology);
		IAState.primitiveRestartEnable = false;
		IAState.flags = 0;
		IAState.pNext = nullptr;
	}

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	{
		VkRect2D vk_scissors[kMaxViewportCount];
		VkViewport vk_viewports[kMaxViewportCount];
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
	}
	VkPipelineRasterizationStateCreateInfo rasterState = {};
	{
		rasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterState.polygonMode = GetVkEnum(desc->MainStates->PolygonMode);
		rasterState.cullMode = GetVkEnum(desc->MainStates->Culling);
		rasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterState.lineWidth = 1.0f;
		rasterState.depthClampEnable = VK_FALSE;
		rasterState.rasterizerDiscardEnable = VK_FALSE;

		rasterState.depthBiasEnable = VK_FALSE;
		rasterState.depthBiasClamp = 0.0f;
		rasterState.depthBiasConstantFactor = 0.0f;
		rasterState.depthBiasSlopeFactor = 0.0f;
	}

	// MSAA isn't supported for now
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

	// Blending isn't supported for now
	VkPipelineColorBlendStateCreateInfo blendState = {};
	VkPipelineColorBlendAttachmentState states[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT] = {};
	{
		blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		while (blendState.attachmentCount <= TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT &&
			   desc->ColorTextureFormats[blendState.attachmentCount] != TGFX_TEXTURE_CHANNELS_UNDEF &&
			   desc->ColorTextureFormats[blendState.attachmentCount] != TGFX_TEXTURE_CHANNELS_UNDEF2)
		{
			blendState.attachmentCount++;
		}
		for (uint32_t i = 0; i < TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT; i++)
		{
			const TGfxBlendState& blendState = desc->MainStates->BlendStates[i];
			states[i].blendEnable = blendState.IsBlendEnabled;
			states[i].colorWriteMask = GetVkEnum(blendState.BlendComponents, desc->ColorTextureFormats[i]);
			states[i].alphaBlendOp = GetVkEnum(blendState.AlphaMode);
			states[i].colorBlendOp = GetVkEnum(blendState.ColorMode);
			states[i].dstAlphaBlendFactor = GetVkEnum(blendState.DstAlphaFactor);
			states[i].dstColorBlendFactor = GetVkEnum(blendState.DstColorFactor);
			states[i].srcAlphaBlendFactor = GetVkEnum(blendState.SrcAlphaFactor);
			states[i].srcColorBlendFactor = GetVkEnum(blendState.SrcColorFactor);
		}
		blendState.pAttachments = states;
		blendState.logicOpEnable = VK_FALSE;
		blendState.logicOp = VK_LOGIC_OP_COPY;
	}

	VkPipelineDynamicStateCreateInfo dynamicStates = {};
	VkDynamicState dynamicStatesList[64];
	uint32_t dynamicStatesCount = 0;
	{
		dynamicStatesList[dynamicStatesCount++] = VK_DYNAMIC_STATE_VIEWPORT;
		dynamicStatesList[dynamicStatesCount++] = VK_DYNAMIC_STATE_SCISSOR;
		dynamicStatesList[dynamicStatesCount++] = VK_DYNAMIC_STATE_DEPTH_BOUNDS;

		dynamicStates.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStates.dynamicStateCount = dynamicStatesCount;
		dynamicStates.pDynamicStates = dynamicStatesList;
	}

	VkPipelineLayout layout = {};
	if (!VKPipelineLayoutCreation(
			GPU, desc->TableCount, desc->Tables, desc->PushConstantOffset, desc->PushConstantSize, &layout))
	{
		return vkPrint(43, "raster pipeline at VKPipelineLayoutCreation()");
	}

	VkPipelineDepthStencilStateCreateInfo depthState = {};
	{
		depthState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthState.flags = 0;
		depthState.pNext = nullptr;
		const auto& state = desc->MainStates->DepthStencilState;

		// Depth states
		{
			depthState.depthTestEnable = state.IsDepthTestEnabled;
			depthState.depthCompareOp = GetVkEnum(state.DepthCompare);
			depthState.depthWriteEnable = state.IsDepthWriteEnabled;
			depthState.depthBoundsTestEnable = state.IsDepthTestEnabled;
			depthState.maxDepthBounds = 1.0;
			depthState.minDepthBounds = 0.0;
		}

		// Stencil States
		{
			depthState.stencilTestEnable = state.IsStencilTestEnabled;
			for (uint32_t i = 0; i < 2; i++)
			{
				VkStencilOpState& vkState = (i == 0) ? depthState.front : depthState.back;
				const auto& tgfxState = (i == 0) ? state.Front : state.Back;
				vkState.compareMask = tgfxState.CompareMask;
				vkState.compareOp = GetVkEnum(tgfxState.CompareOp);
				vkState.depthFailOp = GetVkEnum(tgfxState.StencilDepthFail);
				vkState.failOp = GetVkEnum(tgfxState.StencilFail);
				vkState.passOp = GetVkEnum(tgfxState.StencilPass);
				vkState.reference = tgfxState.Reference;
				vkState.writeMask = tgfxState.WriteMask;
			}
		}
	}

	VkPipeline pipeline;
	{
		VkPipelineRenderingCreateInfoKHR dynCi = {};
		VkGraphicsPipelineCreateInfo ci = {};
		dynCi.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		VkFormat VKCOLORATTACHMENTFORMATS[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT] = {};
		while (dynCi.colorAttachmentCount <= TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT &&
			   desc->ColorTextureFormats[dynCi.colorAttachmentCount] != TGFX_TEXTURE_CHANNELS_UNDEF &&
			   desc->ColorTextureFormats[blendState.attachmentCount] != TGFX_TEXTURE_CHANNELS_UNDEF2)
		{
			VKCOLORATTACHMENTFORMATS[dynCi.colorAttachmentCount] =
				GetVkEnum(desc->ColorTextureFormats[dynCi.colorAttachmentCount]);
			dynCi.colorAttachmentCount++;
		}
		dynCi.pColorAttachmentFormats = VKCOLORATTACHMENTFORMATS;
		dynCi.viewMask = 0;
		dynCi.pNext = {};

		ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		ci.pColorBlendState = &blendState;
		if (desc->DepthStencilTextureFormat != TGFX_TEXTURE_CHANNELS_UNDEF &&
			desc->DepthStencilTextureFormat != TGFX_TEXTURE_CHANNELS_UNDEF2)
		{
			dynCi.depthAttachmentFormat = GetVkEnum(desc->DepthStencilTextureFormat);
			dynCi.stencilAttachmentFormat = GetVkEnum(desc->DepthStencilTextureFormat);
		}
		else
		{
			ci.pDepthStencilState = nullptr;
		}
		ci.pDynamicState = &dynamicStates;
		ci.pInputAssemblyState = &IAState;
		ci.pMultisampleState = &MSAAState;
		ci.pRasterizationState = &rasterState;
		ci.pVertexInputState = &vertexInputState;
		ci.pViewportState = &viewportState;
		ci.pDepthStencilState = &depthState;
		ci.layout = layout;
		ci.stageCount = 2;
		ci.pStages = STAGEs;
		ci.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
		ci.pNext = &dynCi;
		if (vkCreateGraphicsPipelines(GPU->vk_logical, nullptr, 1, &ci, nullptr, &pipeline) != VK_SUCCESS)
		{
			vkPrint(43, "at vkCreateGraphicsPipelines()");
		}
	}

	Pipeline* pipelineObj = GContentManagerContext->Pipelines.CreateObject(GPU);
	pipelineObj->vk_layout.Set(layout);
	pipelineObj->vk_object.Set(pipeline);
	pipelineObj->vk_type = VK_PIPELINE_BIND_POINT_GRAPHICS;
	for (uint32_t i = 0; i < TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT; i++)
	{
		pipelineObj->vk_colorAttachmentFormats[i] = GetVkEnum(desc->ColorTextureFormats[i]);
	}
	pipelineObj->vk_depthAttachmentFormat = GetVkEnum(desc->DepthStencilTextureFormat);
	*hnd = GetOpaqueHandle(pipelineObj);
	return {TC_RESULTSTATE_SUCCESS, 0};
}
/*
TCResult vk_copyRasterPipeline(rasterPipeline_tgfxhnd src, extension_tgfxlsthnd exts,
								  rasterPipeline_tgfxhnd* dst) {
  RASTERPIPELINE_VKOBJ* srcPipeline = getOBJ<PIPELINE_VKOBJ>(src);
#ifdef VULKAN_DEBUGGING
  if (!srcPipeline) {
	printer(TC_RESULTSTATE_FAILURE,
			"vk_copyRasterPipeline() has failed because source raster pipeline isn't found!");
	return {TC_RESULTSTATE_INVALID_ARGUMENT, 0};
  }
#endif

  // Descriptor Set Creation
  RASTERPIPELINE_VKOBJ* dstPipeline = hidden->pipelines.CreateObject(gpu);

  *dst = hidden->pipelines.returnHANDLEfromOBJ(dstPipeline);
  return {TC_RESULTSTATE_SUCCESS,0};
}
*/
TCResult vk_createComputePipeline(TGfxShaderSource Source,
								  unsigned int bindingTableCount,
								  const TGfxBindingTableDescription const* const bindingTableDescs,
								  unsigned char pushConstantOffset,
								  unsigned char pushConstantSize,
								  TGfxPipeline* hnd)
{
	VkComputePipelineCreateInfo cp_ci = {};
	ShaderSource* SHADER = GetVkObject(Source);
	GPU* GPU = GetVkObject(SHADER->Gpu);

	VkPipelineLayout layout = {};
	if (!VKPipelineLayoutCreation(
			GPU, bindingTableCount, bindingTableDescs, pushConstantOffset, pushConstantSize, &layout))
	{
		return vkPrint(43, "compute pipeline at VKPipelineLayoutCreation()!");
	}

	// VkPipeline creation
	VkPipeline pipelineObj = {};
	{
		cp_ci.stage.flags = 0;
		cp_ci.stage.module = SHADER->Module;
		cp_ci.stage.pName = "main";
		cp_ci.stage.pNext = nullptr;
		cp_ci.stage.pSpecializationInfo = nullptr;
		cp_ci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		cp_ci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		cp_ci.basePipelineHandle = VK_NULL_HANDLE;
		cp_ci.basePipelineIndex = -1;
		cp_ci.flags = 0;
		cp_ci.layout = layout;
		cp_ci.pNext = nullptr;
		cp_ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		if (vkCreateComputePipelines(GPU->vk_logical, VK_NULL_HANDLE, 1, &cp_ci, nullptr, &pipelineObj) != VK_SUCCESS)
		{
			return vkPrint(43, "at vkCreateComputePipelines()");
		}
	}

	Pipeline* obj = GContentManagerContext->Pipelines.CreateObject(GPU);
	obj->vk_layout.Set(layout);
	obj->vk_object.Set(pipelineObj);
	obj->vk_type = VK_PIPELINE_BIND_POINT_COMPUTE;
	*hnd = GetOpaqueHandle(obj);
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult vk_copyComputePipeline(TGfxPipeline src, TGfxPipeline dst, TGfxExtension* exts)
{
	return {TC_RESULTSTATE_UNIMPLEMENTED, 0};
}
void vk_destroyPipeline(TGfxPipeline pipe)
{
	Pipeline* vkPipe = GetVkObject(pipe);
	assert(vkPipe && "Invalid pipeline!");
	vkDestroyPipelineLayout((vkPipe->GetGpu())->vk_logical, vkPipe->vk_layout, nullptr);
	vkPipe->vk_layout.SetAsDead();
	vkDestroyPipeline((vkPipe->GetGpu())->vk_logical, vkPipe->vk_object, nullptr);
	vkPipe->vk_object.SetAsDead();
	GContentManagerContext->Pipelines.DestroyObject(vkPipe);
}

static constexpr uint32_t VKCONST_MAXDESCCHANGE_PERCALL = 1024;
static constexpr const char* VKCONST_MAXDESCCHANGE_TEXT =
	"Exceeded max number of changable binding, please report this";
// Set a descriptor of the binding table created with shaderdescriptortype_tgfx_SAMPLER
TCResult vk_setBindingTable_Sampler(TGfxBindingTable bindingtable,
									unsigned int bindingCount,
									const unsigned int* bindingIndices,
									TGfxSampler const* samplerHandles)
{
	BindingTableInstance* set = GetVkObject(bindingtable);
	static constexpr const char* funcName = "at setBindingTable_sampler()";
	if (!set)
	{
		return vkPrint(11, funcName);
	}
	if (bindingCount > VKCONST_MAXDESCCHANGE_PERCALL)
	{
		return vkPrint(16, VKCONST_MAXDESCCHANGE_TEXT);
	}

	VkWriteDescriptorSet writeInfos[VKCONST_MAXDESCCHANGE_PERCALL] = {};
	VkDescriptorImageInfo imageInfos[VKCONST_MAXDESCCHANGE_PERCALL] = {};
	for (uint32_t bindingIter = 0; bindingIter < bindingCount; bindingIter++)
	{
		Sampler* sampler = GetVkObject(samplerHandles[bindingIter]);
		uint32_t elementIdx = bindingIndices[bindingIter];
		if (!set || !sampler || bindingIndices[bindingIter] >= set->m_elementCount ||
			set->vk_descType != VK_DESCRIPTOR_TYPE_SAMPLER)
		{
			return vkPrint(11, funcName);
		}
		sampler_descVK& samplerDESC = ((sampler_descVK*)set->m_descs)[elementIdx];

		samplerDESC.sampler_obj = sampler->vk_sampler;
		imageInfos[bindingIter].sampler = sampler->vk_sampler;

		writeInfos[bindingIter].descriptorCount = 1;
		writeInfos[bindingIter].descriptorType = set->vk_descType;
		writeInfos[bindingIter].dstArrayElement = elementIdx;
		writeInfos[bindingIter].dstBinding = 0;
		writeInfos[bindingIter].pTexelBufferView = nullptr;
		writeInfos[bindingIter].pNext = nullptr;
		writeInfos[bindingIter].dstSet = set->vk_set;
		writeInfos[bindingIter].pImageInfo = &imageInfos[bindingIter];
		writeInfos[bindingIter].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	}
	vkUpdateDescriptorSets((set->GetGpu())->vk_logical, bindingCount, writeInfos, 0, nullptr);
	return {TC_RESULTSTATE_SUCCESS, 0};
}

// Set a descriptor of the binding table created with shaderdescriptortype_tgfx_BUFFER
TCResult vk_setBindingTable_Buffer(TGfxBindingTable table,
								   TU4 bindingCount,
								   const TU4* bindingIndices,
								   TGfxBuffer const* buffers,
								   const TU8* offsets,
								   const TU8* sizes,
								   TGfxExtension* exts)
{
	BindingTableInstance* set = GetVkObject(table);
	static constexpr const char* funcName = "at setBindingTable_buffer()";
	if (!set)
	{
		return vkPrint(11, funcName);
	}
	if (set->vk_descType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER && set->vk_descType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
	{
		return vkPrint(11, funcName);
	}
	if (bindingCount > VKCONST_MAXDESCCHANGE_PERCALL)
	{
		return vkPrint(16, VKCONST_MAXDESCCHANGE_TEXT);
	}

	VkWriteDescriptorSet writeInfos[VKCONST_MAXDESCCHANGE_PERCALL] = {};
	for (uint32_t bindingIter = 0; bindingIter < bindingCount; bindingIter++)
	{
		Buffer* buffer = GetVkObject(buffers[bindingIter]);
		uint32_t elementIdx = bindingIndices[bindingIter];
		buffer_descVK& bufferDESC = ((buffer_descVK*)set->m_descs)[elementIdx];

		if (!buffer || elementIdx >= set->m_elementCount)
		{
			return vkPrint(11, funcName);
		}

		bufferDESC.info.buffer = buffer->vk_buffer;
		bufferDESC.info.offset = (offsets == nullptr) ? (0) : (offsets[bindingIter]);
		bufferDESC.info.range = (sizes == nullptr) ? (buffer->m_intendedSize) : (sizes[bindingIter]);

		writeInfos[bindingIter].descriptorCount = 1;
		writeInfos[bindingIter].descriptorType = set->vk_descType;
		writeInfos[bindingIter].dstArrayElement = elementIdx;
		writeInfos[bindingIter].dstBinding = 0;
		writeInfos[bindingIter].pTexelBufferView = nullptr;
		writeInfos[bindingIter].pNext = nullptr;
		writeInfos[bindingIter].dstSet = set->vk_set;
		writeInfos[bindingIter].pBufferInfo = &bufferDESC.info;
		writeInfos[bindingIter].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	}
	uint64_t duration = 0;
	vkUpdateDescriptorSets((set->GetGpu())->vk_logical, bindingCount, writeInfos, 0, nullptr);

	return {TC_RESULTSTATE_SUCCESS, 0};
}
TCResult vk_setBindingTable_Texture(TGfxBindingTable table,
									unsigned int bindingCount,
									const unsigned int* bindingIndices,
									TGfxTexture const* textures)
{
	BindingTableInstance* set = GetVkObject(table);
	static constexpr const char* funcName = "at setBindingTable_texture()";
	if (!set)
		return vkPrint(11, funcName);
	if (set->vk_descType != VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE && set->vk_descType != VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		return vkPrint(11, funcName);
	if (bindingCount > VKCONST_MAXDESCCHANGE_PERCALL)
		return vkPrint(16, VKCONST_MAXDESCCHANGE_TEXT);

	VkWriteDescriptorSet writeInfos[VKCONST_MAXDESCCHANGE_PERCALL] = {};
	for (uint32_t bindingIter = 0; bindingIter < bindingCount; bindingIter++)
	{
		Texture* texture = GetVkObject(textures[bindingIter]);
		uint32_t elementIdx = bindingIndices[bindingIter];
		texture_descVK& textureDESC = ((texture_descVK*)set->m_descs)[elementIdx];
		if (!texture || elementIdx >= set->m_elementCount)
			return vkPrint(21, funcName);

		textureDESC.info.imageLayout = (set->vk_descType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
										   ? VK_IMAGE_LAYOUT_GENERAL
										   : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		textureDESC.info.imageView = texture->vk_imageView;
		textureDESC.info.sampler = VK_NULL_HANDLE;

		writeInfos[bindingIter].descriptorCount = 1;
		writeInfos[bindingIter].descriptorType = set->vk_descType;
		writeInfos[bindingIter].dstArrayElement = elementIdx;
		writeInfos[bindingIter].dstBinding = 0;
		writeInfos[bindingIter].pTexelBufferView = nullptr;
		writeInfos[bindingIter].pNext = nullptr;
		writeInfos[bindingIter].dstSet = set->vk_set;
		writeInfos[bindingIter].pImageInfo = &textureDESC.info;
		writeInfos[bindingIter].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	}
	vkUpdateDescriptorSets((set->GetGpu())->vk_logical, bindingCount, writeInfos, 0, nullptr);
	return {TC_RESULTSTATE_SUCCESS, 0};
}

/////////////////////////////////////////////////////
//								MEMORY
/////////////////////////////////////////////////////

static constexpr const char* VKCONST_HEAP_EXTENSIONS_NOT_SUPPORTED_TEXT =
	"Extensions're not supported in heap functions for now";
TCResult vk_createHeap(TGfxGpu gpu, TU1 memoryRegionID, TU8 heapSize, TGfxExtension* exts, TGfxHeap* heapHnd)
{
	if (exts)
	{
		return vkPrint(16, VKCONST_HEAP_EXTENSIONS_NOT_SUPPORTED_TEXT);
	}
	VkMemoryAllocateInfo memAlloc;
	memAlloc.allocationSize = heapSize;
	memAlloc.memoryTypeIndex = memoryRegionID;
	memAlloc.pNext = nullptr;
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	GPU* vkGPU = GetVkObject(gpu);
	VkDeviceMemory vkDevMem;
	if (vkAllocateMemory(vkGPU->vk_logical, &memAlloc, nullptr, &vkDevMem) != VK_SUCCESS)
	{
		return vkPrint(44, "at vkAllocateMemory()");
	}

	Heap* heap = GContentManagerContext->Heaps.CreateObject(vkGPU);
	heap->vk_memoryHnd.Set(vkDevMem);
	heap->vk_memoryTypeIndex = memoryRegionID;
	heap->m_size = heapSize;
	*heapHnd = GetOpaqueHandle(heap);
	return {TC_RESULTSTATE_SUCCESS, 0};
}
TCResult vk_getHeapRequirement_Texture(TGfxTexture i_texture, TGfxExtension* exts, TGfxHeapRequirementsInfo* reqs)
{
	if (exts)
	{
		return vkPrint(16, VKCONST_HEAP_EXTENSIONS_NOT_SUPPORTED_TEXT);
	}
	Texture* texture = GetVkObject(i_texture);
	for (uint8_t memTypeIdx = 0; memTypeIdx < 32; memTypeIdx++)
	{
		if (texture->m_memReqs.vk_memReqs.memoryTypeBits & (1u << memTypeIdx))
			reqs->MemoryRegionIds[memTypeIdx] = true;
		else
			reqs->MemoryRegionIds[memTypeIdx] = false;
	}
	reqs->OffsetAlignment = texture->m_memReqs.vk_memReqs.alignment;
	reqs->Size = texture->m_memReqs.vk_memReqs.size;
	return {TC_RESULTSTATE_SUCCESS, 0};
}
TCResult vk_getHeapRequirement_Buffer(TGfxBuffer i_buffer, TGfxExtension* exts, TGfxHeapRequirementsInfo* reqs)
{
	if (exts)
		return vkPrint(16, VKCONST_HEAP_EXTENSIONS_NOT_SUPPORTED_TEXT);

	Buffer* buf = GetVkObject(i_buffer);

	for (uint8_t memTypeIdx = 0; memTypeIdx < 32; memTypeIdx++)
	{
		if (buf->m_memReqs.vk_memReqs.memoryTypeBits & (1u << memTypeIdx))
			reqs->MemoryRegionIds[memTypeIdx] = true;
		else
			reqs->MemoryRegionIds[memTypeIdx] = false;
	}
	reqs->OffsetAlignment = buf->m_memReqs.vk_memReqs.alignment;
	reqs->Size = buf->m_memReqs.vk_memReqs.size;
	return {TC_RESULTSTATE_SUCCESS, 0};
}
// @return FAIL if this feature isn't supported
TCResult vk_getRemainingMemory(TGfxGpu GPU, unsigned char memoryRegionID, TGfxExtension* exts, TU8* size)
{
	return {TC_RESULTSTATE_UNIMPLEMENTED, 0};
}

TCResult vk_bindToHeap_Buffer(TGfxHeap i_heap, TU8 offset, TGfxBuffer i_buffer, TGfxExtension* exts)
{
	if (exts)
		return vkPrint(16, VKCONST_HEAP_EXTENSIONS_NOT_SUPPORTED_TEXT);

	Buffer* buffer = GetVkObject(i_buffer);
	Heap* heap = GetVkObject(i_heap);
	GPU* gpu = (buffer->GetGpu());
	if (buffer->m_memReqs.requiresDedicatedAlloc)
		return vkPrint(45);
	if (offset % buffer->m_memReqs.vk_memReqs.alignment)
		return vkPrint(46);
	if (vkBindBufferMemory(gpu->vk_logical, buffer->vk_buffer, heap->vk_memoryHnd, offset) != VK_SUCCESS)
		return vkPrint(47, "at vkBindBufferMemory()");
	return {TC_RESULTSTATE_SUCCESS, 0};
}
TCResult vk_bindToHeap_Texture(TGfxHeap i_heap, TU8 offset, TGfxTexture i_texture, TGfxExtension* exts)
{
	Texture* texture = GetVkObject(i_texture);
	Heap* heap = GetVkObject(i_heap);
	GPU* gpu = (texture->GetGpu());
	if (texture->m_memReqs.requiresDedicatedAlloc)
		return vkPrint(45);
	if (offset % texture->m_memReqs.vk_memReqs.alignment)
		return vkPrint(46);
	if (vkBindImageMemory(gpu->vk_logical, texture->vk_image, heap->vk_memoryHnd, offset) != VK_SUCCESS)
		vkPrint(47, "at vkBindImageMemory()");

	if (texture->vk_imageView)
		vkDestroyImageView(gpu->vk_logical, texture->vk_imageView, nullptr);

	// Create VkImageView
	VkImageView VkTextureViewObj;
	{
		VkImageViewCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.flags = 0;
		ci.pNext = nullptr;

		ci.image = texture->vk_image;
		if (texture->m_dim == TGFX_TEXTURE_DIMENSIONS_2DCUBE)
		{
			ci.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
			ci.subresourceRange.layerCount = 6;
		}
		else
		{
			ci.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
			ci.subresourceRange.layerCount = 1;
		}
		ci.subresourceRange.baseArrayLayer = 0;
		ci.subresourceRange.baseMipLevel = 0;
		ci.subresourceRange.levelCount = 1;
		ci.format = GetVkEnum(texture->m_channels);
		if (texture->m_channels == TGFX_TEXTURE_CHANNELS_D32)
		{
			ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if (texture->m_channels == TGFX_TEXTURE_CHANNELS_D24S8)
		{
			ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else
		{
			ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		Fill_ComponentMapping_byCHANNELs(texture->m_channels, ci.components);
		VkImageView v;
		if (vkCreateImageView(gpu->vk_logical, &ci, nullptr, &v) != VK_SUCCESS)
			return vkPrint(47, "at vkCreateImageView()");

		texture->vk_imageView.Set(v);
	}

	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult MapHeap(TGfxHeap i_heap, TU8 offset, TU8 size, TGfxExtension* exts, void** mappedRegion)
{
	Heap* heap = GetVkObject(i_heap);
	GPU* gpu = (heap->GetGpu());
	if (vkMapMemory(gpu->vk_logical, heap->vk_memoryHnd, offset, size, 0, mappedRegion) != VK_SUCCESS)
	{
		return vkPrint(48);
	}
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult vk_unmapHeap(TGfxHeap i_heap)
{
	Heap* heap = GetVkObject(i_heap);
	GPU* gpu = (heap->GetGpu());
	vkUnmapMemory(gpu->vk_logical, heap->vk_memoryHnd);
	return {TC_RESULTSTATE_SUCCESS, 0};
}

/////////////////////////////////////////////////////
///				INITIALIZATION PROCEDURE
/////////////////////////////////////////////////////

void ContentManagerContext::HookContentManager(ITGfxResourceManager* r)
{
	r->CompileShaderSource = CompileShaderSource;
	r->CreateBindingTable = CreateBindingTable;
	r->CopyComputePipeline = vk_copyComputePipeline;
	r->CreateComputePipeline = vk_createComputePipeline;
	r->CreateBuffer = vk_createBuffer;
	r->CreateTexture = vk_createTexture;
	r->DestroyShaderSource = vk_destroyShaderSource;
	r->DestroyAllResources = destroyAllResources;
	r->CreateRasterPipeline = vk_createRasterPipeline;
	r->SetBindingTable_Buffer = vk_setBindingTable_Buffer;
	r->SetBindingTable_Texture = vk_setBindingTable_Texture;
	r->CreateHeap = vk_createHeap;
	r->GetHeapRequirement_Buffer = vk_getHeapRequirement_Buffer;
	r->GetHeapRequirement_Texture = vk_getHeapRequirement_Texture;
	r->GetRemainingMemory = vk_getRemainingMemory;
	r->BindToHeap_Buffer = vk_bindToHeap_Buffer;
	r->BindToHeap_Texture = vk_bindToHeap_Texture;
	r->MapHeap = MapHeap;
	r->UnmapHeap = vk_unmapHeap;
	r->CreateSampler = createSampler;
	r->SetBindingTable_Sampler = vk_setBindingTable_Sampler;
	r->DestroyBuffer = vk_destroyBuffer;
	r->DestroyTexture = vk_destroyTexture;
	r->DestroyShaderSource = vk_destroyShaderSource;
	r->DestroySampler = vk_destroySampler;
	r->DestroyBindingTable = vk_destroyBindingTable;
	r->DestroyPipeline = vk_destroyPipeline;
}

TCResult ContentManagerContext::Initialize()
{
	GContentManagerContext = new ContentManagerContext;
	GLSLang::Initialize();
	return {TC_RESULTSTATE_SUCCESS, 0};
}

} // namespace Vulkan
} // namespace TGFX