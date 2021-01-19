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

	struct VK_CommandPool {
		std::mutex Sync;
		vector<VkCommandBuffer> CBs;
		VkCommandPool CPHandle;
		VK_CommandPool();
		VK_CommandPool(const VK_CommandPool& RefCP);
		void operator= (const VK_CommandPool& RefCP);
	};

	struct VK_API VK_QUEUE {
		VK_QUEUEFLAG SupportFlag;
		VkQueue Queue;
		uint32_t QueueFamilyIndex;
		VK_CommandPool CommandPool;
		VkFence RenderGraphFences[2];
		vector<GFX_API::GFXHandle> ActiveSubmits;
		unsigned char QueueFeatureScore = 0;
		VK_QUEUE();
	};

	struct VK_API GPU {
		VkPhysicalDevice Physical_Device = {};
		VkPhysicalDeviceProperties Device_Properties = {};
		VkPhysicalDeviceFeatures Device_Features = {};
		VkPhysicalDeviceMemoryProperties MemoryProperties = {};
		vector<VK_QUEUE> QUEUEs;
		unsigned int TRANSFERs_supportedqueuecount = 0, COMPUTE_supportedqueuecount = 0;
		unsigned int GRAPHICS_QUEUEIndex = 0;
		VkDevice Logical_Device = {};
		vector<VkExtensionProperties> Supported_DeviceExtensions;
		vector<const char*>* Required_DeviceExtensionNames;

		unsigned int Find_vkMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties);
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
		unsigned char PresentationWaitSemaphoreIndexes[2];
		vector<GFX_API::GFXHandle> Swapchain_Textures;
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
		vector<const char*> Required_InstanceExtensionNames;
		GPU* GPU_TO_RENDER;

		VkLayerProperties* Supported_LayerList;
		uint32_t Supported_LayerNumber;
		VkDebugUtilsMessengerEXT Debug_Messenger;

		//CONVERT VENDOR ID TO VENDOR NAME
		const char* Convert_VendorID_toaString(uint32_t VendorID);

		//Required Extensions are defined here
		//Check if Required Extension Names are supported!
		//Returns true if all of the required extensions are supported!
		void Is_RequiredInstanceExtensions_Supported();
		//Same for Device
		void Is_RequiredDeviceExtensions_Supported(GPU* Vulkan_GPU);

		static VKAPI_ATTR VkBool32 VKAPI_CALL VK_DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT Message_Severity,
			VkDebugUtilsMessageTypeFlagsEXT Message_Type,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallback_Data,
			void* pUserData);


		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT();
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT();
	};

#define GFXHandleConverter(ConvertType, Handle) static_cast<ConvertType>((void*)Handle)

	VK_API VkFormat Find_VkFormat_byDataType(GFX_API::DATA_TYPE datatype);
	VK_API VkFormat Find_VkFormat_byTEXTURECHANNELs(GFX_API::TEXTURE_CHANNELs channels);
	VK_API VkAttachmentLoadOp Find_VkLoadOp_byAttachmentReadState(GFX_API::DRAWPASS_LOAD readstate);
	VK_API VkShaderStageFlags Find_VkStages(GFX_API::SHADERSTAGEs_FLAG flag);
	VK_API VkDescriptorType Find_VkDescType_byVisibility(GFX_API::BUFFER_VISIBILITY BUFVIS);
	VK_API VkDescriptorType Find_VkDescType_byMATDATATYPE(GFX_API::MATERIALDATA_TYPE TYPE);

	enum VK_API class SUBALLOCATEBUFFERTYPEs : unsigned char {
		NOWHERE,
		STAGING,
		GPULOCALBUF,
		GPULOCALTEX
	};
}