#include "Vulkan_Includes.h"
#define VK_NO_PROTOTYPES 1
#include "Renderer/Vulkan_Resource.h"
#include <numeric>
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

	VK_CommandPool::VK_CommandPool() {

	}
	VK_CommandPool::VK_CommandPool(const VK_CommandPool& RefCP) {
		CPHandle = RefCP.CPHandle;
		CBs = RefCP.CBs;
	}
	void VK_CommandPool::operator=(const VK_CommandPool& RefCP) {
		CPHandle = RefCP.CPHandle;
		CBs = RefCP.CBs;
	}
	VK_QUEUE::VK_QUEUE() {

	}
	VK_SubAllocation::VK_SubAllocation() {

	}
	VK_SubAllocation::VK_SubAllocation(const VK_SubAllocation& copyblock) {
		isEmpty.store(copyblock.isEmpty.load());
		Size = copyblock.Size;
		Offset = copyblock.Offset;
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


	void DeviceExtensions::Fill_DepthAttachmentReference(VkAttachmentReference& Ref, unsigned int index, GFX_API::TEXTURE_CHANNELs channels, GFX_API::OPERATION_TYPE DEPTHOPTYPE, GFX_API::OPERATION_TYPE STENCILOPTYPE) {
		Ref.attachment = index;
		if (SeperatedDepthStencilLayouts) {
			if (channels == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32) {
				switch (DEPTHOPTYPE) {
				case GFX_API::OPERATION_TYPE::READ_ONLY:
					Ref.layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
				case GFX_API::OPERATION_TYPE::READ_AND_WRITE:
				case GFX_API::OPERATION_TYPE::WRITE_ONLY:
					Ref.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
					break;
				case GFX_API::OPERATION_TYPE::UNUSED:
					Ref.attachment = VK_ATTACHMENT_UNUSED;
					Ref.layout = VK_IMAGE_LAYOUT_UNDEFINED;
					break;
				default:
					LOG_NOTCODED_TAPI("VK::Fill_SubpassStructs() doesn't support this type of Operation Type for DepthBuffer!", true);
				}
			}
			else if (channels == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8) {
				switch (STENCILOPTYPE) {
				case GFX_API::OPERATION_TYPE::READ_ONLY:
					if (DEPTHOPTYPE == GFX_API::OPERATION_TYPE::UNUSED || DEPTHOPTYPE == GFX_API::OPERATION_TYPE::READ_ONLY) {
						Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
					}
					else if (DEPTHOPTYPE == GFX_API::OPERATION_TYPE::READ_AND_WRITE || DEPTHOPTYPE == GFX_API::OPERATION_TYPE::WRITE_ONLY) {
						Ref.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
					}
					break;
				case GFX_API::OPERATION_TYPE::READ_AND_WRITE:
				case GFX_API::OPERATION_TYPE::WRITE_ONLY:
					if (DEPTHOPTYPE == GFX_API::OPERATION_TYPE::UNUSED || DEPTHOPTYPE == GFX_API::OPERATION_TYPE::READ_ONLY) {
						Ref.layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
					}
					else if (DEPTHOPTYPE == GFX_API::OPERATION_TYPE::READ_AND_WRITE || DEPTHOPTYPE == GFX_API::OPERATION_TYPE::WRITE_ONLY) {
						Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					}
					break;
				case GFX_API::OPERATION_TYPE::UNUSED:
					if (DEPTHOPTYPE == GFX_API::OPERATION_TYPE::UNUSED) {
						Ref.attachment = VK_ATTACHMENT_UNUSED;
						Ref.layout = VK_IMAGE_LAYOUT_UNDEFINED;
					}
					else if (DEPTHOPTYPE == GFX_API::OPERATION_TYPE::READ_AND_WRITE || DEPTHOPTYPE == GFX_API::OPERATION_TYPE::WRITE_ONLY) {
						Ref.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
					}
					else if (DEPTHOPTYPE == GFX_API::OPERATION_TYPE::READ_ONLY) {
						Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
					}
					break;
				default:
					LOG_NOTCODED_TAPI("VK::Fill_SubpassStructs() doesn't support this type of Operation Type for DepthSTENCILBuffer!", true);
				}
			}
		}
		else {
			if (DEPTHOPTYPE == GFX_API::OPERATION_TYPE::UNUSED) {
				Ref.attachment = VK_ATTACHMENT_UNUSED;
				Ref.layout = VK_IMAGE_LAYOUT_UNDEFINED;
			}
			else {
				Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
		}
	}
	void DeviceExtensions::Fill_DepthAttachmentDescription(VkAttachmentDescription& Desc, VK_DEPTHSTENCILSLOT* DepthSlot) {
		Desc = {};
		Desc.format = Find_VkFormat_byTEXTURECHANNELs(DepthSlot->RT->CHANNELs);
		Desc.samples = VK_SAMPLE_COUNT_1_BIT;
		Desc.flags = 0;
		Desc.loadOp = Find_LoadOp_byGFXLoadOp(DepthSlot->DEPTH_LOAD);
		Desc.stencilLoadOp = Find_LoadOp_byGFXLoadOp(DepthSlot->STENCIL_LOAD);
		if (DepthSlot->IS_USED_LATER) {
			Desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		}
		else {
			Desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}

		if (SeperatedDepthStencilLayouts) {
			if (DepthSlot->RT->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32) {
				if (DepthSlot->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::READ_ONLY) {
					Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
					Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
				}
				else {
					Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
					Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				}
			}
			else if (DepthSlot->RT->CHANNELs == GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8) {
				if (DepthSlot->DEPTH_OPTYPE == GFX_API::OPERATION_TYPE::READ_ONLY) {
					if (DepthSlot->STENCIL_OPTYPE != GFX_API::OPERATION_TYPE::READ_ONLY &&
						DepthSlot->STENCIL_OPTYPE != GFX_API::OPERATION_TYPE::UNUSED) {
						Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
						Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
					}
					else {
						Desc.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
						Desc.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
					}
				}
				else {
					if (DepthSlot->STENCIL_OPTYPE != GFX_API::OPERATION_TYPE::READ_ONLY &&
						DepthSlot->STENCIL_OPTYPE != GFX_API::OPERATION_TYPE::UNUSED) {
						Desc.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
						Desc.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					}
					else {
						Desc.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
						Desc.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
					}
				}
			}
		}
		else {
			Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
	}

	
	bool IsExtensionSupported(const char* ExtensionName, vector<VkExtensionProperties> SupportedExtensions) {
		bool Is_Found = false;
		for (unsigned int supported_extension_index = 0; supported_extension_index < SupportedExtensions.size(); supported_extension_index++) {
			if (strcmp(ExtensionName, SupportedExtensions[supported_extension_index].extensionName)) {
				return true;
			}
		}
		LOG_WARNING_TAPI("Extension: " + std::string(ExtensionName) + " is not supported by the GPU!");
		return false;
	}
	void Vulkan_States::Check_InstanceExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		for (unsigned int i = 0; i < glfwExtensionCount; i++) {
			if (!IsExtensionSupported(glfwExtensions[i], Supported_InstanceExtensionList)) {
				LOG_NOTCODED_TAPI("Your vulkan instance doesn't support extensions that're required by GLFW. This situation is not tested, so report your device to the author!", true);
				return;
			}
			Active_InstanceExtensionNames.push_back(glfwExtensions[i]);
		}
		

		if (IsExtensionSupported(VK_KHR_SURFACE_EXTENSION_NAME, Supported_InstanceExtensionList)) {
			isActive_SurfaceKHR = true;
			Active_InstanceExtensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		}
		else {
			LOG_WARNING_TAPI("Your Vulkan instance doesn't support to display a window, so you shouldn't use any window related functionality such as: GFXRENDERER->Create_WindowPass, GFX->Create_Window, GFXRENDERER->Swap_Buffers ...");
		}
		

		#ifdef VULKAN_DEBUGGING
		if (IsExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, Supported_InstanceExtensionList)) {
			Active_InstanceExtensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		#endif


		//Check isActive_GetPhysicalDeviceProperties2KHR
		if ((Application_Info.apiVersion == VK_API_VERSION_1_0 && 
			IsExtensionSupported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, Supported_InstanceExtensionList)
			) || Application_Info.apiVersion != VK_API_VERSION_1_0){
			isSupported_PhysicalDeviceProperties2 = true;
			if (Application_Info.apiVersion == VK_API_VERSION_1_0) {
				Active_InstanceExtensionNames.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
			}
		}
	}

	//Some features needs device extensions, so check if they're supported here
	//After finding supported device features, activate them in Activate_DeviceFeatures
	void Vulkan_States::Activate_DeviceExtensions(GPU* Vulkan_GPU) {
		//GET SUPPORTED DEVICE EXTENSIONS
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(Vulkan_GPU->Physical_Device, nullptr, &extensionCount, nullptr);
		Vulkan_GPU->Supported_DeviceExtensions.resize(extensionCount);
		vkEnumerateDeviceExtensionProperties(Vulkan_GPU->Physical_Device, nullptr, &extensionCount, Vulkan_GPU->Supported_DeviceExtensions.data());

		//Check Seperate_DepthStencil
		if (Application_Info.apiVersion == VK_API_VERSION_1_0 || Application_Info.apiVersion == VK_API_VERSION_1_1) {
			if(IsExtensionSupported(VK_KHR_SEPARATE_DEPTH_STENCIL_LAYOUTS_EXTENSION_NAME, Vulkan_GPU->Supported_DeviceExtensions)){
				Vulkan_GPU->ExtensionRelatedDatas.SeperatedDepthStencilLayouts = true;
				Vulkan_GPU->Active_DeviceExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
				if (Application_Info.apiVersion == VK_API_VERSION_1_0) {
					Vulkan_GPU->Active_DeviceExtensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);
					Vulkan_GPU->Active_DeviceExtensions.push_back(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
				}
			}
		}
		else {
			Vulkan_GPU->ExtensionRelatedDatas.SeperatedDepthStencilLayouts = true;
		}

		if (!IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, Vulkan_GPU->Supported_DeviceExtensions)) {
			LOG_WARNING_TAPI("Current GPU doesn't support to display a swapchain, so you shouldn't use any window related functionality such as: GFXRENDERER->Create_WindowPass, GFX->Create_Window, GFXRENDERER->Swap_Buffers ...");
			Vulkan_GPU->ExtensionRelatedDatas.SwapchainDisplay = false;
		}
		else {
			Vulkan_GPU->Active_DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}


		//Check Descriptor Indexing
		//First check Maintenance3 and PhysicalDeviceProperties2 for Vulkan 1.0
		if (Application_Info.apiVersion == VK_API_VERSION_1_0) {
			if (IsExtensionSupported(VK_KHR_MAINTENANCE3_EXTENSION_NAME, Vulkan_GPU->Supported_DeviceExtensions) && isSupported_PhysicalDeviceProperties2) {
				Vulkan_GPU->Active_DeviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
				if (IsExtensionSupported(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, Vulkan_GPU->Supported_DeviceExtensions)) {
					Vulkan_GPU->Active_DeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
					Vulkan_GPU->ExtensionRelatedDatas.DescriptorIndexing = true;
				}
			}
		}
		//Maintenance3 and PhysicalDeviceProperties2 is core in 1.1, so check only Descriptor Indexing
		else {
			//If API is 1.1, check extension
			if (Application_Info.apiVersion == VK_API_VERSION_1_1) {
				if (IsExtensionSupported(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, Vulkan_GPU->Supported_DeviceExtensions)) {
					Vulkan_GPU->Active_DeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
					Vulkan_GPU->ExtensionRelatedDatas.DescriptorIndexing = true;
				}
			}
			else {
				Vulkan_GPU->ExtensionRelatedDatas.DescriptorIndexing = true;
			}
		}



	}
	void Vulkan_States::Activate_DeviceFeatures(GPU* Vulkan_GPU, GFX_API::GPUDescription& GPUDesc, VkDeviceCreateInfo& dev_ci, DeviceExtendedFeatures& dev_ftrs) {
		const void*& dev_ci_Next = dev_ci.pNext;
		void** extending_Next = nullptr;


		//Check for seperate RTSlot blending
		if (Vulkan_GPU->Supported_Features.independentBlend) {
			Vulkan_GPU->Active_Features.independentBlend = VK_TRUE;
			GPUDesc.isSupported_SeperateRTSlotBlending = true;
		}

		//Activate Descriptor Indexing Features
		if(Vulkan_GPU->ExtensionRelatedDatas.DescriptorIndexing){
			dev_ftrs.DescIndexingFeatures.descriptorBindingPartiallyBound = true;
			dev_ftrs.DescIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
			dev_ftrs.DescIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
			dev_ftrs.DescIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
			dev_ftrs.DescIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
			dev_ftrs.DescIndexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
			dev_ftrs.DescIndexingFeatures.runtimeDescriptorArray = VK_FALSE;
			dev_ftrs.DescIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
			dev_ftrs.DescIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
			dev_ftrs.DescIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
			dev_ftrs.DescIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
			dev_ftrs.DescIndexingFeatures.pNext = nullptr;
			dev_ftrs.DescIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

			if (extending_Next) {
				*extending_Next = &dev_ftrs.DescIndexingFeatures;
				extending_Next = &dev_ftrs.DescIndexingFeatures.pNext;
			}
			else {
				dev_ci_Next = &dev_ftrs.DescIndexingFeatures;
				extending_Next = &dev_ftrs.DescIndexingFeatures.pNext;
			}
		}
	}
	//It is safe to use logical device here, because it is already created
	void Vulkan_States::Check_DeviceLimits(GPU* Vulkan_GPU, GFX_API::GPUDescription& GPUDesc) {
		//Check Descriptor Limits
		{
			if (Vulkan_GPU->ExtensionRelatedDatas.DescriptorIndexing) {
				VkPhysicalDeviceDescriptorIndexingProperties* descindexinglimits = new VkPhysicalDeviceDescriptorIndexingProperties{};
				descindexinglimits->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;
				VkPhysicalDeviceProperties2 devprops2;
				devprops2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
				devprops2.pNext = descindexinglimits;
				vkGetPhysicalDeviceProperties2(Vulkan_GPU->Physical_Device, &devprops2);

				GPUDesc.MaxImageTexture_perStage = descindexinglimits->maxPerStageDescriptorUpdateAfterBindStorageImages;
				GPUDesc.MaxSampledTexture_perStage = descindexinglimits->maxPerStageDescriptorUpdateAfterBindSampledImages;
				GPUDesc.MaxUniformBuffer_perStage = descindexinglimits->maxPerStageDescriptorUpdateAfterBindUniformBuffers;
				GPUDesc.MaxStorageBuffer_perStage = descindexinglimits->maxPerStageDescriptorUpdateAfterBindStorageBuffers;
				GPUDesc.MaxUsableResources_perStage = descindexinglimits->maxPerStageUpdateAfterBindResources;

				GPUDesc.MaxShaderInput_SampledTexture = descindexinglimits->maxDescriptorSetUpdateAfterBindSampledImages;
				GPUDesc.MaxShaderInput_ImageTexture = descindexinglimits->maxDescriptorSetUpdateAfterBindStorageImages;
				GPUDesc.MaxShaderInput_UniformBuffer = descindexinglimits->maxDescriptorSetUpdateAfterBindUniformBuffers;
				GPUDesc.MaxShaderInput_StorageBuffer = descindexinglimits->maxDescriptorSetUpdateAfterBindStorageBuffers;
				delete descindexinglimits;
			}
			else {
				GPUDesc.MaxImageTexture_perStage = Vulkan_GPU->Device_Properties.limits.maxPerStageDescriptorStorageImages;
				GPUDesc.MaxSampledTexture_perStage = Vulkan_GPU->Device_Properties.limits.maxPerStageDescriptorSampledImages;
				GPUDesc.MaxUniformBuffer_perStage = Vulkan_GPU->Device_Properties.limits.maxPerStageDescriptorUniformBuffers;
				GPUDesc.MaxStorageBuffer_perStage = Vulkan_GPU->Device_Properties.limits.maxPerStageDescriptorStorageBuffers;
				GPUDesc.MaxUsableResources_perStage = Vulkan_GPU->Device_Properties.limits.maxPerStageResources;

				GPUDesc.MaxShaderInput_SampledTexture = Vulkan_GPU->Device_Properties.limits.maxDescriptorSetSampledImages;
				GPUDesc.MaxShaderInput_ImageTexture = Vulkan_GPU->Device_Properties.limits.maxDescriptorSetStorageImages;
				GPUDesc.MaxShaderInput_UniformBuffer = Vulkan_GPU->Device_Properties.limits.maxDescriptorSetUniformBuffers;
				GPUDesc.MaxShaderInput_StorageBuffer = Vulkan_GPU->Device_Properties.limits.maxDescriptorSetStorageBuffers;
			}
		}
	}

	VK_MemoryAllocation::VK_MemoryAllocation() : Allocated_Blocks(*GFX->JobSys) {

	}
	VK_MemoryAllocation::VK_MemoryAllocation(const VK_MemoryAllocation& copyALLOC) : UnusedSize(copyALLOC.UnusedSize.DirectLoad()),
		Allocated_Blocks(*GFX->JobSys), Buffer(copyALLOC.Buffer), Allocated_Memory(copyALLOC.Allocated_Memory), TYPE(copyALLOC.TYPE),
		MappedMemory(copyALLOC.MappedMemory), MemoryTypeIndex(copyALLOC.MemoryTypeIndex), FullSize(copyALLOC.FullSize) {

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


	void Fill_DepthAttachmentReference(VkAttachmentReference& Ref, unsigned int index, VK_DEPTHSTENCILSLOT* DepthSlot) {

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
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_BGRA8UNORM:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_BGRA8UB:
			return VK_FORMAT_B8G8R8A8_UINT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8UB:
			return VK_FORMAT_R8G8B8A8_UINT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8B:
			return VK_FORMAT_R8G8B8A8_SINT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA32F:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGB8UB:
			return VK_FORMAT_R8G8B8_UINT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32:
			return VK_FORMAT_D32_SFLOAT;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		default:
			LOG_CRASHING_TAPI("Find_VkFormat_byTEXTURECHANNELs doesn't support this type of channel!");
			return VK_FORMAT_UNDEFINED;
		}
	}
	VkShaderStageFlags Find_VkShaderStages(GFX_API::SHADERSTAGEs_FLAG flag) {
		VkShaderStageFlags found = 0;
		if (flag.COLORRTOUTPUT && flag.FRAGMENTSHADER && flag.VERTEXSHADER) {
			found |= VK_SHADER_STAGE_ALL_GRAPHICS;
		}
		if (flag.VERTEXSHADER) {
			found = found | VK_SHADER_STAGE_VERTEX_BIT;
		}
		if (flag.FRAGMENTSHADER) {
			found = found | VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		if (flag.SWAPCHAINDISPLAY) {
			found = 0;
		}
		if (flag.TRANSFERCMD) {
			found = 0;
		}
		return found;
	}
	VkPipelineStageFlags Find_VkPipelineStages(GFX_API::SHADERSTAGEs_FLAG flag) {
		if (flag.COLORRTOUTPUT && flag.FRAGMENTSHADER && flag.VERTEXSHADER) {
			return VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
		}
		if (flag.SWAPCHAINDISPLAY) {
			return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		}
		VkPipelineStageFlags f = 0;
		if (flag.COLORRTOUTPUT) {
			f |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		if (flag.FRAGMENTSHADER) {
			f |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		if (flag.VERTEXSHADER) {
			f |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		}
		if (flag.TRANSFERCMD) {
			f |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		return f;
	}
	VkDescriptorType Find_VkDescType_byMATDATATYPE(GFX_API::SHADERINPUT_TYPE TYPE) {
		switch (TYPE) {
		case GFX_API::SHADERINPUT_TYPE::UBUFFER_G:
		case GFX_API::SHADERINPUT_TYPE::UBUFFER_PI:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case GFX_API::SHADERINPUT_TYPE::SBUFFER_G:
		case GFX_API::SHADERINPUT_TYPE::SBUFFER_PI:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case GFX_API::SHADERINPUT_TYPE::SAMPLER_G:
		case GFX_API::SHADERINPUT_TYPE::SAMPLER_PI:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case GFX_API::SHADERINPUT_TYPE::IMAGE_G:
		case GFX_API::SHADERINPUT_TYPE::IMAGE_PI:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case GFX_API::SHADERINPUT_TYPE::UNDEFINED:
			LOG_CRASHING_TAPI("Find_VkDescType_byMATDATATYPE() has failed because SHADERINPUT_TYPE = UNDEFINED!");
		default:
			LOG_CRASHING_TAPI("Find_VkDescType_byMATDATATYPE() doesn't support this type of SHADERINPUT_TYPE!");
		}
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}

	VkSamplerAddressMode Find_AddressMode_byWRAPPING(GFX_API::TEXTURE_WRAPPING Wrapping) {
		switch(Wrapping) {
		case GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case GFX_API::TEXTURE_WRAPPING::API_TEXTURE_MIRRORED_REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case GFX_API::TEXTURE_WRAPPING::API_TEXTURE_CLAMP_TO_EDGE:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		default:
			LOG_NOTCODED_TAPI("Find_AddressMode_byWRAPPING() doesn't support this wrapping type!", true);
			return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
		}
	}

	VkFilter Find_VkFilter_byGFXFilter(GFX_API::TEXTURE_MIPMAPFILTER filter) {
		switch (filter) {
		case GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_LINEAR_FROM_1MIP:
		case GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_LINEAR_FROM_2MIP:
			return VK_FILTER_LINEAR;
		case GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_NEAREST_FROM_1MIP:
		case GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_NEAREST_FROM_2MIP:
			return VK_FILTER_NEAREST;
		default:
			LOG_NOTCODED_TAPI("Find_VkFilter_byGFXFilter() doesn't support this filter type!", true);
			return VK_FILTER_MAX_ENUM;
		}
	}
	VkSamplerMipmapMode Find_MipmapMode_byGFXFilter(GFX_API::TEXTURE_MIPMAPFILTER filter) {
		switch (filter) {
		case GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_LINEAR_FROM_2MIP:
		case GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_NEAREST_FROM_2MIP:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		case GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_LINEAR_FROM_1MIP:
		case GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_NEAREST_FROM_1MIP:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		}
	}

	VkCullModeFlags Find_CullMode_byGFXCullMode(GFX_API::CULL_MODE mode) {
		switch (mode)
		{
		case GFX_API::CULL_MODE::CULL_OFF:
			return VK_CULL_MODE_NONE;
			break;
		case GFX_API::CULL_MODE::CULL_BACK:
			return VK_CULL_MODE_BACK_BIT;
			break;
		case GFX_API::CULL_MODE::CULL_FRONT:
			return VK_CULL_MODE_FRONT_BIT;
			break;
		default:
			LOG_NOTCODED_TAPI("This culling type isn't supported by Find_CullMode_byGFXCullMode()!", true);
			return VK_CULL_MODE_NONE;
			break;
		}
	}
	VkPolygonMode Find_PolygonMode_byGFXPolygonMode(GFX_API::POLYGON_MODE mode) {
		switch (mode)
		{
		case GFX_API::POLYGON_MODE::FILL:
			return VK_POLYGON_MODE_FILL;
			break;
		case GFX_API::POLYGON_MODE::LINE:
			return VK_POLYGON_MODE_LINE;
			break;
		case GFX_API::POLYGON_MODE::POINT:
			return VK_POLYGON_MODE_POINT;
			break;
		default:
			LOG_NOTCODED_TAPI("This polygon mode isn't support by Find_PolygonMode_byGFXPolygonMode()", true);
			break;
		}
	}
	VkPrimitiveTopology Find_PrimitiveTopology_byGFXVertexListType(GFX_API::VERTEXLIST_TYPEs vertexlist) {
		switch (vertexlist)
		{
		case GFX_API::VERTEXLIST_TYPEs::TRIANGLELIST:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		default:
			LOG_NOTCODED_TAPI("This type of vertex list is not supported by Find_PrimitiveTopology_byGFXVertexListType()", true);
			return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
			break;
		}
	}
	VkIndexType Find_IndexType_byGFXDATATYPE(GFX_API::DATA_TYPE datatype) {
		switch (datatype)
		{
		case GFX_API::DATA_TYPE::VAR_UINT32:
			return VK_INDEX_TYPE_UINT32;
		case GFX_API::DATA_TYPE::VAR_UINT16:
			return VK_INDEX_TYPE_UINT16;
		default:
			LOG_NOTCODED_TAPI("This type of data isn't supported by Find_IndexType_byGFXDATATYPE()", true);
			return VK_INDEX_TYPE_MAX_ENUM;
		}
	}
	VkCompareOp Find_CompareOp_byGFXDepthTest(GFX_API::DEPTH_TESTs test) {
		switch (test) {
		case GFX_API::DEPTH_TESTs::DEPTH_TEST_NEVER:
			return VK_COMPARE_OP_NEVER;
		case GFX_API::DEPTH_TESTs::DEPTH_TEST_ALWAYS:
			return VK_COMPARE_OP_ALWAYS;
		case GFX_API::DEPTH_TESTs::DEPTH_TEST_GEQUAL:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case GFX_API::DEPTH_TESTs::DEPTH_TEST_GREATER:
			return VK_COMPARE_OP_GREATER;
		case GFX_API::DEPTH_TESTs::DEPTH_TEST_LEQUAL:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
		case GFX_API::DEPTH_TESTs::DEPTH_TEST_LESS:
			return VK_COMPARE_OP_LESS;
		default:
			LOG_NOTCODED_TAPI("Find_CompareOp_byGFXDepthTest() doesn't support this type of test!", true);
			return VK_COMPARE_OP_MAX_ENUM;
		}
	}

	void Find_DepthMode_byGFXDepthMode(GFX_API::DEPTH_MODEs mode, VkBool32& ShouldTest, VkBool32& ShouldWrite) {
		switch (mode)
		{
		case GFX_API::DEPTH_MODEs::DEPTH_READ_WRITE:
			ShouldTest = VK_TRUE;
			ShouldWrite = VK_TRUE;
			break;
		case GFX_API::DEPTH_MODEs::DEPTH_READ_ONLY:
			ShouldTest = VK_TRUE;
			ShouldWrite = VK_FALSE;
			break;
		case GFX_API::DEPTH_MODEs::DEPTH_OFF:
			ShouldTest = VK_FALSE;
			ShouldWrite = VK_FALSE;
			break;
		default:
			LOG_NOTCODED_TAPI("Find_DepthMode_byGFXDepthMode() doesn't support this type of depth mode!", true);
			break;
		}
	}

	VkAttachmentLoadOp Find_LoadOp_byGFXLoadOp(GFX_API::DRAWPASS_LOAD load) {
		switch (load) {
		case GFX_API::DRAWPASS_LOAD::CLEAR:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case GFX_API::DRAWPASS_LOAD::FULL_OVERWRITE:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		case GFX_API::DRAWPASS_LOAD::LOAD:
			return VK_ATTACHMENT_LOAD_OP_LOAD;
		default:
			LOG_NOTCODED_TAPI("Find_LoadOp_byGFXLoadOp() doesn't support this type of load!", true);
			return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
		}
	}

	VkCompareOp Find_CompareOp_byGFXStencilCompare(GFX_API::STENCIL_COMPARE op) {
		switch (op)
		{
		case GFX_API::STENCIL_COMPARE::NEVER_PASS:
			return VK_COMPARE_OP_NEVER;
		case GFX_API::STENCIL_COMPARE::LESS_PASS:
			return VK_COMPARE_OP_LESS;
		case GFX_API::STENCIL_COMPARE::LESS_OR_EQUAL_PASS:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
		case GFX_API::STENCIL_COMPARE::EQUAL_PASS:
			return VK_COMPARE_OP_EQUAL;
		case GFX_API::STENCIL_COMPARE::NOTEQUAL_PASS:
			return VK_COMPARE_OP_NOT_EQUAL;
		case GFX_API::STENCIL_COMPARE::GREATER_OR_EQUAL_PASS:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case GFX_API::STENCIL_COMPARE::GREATER_PASS:
			return VK_COMPARE_OP_GREATER;
		case GFX_API::STENCIL_COMPARE::OFF:
		case GFX_API::STENCIL_COMPARE::ALWAYS_PASS:
			return VK_COMPARE_OP_ALWAYS;
		default:
			LOG_NOTCODED_TAPI("Find_CompareOp_byGFXStencilCompare() doesn't support this type of stencil compare!", true);
			return VK_COMPARE_OP_ALWAYS;
		}
	}

	VkStencilOp Find_StencilOp_byGFXStencilOp(GFX_API::STENCIL_OP op) {
		switch (op)
		{
		case GFX_API::STENCIL_OP::DONT_CHANGE:
			return VK_STENCIL_OP_KEEP;
		case GFX_API::STENCIL_OP::SET_ZERO:
			return VK_STENCIL_OP_ZERO;
		case GFX_API::STENCIL_OP::CHANGE:
			return VK_STENCIL_OP_REPLACE;
		case GFX_API::STENCIL_OP::CLAMPED_INCREMENT:
			return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case GFX_API::STENCIL_OP::WRAPPED_INCREMENT:
			return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case GFX_API::STENCIL_OP::CLAMPED_DECREMENT:
			return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case GFX_API::STENCIL_OP::WRAPPED_DECREMENT:
			return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		case GFX_API::STENCIL_OP::BITWISE_INVERT:
			return VK_STENCIL_OP_INVERT;
		default:
			LOG_NOTCODED_TAPI("Find_StencilOp_byGFXStencilOp() doesn't support this type of stencil operation!", true);
			return VK_STENCIL_OP_KEEP;
		}
	}
	VK_API VkBlendOp Find_BlendOp_byGFXBlendMode(GFX_API::BLEND_MODE mode) {
		switch (mode)
		{
		case GFX_API::BLEND_MODE::ADDITIVE:
			return VK_BLEND_OP_ADD;
		case GFX_API::BLEND_MODE::SUBTRACTIVE:
			return VK_BLEND_OP_SUBTRACT;
		case GFX_API::BLEND_MODE::SUBTRACTIVE_SWAPPED:
			return VK_BLEND_OP_REVERSE_SUBTRACT;
		case GFX_API::BLEND_MODE::MIN:
			return VK_BLEND_OP_MIN;
		case GFX_API::BLEND_MODE::MAX:
			return VK_BLEND_OP_MAX;
		default:
			LOG_NOTCODED_TAPI("Find_BlendOp_byGFXBlendMode() doesn't support this type of blend mode!", true);
			return VK_BLEND_OP_MAX_ENUM;
		}
	}
	VK_API VkBlendFactor Find_BlendFactor_byGFXBlendFactor(GFX_API::BLEND_FACTOR factor) {
		switch (factor)
		{
		case GFX_API::BLEND_FACTOR::ONE:
			return VK_BLEND_FACTOR_ONE;
		case GFX_API::BLEND_FACTOR::ZERO:
			return VK_BLEND_FACTOR_ZERO;
		case GFX_API::BLEND_FACTOR::SRC_COLOR:
			return VkBlendFactor::VK_BLEND_FACTOR_SRC_COLOR;
		case GFX_API::BLEND_FACTOR::SRC_1MINUSCOLOR:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case GFX_API::BLEND_FACTOR::SRC_ALPHA:
			return VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;
		case GFX_API::BLEND_FACTOR::SRC_1MINUSALPHA:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case GFX_API::BLEND_FACTOR::DST_COLOR:
			return VkBlendFactor::VK_BLEND_FACTOR_DST_COLOR;
		case GFX_API::BLEND_FACTOR::DST_1MINUSCOLOR:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case GFX_API::BLEND_FACTOR::DST_ALPHA:
			return VkBlendFactor::VK_BLEND_FACTOR_DST_ALPHA;
		case GFX_API::BLEND_FACTOR::DST_1MINUSALPHA:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case GFX_API::BLEND_FACTOR::CONST_COLOR:
			return VkBlendFactor::VK_BLEND_FACTOR_CONSTANT_COLOR;
		case GFX_API::BLEND_FACTOR::CONST_1MINUSCOLOR:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		case GFX_API::BLEND_FACTOR::CONST_ALPHA:
			return VkBlendFactor::VK_BLEND_FACTOR_CONSTANT_ALPHA;
		case GFX_API::BLEND_FACTOR::CONST_1MINUSALPHA:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
		default:
			LOG_NOTCODED_TAPI("Find_BlendFactor_byGFXBlendFactor() doesn't support this type of blend factor!", true);
			return VK_BLEND_FACTOR_MAX_ENUM;
		}
	}
	void Fill_ComponentMapping_byCHANNELs(GFX_API::TEXTURE_CHANNELs channels, VkComponentMapping& mapping) {
		switch (channels)
		{
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D32:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_BGRA8UB:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_BGRA8UNORM:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA32F:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA32UI:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA32I:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8UB:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8B:
			mapping.r = VK_COMPONENT_SWIZZLE_R;
			mapping.g = VK_COMPONENT_SWIZZLE_G;
			mapping.b = VK_COMPONENT_SWIZZLE_B;
			mapping.a = VK_COMPONENT_SWIZZLE_A;
			return;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGB32F:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGB32UI:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGB32I:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGB8UB:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGB8B:
			mapping.r = VK_COMPONENT_SWIZZLE_R;
			mapping.g = VK_COMPONENT_SWIZZLE_G;
			mapping.b = VK_COMPONENT_SWIZZLE_B;
			mapping.a = VK_COMPONENT_SWIZZLE_ZERO;
			return;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RA32F:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RA32UI:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RA32I:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RA8UB:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RA8B:
			mapping.r = VK_COMPONENT_SWIZZLE_R;
			mapping.g = VK_COMPONENT_SWIZZLE_ZERO;
			mapping.b = VK_COMPONENT_SWIZZLE_ZERO;
			mapping.a = VK_COMPONENT_SWIZZLE_A;
			return;
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_R32F:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_R32UI:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_R32I:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_R8UB:
		case GFX_API::TEXTURE_CHANNELs::API_TEXTURE_R8B:
			mapping.r = VK_COMPONENT_SWIZZLE_R;
			mapping.g = VK_COMPONENT_SWIZZLE_ZERO;
			mapping.b = VK_COMPONENT_SWIZZLE_ZERO;
			mapping.a = VK_COMPONENT_SWIZZLE_ZERO;
			return;
		default:
			break;
		}
	}
	VK_API void Find_SubpassAccessPattern(GFX_API::SUBPASS_ACCESS access, bool isSource, VkPipelineStageFlags& stageflag, VkAccessFlags& accessflag) {
		switch (access)
		{
		case GFX_API::SUBPASS_ACCESS::ALLCOMMANDS:
			if (isSource) {
				stageflag |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			}
			else {
				stageflag |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			}
			break;
		case GFX_API::SUBPASS_ACCESS::INDEX_READ:
			stageflag |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			accessflag |= VK_ACCESS_INDEX_READ_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::VERTEXATTRIB_READ:
			stageflag |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			accessflag |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::VERTEXUBUFFER_READONLY:
			stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			accessflag |= VK_ACCESS_UNIFORM_READ_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::VERTEXSBUFFER_READONLY:
		case GFX_API::SUBPASS_ACCESS::VERTEXSAMPLED_READONLY:
		case GFX_API::SUBPASS_ACCESS::VERTEXIMAGE_READONLY:
			stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			accessflag |= VK_ACCESS_SHADER_READ_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::VERTEXSBUFFER_READWRITE:
		case GFX_API::SUBPASS_ACCESS::VERTEXIMAGE_READWRITE:
			stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::VERTEXIMAGE_WRITEONLY:
		case GFX_API::SUBPASS_ACCESS::VERTEXSBUFFER_WRITEONLY:
			stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::VERTEXINPUTS_READONLY:
			stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::VERTEXINPUTS_READWRITE:
			stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::VERTEXINPUTS_WRITEONLY:
			stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
			break;

		case GFX_API::SUBPASS_ACCESS::EARLY_Z_READ:
			stageflag |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::EARLY_Z_READWRITE:
			stageflag |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::EARLY_Z_WRITEONLY:
			stageflag |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::FRAGMENTUBUFFER_READONLY:
			stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			accessflag |= VK_ACCESS_UNIFORM_READ_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::FRAGMENTSBUFFER_READONLY:
		case GFX_API::SUBPASS_ACCESS::FRAGMENTSAMPLED_READONLY:
		case GFX_API::SUBPASS_ACCESS::FRAGMENTIMAGE_READONLY:
			stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			accessflag |= VK_ACCESS_SHADER_READ_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::FRAGMENTSBUFFER_READWRITE:
		case GFX_API::SUBPASS_ACCESS::FRAGMENTIMAGE_READWRITE:
			stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::FRAGMENTIMAGE_WRITEONLY:
		case GFX_API::SUBPASS_ACCESS::FRAGMENTSBUFFER_WRITEONLY:
			stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::FRAGMENTINPUTS_READONLY:
			stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::FRAGMENTINPUTS_READWRITE:
			stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::FRAGMENTINPUTS_WRITEONLY:
			stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::FRAGMENTRT_READONLY:
			stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			accessflag |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::LATE_Z_READ:
			stageflag |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::LATE_Z_READWRITE:
			stageflag |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::LATE_Z_WRITEONLY:
			stageflag |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case GFX_API::SUBPASS_ACCESS::FRAGMENTRT_WRITEONLY:
			stageflag |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			accessflag |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		default:
			break;
		}
	}

	//Don't use this functions outside of the FindAvailableOffset
	VkDeviceSize CalculateOffset(VkDeviceSize baseoffset, VkDeviceSize AlignmentOffset, VkDeviceSize ReqAlignment) {
		VkDeviceSize FinalOffset = 0;
		VkDeviceSize LCM = std::lcm(AlignmentOffset, ReqAlignment);
		FinalOffset = (baseoffset % LCM) ? (((baseoffset / LCM) + 1) * LCM) : baseoffset;
		return FinalOffset;
	}
	VkDeviceSize VK_MemoryAllocation::FindAvailableOffset(VkDeviceSize RequiredSize, VkDeviceSize AlignmentOffset, VkDeviceSize RequiredAlignment) {
		VkDeviceSize FurthestOffset = 0;
		if (AlignmentOffset && !RequiredAlignment) {
			RequiredAlignment = AlignmentOffset;
		}
		else if (!AlignmentOffset && RequiredAlignment) {
			AlignmentOffset = RequiredAlignment;
		}
		else if (!AlignmentOffset && !RequiredAlignment) {
			AlignmentOffset = 1;
			RequiredAlignment = 1;
		}
		for (unsigned int ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
			for (unsigned int i = 0; i < Allocated_Blocks.size(ThreadID); i++) {
				VK_SubAllocation& Block = Allocated_Blocks.get(ThreadID, i);
				if (FurthestOffset <= Block.Offset) {
					FurthestOffset = Block.Offset + Block.Size;
				}
				if (!Block.isEmpty.load()) {
					continue;
				}

				VkDeviceSize Offset = CalculateOffset(Block.Offset, AlignmentOffset, RequiredAlignment);

				if (Offset + RequiredSize - Block.Offset > Block.Size ||
					Offset + RequiredSize - Block.Offset < (Block.Size / 5) * 3) {
					continue;
				}
				bool x = true, y = false;
				//Try to get the block first (Concurrent usages are prevented that way)
				if (!Block.isEmpty.compare_exchange_strong(x, y)) {
					continue;
				}
				//Don't change the block's own offset, because that'd probably cause shifting offset the memory block after free-reuse-free-reuse sequences
				return Offset;
			}
		}

		//None of the current blocks is suitable, so create a new block in this thread's local memoryblocks list
		VK_SubAllocation newblock;
		newblock.isEmpty.store(false);
		newblock.Offset = CalculateOffset(FurthestOffset, AlignmentOffset, RequiredAlignment);
		newblock.Size = RequiredSize + newblock.Offset - FurthestOffset;
		Allocated_Blocks.push_back(GFX->JobSys->GetThisThreadIndex(), newblock);
		return newblock.Offset;
	}
	void VK_MemoryAllocation::FreeBlock(VkDeviceSize Offset) {
		std::unique_lock<std::mutex> Locker;
		Allocated_Blocks.PauseAllOperations(Locker);
		for (unsigned char ThreadID = 0; ThreadID < GFX->JobSys->GetThreadCount(); ThreadID++) {
			for (unsigned int i = 0; i < Allocated_Blocks.size(ThreadID); i++) {
				VK_SubAllocation& Block = Allocated_Blocks.get(ThreadID, i);
				if (Block.isEmpty.load()) {
					continue;
				}
				if (Block.Offset <= Offset && Block.Offset + Block.Size > Offset) {
					Block.isEmpty.store(true);
					return;
				}
			}
		}
	}
}