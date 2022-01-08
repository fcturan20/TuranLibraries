#include "GFX_Includes.h"

#if defined(__cplusplus)
extern "C"{
#endif

	typedef struct tgfx_helper {
		//Hardware Capability Helpers
		void (*GetGPUInfo_General)(tgfx_gpu GPUHandle, const char** NAME, unsigned int* API_VERSION, unsigned int* DRIVER_VERSION, tgfx_gpu_types* GPUTYPE, tgfx_memorytype_list* MemTypes,
			unsigned char* isGraphicsOperationsSupported, unsigned char* isComputeOperationsSupported, unsigned char* isTransferOperationsSupported);
		void (*GetGPUInfo_Memory)(tgfx_memorytype MemoryType, tgfx_memoryallocationtype* AllocType, unsigned long long* MaxAllocSize);
		unsigned char (*GetTextureTypeLimits)(tgfx_texture_dimensions dims, tgfx_texture_order dataorder, tgfx_texture_channels channeltype,
			tgfx_textureusageflag usageflag, tgfx_gpu GPUHandle, unsigned int* MAXWIDTH, unsigned int* MAXHEIGHT, unsigned int* MAXDEPTH,
			unsigned int* MAXMIPLEVEL);
		tgfx_textureusageflag(*CreateTextureUsageFlag)(unsigned char isCopiableFrom, unsigned char isCopiableTo,
			unsigned char isRenderableTo, unsigned char isSampledReadOnly, unsigned char isRandomlyWrittenTo);
		void (*GetSupportedAllocations_ofTexture)(unsigned int GPUIndex, unsigned int* SupportedMemoryTypesBitset);
		//You can't create a memory type, only set allocation size of a given Memory Type (Memory Type is given by the GFX initialization process)
		tgfx_result(*SetMemoryTypeInfo)(tgfx_memorytype MemoryType, unsigned long long AllocationSize, tgfx_extension_list Extensions);
		tgfx_initializationsecondstageinfo(*Create_GFXInitializationSecondStageInfo)(
			tgfx_gpu RendererGPU,
			//You have to specify how much shader input categories you're gonna use max (material types that have general shader inputs + material instances that have per instance shader inputs)
			unsigned int MaterialCount,
			//You have to specify max sum of how much shader inputs you're gonna use for materials (General and Per Instance)
			unsigned int MaxSumMaterial_SampledTexture, unsigned int MaxSumMaterial_ImageTexture, unsigned int MaxSumMaterial_UniformBuffer, unsigned int MaxSumMaterial_StorageBuffer,
			//You have to specify how many global shader inputs you're gonna use (Don't forget to look at descriptor indexing extensions)
			unsigned int GlobalSampledTextureInputCount, unsigned int GlobalImageTextureInputCount, unsigned int GlobalUniformBufferInputCount, unsigned int GlobalInputStorageBufferInputCount,
			//See the documentation
			unsigned char isGlobalUniformBuffer_Index1, unsigned char isGlobalSampledTexture_Index1,
			unsigned char ShouldActive_dearIMGUI,
			tgfx_extension_list EXTList);
		void (*GetMonitor_Resolution_ColorBites_RefreshRate)(tgfx_monitor MonitorHandle, unsigned int* WIDTH, unsigned int* HEIGHT, unsigned int* ColorBites, unsigned int* RefreshRate);
		//Barrier Dependency Helpers

		tgfx_waitsignaldescription(*CreateWaitSignal_DrawIndirectConsume)();
		tgfx_waitsignaldescription(*CreateWaitSignal_VertexInput)	(unsigned char IndexBuffer, unsigned char VertexAttrib);
		tgfx_waitsignaldescription(*CreateWaitSignal_VertexShader)	(unsigned char UniformRead, unsigned char StorageRead, unsigned char StorageWrite);
		tgfx_waitsignaldescription(*CreateWaitSignal_FragmentShader)(unsigned char UniformRead, unsigned char StorageRead, unsigned char StorageWrite);
		tgfx_waitsignaldescription(*CreateWaitSignal_ComputeShader)	(unsigned char UniformRead, unsigned char StorageRead, unsigned char StorageWrite);
		tgfx_waitsignaldescription(*CreateWaitSignal_FragmentTests)	(unsigned char isEarly, unsigned char isRead, unsigned char isWrite);

		//RENDERNODE HELPERS

		//WaitInfos is a pointer because function expects a list of waits (Color attachment output and also VertexShader-UniformReads etc)
		tgfx_passwaitdescription(*CreatePassWait_DrawPass)(tgfx_drawpass* PassHandle, unsigned char SubpassIndex,
			tgfx_waitsignaldescription* WaitInfos, unsigned char isLastFrame);
		//WaitInfo is single, because function expects only one wait and it should be created with CreateWaitSignal_ComputeShader()
		tgfx_passwaitdescription(*CreatePassWait_ComputePass)(tgfx_computepass* PassHandle, unsigned char SubpassIndex,
			tgfx_waitsignaldescription WaitInfo, unsigned char isLastFrame);
		//WaitInfo is single, because function expects only one wait and it should be created with CreateWaitSignal_Transfer()
		tgfx_passwaitdescription(*CreatePassWait_TransferPass)(tgfx_transferpass* PassHandle,
			tgfx_transferpass_type Type, tgfx_waitsignaldescription WaitInfo, unsigned char isLastFrame);
		//There is no option because you can only wait for a penultimate window pass
		//I'd like to support last frame wait too but it confuses the users and it doesn't have much use
		tgfx_passwaitdescription(*CreatePassWait_WindowPass)(tgfx_windowpass* PassHandle);

		tgfx_subdrawpassdescription(*CreateSubDrawPassDescription)();

		tgfx_shaderinputdescription(*CreateShaderInputDescription)(unsigned char isGeneral, tgfx_shaderinput_type Type, unsigned int BINDINDEX,
			unsigned int ELEMENTCOUNT, tgfx_shaderstageflag Stages);
		tgfx_rtslotdescription(*CreateRTSlotDescription_Color)(tgfx_texture Texture0, tgfx_texture Texture1,
			tgfx_operationtype OPTYPE, tgfx_drawpassload LOADTYPE, unsigned char isUsedLater, unsigned char SLOTINDEX, tgfx_vec4 CLEARVALUE);
		tgfx_rtslotdescription(*CreateRTSlotDescription_DepthStencil)(tgfx_texture Texture0, tgfx_texture Texture1,
			tgfx_operationtype DEPTHOP, tgfx_drawpassload DEPTHLOAD, tgfx_operationtype STENCILOP, tgfx_drawpassload STENCILLOAD,
			float DEPTHCLEARVALUE, unsigned char STENCILCLEARVALUE);
		tgfx_rtslotusage(*CreateRTSlotUsage_Color)(unsigned char SLOTINDEX, tgfx_operationtype OPTYPE, tgfx_drawpassload LOADTYPE);
		tgfx_rtslotusage(*CreateRTSlotUsage_Depth)(tgfx_operationtype DEPTHOP, tgfx_drawpassload DEPTHLOAD,
			tgfx_operationtype STENCILOP, tgfx_drawpassload STENCILLOAD);
		tgfx_depthsettings(*CreateDepthConfiguration)(unsigned char ShouldWrite, tgfx_depthtests COMPAREOP);
		tgfx_stencilsettings(*CreateStencilConfiguration)(unsigned char Reference, unsigned char WriteMask, unsigned char CompareMask,
			tgfx_stencilcompare CompareOP, tgfx_stencilop DepthFailOP, tgfx_stencilop StencilFailOP, tgfx_stencilop AllSuccessOP);
		tgfx_blendinginfo(*CreateBlendingConfiguration)(unsigned char ColorSlotIndex, tgfx_vec4 Constant, tgfx_blendfactor SRCFCTR_CLR,
			tgfx_blendfactor SRCFCTR_ALPHA, tgfx_blendfactor DSTFCTR_CLR, tgfx_blendfactor DSTFCTR_ALPHA, tgfx_blendmode BLENDOP_CLR,
			tgfx_blendmode BLENDOP_ALPHA, tgfx_colorcomponents WRITECHANNELs);

		//EXTENSION HELPERS

		void (*Destroy_ExtensionData)(tgfx_extension ExtensionToDestroy);
		unsigned char (*DoesGPUsupportsVKDESCINDEXING)(tgfx_gpu GPU);
	} tgfx_helper;

	//Enumeration Functions
	GFXAPI unsigned int tgfx_Get_UNIFORMTYPEs_SIZEinbytes(tgfx_datatype uniform);
	unsigned char tgfx_GetByteSizeOf_TextureChannels(tgfx_texture_channels channeltype);

#if defined(__cplusplus)
}
#endif

#define TGFXCreateExtensionList(HelperClassHandle, ...) HelperClassHandle##.CreateExtensionList(__VA_ARGS__, INVALID)
