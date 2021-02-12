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

	enum class OPERATION_TYPE : unsigned char {
		READ_ONLY,
		WRITE_ONLY,
		READ_AND_WRITE,
		UNUSED
	};

	enum class DRAWPASS_LOAD : unsigned char {
		//All values will be cleared to a certain value
		CLEAR,
		//You don't need previous data, just gonna ignore and overwrite on them
		FULL_OVERWRITE,
		//You need previous data, so previous data will affect current draw calls
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

	enum class BUFFER_TYPE : unsigned char {
		STAGING,
		VERTEX,
		INDEX,
		GLOBAL
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

	enum class TEXTURE_ORDER : unsigned char {
		SWIZZLE = 0,
		LINEAR = 1
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
		API_TEXTURE_BGRA8UB,	//Unsigned but non-normalized char
		API_TEXTURE_BGRA8UNORM,	//Unsigned and normalized char

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
		bool SWAPCHAINDISPLAY : 1;
		SHADERSTAGEs_FLAG();
	};
	SHADERSTAGEs_FLAG Create_ShaderStageFlag(bool vs, bool fs, bool rt_output, bool transfercmd, bool swpchn_diplay);

	struct GFXAPI TEXTUREUSAGEFLAG {
		//bool hasMipMaps			: 1;	//I don't support it for now!
		bool isCopiableFrom : 1;	//If it is true, other textures or buffers are able to copy something from this texture
		bool isCopiableTo : 1;	//If it is true, this texture may copy data from other buffers or textures
		bool isRenderableTo : 1;	//If it is true, it is a Render Target for at least one DrawPass
		bool isSampledReadOnly : 1;	//If it is true, it is accessed as a uniform texture that you're not able to write to it in the shader
		bool isRandomlyWrittenTo : 1;	//If it is true, compute and draw pipeline shaders are able to write to it (Render Target isn't considered here)
		TEXTUREUSAGEFLAG();
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
		TP_COPY = 1
	};
	
	enum class IMAGE_ACCESS : unsigned char {
		RTCOLOR_READONLY	= 0,
		RTCOLOR_WRITEONLY	= 1,
		RTCOLOR_READWRITE,
		DEPTHSTENCIL_READONLY,
		DEPTHSTENCIL_WRITEONLY,
		DEPTHSTENCIL_READWRITE,
		SWAPCHAIN_DISPLAY,
		TRANSFER_DIST,
		TRANSFER_SRC,
		NO_ACCESS,
		SHADER_SAMPLEONLY,
		SHADER_WRITEONLY,
		SHADER_SAMPLEWRITE
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

	enum class SUBALLOCATEBUFFERTYPEs : unsigned char {
		DEVICELOCAL = 0,
		HOSTVISIBLE = 1,
		FASTHOSTVISIBLE = 2,
		READBACK = 3
	};

	struct GFXAPI BoxRegion {
		unsigned int Width, Height, Depth, WidthOffset, HeightOffset, DepthOffset;
	};

	struct GFXAPI Texture_Properties {
		TEXTURE_DIMENSIONs DIMENSION = TEXTURE_DIMENSIONs::TEXTURE_2D;
		TEXTURE_MIPMAPFILTER MIPMAP_FILTERING = TEXTURE_MIPMAPFILTER::API_TEXTURE_LINEAR_FROM_1MIP;
		TEXTURE_WRAPPING WRAPPING = TEXTURE_WRAPPING::API_TEXTURE_REPEAT;
		TEXTURE_CHANNELs CHANNEL_TYPE = TEXTURE_CHANNELs::API_TEXTURE_RGB8UB;
		TEXTURE_ORDER DATAORDER = TEXTURE_ORDER::SWIZZLE;
		Texture_Properties();
		Texture_Properties(TEXTURE_DIMENSIONs dimension, TEXTURE_MIPMAPFILTER mipmap_filtering = TEXTURE_MIPMAPFILTER::API_TEXTURE_LINEAR_FROM_1MIP,
			TEXTURE_WRAPPING wrapping = TEXTURE_WRAPPING::API_TEXTURE_REPEAT, TEXTURE_CHANNELs channel_type = TEXTURE_CHANNELs::API_TEXTURE_RGB8UB, TEXTURE_ORDER = TEXTURE_ORDER::SWIZZLE);
	};

	/*
		Texture Resource Specifications:
			1) You can use textures to just read on GPU (probably for object material rendering), read-write on GPU (using compute shader to write an image), render target (framebuffer attachment)
			2) Modern APIs let you use a texture in anyway, it's left for your care.
			But OpenGL doesn't let this situation, you are defining your texture's type in creation proccess
			So for now, GFXAPI uses Modern APIs' way and you can use a texture in anyway you want
			But you should use UsageBarrier for this kind of usage and specify it in RenderGraph
			3) You can't use Staging Buffers to store and access textures, they are just for transfer operations
	*/
	struct GFXAPI Texture_Description {
	public:
		Texture_Properties Properties;
		unsigned int WIDTH, HEIGHT;
		TEXTUREUSAGEFLAG USAGE;
	};




	struct GFXAPI MemoryType {
		const SUBALLOCATEBUFFERTYPEs HEAPTYPE;
		const unsigned int MemoryTypeIndex;
		unsigned int AllocationSize = 0;
		MemoryType(SUBALLOCATEBUFFERTYPEs HEAP, unsigned int MemoryTypeIndex);
	};

	struct GFXAPI MonitorDescription {
		GFXHandle Handle;
		string NAME;
		unsigned int WIDTH, HEIGHT, COLOR_BITES, REFRESH_RATE;
		int PHYSICAL_WIDTH, PHYSICAL_HEIGHT;	//milimeters
		WINDOW_MODE DESKTOP_MODE;
	};

	struct GFXAPI GPUDescription {
	public:
		string MODEL;
		uint32_t API_VERSION;
		uint32_t DRIVER_VERSION;
		GPU_TYPEs GPU_TYPE;
		bool is_GraphicOperations_Supported = false, is_ComputeOperations_Supported = false, is_TransferOperations_Supported = false;
		vector<MemoryType> MEMTYPEs;
		uint64_t DEVICELOCAL_MaxMemorySize, HOSTVISIBLE_MaxMemorySize, FASTHOSTVISIBLE_MaxMemorySize, READBACK_MaxMemorySize;
	};

	struct GFXAPI WindowDescription {
		const char* NAME;
		unsigned int WIDTH, HEIGHT;
		GFX_API::WINDOW_MODE MODE;
		GFX_API::GFXHandle MONITOR;
		TEXTUREUSAGEFLAG SWAPCHAINUSAGEs;
	};

}