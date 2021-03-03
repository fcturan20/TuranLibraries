#include "Vulkan_Core.h"
#include "TuranAPI/Logger_Core.h"
#define VKRENDERER ((Renderer*)GFX->RENDERER)

namespace Vulkan {
	Vulkan_Core::Vulkan_Core(vector<GFX_API::MonitorDescription>& Monitors, vector<GFX_API::GPUDescription>& GPUs, TuranAPI::Threading::JobSystem* JobSystem) : GFX_Core(Monitors, GPUs, JobSystem) {
		//Set static GFX_API variable as created Vulkan_Core, because there will only one GFX_API in run-time
		//And we will use this SELF to give commands to GFX_API in window callbacks
		SELF = this;
		LOG_STATUS_TAPI("VulkanCore: Vulkan systems are starting!");

		//Set error callback to handle all glfw errors (including initialization error)!
		glfwSetErrorCallback(Vulkan_Core::GFX_Error_Callback);

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		Save_Monitors(Monitors);

		Create_Instance();
#ifdef VULKAN_DEBUGGING
		Setup_Debugging();
#endif
		Check_Computer_Specs(GPUs);
	}

	TAPIResult Vulkan_Core::Start_SecondStage(unsigned char GPUIndex, const vector<GFX_API::MemoryType>& MEMORYTYPEs, bool Active_dearIMGUI){
		GPU_TO_RENDER = DEVICE_GPUs[GPUIndex];
		//Some basic algorithms accesses some of the GPU's datas
		//Because GFX API doesn't support multi-GPU, just give the GPU Handle to VK_States
		//Because everything in Vulkan API accesses this VK_States
		GPU* VKGPU = GFXHandleConverter(GPU*, GPU_TO_RENDER);
		VK_States.GPU_TO_RENDER = VKGPU; 

		for (unsigned int MemTypeIndex = 0; MemTypeIndex < MEMORYTYPEs.size(); MemTypeIndex++) {
			const GFX_API::MemoryType& MemType = MEMORYTYPEs[MemTypeIndex];
			VKGPU->ALLOCs[MemType.MemoryTypeIndex].FullSize = MemType.AllocationSize;
		}

	
		GFXRENDERER = new Vulkan::Renderer;
		ContentManager = new Vulkan::GPU_ContentManager;

		IMGUI_o = new GFX_API::IMGUI_Core;
		VK_IMGUI = new IMGUI_VK;

		LOG_STATUS_TAPI("VulkanCore: Vulkan systems are started!");
		return TAPI_SUCCESS;
	}
	Vulkan_Core::~Vulkan_Core() {
		Destroy_GFX_Resources();
	}

	void Vulkan_Core::GFX_Error_Callback(int error_code, const char* description) {
		LOG_CRASHING_TAPI(description, true);
	}
	void Vulkan_Core::Save_Monitors(vector<GFX_API::MonitorDescription>& Monitors) {
		Monitors.clear();
		int monitor_count;
		GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
		LOG_STATUS_TAPI("VulkanCore: " + to_string(monitor_count) + " number of monitor(s) detected!");
		for (unsigned int i = 0; i < monitor_count; i++) {
			GLFWmonitor* monitor = monitors[i];

			//Get monitor name provided by OS! It is a driver based name, so it maybe incorrect!
			const char* monitor_name = glfwGetMonitorName(monitor);
			Monitors.push_back(GFX_API::MonitorDescription());
			GFX_API::MonitorDescription& Monitor = Monitors[Monitors.size() - 1];
			Monitor.NAME = monitor_name;

			//Get videomode to detect at which resolution the OS is using the monitor
			const GLFWvidmode* monitor_vid_mode = glfwGetVideoMode(monitor);
			Monitor.WIDTH = monitor_vid_mode->width;
			Monitor.HEIGHT = monitor_vid_mode->height;
			Monitor.COLOR_BITES = monitor_vid_mode->blueBits;
			Monitor.REFRESH_RATE = monitor_vid_mode->refreshRate;
			Monitor.Handle = new MONITOR;
			CONNECTED_Monitors.push_back(Monitor.Handle);

			//Get monitor's physical size, developer may want to use it!
			glfwGetMonitorPhysicalSize(monitor, &Monitor.PHYSICAL_WIDTH, &Monitor.PHYSICAL_HEIGHT);
		}
	}
	void Vulkan_Core::Create_Instance() {
		//APPLICATION INFO
		VkApplicationInfo App_Info = {};
		App_Info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		App_Info.pApplicationName = "Vulkan DLL";
		App_Info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		App_Info.pEngineName = "GFX API";
		App_Info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		App_Info.apiVersion = VK_API_VERSION_1_2;
		VK_States.Application_Info = App_Info;

		//CHECK SUPPORTED EXTENSIONs
		uint32_t extension_count;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		vector<VkExtensionProperties> SupportedExtensions;
		//Doesn't construct VkExtensionProperties object, so we have to use resize!
		SupportedExtensions.resize(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, SupportedExtensions.data());
		for (unsigned int i = 0; i < extension_count; i++) {
			VK_States.Supported_InstanceExtensionList.push_back(SupportedExtensions[i]);
		}
		VK_States.Chech_InstanceExtensions();

		//CHECK SUPPORTED LAYERS
		vkEnumerateInstanceLayerProperties(&VK_States.Supported_LayerNumber, nullptr);
		VK_States.Supported_LayerList = new VkLayerProperties[VK_States.Supported_LayerNumber];
		vkEnumerateInstanceLayerProperties(&VK_States.Supported_LayerNumber, VK_States.Supported_LayerList);

		//INSTANCE CREATION INFO
		VkInstanceCreateInfo InstCreation_Info = {};
		InstCreation_Info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		InstCreation_Info.pApplicationInfo = &App_Info;

		//Extensions
		InstCreation_Info.enabledExtensionCount = VK_States.Active_InstanceExtensionNames.size();
		InstCreation_Info.ppEnabledExtensionNames = VK_States.Active_InstanceExtensionNames.data();

		//Validation Layers
#ifdef VULKAN_DEBUGGING
		const char* Validation_Layers[1] = {
			"VK_LAYER_KHRONOS_validation"
		};
		InstCreation_Info.enabledLayerCount = 1;
		InstCreation_Info.ppEnabledLayerNames = Validation_Layers;
#else
		InstCreation_Info.enabledLayerCount = 0;
		InstCreation_Info.ppEnabledLayerNames = nullptr;
#endif

		if (vkCreateInstance(&InstCreation_Info, nullptr, &VK_States.Vulkan_Instance) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Failed to create a Vulkan Instance!");
		}
		LOG_STATUS_TAPI("Vulkan Instance is created successfully!");

	}
	void Vulkan_Core::Setup_Debugging() {
		VkDebugUtilsMessengerCreateInfoEXT DebugMessenger_CreationInfo = {};
		DebugMessenger_CreationInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		DebugMessenger_CreationInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		DebugMessenger_CreationInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		DebugMessenger_CreationInfo.pfnUserCallback = Vulkan_States::VK_DebugCallback;
		DebugMessenger_CreationInfo.pNext = nullptr;
		DebugMessenger_CreationInfo.pUserData = nullptr;
		
		if (VK_States.vkCreateDebugUtilsMessengerEXT()(VK_States.Vulkan_Instance, &DebugMessenger_CreationInfo, nullptr, &VK_States.Debug_Messenger) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Vulkan's Debug Callback system failed to start!");
		}
		LOG_STATUS_TAPI("Vulkan Debug Callback system is started!");
	}


	void Gather_PhysicalDeviceInformations(GPU* VKGPU) {
		vkGetPhysicalDeviceProperties(VKGPU->Physical_Device, &VKGPU->Device_Properties);
		vkGetPhysicalDeviceFeatures(VKGPU->Physical_Device, &VKGPU->Supported_Features);

		//GET QUEUE FAMILIES, SAVE THEM TO GPU OBJECT, CHECK AND SAVE GRAPHICS,COMPUTE,TRANSFER QUEUEFAMILIES INDEX
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(VKGPU->Physical_Device, &queueFamilyCount, nullptr);
		VKGPU->QueueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(VKGPU->Physical_Device, &queueFamilyCount, VKGPU->QueueFamilyProperties.data());
		
		vkGetPhysicalDeviceMemoryProperties(VKGPU->Physical_Device, &VKGPU->MemoryProperties);
	}
	void Analize_PhysicalDeviceMemoryProperties(GPU* VKGPU, GFX_API::GPUDescription& GPUdesc) {
		for (uint32_t MemoryTypeIndex = 0; MemoryTypeIndex < VKGPU->MemoryProperties.memoryTypeCount; MemoryTypeIndex++) {
			VkMemoryType& MemoryType = VKGPU->MemoryProperties.memoryTypes[MemoryTypeIndex];
			bool isDeviceLocal = false;
			bool isHostVisible = false;
			bool isHostCoherent = false;
			bool isHostCached = false;

			if ((MemoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
				isDeviceLocal = true;
			}
			if ((MemoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
				isHostVisible = true;
			}
			if ((MemoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
				isHostCoherent = true;
			}
			if ((MemoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
				isHostCached = true;
			}

			if (GPUdesc.GPU_TYPE != GFX_API::GPU_TYPEs::DISCRETE_GPU) {
				continue;
			}
			if (!isDeviceLocal && !isHostVisible && !isHostCoherent && !isHostCached) {
				continue;
			}
			if (isDeviceLocal) {
				if (isHostVisible && isHostCoherent) {
					GFX_API::MemoryType MEMTYPE(GFX_API::SUBALLOCATEBUFFERTYPEs::FASTHOSTVISIBLE, GPUdesc.MEMTYPEs.size());
					GPUdesc.MEMTYPEs.push_back(MEMTYPE);
					VK_MemoryAllocation alloc;
					alloc.MemoryTypeIndex = MemoryTypeIndex;
					alloc.TYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::FASTHOSTVISIBLE;
					VKGPU->ALLOCs.push_back(alloc);
					GPUdesc.FASTHOSTVISIBLE_MaxMemorySize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
					LOG_STATUS_TAPI("Found FAST HOST VISIBLE BIT! Size: " + to_string(GPUdesc.FASTHOSTVISIBLE_MaxMemorySize));
				}
				else {
					GFX_API::MemoryType MEMTYPE(GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL, GPUdesc.MEMTYPEs.size());
					GPUdesc.MEMTYPEs.push_back(MEMTYPE);
					VK_MemoryAllocation alloc;
					alloc.TYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL;
					alloc.MemoryTypeIndex = MemoryTypeIndex;
					VKGPU->ALLOCs.push_back(alloc);
					GPUdesc.DEVICELOCAL_MaxMemorySize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
					LOG_STATUS_TAPI("Found DEVICE LOCAL BIT! Size: " + to_string(GPUdesc.DEVICELOCAL_MaxMemorySize));
				}
			}
			else if (isHostVisible && isHostCoherent) {
				if (isHostCached) {
					GFX_API::MemoryType MEMTYPE(GFX_API::SUBALLOCATEBUFFERTYPEs::READBACK, GPUdesc.MEMTYPEs.size());
					GPUdesc.MEMTYPEs.push_back(MEMTYPE);
					VK_MemoryAllocation alloc;
					alloc.MemoryTypeIndex = MemoryTypeIndex;
					alloc.TYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::READBACK;
					VKGPU->ALLOCs.push_back(alloc);
					GPUdesc.READBACK_MaxMemorySize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
					LOG_STATUS_TAPI("Found READBACK BIT! Size: " + to_string(GPUdesc.READBACK_MaxMemorySize));
				}
				else {
					GFX_API::MemoryType MEMTYPE(GFX_API::SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE, GPUdesc.MEMTYPEs.size());
					GPUdesc.MEMTYPEs.push_back(MEMTYPE);
					VK_MemoryAllocation alloc;
					alloc.MemoryTypeIndex = MemoryTypeIndex;
					alloc.TYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE;
					VKGPU->ALLOCs.push_back(alloc);
					GPUdesc.HOSTVISIBLE_MaxMemorySize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
					LOG_STATUS_TAPI("Found HOST VISIBLE BIT! Size: " + to_string(GPUdesc.HOSTVISIBLE_MaxMemorySize));
				}
			}
		}
	}
	void Analize_Queues(GPU* VKGPU, GFX_API::GPUDescription& GPUdesc) {
		bool is_presentationfound = false;
		for (unsigned int queuefamily_index = 0; queuefamily_index < VKGPU->QueueFamilyProperties.size(); queuefamily_index++) {
			VkQueueFamilyProperties* QueueFamily = &VKGPU->QueueFamilyProperties[queuefamily_index];
			VK_QUEUE VKQUEUE;
			VKQUEUE.QueueFamilyIndex = queuefamily_index;
			if (QueueFamily->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				GPUdesc.is_GraphicOperations_Supported = true;
				VKQUEUE.SupportFlag.is_GRAPHICSsupported = true;
				VKQUEUE.QueueFeatureScore++;
			}
			if (QueueFamily->queueFlags & VK_QUEUE_COMPUTE_BIT) {
				GPUdesc.is_ComputeOperations_Supported = true;
				VKQUEUE.SupportFlag.is_COMPUTEsupported = true;
				VKGPU->COMPUTE_supportedqueuecount++;
				VKQUEUE.QueueFeatureScore++;
			}
			if (QueueFamily->queueFlags & VK_QUEUE_TRANSFER_BIT) {
				GPUdesc.is_TransferOperations_Supported = true;
				VKQUEUE.SupportFlag.is_TRANSFERsupported = true;
				VKGPU->TRANSFERs_supportedqueuecount++;
				VKQUEUE.QueueFeatureScore++;
			}

			VKGPU->QUEUEs.push_back(VKQUEUE);
			if (VKQUEUE.SupportFlag.is_GRAPHICSsupported) {
				VKGPU->GRAPHICS_QUEUEIndex = VKGPU->QUEUEs.size() - 1;
			}
		}
		if (!GPUdesc.is_GraphicOperations_Supported || !GPUdesc.is_TransferOperations_Supported || !GPUdesc.is_ComputeOperations_Supported) {
			LOG_CRASHING_TAPI("The GPU doesn't support one of the following operations, so we can't let you use this GPU: Compute, Transfer, Graphics");
			return;
		}
		//Sort the queues by their feature count (Example: Element 0 is Transfer Only, Element 1 is Transfer-Compute, Element 2 is Graphics-Transfer-Compute etc)
		//QuickSort Algorithm
		if (VKGPU->QUEUEs.size()) {
			bool should_Sort = true;
			while (should_Sort) {
				should_Sort = false;
				for (unsigned char QueueIndex = 0; QueueIndex < VKGPU->QUEUEs.size() - 1; QueueIndex++) {
					if (VKGPU->QUEUEs[QueueIndex + 1].QueueFeatureScore < VKGPU->QUEUEs[QueueIndex].QueueFeatureScore) {
						should_Sort = true;
						VK_QUEUE SecondQueue;
						SecondQueue = VKGPU->QUEUEs[QueueIndex + 1];
						VKGPU->QUEUEs[QueueIndex + 1] = VKGPU->QUEUEs[QueueIndex];
						VKGPU->QUEUEs[QueueIndex] = SecondQueue;
					}
				}
			}
		}
	}
	void Vulkan_Core::Check_Computer_Specs(vector<GFX_API::GPUDescription>& GPUdescs) {
		LOG_STATUS_TAPI("Started to check Computer Specifications!");
		GPUdescs.clear();

		//CHECK GPUs
		uint32_t GPU_NUMBER = 0;
		vkEnumeratePhysicalDevices(VK_States.Vulkan_Instance, &GPU_NUMBER, nullptr);
		vector<VkPhysicalDevice> Physical_GPU_LIST;
		for (unsigned int i = 0; i < GPU_NUMBER; i++) {
			Physical_GPU_LIST.push_back(VkPhysicalDevice());
		}
		vkEnumeratePhysicalDevices(VK_States.Vulkan_Instance, &GPU_NUMBER, Physical_GPU_LIST.data());

		if (GPU_NUMBER == 0) {
			LOG_CRASHING_TAPI("There is no GPU that has Vulkan support! Updating your drivers or Upgrading the OS may help");
		}

		//GET GPU INFORMATIONs, QUEUE FAMILIES etc
		for (unsigned int i = 0; i < GPU_NUMBER; i++) {
			GPU* VKGPU = new GPU;
			GFX_API::GPUDescription GPUdesc;
			VKGPU->Physical_Device = Physical_GPU_LIST[i];
			Gather_PhysicalDeviceInformations(VKGPU);
			const char* VendorName = VK_States.Convert_VendorID_toaString(VKGPU->Device_Properties.vendorID);

			//SAVE BASIC INFOs TO THE GPU DESC
			GPUdesc.MODEL = VKGPU->Device_Properties.deviceName;
			GPUdesc.DRIVER_VERSION = VKGPU->Device_Properties.driverVersion;
			GPUdesc.API_VERSION = VKGPU->Device_Properties.apiVersion;
			GPUdesc.DRIVER_VERSION = VKGPU->Device_Properties.driverVersion;

			//CHECK IF GPU IS DISCRETE OR INTEGRATED
			switch (VKGPU->Device_Properties.deviceType) {
			case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				GPUdesc.GPU_TYPE = GFX_API::GPU_TYPEs::DISCRETE_GPU;
				break;
			case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				GPUdesc.GPU_TYPE = GFX_API::GPU_TYPEs::INTEGRATED_GPU;
				break;
			default:
				//const char* CrashingError = Text_Add("Vulkan_Core::Check_Computer_Specs failed to find GPU's Type (Only Discrete and Integrated GPUs supported!), Type is:",
					//std::to_string(VKGPU->Device_Properties.deviceType).c_str());
				LOG_CRASHING_TAPI("There is an error about GPU!");
				break;
			}

			Analize_PhysicalDeviceMemoryProperties(VKGPU, GPUdesc);
			Analize_Queues(VKGPU, GPUdesc);


			vector<VkDeviceQueueCreateInfo> QueueCreationInfos;
			//Queue Creation Processes
			float QueuePriority = 1.0f;
			for (unsigned int QueueIndex = 0; QueueIndex < VKGPU->QUEUEs.size(); QueueIndex++) {
				VK_QUEUE& QUEUE = VKGPU->QUEUEs[QueueIndex];
				VkDeviceQueueCreateInfo QueueInfo = {};
				QueueInfo.flags = 0;
				QueueInfo.pNext = nullptr;
				QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				QueueInfo.queueFamilyIndex = QUEUE.QueueFamilyIndex;
				QueueInfo.pQueuePriorities = &QueuePriority;
				QueueInfo.queueCount = 1;
				QueueCreationInfos.push_back(QueueInfo);
			}

			VkDeviceCreateInfo Logical_Device_CreationInfo{};
			Logical_Device_CreationInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			Logical_Device_CreationInfo.flags = 0;
			Logical_Device_CreationInfo.pQueueCreateInfos = QueueCreationInfos.data();
			Logical_Device_CreationInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreationInfos.size());
			VK_States.Check_DeviceExtensions(VKGPU);
			VK_States.Check_DeviceFeatures(VKGPU, GPUdesc);


			Logical_Device_CreationInfo.enabledExtensionCount = VKGPU->Active_DeviceExtensions.size();
			Logical_Device_CreationInfo.ppEnabledExtensionNames = VKGPU->Active_DeviceExtensions.data();
			Logical_Device_CreationInfo.pEnabledFeatures = &VKGPU->Active_Features;

			Logical_Device_CreationInfo.enabledLayerCount = 0;

			if (vkCreateDevice(VKGPU->Physical_Device, &Logical_Device_CreationInfo, nullptr, &VKGPU->Logical_Device) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Vulkan failed to create a Logical Device!");
				return;
			}

			VKGPU->AllQueueFamilies = new uint32_t[VKGPU->QUEUEs.size()];
			for (unsigned int QueueIndex = 0; QueueIndex < VKGPU->QUEUEs.size(); QueueIndex++) {
				vkGetDeviceQueue(VKGPU->Logical_Device, VKGPU->QUEUEs[QueueIndex].QueueFamilyIndex, 0, &VKGPU->QUEUEs[QueueIndex].Queue);
				VKGPU->AllQueueFamilies[QueueIndex] = VKGPU->QUEUEs[QueueIndex].QueueFamilyIndex;
			}
			LOG_STATUS_TAPI("Vulkan created a Logical Device!");

			GPUdescs.push_back(GPUdesc);
			DEVICE_GPUs.push_back(VKGPU);
		}

		LOG_STATUS_TAPI("Finished checking Computer Specifications!");
	}

	bool Vulkan_Core::Create_WindowSwapchain(WINDOW* Vulkan_Window, unsigned int WIDTH, unsigned int HEIGHT, VkSwapchainKHR* SwapchainOBJ, GFX_API::GFXHandle* SwapchainTextureHandles) {
		GPU* Vulkan_GPU = VK_States.GPU_TO_RENDER;

		//Choose Surface Format
		VkSurfaceFormatKHR Window_SurfaceFormat = {};
		for (unsigned int i = 0; i < Vulkan_Window->SurfaceFormats.size(); i++) {
			VkSurfaceFormatKHR& SurfaceFormat = Vulkan_Window->SurfaceFormats[i];
			if (SurfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && SurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				Window_SurfaceFormat = SurfaceFormat;
			}
		}

		//Choose Surface Presentation Mode
		VkPresentModeKHR Window_PresentationMode = {};
		for (unsigned int i = 0; i < Vulkan_Window->PresentationModes.size(); i++) {
			VkPresentModeKHR& PresentationMode = Vulkan_Window->PresentationModes[i];
			if (PresentationMode == VK_PRESENT_MODE_FIFO_KHR) {
				Window_PresentationMode = PresentationMode;
			}
		}


		VkExtent2D Window_ImageExtent = { WIDTH, HEIGHT };
		uint32_t image_count = 0;
		if (Vulkan_Window->SurfaceCapabilities.maxImageCount > Vulkan_Window->SurfaceCapabilities.minImageCount) {
			image_count = 2;
		}
		else {
			LOG_NOTCODED_TAPI("VulkanCore: Window Surface Capabilities have issues, maxImageCount <= minImageCount!", true);
			return false;
		}
		VkSwapchainCreateInfoKHR swpchn_ci = {};
		swpchn_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swpchn_ci.flags = 0;
		swpchn_ci.pNext = nullptr;
		swpchn_ci.presentMode = Window_PresentationMode;
		swpchn_ci.surface = Vulkan_Window->Window_Surface;
		swpchn_ci.minImageCount = image_count;
		swpchn_ci.imageFormat = Window_SurfaceFormat.format;
		swpchn_ci.imageColorSpace = Window_SurfaceFormat.colorSpace;
		swpchn_ci.imageExtent = Window_ImageExtent;
		swpchn_ci.imageArrayLayers = 1;
		swpchn_ci.imageUsage = 0;
		if (Vulkan_Window->SWAPCHAINUSAGE.isCopiableFrom) {
			swpchn_ci.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
		if (Vulkan_Window->SWAPCHAINUSAGE.isCopiableTo) {
			swpchn_ci.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		if (Vulkan_Window->SWAPCHAINUSAGE.isRandomlyWrittenTo) {
			swpchn_ci.imageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}
		if (Vulkan_Window->SWAPCHAINUSAGE.isRenderableTo) {
			swpchn_ci.imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		if (Vulkan_Window->SWAPCHAINUSAGE.isSampledReadOnly) {
			swpchn_ci.imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		swpchn_ci.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swpchn_ci.clipped = VK_TRUE;
		swpchn_ci.preTransform = Vulkan_Window->SurfaceCapabilities.currentTransform;
		swpchn_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		if (Vulkan_Window->Window_SwapChain) {
			swpchn_ci.oldSwapchain = Vulkan_Window->Window_SwapChain;
		}
		else {
			swpchn_ci.oldSwapchain = nullptr;
		}

		if (Vulkan_GPU->QUEUEs.size() > 1) {
			swpchn_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		}
		else {
			swpchn_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		swpchn_ci.pQueueFamilyIndices = Vulkan_GPU->AllQueueFamilies;
		swpchn_ci.queueFamilyIndexCount = Vulkan_GPU->QUEUEs.size();


		if (vkCreateSwapchainKHR(Vulkan_GPU->Logical_Device, &swpchn_ci, nullptr, SwapchainOBJ) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("VulkanCore: Failed to create a SwapChain for a Window");
			return false;
		}

		//Get Swapchain images
		uint32_t created_imagecount = 0;
		vkGetSwapchainImagesKHR(Vulkan_GPU->Logical_Device, *SwapchainOBJ, &created_imagecount, nullptr);
		VkImage* SWPCHN_IMGs = new VkImage[created_imagecount];
		vkGetSwapchainImagesKHR(Vulkan_GPU->Logical_Device, *SwapchainOBJ, &created_imagecount, SWPCHN_IMGs);
		if (created_imagecount < 2) {
			LOG_ERROR_TAPI("GFX API asked for 2 swapchain textures but Vulkan gave less number of textures!");
			return false;
		}
		else if (created_imagecount > 2) {
			LOG_STATUS_TAPI("GFX API asked for 2 swapchain textures but Vulkan gave more than that, so GFX API only used 2 of them!");
		}
		for (unsigned int vkim_index = 0; vkim_index < 2; vkim_index++) {
			VK_Texture* SWAPCHAINTEXTURE = new VK_Texture;
			SWAPCHAINTEXTURE->CHANNELs = GFX_API::TEXTURE_CHANNELs::API_TEXTURE_BGRA8UNORM;
			SWAPCHAINTEXTURE->WIDTH = WIDTH;
			SWAPCHAINTEXTURE->HEIGHT = HEIGHT;
			SWAPCHAINTEXTURE->DATA_SIZE = SWAPCHAINTEXTURE->WIDTH * SWAPCHAINTEXTURE->HEIGHT * 4;
			SWAPCHAINTEXTURE->Image = SWPCHN_IMGs[vkim_index];
			SWAPCHAINTEXTURE->USAGE.isCopiableFrom = true;
			SWAPCHAINTEXTURE->USAGE.isCopiableTo = true;
			SWAPCHAINTEXTURE->USAGE.isRenderableTo = true;
			SWAPCHAINTEXTURE->USAGE.isSampledReadOnly = true;

			SwapchainTextureHandles[vkim_index] = SWAPCHAINTEXTURE;
		}
		delete SWPCHN_IMGs;

		if (Vulkan_Window->SWAPCHAINUSAGE.isRandomlyWrittenTo || Vulkan_Window->SWAPCHAINUSAGE.isRenderableTo || Vulkan_Window->SWAPCHAINUSAGE.isSampledReadOnly) {
			for (unsigned int i = 0; i < 2; i++) {
				VkImageViewCreateInfo ImageView_ci = {};
				ImageView_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				VK_Texture* SwapchainTexture = GFXHandleConverter(VK_Texture*, SwapchainTextureHandles[i]);
				ImageView_ci.image = SwapchainTexture->Image;
				ImageView_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
				ImageView_ci.format = Window_SurfaceFormat.format;
				ImageView_ci.flags = 0;
				ImageView_ci.pNext = nullptr;
				ImageView_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				ImageView_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				ImageView_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				ImageView_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
				ImageView_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				ImageView_ci.subresourceRange.baseArrayLayer = 0;
				ImageView_ci.subresourceRange.baseMipLevel = 0;
				ImageView_ci.subresourceRange.layerCount = 1;
				ImageView_ci.subresourceRange.levelCount = 1;

				if (vkCreateImageView(Vulkan_GPU->Logical_Device, &ImageView_ci, nullptr, &SwapchainTexture->ImageView) != VK_SUCCESS) {
					LOG_CRASHING_TAPI("VulkanCore: Image View creation has failed!");
					return false;
				}
			}
		}
		return true;
	}
	GFX_API::GFXHandle Vulkan_Core::CreateWindow(const GFX_API::WindowDescription& Desc, GFX_API::GFXHandle* SwapchainTextureHandles, GFX_API::Texture_Properties& SwapchainTextureProperties) {
		LOG_STATUS_TAPI("Window creation has started!");
		GPU* Vulkan_GPU = GFXHandleConverter(GPU*, GPU_TO_RENDER);
		
		if (Desc.resize_cb) {
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		}
		//Create window as it will share resources with Renderer Context to get display texture!
		GLFWwindow* glfw_window = glfwCreateWindow(Desc.WIDTH, Desc.HEIGHT, Desc.NAME, NULL, nullptr);
		WINDOW* Vulkan_Window = new WINDOW;
		Vulkan_Window->LASTWIDTH = Desc.WIDTH;
		Vulkan_Window->LASTHEIGHT = Desc.HEIGHT;
		Vulkan_Window->DISPLAYMODE = Desc.MODE;
		Vulkan_Window->MONITOR = Desc.MONITOR;
		Vulkan_Window->NAME = Desc.NAME;
		Vulkan_Window->GLFW_WINDOW = glfw_window;
		Vulkan_Window->SWAPCHAINUSAGE = Desc.SWAPCHAINUSAGEs;
		Vulkan_Window->resize_cb = Desc.resize_cb;
		Vulkan_Window->UserPTR = Desc.UserPointer;
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		//Check and Report if GLFW fails
		if (glfw_window == NULL) {
			LOG_CRASHING_TAPI("VulkanCore: We failed to create the window because of GLFW!");
			delete Vulkan_Window;
			return nullptr;
		}

		//Window VulkanSurface Creation
		VkSurfaceKHR Window_Surface = {};
		if (glfwCreateWindowSurface(VK_States.Vulkan_Instance, Vulkan_Window->GLFW_WINDOW, nullptr, &Window_Surface) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("GLFW failed to create a window surface");
			return nullptr;
		}
		else {
			LOG_STATUS_TAPI("GLFW created a window surface!");
		}
		Vulkan_Window->Window_Surface = Window_Surface;

		if (Desc.resize_cb) {
			glfwSetWindowUserPointer(Vulkan_Window->GLFW_WINDOW, Vulkan_Window);
			glfwSetWindowSizeCallback(Vulkan_Window->GLFW_WINDOW, Window_ResizeCallback);
		}


		//Finding GPU_TO_RENDER's Surface Capabilities
		
		for (unsigned int QueueIndex = 0; QueueIndex < Vulkan_GPU->QUEUEs.size(); QueueIndex++) {
			VkBool32 Does_Support = 0;
			vkGetPhysicalDeviceSurfaceSupportKHR(Vulkan_GPU->Physical_Device, Vulkan_GPU->QUEUEs[QueueIndex].QueueFamilyIndex, Vulkan_Window->Window_Surface, &Does_Support);
			if (Does_Support) {
				Vulkan_GPU->QUEUEs[QueueIndex].SupportFlag.is_PRESENTATIONsupported = true;
				Vulkan_GPU->QUEUEs[QueueIndex].QueueFeatureScore++;
				Vulkan_Window->DISPLAY_QUEUEIndex = QueueIndex;
			}
		}

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Vulkan_GPU->Physical_Device, Vulkan_Window->Window_Surface, &Vulkan_Window->SurfaceCapabilities);
		uint32_t FormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan_GPU->Physical_Device, Vulkan_Window->Window_Surface, &FormatCount, nullptr);
		Vulkan_Window->SurfaceFormats.resize(FormatCount);
		if (FormatCount != 0) {
			vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan_GPU->Physical_Device, Vulkan_Window->Window_Surface, &FormatCount, Vulkan_Window->SurfaceFormats.data());
		}
		else {
			LOG_CRASHING_TAPI("This GPU doesn't support this type of windows, please try again with a different window configuration!");
			delete Vulkan_Window;
			return nullptr;
		}

		uint32_t PresentationModesCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan_GPU->Physical_Device, Vulkan_Window->Window_Surface, &PresentationModesCount, nullptr);
		Vulkan_Window->PresentationModes.resize(PresentationModesCount);
		if (PresentationModesCount != 0) {
			vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan_GPU->Physical_Device, Vulkan_Window->Window_Surface, &PresentationModesCount, Vulkan_Window->PresentationModes.data());
		}

		if (!Create_WindowSwapchain(Vulkan_Window, Vulkan_Window->LASTWIDTH, Vulkan_Window->LASTHEIGHT, &Vulkan_Window->Window_SwapChain, SwapchainTextureHandles)) {
			LOG_ERROR_TAPI("Window's swapchain creation has failed, so window's creation!");
			vkDestroySurfaceKHR(VK_States.Vulkan_Instance, Vulkan_Window->Window_Surface, nullptr);
			glfwDestroyWindow(Vulkan_Window->GLFW_WINDOW);
			delete Vulkan_Window;
			return nullptr;
		}
		Vulkan_Window->Swapchain_Textures[0] = SwapchainTextureHandles[0];
		Vulkan_Window->Swapchain_Textures[1] = SwapchainTextureHandles[1];

		//Create presentation wait semaphores
		//We are creating 3 semaphores because if 2+ frames combined is faster than vertical blank, there is tearing!
		//3 semaphores fixes it because VkQueuePresentKHR already blocks if you try to present the texture currently displayed
		for (unsigned char SemaphoreIndex = 0; SemaphoreIndex < 3; SemaphoreIndex++) {
			VkSemaphoreCreateInfo Semaphore_ci = {};
			Semaphore_ci.flags = 0;
			Semaphore_ci.pNext = nullptr;
			Semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VK_Semaphore VKdata;
			VKdata.isUsed = true;


			if (vkCreateSemaphore(Vulkan_GPU->Logical_Device, &Semaphore_ci, nullptr, &VKdata.SPHandle) != VK_SUCCESS) {
				LOG_CRASHING_TAPI("Window creation has failed while creating semaphores for each swapchain texture!");
				return nullptr;
			}

			VKRENDERER->Semaphores.push_back(VKdata);
			Vulkan_Window->PresentationWaitSemaphoreIndexes[SemaphoreIndex] = VKRENDERER->Semaphores.size() - 1;
		}


		LOG_STATUS_TAPI("Window creation is successful!");
		WINDOWs.push_back(Vulkan_Window);
		return Vulkan_Window;
	}
	unsigned char Vulkan_Core::Get_WindowFrameIndex(GFX_API::GFXHandle WindowHandle) {
		return GFXHandleConverter(WINDOW*, WindowHandle)->CurrentFrameSWPCHNIndex;
	}
	vector<GFX_API::GFXHandle>& Vulkan_Core::Get_WindowHandles() {
		return WINDOWs;
	}
	bool Vulkan_Core::GetTextureTypeLimits(const GFX_API::Texture_Properties& Properties, GFX_API::TEXTUREUSAGEFLAG UsageFlag, unsigned int GPUIndex,
		unsigned int& MAXWIDTH, unsigned int& MAXHEIGHT, unsigned int& MAXDEPTH, unsigned int& MAXMIPLEVEL) {
		GPU* VKGPU = GFXHandleConverter(GPU*, DEVICE_GPUs[GPUIndex]);

		VkImageFormatProperties props;
		if (vkGetPhysicalDeviceImageFormatProperties(VKGPU->Physical_Device, Find_VkFormat_byTEXTURECHANNELs(Properties.CHANNEL_TYPE), 
			Find_VkImageType(Properties.DIMENSION), Find_VkTiling(Properties.DATAORDER), Find_VKImageUsage_forGFXTextureDesc(UsageFlag, Properties.CHANNEL_TYPE), 
			0, &props) != VK_SUCCESS) {
			LOG_ERROR_TAPI("GFX->GetTextureTypeLimits() has failed!");
			return false;
		}
		MAXWIDTH = props.maxExtent.width;
		MAXHEIGHT = props.maxExtent.height;
		MAXDEPTH = props.maxExtent.depth;
		MAXMIPLEVEL = props.maxMipLevels;
		return true;
	}
	void Vulkan_Core::GetSupportedAllocations_ofTexture(const GFX_API::Texture_Description& TEXTURE, unsigned int GPUIndex, unsigned int& SupportedMemoryTypesBitset) {
		GPU* VKGPU = GFXHandleConverter(GPU*, DEVICE_GPUs[GPUIndex]);

		VkImageCreateInfo im_ci = {};
		im_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		im_ci.arrayLayers = 1;
		im_ci.extent.width = TEXTURE.WIDTH;
		im_ci.extent.height = TEXTURE.HEIGHT;
		im_ci.extent.depth = 1;
		im_ci.flags = 0;
		im_ci.format = Find_VkFormat_byTEXTURECHANNELs(TEXTURE.Properties.CHANNEL_TYPE);
		im_ci.imageType = Find_VkImageType(TEXTURE.Properties.DIMENSION);
		im_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		im_ci.mipLevels = 1;
		im_ci.pNext = nullptr;

		
		if (VKGPU->QUEUEs.size() > 1) {
			im_ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
		}
		else {
			im_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		im_ci.pQueueFamilyIndices = VKGPU->AllQueueFamilies;
		im_ci.queueFamilyIndexCount = VKGPU->QUEUEs.size();
		im_ci.tiling = Find_VkTiling(TEXTURE.Properties.DATAORDER);
		im_ci.usage = Find_VKImageUsage_forGFXTextureDesc(TEXTURE.USAGE, TEXTURE.Properties.CHANNEL_TYPE);
		im_ci.samples = VK_SAMPLE_COUNT_1_BIT;


		VkImage Imageobj;
		if (vkCreateImage(VKGPU->Logical_Device, &im_ci, nullptr, &Imageobj) != VK_SUCCESS) {
			LOG_ERROR_TAPI("GFX->IsTextureSupported() has failed in vkCreateImage()!");
			return;
		}

		VkMemoryRequirements req;
		vkGetImageMemoryRequirements(VKGPU->Logical_Device, Imageobj, &req);
		vkDestroyImage(VKGPU->Logical_Device, Imageobj, nullptr);
		bool isFound = false;
		for (unsigned int GFXMemoryTypeIndex = 0; GFXMemoryTypeIndex < VKGPU->ALLOCs.size(); GFXMemoryTypeIndex++) {
			VK_MemoryAllocation& ALLOC = VKGPU->ALLOCs[GFXMemoryTypeIndex];
			if (req.memoryTypeBits & (1u << ALLOC.MemoryTypeIndex)) {
				SupportedMemoryTypesBitset |= (1u << GFXMemoryTypeIndex);
				isFound = true;
			}
		}
		if (!isFound) {
			LOG_CRASHING_TAPI("Your image is supported by a memory allocation that GFX API doesn't support to allocate, please report this issue!");
		}
	}

	//Destroy Operations

	void Vulkan_Core::Destroy_GFX_Resources() {
		GPU* Vulkan_GPU = GFXHandleConverter(GPU*, GPU_TO_RENDER);

		VkDescriptorPool IMGUIPOOL = ((Renderer*)RENDERER)->IMGUIPOOL;
		delete (Renderer*)RENDERER;
		delete (GPU_ContentManager*)ContentManager;
		//Destroy dear IMGUI
		VK_IMGUI->Destroy_IMGUIResources();
		vkDestroyDescriptorPool(Vulkan_GPU->Logical_Device, IMGUIPOOL, nullptr);

		//Close windows and delete related datas
		for (unsigned int WindowIndex = 0; WindowIndex < WINDOWs.size(); WindowIndex++) {
			WINDOW* window = GFXHandleConverter(WINDOW*, WINDOWs[WindowIndex]);
			VK_Texture* Texture0 = GFXHandleConverter(VK_Texture*, window->Swapchain_Textures[0]);
			VK_Texture* Texture1 = GFXHandleConverter(VK_Texture*, window->Swapchain_Textures[1]);
			vkDestroyImageView(Vulkan_GPU->Logical_Device, Texture0->ImageView, nullptr);
			vkDestroyImageView(Vulkan_GPU->Logical_Device, Texture1->ImageView, nullptr);

			vkDestroySwapchainKHR(Vulkan_GPU->Logical_Device, window->Window_SwapChain, nullptr);
			vkDestroySurfaceKHR(VK_States.Vulkan_Instance, window->Window_Surface, nullptr);
			glfwDestroyWindow(window->GLFW_WINDOW);
		}

		//Free the allocated memories
		for (unsigned int AllocIndex = 0; AllocIndex < Vulkan_GPU->ALLOCs.size(); AllocIndex++) {
			VK_MemoryAllocation& Alloc = Vulkan_GPU->ALLOCs[AllocIndex];
			if (Alloc.FullSize) {
				vkDestroyBuffer(Vulkan_GPU->Logical_Device, Alloc.Buffer, nullptr);
				vkFreeMemory(Vulkan_GPU->Logical_Device, Alloc.Allocated_Memory, nullptr);
			}
		}
		Vulkan_GPU->ALLOCs.clear();

		//GPU deleting
		for (unsigned int i = 0; i < DEVICE_GPUs.size(); i++) {
			GPU* a_Vulkan_GPU = GFXHandleConverter(GPU*, DEVICE_GPUs[i]);
			delete[] a_Vulkan_GPU->AllQueueFamilies;
			for (unsigned int QueueIndex = 0; QueueIndex < a_Vulkan_GPU->QUEUEs.size(); QueueIndex++) {
				VK_QUEUE& Queue = a_Vulkan_GPU->QUEUEs[QueueIndex];
				if (Queue.CommandPools[0].CPHandle) {
					vkDestroyCommandPool(a_Vulkan_GPU->Logical_Device, Queue.CommandPools[0].CPHandle, nullptr);
					vkDestroyCommandPool(a_Vulkan_GPU->Logical_Device, Queue.CommandPools[1].CPHandle, nullptr);
					vkDestroyFence(a_Vulkan_GPU->Logical_Device, Queue.RenderGraphFences[0].Fence_o, nullptr);
					vkDestroyFence(a_Vulkan_GPU->Logical_Device, Queue.RenderGraphFences[1].Fence_o, nullptr);
				}
			}

			vkDestroyDevice(a_Vulkan_GPU->Logical_Device, nullptr);
		}
		if (VK_States.Debug_Messenger) {
			VK_States.vkDestroyDebugUtilsMessengerEXT()(VK_States.Vulkan_Instance, VK_States.Debug_Messenger, nullptr);
		}
		vkDestroyInstance(VK_States.Vulkan_Instance, nullptr);

		glfwTerminate();


		delete IMGUI_o;
		LOG_STATUS_TAPI("Vulkan Resources are destroyed!");
	}

	//Input (Keyboard-Controller) Operations
	void Vulkan_Core::Take_Inputs() {
		Vulkan_Core* VKCORE = ((Vulkan_Core*)GFX);
		GPU* VKGPU = (GPU*)VKCORE->VK_States.GPU_TO_RENDER;

		glfwPollEvents();
		if (isAnyWindowResized) {
			vkDeviceWaitIdle(VKGPU->Logical_Device);
			for (unsigned int WindowIndex = 0; WindowIndex < WINDOWs.size(); WindowIndex++) {
				WINDOW* VKWINDOW = GFXHandleConverter(WINDOW*, WINDOWs[WindowIndex]);
				if (!VKWINDOW->isResized) {
					continue;
				}
				if (VKWINDOW->NEWWIDTH == VKWINDOW->LASTWIDTH && VKWINDOW->NEWHEIGHT == VKWINDOW->LASTHEIGHT) {
					VKWINDOW->isResized = false;
					continue;
				}

				VkSwapchainKHR swpchn;
				GFX_API::GFXHandle* swpchntextures = new GFX_API::GFXHandle[2];
				//If new window size isn't able to create a swapchain, return to last window size
				if (!VKCORE->Create_WindowSwapchain(VKWINDOW, VKWINDOW->NEWWIDTH, VKWINDOW->NEWHEIGHT, &swpchn, swpchntextures)) {
					LOG_ERROR_TAPI("New size for the window is not possible, returns to the last successful size!");
					glfwSetWindowSize(VKWINDOW->GLFW_WINDOW, VKWINDOW->LASTWIDTH, VKWINDOW->LASTHEIGHT);
					VKWINDOW->isResized = false;
					delete[] swpchntextures;
					continue;
				}

				VK_Texture* oldswpchn0 = GFXHandleConverter(VK_Texture*, VKWINDOW->Swapchain_Textures[0]);
				VK_Texture* oldswpchn1 = GFXHandleConverter(VK_Texture*, VKWINDOW->Swapchain_Textures[1]);

				vkDestroyImageView(VKGPU->Logical_Device, oldswpchn0->ImageView, nullptr);
				vkDestroyImageView(VKGPU->Logical_Device, oldswpchn1->ImageView, nullptr);
				vkDestroySwapchainKHR(VKGPU->Logical_Device, VKWINDOW->Window_SwapChain, nullptr);
				oldswpchn0->Image = VK_NULL_HANDLE;
				oldswpchn0->ImageView = VK_NULL_HANDLE;
				oldswpchn1->Image = VK_NULL_HANDLE;
				oldswpchn1->ImageView = VK_NULL_HANDLE;
				GFXContentManager->Delete_Texture(oldswpchn0, false);
				GFXContentManager->Delete_Texture(oldswpchn1, false);

				VKWINDOW->LASTHEIGHT = VKWINDOW->NEWHEIGHT;
				VKWINDOW->LASTWIDTH = VKWINDOW->NEWWIDTH;
				VKWINDOW->Swapchain_Textures[0] = swpchntextures[0];
				VKWINDOW->Swapchain_Textures[1] = swpchntextures[1];
				VKWINDOW->Window_SwapChain = swpchn;
				VKWINDOW->CurrentFrameSWPCHNIndex = 0;
				LOG_NOTCODED_TAPI("Destroy swapchain mechanism should destroy last swapchain textures next frame because of RTSlotSet compatibility testing!", false);
				VKWINDOW->resize_cb(VKWINDOW, VKWINDOW->UserPTR, VKWINDOW->NEWWIDTH, VKWINDOW->NEWHEIGHT, VKWINDOW->Swapchain_Textures);
				delete swpchntextures;
			}
		}
	}

	//CODE ALL OF THE BELOW FUNCTIONS!!!!
	void Vulkan_Core::Change_Window_Resolution(GFX_API::GFXHandle WindowHandle, unsigned int width, unsigned int height) {
		LOG_NOTCODED_TAPI("VulkanCore: Change_Window_Resolution isn't coded!", true);
	}
	void Vulkan_Core::Window_ResizeCallback(GLFWwindow* window, int WIDTH, int HEIGHT) {
		WINDOW* VKWINDOW = (WINDOW*)glfwGetWindowUserPointer(window);
		Vulkan_Core* VKCORE = ((Vulkan_Core*)GFX);
		GPU* VKGPU = (GPU*)VKCORE->VK_States.GPU_TO_RENDER;

		VKWINDOW->NEWWIDTH = WIDTH;
		VKWINDOW->NEWHEIGHT = HEIGHT;
		VKWINDOW->isResized = true;
		VKCORE->isAnyWindowResized = true;
	}
	void Vulkan_Core::Check_Errors() {
		LOG_NOTCODED_TAPI("VulkanCore: Check_Errors isn't coded!", true);
	}
}