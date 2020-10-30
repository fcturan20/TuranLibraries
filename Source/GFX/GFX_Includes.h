#pragma once
#include "TuranAPI/API_includes.h"
#include "TuranAPI/Logger_Core.h"
#include "TuranAPI/FileSystem_Core.h"
#include "TuranAPI/Bitset.h"

/*
#ifdef WINDOWS10_FORENGINE
	#ifdef GFX_BUILD
		#define GFXAPI __declspec(dllexport)
	#else
		#define GFXAPI __declspec(dllimport)
	#endif // GFX_API
#else
	#error GFX_API only support Windows 10 for now!
#endif*/

#define GFXAPI
namespace GFX_API {
	struct GFXAPI GFXHandle {
		operator void* () const;
		GFXHandle(void* PTR);
		GFXHandle();
	private:
		void* Handle;
	};

	//Variable Types!
	enum class DATA_TYPE : unsigned char {
		VAR_UBYTE8 = 0, VAR_BYTE8, VAR_UINT32, VAR_INT32, VAR_FLOAT32,
		VAR_VEC2, VAR_VEC3, VAR_VEC4, VAR_MAT4x4
	};
	GFXAPI unsigned int Get_UNIFORMTYPEs_SIZEinbytes(DATA_TYPE uniform);
	GFXAPI const char* Find_UNIFORM_VARTYPE_Name(DATA_TYPE uniform_var_type);
	/*
	enum class RT_ATTACHMENTs : unsigned char {
		TEXTURE_ATTACHMENT_COLOR,
		TEXTURE_ATTACHMENT_DEPTH,
		TEXTURE_ATTACHMENT_DEPTHSTENCIL
	};*/

	enum class OPERATION_TYPE : unsigned char {
		READ_ONLY,
		WRITE_ONLY,
		READ_AND_WRITE,
		UNUSED
	};

	enum class DRAWPASS_LOAD : unsigned char {
		CLEAR,
		FULL_OVERWRITE,
		LOAD
	};

	enum class DEPTH_TESTs : unsigned char {
		DEPTH_TEST_ALWAYS,
		DEPTH_TEST_NEVER,
		DEPTH_TEST_LESS,
		DEPTH_TEST_LEQUAL,
		DEPTH_TEST_GREATER,
		DEPTH_TEST_GEQUAL
	};

	enum class DEPTH_MODEs : unsigned char {
		DEPTH_READ_WRITE,
		DEPTH_READ_ONLY,
		DEPTH_OFF
	};

	enum class CULL_MODE : unsigned char {
		CULL_OFF,
		CULL_BACK,
		CULL_FRONT
	};

	enum class RENDERNODE_TYPEs {
		SUBDRAWPASS,
		COMPUTEPASS,
		TRANSFERPASS
	};

	enum class TEXTURE_DIMENSIONs : unsigned char {
		TEXTURE_2D = 0,
		TEXTURE_3D = 1
	};

	enum class TEXTURE_MIPMAPFILTER : unsigned char {
		API_TEXTURE_NEAREST_FROM_1MIP,
		API_TEXTURE_LINEAR_FROM_1MIP,
		API_TEXTURE_NEAREST_FROM_2MIP,
		API_TEXTURE_LINEAR_FROM_2MIP
	};

	enum class TEXTURE_WRAPPING : unsigned char {
		API_TEXTURE_REPEAT,
		API_TEXTURE_MIRRORED_REPEAT,
		API_TEXTURE_CLAMP_TO_EDGE
	};
	GFXAPI const char* GetNameOf_TextureWRAPPING(TEXTURE_WRAPPING WRAPPING);
	GFXAPI vector<const char*> GetNames_TextureWRAPPING();
	GFXAPI TEXTURE_WRAPPING GetTextureWRAPPING_byIndex(unsigned int Index);

	enum class TEXTURE_CHANNELs : unsigned char {
		API_TEXTURE_RGBA32F,
		API_TEXTURE_RGBA32UI,
		API_TEXTURE_RGBA32I,
		API_TEXTURE_RGBA8UB,
		API_TEXTURE_RGBA8B,

		API_TEXTURE_RGB32F,
		API_TEXTURE_RGB32UI,
		API_TEXTURE_RGB32I,
		API_TEXTURE_RGB8UB,
		API_TEXTURE_RGB8B,

		API_TEXTURE_RA32F,
		API_TEXTURE_RA32UI,
		API_TEXTURE_RA32I,
		API_TEXTURE_RA8UB,
		API_TEXTURE_RA8B,

		API_TEXTURE_R32F,
		API_TEXTURE_R32UI,
		API_TEXTURE_R32I,
		API_TEXTURE_R8UB,
		API_TEXTURE_R8B,

		API_TEXTURE_D32,
		API_TEXTURE_D24S8
	};
	GFXAPI unsigned char GetByteSizeOf_TextureChannels(const TEXTURE_CHANNELs& channeltype);
	GFXAPI const char* GetNameOf_TextureCHANNELs(TEXTURE_CHANNELs CHANNEL);
	GFXAPI vector<const char*> GetNames_TextureCHANNELs();
	GFXAPI TEXTURE_CHANNELs GetTextureCHANNEL_byIndex(unsigned int Index);
	GFXAPI unsigned int GetIndexOf_TextureCHANNEL(TEXTURE_CHANNELs CHANNEL);

	enum class TEXTURE_ACCESS : unsigned char {
		SAMPLER_OPERATION,
		IMAGE_OPERATION,
	};


	enum class GPU_TYPEs : unsigned char {
		DISCRETE_GPU,
		INTEGRATED_GPU
	};

	enum class V_SYNC : unsigned char {
		VSYNC_OFF,
		VSYNC_DOUBLEBUFFER,
		VSYNC_TRIPLEBUFFER
	};

	enum class WINDOW_MODE : unsigned char {
		FULLSCREEN,
		WINDOWED
	};

	enum class GFX_APIs : unsigned char {
		OPENGL4 = 0,
		VULKAN = 1
	};

	enum class SHADER_LANGUAGEs : unsigned char {
		GLSL = 0,
		HLSL = 1,
		SPIRV = 2,
		TSL = 3
	};
	GFXAPI const char* GetNameof_SHADERLANGUAGE(SHADER_LANGUAGEs LANGUAGE);
	GFXAPI vector<const char*> GetNames_SHADERLANGUAGEs();
	GFXAPI SHADER_LANGUAGEs GetSHADERLANGUAGE_byIndex(unsigned int Index);

	struct GFXAPI SHADERSTAGEs_FLAG {
		bool VERTEXSHADER : 1;
		bool FRAGMENTSHADER : 1;
		bool COLORRTOUTPUT : 1;
		bool TRANSFERCMD : 1;
		SHADERSTAGEs_FLAG();
	};


	//If you change this enum, don't forget that Textures and Global Buffers uses this enum. So, consider them.

	enum class BUFFER_VISIBILITY : unsigned char {
		CPUREADWRITE_GPUREADONLY = 0,			//Use this when all the data responsibility on the CPU and GPU just reads it (Global Camera Matrixes, CPU Software Rasterization Depth reads from the GPU etc.)
		CPUREADWRITE_GPUREADWRITE,				//Use this when both CPU and GPU changes the data, no matter frequency. This has the worst performance on modern APIs
		CPUREADONLY_GPUREADWRITE,				//Use this when CPU needs feedback of the data proccessed on the GPU (Occlusion Culling on the CPU according to last frame's depth buffer etc.)
		CPUEXISTENCE_GPUREADWRITE,				//Use this for CPU never touchs the data and GPU has the all responsibility (Framebuffer attachments, GPU-driven pipeline buffers etc.)
		CPUEXISTENCE_GPUREADONLY,				//Use this when data never changes, just uploaded or deleted from the GPU (Object material rendering textures, constant global buffers etc.)
	};


	enum class IMAGEUSAGE : unsigned char {
		READONLY_RTATTACHMENT = 0,				//That means you are only reading the attachment (Attachment Type will be found by GFX API)
		READWRITE_RTATTACHMENT = 1,				//That means you have both read and write access to the attachment (Attachment Type will be found by GFX API)
		SHADERTEXTURESAMPLING,					//That means you are reading texture in shader as uniform (most performant way)
		SHADERIMAGELOADING,						//That means you have both random "read and write" access in shaders to the texture (But not as RT)
		TEXTUREDATATRANSFER,					//That means you are reading or writing to this texture with GPU or CPU copy operations
		UNUSED									//That means you are using it for the first time (PREVIOUS_IMUSAGE) or not using it again (LATER_IMUSAGE) which may improve performance
	};

	enum class BARRIERPLACE : unsigned char {
		ONLYSTART = 0,			//Barrier is used only at the start of the pass
		BEFORE_EVERYCALL = 1,	//Barrier is used before everycall (So, at the start of the pass too)
		BETWEEN_EVERYCALL = 2,	//Barrier is only used between everycall (So, it's used neither at the start nor at the end)
		AFTER_EVERYCALL = 3,	//Barrier is used after everycall (So, at the end of the end too)
		ONLYEND = 4				//Barrier is used only at the end the pass
	};
	enum class TRANFERPASS_TYPE : unsigned char {
		TP_BARRIER = 0,
		TP_UPLOAD = 1,
		TP_COPY = 2,
		TP_DOWNLOAD = 3
	};


	//Appendix "PI" means per material instance, "G" means material type general
	//If there is any PI data type in Material Type, we assume that you're accessing it in shader with MaterialInstance index (Back-end handles that)
	//If there is no title, that means it's modifiable and possibly includes a double buffered implementation in GL back-end
	//CONST title means it can't be changed after the creation
	enum class MATERIALDATA_TYPE : unsigned char {
		UNDEFINED = 0,
		//DRAWCALL_DESCRIPTOR = 1,
		CONSTSAMPLER_PI,
		CONSTSAMPLER_G,
		CONSTIMAGE_PI,
		CONSTIMAGE_G,
		CONSTUBUFFER_PI,
		CONSTUBUFFER_G,
		CONSTSBUFFER_PI,
		CONSTSBUFFER_G
	};

	enum class SWAPCHAIN_IDENTIFIER : unsigned char {
		NO_SWPCHN = 0,
		CURRENTFRAME_SWPCHN = 1,
		LASTFRAME_SWPCHN = 2
	};
}