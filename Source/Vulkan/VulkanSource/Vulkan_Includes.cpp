#include "Vulkan_Includes.h"

namespace Vulkan {
	unsigned int GPU::Find_vkMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		for (uint32_t i = 0; i < MemoryProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		TuranAPI::LOG_ERROR("Find_vkMemoryTypeIndex() has failed! Returned 0");
		return 0;
	}

	Vulkan_States::Vulkan_States()  {

	}
	const char* const* Vulkan_States::Get_Supported_LayerNames(const VkLayerProperties* list, uint32_t length) {
		const char** NameList = new const char* [length];
		for (unsigned int i = 0; i < length; i++) {
			NameList[i] = list[i].layerName;
		}
		return NameList;
	}
	
	
	void Vulkan_States::Is_RequiredInstanceExtensions_Supported() {
		//DEFINE REQUIRED EXTENSIONS
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		for (unsigned int i = 0; i < glfwExtensionCount; i++) {
			Required_InstanceExtensionNames.push_back(glfwExtensions[i]);
			std::cout << glfwExtensions[i] << " GLFW extension is required!\n";
		}

		#ifdef VULKAN_DEBUGGING
			Required_InstanceExtensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		#endif

		//CHECK IF ALL OF THE REQUIRED EXTENSIONS SUPPORTED
		for (unsigned int i = 0; i < Required_InstanceExtensionNames.size(); i++) {
			bool Is_Found = false;
			for (unsigned int supported_extension_index = 0; supported_extension_index < Supported_InstanceExtensionList.size(); supported_extension_index++) {
				if (strcmp(Required_InstanceExtensionNames[i], Supported_InstanceExtensionList[supported_extension_index].extensionName)) {
					Is_Found = true;
					break;
				}
			}
			if (Is_Found == false) {
				TuranAPI::LOG_CRASHING("A required extension isn't supported!");
			}
		}
		TuranAPI::LOG_STATUS("All of the Vulkan Instance extensions are checked!");
	}

	void Vulkan_States::Is_RequiredDeviceExtensions_Supported(GPU* Vulkan_GPU) {
		Vulkan_GPU->Required_DeviceExtensionNames = new vector<const char*>;
		//GET SUPPORTED DEVICE EXTENSIONS
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(Vulkan_GPU->Physical_Device, nullptr, &extensionCount, nullptr);
		Vulkan_GPU->Supported_DeviceExtensions.resize(extensionCount);
		vkEnumerateDeviceExtensionProperties(Vulkan_GPU->Physical_Device, nullptr, &extensionCount, Vulkan_GPU->Supported_DeviceExtensions.data());


		//CODE REQUIRED DEVICE EXTENSIONS
		Vulkan_GPU->Required_DeviceExtensionNames->push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);


		TuranAPI::LOG_STATUS("Required Extensions for Vulkan API is set!");

		//CHECK IF ALL OF THE REQUIRED EXTENSIONS SUPPORTED
		for (unsigned int i = 0; i < Vulkan_GPU->Required_DeviceExtensionNames->size(); i++) {
			bool Is_Found = false;
			for (unsigned int supported_extension_index = 0; supported_extension_index < Vulkan_GPU->Supported_DeviceExtensions.size(); supported_extension_index++) {
				std::cout << "Checking against: " << Vulkan_GPU->Supported_DeviceExtensions[supported_extension_index].extensionName << std::endl;
				if (strcmp((*Vulkan_GPU->Required_DeviceExtensionNames)[i], Vulkan_GPU->Supported_DeviceExtensions[supported_extension_index].extensionName)) {
					Is_Found = true;
					break;
				}
			}
			if (Is_Found == false) {
				TuranAPI::LOG_CRASHING((*Vulkan_GPU->Required_DeviceExtensionNames)[i]);
			}
		}
		TuranAPI::LOG_STATUS("Checked Required Device Extensions for the GPU!");
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL Vulkan_States::VK_DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Message_Severity, VkDebugUtilsMessageTypeFlagsEXT Message_Type, const VkDebugUtilsMessengerCallbackDataEXT* pCallback_Data, void* pUserData) {
		string Callback_Type = "";
		switch (Message_Type) {
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			Callback_Type = "VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT : Some event has happened that is unrelated to the specification or performance\n";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			Callback_Type = "VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: Something has happened that violates the specification or indicates a possible mistake\n";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			Callback_Type = "VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: Potential non-optimal use of Vulkan\n";
			break;
		default:
			TuranAPI::LOG_CRASHING("Vulkan Callback has returned a unsupported Message_Type");
			return true;
			break;
		}

		switch (Message_Severity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			TuranAPI::LOG_STATUS(pCallback_Data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			TuranAPI::LOG_WARNING(pCallback_Data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			TuranAPI::LOG_STATUS(pCallback_Data->pMessage);
			break;
		default:
			TuranAPI::LOG_CRASHING("Vulkan Callback has returned a unsupported Message_Severity");
			return true;
			break;
		}

		TuranAPI::LOG_NOTCODED("Vulkan Callback has lots of data to debug such as used object's name, object's type etc\n", false);
		return false;
	}

	PFN_vkCreateDebugUtilsMessengerEXT Vulkan_States::vkCreateDebugUtilsMessengerEXT() {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Vulkan_Instance, "vkCreateDebugUtilsMessengerEXT");
		if (func == nullptr) {
			TuranAPI::LOG_ERROR("Vulkan failed to load vkCreateDebugUtilsMessengerEXT function!");
			return nullptr;
		}
		return func;
	}

	PFN_vkDestroyDebugUtilsMessengerEXT Vulkan_States::vkDestroyDebugUtilsMessengerEXT() {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Vulkan_Instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func == nullptr) {
			TuranAPI::LOG_ERROR("Vulkan failed to load vkDestroyDebugUtilsMessengerEXT function!");
			return nullptr;
		}
		return func;
	}
	

	const char* Vulkan_States::Convert_VendorID_toaString(uint32_t VendorID) {
		switch (VendorID) {
		case 0x1002:
			return "AMD";
		case 0x10DE:
			return "Nvidia";
		case 0x8086:
			return "Intel";
		case 0x13B5:
			return "ARM";
		default:
			TuranAPI::LOG_CRASHING("Vulkan_Core::Check_Computer_Specs failed to find GPU's Vendor, Vendor ID is: " + VendorID);
			return "NULL";
		}
	}
	WINDOW::WINDOW(unsigned int width, unsigned int height, GFX_API::WINDOW_MODE display_mode, GFX_API::MONITOR* display_monitor,
		unsigned int refresh_rate, const char* window_name, GFX_API::V_SYNC v_sync) {
		WIDTH = width;
		HEIGHT = height;
		DISPLAY_MODE = display_mode;
		DISPLAY_MONITOR = display_monitor;
		REFRESH_RATE = REFRESH_RATE;
		WINDOW_NAME = window_name;
		VSYNC_MODE = v_sync;
	}

	GPU::GPU(){

	}



	VkFormat Find_VkFormat_byDataType(GFX_API::DATA_TYPE datatype) {
		switch (datatype) {
		case GFX_API::DATA_TYPE::VAR_VEC2:
			return VK_FORMAT_R32G32_SFLOAT;
		case GFX_API::DATA_TYPE::VAR_VEC3:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case GFX_API::DATA_TYPE::VAR_VEC4:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		default:
			TuranAPI::LOG_ERROR("Find_VkFormat_byDataType() doesn't support this data type! UNDEFINED");
			return VK_FORMAT_UNDEFINED;
		}
	}
	VkFormat Find_VkFormat_byTEXTURECHANNELs(GFX_API::TEXTURE_CHANNELs channels) {
		switch (channels) {
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8UB:
			return VK_FORMAT_R8G8B8A8_UINT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8B:
			return VK_FORMAT_R8G8B8A8_SINT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA32F:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		default:
			TuranAPI::LOG_CRASHING("Find_VkFormat_byTEXTURECHANNELs doesn't support this type of channel!");
			return VK_FORMAT_UNDEFINED;
		}
	}
	VkAttachmentLoadOp Find_VkLoadOp_byAttachmentReadState(GFX_API::ATTACHMENT_READSTATE readstate) {
		switch (readstate)
		{
		case GFX_API::ATTACHMENT_READSTATE::CLEAR:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case GFX_API::ATTACHMENT_READSTATE::DONT_CARE:
		case GFX_API::ATTACHMENT_READSTATE::UNUSED:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		default:
			TuranAPI::LOG_CRASHING("Find_VkLoadOp_byAttachmentReadState() doesn't support this read state! Don't care is returned");
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		}
	}
	
	VkImageLayout Find_VkImageFinalLayout_byTextureType(GFX_API::TEXTURE_TYPEs type) {
		switch (type)
		{
		case GFX_API::TEXTURE_TYPEs::SWAPCHAIN_COLOR:
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		case GFX_API::TEXTURE_TYPEs::COLORRT_TEXTURE:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case GFX_API::TEXTURE_TYPEs::COLOR_TEXTURE:
		case GFX_API::TEXTURE_TYPEs::OPACITYTEXTURE:
		case GFX_API::TEXTURE_TYPEs::DEPTHSTENCIL:
		case GFX_API::TEXTURE_TYPEs::DEPTHTEXTURE:
		default:
			TuranAPI::LOG_CRASHING("Find_VkImageFinalLayout_byTextureType() doesn't support this type for now!");
			return VK_IMAGE_LAYOUT_UNDEFINED;	//This enum causes error in FinalLayout, this is why I use it for now!
		}
	}

	VkShaderStageFlags Find_VkStages(GFX_API::SHADERSTAGEs_FLAG flag) {
		VkShaderStageFlags found = 0;
		if (flag.VERTEXSHADER) {
			found = found | VK_SHADER_STAGE_VERTEX_BIT;
		}
		if (flag.FRAGMENTSHADER) {
			found = found | VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		return found;
	}
	VkDescriptorType Find_VkDescType_byVisibility(GFX_API::BUFFER_VISIBILITY BUFVIS) {
		switch (BUFVIS) {
		case GFX_API::BUFFER_VISIBILITY::CPUEXISTENCE_GPUREADWRITE:
		case GFX_API::BUFFER_VISIBILITY::CPUREADONLY_GPUREADWRITE:
		case GFX_API::BUFFER_VISIBILITY::CPUREADWRITE_GPUREADWRITE:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case GFX_API::BUFFER_VISIBILITY::CPUEXISTENCE_GPUREADONLY:
		case GFX_API::BUFFER_VISIBILITY::CPUREADWRITE_GPUREADONLY:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		default:
			TuranAPI::LOG_CRASHING("Find_VkDescType_byBUFVIS() doesn't support this type of Buffer_Visibility!");
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}
	}
	VkDescriptorType Find_VkDescType_byMATDATATYPE(GFX_API::MATERIALDATA_TYPE TYPE) {
		switch (TYPE) {
		case GFX_API::MATERIALDATA_TYPE::CONSTUBUFFER_G:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case GFX_API::MATERIALDATA_TYPE::CONSTSBUFFER_G:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case GFX_API::MATERIALDATA_TYPE::CONSTSAMPLER_G:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case GFX_API::MATERIALDATA_TYPE::CONSTIMAGE_G:
			TuranAPI::LOG_ERROR("MATERIALDATA_TYPE = CONSTIMAGE_G which I don't want to support for now, this return COMBINED_IMAGE_SAMPLER anyway!");
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case GFX_API::MATERIALDATA_TYPE::UNDEFINED:
			TuranAPI::LOG_CRASHING("Find_VkDescType_byMATDATATYPE() has failed because MATERIALDATA_TYPE = UNDEFINED!");
		default:
			TuranAPI::LOG_CRASHING("Find_VkDescType_byMATDATATYPE() doesn't support this type of MATERIALDATA_TYPE!");
		}
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}