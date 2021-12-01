#pragma once

#include "PREPROCESSORs.inl"

#if defined(__cplusplus)
extern "C" {
#endif

	//OBJECTS
	TGFXOBJHANDLE(gpu)
		TGFXOBJHANDLE(window)
		TGFXOBJHANDLE(texture)
		TGFXOBJHANDLE(monitor)
		TGFXOBJHANDLE(drawpass)
		TGFXOBJHANDLE(computepass)
		TGFXOBJHANDLE(transferpass)
		TGFXOBJHANDLE(windowpass)
		TGFXOBJHANDLE(subdrawpass)
		TGFXOBJHANDLE(subcomputepass)
		TGFXOBJHANDLE(rtslotset)
		TGFXOBJHANDLE(buffer)
		TGFXOBJHANDLE(samplingtype)
		TGFXOBJHANDLE(vertexattributelayout)
		TGFXOBJHANDLE(shadersource)
		TGFXOBJHANDLE(computeshadertype)
		TGFXOBJHANDLE(computeshaderinstance)
		TGFXOBJHANDLE(rasterpipelinetype)
		TGFXOBJHANDLE(rasterpipelineinstance)
		TGFXOBJHANDLE(inheritedrtslotset)




		//DATAS
		TGFXDATAHANDLE(extension)
		TGFXDATAHANDLE(textureusageflag)
		TGFXDATAHANDLE(waitsignaldescription)
		TGFXDATAHANDLE(passwaitdescription)
		TGFXDATAHANDLE(subdrawpassdescription)
		TGFXDATAHANDLE(vertexattributelist)
		TGFXDATAHANDLE(shaderstageflag)
		TGFXDATAHANDLE(rtslotdescription)
		TGFXDATAHANDLE(rtslotusage)
		TGFXDATAHANDLE(stencilsettings)
		TGFXDATAHANDLE(depthsettings)
		TGFXDATAHANDLE(memorytype)
		TGFXDATAHANDLE(initializationsecondstageinfo)
		TGFXDATAHANDLE(shaderinputdescription)
		TGFXDATAHANDLE(blendinginfo)



		//LISTS
		TGFXHANDLELIST(waitsignaldescription)
		TGFXHANDLELIST(passwaitdescription)
		TGFXHANDLELIST(monitor)
		TGFXHANDLELIST(gpu)
		TGFXHANDLELIST(extension)
		TGFXHANDLELIST(subdrawpassdescription)
		TGFXHANDLELIST(rtslotdescription)
		TGFXHANDLELIST(rtslotusage)
		TGFXHANDLELIST(memorytype)
		TGFXHANDLELIST(subdrawpass)
		TGFXHANDLELIST(shadersource)
		TGFXHANDLELIST(shaderinputdescription)
		TGFXHANDLELIST(blendinginfo)



		//These are very basic data types that doesn't mean anything special

		typedef struct tgfx_uvec3 {
		unsigned int x, y, z;
	} tgfx_uvec3;

	typedef struct tgfx_vec2 {
		float x, y;
	} tgfx_vec2;

	typedef struct tgfx_vec3 {
		float x, y, z;
	} tgfx_vec3;

	typedef struct tgfx_vec4 {
		float x, y, z, w;
	} tgfx_vec4;

	typedef struct tgfx_BoxRegion {
		unsigned int XOffset, YOffset, WIDTH, HEIGHT;
	} tgfx_BoxRegion;

	typedef struct tgfx_CubeRegion {
		unsigned int XOffset, YOffset, ZOffset, WIDTH, HEIGHT, DEPTH;
	} tgfx_CubeRegion;



	typedef enum tgfx_result {
		tgfx_result_SUCCESS = 0,
		tgfx_result_FAIL = 1,
		tgfx_result_NOTCODED = 2,
		tgfx_result_INVALIDARGUMENT = 3,
		tgfx_result_WRONGTIMING = 4,		//This means the operation is called at the wrong time!
		tgfx_result_WARNING = 5
	} tgfx_result;

	//Variable Types!
	typedef enum tgfx_datatype {
		tgfx_datatype_UNDEFINED = 0, tgfx_datatype_VAR_UBYTE8 = 1, tgfx_datatype_VAR_BYTE8 = 2, tgfx_datatype_VAR_UINT16 = 3,
		tgfx_datatype_VAR_INT16 = 4, tgfx_datatype_VAR_UINT32 = 5, tgfx_datatype_VAR_INT32 = 6, tgfx_datatype_VAR_FLOAT32,
		tgfx_datatype_VAR_VEC2, tgfx_datatype_VAR_VEC3, tgfx_datatype_VAR_VEC4, tgfx_datatype_VAR_MAT4x4
	} tgfx_datatype;

	typedef enum tgfx_cubeface {
		FRONT = 0,
		BACK = 1,
		LEFT,
		RIGHT,
		TOP,
		BOTTOM
	} tgfx_cubeface;

	typedef enum tgfx_operationtype {
		tgfx_operationtype_READ_ONLY,
		tgfx_operationtype_WRITE_ONLY,
		tgfx_operationtype_READ_AND_WRITE,
		tgfx_operationtype_UNUSED
	} tgfx_operationtype;

	typedef enum tgfx_drawpassload {
		//All values will be cleared to a certain value
		tgfx_drawpassload_CLEAR,
		//You don't need previous data, just gonna ignore and overwrite on them
		tgfx_drawpassload_FULL_OVERWRITE,
		//You need previous data, so previous data will affect current draw calls
		tgfx_drawpassload_LOAD
	} tgfx_drawpassload;

	typedef enum tgfx_depthtests {
		tgfx_DEPTH_TEST_ALWAYS,
		tgfx_DEPTH_TEST_NEVER,
		tgfx_DEPTH_TEST_LESS,
		tgfx_DEPTH_TEST_LEQUAL,
		tgfx_DEPTH_TEST_GREATER,
		tgfx_DEPTH_TEST_GEQUAL
	} tgfx_depthtests;

	typedef enum tgfx_buffertype {
		STAGING,
		VERTEX,
		INDEX,
		GLOBAL
	} tgfx_buffertype;

	typedef enum tgfx_depthmodes {
		tgfx_depthmode_READ_WRITE,
		tgfx_depthmode_READ_ONLY,
		tgfx_depthmode_OFF
	} tgfx_depthmodes;

	typedef enum tgfx_cullmode {
		CULL_OFF,
		CULL_BACK,
		CULL_FRONT
	} tgfx_cullmode;

	typedef enum tgfx_stencilcompare {
		OFF = 0,
		NEVER_PASS,
		LESS_PASS,
		LESS_OR_EQUAL_PASS,
		EQUAL_PASS,
		NOTEQUAL_PASS,
		GREATER_OR_EQUAL_PASS,
		GREATER_PASS,
		ALWAYS_PASS,
	} tgfx_stencilcompare;

	typedef enum tgfx_stencilop {
		tgfx_stencilop_DONT_CHANGE = 0,
		tgfx_stencilop_SET_ZERO = 1,
		tgfx_stencilop_CHANGE = 2,
		tgfx_stencilop_CLAMPED_INCREMENT,
		tgfx_stencilop_WRAPPED_INCREMENT,
		tgfx_stencilop_CLAMPED_DECREMENT,
		tgfx_stencilop_WRAPPED_DECREMENT,
		tgfx_stencilop_BITWISE_INVERT
	} tgfx_stencilop;



	typedef enum tgfx_blendfactor {
		tgfx_blendfactor_ONE = 0,
		tgfx_blendfactor_ZERO = 1,
		tgfx_blendfactor_SRC_COLOR,
		tgfx_blendfactor_SRC_1MINUSCOLOR,
		tgfx_blendfactor_SRC_ALPHA,
		tgfx_blendfactor_SRC_1MINUSALPHA,
		tgfx_blendfactor_DST_COLOR,
		tgfx_blendfactor_DST_1MINUSCOLOR,
		tgfx_blendfactor_DST_ALPHA,
		tgfx_blendfactor_DST_1MINUSALPHA,
		tgfx_blendfactor_CONST_COLOR,
		tgfx_blendfactor_CONST_1MINUSCOLOR,
		tgfx_blendfactor_CONST_ALPHA,
		tgfx_blendfactor_CONST_1MINUSALPHA
	} tgfx_blendfactor;

	typedef enum tgfx_colorcomponents {
		tgfx_colorcomponents_NONE = 0,
		tgfx_colorcomponents_ALL = 1,
		tgfx_colorcomponents_R = 2,
		tgfx_colorcomponents_G = 3,
		tgfx_colorcomponents_B = 4,
		tgfx_colorcomponents_RG = 5,
		tgfx_colorcomponents_GB = 6,
		tgfx_colorcomponents_RB = 7,
		tgfx_colorcomponents_RGB = 8,
	} tgfx_colorcomponents;

	typedef enum tgfx_blendmode {
		tgfx_blendmode_ADDITIVE,
		tgfx_blendmode_SUBTRACTIVE,
		tgfx_blendmode_SUBTRACTIVE_SWAPPED,
		tgfx_blendmode_MIN,
		tgfx_blendmode_MAX
	} tgfx_blendmode;


	typedef enum tgfx_polygonmode {
		tgfx_polygonmode_FILL,
		tgfx_polygonmode_LINE,
		tgfx_polygonmode_POINT
	} tgfx_polygonmode;

	typedef enum tgfx_vertexlisttypes {
		TRIANGLELIST,
		TRIANGLESTRIP,
		LINELIST,
		LINESTRIP,
		POINTLIST
	} tgfx_vertexlisttypes;

	typedef enum tgfx_rendernode_types {
		tgfx_rendernode_types_SUBDRAWPASS,
		tgfx_rendernode_types_COMPUTEPASS,
		tgfx_rendernode_types_TRANSFERPASS
	} tgfx_rendernode_types;

	typedef enum tgfx_texture_dimensions {
		TEXTURE_2D = 0,
		TEXTURE_3D = 1,
		TEXTURE_CUBE = 2
	}  tgfx_texture_dimensions;

	typedef enum tgfx_texture_mipmapfilter {
		NEAREST_FROM_1MIP,
		LINEAR_FROM_1MIP,
		NEAREST_FROM_2MIP,
		LINEAR_FROM_2MIP
	}  tgfx_texture_mipmapfilter;

	typedef enum tgfx_texture_order {
		tgfx_texture_order_SWIZZLE = 0,
		tgfx_texture_order_LINEAR = 1
	}  tgfx_texture_order;

	typedef enum tgfx_texture_wrapping {
		tgfx_texture_wrapping_REPEAT,
		tgfx_texture_wrapping_MIRRORED_REPEAT,
		tgfx_texture_wrapping_CLAMP_TO_EDGE
	} tgfx_texture_wrapping;

	typedef enum tgfx_texture_channels {
		tgfx_texture_channels_BGRA8UB,	//Unsigned but non-normalized char
		tgfx_texture_channels_BGRA8UNORM,	//Unsigned and normalized char

		tgfx_texture_channels_RGBA32F,
		tgfx_texture_channels_RGBA32UI,
		tgfx_texture_channels_RGBA32I,
		tgfx_texture_channels_RGBA8UB,
		tgfx_texture_channels_RGBA8B,

		tgfx_texture_channels_RGB32F,
		tgfx_texture_channels_RGB32UI,
		tgfx_texture_channels_RGB32I,
		tgfx_texture_channels_RGB8UB,
		tgfx_texture_channels_RGB8B,

		tgfx_texture_channels_RA32F,
		tgfx_texture_channels_RA32UI,
		tgfx_texture_channels_RA32I,
		tgfx_texture_channels_RA8UB,
		tgfx_texture_channels_RA8B,

		tgfx_texture_channels_R32F,
		tgfx_texture_channels_R32UI,
		tgfx_texture_channels_R32I,
		tgfx_texture_channels_R8UB,
		tgfx_texture_channels_R8B,

		tgfx_texture_channels_D32,
		tgfx_texture_channels_D24S8
	}  tgfx_texture_channels;

	typedef enum tgfx_texture_access {
		SAMPLER_OPERATION,
		IMAGE_OPERATION,
	} tgfx_texture_access;


	typedef enum tgfx_gpu_types {
		tgfx_DISCRETE_GPU,
		tgfx_INTEGRATED_GPU
	} tgfx_gpu_types;

	typedef enum tgfx_vsync {
		VSYNC_OFF,
		VSYNC_DOUBLEBUFFER,
		VSYNC_TRIPLEBUFFER
	} tgfx_vsync;

	typedef enum tgfx_windowmode {
		tgfx_windowmode_FULLSCREEN,
		tgfx_windowmode_WINDOWED
	} tgfx_windowmode;

	typedef enum tgfx_backends {
		tgfx_backends_OPENGL4 = 0,
		tgfx_backends_VULKAN = 1
	} tgfx_backends;

	typedef enum tgfx_shaderlanguages {
		tgfx_shaderlanguages_GLSL = 0,
		tgfx_shaderlanguages_HLSL = 1,
		tgfx_shaderlanguages_SPIRV = 2,
		tgfx_shaderlanguages_TSL = 3
	} tgfx_shaderlanguages;

	typedef enum tgfx_shaderstage {
		tgfx_shaderstage_VERTEXSHADER,
		tgfx_shaderstage_FRAGMENTSHADER
	} tgfx_shaderstage;



	typedef enum tgfx_barrierplace {
		tgfx_barrierplace_ONLYSTART = 0,			//Barrier is used only at the start of the pass
		tgfx_barrierplace_BEFORE_EVERYCALL = 1,	//Barrier is used before everycall (So, at the start of the pass too)
		tgfx_barrierplace_BETWEEN_EVERYCALL = 2,	//Barrier is only used between everycall (So, it's used neither at the start nor at the end)
		tgfx_barrierplace_AFTER_EVERYCALL = 3,	//Barrier is used after everycall (So, at the end of the end too)
		tgfx_barrierplace_ONLYEND = 4				//Barrier is used only at the end the pass
	} tgfx_barrierplace;
	typedef enum tgfx_transferpass_type {
		tgfx_transferpass_type_BARRIER = 0,
		tgfx_transferpass_type_COPY = 1
	} tgfx_transferpass_type;

	typedef enum tgfx_imageaccess {
		tgfx_imageaccess_RTCOLOR_READONLY = 0,
		tgfx_imageaccess_RTCOLOR_WRITEONLY = 1,
		tgfx_imageaccess_RTCOLOR_READWRITE,
		tgfx_imageaccess_SWAPCHAIN_DISPLAY,
		tgfx_imageaccess_TRANSFER_DIST,
		tgfx_imageaccess_TRANSFER_SRC,
		tgfx_imageaccess_NO_ACCESS,
		tgfx_imageaccess_SHADER_SAMPLEONLY,
		tgfx_imageaccess_SHADER_WRITEONLY,
		tgfx_imageaccess_SHADER_SAMPLEWRITE,
		tgfx_imageaccess_DEPTHSTENCIL_READONLY,
		tgfx_imageaccess_DEPTHSTENCIL_WRITEONLY,
		tgfx_imageaccess_DEPTHSTENCIL_READWRITE,
		tgfx_imageaccess_DEPTH_READONLY,
		tgfx_imageaccess_DEPTH_WRITEONLY,
		tgfx_imageaccess_DEPTH_READWRITE,
		tgfx_imageaccess_DEPTHREAD_STENCILREADWRITE,
		tgfx_imageaccess_DEPTHREAD_STENCILWRITE,
		tgfx_imageaccess_DEPTHWRITE_STENCILREAD,
		tgfx_imageaccess_DEPTHWRITE_STENCILREADWRITE,
		tgfx_imageaccess_DEPTHREADWRITE_STENCILREAD,
		tgfx_imageaccess_DEPTHREADWRITE_STENCILWRITE
	} tgfx_imageaccess;

	typedef enum tgfx_subdrawpass_access {
		tgfx_subdrawpass_access_ALLCOMMANDS,
		tgfx_subdrawpass_access_INDEX_READ,
		tgfx_subdrawpass_access_VERTEXATTRIB_READ,
		tgfx_subdrawpass_access_VERTEXUBUFFER_READONLY,
		tgfx_subdrawpass_access_VERTEXSBUFFER_READONLY,
		tgfx_subdrawpass_access_VERTEXSBUFFER_READWRITE,
		tgfx_subdrawpass_access_VERTEXSBUFFER_WRITEONLY,
		tgfx_subdrawpass_access_VERTEXSAMPLED_READONLY,
		tgfx_subdrawpass_access_VERTEXIMAGE_READONLY,
		tgfx_subdrawpass_access_VERTEXIMAGE_READWRITE,
		tgfx_subdrawpass_access_VERTEXIMAGE_WRITEONLY,
		tgfx_subdrawpass_access_VERTEXINPUTS_READONLY,
		tgfx_subdrawpass_access_VERTEXINPUTS_READWRITE,
		tgfx_subdrawpass_access_VERTEXINPUTS_WRITEONLY,
		tgfx_subdrawpass_access_EARLY_Z_READ,
		tgfx_subdrawpass_access_EARLY_Z_READWRITE,
		tgfx_subdrawpass_access_EARLY_Z_WRITEONLY,
		tgfx_subdrawpass_access_FRAGMENTUBUFFER_READONLY,
		tgfx_subdrawpass_access_FRAGMENTSBUFFER_READONLY,
		tgfx_subdrawpass_access_FRAGMENTSBUFFER_READWRITE,
		tgfx_subdrawpass_access_FRAGMENTSBUFFER_WRITEONLY,
		tgfx_subdrawpass_access_FRAGMENTSAMPLED_READONLY,
		tgfx_subdrawpass_access_FRAGMENTIMAGE_READONLY,
		tgfx_subdrawpass_access_FRAGMENTIMAGE_READWRITE,
		tgfx_subdrawpass_access_FRAGMENTIMAGE_WRITEONLY,
		tgfx_subdrawpass_access_FRAGMENTINPUTS_READONLY,
		tgfx_subdrawpass_access_FRAGMENTINPUTS_READWRITE,
		tgfx_subdrawpass_access_FRAGMENTINPUTS_WRITEONLY,
		tgfx_subdrawpass_access_FRAGMENTRT_READONLY,
		tgfx_subdrawpass_access_LATE_Z_READ,
		tgfx_subdrawpass_access_LATE_Z_READWRITE,
		tgfx_subdrawpass_access_LATE_Z_WRITEONLY,
		tgfx_subdrawpass_access_FRAGMENTRT_WRITEONLY
	} tgfx_subdrawpass_access;



	//Appendix "PI" means per material instance, "G" means material type general
	//If there is any PI data type in Material Type, we assume that you're accessing it in shader with MaterialInstance index (Back-end handles that)
	//If there is no title, that means it's modifiable and possibly includes a double buffered implementation in GL back-end
	//CONST title means it can't be changed after the creation
	typedef enum tgfx_shaderinput_type {
		tgfx_shaderinput_type_UNDEFINED = 0,
		//DRAWCALL_DESCRIPTOR = 1,
		tgfx_shaderinput_type_SAMPLER_PI,
		tgfx_shaderinput_type_SAMPLER_G,
		tgfx_shaderinput_type_IMAGE_PI,
		tgfx_shaderinput_type_IMAGE_G,
		tgfx_shaderinput_type_UBUFFER_PI,
		tgfx_shaderinput_type_UBUFFER_G,
		tgfx_shaderinput_type_SBUFFER_PI,
		tgfx_shaderinput_type_SBUFFER_G
	} tgfx_shaderinput_type;

	typedef enum tgfx_memoryallocationtype {
		DEVICELOCAL = 0,
		HOSTVISIBLE = 1,
		FASTHOSTVISIBLE = 2,
		READBACK = 3
	} tgfx_memoryallocationtype;

	/*
	struct GFXAPI Texture_Properties {
		texture_mipmapfilter MIPMAP_FILTERING = texture_mipmapfilter::API_TEXTURE_LINEAR_FROM_1MIP;
		TEXTURE_WRAPPING WRAPPING = TEXTURE_WRAPPING::API_TEXTURE_REPEAT;
		Texture_Properties()
		Texture_Properties(TEXTURE_DIMENSIONs dimension, texture_mipmapfilter mipmap_filtering = texture_mipmapfilter::API_TEXTURE_LINEAR_FROM_1MIP,
			TEXTURE_WRAPPING wrapping = TEXTURE_WRAPPING::API_TEXTURE_REPEAT, TEXTURE_CHANNELs channel_type = TEXTURE_CHANNELs::API_TEXTURE_RGB8UB, TEXTURE_ORDER = TEXTURE_ORDER::SWIZZLE)
	}*/

	/*
		Texture Resource Specifications:
			1) You can use textures to just read on GPU (probably for object material rendering), read-write on GPU (using compute shader to write an image), render target (framebuffer attachment)
			2) Modern APIs let you use a texture in anyway, it's left for your care.
			But OpenGL doesn't let this situation, you are defining your texture's type in creation proccess
			So for now, GFXAPI uses Modern APIs' way and you can use a texture in anyway you want
			But you should use UsageBarrier for this kind of usage and specify it in RenderGraph
			3) You can't use Staging Buffers to store and access textures, they are just for transfer operations
	*/
	/*
typedef struct GPUDescription) {
	const char* MODEL;
	unsigned int API_VERSION, DRIVER_VERSION;
	tgfx_gpu_types GPU_TYPE;
	unsigned char is_GraphicOperations_Supported, is_ComputeOperations_Supported, is_TransferOperations_Supported;
	//const tgfx_memorytype* MEMTYPEs;
	unsigned char MEMTYPEsCOUNT;
	unsigned long long DEVICELOCAL_MaxMemorySize, HOSTVISIBLE_MaxMemorySize, FASTHOSTVISIBLE_MaxMemorySize, READBACK_MaxMemorySize;
	unsigned char isSupported_SeperateDepthStencilLayouts, isSupported_SeperateRTSlotBlending,
		isSupported_NonUniformShaderInputIndexing;
	//These limits are maximum count of usable resources in a shader stage (VS, FS etc.)
	//Don't forget that sum of the accesses in a shader stage shouldn't exceed MaxUsableResources_perStage!
	unsigned int MaxSampledTexture_perStage, MaxImageTexture_perStage, MaxUniformBuffer_perStage, MaxStorageBuffer_perStage, MaxUsableResources_perStage;
	unsigned int MaxShaderInput_SampledTexture, MaxShaderInput_ImageTexture, MaxShaderInput_UniformBuffer, MaxShaderInput_StorageBuffer;
} ;GPUDescription)
*/

	typedef void (*tgfx_windowResizeCallback)(tgfx_window WindowHandle, void* UserPointer, unsigned int WIDTH, unsigned int HEIGHT,
		tgfx_texture* SwapchainTextureHandles);
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

#define TGFXCreateExtensionList(HelperClassHandle, ...) HelperClassHandle##.CreateExtensionList(__VA_ARGS__, NULL)
