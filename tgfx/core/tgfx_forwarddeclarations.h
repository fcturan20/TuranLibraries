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

typedef struct tgfx_gpu_obj*                   gpu_tgfxhnd;
typedef struct tgfx_window_obj*                window_tgfxhnd;
typedef struct tgfx_texture_obj*               texture_tgfxhnd;
typedef struct tgfx_monitor_obj*               monitor_tgfxhnd;
typedef struct tgfx_rtslotset_obj*             RTSlotset_tgfxhnd;
typedef struct tgfx_buffer_obj*                buffer_tgfxhnd;
typedef struct tgfx_samplingtype_obj*          sampler_tgfxhnd;
typedef struct tgfx_vertexattributelayout_obj* vertexAttributeLayout_tgfxhnd;
typedef struct tgfx_shadersource_obj*          shaderSource_tgfxhnd;
typedef struct tgfx_inheritedrtslotset_obj*    inheritedRTSlotset_tgfxhnd;
typedef struct tgfx_bindingtabletype_obj*      bindingTableType_tgfxhnd;
typedef struct tgfx_bindingtable_obj*          bindingTable_tgfxhnd;
typedef struct tgfx_commandbuffer_obj*         commandBuffer_tgfxhnd;
typedef struct tgfx_commandbundle_obj*         commandBundle_tgfxhnd;
typedef struct tgfx_gpuqueue_obj*              gpuQueue_tgfxhnd;
typedef struct tgfx_subrasterpass_obj*         subRasterpass_tgfxhnd;
typedef struct tgfx_fence_obj*                 fence_tgfxhnd;
typedef struct tgfx_heap_obj*                  heap_tgfxhnd;
typedef struct tgfx_pipeline_obj*              pipeline_tgfxhnd;

// DATA HANDLES
//////////////////////////////////////

typedef struct tgfx_extension_data*         extension_tgfx_handle;
typedef struct tgfx_textureusageflag_data*  textureUsageFlag_tgfxhnd;
typedef struct tgfx_vertexattributedata*    vertexattributetgfx_handle;
typedef struct tgfx_shaderstageflag_data*   shaderStageFlag_tgfxhnd;
typedef struct tgfx_rtslotdescription_data* RTSlotDescription_tgfxhnd;
typedef struct tgfx_rtslotusage_data*       rtslotusage_tgfx_handle;
typedef struct tgfx_stencilsettings_data*   stencilcnfg_tgfxnd;
typedef struct tgfx_depthsettings_data*     depthsettings_tgfxhnd;
typedef struct tgfx_memorytype_data*        memorytype_tgfx_handle;
typedef struct tgfx_bindingtypeinfo_data*   bindingtypeinfo_tgfx_handle;
typedef struct tgfx_blendinginfo_data*      blendinginfo_tgfx_handle;
typedef struct tgfx_buffereddrawcall*       buffereddrawcall_tgfx_handle;
typedef struct tgfx_buffereddispatchcall*   buffereddispatchcall_tgfx_handle;
typedef struct tgfx_buffer_usage_flag_data* bufferUsageFlag_tgfxhnd;

// LISTS
//////////////////////////////////////

typedef monitor_tgfxhnd*           monitor_tgfxlsthnd;
typedef gpu_tgfxhnd*               gpu_tgfxlsthnd;
typedef extension_tgfx_handle*     extension_tgfxlsthnd;
typedef RTSlotDescription_tgfxhnd* RTSlotDescription_tgfxlsthnd;
typedef rtslotusage_tgfx_handle*   RTSlotUsage_tgfxlsthnd;
typedef memorytype_tgfx_handle*    memorytype_tgfxlsthnd;
typedef shaderSource_tgfxhnd*      shaderSource_tgfxlsthnd;
typedef blendinginfo_tgfx_handle*  blendingInfo_tgfxlsthnd;
typedef sampler_tgfxhnd*           sampler_tgfxlsthnd;
typedef bindingTableType_tgfxhnd*  bindingTableType_tgfxlsthnd;
typedef bindingTable_tgfxhnd*      bindingTable_tgfxlsthnd;
typedef commandBuffer_tgfxhnd*     commandBuffer_tgfxlsthnd;
typedef fence_tgfxhnd*             fence_tgfxlsthnd;
typedef gpuQueue_tgfxhnd*          gpuQueue_tgfxlsthnd;
typedef window_tgfxhnd*            window_tgfxlsthnd;
typedef commandBundle_tgfxhnd*     commandBundle_tgfxlsthnd;
typedef buffer_tgfxhnd*            buffer_tgfxlsthnd;
typedef texture_tgfxhnd*           texture_tgfxlsthnd;
typedef subRasterpass_tgfxhnd*     subRasterpass_tgfxlsthnd;

// STRUCTS
/////////////////////////////////////

typedef struct tgfx_uvec2                     uvec2_tgfx;
typedef struct tgfx_uvec3                     uvec3_tgfx;
typedef struct tgfx_uvec4                     uvec4_tgfx;
typedef struct tgfx_vec2                      vec2_tgfx;
typedef struct tgfx_vec3                      vec3_tgfx;
typedef struct tgfx_vec4                      vec4_tgfx;
typedef struct tgfx_ivec2                     ivec2_tgfx;
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
typedef enum {
  result_tgfx_SUCCESS         = 0,
  result_tgfx_FAIL            = 1,
  result_tgfx_NOTCODED        = 2,
  result_tgfx_INVALIDARGUMENT = 3,
  // This means the operation is called at the wrong time!
  result_tgfx_WRONGTIMING = 4,
  result_tgfx_WARNING     = 5
} result_tgfx;

// Variable Types!
typedef enum {
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

typedef enum {
  cubeface_tgfx_FRONT = 0,
  cubeface_tgfx_BACK  = 1,
  cubeface_tgfx_LEFT,
  cubeface_tgfx_RIGHT,
  cubeface_tgfx_TOP,
  cubeface_tgfx_BOTTOM,
  cubeface_tgfx_ALL
} cubeface_tgfx;

typedef enum {
  operationtype_tgfx_READ_ONLY,
  operationtype_tgfx_WRITE_ONLY,
  operationtype_tgfx_READ_AND_WRITE,
  operationtype_tgfx_UNUSED
} operationtype_tgfx;

typedef enum {
  // All values will be cleared to a certain value
  rasterpassLoad_tgfx_CLEAR,
  // Loaded data is random (undef or current value) & driver probably clear
  rasterpassLoad_tgfx_DISCARD,
  // You need previous data, so previous data will affect current draw calls
  rasterpassLoad_tgfx_LOAD,
  // There won't be any access to previous data, use this for transient resources
  rasterpassLoad_tgfx_NONE
} rasterpassLoad_tgfx;

typedef enum {
  // Driver do whatever it wants with the data (either write or ignores it)
  rasterpassStore_tgfx_DISCARD,
  // Driver should write the data to memory
  rasterpassStore_tgfx_STORE,
  // Driver should ignore writing the data to memory, use this for transient resources
  rasterpassStore_tgfx_NONE
} rasterpassStore_tgfx;

typedef enum {
  depthtest_tgfx_ALWAYS,
  depthtest_tgfx_NEVER,
  depthtest_tgfx_LESS,
  depthtest_tgfx_LEQUAL,
  depthtest_tgfx_GREATER,
  depthtest_tgfx_GEQUAL
} depthtest_tgfx;

typedef enum {
  depthmode_tgfx_READ_WRITE,
  depthmode_tgfx_READ_ONLY,
  depthmode_tgfx_OFF
} depthmode_tgfx;

typedef enum { cullmode_tgfx_OFF, cullmode_tgfx_BACK, cullmode_tgfx_FRONT } cullmode_tgfx;

typedef enum {
  stencilcompare_tgfx_OFF = 0,
  stencilcompare_tgfx_NEVER_PASS,
  stencilcompare_tgfx_LESS_PASS,
  stencilcompare_tgfx_LESS_OR_EQUAL_PASS,
  stencilcompare_tgfx_EQUAL_PASS,
  stencilcompare_tgfx_NOTEQUAL_PASS,
  stencilcompare_tgfx_GREATER_OR_EQUAL_PASS,
  stencilcompare_tgfx_GREATER_PASS,
  stencilcompare_tgfx_ALWAYS_PASS,
} stencilcompare_tgfx;

typedef enum {
  stencilop_tgfx_DONT_CHANGE = 0,
  stencilop_tgfx_SET_ZERO    = 1,
  stencilop_tgfx_CHANGE      = 2,
  stencilop_tgfx_CLAMPED_INCREMENT,
  stencilop_tgfx_WRAPPED_INCREMENT,
  stencilop_tgfx_CLAMPED_DECREMENT,
  stencilop_tgfx_WRAPPED_DECREMENT,
  stencilop_tgfx_BITWISE_INVERT
} stencilop_tgfx;

typedef enum {
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

typedef enum {
  colorcomponents_tgfx_NONE = 0,
  colorcomponents_tgfx_ALL  = 1,
  colorcomponents_tgfx_R    = 2,
  colorcomponents_tgfx_G    = 3,
  colorcomponents_tgfx_B    = 4,
  colorcomponents_tgfx_RG   = 5,
  colorcomponents_tgfx_GB   = 6,
  colorcomponents_tgfx_RB   = 7,
  colorcomponents_tgfx_RGB  = 8,
} colorcomponents_tgfx;

typedef enum {
  blendmode_tgfx_ADDITIVE,
  blendmode_tgfx_SUBTRACTIVE,
  blendmode_tgfx_SUBTRACTIVE_SWAPPED,
  blendmode_tgfx_MIN,
  blendmode_tgfx_MAX
} blendmode_tgfx;

typedef enum {
  polygonmode_tgfx_FILL,
  polygonmode_tgfx_LINE,
  polygonmode_tgfx_POINT
} polygonmode_tgfx;

typedef enum {
  vertexlisttypes_tgfx_TRIANGLELIST,
  vertexlisttypes_tgfx_TRIANGLESTRIP,
  vertexlisttypes_tgfx_LINELIST,
  vertexlisttypes_tgfx_LINESTRIP,
  vertexlisttypes_tgfx_POINTLIST
} vertexlisttypes_tgfx;

typedef enum {
  rendernodetypes_tgfx_SUBDRAWPASS,
  rendernodetypes_tgfx_COMPUTEPASS,
  rendernodetypes_tgfx_TRANSFERPASS
} rendernodetypes_tgfx;

typedef enum {
  texture_dimensions_tgfx_2D     = 0,
  texture_dimensions_tgfx_3D     = 1,
  texture_dimensions_tgfx_2DCUBE = 2
} texture_dimensions_tgfx;

typedef enum {
  texture_mipmapfilter_tgfx_NEAREST_FROM_1MIP,
  texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP,
  texture_mipmapfilter_tgfx_NEAREST_FROM_2MIP,
  texture_mipmapfilter_tgfx_LINEAR_FROM_2MIP
} texture_mipmapfilter_tgfx;

typedef enum { textureOrder_tgfx_SWIZZLE = 0, textureOrder_tgfx_LINEAR = 1 } textureOrder_tgfx;

typedef enum {
  texture_wrapping_tgfx_REPEAT,
  texture_wrapping_tgfx_MIRRORED_REPEAT,
  texture_wrapping_tgfx_CLAMP_TO_EDGE
} texture_wrapping_tgfx;

typedef enum {
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

typedef enum {
  texture_access_tgfx_SAMPLER_OPERATION,
  texture_access_tgfx_IMAGE_OPERATION,
} texture_access_tgfx;

typedef enum { gpu_type_tgfx_DISCRETE_GPU, gpu_type_tgfx_INTEGRATED_GPU } gpu_type_tgfx;

typedef enum { VSYNC_OFF, VSYNC_DOUBLEBUFFER, VSYNC_TRIPLEBUFFER } vsync_tgfx;

typedef enum { windowmode_tgfx_FULLSCREEN, windowmode_tgfx_WINDOWED } windowmode_tgfx;

typedef enum { backends_tgfx_VULKAN = 1, backends_tgfx_D3D12 = 2 } backends_tgfx;

typedef enum {
  shaderlanguages_tgfx_GLSL  = 0,
  shaderlanguages_tgfx_HLSL  = 1,
  shaderlanguages_tgfx_SPIRV = 2
} shaderlanguages_tgfx;

typedef enum {
  shaderstage_tgfx_VERTEXSHADER   = 0,
  shaderstage_tgfx_FRAGMENTSHADER = 1,
  shaderstage_tgfx_COMPUTESHADER  = 2
} shaderstage_tgfx;

typedef enum {
  pipelineType_tgfx_RASTER     = 0,
  pipelineType_tgfx_COMPUTE    = 1,
  pipelineType_tgfx_RAYTRACING = 2
} pipelineType_tgfx;

typedef enum {
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
typedef enum {
  transferpasstype_tgfx_BARRIER = 0,
  transferpasstype_tgfx_COPY    = 1
} transferpasstype_tgfx;

// Don't forget that TGFX stores how you access them in shaders
// So backend'll probably cull some unnecessary transitions
typedef enum {
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

typedef enum {
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

typedef enum {
  shaderdescriptortype_tgfx_SAMPLER = 0,
  shaderdescriptortype_tgfx_SAMPLEDTEXTURE,
  shaderdescriptortype_tgfx_STORAGEIMAGE,
  shaderdescriptortype_tgfx_BUFFER,
  // TODO: Extensions will be supported in the future
  shaderdescriptortype_tgfx_EXT_UNIFORMBUFFER,
  shaderdescriptortype_tgfx_VKEXT_UNIFORMBLOCK
} shaderdescriptortype_tgfx;

typedef enum {
  memoryallocationtype_DEVICELOCAL     = 0,
  memoryallocationtype_HOSTVISIBLE     = 1,
  memoryallocationtype_FASTHOSTVISIBLE = 2,
  memoryallocationtype_READBACK        = 3
} memoryallocationtype_tgfx;

typedef enum { BLACK_ALPHA0 = 0, BLACK_ALPHA1 = 1, WHITE_ALPHA1 = 2 } constantsampler_color;

typedef enum {
  colorspace_tgfx_sRGB_NONLINEAR,
  colorspace_tgfx_EXTENDED_sRGB_LINEAR,
  colorspace_tgfx_HDR10_ST2084
} colorspace_tgfx;

typedef enum { windowcomposition_tgfx_OPAQUE } windowcomposition_tgfx;

typedef enum windowpresentation_tgfx {
  windowpresentation_tgfx_FIFO,
  windowpresentation_tgfx_FIFO_RELAXED,
  windowpresentation_tgfx_IMMEDIATE,
  windowpresentation_tgfx_MAILBOX
} windowpresentation_tgfx;

// CALLBACKS

typedef void (*tgfx_windowResizeCallback)(window_tgfxhnd WindowHandle, void* UserPointer,
                                          unsigned int WIDTH, unsigned int HEIGHT,
                                          texture_tgfxhnd* SwapchainTextureHandles);
typedef void (*tgfx_PrintLogCallback)(result_tgfx result, const char* text);

// SYSTEMS
typedef struct tgfx_helper         helper_tgfx;
typedef struct tgfx_gpudatamanager gpudatamanager_tgfx;
typedef struct tgfx_renderer       renderer_tgfx;
typedef struct tgfx_dearimgui      dearimgui_tgfx;
