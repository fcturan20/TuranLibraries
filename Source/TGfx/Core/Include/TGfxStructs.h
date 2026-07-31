#pragma once
#include <TCoreMacros.h>
#include <TGfxMath.h>
#include <TGfxDeclarations.h>
TCORE_BEGIN_C_LINKAGE

typedef enum TGfxTextureMipmapFilter
{
	TGFX_TEXTURE_MIPMAPFILTER_NEAREST_FROM_1MIP,
	TGFX_TEXTURE_MIPMAPFILTER_LINEAR_FROM_1MIP,
	TGFX_TEXTURE_MIPMAPFILTER_NEAREST_FROM_2MIP,
	TGFX_TEXTURE_MIPMAPFILTER_LINEAR_FROM_2MIP
} TGfxTextureMipmapFilter;

typedef enum TGfxTextureWrapping
{
	TGFX_TEXTURE_WRAPPING_REPEAT,
	TGFX_TEXTURE_WRAPPING_MIRRORED_REPEAT,
	TGFX_TEXTURE_WRAPPING_CLAMP_TO_EDGE
} TGfxTextureWrapping;

typedef struct TGfxSamplerDescription
{
	TU4 MinMipLevel, MaxMipLevel;
	TGfxTextureMipmapFilter MinFilter, MagFilter;
	TGfxTextureWrapping WrapWidth, WrapHeight, WrapDepth;
	TGfxUVec4 BorderColor;
} TGfxSamplerDescription;

typedef enum TGfxTextureOrder
{
	TGFX_TEXTURE_ORDER_SWIZZLE = 0,
	TGFX_TEXTURE_ORDER_LINEAR = 1
} TGfxTextureOrder;

typedef struct TGfxTextureDescription
{
	TGfxTextureDimensions Dimension;
	TGfxUVec2 Resolution;
	TGfxTextureChannels ChannelType;
	TU4 MipCount;
	TGfxTextureOrder DataOrder;
} TGfxTextureDescription;

typedef struct TGfxBufferDescription
{
	TU8 Size;
	struct tgfx_extension* Extensions;
} TGfxBufferDescription;

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

typedef struct TGfxBindingTableDescription
{
	TGfxShaderDescriptorType DescriptorType;
	TU4 ElementCount;
	TBool IsDynamic;
} TGfxBindingTableDescription;

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

typedef enum TGfxCompare
{
	TGFX_COMPARE_ALWAYS,
	TGFX_COMPARE_NEVER,
	TGFX_COMPARE_LESS,
	TGFX_COMPARE_LEQUAL,
	TGFX_COMPARE_GREATER,
	TGFX_COMPARE_GEQUAL
} TGfxCompare;

typedef struct TGfxStencilState
{
	TGfxStencilOp StencilFail, StencilPass, StencilDepthFail;
	TGfxCompare CompareOp;
	TU4 CompareMask, WriteMask, Reference;
} TGfxStencilState;

typedef struct TGfxDepthStencilState
{
	TBool IsDepthTestEnabled, IsDepthWriteEnabled, IsStencilTestEnabled;
	TGfxCompare DepthCompare;
	TGfxStencilState Front, Back;
} TGfxDepthStencilState;

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

typedef struct TGfxBlendState
{
	TBool IsBlendEnabled;
	TGfxBlendFactor SrcColorFactor, DstColorFactor, SrcAlphaFactor, DstAlphaFactor;
	TGfxBlendMode ColorMode, AlphaMode;
	TGfxTextureComponentMask BlendComponents;
} TGfxBlendState;

typedef enum TGfxCullMode
{
	TGFX_CULLMODE_OFF,
	TGFX_CULLMODE_BACK,
	TGFX_CULLMODE_FRONT
} TGfxCullMode;

typedef enum TGfxPolygonMode
{
	TGFX_POLYGONMODE_FILL,
	TGFX_POLYGONMODE_LINE,
	TGFX_POLYGONMODE_POINT
} TGfxPolygonMode;

typedef enum TGfxVertexListType
{
	TGFX_VERTEXLISTTYPE_TRIANGLELIST,
	TGFX_VERTEXLISTTYPE_TRIANGLESTRIP,
	TGFX_VERTEXLISTTYPE_LINELIST,
	TGFX_VERTEXLISTTYPE_LINESTRIP,
	TGFX_VERTEXLISTTYPE_POINTLIST
} TGfxVertexListType;

#define TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT 8
typedef struct TGfxRasterStateDescription
{
	TGfxCullMode Culling;
	TGfxPolygonMode PolygonMode;
	TGfxDepthStencilState DepthStencilState;
	TGfxBlendState BlendStates[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT];
	TGfxVertexListType Topology;
} TGfxRasterStateDescription;

typedef struct TGfxHeapRequirementsInfo
{
	// Single GPU can have max 32 regions
	// GPU should be same with the one used in CreateTexture()
	TU1 MemoryRegionIds[32];
	TU4 OffsetAlignment;
	TU8 Size;
} TGfxHeapRequirementsInfo;

// 32 byte data to store extreme colors (RGBA64)
typedef struct TGfxTypelessColor
{
	TU1 data[32];
} TGfxTypelessColor;

typedef struct TGfxVertexAttributeDescription
{
	TU4 AttributeIdx, BindingIdx, Offset;
	TGfxDataType DataType;
} TGfxVertexAttributeDescription;

typedef enum TGfxVertexBindingInputRate
{
	TGFX_VERTEXBINDINGINPUTRATE_UNDEF,
	TGFX_VERTEXBINDINGINPUTRATE_VERTEX,
	TGFX_VERTEXBINDINGINPUTRATE_INSTANCE
} TGfxVertexBindingInputRate;

typedef struct TGfxVertexBindingDescription
{
	TU4 BindingIdx, Stride;
	TGfxVertexBindingInputRate InputRate;
} TGfxVertexBindingDescription;

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

// TGFX_SUBPASS_EXTENSION
typedef struct TGfxSubpassSlotDescription
{
	TGfxRasterPassStore StoreType;
	TGfxRasterPassLoad LoadType;
	TGfxTypelessColor ClearValue;
	TGfxTextureChannels Format;
	TGfxImageAccess Layout;
} TGfxSubpassSlotDescription;

typedef struct TGfxViewportInfo
{
	TGfxFVec2 TopLeftCorner, Size, DepthMinMax;
} TGfxViewportInfo;

typedef struct TGfxRasterInputAssemblerDescription
{
	TU4 AttribCount, BindingCount;
	const TGfxVertexAttributeDescription* Attributes;
	const TGfxVertexBindingDescription* Bindings;
} TGfxRasterInputAssemblerDescription;

typedef struct TGfxRasterPipelineDescription
{
	TU4 ShaderCount;
	TGfxShaderSource const* Shaders;
	TGfxRasterInputAssemblerDescription AttributeLayout;
	const TGfxRasterStateDescription* MainStates;
	TGfxTextureChannels ColorTextureFormats[TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT];
	const TGfxBindingTableDescription* Tables;
	TU4 TableCount;
	TGfxTextureChannels DepthStencilTextureFormat;
	struct tgfx_extension* const* Exts;
	TU1 PushConstantOffset, PushConstantSize;
} TGfxRasterPipelineDescription;

typedef struct TGfxRasterPassBeginSlotInfo
{
	TGfxTexture Texture;
	TGfxImageAccess ImageAccess;
	TGfxRasterPassLoad LoadOp, LoadStencilOp;
	TGfxRasterPassStore StoreOp, StoreStencilOp;
	TGfxTypelessColor ClearValue;
} TGfxRasterPassBeginSlotInfo;

typedef struct TGfxDrawIndexedIndirectArgument
{
	TU4 IndexCountPerInstance;
	TU4 InstanceCount;
	TU4 FirstIndex;
	TI4 VertexOffset;
	TU4 FirstInstance;
} TGfxDrawIndexedIndirectArgument;

typedef struct TGfxDrawNonIndexedIndirectArgument
{
	TU4 VertexCountPerInstance;
	TU4 InstanceCount;
	TU4 FirstVertex;
	TU4 FirstInstance;
} TGfxDrawNonIndexedIndirectArgument;

typedef struct TGfxDispatchIndirectArgument
{
	TGfxUVec3 ThreadGroupCount;
} TGfxDispatchIndirectArgument;

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

typedef enum TGfxDepthMode
{
	TGFX_DEPTHMODE_READ_WRITE,
	TGFX_DEPTHMODE_READ_ONLY,
	TGFX_DEPTHMODE_OFF
} TGfxDepthMode;

typedef enum TGfxTextureAccess
{
	TGFX_TEXTURE_ACCESS_SAMPLER_OPERATION,
	TGFX_TEXTURE_ACCESS_IMAGE_OPERATION,
} TGfxTextureAccess;

typedef enum TGfxVsync
{
	TGFX_VSYNC_OFF,
	TGFX_VSYNC_DOUBLEBUFFER,
	TGFX_VSYNC_TRIPLEBUFFER
} TGfxVsync;

typedef enum TGfxShaderLanguage
{
	TGFX_SHADERLANGUAGE_GLSL = 0,
	TGFX_SHADERLANGUAGE_HLSL = 1,
	TGFX_SHADERLANGUAGE_SPIRV = 2
} TGfxShaderLanguage;

typedef enum TGfxShaderStage
{
	TGFX_SHADERSTAGE_VERTEXSHADER = 1,
	TGFX_SHADERSTAGE_FRAGMENTSHADER = 1 << 1,
	TGFX_SHADERSTAGE_COMPUTESHADER = 1 << 2
} TGfxShaderStage;

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

typedef enum TGfxConstantSamplerColor
{
	TGFX_CONSTANTSAMPLERCOLOR_BLACK_ALPHA0 = 0,
	TGFX_CONSTANTSAMPLERCOLOR_BLACK_ALPHA1 = 1,
	TGFX_CONSTANTSAMPLERCOLOR_WHITE_ALPHA1 = 2
} TGfxConstantSamplerColor;

typedef enum TGfxIndirectOperationType
{
	TGFX_INDIRECTOPERATIONTYPE_UNDEF,
	TGFX_INDIRECTOPERATIONTYPE_DRAWNONINDEXED,
	TGFX_INDIRECTOPERATIONTYPE_DRAWINDEXED,
	TGFX_INDIRECTOPERATIONTYPE_DISPATCH,
	TGFX_INDIRECTOPERATIONTYPE_BINDINDEXBUFFER_TGFXEXT_EXTENDEDINDIRECT,
	TGFX_INDIRECTOPERATIONTYPE_UNDEF2
} TGfxIndirectOperationType;

typedef void (*tgfx_PrintLogCallback)(TU4 logCode, const char* extraInfo);

TCORE_END_C_LINKAGE