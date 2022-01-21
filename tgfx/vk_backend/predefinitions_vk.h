#pragma once
//Vulkan Libs
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <tgfx_forwarddeclarations.h>


//Systems 

struct core_tgfx;
extern core_tgfx* core_tgfx_main;
struct core_public;
extern core_public* core_vk;
struct renderer_public;
extern renderer_public* renderer;
struct gpudatamanager_public;
extern gpudatamanager_public* contentmanager;
struct IMGUI_VK;
extern IMGUI_VK* imgui;
struct gpu_public;
extern gpu_public* rendergpu;
typedef struct threadingsys_tapi threadingsys_tapi;
extern threadingsys_tapi* threadingsys;
extern unsigned int threadcount;
struct allocatorsys_vk;
extern allocatorsys_vk* allocatorsys;
struct queuesys_vk;
extern queuesys_vk* queuesys;

extern tgfx_PrintLogCallback printer;


//Synchronization systems and objects

struct semaphoresys_vk;
extern semaphoresys_vk* semaphoresys;
struct fencesys_vk;
extern fencesys_vk* fencesys;
struct texture_vk;
#ifdef VULKAN_DEBUGGING
typedef unsigned int semaphore_idtype_vk;
static constexpr semaphore_idtype_vk invalid_semaphoreid = UINT32_MAX;
typedef unsigned int fence_idtype_vk;
static constexpr fence_idtype_vk invalid_fenceid = UINT32_MAX;
#else
struct semaphore_vk;
typedef semaphore_vk* semaphore_idtype_vk;
static constexpr semaphore_idtype_vk invalid_semaphoreid = nullptr;
struct fence_vk;
typedef fence_vk* fence_idtype_vk;
static constexpr fence_idtype_vk invalid_fenceid = nullptr;
#endif


//Structs

struct memorytype_vk;
struct queuefam_vk;
struct extension_manager;	//Stores activated extensions and set function pointers according to that
struct drawpass_vk;
struct transferpass_vk;
struct windowpass_vk;
struct computepass_vk;
struct subdrawpass_vk;
struct indexeddrawcall_vk;
struct nonindexeddrawcall_vk;
struct irtslotset_vk;
struct rtslotset_vk;

//Enums
enum class desctype_vk : unsigned char {
	SAMPLER = 0,
	IMAGE = 1,
	SBUFFER = 2,
	UBUFFER = 3
};


//Simple enumaration functions
extern VkFormat Find_VkFormat_byDataType(datatype_tgfx datatype);
extern VkFormat Find_VkFormat_byTEXTURECHANNELs(texture_channels_tgfx channels);
extern VkDescriptorType Find_VkDescType_byMATDATATYPE(shaderinputtype_tgfx TYPE);
extern VkSamplerAddressMode Find_AddressMode_byWRAPPING(texture_wrapping_tgfx Wrapping);
extern VkFilter Find_VkFilter_byGFXFilter(texture_mipmapfilter_tgfx filter);
extern VkSamplerMipmapMode Find_MipmapMode_byGFXFilter(texture_mipmapfilter_tgfx filter);
extern VkCullModeFlags Find_CullMode_byGFXCullMode(cullmode_tgfx mode);
extern VkPolygonMode Find_PolygonMode_byGFXPolygonMode(polygonmode_tgfx mode);
extern VkPrimitiveTopology Find_PrimitiveTopology_byGFXVertexListType(vertexlisttypes_tgfx type);
extern VkIndexType Find_IndexType_byGFXDATATYPE(datatype_tgfx datatype);
extern VkCompareOp Find_CompareOp_byGFXDepthTest(depthtest_tgfx test);
extern void Find_DepthMode_byGFXDepthMode(depthmode_tgfx mode, VkBool32& ShouldTest, VkBool32& ShouldWrite);
extern VkAttachmentLoadOp Find_LoadOp_byGFXLoadOp(drawpassload_tgfx load);
extern VkCompareOp Find_CompareOp_byGFXStencilCompare(stencilcompare_tgfx op);
extern VkStencilOp Find_StencilOp_byGFXStencilOp(stencilop_tgfx op);
extern VkBlendOp Find_BlendOp_byGFXBlendMode(blendmode_tgfx mode);
extern VkBlendFactor Find_BlendFactor_byGFXBlendFactor(blendfactor_tgfx factor);
extern void Fill_ComponentMapping_byCHANNELs(texture_channels_tgfx channels, VkComponentMapping& mapping);
extern void Find_SubpassAccessPattern(subdrawpassaccess_tgfx access, bool isSource, VkPipelineStageFlags& stageflag, VkAccessFlags& accessflag);
extern desctype_vk Find_DescType_byGFXShaderInputType(shaderinputtype_tgfx type);
extern VkDescriptorType Find_VkDescType_byDescTypeCategoryless(desctype_vk type);
extern VkImageType Find_VkImageType(texture_dimensions_tgfx dimensions);
extern VkImageTiling Find_VkTiling(texture_order_tgfx order);

//Resources
struct texture_vk;


extern VkInstance Vulkan_Instance;
extern VkApplicationInfo Application_Info;