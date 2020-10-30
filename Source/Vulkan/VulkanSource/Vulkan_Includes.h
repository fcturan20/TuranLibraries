#pragma once
#include "GFX/GFX_Core.h"

//Vulkan Libs
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#define VK_API
#define VULKAN_DEBUGGING

namespace Vulkan {
	struct VK_QUEUEFLAG {
		bool is_GRAPHICSsupported : 1;
		bool is_PRESENTATIONsupported : 1;
		bool is_COMPUTEsupported : 1;
		bool is_TRANSFERsupported : 1;
		//bool is_VTMEMsupported : 1;	Not supported for now!
	};

	struct VK_API VK_QUEUE {
		VK_QUEUEFLAG SupportFlag;
		VkQueue Queue;
		uint32_t QueueFamilyIndex;
		VkCommandPool CommandPool;
	};

	class VK_API GPU : public GFX_API::GPU {
	public:
		GPU();
		VkPhysicalDevice Physical_Device = {};
		VkPhysicalDeviceProperties Device_Properties = {};
		VkPhysicalDeviceFeatures Device_Features = {};
		VkPhysicalDeviceMemoryProperties MemoryProperties = {};
		vector<VkQueueFamilyProperties> QueueFamilies;
		vector<VK_QUEUE> QUEUEs;
		unsigned int TRANSFERs_supportedqueuecount = 0, COMPUTE_supportedqueuecount = 0;
		unsigned int GRAPHICS_QUEUEIndex = 0, DISPLAY_QUEUEIndex = 0;
		VkDevice Logical_Device = {};
		vector<VkExtensionProperties> Supported_DeviceExtensions;
		vector<const char*>* Required_DeviceExtensionNames;

		VkSurfaceCapabilitiesKHR SurfaceCapabilities = {};
		vector<VkSurfaceFormatKHR> SurfaceFormats;
		vector<VkPresentModeKHR> PresentationModes;
		unsigned int Find_vkMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	};

	//Window will be accessed from only Vulkan.dll, so the programmer may be aware what he can cause
	class VK_API WINDOW : public GFX_API::WINDOW {
	public:
		WINDOW(unsigned int width, unsigned int height, GFX_API::WINDOW_MODE display_mode, GFX_API::MONITOR* display_monitor, unsigned int refresh_rate, const char* window_name, GFX_API::V_SYNC v_sync);
		VkSurfaceKHR Window_Surface = {};
		VkSwapchainKHR Window_SwapChain = {};
		GLFWwindow* GLFW_WINDOW = {};
		vector<GFX_API::GFXHandle> Swapchain_Textures;
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