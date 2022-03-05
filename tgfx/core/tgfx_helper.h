#pragma once
#include "tgfx_forwarddeclarations.h"
#include "tgfx_structs.h"

typedef struct helper_tgfx {
    //Hardware Capability Helpers
    void (*GetGPUInfo_General)(gpu_tgfx_handle GPUHandle, const char** NAME, unsigned int* API_VERSION, unsigned int* DRIVER_VERSION, gpu_type_tgfx* GPUTYPE, const memory_description_tgfx** MemType_descs,
        unsigned int* MemType_descsCount, unsigned char* isGraphicsOperationsSupported, unsigned char* isComputeOperationsSupported, unsigned char* isTransferOperationsSupported);
    unsigned char (*GetTextureTypeLimits)(texture_dimensions_tgfx dims, texture_order_tgfx dataorder, texture_channels_tgfx channeltype,
        textureusageflag_tgfx_handle usageflag, gpu_tgfx_handle GPUHandle, unsigned int* MAXWIDTH, unsigned int* MAXHEIGHT, unsigned int* MAXDEPTH,
        unsigned int* MAXMIPLEVEL);
    textureusageflag_tgfx_handle(*CreateTextureUsageFlag)(unsigned char isCopiableFrom, unsigned char isCopiableTo,
        unsigned char isRenderableTo, unsigned char isSampledReadOnly, unsigned char isRandomlyWrittenTo);
    void (*GetSupportedAllocations_ofTexture)(unsigned int GPUIndex, unsigned int* SupportedMemoryTypesBitset);
    //You can't create a memory type, only set allocation size of a given Memory Type (Memory Type is given by the GFX initialization process)
    result_tgfx(*SetMemoryTypeInfo)(unsigned int MemoryType_id, unsigned long long AllocationSize, extension_tgfx_listhandle Extensions);
    initializationsecondstageinfo_tgfx_handle(*Create_GFXInitializationSecondStageInfo)(
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
        extension_tgfx_listhandle EXTList);
    void (*GetMonitor_Resolution_ColorBites_RefreshRate)(monitor_tgfx_handle MonitorHandle, unsigned int* WIDTH, unsigned int* HEIGHT, unsigned int* ColorBites, unsigned int* RefreshRate);
    //Barrier Dependency Helpers

    waitsignaldescription_tgfx_handle(*CreateWaitSignal_DrawIndirectConsume)();
    waitsignaldescription_tgfx_handle(*CreateWaitSignal_VertexInput)	(unsigned char IndexBuffer, unsigned char VertexAttrib);
    waitsignaldescription_tgfx_handle(*CreateWaitSignal_VertexShader)	(unsigned char UniformRead, unsigned char StorageRead, unsigned char StorageWrite);
    waitsignaldescription_tgfx_handle(*CreateWaitSignal_FragmentShader) (unsigned char UniformRead, unsigned char StorageRead, unsigned char StorageWrite);
    waitsignaldescription_tgfx_handle(*CreateWaitSignal_ComputeShader)	(unsigned char UniformRead, unsigned char StorageRead, unsigned char StorageWrite);
    waitsignaldescription_tgfx_handle(*CreateWaitSignal_FragmentTests)	(unsigned char isEarly, unsigned char isRead, unsigned char isWrite);
    //Don't remember how a transfer signal should be, just added for now
    waitsignaldescription_tgfx_handle(*CreateWaitSignal_Transfer)       (unsigned char UniformRead, unsigned char StorageRead);

    //RENDERNODE HELPERS


    //WaitInfos is a pointer because function expects a list of waits (Color attachment output and also VertexShader-UniformReads etc)
    passwaitdescription_tgfx_handle(*CreatePassWait_DrawPass)(drawpass_tgfx_handle* PassHandle, unsigned char SubpassIndex,
        waitsignaldescription_tgfx_handle* WaitInfos, unsigned char isLastFrame);
    //WaitInfo is single, because function expects only one wait and it should be created with CreateWaitSignal_ComputeShader()
    passwaitdescription_tgfx_handle(*CreatePassWait_ComputePass)(computepass_tgfx_handle* PassHandle, unsigned char SubpassIndex,
        waitsignaldescription_tgfx_handle WaitInfo, unsigned char isLastFrame);
    //WaitInfo is single, because function expects only one wait and it should be created with CreateWaitSignal_Transfer()
    passwaitdescription_tgfx_handle(*CreatePassWait_TransferPass)(transferpass_tgfx_handle* PassHandle,
        transferpasstype_tgfx Type, waitsignaldescription_tgfx_handle WaitInfo, unsigned char isLastFrame);
    //There is no option because you can only wait for a penultimate window pass
    //I'd like to support last frame wait too but it confuses the users and it doesn't have much use
    passwaitdescription_tgfx_handle(*CreatePassWait_WindowPass)(windowpass_tgfx_handle* PassHandle);

    subdrawpassdescription_tgfx_handle(*CreateSubDrawPassDescription)(inheritedrtslotset_tgfx_handle irtslotset, subdrawpassaccess_tgfx WaitOP, subdrawpassaccess_tgfx ContinueOP);


    shaderstageflag_tgfx_handle(*CreateShaderStageFlag)(unsigned char count, ...);
    shaderinputdescription_tgfx_handle(*CreateShaderInputDescription)(shaderinputtype_tgfx Type, unsigned int BINDINDEX,
        unsigned int ELEMENTCOUNT, shaderstageflag_tgfx_handle Stages);
    rtslotdescription_tgfx_handle(*CreateRTSlotDescription_Color)(texture_tgfx_handle Texture0, texture_tgfx_handle Texture1,
        operationtype_tgfx OPTYPE, drawpassload_tgfx LOADTYPE, unsigned char isUsedLater, unsigned char SLOTINDEX, vec4_tgfx CLEARVALUE);
    rtslotdescription_tgfx_handle(*CreateRTSlotDescription_DepthStencil)(texture_tgfx_handle Texture0, texture_tgfx_handle Texture1,
        operationtype_tgfx DEPTHOP, drawpassload_tgfx DEPTHLOAD, operationtype_tgfx STENCILOP, drawpassload_tgfx STENCILLOAD,
        float DEPTHCLEARVALUE, unsigned char STENCILCLEARVALUE);
    rtslotusage_tgfx_handle(*CreateRTSlotUsage_Color)(rtslotdescription_tgfx_handle base_slot, operationtype_tgfx OPTYPE, drawpassload_tgfx LOADTYPE);
    rtslotusage_tgfx_handle(*CreateRTSlotUsage_Depth)(rtslotdescription_tgfx_handle base_slot, operationtype_tgfx DEPTHOP, drawpassload_tgfx DEPTHLOAD,
        operationtype_tgfx STENCILOP, drawpassload_tgfx STENCILLOAD);
    depthsettings_tgfx_handle(*CreateDepthConfiguration)(unsigned char ShouldWrite, depthtest_tgfx COMPAREOP);
    stencilsettings_tgfx_handle(*CreateStencilConfiguration)(unsigned char Reference, unsigned char WriteMask, unsigned char CompareMask,
        stencilcompare_tgfx CompareOP, stencilop_tgfx DepthFailOP, stencilop_tgfx StencilFailOP, stencilop_tgfx AllSuccessOP);
    blendinginfo_tgfx_handle(*CreateBlendingConfiguration)(unsigned char ColorSlotIndex, vec4_tgfx Constant, blendfactor_tgfx SRCFCTR_CLR,
        blendfactor_tgfx SRCFCTR_ALPHA, blendfactor_tgfx DSTFCTR_CLR, blendfactor_tgfx DSTFCTR_ALPHA, blendmode_tgfx BLENDOP_CLR,
        blendmode_tgfx BLENDOP_ALPHA, colorcomponents_tgfx WRITECHANNELs);
    
    //EXTENSION HELPERS

    void (*Destroy_ExtensionData)(extension_tgfx_handle ExtensionToDestroy);
    unsigned char (*DoesGPUsupportsVKDESCINDEXING)(gpu_tgfx_handle GPU);
} helper_tgfx;
