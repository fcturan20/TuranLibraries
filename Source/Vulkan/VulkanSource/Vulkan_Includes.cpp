#include "Vulkan_Includes.h"
#define VKGPU ((Vulkan::GPU*)GFX->GPU_TO_RENDER)

namespace Vulkan {
	VK_QUEUEFLAG::VK_QUEUEFLAG() {
		is_GRAPHICSsupported = false;
		is_COMPUTEsupported = false;
		is_PRESENTATIONsupported = false;
		is_TRANSFERsupported = false;
	}
	VK_QUEUE* GPU::Find_BestQueue(const VK_QUEUEFLAG& Flag) {
		if (!Flag.is_COMPUTEsupported && !Flag.is_GRAPHICSsupported && !Flag.is_TRANSFERsupported && !Flag.is_PRESENTATIONsupported) {
			return nullptr;
		}
		unsigned char BestScore = 0;
		VK_QUEUE* BestQueue = nullptr;
		for (unsigned char QueueIndex = 0; QueueIndex < QUEUEs.size(); QueueIndex++) {
			VK_QUEUE* Queue = &QUEUEs[QueueIndex];
			if (DoesQueue_Support(Queue, Flag)) {
				if (!BestScore || BestScore > Queue->QueueFeatureScore) {
					BestScore = Queue->QueueFeatureScore;
					BestQueue = Queue;
				}
			}
		}
		return BestQueue;
	}
	bool GPU::DoesQueue_Support(const VK_QUEUE* QUEUE, const VK_QUEUEFLAG& Flag) {
		const VK_QUEUEFLAG& supportflag = QUEUE->SupportFlag;
		if (Flag.is_COMPUTEsupported && !supportflag.is_COMPUTEsupported) {
			return false;
		}
		if (Flag.is_GRAPHICSsupported && !supportflag.is_GRAPHICSsupported) {
			return false;
		}
		if (Flag.is_TRANSFERsupported && !supportflag.is_TRANSFERsupported) {
			return false;
		}
		if (Flag.is_PRESENTATIONsupported && !supportflag.is_PRESENTATIONsupported) {
			return false;
		}
		return true;
	}
	unsigned int GPU::Find_vkMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		for (uint32_t i = 0; i < MemoryProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		LOG_CRASHING_TAPI("Find_vkMemoryTypeIndex() has failed! Returned 0");
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
				LOG_CRASHING_TAPI("A required extension isn't supported!");
			}
		}
		LOG_STATUS_TAPI("All of the Vulkan Instance extensions are checked!");
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


		LOG_STATUS_TAPI("Required Extensions for Vulkan API is set!");

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
				LOG_CRASHING_TAPI((*Vulkan_GPU->Required_DeviceExtensionNames)[i]);
			}
		}
		LOG_STATUS_TAPI("Checked Required Device Extensions for the GPU!");
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
			LOG_CRASHING_TAPI("Vulkan Callback has returned a unsupported Message_Type");
			return true;
			break;
		}

		switch (Message_Severity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			LOG_STATUS_TAPI(pCallback_Data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			LOG_WARNING_TAPI(pCallback_Data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			LOG_STATUS_TAPI(pCallback_Data->pMessage);
			break;
		default:
			LOG_CRASHING_TAPI("Vulkan Callback has returned a unsupported Message_Severity");
			return true;
			break;
		}

		LOG_NOTCODED_TAPI("Vulkan Callback has lots of data to debug such as used object's name, object's type etc\n", false);
		return false;
	}

	PFN_vkCreateDebugUtilsMessengerEXT Vulkan_States::vkCreateDebugUtilsMessengerEXT() {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Vulkan_Instance, "vkCreateDebugUtilsMessengerEXT");
		if (func == nullptr) {
			LOG_ERROR_TAPI("Vulkan failed to load vkCreateDebugUtilsMessengerEXT function!");
			return nullptr;
		}
		return func;
	}

	PFN_vkDestroyDebugUtilsMessengerEXT Vulkan_States::vkDestroyDebugUtilsMessengerEXT() {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Vulkan_Instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func == nullptr) {
			LOG_ERROR_TAPI("Vulkan failed to load vkDestroyDebugUtilsMessengerEXT function!");
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
			LOG_CRASHING_TAPI("Vulkan_Core::Check_Computer_Specs failed to find GPU's Vendor, Vendor ID is: " + VendorID);
			return "NULL";
		}
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
			LOG_ERROR_TAPI("Find_VkFormat_byDataType() doesn't support this data type! UNDEFINED");
			return VK_FORMAT_UNDEFINED;
		}
	}
	VkFormat Find_VkFormat_byTEXTURECHANNELs(GFX_API::TEXTURE_CHANNELs channels) {
		switch (channels) {
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8UB:
			return VK_FORMAT_B8G8R8A8_SRGB;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8B:
			return VK_FORMAT_R8G8B8A8_SINT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA32F:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32:
			return VK_FORMAT_D32_SFLOAT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		default:
			LOG_CRASHING_TAPI("Find_VkFormat_byTEXTURECHANNELs doesn't support this type of channel!");
			return VK_FORMAT_UNDEFINED;
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
			LOG_CRASHING_TAPI("Find_VkDescType_byBUFVIS() doesn't support this type of Buffer_Visibility!");
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}
	}
	VkDescriptorType Find_VkDescType_byMATDATATYPE(GFX_API::MATERIALDATA_TYPE TYPE) {
		switch (TYPE) {
		case GFX_API::MATERIALDATA_TYPE::CONSTUBUFFER_G:
		case GFX_API::MATERIALDATA_TYPE::CONSTUBUFFER_PI:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case GFX_API::MATERIALDATA_TYPE::CONSTSBUFFER_G:
		case GFX_API::MATERIALDATA_TYPE::CONSTSBUFFER_PI:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case GFX_API::MATERIALDATA_TYPE::CONSTSAMPLER_G:
		case GFX_API::MATERIALDATA_TYPE::CONSTSAMPLER_PI:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case GFX_API::MATERIALDATA_TYPE::CONSTIMAGE_G:
		case GFX_API::MATERIALDATA_TYPE::CONSTIMAGE_PI:
			LOG_ERROR_TAPI("MATERIALDATA_TYPE = CONSTIMAGE_G which I don't want to support for now, this return COMBINED_IMAGE_SAMPLER anyway!");
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case GFX_API::MATERIALDATA_TYPE::UNDEFINED:
			LOG_CRASHING_TAPI("Find_VkDescType_byMATDATATYPE() has failed because MATERIALDATA_TYPE = UNDEFINED!");
		default:
			LOG_CRASHING_TAPI("Find_VkDescType_byMATDATATYPE() doesn't support this type of MATERIALDATA_TYPE!");
		}
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}


}