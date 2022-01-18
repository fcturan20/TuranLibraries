#include <iostream>
#include "tgfx_forwarddeclarations.h"
#include "tgfx_structs.h"
#include "helper.h"
#include <tgfx_core.h>
#include <tgfx_helper.h>
#include "core.h"


struct helper_functions {
    //Hardware Capability Helpers
    static void  GetGPUInfo_General(gpu_tgfx_handle GPUHandle, const char** NAME, unsigned int* API_VERSION, unsigned int* DRIVER_VERSION, gpu_type_tgfx* GPUTYPE, const memory_description_tgfx** MemType_descs,
        unsigned int* MemType_descsCount, unsigned char* isGraphicsOperationsSupported, unsigned char* isComputeOperationsSupported, unsigned char* isTransferOperationsSupported) {
        gpu_public* VKGPU = (gpu_public*)GPUHandle;
        if (NAME) { *NAME = VKGPU->DEVICENAME().c_str(); }
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
        return 0;
    }
    static textureusageflag_tgfx_handle CreateTextureUsageFlag(unsigned char isCopiableFrom, unsigned char isCopiableTo,
        unsigned char isRenderableTo, unsigned char isSampledReadOnly, unsigned char isRandomlyWrittenTo) {
        return nullptr;
    }
    static void  GetSupportedAllocations_ofTexture(unsigned int GPUIndex, unsigned int* SupportedMemoryTypesBitset) {

    }
    //You can't create a memory type, only set allocation size of a given Memory Type (Memory Type is given by the GFX initialization process)
    static result_tgfx SetMemoryTypeInfo(unsigned int MemoryType_id, unsigned long long AllocationSize, extension_tgfx_listhandle Extensions) {
        return result_tgfx_FAIL;
    }
    static initializationsecondstageinfo_tgfx_handle Create_GFXInitializationSecondStageInfo(
        gpu_tgfx_handle RendererGPU,
        //You have to specify how much shader input categories you're gonna use max (material types that have general shader inputs + material instances that have per instance shader inputs)
        unsigned int MaterialCount,
        //You have to specify max sum of how much shader inputs you're gonna use for materials (General and Per Instance)
        unsigned int MaxSumMaterial_SampledTexture, unsigned int MaxSumMaterial_ImageTexture, unsigned int MaxSumMaterial_UniformBuffer, unsigned int MaxSumMaterial_StorageBuffer,
        //You have to specify how many global shader inputs you're gonna use (Don't forget to look at descriptor indexing extensions)
        unsigned int GlobalSampledTextureInputCount, unsigned int GlobalImageTextureInputCount, unsigned int GlobalUniformBufferInputCount, unsigned int GlobalInputStorageBufferInputCount,
        //See the documentation
        unsigned char isGlobalUniformBuffer_Index1, unsigned char isGlobalSampledTexture_Index1,
        unsigned char ShouldActive_dearIMGUI,
        extension_tgfx_listhandle EXTList) {
        return nullptr;
    }
    static void  GetMonitor_Resolution_ColorBites_RefreshRate(monitor_tgfx_handle MonitorHandle, unsigned int* WIDTH, unsigned int* HEIGHT, unsigned int* ColorBites, unsigned int* RefreshRate) {

    }
    //Barrier Dependency Helpers

    static waitsignaldescription_tgfx_handle CreateWaitSignal_DrawIndirectConsume() {
        return nullptr;
    }
    static waitsignaldescription_tgfx_handle CreateWaitSignal_VertexInput(unsigned char IndexBuffer, unsigned char VertexAttrib) {
        return nullptr;
    }
    static waitsignaldescription_tgfx_handle CreateWaitSignal_VertexShader(unsigned char UniformRead, unsigned char StorageRead, unsigned char StorageWrite) {
        return nullptr;
    }
    static waitsignaldescription_tgfx_handle CreateWaitSignal_FragmentShader(unsigned char UniformRead, unsigned char StorageRead, unsigned char StorageWrite) {
        return nullptr;
    }
    static waitsignaldescription_tgfx_handle CreateWaitSignal_ComputeShader(unsigned char UniformRead, unsigned char StorageRead, unsigned char StorageWrite) {
        return nullptr;
    }
    static waitsignaldescription_tgfx_handle CreateWaitSignal_FragmentTests(unsigned char isEarly, unsigned char isRead, unsigned char isWrite) {
        return nullptr;
    }

    //RENDERNODE HELPERS


    //WaitInfos is a pointer because function expects a list of waits (Color attachment output and also VertexShader-UniformReads etc)
    static passwaitdescription_tgfx_handle CreatePassWait_DrawPass(drawpass_tgfx_handle* PassHandle, unsigned char SubpassIndex,
        waitsignaldescription_tgfx_handle* WaitInfos, unsigned char isLastFrame) {
        return nullptr;
    }
    //WaitInfo is single, because function expects only one wait and it should be created with CreateWaitSignal_ComputeShader()
    static passwaitdescription_tgfx_handle CreatePassWait_ComputePass(computepass_tgfx_handle* PassHandle, unsigned char SubpassIndex,
        waitsignaldescription_tgfx_handle WaitInfo, unsigned char isLastFrame) {
        return nullptr;
    }
    //WaitInfo is single, because function expects only one wait and it should be created with CreateWaitSignal_Transfer()
    static passwaitdescription_tgfx_handle CreatePassWait_TransferPass(transferpasstype_tgfx* PassHandle,
        transferpasstype_tgfx Type, waitsignaldescription_tgfx_handle WaitInfo, unsigned char isLastFrame) {
        return nullptr;
    }
    //There is no option because you can only wait for a penultimate window pass
    //I'd like to support last frame wait too but it confuses the users and it doesn't have much use
    static passwaitdescription_tgfx_handle CreatePassWait_WindowPass(windowpass_tgfx_handle* PassHandle) {
        return nullptr;
    }

    static subdrawpassdescription_tgfx_handle CreateSubDrawPassDescription() {
        return nullptr;
    }


    static shaderinputdescription_tgfx_handle CreateShaderInputDescription(unsigned char isGeneral, shaderinputtype_tgfx Type, unsigned int BINDINDEX,
        unsigned int ELEMENTCOUNT, shaderstageflag_tgfx_handle Stages) {
        return nullptr;
    }
    static rtslotdescription_tgfx_handle CreateRTSlotDescription_Color(texture_tgfx_handle Texture0, texture_tgfx_handle Texture1,
        operationtype_tgfx OPTYPE, drawpassload_tgfx LOADTYPE, unsigned char isUsedLater, unsigned char SLOTINDEX, vec4_tgfx CLEARVALUE) {
        return nullptr;
    }
    static rtslotdescription_tgfx_handle CreateRTSlotDescription_DepthStencil(texture_tgfx_handle Texture0, texture_tgfx_handle Texture1,
        operationtype_tgfx DEPTHOP, drawpassload_tgfx DEPTHLOAD, operationtype_tgfx STENCILOP, drawpassload_tgfx STENCILLOAD,
        float DEPTHCLEARVALUE, unsigned char STENCILCLEARVALUE) {
        return nullptr;
    }
    static rtslotusage_tgfx_handle CreateRTSlotUsage_Color(unsigned char SLOTINDEX, operationtype_tgfx OPTYPE, drawpassload_tgfx LOADTYPE){
        return nullptr;
    }
    static rtslotusage_tgfx_handle CreateRTSlotUsage_Depth(operationtype_tgfx DEPTHOP, drawpassload_tgfx DEPTHLOAD,
        operationtype_tgfx STENCILOP, drawpassload_tgfx STENCILLOAD) {
        return nullptr;
    }
    static depthsettings_tgfx_handle CreateDepthConfiguration(unsigned char ShouldWrite, depthtest_tgfx COMPAREOP) {
        return nullptr;
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
    static unsigned char DoesGPUsupportsVKDESCINDEXING(gpu_tgfx_handle GPU) {
        return 0;
    }
};


extern void set_helper_functions() {
    core_tgfx_main->helpers->CreateBlendingConfiguration = &helper_functions::CreateBlendingConfiguration;
    core_tgfx_main->helpers->CreateDepthConfiguration = &helper_functions::CreateDepthConfiguration;
    core_tgfx_main->helpers->CreatePassWait_ComputePass = &helper_functions::CreatePassWait_ComputePass;
    core_tgfx_main->helpers->CreatePassWait_DrawPass = &helper_functions::CreatePassWait_DrawPass;
    core_tgfx_main->helpers->CreatePassWait_TransferPass = &helper_functions::CreatePassWait_TransferPass;
    core_tgfx_main->helpers->CreatePassWait_WindowPass = &helper_functions::CreatePassWait_WindowPass;
    core_tgfx_main->helpers->CreateRTSlotDescription_Color = &helper_functions::CreateRTSlotDescription_Color;
    core_tgfx_main->helpers->CreateRTSlotDescription_DepthStencil = &helper_functions::CreateRTSlotDescription_DepthStencil;
    core_tgfx_main->helpers->CreateRTSlotUsage_Color = &helper_functions::CreateRTSlotUsage_Color;
    core_tgfx_main->helpers->CreateRTSlotUsage_Depth = &helper_functions::CreateRTSlotUsage_Depth;
    core_tgfx_main->helpers->CreateShaderInputDescription = &helper_functions::CreateShaderInputDescription;
    core_tgfx_main->helpers->CreateStencilConfiguration = &helper_functions::CreateStencilConfiguration;
    core_tgfx_main->helpers->CreateSubDrawPassDescription = &helper_functions::CreateSubDrawPassDescription;
    core_tgfx_main->helpers->CreateTextureUsageFlag = &helper_functions::CreateTextureUsageFlag;
    core_tgfx_main->helpers->CreateWaitSignal_ComputeShader = &helper_functions::CreateWaitSignal_ComputeShader;
    core_tgfx_main->helpers->CreateWaitSignal_DrawIndirectConsume = &helper_functions::CreateWaitSignal_DrawIndirectConsume;
    core_tgfx_main->helpers->CreateWaitSignal_FragmentShader = &helper_functions::CreateWaitSignal_FragmentShader;
    core_tgfx_main->helpers->CreateWaitSignal_FragmentTests = &helper_functions::CreateWaitSignal_FragmentTests;
    core_tgfx_main->helpers->CreateWaitSignal_VertexInput = &helper_functions::CreateWaitSignal_VertexInput;
    core_tgfx_main->helpers->CreateWaitSignal_VertexShader = &helper_functions::CreateWaitSignal_VertexShader;
    core_tgfx_main->helpers->Create_GFXInitializationSecondStageInfo = &helper_functions::Create_GFXInitializationSecondStageInfo;
    core_tgfx_main->helpers->Destroy_ExtensionData = &helper_functions::Destroy_ExtensionData;
    core_tgfx_main->helpers->DoesGPUsupportsVKDESCINDEXING = &helper_functions::DoesGPUsupportsVKDESCINDEXING;
    core_tgfx_main->helpers->GetGPUInfo_General = &helper_functions::GetGPUInfo_General;
    core_tgfx_main->helpers->GetMonitor_Resolution_ColorBites_RefreshRate = &helper_functions::GetMonitor_Resolution_ColorBites_RefreshRate;
    core_tgfx_main->helpers->GetSupportedAllocations_ofTexture = &helper_functions::GetSupportedAllocations_ofTexture;
    core_tgfx_main->helpers->GetTextureTypeLimits = &helper_functions::GetTextureTypeLimits;
    core_tgfx_main->helpers->SetMemoryTypeInfo = &helper_functions::SetMemoryTypeInfo;
}