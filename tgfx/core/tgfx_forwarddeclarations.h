#pragma once
#include <predefinitions_tapi.h>

            //OBJECTS

typedef struct tgfx_gpu_obj* gpu_tgfx_handle;
typedef struct tgfx_window_obj* window_tgfx_handle;
typedef struct tgfx_texture_obj* texture_tgfx_handle;
typedef struct tgfx_monitor_obj* monitor_tgfx_handle;
typedef struct tgfx_rtslotset_obj* rtslotset_tgfx_handle;
typedef struct tgfx_buffer_obj* buffer_tgfx_handle;
typedef struct tgfx_samplingtype_obj* samplingtype_tgfx_handle;
typedef struct tgfx_vertexattributelayout_obj* vertexattributelayout_tgfx_handle;
typedef struct tgfx_shadersource_obj* shadersource_tgfx_handle;
typedef struct tgfx_computeshadertype_obj* computeshadertype_tgfx_handle;
typedef struct tgfx_computeshaderinstance_obj* computeshaderinstance_tgfx_handle;
typedef struct tgfx_rasterpipelinetype_obj* rasterpipelinetype_tgfx_handle;
typedef struct tgfx_rasterpipelineinstance_obj* rasterpipelineinstance_tgfx_handle;
typedef struct tgfx_inheritedrtslotset_obj* inheritedrtslotset_tgfx_handle;
typedef struct tgfx_bindingtable_obj* bindingtable_tgfx_handle;
typedef struct tgfx_commandbundle_obj* commandbundle_tgfx_handle;
typedef struct tgfx_gpuqueue_obj* gpuqueue_tgfx_handle; //This is a Vulkan Queue, not Queue Family
typedef struct tgfx_renderpass_obj* renderpass_tgfx_handle;
typedef struct tgfx_rendersubpass_obj* rendersubpass_tgfx_handle;
typedef struct tgfx_fence_obj* fence_tgfx_handle;
typedef struct tgfx_semaphore_obj* semaphore_tgfx_handle;

            //DATAS

typedef struct tgfx_extension_data* extension_tgfx_handle;
typedef struct tgfx_textureusageflag_data* textureusageflag_tgfx_handle;
typedef struct tgfx_vertexattributedata* vertexattributetgfx_handle;
typedef struct tgfx_shaderstageflag_data* shaderstageflag_tgfx_handle;
typedef struct tgfx_rtslotdescription_data* rtslotdescription_tgfx_handle;
typedef struct tgfx_rtslotusage_data* rtslotusage_tgfx_handle;
typedef struct tgfx_stencilsettings_data* stencilsettings_tgfx_handle;
typedef struct tgfx_depthsettings_data* depthsettings_tgfx_handle;
typedef struct tgfx_memorytype_data* memorytype_tgfx_handle;
typedef struct tgfx_initializationsecondstageinfo_data* initializationsecondstageinfo_tgfx_handle;
typedef struct tgfx_bindingtypeinfo_data* bindingtypeinfo_tgfx_handle;
typedef struct tgfx_blendinginfo_data* blendinginfo_tgfx_handle;
typedef struct tgfx_buffereddrawcall* buffereddrawcall_tgfx_handle;
typedef struct tgfx_buffereddispatchcall* buffereddispatchcall_tgfx_handle;
typedef struct tgfx_commandbuffer_data* commandbuffer_tgfx_handle;


            //LISTS

typedef monitor_tgfx_handle* monitor_tgfx_listhandle;
typedef gpu_tgfx_handle* gpu_tgfx_listhandle;
typedef extension_tgfx_handle* extension_tgfx_listhandle;
typedef rtslotdescription_tgfx_handle* rtslotdescription_tgfx_listhandle;
typedef rtslotusage_tgfx_handle* rtslotusage_tgfx_listhandle;
typedef memorytype_tgfx_handle* memorytype_tgfx_listhandle;
typedef shadersource_tgfx_handle* shadersource_tgfx_listhandle;
typedef blendinginfo_tgfx_handle* blendinginfo_tgfx_listhandle;
typedef samplingtype_tgfx_handle* samplingtype_tgfx_listhandle;
typedef bindingtable_tgfx_handle* bindingtable_tgfx_listhandle;
typedef commandbuffer_tgfx_handle* commandbuffer_tgfx_listhandle;
typedef fence_tgfx_handle* fence_tgfx_listhandle;
typedef semaphore_tgfx_handle* semaphore_tgfx_listhandle;


            //ENUMS
typedef enum result_tgfx {
    result_tgfx_SUCCESS = 0,
    result_tgfx_FAIL = 1,
    result_tgfx_NOTCODED = 2,
    result_tgfx_INVALIDARGUMENT = 3,
    result_tgfx_WRONGTIMING = 4,		//This means the operation is called at the wrong time!
    result_tgfx_WARNING = 5
} result_tgfx;

//Variable Types!
typedef enum datatype_tgfx{
    datatype_tgfx_UNDEFINED = 0, datatype_tgfx_VAR_UBYTE8 = 1, datatype_tgfx_VAR_BYTE8 = 2, datatype_tgfx_VAR_UINT16 = 3,
    datatype_tgfx_VAR_INT16 = 4, datatype_tgfx_VAR_UINT32 = 5, datatype_tgfx_VAR_INT32 = 6, datatype_tgfx_VAR_FLOAT32,
    datatype_tgfx_VAR_VEC2, datatype_tgfx_VAR_VEC3, datatype_tgfx_VAR_VEC4, datatype_tgfx_VAR_MAT4x4
} datatype_tgfx;

typedef enum cubeface_tgfx {
    cubeface_tgfx_FRONT = 0,
    cubeface_tgfx_BACK = 1,
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

typedef enum drawpassload_tgfx {
    //All values will be cleared to a certain value
    drawpassload_tgfx_CLEAR,
    //You don't need previous data, just gonna ignore and overwrite on them
    drawpassload_tgfx_FULL_OVERWRITE,
    //You need previous data, so previous data will affect current draw calls
    drawpassload_tgfx_LOAD
} drawpassload_tgfx;

typedef enum depthtest_tgfx {
    depthtest_tgfx_ALWAYS,
    depthtest_tgfx_NEVER,
    depthtest_tgfx_LESS,
    depthtest_tgfx_LEQUAL,
    depthtest_tgfx_GREATER,
    depthtest_tgfx_GEQUAL
} depthtest_tgfx;

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

typedef enum stencilcompare_tgfx {
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

typedef enum stencilop_tgfx {
    stencilop_tgfx_DONT_CHANGE = 0,
    stencilop_tgfx_SET_ZERO = 1,
    stencilop_tgfx_CHANGE = 2,
    stencilop_tgfx_CLAMPED_INCREMENT,
    stencilop_tgfx_WRAPPED_INCREMENT,
    stencilop_tgfx_CLAMPED_DECREMENT,
    stencilop_tgfx_WRAPPED_DECREMENT,
    stencilop_tgfx_BITWISE_INVERT
} stencilop_tgfx;



typedef enum blendfactor_tgfx {
    blendfactor_tgfx_ONE = 0,
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

typedef enum colorcomponents_tgfx {
    colorcomponents_tgfx_NONE = 0,
    colorcomponents_tgfx_ALL = 1,
    colorcomponents_tgfx_R = 2,
    colorcomponents_tgfx_G = 3,
    colorcomponents_tgfx_B = 4,
    colorcomponents_tgfx_RG = 5,
    colorcomponents_tgfx_GB = 6,
    colorcomponents_tgfx_RB = 7,
    colorcomponents_tgfx_RGB = 8,
} colorcomponents_tgfx;

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

typedef enum rendernodetypes_tgfx {
    rendernodetypes_tgfx_SUBDRAWPASS,
    rendernodetypes_tgfx_COMPUTEPASS,
    rendernodetypes_tgfx_TRANSFERPASS
} rendernodetypes_tgfx;

typedef enum texture_dimensions_tgfx {
    texture_dimensions_tgfx_2D = 0,
    texture_dimensions_tgfx_3D = 1,
    texture_dimensions_tgfx_2DCUBE = 2
}  texture_dimensions_tgfx;

typedef enum texture_mipmapfilter_tgfx {
    texture_mipmapfilter_tgfx_NEAREST_FROM_1MIP,
    texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP,
    texture_mipmapfilter_tgfx_NEAREST_FROM_2MIP,
    texture_mipmapfilter_tgfx_LINEAR_FROM_2MIP
}  texture_mipmapfilter_tgfx;

typedef enum texture_order_tgfx {
    texture_order_tgfx_SWIZZLE = 0,
    texture_order_tgfx_LINEAR = 1
}  texture_order_tgfx;

typedef enum texture_wrapping_tgfx {
    texture_wrapping_tgfx_REPEAT,
    texture_wrapping_tgfx_MIRRORED_REPEAT,
    texture_wrapping_tgfx_CLAMP_TO_EDGE
} texture_wrapping_tgfx;

typedef enum texture_channels_tgfx {
    texture_channels_tgfx_BGRA8UB,	//Unsigned but non-normalized char
    texture_channels_tgfx_BGRA8UNORM,	//Unsigned and normalized char

    texture_channels_tgfx_RGBA32F,
    texture_channels_tgfx_RGBA32UI,
    texture_channels_tgfx_RGBA32I,
    texture_channels_tgfx_RGBA8UB,
    texture_channels_tgfx_RGBA8B,

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
    texture_channels_tgfx_D24S8
}  texture_channels_tgfx;

typedef enum texture_access_tgfx {
    texture_access_tgfx_SAMPLER_OPERATION,
    texture_access_tgfx_IMAGE_OPERATION,
} texture_access_tgfx;


typedef enum gpu_type_tgfx {
    gpu_type_tgfx_DISCRETE_GPU,
    gpu_type_tgfx_INTEGRATED_GPU
} gpu_type_tgfx;

typedef enum vsync_tgfx {
    VSYNC_OFF,
    VSYNC_DOUBLEBUFFER,
    VSYNC_TRIPLEBUFFER
} vsync_tgfx;

typedef enum windowmode_tgfx {
    windowmode_tgfx_FULLSCREEN,
    windowmode_tgfx_WINDOWED
} windowmode_tgfx;

typedef enum backends_tgfx {
    backends_tgfx_OPENGL4 = 0,
    backends_tgfx_VULKAN = 1,
    backends_tgfx_D3D12 = 2
} backends_tgfx;

typedef enum shaderlanguages_tgfx {
    shaderlanguages_tgfx_GLSL = 0,
    shaderlanguages_tgfx_HLSL = 1,
    shaderlanguages_tgfx_SPIRV = 2,
    shaderlanguages_tgfx_TSL = 3
} shaderlanguages_tgfx;

typedef enum shaderstage_tgfx {
    shaderstage_tgfx_VERTEXSHADER = 0,
    shaderstage_tgfx_FRAGMENTSHADER = 1,
    shaderstage_tgfx_COMPUTESHADER = 2
} shaderstage_tgfx;



typedef enum barrierplace_tgfx {
    barrierplace_tgfx_ONLYSTART = 0,			//Barrier is used only at the start of the pass
    barrierplace_tgfx_BEFORE_EVERYCALL = 1,	//Barrier is used before everycall (So, at the start of the pass too)
    barrierplace_tgfx_BETWEEN_EVERYCALL = 2,	//Barrier is only used between everycall (So, it's used neither at the start nor at the end)
    barrierplace_tgfx_AFTER_EVERYCALL = 3,	//Barrier is used after everycall (So, at the end of the end too)
    barrierplace_tgfx_ONLYEND = 4				//Barrier is used only at the end the pass
} barrierplace_tgfx;
typedef enum transferpasstype_tgfx {
    transferpasstype_tgfx_BARRIER = 0,
    transferpasstype_tgfx_COPY = 1
} transferpasstype_tgfx;

//Don't forget that TGFX stores how you access them in shaders
//So backend'll probably cull some unnecessary transitions
typedef enum image_access_tgfx {
    image_access_tgfx_RTCOLOR_READONLY = 0,
    image_access_tgfx_RTCOLOR_WRITEONLY = 1,
    image_access_tgfx_RTCOLOR_READWRITE,
    image_access_tgfx_SWAPCHAIN_DISPLAY,
    image_access_tgfx_TRANSFER_DIST,
    image_access_tgfx_TRANSFER_SRC,
    image_access_tgfx_NO_ACCESS,
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
    //TODO: Extensions will be supported in the future
    shaderdescriptortype_tgfx_EXT_UNIFORMBUFFER,
    shaderdescriptortype_tgfx_VKEXT_UNIFORMBLOCK
} shaderinputtype_tgfx;

typedef enum memoryallocationtype_tgfx {
    memoryallocationtype_DEVICELOCAL = 0,
    memoryallocationtype_HOSTVISIBLE = 1,
    memoryallocationtype_FASTHOSTVISIBLE = 2,
    memoryallocationtype_READBACK = 3
} memoryallocationtype_tgfx;


typedef enum constantsampler_color {
    BLACK_ALPHA0 = 0,
    BLACK_ALPHA1 = 1,
    WHITE_ALPHA1 = 2
};

typedef enum colorspace_tgfx {
    colorspace_tgfx_sRGB_NONLINEAR
};

typedef enum windowcomposition_tgfx {
    windowcomposition_tgfx_OPAQUE
};

typedef enum windowpresentation_tgfx {
    windowpresentation_tgfx_FIFO,
    windowpresentation_tgfx_FIFO_RELAXED,
    windowpresentation_tgfx_IMMEDIATE,
    windowpresentation_tgfx_MAILBOX
};

            //CALLBACKS

typedef void (*tgfx_windowResizeCallback)(window_tgfx_handle WindowHandle, void* UserPointer, unsigned int WIDTH, unsigned int HEIGHT,
    texture_tgfx_handle* SwapchainTextureHandles);
typedef void (*tgfx_PrintLogCallback)(result_tgfx result, const char* text);
    
            //SYSTEMS
typedef struct renderer_tgfx renderer_tgfx;
typedef struct gpudatamanager_tgfx gpudatamanager_tgfx;
typedef struct dearimgui_tgfx dearimgui_tgfx;
typedef struct helper_tgfx helper_tgfx;