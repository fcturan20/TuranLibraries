#pragma once
#include <TCoreMacros.h>
TCORE_BEGIN_C_LINKAGE

typedef int textureUsageMask_tgfxflag;
typedef int bufferUsageMask_tgfxflag;
typedef int shaderStage_tgfxflag;
struct tgfx_uvec4
{
	unsigned int x, y, z, w;
};

struct tgfx_uvec3
{
	unsigned int x, y, z;
};

struct tgfx_uvec2
{
	unsigned int x, y;
};

struct tgfx_vec2
{
	float x, y;
};

struct tgfx_vec3
{
	float x, y, z;
};

struct tgfx_vec4
{
	float x, y, z, w;
};

struct tgfx_ivec2
{
	int x, y;
};

struct tgfx_ivec3
{
	int x, y, z;
};

struct tgfx_boxRegion
{
	unsigned int XOffset, YOffset, WIDTH, HEIGHT;
};

struct tgfx_cubeRegion
{
	unsigned int XOffset, YOffset, ZOffset, WIDTH, HEIGHT, DEPTH;
};

struct tgfx_memoryDescription
{
	unsigned char memoryTypeId;
	enum memoryallocationtype_tgfx allocationType;
	unsigned long maxAllocationSize;
};

struct tgfx_gpuDescription
{
	const wchar_t* name;
	unsigned int gfxApiVersion, driverVersion;
	enum gpu_type_tgfx type;
	unsigned char operationSupport_raster, operationSupport_compute, operationSupport_transfer, queueFamilyCount;
	const struct tgfx_memoryDescription* memRegions;
	unsigned char memRegionsCount;
};

typedef void (*tgfx_windowResizeCallback)(struct tgfx_window* windowHnd,
										  void* userPtr,
										  tgfx_uvec2 resolution,
										  struct tgfx_texture** swapchainTextures);
// @param scanCode: System-specific scan code
typedef void (*tgfx_windowKeyCallback)(struct tgfx_window* windowHnd,
									   void* userPointer,
									   enum key_tgfx key,
									   int scanCode,
									   enum keyAction_tgfx action,
									   enum keyMod_tgfx mode);
typedef void (*tgfx_windowCloseCallback)(struct tgfx_window* windowHnd, void* userPtr);

struct tgfx_windowDescription
{
	struct tgfx_uvec2 size;
	struct tgfx_monitor* monitor;
	enum windowmode_tgfx mode;
	const wchar_t* name;
	tgfx_windowResizeCallback resizeCb;
	tgfx_windowKeyCallback keyCb;
	tgfx_windowCloseCallback closeCb;
};

struct tgfx_swapchainDescription
{
	struct tgfx_window* window;
	enum windowpresentation_tgfx presentationMode;
	enum windowcomposition_tgfx composition;
	enum colorspace_tgfx colorSpace;
	enum textureChannels_tgfx channels;
	textureUsageMask_tgfxflag swapchainUsage;
	unsigned int permittedQueueCount;
	struct tgfx_gpuQueue* const* permittedQueues;
	unsigned int imageCount;
};

#define TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT 24
#define TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT 64
#define TGFX_WINDOWGPUSUPPORT_MAXPRESENTATIONMODE 6
struct tgfx_windowGPUsupport
{
	unsigned int maxImageCount;
	struct tgfx_uvec2 minExtent, maxExtent;
	textureUsageMask_tgfxflag usageFlag;
	enum windowpresentation_tgfx presentationModes[TGFX_WINDOWGPUSUPPORT_MAXPRESENTATIONMODE];
	enum colorspace_tgfx colorSpace[TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT];
	enum textureChannels_tgfx channels[TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT];
	struct tgfx_gpuQueue* queues[TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT];
};

struct tgfx_samplerDescription
{
	unsigned int minMipLevel, maxMipLevel;
	enum texture_mipmapfilter_tgfx minFilter, magFilter;
	enum texture_wrapping_tgfx wrapWidth, wrapHeight, wrapDepth;
	struct tgfx_uvec4 bordercolor;
};

struct tgfx_textureDescription
{
	enum texture_dimensions_tgfx dimension;
	struct tgfx_uvec2 resolution;
	enum textureChannels_tgfx channelType;
	unsigned char mipCount;
	textureUsageMask_tgfxflag usage;
	enum textureOrder_tgfx dataOrder;
	unsigned int permittedQueueCount;
	struct tgfx_gpuQueue* const* permittedQueues;
};

struct tgfx_bufferDescription
{
	unsigned int dataSize;
	bufferUsageMask_tgfxflag usageFlag;
	unsigned int permittedQueueCount;
	struct tgfx_gpuQueue* const* permittedQueues;
	unsigned int extCount;
	struct tgfx_extension* const* exts;
};

struct tgfx_bindingTableDescription
{
	enum shaderdescriptortype_tgfx descriptorType;
	unsigned int elementCount;
	shaderStage_tgfxflag visibleStagesMask;
	unsigned int staticSamplerCount;
	struct tgfx_sampler* const* staticSamplers;
	unsigned char isDynamic;
};

#define TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT 8
struct tgfx_stencilState
{
	enum stencilop_tgfx stencilFail, pass, depthFail;
	enum compare_tgfx compareOp;
	unsigned int compareMask, writeMask, reference;
};

struct tgfx_depthStencilState
{
	unsigned char depthTestEnabled, depthWriteEnabled, stencilTestEnabled;
	enum compare_tgfx depthCompare;
	struct tgfx_stencilState front, back;
};

struct tgfx_blendState
{
	unsigned char blendEnabled;
	enum blendfactor_tgfx srcColorFactor, dstColorFactor, srcAlphaFactor, dstAlphaFactor;
	enum blendmode_tgfx colorMode, alphaMode;
	enum textureComponentMask_tgfx blendComponents;
};

struct tgfx_rasterStateDescription
{
	enum cullmode_tgfx culling;
	enum polygonmode_tgfx polygonmode;
	struct tgfx_depthStencilState depthStencilState;
	struct tgfx_blendState blendStates[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT];
	enum vertexlisttypes_tgfx topology;
};

struct tgfx_heapRequirementsInfo
{
	// Single GPU can have max 32 regions
	// GPU should be same with the one used in CreateTexture()
	unsigned char memoryRegionIDs[32];
	unsigned int offsetAlignment;
	unsigned long long size;
};

// 32 byte data to store extreme colors (RGBA64)
struct tgfx_typelessColor
{
	char data[32];
};

struct tgfx_vertexAttributeDescription
{
	unsigned int attributeIndx, bindingIndx, offset;
	enum datatype_tgfx dataType;
};

struct tgfx_vertexBindingDescription
{
	unsigned int bindingIndx, stride;
	enum vertexBindingInputRate_tgfx inputRate;
};

// TGFX_SUBPASS_EXTENSION
struct tgfx_subpassSlotDescription
{
	enum rasterpassStore_tgfx storeType;
	enum rasterpassLoad_tgfx loadType;
	struct tgfx_typelessColor clearValue;
	enum textureChannels_tgfx format;
	enum image_access_tgfx layout;
};

struct tgfx_viewportInfo
{
	struct tgfx_vec2 topLeftCorner, size, depthMinMax;
};

struct tgfx_rasterInputAssemblerDescription
{
	unsigned int attribCount, bindingCount;
	const struct tgfx_vertexAttributeDescription* i_attributes;
	const struct tgfx_vertexBindingDescription* i_bindings;
};

struct tgfx_rasterPipelineDescription
{
	unsigned int shaderCount;
	struct tgfx_shaderSource* const* shaders;
	struct tgfx_rasterInputAssemblerDescription attribLayout;
	const tgfx_rasterStateDescription* mainStates;
	enum textureChannels_tgfx colorTextureFormats[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT];
	const struct tgfx_bindingTableDescription* tables;
	unsigned int tableCount;
	enum textureChannels_tgfx depthStencilTextureFormat;
	unsigned int extCount;
	struct tgfx_extension* const* exts;
	unsigned char pushConstantOffset, pushConstantSize;
};

struct tgfx_rasterpassBeginSlotInfo
{
	struct tgfx_texture* texture;
	enum image_access_tgfx imageAccess;
	enum rasterpassLoad_tgfx loadOp, loadStencilOp;
	enum rasterpassStore_tgfx storeOp, storeStencilOp;
	struct tgfx_typelessColor clearValue;
};

struct tgfx_drawIndexedIndirectArgument
{
	unsigned int indexCountPerInstance;
	unsigned int instanceCount;
	unsigned int firstIndex;
	int vertexOffset;
	unsigned int firstInstance;
};

struct tgfx_drawNonIndexedIndirectArgument
{
	unsigned int vertexCountPerInstance;
	unsigned int instanceCount;
	unsigned int firstVertex;
	unsigned int firstInstance;
};

struct tgfx_dispatchIndirectArgument
{
	struct tgfx_uvec3 threadGroupCount;
};

typedef enum TGfxDataType
{
	TGFX_DATATYPE_UNDEFINED = 0,
	TGFX_DATATYPE_VAR_UBYTE8 = 1,
	TGFX_DATATYPE_VAR_BYTE8 = 2,
	TGFX_DATATYPE_VAR_UINT16 = 3,
	TGFX_DATATYPE_VAR_INT16 = 4,
	TGFX_DATATYPE_VAR_UINT32 = 5,
	TGFX_DATATYPE_VAR_INT32 = 6,
	TGFX_DATATYPE_VAR_FLOAT32,
	TGFX_DATATYPE_VAR_VEC2,
	TGFX_DATATYPE_VAR_VEC3,
	TGFX_DATATYPE_VAR_VEC4,
	TGFX_DATATYPE_VAR_MAT4x4
} TGfxDataType;

typedef enum TGfxCubeFace
{
	TGFX_CUBEFACE_FRONT = 0,
	TGFX_CUBEFACE_BACK = 1,
	TGFX_CUBEFACE_LEFT,
	TGFX_CUBEFACE_RIGHT,
	TGFX_CUBEFACE_TOP,
	TGFX_CUBEFACE_BOTTOM,
	TGFX_CUBEFACE_ALL
} TGfxCubeFace;

typedef enum TGfxOperationType
{
	TGFX_OPERATIONTYPE_READ_ONLY,
	TGFX_OPERATIONTYPE_WRITE_ONLY,
	TGFX_OPERATIONTYPE_READ_AND_WRITE,
	TGFX_OPERATIONTYPE_UNUSED
} TGfxOperationType;

typedef enum TGfxRasterPassLoad
{
	// All values will be cleared to a certain value
	TGFX_RASTERPASSLOAD_CLEAR,
	// Loaded data is random (undef or current value) & driver probably clear
	TGFX_RASTERPASSLOAD_DISCARD,
	// You need previous data, so previous data will affect current draw calls
	TGFX_RASTERPASSLOAD_LOAD,
	// There won't be any access to previous data, use this for transient resources
	TGFX_RASTERPASSLOAD_NONE
} TGfxRasterPassLoad;

typedef enum TGfxRasterPassStore
{
	// Driver do whatever it wants with the data (either write or ignores it)
	TGFX_RASTERPASSSTORE_DISCARD,
	// Driver should write the data to memory
	TGFX_RASTERPASSSTORE_STORE,
	// Driver should ignore writing the data to memory, use this for transient resources
	TGFX_RASTERPASSSTORE_NONE
} TGfxRasterPassStore;

typedef enum TGfxCompare
{
	TGFX_COMPARE_ALWAYS,
	TGFX_COMPARE_NEVER,
	TGFX_COMPARE_LESS,
	TGFX_COMPARE_LEQUAL,
	TGFX_COMPARE_GREATER,
	TGFX_COMPARE_GEQUAL
} TGfxCompare;

typedef enum TGfxDepthMode
{
	TGFX_DEPTHMODE_READ_WRITE,
	TGFX_DEPTHMODE_READ_ONLY,
	TGFX_DEPTHMODE_OFF
} TGfxDepthMode;

typedef enum TGfxCullMode
{
	TGFX_CULLMODE_OFF,
	TGFX_CULLMODE_BACK,
	TGFX_CULLMODE_FRONT
} TGfxCullMode;

typedef enum TGfxStencilOp
{
	TGFX_STENCILOP_DONT_CHANGE = 0,
	TGFX_STENCILOP_SET_ZERO = 1,
	TGFX_STENCILOP_CHANGE = 2,
	TGFX_STENCILOP_CLAMPED_INCREMENT,
	TGFX_STENCILOP_WRAPPED_INCREMENT,
	TGFX_STENCILOP_CLAMPED_DECREMENT,
	TGFX_STENCILOP_WRAPPED_DECREMENT,
	TGFX_STENCILOP_BITWISE_INVERT
} TGfxStencilOp;

typedef enum TGfxBlendFactor
{
	TGFX_BLENDFACTOR_ONE = 0,
	TGFX_BLENDFACTOR_ZERO = 1,
	TGFX_BLENDFACTOR_SRC_COLOR,
	TGFX_BLENDFACTOR_SRC_1MINUSCOLOR,
	TGFX_BLENDFACTOR_SRC_ALPHA,
	TGFX_BLENDFACTOR_SRC_1MINUSALPHA,
	TGFX_BLENDFACTOR_DST_COLOR,
	TGFX_BLENDFACTOR_DST_1MINUSCOLOR,
	TGFX_BLENDFACTOR_DST_ALPHA,
	TGFX_BLENDFACTOR_DST_1MINUSALPHA,
	TGFX_BLENDFACTOR_CONST_COLOR,
	TGFX_BLENDFACTOR_CONST_1MINUSCOLOR,
	TGFX_BLENDFACTOR_CONST_ALPHA,
	TGFX_BLENDFACTOR_CONST_1MINUSALPHA
} TGfxBlendFactor;

typedef enum TGfxBlendMode
{
	TGFX_BLENDMODE_ADDITIVE,
	TGFX_BLENDMODE_SUBTRACTIVE,
	TGFX_BLENDMODE_SUBTRACTIVE_SWAPPED,
	TGFX_BLENDMODE_MIN,
	TGFX_BLENDMODE_MAX
} TGfxBlendMode;

typedef enum TGfxPolygonMode
{
	TGFX_POLYGONMODE_FILL,
	TGFX_POLYGONMODE_LINE,
	TGFX_POLYGONMODE_POINT
} TGfxPolygonMode;

typedef enum TGfxVertexListTypes
{
	TGFX_VERTEXLISTTYPES_TRIANGLELIST,
	TGFX_VERTEXLISTTYPES_TRIANGLESTRIP,
	TGFX_VERTEXLISTTYPES_LINELIST,
	TGFX_VERTEXLISTTYPES_LINESTRIP,
	TGFX_VERTEXLISTTYPES_POINTLIST
} TGfxVertexListTypes;

typedef enum TGfxTextureDimensions
{
	TGFX_TEXTURE_DIMENSIONS_2D = 0,
	TGFX_TEXTURE_DIMENSIONS_3D = 1,
	TGFX_TEXTURE_DIMENSIONS_2DCUBE = 2
} TGfxTextureDimensions;

typedef enum TGfxTextureMipmapFilter
{
	TGFX_TEXTURE_MIPMAPFILTER_NEAREST_FROM_1MIP,
	TGFX_TEXTURE_MIPMAPFILTER_LINEAR_FROM_1MIP,
	TGFX_TEXTURE_MIPMAPFILTER_NEAREST_FROM_2MIP,
	TGFX_TEXTURE_MIPMAPFILTER_LINEAR_FROM_2MIP
} TGfxTextureMipmapFilter;

typedef enum TGfxTextureOrder
{
	TGFX_TEXTURE_ORDER_SWIZZLE = 0,
	TGFX_TEXTURE_ORDER_LINEAR = 1
} TGfxTextureOrder;

typedef enum TGfxTextureWrapping
{
	TGFX_TEXTURE_WRAPPING_REPEAT,
	TGFX_TEXTURE_WRAPPING_MIRRORED_REPEAT,
	TGFX_TEXTURE_WRAPPING_CLAMP_TO_EDGE
} TGfxTextureWrapping;

typedef enum TGfxTextureChannels
{
	TGFX_TEXTURE_CHANNELS_UNDEF,
	TGFX_TEXTURE_CHANNELS_BGRA8UB,	  // Unsigned but non-normalized char
	TGFX_TEXTURE_CHANNELS_BGRA8UNORM, // Unsigned and normalized char
	TGFX_TEXTURE_CHANNELS_BGRA8SRGB,

	TGFX_TEXTURE_CHANNELS_RGBA32F,
	TGFX_TEXTURE_CHANNELS_RGBA32UI,
	TGFX_TEXTURE_CHANNELS_RGBA32I,
	TGFX_TEXTURE_CHANNELS_RGBA8UB,
	TGFX_TEXTURE_CHANNELS_RGBA8UNORM,
	TGFX_TEXTURE_CHANNELS_RGBA8SRGB,
	TGFX_TEXTURE_CHANNELS_RGBA8B,
	TGFX_TEXTURE_CHANNELS_RGBA16F,

	TGFX_TEXTURE_CHANNELS_RGB32F,
	TGFX_TEXTURE_CHANNELS_RGB32UI,
	TGFX_TEXTURE_CHANNELS_RGB32I,
	TGFX_TEXTURE_CHANNELS_RGB8UB,
	TGFX_TEXTURE_CHANNELS_RGB8B,

	TGFX_TEXTURE_CHANNELS_RA32F,
	TGFX_TEXTURE_CHANNELS_RA32UI,
	TGFX_TEXTURE_CHANNELS_RA32I,
	TGFX_TEXTURE_CHANNELS_RA8UB,
	TGFX_TEXTURE_CHANNELS_RA8B,

	TGFX_TEXTURE_CHANNELS_R32F,
	TGFX_TEXTURE_CHANNELS_R32UI,
	TGFX_TEXTURE_CHANNELS_R32I,
	TGFX_TEXTURE_CHANNELS_R8UB,
	TGFX_TEXTURE_CHANNELS_R8B,

	TGFX_TEXTURE_CHANNELS_D32,
	TGFX_TEXTURE_CHANNELS_D24S8,
	TGFX_TEXTURE_CHANNELS_A2B10G10R10_UNORM,
	TGFX_TEXTURE_CHANNELS_UNDEF2
} TGfxTextureChannels;

typedef enum TGfxTextureAccess
{
	TGFX_TEXTURE_ACCESS_SAMPLER_OPERATION,
	TGFX_TEXTURE_ACCESS_IMAGE_OPERATION,
} TGfxTextureAccess;

typedef enum TGfxGpuType
{
	TGFX_GPU_TYPE_DISCRETE_GPU,
	TGFX_GPU_TYPE_INTEGRATED_GPU
} TGfxGpuType;

typedef enum TGfxVsync
{
	TGFX_VSYNC_OFF,
	TGFX_VSYNC_DOUBLEBUFFER,
	TGFX_VSYNC_TRIPLEBUFFER
} TGfxVsync;

typedef enum TGfxWindowMode
{
	TGFX_WINDOWMODE_FULLSCREEN,
	TGFX_WINDOWMODE_WINDOWED
} TGfxWindowMode;

typedef enum TGfxBackends
{
	TGFX_BACKENDS_VULKAN = 1,
	TGFX_BACKENDS_D3D12 = 2
} TGfxBackends;

typedef enum TGfxShaderLanguages
{
	TGFX_SHADERLANGUAGES_GLSL = 0,
	TGFX_SHADERLANGUAGES_HLSL = 1,
	TGFX_SHADERLANGUAGES_SPIRV = 2
} TGfxShaderLanguages;

typedef enum TGfxShaderStage
{
	TGFX_SHADERSTAGE_VERTEXSHADER = 1,
	TGFX_SHADERSTAGE_FRAGMENTSHADER = 1 << 1,
	TGFX_SHADERSTAGE_COMPUTESHADER = 1 << 2
} TGfxShaderStage;

typedef enum TGfxPipelineType
{
	TGFX_PIPELINETYPE_RASTER = 0,
	TGFX_PIPELINETYPE_COMPUTE = 1,
	TGFX_PIPELINETYPE_RAYTRACING = 2
} TGfxPipelineType;

typedef enum TGfxBarrierPlace
{
	// Barrier is used only at the start of the pass
	TGFX_BARRIERPLACE_ONLYSTART = 0,
	// Barrier is used before everycall (So, at the start of the pass too)
	TGFX_BARRIERPLACE_BEFORE_EVERYCALL = 1,
	// Barrier is used between everycall (Neither at the start nor at the end)
	TGFX_BARRIERPLACE_BETWEEN_EVERYCALL = 2,
	// Barrier is used after everycall (So, at the end of the end too)
	TGFX_BARRIERPLACE_AFTER_EVERYCALL = 3,
	// Barrier is used only at the end the pass
	TGFX_BARRIERPLACE_ONLYEND = 4
} TGfxBarrierPlace;
typedef enum TGfxTransferPassType
{
	TGFX_TRANSFERPASSTYPE_BARRIER = 0,
	TGFX_TRANSFERPASSTYPE_COPY = 1
} TGfxTransferPassType;

// Don't forget that TGFX stores how you access them in shaders
// So backend'll probably cull some unnecessary transitions
typedef enum TGfxImageAccess
{
	TGFX_IMAGE_ACCESS_NO_ACCESS,
	TGFX_IMAGE_ACCESS_RTCOLOR_READONLY,
	TGFX_IMAGE_ACCESS_RTCOLOR_WRITEONLY,
	TGFX_IMAGE_ACCESS_RTCOLOR_READWRITE,
	TGFX_IMAGE_ACCESS_SWAPCHAIN_DISPLAY,
	TGFX_IMAGE_ACCESS_TRANSFER_DIST,
	TGFX_IMAGE_ACCESS_TRANSFER_SRC,
	TGFX_IMAGE_ACCESS_SHADER_SAMPLEONLY,
	TGFX_IMAGE_ACCESS_SHADER_WRITEONLY,
	TGFX_IMAGE_ACCESS_SHADER_SAMPLEWRITE,
	TGFX_IMAGE_ACCESS_DEPTHSTENCIL_READONLY,
	TGFX_IMAGE_ACCESS_DEPTHSTENCIL_WRITEONLY,
	TGFX_IMAGE_ACCESS_DEPTHSTENCIL_READWRITE,
	TGFX_IMAGE_ACCESS_DEPTH_READONLY,
	TGFX_IMAGE_ACCESS_DEPTH_WRITEONLY,
	TGFX_IMAGE_ACCESS_DEPTH_READWRITE,
	TGFX_IMAGE_ACCESS_DEPTHREAD_STENCILREADWRITE,
	TGFX_IMAGE_ACCESS_DEPTHREAD_STENCILWRITE,
	TGFX_IMAGE_ACCESS_DEPTHWRITE_STENCILREAD,
	TGFX_IMAGE_ACCESS_DEPTHWRITE_STENCILREADWRITE,
	TGFX_IMAGE_ACCESS_DEPTHREADWRITE_STENCILREAD,
	TGFX_IMAGE_ACCESS_DEPTHREADWRITE_STENCILWRITE
} TGfxImageAccess;

typedef enum TGfxSubDrawPassAccess
{
	TGFX_SUBDRAWPASSACCESS_ALLCOMMANDS,
	TGFX_SUBDRAWPASSACCESS_INDEX_READ,
	TGFX_SUBDRAWPASSACCESS_VERTEXATTRIB_READ,
	TGFX_SUBDRAWPASSACCESS_VERTEXUBUFFER_READONLY,
	TGFX_SUBDRAWPASSACCESS_VERTEXSBUFFER_READONLY,
	TGFX_SUBDRAWPASSACCESS_VERTEXSBUFFER_READWRITE,
	TGFX_SUBDRAWPASSACCESS_VERTEXSBUFFER_WRITEONLY,
	TGFX_SUBDRAWPASSACCESS_VERTEXSAMPLED_READONLY,
	TGFX_SUBDRAWPASSACCESS_VERTEXIMAGE_READONLY,
	TGFX_SUBDRAWPASSACCESS_VERTEXIMAGE_READWRITE,
	TGFX_SUBDRAWPASSACCESS_VERTEXIMAGE_WRITEONLY,
	TGFX_SUBDRAWPASSACCESS_VERTEXINPUTS_READONLY,
	TGFX_SUBDRAWPASSACCESS_VERTEXINPUTS_READWRITE,
	TGFX_SUBDRAWPASSACCESS_VERTEXINPUTS_WRITEONLY,
	TGFX_SUBDRAWPASSACCESS_EARLY_Z_READ,
	TGFX_SUBDRAWPASSACCESS_EARLY_Z_READWRITE,
	TGFX_SUBDRAWPASSACCESS_EARLY_Z_WRITEONLY,
	TGFX_SUBDRAWPASSACCESS_FRAGMENTUBUFFER_READONLY,
	TGFX_SUBDRAWPASSACCESS_FRAGMENTSBUFFER_READONLY,
	TGFX_SUBDRAWPASSACCESS_FRAGMENTSBUFFER_READWRITE,
	TGFX_SUBDRAWPASSACCESS_FRAGMENTSBUFFER_WRITEONLY,
	TGFX_SUBDRAWPASSACCESS_FRAGMENTSAMPLED_READONLY,
	TGFX_SUBDRAWPASSACCESS_FRAGMENTIMAGE_READONLY,
	TGFX_SUBDRAWPASSACCESS_FRAGMENTIMAGE_READWRITE,
	TGFX_SUBDRAWPASSACCESS_FRAGMENTIMAGE_WRITEONLY,
	TGFX_SUBDRAWPASSACCESS_FRAGMENTINPUTS_READONLY,
	TGFX_SUBDRAWPASSACCESS_FRAGMENTINPUTS_READWRITE,
	TGFX_SUBDRAWPASSACCESS_FRAGMENTINPUTS_WRITEONLY,
	TGFX_SUBDRAWPASSACCESS_FRAGMENTRT_READONLY,
	TGFX_SUBDRAWPASSACCESS_LATE_Z_READ,
	TGFX_SUBDRAWPASSACCESS_LATE_Z_READWRITE,
	TGFX_SUBDRAWPASSACCESS_LATE_Z_WRITEONLY,
	TGFX_SUBDRAWPASSACCESS_FRAGMENTRT_WRITEONLY
} TGfxSubDrawPassAccess;

typedef enum TGfxShaderDescriptorType
{
	TGFX_SHADERDESCRIPTORTYPE_SAMPLER = 0,
	TGFX_SHADERDESCRIPTORTYPE_SAMPLEDTEXTURE,
	TGFX_SHADERDESCRIPTORTYPE_STORAGEIMAGE,
	TGFX_SHADERDESCRIPTORTYPE_BUFFER,
	// TODO: Extensions will be supported in the future
	TGFX_SHADERDESCRIPTORTYPE_EXT_UNIFORMBUFFER,
	TGFX_SHADERDESCRIPTORTYPE_VKEXT_UNIFORMBLOCK
} TGfxShaderDescriptorType;

typedef enum TGfxMemoryAllocationType
{
	TGFX_MEMORYALLOCATIONTYPE_DEVICELOCAL = 0,
	TGFX_MEMORYALLOCATIONTYPE_HOSTVISIBLE = 1,
	TGFX_MEMORYALLOCATIONTYPE_FASTHOSTVISIBLE = 2,
	TGFX_MEMORYALLOCATIONTYPE_READBACK = 3
} TGfxMemoryAllocationType;

typedef enum TGfxConstantSamplerColor
{
	TGFX_CONSTANTSAMPLERCOLOR_BLACK_ALPHA0 = 0,
	TGFX_CONSTANTSAMPLERCOLOR_BLACK_ALPHA1 = 1,
	TGFX_CONSTANTSAMPLERCOLOR_WHITE_ALPHA1 = 2
} TGfxConstantSamplerColor;

typedef enum TGfxColorSpace
{
	TGFX_COLORSPACE_sRGB_NONLINEAR,
	TGFX_COLORSPACE_EXTENDED_sRGB_LINEAR,
	TGFX_COLORSPACE_HDR10_ST2084
} TGfxColorSpace;

typedef enum TGfxWindowComposition
{
	TGFX_WINDOWCOMPOSITION_OPAQUE
} TGfxWindowComposition;

typedef enum TGfxWindowPresentation
{
	TGFX_WINDOWPRESENTATION_FIFO,
	TGFX_WINDOWPRESENTATION_FIFO_RELAXED,
	TGFX_WINDOWPRESENTATION_IMMEDIATE,
	TGFX_WINDOWPRESENTATION_MAILBOX
} TGfxWindowPresentation;

typedef enum TGfxTextureComponentMask
{
	TGFX_TEXTURECOMPONENTMASK_R = 1,
	TGFX_TEXTURECOMPONENTMASK_G = 1 << 1,
	TGFX_TEXTURECOMPONENTMASK_B = 1 << 2,
	TGFX_TEXTURECOMPONENTMASK_A = 1 << 3,
	TGFX_TEXTURECOMPONENTMASK_RG = TGFX_TEXTURECOMPONENTMASK_R | TGFX_TEXTURECOMPONENTMASK_G,
	TGFX_TEXTURECOMPONENTMASK_RB = TGFX_TEXTURECOMPONENTMASK_R | TGFX_TEXTURECOMPONENTMASK_B,
	TGFX_TEXTURECOMPONENTMASK_RA = TGFX_TEXTURECOMPONENTMASK_R | TGFX_TEXTURECOMPONENTMASK_A,
	TGFX_TEXTURECOMPONENTMASK_GB = TGFX_TEXTURECOMPONENTMASK_G | TGFX_TEXTURECOMPONENTMASK_B,
	TGFX_TEXTURECOMPONENTMASK_GA = TGFX_TEXTURECOMPONENTMASK_G | TGFX_TEXTURECOMPONENTMASK_A,
	TGFX_TEXTURECOMPONENTMASK_BA = TGFX_TEXTURECOMPONENTMASK_B | TGFX_TEXTURECOMPONENTMASK_A,
	TGFX_TEXTURECOMPONENTMASK_RGB =
		TGFX_TEXTURECOMPONENTMASK_R | TGFX_TEXTURECOMPONENTMASK_G | TGFX_TEXTURECOMPONENTMASK_B,
	TGFX_TEXTURECOMPONENTMASK_RGA =
		TGFX_TEXTURECOMPONENTMASK_R | TGFX_TEXTURECOMPONENTMASK_G | TGFX_TEXTURECOMPONENTMASK_A,
	TGFX_TEXTURECOMPONENTMASK_RBA =
		TGFX_TEXTURECOMPONENTMASK_R | TGFX_TEXTURECOMPONENTMASK_B | TGFX_TEXTURECOMPONENTMASK_A,
	TGFX_TEXTURECOMPONENTMASK_GBA =
		TGFX_TEXTURECOMPONENTMASK_G | TGFX_TEXTURECOMPONENTMASK_B | TGFX_TEXTURECOMPONENTMASK_A,
	TGFX_TEXTURECOMPONENTMASK_RGBA = TGFX_TEXTURECOMPONENTMASK_R | TGFX_TEXTURECOMPONENTMASK_G |
							 TGFX_TEXTURECOMPONENTMASK_B | TGFX_TEXTURECOMPONENTMASK_A,
	TGFX_TEXTURECOMPONENTMASK_ALL, // All possible values if texture's format is known
	TGFX_TEXTURECOMPONENTMASK_NONE
} TGfxTextureComponentMask;

typedef enum TGfxTextureUsageMask
{
	TGFX_TEXTUREUSAGEMASK_COPYFROM = 1,
	TGFX_TEXTUREUSAGEMASK_COPYTO = 1 << 1,
	TGFX_TEXTUREUSAGEMASK_RENDERATTACHMENT = 1 << 2,
	TGFX_TEXTUREUSAGEMASK_RASTERSAMPLE = 1 << 3,
	TGFX_TEXTUREUSAGEMASK_RANDOMACCESS = 1 << 4
} TGfxTextureUsageMask;

typedef enum TGfxBufferUsageMask
{
	TGFX_BUFFERUSAGEMASK_COPYFROM = 1,
	TGFX_BUFFERUSAGEMASK_COPYTO = 1 << 1,
	TGFX_BUFFERUSAGEMASK_UNIFORMBUFFER = 1 << 2,
	TGFX_BUFFERUSAGEMASK_STORAGEBUFFER = 1 << 3,
	TGFX_BUFFERUSAGEMASK_VERTEXBUFFER = 1 << 4,
	TGFX_BUFFERUSAGEMASK_INDEXBUFFER = 1 << 5,
	TGFX_BUFFERUSAGEMASK_INDIRECTBUFFER = 1 << 6,
	TGFX_BUFFERUSAGEMASK_accessByPointerInShader = 1 << 7
} TGfxBufferUsageMask;

typedef enum TGfxVertexBindingInputRate
{
	TGFX_VERTEXBINDINGINPUTRATE_UNDEF,
	TGFX_VERTEXBINDINGINPUTRATE_VERTEX,
	TGFX_VERTEXBINDINGINPUTRATE_INSTANCE
} TGfxVertexBindingInputRate;

typedef enum TGfxIndirectOperationType
{
	TGFX_INDIRECTOPERATIONTYPE_UNDEF,
	TGFX_INDIRECTOPERATIONTYPE_DRAWNONINDEXED,
	TGFX_INDIRECTOPERATIONTYPE_DRAWINDEXED,
	TGFX_INDIRECTOPERATIONTYPE_DISPATCH,
	TGFX_INDIRECTOPERATIONTYPE_BINDINDEXBUFFER_TGFXEXT_EXTENDEDINDIRECT,
	TGFX_INDIRECTOPERATIONTYPE_UNDEF2
} TGfxIndirectOperationType;

typedef void (*tgfx_PrintLogCallback)(unsigned int logCode, const char* extraInfo);

TCORE_END_C_LINKAGE