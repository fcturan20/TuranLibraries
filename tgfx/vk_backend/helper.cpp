#include <iostream>
#include "tgfx_forwarddeclarations.h"
#include "tgfx_structs.h"
#include "helper.h"
#include <tgfx_core.h>
#include <tgfx_helper.h>
#include "core.h"
#include "extension.h"
#include "resource.h"
#include "predefinitions_vk.h"
#include "includes.h"
#include "renderer.h"
#include "gpucontentmanager.h"

//Hardware Capability Helpers
static void  GetGPUInfo_General(gpu_tgfx_handle GPUHandle, const char** NAME, unsigned int* API_VERSION, unsigned int* DRIVER_VERSION, gpu_type_tgfx* GPUTYPE, const memory_description_tgfx** MemType_descs,
    unsigned int* MemType_descsCount, unsigned char* isGraphicsOperationsSupported, unsigned char* isComputeOperationsSupported, unsigned char* isTransferOperationsSupported) {
    GPU_VKOBJ* VKGPU = (GPU_VKOBJ*)GPUHandle;
    if (NAME) { *NAME = VKGPU->DEVICENAME(); }
    if (API_VERSION) { *API_VERSION = VKGPU->APIVERSION(); }
    if (DRIVER_VERSION) { *DRIVER_VERSION = VKGPU->DRIVERSION(); }
    if (GPUTYPE) { *GPUTYPE = VKGPU->DEVICETYPE(); }

    if (MemType_descs) { *MemType_descs = VKGPU->DESC().MEMTYPEs; *MemType_descsCount = VKGPU->DESC().MEMTYPEsCOUNT; }
    if (isGraphicsOperationsSupported) { *isGraphicsOperationsSupported = VKGPU->GRAPHICSSUPPORTED(); }
    if (isComputeOperationsSupported) { *isComputeOperationsSupported = VKGPU->COMPUTESUPPORTED(); }
    if (isTransferOperationsSupported) { *isTransferOperationsSupported = VKGPU->TRANSFERSUPPORTED(); }
}
static unsigned char  GetTextureTypeLimits(texture_dimensions_tgfx dims, texture_order_tgfx dataorder, texture_channels_tgfx channeltype,
    textureusageflag_tgfx_handle usageflag, gpu_tgfx_handle GPUHandle, unsigned int* MAXWIDTH, unsigned int* MAXHEIGHT, unsigned int* MAXDEPTH,
    unsigned int* MAXMIPLEVEL) {
    VkImageFormatProperties props;
    VkImageUsageFlags flag = *(VkImageUsageFlags*)usageflag;
    if (channeltype == texture_channels_tgfx_D24S8 || channeltype == texture_channels_tgfx_D32) { flag &= ~(1UL << 4); }
    else { flag &= ~(1UL << 5); }
    VkResult result = vkGetPhysicalDeviceImageFormatProperties(rendergpu->PHYSICALDEVICE(), Find_VkFormat_byTEXTURECHANNELs(channeltype),
        Find_VkImageType(dims), Find_VkTiling(dataorder), flag,
        0, &props);
    if (result != VK_SUCCESS) {
        printer(result_tgfx_FAIL, ("GFX->GetTextureTypeLimits() has failed with: " + std::to_string(result)).c_str());
        return false;
    }

    if (MAXWIDTH) { *MAXWIDTH = props.maxExtent.width; }
    if(MAXHEIGHT){ *MAXHEIGHT = props.maxExtent.height; }
    if(MAXDEPTH){ *MAXDEPTH = props.maxExtent.depth; }
    if(MAXMIPLEVEL){ *MAXMIPLEVEL = props.maxMipLevels; }
    return true;
}
static textureusageflag_tgfx_handle CreateTextureUsageFlag(unsigned char isCopiableFrom, unsigned char isCopiableTo,
    unsigned char isRenderableTo, unsigned char isSampledReadOnly, unsigned char isRandomlyWrittenTo) {
    VkImageUsageFlags* UsageFlag = nullptr, FlagObj = 0;
    if (VKCONST_isPointerContainVKFLAG) { UsageFlag = &FlagObj; }
    else { UsageFlag = new VkImageUsageFlags; }
    *UsageFlag = ((isCopiableFrom) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0) |
        ((isCopiableTo) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0) |
        ((isRenderableTo) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0) |
        ((isSampledReadOnly) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0) |
        ((isRandomlyWrittenTo) ? VK_IMAGE_USAGE_STORAGE_BIT : 0);
    if (VKCONST_isPointerContainVKFLAG) { return *(textureusageflag_tgfx_handle*)&FlagObj; }
    else { return (textureusageflag_tgfx_handle)UsageFlag; }
}
//You can't create a memory type, only set allocation size of a given Memory Type (Memory Type is given by the GFX initialization process)
extern result_tgfx SetMemoryTypeInfo(unsigned int MemoryType_id, unsigned long long AllocationSize, extension_tgfx_listhandle Extensions);
static initializationsecondstageinfo_tgfx_handle Create_GFXInitializationSecondStageInfo(
    gpu_tgfx_handle RendererGPU,
    unsigned int MAXDYNAMICSAMPLER_DESCCOUNT, unsigned int MAXSAMPLEDTEXTURE_DESCCOUNT, unsigned int MAXSTORAGEIMAGE_DESCCOUNT, unsigned int MAXBUFFER_DESCCOUNT,
    unsigned int CALLBUFFERSIZE,
    unsigned char ShouldActive_dearIMGUI,
    extension_tgfx_listhandle EXTList) {
    GPU_VKOBJ* VKGPU = (GPU_VKOBJ*)RendererGPU;


    
    if (VKGPU->EXTMANAGER()->GETMAXDESCALL() < MAXDYNAMICSAMPLER_DESCCOUNT + MAXSAMPLEDTEXTURE_DESCCOUNT + MAXSTORAGEIMAGE_DESCCOUNT + MAXBUFFER_DESCCOUNT) { printer(result_tgfx_FAIL, "MAX****_DESCCOUNT's total exceeds the max number of descriptors your GPU supports!"); return NULL;}
    else{
        unsigned int TOTALNEEDEDDESCCOUNTS_PERTYPE[VKCONST_DYNAMICDESCRIPTORTYPESCOUNT] = {}, TOTAL_NEEDEDDESCSETCOUNT = 0;
        const unsigned int maxdessetcount = VKGPU->EXTMANAGER()->GETMAXBOUNDDESCSETCOUNT();
        
        if (TOTAL_NEEDEDDESCSETCOUNT > maxdessetcount) { printer(result_tgfx_FAIL, ("GPU supports " + std::to_string(maxdessetcount) + " but you need more than that. So please change resource binding model infos according to that!").c_str()); return NULL; }
        
    }


    initialization_secondstageinfo* info = new initialization_secondstageinfo;

    info->renderergpu = VKGPU;
    info->shouldActivate_DearIMGUI = ShouldActive_dearIMGUI;
    info->MAXDESCCOUNTs[VKCONST_DESCSETID_BUFFER] = MAXBUFFER_DESCCOUNT;
    info->MAXDESCCOUNTs[VKCONST_DESCSETID_DYNAMICSAMPLER] = MAXDYNAMICSAMPLER_DESCCOUNT;
    info->MAXDESCCOUNTs[VKCONST_DESCSETID_SAMPLEDTEXTURE] = MAXSAMPLEDTEXTURE_DESCCOUNT;
    info->MAXDESCCOUNTs[VKCONST_DESCSETID_STORAGEIMAGE] = MAXSTORAGEIMAGE_DESCCOUNT;

    return (initializationsecondstageinfo_tgfx_handle)info;
}



static shaderstageflag_tgfx_handle CreateShaderStageFlag(unsigned char count, ...) {
    //If size of a pointer isn't bigger than or equal to a VkShaderStageFlag, we should return a pointer to the flag otherwise data is lost
    VkShaderStageFlags* flag = nullptr;
    VkShaderStageFlags flag_d = 0;
    if (VKCONST_isPointerContainVKFLAG) { flag = &flag_d; }
    else { flag = new VkShaderStageFlags; *flag = flag_d; }
    va_list args;
    va_start(args, count);
    for (unsigned char i = 0; i < count; i++) {
        shaderstage_tgfx type = va_arg(args, shaderstage_tgfx);
        if (type == shaderstage_tgfx_VERTEXSHADER) { *flag = *flag | VK_SHADER_STAGE_VERTEX_BIT; }
        if (type == shaderstage_tgfx_FRAGMENTSHADER) { *flag = *flag | VK_SHADER_STAGE_FRAGMENT_BIT; }
        if (type == shaderstage_tgfx_COMPUTESHADER) { *flag = *flag | VK_SHADER_STAGE_COMPUTE_BIT; }
    }
    va_end(args);
    if (VKCONST_isPointerContainVKFLAG) { return (shaderstageflag_tgfx_handle)*flag; }
    else { return (shaderstageflag_tgfx_handle)flag; }
}
static rtslotdescription_tgfx_handle CreateRTSlotDescription_Color(texture_tgfx_handle Texture0, texture_tgfx_handle Texture1,
    operationtype_tgfx OPTYPE, drawpassload_tgfx LOADTYPE, unsigned char isUsedLater, unsigned char SLOTINDEX, vec4_tgfx clear_value) {
    rtslot_create_description_vk* desc = new rtslot_create_description_vk;
    desc->clear_value = clear_value;
    desc->isUsedLater = isUsedLater;
    desc->loadtype = LOADTYPE;
    desc->optype = OPTYPE;
    desc->textures[0] = (TEXTURE_VKOBJ*)Texture0;
    desc->textures[1] = (TEXTURE_VKOBJ*)Texture1;
    return (rtslotdescription_tgfx_handle)desc;
}
static rtslotdescription_tgfx_handle CreateRTSlotDescription_DepthStencil(texture_tgfx_handle Texture0, texture_tgfx_handle Texture1,
    operationtype_tgfx DEPTHOP, drawpassload_tgfx DEPTHLOAD, operationtype_tgfx STENCILOP, drawpassload_tgfx STENCILLOAD,
    float DEPTHCLEARVALUE, unsigned char STENCILCLEARVALUE) {
    printer(result_tgfx_WARNING, "CreateRTSlotDescription_DepthStencil should be implemented properly!");
    rtslot_create_description_vk* desc = new rtslot_create_description_vk;
    desc->clear_value.x = DEPTHCLEARVALUE; desc->clear_value.y = STENCILCLEARVALUE;
    desc->isUsedLater = true;
    desc->loadtype = DEPTHLOAD;
    desc->optype = DEPTHOP;
    desc->textures[0] = (TEXTURE_VKOBJ*)Texture0;
    desc->textures[1] = (TEXTURE_VKOBJ*)Texture1;
    return (rtslotdescription_tgfx_handle)desc;
}
static rtslotusage_tgfx_handle CreateRTSlotUsage_Color(rtslotdescription_tgfx_handle base_slot, operationtype_tgfx OPTYPE, drawpassload_tgfx LOADTYPE){
    printer(result_tgfx_WARNING, "Vulkan backend doesn't use LOADTYPE for now!");
    if (!base_slot) { printer(result_tgfx_INVALIDARGUMENT, "CreateRTSlotUsage_Color() has failed because base_slot is nullptr!"); return nullptr; }
    rtslot_create_description_vk* baseslot = (rtslot_create_description_vk*)base_slot;
    if (baseslot->optype == operationtype_tgfx_READ_ONLY &&
        (OPTYPE == operationtype_tgfx_WRITE_ONLY || OPTYPE == operationtype_tgfx_READ_AND_WRITE)
        )
    {
        printer(result_tgfx_INVALIDARGUMENT, "Inherite_RTSlotSet() has failed because you can't use a Read-Only ColorSlot with Write Access in a Inherited Set!");
        return nullptr;
    }
    rtslot_inheritance_descripton_vk* usage = new rtslot_inheritance_descripton_vk;
    usage->IS_DEPTH = false;
    usage->OPTYPE = OPTYPE;
    usage->OPTYPESTENCIL = operationtype_tgfx_UNUSED;
    usage->LOADTYPE = LOADTYPE;
    usage->LOADTYPESTENCIL = drawpassload_tgfx_CLEAR;
    return (rtslotusage_tgfx_handle)usage;
}
static rtslotusage_tgfx_handle CreateRTSlotUsage_Depth(rtslotdescription_tgfx_handle base_slot, operationtype_tgfx DEPTHOP, drawpassload_tgfx DEPTHLOAD,
    operationtype_tgfx STENCILOP, drawpassload_tgfx STENCILLOAD) {
    if (!base_slot) { printer(result_tgfx_INVALIDARGUMENT, "CreateRTSlotUsage_Color() has failed because base_slot is nullptr!"); return nullptr; }
    rtslot_create_description_vk* baseslot = (rtslot_create_description_vk*)base_slot;
    if (baseslot->optype == operationtype_tgfx_READ_ONLY &&
        (DEPTHOP == operationtype_tgfx_WRITE_ONLY || DEPTHOP == operationtype_tgfx_READ_AND_WRITE)
        )
    {
        printer(result_tgfx_INVALIDARGUMENT, "Inherite_RTSlotSet() has failed because you can't use a Read-Only DepthSlot with Write Access in a Inherited Set!");
        return nullptr;
    }
    rtslot_inheritance_descripton_vk* usage = new rtslot_inheritance_descripton_vk;
    usage->IS_DEPTH = true;
    usage->OPTYPE = DEPTHOP;
    usage->OPTYPESTENCIL = STENCILOP;
    usage->LOADTYPE = DEPTHLOAD;
    usage->LOADTYPESTENCIL = STENCILLOAD;
    return (rtslotusage_tgfx_handle)usage;
}
static depthsettings_tgfx_handle CreateDepthConfiguration(unsigned char ShouldWrite, depthtest_tgfx COMPAREOP, extension_tgfx_listhandle EXTENSIONS) {
    depthsettingsdesc_vk* desc = new depthsettingsdesc_vk;
    TGFXLISTCOUNT(core_tgfx_main, EXTENSIONS, extcount);
    for (unsigned int ext_i = 0; ext_i < extcount; ext_i++) {
        if (*(VKEXT_TYPES*)(EXTENSIONS[ext_i]) != VKEXT_DEPTHBOUNDS) {
            VKEXT_DEPTHBOUNDS_STRUCT* ext = (VKEXT_DEPTHBOUNDS_STRUCT*)EXTENSIONS[ext_i];
            desc->DepthBoundsEnable = ext->DepthBoundsEnable;
            desc->DepthBoundsMax = ext->DepthBoundsMax;
            desc->DepthBoundsMin = ext->DepthBoundsMin;
        }
    }
    desc->ShouldWrite = ShouldWrite;
    desc->DepthCompareOP = Find_CompareOp_byGFXDepthTest(COMPAREOP);
    return (depthsettings_tgfx_handle)desc;
}
static stencilsettings_tgfx_handle CreateStencilConfiguration(unsigned char Reference, unsigned char WriteMask, unsigned char CompareMask,
    stencilcompare_tgfx CompareOP, stencilop_tgfx DepthFailOP, stencilop_tgfx StencilFailOP, stencilop_tgfx AllSuccessOP) {
    return nullptr;
}
static blendinginfo_tgfx_handle CreateBlendingConfiguration(unsigned char ColorSlotIndex, vec4_tgfx Constant, blendfactor_tgfx SRCFCTR_CLR,
    blendfactor_tgfx SRCFCTR_ALPHA, blendfactor_tgfx DSTFCTR_CLR, blendfactor_tgfx DSTFCTR_ALPHA, blendmode_tgfx BLENDOP_CLR,
    blendmode_tgfx BLENDOP_ALPHA, colorcomponents_tgfx WRITECHANNELs) {
    return nullptr;
}


//EXTENSION HELPERS

static void Destroy_ExtensionData(extension_tgfx_handle ExtensionToDestroy) {

}
static extension_tgfx_handle EXT_DepthBoundsInfo(float BoundMin, float BoundMax) {
    VKEXT_DEPTHBOUNDS_STRUCT* ext = new VKEXT_DEPTHBOUNDS_STRUCT;
    ext->type = VKEXT_DEPTHBOUNDS;
    ext->DepthBoundsEnable = VK_TRUE;
    ext->DepthBoundsMin = BoundMin;
    ext->DepthBoundsMax = BoundMax;
    return (extension_tgfx_handle)ext;
}

extern void set_helper_functions() {
    core_tgfx_main->helpers->CreateBlendingConfiguration = &CreateBlendingConfiguration;
    core_tgfx_main->helpers->CreateDepthConfiguration = &CreateDepthConfiguration;
    core_tgfx_main->helpers->CreateRTSlotDescription_Color = &CreateRTSlotDescription_Color;
    core_tgfx_main->helpers->CreateRTSlotDescription_DepthStencil = &CreateRTSlotDescription_DepthStencil;
    core_tgfx_main->helpers->CreateRTSlotUsage_Color = &CreateRTSlotUsage_Color;
    core_tgfx_main->helpers->CreateRTSlotUsage_Depth = &CreateRTSlotUsage_Depth;
    core_tgfx_main->helpers->CreateStencilConfiguration = &CreateStencilConfiguration;
    core_tgfx_main->helpers->CreateTextureUsageFlag = &CreateTextureUsageFlag;
    core_tgfx_main->helpers->Create_GFXInitializationSecondStageInfo = &Create_GFXInitializationSecondStageInfo;
    core_tgfx_main->helpers->Destroy_ExtensionData = &Destroy_ExtensionData;
    core_tgfx_main->helpers->GetGPUInfo_General = &GetGPUInfo_General;
    core_tgfx_main->helpers->GetTextureTypeLimits = &GetTextureTypeLimits;
    core_tgfx_main->helpers->SetMemoryTypeInfo = &SetMemoryTypeInfo;
    core_tgfx_main->helpers->CreateShaderStageFlag = &CreateShaderStageFlag;
    //RenderNode helpers are moved to RG_Description.cpp


    core_tgfx_main->helpers->EXT_DepthBoundsInfo = &EXT_DepthBoundsInfo;
}