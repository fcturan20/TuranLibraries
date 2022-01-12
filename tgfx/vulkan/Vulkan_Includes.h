#pragma once
#define TAPI_THREADING_CPP_HELPER
#include <TAPI/TURANAPI.h>
#include <TGFX/GFX_Core.h>
#include <glm/glm.hpp>

//Vulkan Libs
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <atomic>


//#define VULKAN_DEBUGGING

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
typedef struct GPUDescription {
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
} GPUDescription;
*/

class Vulkan_Core;
extern Vulkan_Core* VKCORE;
class Renderer;
extern Renderer* VKRENDERER;
class vk_gpudatamanager;
extern vk_gpudatamanager* VKContentManager;
class IMGUI_VK;
extern IMGUI_VK* VK_IMGUI;
struct GPU;
extern GPU* RENDERGPU;
extern tapi_threadingsystem JobSys;
class VK_SemaphoreSystem;
extern VK_SemaphoreSystem* SEMAPHORESYS;
class VK_FenceSystem;
extern VK_FenceSystem* FENCESYS;


struct VK_Extension {
	enum class EXTTYPE : unsigned int {
		UNDEFINED = 0
	};
	EXTTYPE TYPE = EXTTYPE::UNDEFINED;
	void* DATA = nullptr;
};

static VkInstance Vulkan_Instance;
static VkApplicationInfo Application_Info;
static std::vector<VkExtensionProperties> Supported_InstanceExtensionList;
static std::vector<const char*> Active_InstanceExtensionNames;
void SetGFXHelperFunctions();

//Extensions
static bool isActive_SurfaceKHR = false, isSupported_PhysicalDeviceProperties2 = false;

//Forward declarations
struct VK_Texture; struct VK_MemoryAllocation; struct VK_IRTSLOTSET;

struct InitializationSecondStageInfo {
	GPU* RENDERERGPU;
	//Global Descriptors Info
	unsigned int MaxMaterialCount, MaxSumMaterial_SampledTexture, MaxSumMaterial_ImageTexture, MaxSumMaterial_UniformBuffer, MaxSumMaterial_StorageBuffer,
		GlobalShaderInput_UniformBufferCount, GlobalShaderInput_StorageBufferCount, GlobalShaderInput_SampledTextureCount, GlobalShaderInput_ImageTextureCount;
	bool shouldActivate_DearIMGUI : 1, isUniformBuffer_Index1 : 1, isSampledTexture_Index1 : 1;

};

struct VK_ShaderStageFlag {
	VkShaderStageFlags flag;
};

enum class RenderGraphStatus : unsigned char {
	Invalid = 0,
	StartedConstruction = 1,
	FinishConstructionCalled = 2,
	HalfConstructed = 3,
	Valid = 4
};
struct VK_SubDrawPassDesc {
	VK_IRTSLOTSET* INHERITEDSLOTSET;
	tgfx_subdrawpass_access WaitOp, ContinueOp;
	unsigned char SubDrawPass_Index;
};
struct VK_ShaderInputDesc {
	bool isGeneral = false;
	tgfx_shaderinput_type TYPE = tgfx_shaderinput_type_UNDEFINED;
	unsigned int BINDINDEX = UINT32_MAX;
	unsigned int ELEMENTCOUNT = 0;
	VK_ShaderStageFlag ShaderStages;
};
struct VK_RTSlotDesc {
	VK_Texture* TextureHandles[2]{ NULL };
	tgfx_operationtype OPTYPE = tgfx_operationtype_UNUSED, OPTYPESTENCIL = tgfx_operationtype_UNUSED;
	tgfx_drawpassload LOADTYPE = tgfx_drawpassload_LOAD, LOADTYPESTENCIL = tgfx_drawpassload_LOAD;
	bool isUSEDLATER = false;
	unsigned char SLOTINDEX = 255;
	glm::vec4 CLEAR_VALUE;
};
struct VK_RTSlotUsage {
	bool IS_DEPTH = false;
	tgfx_operationtype OPTYPE = tgfx_operationtype_UNUSED, OPTYPESTENCIL = tgfx_operationtype_UNUSED;
	unsigned char SLOTINDEX = 255;
};
struct VK_DepthSettingDescription {
	VkBool32 ShouldWrite = VK_FALSE;
	VkCompareOp DepthCompareOP = VkCompareOp::VK_COMPARE_OP_MAX_ENUM;
	//DepthBounds Extension
	VkBool32 DepthBoundsEnable = VK_FALSE; float DepthBoundsMin = FLT_MIN, DepthBoundsMax = FLT_MAX;
};
struct VK_StencilDescription {
	VkStencilOpState OPSTATE;
};
struct VK_BlendingInfo {
	unsigned char COLORSLOT_INDEX = 255;
	glm::vec4 BLENDINGCONSTANTs = glm::vec4(FLT_MAX);
	VkPipelineColorBlendAttachmentState BlendState = {};
};

	//Initializes as everything is false (same as CreateInvalidNullFlag)
struct VK_QUEUEFLAG {
	bool is_GRAPHICSsupported : 1;
	bool is_PRESENTATIONsupported : 1;
	bool is_COMPUTEsupported : 1;
	bool is_TRANSFERsupported : 1;
	bool doesntNeedAnything : 1;	//This is a special flag to be used as "Don't care other parameters, this is a special operation"
	VK_QUEUEFLAG();
	static VK_QUEUEFLAG CreateInvalidNullFlag();	//Returned flag's every bit is false. You should set at least one of them as true.
	bool isFlagValid() const;
	//bool is_VTMEMsupported : 1;	Not supported for now!
};

struct VK_CommandBuffer {
	typedef unsigned int CommandBufferIDType;
	static constexpr CommandBufferIDType INVALID_ID = UINT32_MAX;
	VkCommandBuffer CB;
	bool is_Used = false;
	CommandBufferIDType ID = INVALID_ID;
};

struct VK_CommandPool {
	VK_CommandPool();
	VK_CommandPool(const VK_CommandPool& RefCP);
	void operator= (const VK_CommandPool& RefCP);
	VK_CommandBuffer& CreateCommandBuffer();
	void DestroyCommandBuffer();
private:
	std::mutex Sync;
	std::vector<VK_CommandBuffer> CBs;
	VkCommandPool CPHandle = VK_NULL_HANDLE;
};

struct VK_Fence {
	typedef unsigned int FenceIDType;
	static constexpr FenceIDType INVALID_ID = UINT32_MAX;
	VkFence Fence_o = VK_NULL_HANDLE;
	bool is_Used = false;
	const FenceIDType ID;
	VK_Fence(FenceIDType id);
};

class VK_FenceSystem {
public:
	VK_Fence& CreateFence();
	void DestroyFence(VK_Fence::FenceIDType FenceID);
private:
	std::vector<VK_Fence> Fences;
};

struct VK_QUEUE {
	void Initialize(VkQueue QUEUE, float PRIORITY);
	~VK_QUEUE();
private:
	VK_Fence::FenceIDType RenderGraphFences[2];
	float Priority = 0.0f;
	VkQueue Queue = VK_NULL_HANDLE;
};

struct VK_QUEUEFAM {
	void Initialize(const VkQueueFamilyProperties& FamProps, unsigned int QueueFamIndex, const std::vector<VkQueue>& Queues);
	inline unsigned char QUEUEFEATURESCORE() const { return QueueFeatureScore; }
	inline VK_QUEUEFLAG QUEUEFLAG() const { return SupportFlag; }
	inline unsigned int QUEUECOUNT() const { return QueueCount; }
	inline uint32_t QUEUEFAMINDEX() const { return QueueFamilyIndex; }
	inline void SUPPORT_PRESENTATION() { if (!SupportFlag.is_PRESENTATIONsupported) { SupportFlag.is_PRESENTATIONsupported = true; QueueFeatureScore++; } }
	~VK_QUEUEFAM();
private:
	VK_QUEUEFLAG SupportFlag;
	VK_QUEUE* QUEUEs;
	unsigned int QueueCount;
	uint32_t QueueFamilyIndex;
	unsigned char QueueFeatureScore = 0;
};

struct VK_SubAllocation {
	VkDeviceSize Size = 0, Offset = 0;
	std::atomic<bool> isEmpty;
	VK_SubAllocation();
	VK_SubAllocation(const VK_SubAllocation& copyblock);
};

struct VK_MemoryAllocation {
	tapi_atomic_uint UnusedSize = 0;
	uint32_t MemoryTypeIndex;
	unsigned long long MaxSize = 0, ALLOCATIONSIZE = 0;
	void* MappedMemory;
	tgfx_memoryallocationtype TYPE;

	VkDeviceMemory Allocated_Memory;
	VkBuffer Buffer;
	tapi_threadlocal_vector<VK_SubAllocation> Allocated_Blocks;
	VK_MemoryAllocation();
	VK_MemoryAllocation(const VK_MemoryAllocation& copyALLOC);
	//Use this function in Suballocate_Buffer and Suballocate_Image after you're sure that you have enough memory in the allocation
	VkDeviceSize FindAvailableOffset(VkDeviceSize RequiredSize, VkDeviceSize AlignmentOffset, VkDeviceSize RequiredAlignment);
	//Free a block
	void FreeBlock(VkDeviceSize Offset);
};

//Forward declarations for struct that will be used in Vulkan Versioning/Extension system
struct VK_DEPTHSTENCILSLOT;
//Some features needs extensions and needs to be chained while creating Vulkan Logical Device
//Chained structs are stored here, which are deleted after GPU creation
struct DeviceExtendedFeatures {
	VkPhysicalDeviceDescriptorIndexingFeatures DescIndexingFeatures = {};
};
enum class DescType : unsigned char {
	IMAGE,
	SAMPLER,
	UBUFFER,
	SBUFFER
};
struct GPU {
private:
	std::string NAME;
	unsigned int APIVER = UINT32_MAX, DRIVERVER = UINT32_MAX;
	tgfx_gpu_types GPUTYPE;

	VkPhysicalDevice Physical_Device = {};
	VkPhysicalDeviceProperties Device_Properties = {};
	VkPhysicalDeviceMemoryProperties MemoryProperties = {};
	VkQueueFamilyProperties* QueueFamilyProperties; unsigned int QueueFamiliesCount = 0;
	//Use SortedQUEUEFAMsLIST to access queue families in increasing feature score order
	VK_QUEUEFAM* QUEUEFAMs; unsigned int* SortedQUEUEFAMsLIST;
	//If GRAPHICS_QUEUEIndex is UINT32_MAX, Graphics isn't supported by the device
	unsigned int TRANSFERs_supportedqueuecount = 0, COMPUTE_supportedqueuecount = 0, GRAPHICS_QUEUEFamIndex = UINT32_MAX;
	VkDevice Logical_Device = {};
	VkExtensionProperties* Supported_DeviceExtensions; unsigned int Supported_DeviceExtensionsCount = 0;
	std::vector<const char*> Active_DeviceExtensions;
	VkPhysicalDeviceFeatures Supported_Features = {}, Active_Features = {};

	std::vector<VK_MemoryAllocation> ALLOCs;
	uint32_t* AllQueueFamilies;


	struct DeviceExtensions {
	private:
		//Swapchain
		bool SwapchainDisplay = false;

		//Seperated Depth Stencil
		bool SeperatedDepthStencilLayouts = false;

		//If GPU supports descriptor indexing, it is activated!
		bool isDescriptorIndexingSupported = false;
		//These limits are maximum count of defined resources in material linking (including global buffers and textures)
		//That means; at specified shader input type-> all global shader inputs + general shader inputs + per instance shader inputs shouldn't exceed the related limit
		unsigned int MaxDesc_SampledTexture = 0, MaxDesc_ImageTexture = 0, MaxDesc_UniformBuffer = 0, MaxDesc_StorageBuffer = 0, MaxDesc_ALL = 0, MaxAccessibleDesc_PerStage = 0;
	public:
		DeviceExtensions();
		void (*Fill_DepthAttachmentReference)(VkAttachmentReference& Ref, unsigned int index,
			tgfx_texture_channels channels, tgfx_operationtype DEPTHOPTYPE, tgfx_operationtype STENCILOPTYPE);
		void (*Fill_DepthAttachmentDescription)(VkAttachmentDescription& Desc, VK_DEPTHSTENCILSLOT* DepthSlot);

		//GETTER-SETTERS
		inline bool ISSUPPORTED_DESCINDEXING() { return isDescriptorIndexingSupported; }
		inline unsigned int& GETMAXDESC(DescType TYPE) { switch (TYPE) { case DescType::IMAGE: return MaxDesc_ImageTexture; case DescType::SAMPLER: return MaxDesc_SampledTexture; case DescType::SBUFFER: return MaxDesc_StorageBuffer; case DescType::UBUFFER: return MaxDesc_UniformBuffer; } }
		inline unsigned int& GETMAXDESCALL() { return MaxDesc_ALL; }
		inline unsigned int& GETMAXDESCPERSTAGE() { return MaxAccessibleDesc_PerStage; }
		inline bool ISSUPPORTED_SEPERATEDDEPTHSTENCILLAYOUTS() { return SeperatedDepthStencilLayouts; }
		inline bool ISSUPPORTED_SWAPCHAINDISPLAY() { return SwapchainDisplay; }

		//SUPPORTERS : Because all variables are private, these are to set them as true. All extensions are set as false by default.

		void SUPPORT_DESCINDEXING();
		void SUPPORT_SWAWPCHAINDISPLAY();
		void SUPPORT_SEPERATEDDEPTHSTENCILLAYOUTS();

		//ACTIVATORS : These are the supported extensions that doesn't only relaxes the rules but also adds new rules and usages.

	};




	//Initialization
	void Gather_PhysicalDeviceInformations();
	void Analize_PhysicalDeviceMemoryProperties();
	void Analize_Queues();
	const char* Convert_VendorID_toaString(uint32_t VendorID);
	void Activate_DeviceExtensions();
	void Activate_DeviceFeatures(VkDeviceCreateInfo& dev_ci, DeviceExtendedFeatures& dev_ftrs);
	void Check_DeviceLimits();
public:
	GPU(VkPhysicalDevice PhysicalDevice);
	inline const std::string DEVICENAME() { return NAME; }
	inline const unsigned int APIVERSION() { return APIVER; }
	inline const unsigned int DRIVERSION() { return DRIVERVER; }
	inline const tgfx_gpu_types DEVICETYPE() { return GPUTYPE; }
	inline const bool GRAPHICSSUPPORTED() { return (GRAPHICS_QUEUEFamIndex == UINT32_MAX) ? (false) : (true); }
	inline const bool COMPUTESUPPORTED() { return COMPUTE_supportedqueuecount; }
	inline const bool TRANSFERSUPPORTED() { return TRANSFERs_supportedqueuecount; }
	inline VkPhysicalDevice PHYSICALDEVICE() { return Physical_Device; }
	inline const VkPhysicalDeviceProperties& DEVICEPROPERTIES() const { return Device_Properties; }
	inline const VkPhysicalDeviceMemoryProperties& MEMORYPROPERTIES() const { return MemoryProperties; }
	inline VkQueueFamilyProperties* QUEUEFAMILYPROPERTIES() const { return QueueFamilyProperties; }
	inline VK_QUEUEFAM* QUEUEFAMS() const { return QUEUEFAMs; }
	inline const uint32_t* ALLQUEUEFAMILIES() const { return AllQueueFamilies; }
	inline unsigned int QUEUEFAMSCOUNT() const { return QueueFamiliesCount; }
	inline unsigned int TRANSFERSUPPORTEDQUEUECOUNT() { return TRANSFERs_supportedqueuecount; }
	inline VkDevice LOGICALDEVICE() { return Logical_Device; }
	inline VkExtensionProperties* EXTPROPERTIES() { return Supported_DeviceExtensions; }
	inline VkPhysicalDeviceFeatures DEVICEFEATURES_SUPPORTED() { return Supported_Features; }
	inline VkPhysicalDeviceFeatures DEVICEFEATURES_ACTIVE() { return Active_Features; }
	inline std::vector<VK_MemoryAllocation>& ALLOCS() { return ALLOCs; }
	inline unsigned int GRAPHICSQUEUEINDEX() { return GRAPHICS_QUEUEFamIndex; }
	//This function searches the best queue that has least specs but needed specs
	//For example: Queue 1->Graphics,Transfer,Compute - Queue 2->Transfer, Compute - Queue 3->Transfer
	//If flag only supports Transfer, then it is Queue 3
	//If flag only supports Compute, then it is Queue 2
	//This is because user probably will use Direct Queues (Graphics,Transfer,Compute supported queue and every GPU has at least one)
	//Users probably gonna leave the Direct Queue when they need async operations (Async compute or Async Transfer)
	//Also if flag doesn't need anything (probably Barrier TP only), returns nullptr
	VK_QUEUEFAM* Find_BestQueue(const VK_QUEUEFLAG& Branch);
	bool DoesQueue_Support(const VK_QUEUEFAM* QUEUEFAM, const VK_QUEUEFLAG& FLAG) const;
	bool isWindowSupported(VkSurfaceKHR WindowSurface);

	DeviceExtensions ExtensionRelatedDatas;
};

struct vkMONITOR {
	unsigned int WIDTH, HEIGHT, COLOR_BITES, REFRESH_RATE, PHYSICALWIDTH, PHYSICALHEIGHT;
	const char* NAME;
	//I will fill this structure when I investigate monitor configurations deeper!
};

struct VK_Semaphore {
#ifdef VULKAN_DEBUGGING
	typedef unsigned int SemaphoreIDType;
	static constexpr SemaphoreIDType INVALID_ID = UINT32_MAX;
#else
	typedef VK_Semaphore* SemaphoreIDType;
	static constexpr SemaphoreIDType INVALID_ID = nullptr;
#endif
	inline SemaphoreIDType Get_ID() {
#ifdef VULKAN_DEBUGGING 
		return ID;
#else
		return this;
#endif
	}
	inline VkSemaphore VKSEMAPHORE() { return SPHandle; }
	inline bool ISUSED() { return isUsed; };
private:
	VkSemaphore SPHandle = VK_NULL_HANDLE;
	bool isUsed = false;
#ifdef VULKAN_DEBUGGING
	SemaphoreIDType ID = INVALID_ID;
	friend class VK_SemaphoreSystem;
#endif
};
struct VK_SemaphoreSystem {
	static void ClearList();
	//Searches for the previously created but later destroyed semaphores, if there isn't
	//Then creates a new VkSemaphore object
	static VK_Semaphore& Create_Semaphore();
	//If you semaphore object is signaled then unsignaled, you should destroy it
	//Vulkan object won't be destroyed, it is gonna be marked as "Usable" and then recycled
	//Create_Semaphore() will return the destroyed semaphore (because it is just marked as "usable").
	static void DestroySemaphore(VK_Semaphore::SemaphoreIDType SemaphoreID);
	inline static VK_Semaphore& GetSemaphore_byID(VK_Semaphore::SemaphoreIDType ID) {
#ifdef VULKAN_DEBUGGING
		return GetSemaphore(ID); 
#else
		return *(VK_Semaphore*)ID;
#endif
	}
	VK_SemaphoreSystem();
	~VK_SemaphoreSystem();
private:
	//First object in the list is an invalid semaphore to return in GetSemaphore() functions!
	std::vector<VK_Semaphore*> Semaphores;
#ifdef VULKAN_DEBUGGING
	bool isSorted = true;
	static void Sort();
	static VK_Semaphore& GetSemaphore(VK_Semaphore::SemaphoreIDType ID);
#endif
};


struct VK_Texture;
struct WINDOW {
	unsigned int LASTWIDTH, LASTHEIGHT, NEWWIDTH, NEWHEIGHT;
	tgfx_windowmode DISPLAYMODE;
	vkMONITOR* MONITOR;
	std::string NAME;
	VkImageUsageFlags SWAPCHAINUSAGE;
	tgfx_windowResizeCallback resize_cb = nullptr;
	void* UserPTR;

	VkSurfaceKHR Window_Surface = {};
	VkSwapchainKHR Window_SwapChain = {};
	GLFWwindow* GLFW_WINDOW = {};
	VK_Semaphore::SemaphoreIDType PresentationSemaphores[2];
	unsigned char CurrentFrameSWPCHNIndex = 0;
	VK_Texture* Swapchain_Textures[2];
	VkSurfaceCapabilitiesKHR SurfaceCapabilities = {};
	std::vector<VkSurfaceFormatKHR> SurfaceFormats;
	std::vector<VkPresentModeKHR> PresentationModes;
	bool isResized = false;
};




namespace WaitSignal {
	//Maximum count of types is 64
	enum class WaitType : unsigned char {
		DRAW_INDIRECT_CONSUME,
		VERTEX_INPUT,
		VERTEX_SHADER,
		GEOMETRY_SHADER,
		EARLY_FRAGMENTTESTS,
		LATE_FRAGMENTTESTS,
		COLOR_ATTACHMENTOUTPUT,
		FRAGMENT_SHADER,
		COMPUTE_SHADER,
		TRANSFER
	};


	struct VKWaitSignalData {
		uint16_t Operations : 9;
		//If this is true, that means this WaitSignalData is in a list and the pointer should be incremented to find other waitsignal datas
		uint16_t isThereNext : 1;
		WaitType Type : 6;
		inline uint16_t GetData() {
			uint16_t type_16bit = static_cast<unsigned char>(Type);
			uint16_t data = (type_16bit << 10) | (isThereNext << 9) | Operations;
			return data;
		}
	};
}

VkFormat Find_VkFormat_byDataType(tgfx_datatype datatype);
VkFormat Find_VkFormat_byTEXTURECHANNELs(tgfx_texture_channels channels);
VkDescriptorType Find_VkDescType_byMATDATATYPE(tgfx_shaderinput_type TYPE);
VkSamplerAddressMode Find_AddressMode_byWRAPPING(tgfx_texture_wrapping Wrapping);
VkFilter Find_VkFilter_byGFXFilter(tgfx_texture_mipmapfilter filter);
VkSamplerMipmapMode Find_MipmapMode_byGFXFilter(tgfx_texture_mipmapfilter filter);
VkCullModeFlags Find_CullMode_byGFXCullMode(tgfx_cullmode mode);
VkPolygonMode Find_PolygonMode_byGFXPolygonMode(tgfx_polygonmode mode);
VkPrimitiveTopology Find_PrimitiveTopology_byGFXVertexListType(tgfx_vertexlisttypes type);
VkIndexType Find_IndexType_byGFXDATATYPE(tgfx_datatype datatype);
VkCompareOp Find_CompareOp_byGFXDepthTest(tgfx_depthtests test);
void Find_DepthMode_byGFXDepthMode(tgfx_depthmodes mode, VkBool32& ShouldTest, VkBool32& ShouldWrite);
VkAttachmentLoadOp Find_LoadOp_byGFXLoadOp(tgfx_drawpassload load);
VkCompareOp Find_CompareOp_byGFXStencilCompare(tgfx_stencilcompare op);
VkStencilOp Find_StencilOp_byGFXStencilOp(tgfx_stencilop op);
VkBlendOp Find_BlendOp_byGFXBlendMode(tgfx_blendmode mode);
VkBlendFactor Find_BlendFactor_byGFXBlendFactor(tgfx_blendfactor factor);
void Fill_ComponentMapping_byCHANNELs(tgfx_texture_channels channels, VkComponentMapping& mapping);
void Find_SubpassAccessPattern(tgfx_subdrawpass_access access, bool isSource, VkPipelineStageFlags& stageflag, VkAccessFlags& accessflag);
DescType Find_DescType_byGFXShaderInputType(tgfx_shaderinput_type type);
VkDescriptorType Find_VkDescType_byDescTypeCategoryless(DescType type);