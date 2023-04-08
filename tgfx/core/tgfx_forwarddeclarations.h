#pragma once
#include <predefinitions_tapi.h>

//        NAMING RULES:
//  Naming:
//  1) Funcs starts with action (get, set, create, destroy etc.)
//  2) camelCase for each searchable in-context keyword,
//    seperate sub-context keywords with "_"
//    Example: getBindingTableType -> BindingTableType is searchable keyword
//    Example: getBindingTableType_Buffer -> Buffer is a searchable sub-context in BindingTableType
//  3) Acronyms like CPU, CPU, RAM has to be all upper case
//    Example: getBufferPointer_GPU -> BufferPointer is searchable context
//        and GPU is a subcontext that returns a buffer's GPU side pointer value
//  4) Enums and global callbacks uses type first naming to avoid wrong casting
//    Example: texture_channels_tgfx_RGBA32F

// OBJECT HANDLES
///////////////////////////////////////

typedef struct tgfx_gpu_obj*           gpu_tgfxhnd;
typedef struct tgfx_window_obj*        window_tgfxhnd;
typedef struct tgfx_texture_obj*       texture_tgfxhnd;
typedef struct tgfx_monitor_obj*       monitor_tgfxhnd;
typedef struct tgfx_buffer_obj*        buffer_tgfxhnd;
typedef struct tgfx_samplingtype_obj*  sampler_tgfxhnd;
typedef struct tgfx_shadersource_obj*  shaderSource_tgfxhnd;
typedef struct tgfx_bindingtable_obj*  bindingTable_tgfxhnd;
typedef struct tgfx_commandbuffer_obj* commandBuffer_tgfxhnd;
typedef struct tgfx_commandbundle_obj* commandBundle_tgfxhnd;
typedef struct tgfx_gpuqueue_obj*      gpuQueue_tgfxhnd;
typedef struct tgfx_fence_obj*         fence_tgfxhnd;
typedef struct tgfx_heap_obj*          heap_tgfxhnd;
typedef struct tgfx_pipeline_obj*      pipeline_tgfxhnd;

// DATA HANDLES
//////////////////////////////////////

typedef struct tgfx_extension_data* extension_tgfxhnd;

// STRUCTS
/////////////////////////////////////

typedef struct tgfx_uvec2                     uvec2_tgfx;
typedef struct tgfx_uvec3                     uvec3_tgfx;
typedef struct tgfx_uvec4                     uvec4_tgfx;
typedef struct tgfx_vec2                      vec2_tgfx;
typedef struct tgfx_vec3                      vec3_tgfx;
typedef struct tgfx_vec4                      vec4_tgfx;
typedef struct tgfx_ivec2                     ivec2_tgfx;
typedef struct tgfx_ivec3                     ivec3_tgfx;
typedef struct tgfx_boxRegion                 boxRegion_tgfx;
typedef struct tgfx_cubeRegion                cubeRegion_tgfx;
typedef struct tgfx_memory_description        memoryDescription_tgfx;
typedef struct tgfx_gpu_description           gpuDescription_tgfx;
typedef struct tgfx_window_description        windowDescription_tgfx;
typedef struct tgfx_swapchain_description     swapchainDescription_tgfx;
typedef struct tgfx_texture_description       textureDescription_tgfx;
typedef struct tgfx_buffer_description        bufferDescription_tgfx;
typedef struct tgfx_binding_table_description bindingTableDescription_tgfx;
typedef struct tgfx_raster_state_description  rasterStateDescription_tgfx;
typedef struct tgfx_init_secondstage_info     initSecondStageInfo_tgfx;
typedef struct tgfx_gpu_info                  gpuInfo_tgfx;
typedef struct tgfx_heap_requirements_info    heapRequirementsInfo_tgfx;
typedef struct tgfx_window_gpu_support        windowGPUsupport_tgfx;
typedef struct tgfx_typelessColor             typelessColor_tgfx;

// ENUMS
typedef enum result_tgfx {
  result_tgfx_SUCCESS         = 0,
  result_tgfx_FAIL            = 1,
  result_tgfx_NOTCODED        = 2,
  result_tgfx_INVALIDARGUMENT = 3,
  // This means the operation is called at the wrong time!
  result_tgfx_WRONGTIMING = 4,
  result_tgfx_WARNING     = 5
} result_tgfx;

// Variable Types!
typedef enum datatype_tgfx {
  datatype_tgfx_UNDEFINED  = 0,
  datatype_tgfx_VAR_UBYTE8 = 1,
  datatype_tgfx_VAR_BYTE8  = 2,
  datatype_tgfx_VAR_UINT16 = 3,
  datatype_tgfx_VAR_INT16  = 4,
  datatype_tgfx_VAR_UINT32 = 5,
  datatype_tgfx_VAR_INT32  = 6,
  datatype_tgfx_VAR_FLOAT32,
  datatype_tgfx_VAR_VEC2,
  datatype_tgfx_VAR_VEC3,
  datatype_tgfx_VAR_VEC4,
  datatype_tgfx_VAR_MAT4x4
} datatype_tgfx;

typedef enum cubeface_tgfx {
  cubeface_tgfx_FRONT = 0,
  cubeface_tgfx_BACK  = 1,
  cubeface_tgfx_LEFT,
  cubeface_tgfx_RIGHT,
  cubeface_tgfx_TOP,
  cubeface_tgfx_BOTTOM,
  cubeface_tgfx_ALL
} cubeface_tgfx;

typedef enum operationtype_tgfx {
  operationtype_tgfx_READ_ONLY,
  operationtype_tgfx_WRITE_ONLY,
  operationtype_tgfx_READ_AND_WRITE,
  operationtype_tgfx_UNUSED
} operationtype_tgfx;

typedef enum rasterpassLoad_tgfx {
  // All values will be cleared to a certain value
  rasterpassLoad_tgfx_CLEAR,
  // Loaded data is random (undef or current value) & driver probably clear
  rasterpassLoad_tgfx_DISCARD,
  // You need previous data, so previous data will affect current draw calls
  rasterpassLoad_tgfx_LOAD,
  // There won't be any access to previous data, use this for transient resources
  rasterpassLoad_tgfx_NONE
} rasterpassLoad_tgfx;

typedef enum rasterpassStore_tgfx {
  // Driver do whatever it wants with the data (either write or ignores it)
  rasterpassStore_tgfx_DISCARD,
  // Driver should write the data to memory
  rasterpassStore_tgfx_STORE,
  // Driver should ignore writing the data to memory, use this for transient resources
  rasterpassStore_tgfx_NONE
} rasterpassStore_tgfx;

typedef enum compare_tgfx {
  compare_tgfx_ALWAYS,
  compare_tgfx_NEVER,
  compare_tgfx_LESS,
  compare_tgfx_LEQUAL,
  compare_tgfx_GREATER,
  compare_tgfx_GEQUAL
} compare_tgfx;

typedef enum depthmode_tgfx {
  depthmode_tgfx_READ_WRITE,
  depthmode_tgfx_READ_ONLY,
  depthmode_tgfx_OFF
} depthmode_tgfx;

typedef enum cullmode_tgfx {
  cullmode_tgfx_OFF,
  cullmode_tgfx_BACK,
  cullmode_tgfx_FRONT
} cullmode_tgfx;

typedef enum stencilop_tgfx {
  stencilop_tgfx_DONT_CHANGE = 0,
  stencilop_tgfx_SET_ZERO    = 1,
  stencilop_tgfx_CHANGE      = 2,
  stencilop_tgfx_CLAMPED_INCREMENT,
  stencilop_tgfx_WRAPPED_INCREMENT,
  stencilop_tgfx_CLAMPED_DECREMENT,
  stencilop_tgfx_WRAPPED_DECREMENT,
  stencilop_tgfx_BITWISE_INVERT
} stencilop_tgfx;

typedef enum blendfactor_tgfx {
  blendfactor_tgfx_ONE  = 0,
  blendfactor_tgfx_ZERO = 1,
  blendfactor_tgfx_SRC_COLOR,
  blendfactor_tgfx_SRC_1MINUSCOLOR,
  blendfactor_tgfx_SRC_ALPHA,
  blendfactor_tgfx_SRC_1MINUSALPHA,
  blendfactor_tgfx_DST_COLOR,
  blendfactor_tgfx_DST_1MINUSCOLOR,
  blendfactor_tgfx_DST_ALPHA,
  blendfactor_tgfx_DST_1MINUSALPHA,
  blendfactor_tgfx_CONST_COLOR,
  blendfactor_tgfx_CONST_1MINUSCOLOR,
  blendfactor_tgfx_CONST_ALPHA,
  blendfactor_tgfx_CONST_1MINUSALPHA
} blendfactor_tgfx;

typedef enum blendmode_tgfx {
  blendmode_tgfx_ADDITIVE,
  blendmode_tgfx_SUBTRACTIVE,
  blendmode_tgfx_SUBTRACTIVE_SWAPPED,
  blendmode_tgfx_MIN,
  blendmode_tgfx_MAX
} blendmode_tgfx;

typedef enum polygonmode_tgfx {
  polygonmode_tgfx_FILL,
  polygonmode_tgfx_LINE,
  polygonmode_tgfx_POINT
} polygonmode_tgfx;

typedef enum vertexlisttypes_tgfx {
  vertexlisttypes_tgfx_TRIANGLELIST,
  vertexlisttypes_tgfx_TRIANGLESTRIP,
  vertexlisttypes_tgfx_LINELIST,
  vertexlisttypes_tgfx_LINESTRIP,
  vertexlisttypes_tgfx_POINTLIST
} vertexlisttypes_tgfx;

typedef enum texture_dimensions_tgfx {
  texture_dimensions_tgfx_2D     = 0,
  texture_dimensions_tgfx_3D     = 1,
  texture_dimensions_tgfx_2DCUBE = 2
} texture_dimensions_tgfx;

typedef enum texture_mipmapfilter_tgfx {
  texture_mipmapfilter_tgfx_NEAREST_FROM_1MIP,
  texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP,
  texture_mipmapfilter_tgfx_NEAREST_FROM_2MIP,
  texture_mipmapfilter_tgfx_LINEAR_FROM_2MIP
} texture_mipmapfilter_tgfx;

typedef enum textureOrder_tgfx {
  textureOrder_tgfx_SWIZZLE = 0,
  textureOrder_tgfx_LINEAR  = 1
} textureOrder_tgfx;

typedef enum texture_wrapping_tgfx {
  texture_wrapping_tgfx_REPEAT,
  texture_wrapping_tgfx_MIRRORED_REPEAT,
  texture_wrapping_tgfx_CLAMP_TO_EDGE
} texture_wrapping_tgfx;

typedef enum textureChannels_tgfx {
  texture_channels_tgfx_UNDEF,
  texture_channels_tgfx_BGRA8UB,    // Unsigned but non-normalized char
  texture_channels_tgfx_BGRA8UNORM, // Unsigned and normalized char
  texture_channels_tgfx_BGRA8SRGB,

  texture_channels_tgfx_RGBA32F,
  texture_channels_tgfx_RGBA32UI,
  texture_channels_tgfx_RGBA32I,
  texture_channels_tgfx_RGBA8UB,
  texture_channels_tgfx_RGBA8UNORM,
  texture_channels_tgfx_RGBA8SRGB,
  texture_channels_tgfx_RGBA8B,
  texture_channels_tgfx_RGBA16F,

  texture_channels_tgfx_RGB32F,
  texture_channels_tgfx_RGB32UI,
  texture_channels_tgfx_RGB32I,
  texture_channels_tgfx_RGB8UB,
  texture_channels_tgfx_RGB8B,

  texture_channels_tgfx_RA32F,
  texture_channels_tgfx_RA32UI,
  texture_channels_tgfx_RA32I,
  texture_channels_tgfx_RA8UB,
  texture_channels_tgfx_RA8B,

  texture_channels_tgfx_R32F,
  texture_channels_tgfx_R32UI,
  texture_channels_tgfx_R32I,
  texture_channels_tgfx_R8UB,
  texture_channels_tgfx_R8B,

  texture_channels_tgfx_D32,
  texture_channels_tgfx_D24S8,
  texture_channels_tgfx_A2B10G10R10_UNORM,
  texture_channels_tgfx_UNDEF2
} textureChannels_tgfx;

typedef enum texture_access_tgfx {
  texture_access_tgfx_SAMPLER_OPERATION,
  texture_access_tgfx_IMAGE_OPERATION,
} texture_access_tgfx;

typedef enum gpu_type_tgfx {
  gpu_type_tgfx_DISCRETE_GPU,
  gpu_type_tgfx_INTEGRATED_GPU
} gpu_type_tgfx;

typedef enum vsync_tgfx { VSYNC_OFF, VSYNC_DOUBLEBUFFER, VSYNC_TRIPLEBUFFER } vsync_tgfx;

typedef enum windowmode_tgfx {
  windowmode_tgfx_FULLSCREEN,
  windowmode_tgfx_WINDOWED
} windowmode_tgfx;

typedef enum backends_tgfx { backends_tgfx_VULKAN = 1, backends_tgfx_D3D12 = 2 } backends_tgfx;

typedef enum shaderlanguages_tgfx {
  shaderlanguages_tgfx_GLSL  = 0,
  shaderlanguages_tgfx_HLSL  = 1,
  shaderlanguages_tgfx_SPIRV = 2
} shaderlanguages_tgfx;

typedef enum shaderStage_tgfx {
  shaderStage_tgfx_VERTEXSHADER   = 1,
  shaderStage_tgfx_FRAGMENTSHADER = 1 << 1,
  shaderStage_tgfx_COMPUTESHADER  = 1 << 2
} shaderStage_tgfx;
typedef int shaderStage_tgfxflag;

typedef enum pipelineType_tgfx {
  pipelineType_tgfx_RASTER     = 0,
  pipelineType_tgfx_COMPUTE    = 1,
  pipelineType_tgfx_RAYTRACING = 2
} pipelineType_tgfx;

typedef enum barrierplace_tgfx {
  // Barrier is used only at the start of the pass
  barrierplace_tgfx_ONLYSTART = 0,
  // Barrier is used before everycall (So, at the start of the pass too)
  barrierplace_tgfx_BEFORE_EVERYCALL = 1,
  // Barrier is used between everycall (Neither at the start nor at the end)
  barrierplace_tgfx_BETWEEN_EVERYCALL = 2,
  // Barrier is used after everycall (So, at the end of the end too)
  barrierplace_tgfx_AFTER_EVERYCALL = 3,
  // Barrier is used only at the end the pass
  barrierplace_tgfx_ONLYEND = 4
} barrierplace_tgfx;
typedef enum transferpasstype_tgfx {
  transferpasstype_tgfx_BARRIER = 0,
  transferpasstype_tgfx_COPY    = 1
} transferpasstype_tgfx;

// Don't forget that TGFX stores how you access them in shaders
// So backend'll probably cull some unnecessary transitions
typedef enum image_access_tgfx {
  image_access_tgfx_NO_ACCESS,
  image_access_tgfx_RTCOLOR_READONLY,
  image_access_tgfx_RTCOLOR_WRITEONLY,
  image_access_tgfx_RTCOLOR_READWRITE,
  image_access_tgfx_SWAPCHAIN_DISPLAY,
  image_access_tgfx_TRANSFER_DIST,
  image_access_tgfx_TRANSFER_SRC,
  image_access_tgfx_SHADER_SAMPLEONLY,
  image_access_tgfx_SHADER_WRITEONLY,
  image_access_tgfx_SHADER_SAMPLEWRITE,
  image_access_tgfx_DEPTHSTENCIL_READONLY,
  image_access_tgfx_DEPTHSTENCIL_WRITEONLY,
  image_access_tgfx_DEPTHSTENCIL_READWRITE,
  image_access_tgfx_DEPTH_READONLY,
  image_access_tgfx_DEPTH_WRITEONLY,
  image_access_tgfx_DEPTH_READWRITE,
  image_access_tgfx_DEPTHREAD_STENCILREADWRITE,
  image_access_tgfx_DEPTHREAD_STENCILWRITE,
  image_access_tgfx_DEPTHWRITE_STENCILREAD,
  image_access_tgfx_DEPTHWRITE_STENCILREADWRITE,
  image_access_tgfx_DEPTHREADWRITE_STENCILREAD,
  image_access_tgfx_DEPTHREADWRITE_STENCILWRITE
} image_access_tgfx;

typedef enum subdrawpassaccess_tgfx {
  subdrawpassaccess_tgfx_ALLCOMMANDS,
  subdrawpassaccess_tgfx_INDEX_READ,
  subdrawpassaccess_tgfx_VERTEXATTRIB_READ,
  subdrawpassaccess_tgfx_VERTEXUBUFFER_READONLY,
  subdrawpassaccess_tgfx_VERTEXSBUFFER_READONLY,
  subdrawpassaccess_tgfx_VERTEXSBUFFER_READWRITE,
  subdrawpassaccess_tgfx_VERTEXSBUFFER_WRITEONLY,
  subdrawpassaccess_tgfx_VERTEXSAMPLED_READONLY,
  subdrawpassaccess_tgfx_VERTEXIMAGE_READONLY,
  subdrawpassaccess_tgfx_VERTEXIMAGE_READWRITE,
  subdrawpassaccess_tgfx_VERTEXIMAGE_WRITEONLY,
  subdrawpassaccess_tgfx_VERTEXINPUTS_READONLY,
  subdrawpassaccess_tgfx_VERTEXINPUTS_READWRITE,
  subdrawpassaccess_tgfx_VERTEXINPUTS_WRITEONLY,
  subdrawpassaccess_tgfx_EARLY_Z_READ,
  subdrawpassaccess_tgfx_EARLY_Z_READWRITE,
  subdrawpassaccess_tgfx_EARLY_Z_WRITEONLY,
  subdrawpassaccess_tgfx_FRAGMENTUBUFFER_READONLY,
  subdrawpassaccess_tgfx_FRAGMENTSBUFFER_READONLY,
  subdrawpassaccess_tgfx_FRAGMENTSBUFFER_READWRITE,
  subdrawpassaccess_tgfx_FRAGMENTSBUFFER_WRITEONLY,
  subdrawpassaccess_tgfx_FRAGMENTSAMPLED_READONLY,
  subdrawpassaccess_tgfx_FRAGMENTIMAGE_READONLY,
  subdrawpassaccess_tgfx_FRAGMENTIMAGE_READWRITE,
  subdrawpassaccess_tgfx_FRAGMENTIMAGE_WRITEONLY,
  subdrawpassaccess_tgfx_FRAGMENTINPUTS_READONLY,
  subdrawpassaccess_tgfx_FRAGMENTINPUTS_READWRITE,
  subdrawpassaccess_tgfx_FRAGMENTINPUTS_WRITEONLY,
  subdrawpassaccess_tgfx_FRAGMENTRT_READONLY,
  subdrawpassaccess_tgfx_LATE_Z_READ,
  subdrawpassaccess_tgfx_LATE_Z_READWRITE,
  subdrawpassaccess_tgfx_LATE_Z_WRITEONLY,
  subdrawpassaccess_tgfx_FRAGMENTRT_WRITEONLY
} subdrawpassaccess_tgfx;

typedef enum shaderdescriptortype_tgfx {
  shaderdescriptortype_tgfx_SAMPLER = 0,
  shaderdescriptortype_tgfx_SAMPLEDTEXTURE,
  shaderdescriptortype_tgfx_STORAGEIMAGE,
  shaderdescriptortype_tgfx_BUFFER,
  // TODO: Extensions will be supported in the future
  shaderdescriptortype_tgfx_EXT_UNIFORMBUFFER,
  shaderdescriptortype_tgfx_VKEXT_UNIFORMBLOCK
} shaderdescriptortype_tgfx;

typedef enum memoryallocationtype_tgfx {
  memoryallocationtype_DEVICELOCAL     = 0,
  memoryallocationtype_HOSTVISIBLE     = 1,
  memoryallocationtype_FASTHOSTVISIBLE = 2,
  memoryallocationtype_READBACK        = 3
} memoryallocationtype_tgfx;

typedef enum constantsampler_color {
  BLACK_ALPHA0 = 0,
  BLACK_ALPHA1 = 1,
  WHITE_ALPHA1 = 2
} constantsampler_color;

typedef enum colorspace_tgfx {
  colorspace_tgfx_sRGB_NONLINEAR,
  colorspace_tgfx_EXTENDED_sRGB_LINEAR,
  colorspace_tgfx_HDR10_ST2084
} colorspace_tgfx;

typedef enum windowcomposition_tgfx { windowcomposition_tgfx_OPAQUE } windowcomposition_tgfx;

typedef enum windowpresentation_tgfx {
  windowpresentation_tgfx_FIFO,
  windowpresentation_tgfx_FIFO_RELAXED,
  windowpresentation_tgfx_IMMEDIATE,
  windowpresentation_tgfx_MAILBOX
} windowpresentation_tgfx;

typedef enum texture_component_mask_tgfx {
  textureComponentMask_tgfx_R  = 1,
  textureComponentMask_tgfx_G  = 1 << 1,
  textureComponentMask_tgfx_B  = 1 << 2,
  textureComponentMask_tgfx_A  = 1 << 3,
  textureComponentMask_tgfx_RG = textureComponentMask_tgfx_R | textureComponentMask_tgfx_G,
  textureComponentMask_tgfx_RB = textureComponentMask_tgfx_R | textureComponentMask_tgfx_B,
  textureComponentMask_tgfx_RA = textureComponentMask_tgfx_R | textureComponentMask_tgfx_A,
  textureComponentMask_tgfx_GB = textureComponentMask_tgfx_G | textureComponentMask_tgfx_B,
  textureComponentMask_tgfx_GA = textureComponentMask_tgfx_G | textureComponentMask_tgfx_A,
  textureComponentMask_tgfx_BA = textureComponentMask_tgfx_B | textureComponentMask_tgfx_A,
  textureComponentMask_tgfx_RGB =
    textureComponentMask_tgfx_R | textureComponentMask_tgfx_G | textureComponentMask_tgfx_B,
  textureComponentMask_tgfx_RGA =
    textureComponentMask_tgfx_R | textureComponentMask_tgfx_G | textureComponentMask_tgfx_A,
  textureComponentMask_tgfx_RBA =
    textureComponentMask_tgfx_R | textureComponentMask_tgfx_B | textureComponentMask_tgfx_A,
  textureComponentMask_tgfx_GBA =
    textureComponentMask_tgfx_G | textureComponentMask_tgfx_B | textureComponentMask_tgfx_A,
  textureComponentMask_tgfx_RGBA = textureComponentMask_tgfx_R | textureComponentMask_tgfx_G |
                                   textureComponentMask_tgfx_B | textureComponentMask_tgfx_A,
  textureComponentMask_tgfx_ALL, // All possible values if texture's format is known
  textureComponentMask_tgfx_NONE
} textureComponentMask_tgfx;

typedef enum texture_usage_mask_tgfx {
  textureUsageMask_tgfx_COPYFROM         = 1,
  textureUsageMask_tgfx_COPYTO           = 1 << 1,
  textureUsageMask_tgfx_RENDERATTACHMENT = 1 << 2,
  textureUsageMask_tgfx_RASTERSAMPLE     = 1 << 3,
  textureUsageMask_tgfx_RANDOMACCESS     = 1 << 4
} textureUsageMask_tgfx;
typedef int textureUsageMask_tgfxflag;

typedef enum buffer_usage_mask_tgfx {
  bufferUsageMask_tgfx_COPYFROM                = 1,
  bufferUsageMask_tgfx_COPYTO                  = 1 << 1,
  bufferUsageMask_tgfx_UNIFORMBUFFER           = 1 << 2,
  bufferUsageMask_tgfx_STORAGEBUFFER           = 1 << 3,
  bufferUsageMask_tgfx_VERTEXBUFFER            = 1 << 4,
  bufferUsageMask_tgfx_INDEXBUFFER             = 1 << 5,
  bufferUsageMask_tgfx_INDIRECTBUFFER          = 1 << 6,
  bufferUsageMask_tgfx_accessByPointerInShader = 1 << 7
} bufferUsageMask_tgfx;
typedef int bufferUsageMask_tgfxflag;

typedef enum vertex_binding_input_rate_tgfx {
  vertexBindingInputRate_tgfx_UNDEF,
  vertexBindingInputRate_tgfx_VERTEX,
  vertexBindingInputRate_tgfx_INSTANCE
} vertexBindingInputRate_tgfx;

typedef enum indirect_operation_type_tgfx {
  indirectOperationType_tgfx_UNDEF,
  indirectOperationType_tgfx_DRAWNONINDEXED,
  indirectOperationType_tgfx_DRAWINDEXED,
  indirectOperationType_tgfx_DISPATCH,
  indirectOperationType_tgfx_BINDINDEXBUFFER_TGFXEXT_EXTENDEDINDIRECT,
  indirectOperationType_tgfx_UNDEF2
} indirectOperationType_tgfx;

typedef enum key_tgfx {
  /* The unknown key */
  key_tgfx_UNKNOWN,

  /* Printable keys */
  key_tgfx_SPACE,
  key_tgfx_APOSTROPHE, /* ' */
  key_tgfx_COMMA,      /* , */
  key_tgfx_MINUS,      /* - */
  key_tgfx_PERIOD,     /* . */
  key_tgfx_SLASH,      /* / */
  key_tgfx_0,
  key_tgfx_1,
  key_tgfx_2,
  key_tgfx_3,
  key_tgfx_4,
  key_tgfx_5,
  key_tgfx_6,
  key_tgfx_7,
  key_tgfx_8,
  key_tgfx_9,
  key_tgfx_SEMICOLON, /* ; */
  key_tgfx_EQUAL,     /* = */
  key_tgfx_A,
  key_tgfx_B,
  key_tgfx_C,
  key_tgfx_D,
  key_tgfx_E,
  key_tgfx_F,
  key_tgfx_G,
  key_tgfx_H,
  key_tgfx_I,
  key_tgfx_J,
  key_tgfx_K,
  key_tgfx_L,
  key_tgfx_M,
  key_tgfx_N,
  key_tgfx_O,
  key_tgfx_P,
  key_tgfx_Q,
  key_tgfx_R,
  key_tgfx_S,
  key_tgfx_T,
  key_tgfx_U,
  key_tgfx_V,
  key_tgfx_W,
  key_tgfx_X,
  key_tgfx_Y,
  key_tgfx_Z,
  key_tgfx_LEFT_BRACKET,  /* [ */
  key_tgfx_BACKSLASH,     /* \ */
  key_tgfx_RIGHT_BRACKET, /* ] */
  key_tgfx_GRAVE_ACCENT,  /* ` */
  key_tgfx_WORLD_1,       /* non-US # */
  key_tgfx_WORLD_2,       /* non-US #2 */

  /* Function keys */
  key_tgfx_ESCAPE,
  key_tgfx_ENTER,
  key_tgfx_TAB,
  key_tgfx_BACKSPACE,
  key_tgfx_INSERT,
  key_tgfx_DELETE,
  key_tgfx_RIGHT,
  key_tgfx_LEFT,
  key_tgfx_DOWN,
  key_tgfx_UP,
  key_tgfx_PAGE_UP,
  key_tgfx_PAGE_DOWN,
  key_tgfx_HOME,
  key_tgfx_END,
  key_tgfx_CAPS_LOCK,
  key_tgfx_SCROLL_LOCK,
  key_tgfx_NUM_LOCK,
  key_tgfx_PRINT_SCREEN,
  key_tgfx_PAUSE,
  key_tgfx_F1,
  key_tgfx_F2,
  key_tgfx_F3,
  key_tgfx_F4,
  key_tgfx_F5,
  key_tgfx_F6,
  key_tgfx_F7,
  key_tgfx_F8,
  key_tgfx_F9,
  key_tgfx_F10,
  key_tgfx_F11,
  key_tgfx_F12,
  key_tgfx_F13,
  key_tgfx_F14,
  key_tgfx_F15,
  key_tgfx_F16,
  key_tgfx_F17,
  key_tgfx_F18,
  key_tgfx_F19,
  key_tgfx_F20,
  key_tgfx_F21,
  key_tgfx_F22,
  key_tgfx_F23,
  key_tgfx_F24,
  key_tgfx_F25,
  key_tgfx_KP_0,
  key_tgfx_KP_1,
  key_tgfx_KP_2,
  key_tgfx_KP_3,
  key_tgfx_KP_4,
  key_tgfx_KP_5,
  key_tgfx_KP_6,
  key_tgfx_KP_7,
  key_tgfx_KP_8,
  key_tgfx_KP_9,
  key_tgfx_KP_DECIMAL,
  key_tgfx_KP_DIVIDE,
  key_tgfx_KP_MULTIPLY,
  key_tgfx_KP_SUBTRACT,
  key_tgfx_KP_ADD,
  key_tgfx_KP_ENTER,
  key_tgfx_KP_EQUAL,
  key_tgfx_LEFT_SHIFT,
  key_tgfx_LEFT_CONTROL,
  key_tgfx_LEFT_ALT,
  key_tgfx_LEFT_SUPER,
  key_tgfx_RIGHT_SHIFT,
  key_tgfx_RIGHT_CONTROL,
  key_tgfx_RIGHT_ALT,
  key_tgfx_RIGHT_SUPER,
  key_tgfx_MENU,
  key_tgfx_MOUSE_LEFT,
  key_tgfx_MOUSE_MIDDLE,
  key_tgfx_MOUSE_RIGHT,
  key_tgfx_MAX_ENUM
} key_tgfx;

typedef enum key_action_tgfx {
  keyAction_tgfx_RELEASE,
  keyAction_tgfx_PRESS,
  keyAction_tgfx_REPEAT
} keyAction_tgfx;

typedef enum key_modifier_tgfx {
  keyMod_tgfx_NONE,
  keyMod_tgfx_SHIFT,
  keyMod_tgfx_CONTROL,
  keyMod_tgfx_ALT,
  keyMod_tgfx_SUPER,
  keyMod_tgfx_CAPSLOCK,
  keyMod_tgfx_NUMLOCK
} keyMod_tgfx;

typedef enum cursor_mode_tgfx {
  cursorMode_tgfx_NORMAL,
  cursorMode_tgfx_HIDDEN,
  cursorMode_tgfx_DISABLED,
  cursorMode_tgfx_RAW
} cursorMode_tgfx;

// CALLBACKS

typedef void (*tgfx_windowResizeCallback)(window_tgfxhnd windowHnd, void* userPtr,
                                          tgfx_uvec2       resolution,
                                          texture_tgfxhnd* swapchainTextures);
// logCode is an index to look up from all tgfx logs. You should use core->getLogMessage() for text
// extraInfo is the text from the backend, probably contains specific info about your system
typedef void (*tgfx_logCallback)(unsigned int logCode, const wchar_t* extraInfo);
// @param scanCode: System-specific scan code
typedef void (*tgfx_windowKeyCallback)(window_tgfxhnd windowHnd, void* userPointer, key_tgfx key,
                                       int scanCode, key_action_tgfx action, keyMod_tgfx mode);

// SYSTEMS
typedef struct tgfx_helper         helper_tgfx;
typedef struct tgfx_gpudatamanager gpudatamanager_tgfx;
typedef struct tgfx_renderer       renderer_tgfx;
typedef struct tgfx_dearimgui      dearimgui_tgfx;
