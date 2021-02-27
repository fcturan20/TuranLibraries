#pragma once
#include "GFX/GFX_Core.h"

//Vulkan Libs
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#define VK_API
#define VULKAN_DEBUGGING

namespace Vulkan {
	//Initializes as everything is false
	struct VK_QUEUEFLAG {
		bool is_GRAPHICSsupported : 1;
		bool is_PRESENTATIONsupported : 1;
		bool is_COMPUTEsupported : 1;
		bool is_TRANSFERsupported : 1;
		VK_QUEUEFLAG();
		//bool is_VTMEMsupported : 1;	Not supported for now!
	};

	struct VK_CommandBuffer {
		VkCommandBuffer CB;
		bool is_Used = false;
	};

	struct VK_CommandPool {
		std::mutex Sync;
		vector<VK_CommandBuffer> CBs;
		VkCommandPool CPHandle = VK_NULL_HANDLE;
		VK_CommandPool();
		VK_CommandPool(const VK_CommandPool& RefCP);
		void operator= (const VK_CommandPool& RefCP);
	};

	struct VK_Fence {
		VkFence Fence_o = VK_NULL_HANDLE;
		bool is_Used = false;
	};

	struct VK_API VK_QUEUE {
		VK_QUEUEFLAG SupportFlag;
		VkQueue Queue;
		uint32_t QueueFamilyIndex;
		VK_CommandPool CommandPools[2];
		VK_Fence RenderGraphFences[2];
		vector<GFX_API::GFXHandle> ActiveSubmits;
		unsigned char QueueFeatureScore = 0;
		VK_QUEUE();
	};

	struct VK_API VK_SubAllocation {
		VkDeviceSize Size = 0, Offset = 0;
		std::atomic_bool isEmpty = true;
		VK_SubAllocation();
		VK_SubAllocation(const VK_SubAllocation& copyblock);
	};

	struct VK_API VK_MemoryAllocation {
		TuranAPI::Threading::AtomicUINT UnusedSize = 0;
		uint32_t MemoryTypeIndex = 0, FullSize = 0;
		void* MappedMemory;
		GFX_API::SUBALLOCATEBUFFERTYPEs TYPE;

		VkDeviceMemory Allocated_Memory;
		VkBuffer Buffer;
		TuranAPI::Threading::TLVector<VK_SubAllocation> Allocated_Blocks;
		VK_MemoryAllocation();
		VK_MemoryAllocation(const VK_MemoryAllocation& copyALLOC);
	};

	//Forward declarations for struct that will be used in Vulkan Versioning/Extension system
	struct VK_API VK_DEPTHSTENCILSLOT;
	struct VK_API GPU {
		VkPhysicalDevice Physical_Device = {};
		VkPhysicalDeviceProperties Device_Properties = {};
		VkPhysicalDeviceMemoryProperties MemoryProperties = {};
		vector<VkQueueFamilyProperties> QueueFamilyProperties;
		vector<VK_QUEUE> QUEUEs;
		unsigned int TRANSFERs_supportedqueuecount = 0, COMPUTE_supportedqueuecount = 0;
		unsigned int GRAPHICS_QUEUEIndex = 0;
		VkDevice Logical_Device = {};
		vector<VkExtensionProperties> Supported_DeviceExtensions;
		vector<const char*> Active_DeviceExtensions;
		VkPhysicalDeviceFeatures Supported_Features = {};
		VkPhysicalDeviceFeatures Active_Features = {};

		vector<VK_MemoryAllocation> ALLOCs;
		uint32_t* AllQueueFamilies;
		//This function searches the best queue that has least specs but needed specs
		//For example: Queue 1->Graphics,Transfer,Compute - Queue 2->Transfer, Compute - Queue 3->Transfer
		//If flag only supports Transfer, then it is Queue 3
		//If flag only supports Compute, then it is Queue 2
		//This is because user probably will use Direct Queues (Graphics,Transfer,Compute supported queue and every GPU has at least one)
		//Users probably gonna leave the Direct Queue when they need async operations (Async compute or Async Transfer)
		//Also if flag doesn't need anything (probably Barrier TP only), returns nullptr
		VK_QUEUE* Find_BestQueue(const VK_QUEUEFLAG& Branch);
		bool DoesQueue_Support(const VK_QUEUE* QUEUE, const VK_QUEUEFLAG& FLAG);


		//Extension Related Functions
		bool SeperatedDepthStencilLayouts = false, SwapchainDisplay = true;
		void Fill_DepthAttachmentReference(VkAttachmentReference& Ref, unsigned int index, 
			GFX_API::TEXTURE_CHANNELs channels, GFX_API::OPERATION_TYPE DEPTHOPTYPE, GFX_API::OPERATION_TYPE STENCILOPTYPE);
		void Fill_DepthAttachmentDescription(VkAttachmentDescription& Desc, VK_DEPTHSTENCILSLOT* DepthSlot);
	};

	struct VK_API MONITOR {
		//I will fill this structure when I investigate monitor configurations deeper!
	};

	//Window will be accessed from only Vulkan.dll, so the programmer may be aware what he can cause
	struct VK_API WINDOW {
		unsigned int WIDTH, HEIGHT, DISPLAY_QUEUEIndex = 0;
		GFX_API::WINDOW_MODE DISPLAYMODE;
		GFX_API::GFXHandle MONITOR;
		string NAME;

		VkSurfaceKHR Window_Surface = {};
		VkSwapchainKHR Window_SwapChain = {};
		GLFWwindow* GLFW_WINDOW = {};
		unsigned char PresentationWaitSemaphoreIndexes[3];
		GFX_API::GFXHandle Swapchain_Textures[2];
		VkSurfaceCapabilitiesKHR SurfaceCapabilities = {};
		vector<VkSurfaceFormatKHR> SurfaceFormats;
		vector<VkPresentModeKHR> PresentationModes;
	};

	//Use this class to store Vulkan Global Variables (Such as VkInstance, VkApplicationInfo etc)
	class VK_API Vulkan_States {
		const char* const* Get_Supported_LayerNames(const VkLayerProperties* list, uint32_t length);
	public:
		Vulkan_States();
		VkInstance Vulkan_Instance;
		VkApplicationInfo Application_Info;
		vector<VkExtensionProperties> Supported_InstanceExtensionList;
		vector<const char*> Active_InstanceExtensionNames;
		GPU* GPU_TO_RENDER;

		VkLayerProperties* Supported_LayerList;
		uint32_t Supported_LayerNumber;
		VkDebugUtilsMessengerEXT Debug_Messenger;

		//CONVERT VENDOR ID TO VENDOR NAME
		const char* Convert_VendorID_toaString(uint32_t VendorID);

		//Required Extensions are defined here
		//Check if Required Extension Names are supported!
		//Returns true if all of the required extensions are supported!
		void Chech_InstanceExtensions();
		//Same for Device
		void Check_DeviceExtensions(GPU* Vulkan_GPU);
		void Check_DeviceFeatures(GPU* Vulkan_GPU, GFX_API::GPUDescription& GPUDesc);

		static VKAPI_ATTR VkBool32 VKAPI_CALL VK_DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT Message_Severity,
			VkDebugUtilsMessageTypeFlagsEXT Message_Type,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallback_Data,
			void* pUserData);


		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT();
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT();

		//Instance Extension Related
		bool isActive_SurfaceKHR = false, isActive_GetPhysicalDeviceProperties2KHR = false;
	};

#define GFXHandleConverter(ConvertType, Handle) static_cast<ConvertType>((void*)Handle)

	VK_API VkFormat Find_VkFormat_byDataType(GFX_API::DATA_TYPE datatype);
	VK_API VkFormat Find_VkFormat_byTEXTURECHANNELs(GFX_API::TEXTURE_CHANNELs channels);
	VK_API VkShaderStageFlags Find_VkShaderStages(GFX_API::SHADERSTAGEs_FLAG flag);
	VK_API VkPipelineStageFlags Find_VkPipelineStages(GFX_API::SHADERSTAGEs_FLAG flag);
	VK_API VkDescriptorType Find_VkDescType_byMATDATATYPE(GFX_API::SHADERINPUT_TYPE TYPE);
	VK_API VkSamplerAddressMode Find_AddressMode_byWRAPPING(GFX_API::TEXTURE_WRAPPING Wrapping);
	VK_API VkFilter Find_VkFilter_byGFXFilter(GFX_API::TEXTURE_MIPMAPFILTER filter);
	VK_API VkSamplerMipmapMode Find_MipmapMode_byGFXFilter(GFX_API::TEXTURE_MIPMAPFILTER filter);
	VK_API VkCullModeFlags Find_CullMode_byGFXCullMode(GFX_API::CULL_MODE mode);
	VK_API VkPolygonMode Find_PolygonMode_byGFXPolygonMode(GFX_API::POLYGON_MODE mode);
	VK_API VkPrimitiveTopology Find_PrimitiveTopology_byGFXVertexListType(GFX_API::VERTEXLIST_TYPEs type);
	VK_API VkIndexType Find_IndexType_byGFXDATATYPE(GFX_API::DATA_TYPE datatype);
	VK_API VkCompareOp Find_CompareOp_byGFXDepthTest(GFX_API::DEPTH_TESTs test);
	VK_API void Find_DepthMode_byGFXDepthMode(GFX_API::DEPTH_MODEs mode, VkBool32& ShouldTest, VkBool32& ShouldWrite);
	VK_API VkAttachmentLoadOp Find_LoadOp_byGFXLoadOp(GFX_API::DRAWPASS_LOAD load);
	VK_API VkCompareOp Find_CompareOp_byGFXStencilCompare(GFX_API::STENCIL_COMPARE op);
	VK_API VkStencilOp Find_StencilOp_byGFXStencilOp(GFX_API::STENCIL_OP op);
	VK_API VkBlendOp Find_BlendOp_byGFXBlendMode(GFX_API::BLEND_MODE mode);
	VK_API VkBlendFactor Find_BlendFactor_byGFXBlendFactor(GFX_API::BLEND_FACTOR factor);
	VK_API void Fill_ComponentMapping_byCHANNELs(GFX_API::TEXTURE_CHANNELs channels, VkComponentMapping& mapping);
	VK_API void Find_SubpassAccessPattern(GFX_API::SUBPASS_ACCESS access, bool isSource, VkPipelineStageFlags& stageflag, VkAccessFlags& accessflag);
}