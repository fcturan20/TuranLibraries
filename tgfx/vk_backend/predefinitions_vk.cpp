#include "predefinitions_vk.h"
#include "resource.h"

core_tgfx* core_tgfx_main = nullptr;
core_public* core_vk = nullptr;
renderer_public* renderer = nullptr;
gpudatamanager_public* contentmanager = nullptr;
IMGUI_VK* imgui = nullptr;
gpu_public* rendergpu = nullptr;
threadingsys_tapi* threadingsys = nullptr;
semaphoresys_vk* semaphoresys = nullptr;
fencesys_vk* fencesys = nullptr;
allocatorsys_vk* allocatorsys = nullptr;
queuesys_vk* queuesys = nullptr;

VkInstance Vulkan_Instance = VK_NULL_HANDLE;
VkApplicationInfo Application_Info;

tgfx_PrintLogCallback printer = nullptr;



VkFormat Find_VkFormat_byDataType(datatype_tgfx datatype) {
	switch (datatype) {
	case datatype_tgfx_VAR_VEC2:
		return VK_FORMAT_R32G32_SFLOAT;
	case datatype_tgfx_VAR_VEC3:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case datatype_tgfx_VAR_VEC4:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	default:
		printer(result_tgfx_FAIL, "(Find_VkFormat_byDataType() doesn't support this data type! UNDEFINED");
		return VK_FORMAT_UNDEFINED;
	}
}
VkFormat Find_VkFormat_byTEXTURECHANNELs(texture_channels_tgfx channels) {
	switch (channels) {
	case texture_channels_tgfx_BGRA8UNORM:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case texture_channels_tgfx_BGRA8UB:
		return VK_FORMAT_B8G8R8A8_UINT;
	case texture_channels_tgfx_RGBA8UB:
		return VK_FORMAT_R8G8B8A8_UINT;
	case texture_channels_tgfx_RGBA8B:
		return VK_FORMAT_R8G8B8A8_SINT;
	case texture_channels_tgfx_RGBA32F:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case texture_channels_tgfx_RGB8UB:
		return VK_FORMAT_R8G8B8_UINT;
	case texture_channels_tgfx_D32:
		return VK_FORMAT_D32_SFLOAT;
	case texture_channels_tgfx_D24S8:
		return VK_FORMAT_D24_UNORM_S8_UINT;
	default:
		printer(result_tgfx_FAIL, "(Find_VkFormat_byTEXTURECHANNELs doesn't support this type of channel!");
		return VK_FORMAT_UNDEFINED;
	}
}
VkDescriptorType Find_VkDescType_byMATDATATYPE(shaderinputtype_tgfx TYPE) {
	switch (TYPE) {
	case shaderinputtype_tgfx_UBUFFER_G:
	case shaderinputtype_tgfx_UBUFFER_PI:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case shaderinputtype_tgfx_SBUFFER_G:
	case shaderinputtype_tgfx_SBUFFER_PI:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case shaderinputtype_tgfx_SAMPLER_G:
	case shaderinputtype_tgfx_SAMPLER_PI:
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case shaderinputtype_tgfx_IMAGE_G:
	case shaderinputtype_tgfx_IMAGE_PI:
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case shaderinputtype_tgfx_UNDEFINED:
		printer(result_tgfx_FAIL, "(Find_VkDescType_byMATDATATYPE() has failed because SHADERINPUT_TYPE = UNDEFINED!");
	default:
		printer(result_tgfx_FAIL, "(Find_VkDescType_byMATDATATYPE() doesn't support this type of SHADERINPUT_TYPE!");
	}
	return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}
VkDescriptorType Find_VkDescType_byDescTypeCategoryless(desctype_vk type) {
	switch (type)
	{
	case desctype_vk::IMAGE:
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case desctype_vk::SAMPLER:
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case desctype_vk::UBUFFER:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case desctype_vk::SBUFFER:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	default:
		printer(result_tgfx_FAIL, "(Find_VkDescType_byDescTypeCategoryless() doesn't support this type!");
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

VkSamplerAddressMode Find_AddressMode_byWRAPPING(texture_wrapping_tgfx Wrapping) {
	switch (Wrapping) {
	case texture_wrapping_tgfx_REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case texture_wrapping_tgfx_MIRRORED_REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case texture_wrapping_tgfx_CLAMP_TO_EDGE:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	default:
		printer(result_tgfx_INVALIDARGUMENT, "Find_AddressMode_byWRAPPING() doesn't support this wrapping type!");
		return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
	}
}

VkFilter Find_VkFilter_byGFXFilter(texture_mipmapfilter_tgfx filter) {
	switch (filter) {
	case texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP:
	case texture_mipmapfilter_tgfx_LINEAR_FROM_2MIP:
		return VK_FILTER_LINEAR;
	case texture_mipmapfilter_tgfx_NEAREST_FROM_1MIP:
	case texture_mipmapfilter_tgfx_NEAREST_FROM_2MIP:
		return VK_FILTER_NEAREST;
	default:
		printer(result_tgfx_INVALIDARGUMENT, "Find_VkFilter_byGFXFilter() doesn't support this filter type!");
		return VK_FILTER_MAX_ENUM;
	}
}
VkSamplerMipmapMode Find_MipmapMode_byGFXFilter(texture_mipmapfilter_tgfx filter) {
	switch (filter) {
	case texture_mipmapfilter_tgfx_LINEAR_FROM_2MIP:
	case texture_mipmapfilter_tgfx_NEAREST_FROM_2MIP:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	case texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP:
	case texture_mipmapfilter_tgfx_NEAREST_FROM_1MIP:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}
}

VkCullModeFlags Find_CullMode_byGFXCullMode(cullmode_tgfx mode) {
	switch (mode)
	{
	case cullmode_tgfx_OFF:
		return VK_CULL_MODE_NONE;
		break;
	case cullmode_tgfx_BACK:
		return VK_CULL_MODE_BACK_BIT;
		break;
	case cullmode_tgfx_FRONT:
		return VK_CULL_MODE_FRONT_BIT;
		break;
	default:
		printer(result_tgfx_INVALIDARGUMENT, "This culling type isn't supported by Find_CullMode_byGFXCullMode()!");
		return VK_CULL_MODE_NONE;
		break;
	}
}
VkPolygonMode Find_PolygonMode_byGFXPolygonMode(polygonmode_tgfx mode) {
	switch (mode)
	{
	case polygonmode_tgfx_FILL:
		return VK_POLYGON_MODE_FILL;
		break;
	case polygonmode_tgfx_LINE:
		return VK_POLYGON_MODE_LINE;
		break;
	case polygonmode_tgfx_POINT:
		return VK_POLYGON_MODE_POINT;
		break;
	default:
		printer(result_tgfx_INVALIDARGUMENT, "This polygon mode isn't support by Find_PolygonMode_byGFXPolygonMode()");
		break;
	}
}
VkPrimitiveTopology Find_PrimitiveTopology_byGFXVertexListType(vertexlisttypes_tgfx vertexlist) {
	switch (vertexlist)
	{
	case vertexlisttypes_tgfx_TRIANGLELIST:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	default:
		printer(result_tgfx_INVALIDARGUMENT, "This type of vertex list is not supported by Find_PrimitiveTopology_byGFXVertexListType()");
		return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
		break;
	}
}
VkIndexType Find_IndexType_byGFXDATATYPE(datatype_tgfx datatype) {
	switch (datatype)
	{
	case datatype_tgfx_VAR_UINT32:
		return VK_INDEX_TYPE_UINT32;
	case datatype_tgfx_VAR_UINT16:
		return VK_INDEX_TYPE_UINT16;
	default:
		printer(result_tgfx_INVALIDARGUMENT, "This type of data isn't supported by Find_IndexType_byGFXDATATYPE()");
		return VK_INDEX_TYPE_MAX_ENUM;
	}
}
VkCompareOp Find_CompareOp_byGFXDepthTest(depthtest_tgfx test) {
	switch (test) {
	case depthtest_tgfx_NEVER:
		return VK_COMPARE_OP_NEVER;
	case depthtest_tgfx_ALWAYS:
		return VK_COMPARE_OP_ALWAYS;
	case depthtest_tgfx_GEQUAL:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case depthtest_tgfx_GREATER:
		return VK_COMPARE_OP_GREATER;
	case depthtest_tgfx_LEQUAL:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case depthtest_tgfx_LESS:
		return VK_COMPARE_OP_LESS;
	default:
		printer(result_tgfx_INVALIDARGUMENT, "Find_CompareOp_byGFXDepthTest() doesn't support this type of test!");
		return VK_COMPARE_OP_MAX_ENUM;
	}
}

void Find_DepthMode_byGFXDepthMode(depthmode_tgfx mode, VkBool32& ShouldTest, VkBool32& ShouldWrite) {
	switch (mode)
	{
	case depthmode_tgfx_READ_WRITE:
		ShouldTest = VK_TRUE;
		ShouldWrite = VK_TRUE;
		break;
	case depthmode_tgfx_READ_ONLY:
		ShouldTest = VK_TRUE;
		ShouldWrite = VK_FALSE;
		break;
	case depthmode_tgfx_OFF:
		ShouldTest = VK_FALSE;
		ShouldWrite = VK_FALSE;
		break;
	default:
		printer(result_tgfx_INVALIDARGUMENT, "Find_DepthMode_byGFXDepthMode() doesn't support this type of depth mode!");
		break;
	}
}

VkAttachmentLoadOp Find_LoadOp_byGFXLoadOp(drawpassload_tgfx load) {
	switch (load) {
	case drawpassload_tgfx_CLEAR:
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case drawpassload_tgfx_FULL_OVERWRITE:
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	case drawpassload_tgfx_LOAD:
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	default:
		printer(result_tgfx_INVALIDARGUMENT, "Find_LoadOp_byGFXLoadOp() doesn't support this type of load!");
		return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
	}
}

VkCompareOp Find_CompareOp_byGFXStencilCompare(stencilcompare_tgfx op) {
	switch (op)
	{
	case stencilcompare_tgfx_NEVER_PASS:
		return VK_COMPARE_OP_NEVER;
	case stencilcompare_tgfx_LESS_PASS:
		return VK_COMPARE_OP_LESS;
	case stencilcompare_tgfx_LESS_OR_EQUAL_PASS:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case stencilcompare_tgfx_EQUAL_PASS:
		return VK_COMPARE_OP_EQUAL;
	case stencilcompare_tgfx_NOTEQUAL_PASS:
		return VK_COMPARE_OP_NOT_EQUAL;
	case stencilcompare_tgfx_GREATER_OR_EQUAL_PASS:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case stencilcompare_tgfx_GREATER_PASS:
		return VK_COMPARE_OP_GREATER;
	case stencilcompare_tgfx_OFF:
	case stencilcompare_tgfx_ALWAYS_PASS:
		return VK_COMPARE_OP_ALWAYS;
	default:
		printer(result_tgfx_INVALIDARGUMENT, "Find_CompareOp_byGFXStencilCompare() doesn't support this type of stencil compare!");
		return VK_COMPARE_OP_ALWAYS;
	}
}

VkStencilOp Find_StencilOp_byGFXStencilOp(stencilop_tgfx op) {
	switch (op)
	{
	case stencilop_tgfx_DONT_CHANGE:
		return VK_STENCIL_OP_KEEP;
	case stencilop_tgfx_SET_ZERO:
		return VK_STENCIL_OP_ZERO;
	case stencilop_tgfx_CHANGE:
		return VK_STENCIL_OP_REPLACE;
	case stencilop_tgfx_CLAMPED_INCREMENT:
		return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
	case stencilop_tgfx_WRAPPED_INCREMENT:
		return VK_STENCIL_OP_INCREMENT_AND_WRAP;
	case stencilop_tgfx_CLAMPED_DECREMENT:
		return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
	case stencilop_tgfx_WRAPPED_DECREMENT:
		return VK_STENCIL_OP_DECREMENT_AND_WRAP;
	case stencilop_tgfx_BITWISE_INVERT:
		return VK_STENCIL_OP_INVERT;
	default:
		printer(result_tgfx_INVALIDARGUMENT, "Find_StencilOp_byGFXStencilOp() doesn't support this type of stencil operation!");
		return VK_STENCIL_OP_KEEP;
	}
}
VkBlendOp Find_BlendOp_byGFXBlendMode(blendmode_tgfx mode) {
	switch (mode)
	{
	case blendmode_tgfx_ADDITIVE:
		return VK_BLEND_OP_ADD;
	case blendmode_tgfx_SUBTRACTIVE:
		return VK_BLEND_OP_SUBTRACT;
	case blendmode_tgfx_SUBTRACTIVE_SWAPPED:
		return VK_BLEND_OP_REVERSE_SUBTRACT;
	case blendmode_tgfx_MIN:
		return VK_BLEND_OP_MIN;
	case blendmode_tgfx_MAX:
		return VK_BLEND_OP_MAX;
	default:
		printer(result_tgfx_INVALIDARGUMENT, "Find_BlendOp_byGFXBlendMode() doesn't support this type of blend mode!");
		return VK_BLEND_OP_MAX_ENUM;
	}
}
VkBlendFactor Find_BlendFactor_byGFXBlendFactor(blendfactor_tgfx factor) {
	switch (factor)
	{
	case blendfactor_tgfx_ONE:
		return VK_BLEND_FACTOR_ONE;
	case blendfactor_tgfx_ZERO:
		return VK_BLEND_FACTOR_ZERO;
	case blendfactor_tgfx_SRC_COLOR:
		return VK_BLEND_FACTOR_SRC_COLOR;
	case blendfactor_tgfx_SRC_1MINUSCOLOR:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case blendfactor_tgfx_SRC_ALPHA:
		return VK_BLEND_FACTOR_SRC_ALPHA;
	case blendfactor_tgfx_SRC_1MINUSALPHA:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case blendfactor_tgfx_DST_COLOR:
		return VK_BLEND_FACTOR_DST_COLOR;
	case blendfactor_tgfx_DST_1MINUSCOLOR:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case blendfactor_tgfx_DST_ALPHA:
		return VK_BLEND_FACTOR_DST_ALPHA;
	case blendfactor_tgfx_DST_1MINUSALPHA:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	case blendfactor_tgfx_CONST_COLOR:
		return VK_BLEND_FACTOR_CONSTANT_COLOR;
	case blendfactor_tgfx_CONST_1MINUSCOLOR:
		return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
	case blendfactor_tgfx_CONST_ALPHA:
		return VK_BLEND_FACTOR_CONSTANT_ALPHA;
	case blendfactor_tgfx_CONST_1MINUSALPHA:
		return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
	default:
		printer(result_tgfx_INVALIDARGUMENT, "Find_BlendFactor_byGFXBlendFactor() doesn't support this type of blend factor!");
		return VK_BLEND_FACTOR_MAX_ENUM;
	}
}
void Fill_ComponentMapping_byCHANNELs(texture_channels_tgfx channels, VkComponentMapping& mapping) {
	switch (channels)
	{
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
		mapping.a = VK_COMPONENT_SWIZZLE_ZERO;
		return;
	case texture_channels_tgfx_RA32F:
	case texture_channels_tgfx_RA32UI:
	case texture_channels_tgfx_RA32I:
	case texture_channels_tgfx_RA8UB:
	case texture_channels_tgfx_RA8B:
		mapping.r = VK_COMPONENT_SWIZZLE_R;
		mapping.g = VK_COMPONENT_SWIZZLE_ZERO;
		mapping.b = VK_COMPONENT_SWIZZLE_ZERO;
		mapping.a = VK_COMPONENT_SWIZZLE_A;
		return;
	case texture_channels_tgfx_R32F:
	case texture_channels_tgfx_R32UI:
	case texture_channels_tgfx_R32I:
	case texture_channels_tgfx_R8UB:
	case texture_channels_tgfx_R8B:
		mapping.r = VK_COMPONENT_SWIZZLE_R;
		mapping.g = VK_COMPONENT_SWIZZLE_ZERO;
		mapping.b = VK_COMPONENT_SWIZZLE_ZERO;
		mapping.a = VK_COMPONENT_SWIZZLE_ZERO;
		return;
	default:
		break;
	}
}
void Find_SubpassAccessPattern(subdrawpassaccess_tgfx access, bool isSource, VkPipelineStageFlags& stageflag, VkAccessFlags& accessflag) {
	switch (access)
	{
	case subdrawpassaccess_tgfx_ALLCOMMANDS:
		if (isSource) {
			stageflag |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}
		else {
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
		accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
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
		accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
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
		accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
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
		accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
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
		break;
	}
}
desctype_vk Find_DescType_byGFXShaderInputType(shaderinputtype_tgfx dtype) {
	switch (dtype)
	{
	case shaderinputtype_tgfx_SAMPLER_PI:
	case shaderinputtype_tgfx_SAMPLER_G:
		return desctype_vk::SAMPLER;
	case shaderinputtype_tgfx_IMAGE_PI:
	case shaderinputtype_tgfx_IMAGE_G:
		return desctype_vk::IMAGE;
	case shaderinputtype_tgfx_UBUFFER_PI:
	case shaderinputtype_tgfx_UBUFFER_G:
		return desctype_vk::UBUFFER;
	case shaderinputtype_tgfx_SBUFFER_PI:
	case shaderinputtype_tgfx_SBUFFER_G:
		return desctype_vk::SBUFFER;
	default:
		printer(result_tgfx_FAIL, "(Find_DescType_byGFXShaderInputType() doesn't support this type!");
		return desctype_vk::SAMPLER;
	}
}
