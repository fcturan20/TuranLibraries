#include "Vulkan_Core.h"
#include "TuranAPI/Logger_Core.h"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720


namespace Vulkan {
	Vulkan_Core::Vulkan_Core() {
		//Set static GFX_API variable as created Vulkan_Core, because there will only one GFX_API in run-time
		//And we will use this SELF to give commands to GFX_API in window callbacks
		SELF = this;
		LOG_STATUS_TAPI("VulkanCore: Vulkan systems are starting!");

		//Set error callback to handle all glfw errors (including initialization error)!
		glfwSetErrorCallback(Vulkan_Core::GFX_Error_Callback);

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		Save_Monitors();

		//Note that: Vulkan initialization need a Window to be created before, so we should create one with GLFW
		Create_MainWindow();
		Initialization();

		GFXRENDERER = new Vulkan::Renderer;
		ContentManager = new Vulkan::GPU_ContentManager;
		LOG_NOTCODED_TAPI("VulkanCore: Vulkan's IMGUI support isn't coded!\n", false);

		LOG_STATUS_TAPI("VulkanCore: Vulkan systems are started!");
	}
	Vulkan_Core::~Vulkan_Core() {
		Destroy_GFX_Resources();
	}

	void Vulkan_Core::Create_MainWindow() {
		LOG_STATUS_TAPI("VulkanCore: Creating a window named");

		//Create window as it will share resources with Renderer Context to get display texture!
		GLFWwindow* glfw_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Window", NULL, nullptr);
		WINDOW* Vulkan_Window = new WINDOW(WINDOW_WIDTH, WINDOW_HEIGHT, GFX_API::WINDOW_MODE::WINDOWED, &CONNECTED_Monitors[0], CONNECTED_Monitors[0].REFRESH_RATE, "Vulkan Window", GFX_API::V_SYNC::VSYNC_OFF);
		//glfwSetWindowMonitor(glfw_window, NULL, 0, 0, Vulkan_Window->Get_Window_Mode().x, Vulkan_Window->Get_Window_Mode().y, Vulkan_Window->Get_Window_Mode().z);
		Vulkan_Window->GLFW_WINDOW = glfw_window;

		//Check and Report if GLFW fails
		if (glfw_window == NULL) {
			LOG_CRASHING_TAPI("VulkanCore: We failed to create the window because of GLFW!");
			glfwTerminate();
		}

		Main_Window = Vulkan_Window;
	}

	void Vulkan_Core::Initialization() {
		//GLFW initialized in Vulkan_Core::Vulkan_Core()
		
		Create_Instance();
#ifdef VULKAN_DEBUGGING
		Setup_Debugging();
#endif
		Create_Surface_forWindow();
		Check_Computer_Specs();
		
		Setup_LogicalDevice();
		Create_MainWindow_SwapChain();
	}
	void Vulkan_Core::GFX_Error_Callback(int error_code, const char* description) {
		LOG_CRASHING_TAPI(description, true);
	}
	void Vulkan_Core::Create_Instance() {
		//APPLICATION INFO
		VkApplicationInfo App_Info = {};
		App_Info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		App_Info.pApplicationName = "Vulkan DLL";
		App_Info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		App_Info.pEngineName = "GFX API";
		App_Info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		App_Info.apiVersion = VK_API_VERSION_1_0;
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
			std::cout << "Supported Extension: " << SupportedExtensions[i].extensionName << " is added to the Vector!\n";
		}
		std::cout << "Supported Extension Count: " << extension_count << std::endl;
		VK_States.Is_RequiredInstanceExtensions_Supported();

		//CHECK SUPPORTED LAYERS
		vkEnumerateInstanceLayerProperties(&VK_States.Supported_LayerNumber, nullptr);
		VK_States.Supported_LayerList = new VkLayerProperties[VK_States.Supported_LayerNumber];
		vkEnumerateInstanceLayerProperties(&VK_States.Supported_LayerNumber, VK_States.Supported_LayerList);
		for (unsigned int i = 0; i < VK_States.Supported_LayerNumber; i++) {
			std::cout << VK_States.Supported_LayerList[i].layerName << " layer is supported!\n";
		}

		//INSTANCE CREATION INFO
		VkInstanceCreateInfo InstCreation_Info = {};
		InstCreation_Info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		InstCreation_Info.pApplicationInfo = &App_Info;

		//Extensions
		InstCreation_Info.enabledExtensionCount = VK_States.Required_InstanceExtensionNames.size();
		vector<const char*> ExtensionNames;
		for (unsigned int i = 0; i < VK_States.Required_InstanceExtensionNames.size(); i++) {
			ExtensionNames.push_back(VK_States.Required_InstanceExtensionNames[i]);
			std::cout << "Added an Extension: " << VK_States.Required_InstanceExtensionNames[i] << std::endl;
		}
		InstCreation_Info.ppEnabledExtensionNames = ExtensionNames.data();

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
	void Vulkan_Core::Create_Surface_forWindow() {
		WINDOW* Vulkan_Window = (WINDOW*)Main_Window;
		VkSurfaceKHR Window_Surface = {};
		if (glfwCreateWindowSurface(VK_States.Vulkan_Instance, Vulkan_Window->GLFW_WINDOW, nullptr, &Window_Surface) != VK_SUCCESS) {
			LOG_ERROR_TAPI("GLFW failed to create a window surface");
		}
		else {
			LOG_STATUS_TAPI("GLFW created a window surface!");
		}
		Vulkan_Window->Window_Surface = Window_Surface;
	}
	void Vulkan_Core::Check_Computer_Specs() {
		LOG_STATUS_TAPI("Started to check Computer Specifications!");

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
			GPU* Vulkan_GPU = new GPU;
			Vulkan_GPU->Physical_Device = Physical_GPU_LIST[i];
			vkGetPhysicalDeviceProperties(Vulkan_GPU->Physical_Device, &Vulkan_GPU->Device_Properties);
			vkGetPhysicalDeviceFeatures(Vulkan_GPU->Physical_Device, &Vulkan_GPU->Device_Features);
			const char* VendorName = VK_States.Convert_VendorID_toaString(Vulkan_GPU->Device_Properties.vendorID);

			//SAVE BASIC INFOs TO THE GPU OBJECT
			Vulkan_GPU->MODEL = Vulkan_GPU->Device_Properties.deviceName;
			Vulkan_GPU->DRIVER_VERSION = Vulkan_GPU->Device_Properties.driverVersion;
			Vulkan_GPU->API_VERSION = Vulkan_GPU->Device_Properties.apiVersion;
			Vulkan_GPU->DRIVER_VERSION = Vulkan_GPU->Device_Properties.driverVersion;

			//CHECK IF GPU IS DISCRETE OR INTEGRATED
			switch (Vulkan_GPU->Device_Properties.deviceType) {
			case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				Vulkan_GPU->GPU_TYPE = GFX_API::GPU_TYPEs::DISCRETE_GPU;
				break;
			case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				Vulkan_GPU->GPU_TYPE = GFX_API::GPU_TYPEs::INTEGRATED_GPU;
				break;
			default:
				//const char* CrashingError = Text_Add("Vulkan_Core::Check_Computer_Specs failed to find GPU's Type (Only Discrete and Integrated GPUs supported!), Type is:",
					//std::to_string(Vulkan_GPU->Device_Properties.deviceType).c_str());
				LOG_CRASHING_TAPI("There is an error about GPU!");
				break;
			}

			//GET QUEUE FAMILIES, SAVE THEM TO GPU OBJECT, CHECK AND SAVE GRAPHICS,COMPUTE,TRANSFER QUEUEFAMILIES INDEX
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(Vulkan_GPU->Physical_Device, &queueFamilyCount, nullptr);
			Vulkan_GPU->QueueFamilies.resize(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(Vulkan_GPU->Physical_Device, &queueFamilyCount, Vulkan_GPU->QueueFamilies.data());

			bool is_presentationfound = false;
			for (unsigned int queuefamily_index = 0; queuefamily_index < Vulkan_GPU->QueueFamilies.size(); queuefamily_index++) {
				VkQueueFamilyProperties* QueueFamily = &Vulkan_GPU->QueueFamilies[queuefamily_index];
				VK_QUEUE VKQUEUE;
				VKQUEUE.QueueFamilyIndex = queuefamily_index;
				if (QueueFamily->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					Vulkan_GPU->is_GraphicOperations_Supported = true;
					VKQUEUE.SupportFlag.is_GRAPHICSsupported = true;
				}
				if (QueueFamily->queueFlags & VK_QUEUE_COMPUTE_BIT) {
					Vulkan_GPU->is_ComputeOperations_Supported = true;
					VKQUEUE.SupportFlag.is_COMPUTEsupported = true;
					Vulkan_GPU->COMPUTE_supportedqueuecount++;
				}
				if (QueueFamily->queueFlags & VK_QUEUE_TRANSFER_BIT) {
					Vulkan_GPU->is_TransferOperations_Supported = true;
					VKQUEUE.SupportFlag.is_TRANSFERsupported = true;
					Vulkan_GPU->TRANSFERs_supportedqueuecount++;
				}

				//Check Presentation Support
				if (!is_presentationfound) {
					VkBool32 is_Presentation_Supported;
					vkGetPhysicalDeviceSurfaceSupportKHR(Vulkan_GPU->Physical_Device, queuefamily_index, ((WINDOW*)Main_Window)->Window_Surface, &is_Presentation_Supported);
					if (is_Presentation_Supported) {
						Vulkan_GPU->is_DisplayOperations_Supported = true;
						VKQUEUE.SupportFlag.is_PRESENTATIONsupported = true;
					}
				}

				Vulkan_GPU->QUEUEs.push_back(VKQUEUE);
				if (VKQUEUE.SupportFlag.is_GRAPHICSsupported) {
					Vulkan_GPU->GRAPHICS_QUEUEIndex = Vulkan_GPU->QUEUEs.size() - 1;
				}
				if (VKQUEUE.SupportFlag.is_PRESENTATIONsupported) {
					Vulkan_GPU->DISPLAY_QUEUEIndex = Vulkan_GPU->QUEUEs.size() - 1;
				}
			}

			//Check GPU Surface Capabilities
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Vulkan_GPU->Physical_Device, ((WINDOW*)Main_Window)->Window_Surface, &Vulkan_GPU->SurfaceCapabilities);

			uint32_t FormatCount = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan_GPU->Physical_Device, ((WINDOW*)Main_Window)->Window_Surface, &FormatCount, nullptr);
			Vulkan_GPU->SurfaceFormats.resize(FormatCount);
			if (FormatCount != 0) {
				vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan_GPU->Physical_Device, ((WINDOW*)Main_Window)->Window_Surface, &FormatCount, Vulkan_GPU->SurfaceFormats.data());
			}

			uint32_t PresentationModesCount = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan_GPU->Physical_Device, ((WINDOW*)Main_Window)->Window_Surface, &PresentationModesCount, nullptr);
			Vulkan_GPU->PresentationModes.resize(PresentationModesCount);
			if (PresentationModesCount != 0) {
				vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan_GPU->Physical_Device, ((WINDOW*)Main_Window)->Window_Surface, &PresentationModesCount, Vulkan_GPU->PresentationModes.data());
			}

			vkGetPhysicalDeviceMemoryProperties(Vulkan_GPU->Physical_Device, &Vulkan_GPU->MemoryProperties);

			DEVICE_GPUs.push_back(Vulkan_GPU);
		}
		LOG_STATUS_TAPI("Probably one GPU is detected!");
		if (DEVICE_GPUs.size() == 1) {
			GPU_TO_RENDER = DEVICE_GPUs[0];
			LOG_STATUS_TAPI("The renderer GPU selected as first GPU, because there is only one GPU");
		}
		else {
			LOG_WARNING_TAPI("There are more than one GPUs, please select one to use in rendering operations!");
			std::cout << "GPU index: ";
			int i = 0;
			std::cin >> i;
			while (i >= DEVICE_GPUs.size()) {
				std::cout << "Retry please, GPU index: ";
				int i = 0;
				std::cin >> i;
			}
			GPU_TO_RENDER = DEVICE_GPUs[i];
			LOG_STATUS_TAPI("GPU: " + GPU_TO_RENDER->MODEL + "is selected for rendering operations!");
		}


		LOG_STATUS_TAPI("Finished checking Computer Specifications!");
	}
	void Vulkan_Core::Setup_LogicalDevice() {
		LOG_STATUS_TAPI("Starting to setup logical device");
		GPU* Vulkan_GPU = (GPU*)this->GPU_TO_RENDER;
		//We don't need for now, so leave it empty. But GPU has its own feature list already
		VkPhysicalDeviceFeatures Features = {};

		vector<VkDeviceQueueCreateInfo> QueueCreationInfos;
		//Queue Creation Processes
		float QueuePriority = 1.0f;
		for (unsigned int QueueIndex = 0; QueueIndex < Vulkan_GPU->QUEUEs.size(); QueueIndex++) {
			VK_QUEUE& QUEUE = Vulkan_GPU->QUEUEs[QueueIndex];
			VkDeviceQueueCreateInfo QueueInfo = {};
			QueueInfo.flags = 0;
			QueueInfo.pNext = nullptr;
			QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			QueueInfo.queueFamilyIndex = QUEUE.QueueFamilyIndex;
			QueueInfo.pQueuePriorities = &QueuePriority;
			QueueInfo.queueCount = 1;
			QueueCreationInfos.push_back(QueueInfo);
		}
		if (!Vulkan_GPU->is_GraphicOperations_Supported && !Vulkan_GPU->is_DisplayOperations_Supported) {
			LOG_CRASHING_TAPI("GPU doesn't support Graphics or Display Queue, so logical device creation failed!");
			return;
		}

		VkDeviceCreateInfo Logical_Device_CreationInfo{};
		Logical_Device_CreationInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		Logical_Device_CreationInfo.flags = 0;
		Logical_Device_CreationInfo.pQueueCreateInfos = QueueCreationInfos.data();
		Logical_Device_CreationInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreationInfos.size());
		Logical_Device_CreationInfo.pEnabledFeatures = &Features;
		VK_States.Is_RequiredDeviceExtensions_Supported(Vulkan_GPU);


		Logical_Device_CreationInfo.enabledExtensionCount = Vulkan_GPU->Required_DeviceExtensionNames->size();
		Logical_Device_CreationInfo.ppEnabledExtensionNames = Vulkan_GPU->Required_DeviceExtensionNames->data();

		Logical_Device_CreationInfo.enabledLayerCount = 0;

		if (vkCreateDevice(Vulkan_GPU->Physical_Device, &Logical_Device_CreationInfo, nullptr, &Vulkan_GPU->Logical_Device) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Vulkan failed to create a Logical Device!");
			return;
		}
		LOG_STATUS_TAPI("Vulkan created a Logical Device!");

		for (unsigned int QueueIndex = 0; QueueIndex < Vulkan_GPU->QUEUEs.size(); QueueIndex++) {
			vkGetDeviceQueue(Vulkan_GPU->Logical_Device, Vulkan_GPU->QUEUEs[QueueIndex].QueueFamilyIndex, 0, &Vulkan_GPU->QUEUEs[QueueIndex].Queue);

		}
		LOG_STATUS_TAPI("VulkanCore: Created logical device succesfully!");
	}
	void Vulkan_Core::Create_MainWindow_SwapChain() {
		LOG_STATUS_TAPI("VulkanCore: Started to create SwapChain for GPU according to a Window");
		WINDOW* Vulkan_Window = (WINDOW*)Main_Window;
		GPU* Vulkan_GPU = (GPU*)GPU_TO_RENDER;

		//Choose Surface Format
		VkSurfaceFormatKHR Window_SurfaceFormat = {};
		for (unsigned int i = 0; i < Vulkan_GPU->SurfaceFormats.size(); i++) {
			VkSurfaceFormatKHR& SurfaceFormat = Vulkan_GPU->SurfaceFormats[i];
			if (SurfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && SurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				Window_SurfaceFormat = SurfaceFormat;
			}
		}

		//Choose Surface Presentation Mode
		VkPresentModeKHR Window_PresentationMode = {};
		for (unsigned int i = 0; i < Vulkan_GPU->PresentationModes.size(); i++) {
			VkPresentModeKHR& PresentationMode = Vulkan_GPU->PresentationModes[i];
			if (PresentationMode == VK_PRESENT_MODE_FIFO_KHR) {
				Window_PresentationMode = PresentationMode;
			}
		}


		VkExtent2D Window_ImageExtent = { Vulkan_Window->Get_Window_Mode().x, Vulkan_Window->Get_Window_Mode().y };
		uint32_t image_count = 0;
		if (Vulkan_GPU->SurfaceCapabilities.maxImageCount > Vulkan_GPU->SurfaceCapabilities.minImageCount) {
			image_count = 2;
		}
		else {
			LOG_NOTCODED_TAPI("VulkanCore: Window Surface Capabilities have issues, maxImageCount <= minImageCount!", true);
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
		swpchn_ci.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swpchn_ci.clipped = VK_TRUE;
		swpchn_ci.preTransform = Vulkan_GPU->SurfaceCapabilities.currentTransform;
		swpchn_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swpchn_ci.oldSwapchain = nullptr;

		swpchn_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swpchn_ci.queueFamilyIndexCount = 1;
		swpchn_ci.pQueueFamilyIndices = &Vulkan_GPU->QUEUEs[Vulkan_GPU->DISPLAY_QUEUEIndex].QueueFamilyIndex;


		if (vkCreateSwapchainKHR(Vulkan_GPU->Logical_Device, &swpchn_ci, nullptr, &Vulkan_Window->Window_SwapChain) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("VulkanCore: Failed to create a SwapChain for a Window");
		}
		LOG_STATUS_TAPI("VulkanCore: Finished creating SwapChain for GPU according to a Window");

		//Get Swapchain images
		uint32_t created_imagecount = 0;
		vkGetSwapchainImagesKHR(Vulkan_GPU->Logical_Device, Vulkan_Window->Window_SwapChain, &created_imagecount, nullptr);
		VkImage* SWPCHN_IMGs = new VkImage[created_imagecount];
		vkGetSwapchainImagesKHR(Vulkan_GPU->Logical_Device, Vulkan_Window->Window_SwapChain, &created_imagecount, SWPCHN_IMGs);
		for (unsigned int vkim_index = 0; vkim_index < created_imagecount; vkim_index++) {
			VK_Texture* SWAPCHAINTEXTURE = new VK_Texture;
			SWAPCHAINTEXTURE->CHANNELs = GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8UB;
			SWAPCHAINTEXTURE->HEIGHT = Main_Window->Get_Window_Mode().y;
			SWAPCHAINTEXTURE->WIDTH = Main_Window->Get_Window_Mode().x;
			SWAPCHAINTEXTURE->DATA_SIZE = SWAPCHAINTEXTURE->WIDTH * SWAPCHAINTEXTURE->HEIGHT * 4;
			SWAPCHAINTEXTURE->Image = SWPCHN_IMGs[vkim_index];

			Vulkan_Window->Swapchain_Textures.push_back(SWAPCHAINTEXTURE);
		}

		LOG_STATUS_TAPI("VulkanCore: Finished getting VkImages of Swapchain");

		LOG_STATUS_TAPI("VulkanCore: Started getting VkImageViews of Swapchain");
		for (unsigned int i = 0; i < Vulkan_Window->Swapchain_Textures.size(); i++) {
			VkImageViewCreateInfo ImageView_ci = {};
			ImageView_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			VK_Texture* SwapchainTexture = GFXHandleConverter(VK_Texture*, Vulkan_Window->Swapchain_Textures[i]);
			ImageView_ci.image = SwapchainTexture->Image;
			ImageView_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
			//I'm tired, so set the value manually!
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
			}
			LOG_STATUS_TAPI("VulkanCore: Created an Image View successfully!");
		}
	}

	void Vulkan_Core::Save_Monitors() {
		int monitor_count;
		GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
		LOG_STATUS_TAPI("VulkanCore: " + to_string(monitor_count) + " number of monitor(s) detected!");
		for (unsigned int i = 0; i < monitor_count; i++) {
			GLFWmonitor* monitor = monitors[i];

			//Get monitor name provided by OS! It is a driver based name, so it maybe incorrect!
			const char* monitor_name = glfwGetMonitorName(monitor);
			GFX_API::MONITOR* gfx_monitor = new GFX_API::MONITOR(monitor, monitor_name);

			//Get videomode to detect at which resolution the OS is using the monitor
			const GLFWvidmode* monitor_vid_mode = glfwGetVideoMode(monitor);
			gfx_monitor->Set_Monitor_VidMode(monitor_vid_mode->width, monitor_vid_mode->height, monitor_vid_mode->blueBits, monitor_vid_mode->refreshRate);

			//Get monitor's physical size, developer may want to use it!
			glfwGetMonitorPhysicalSize(monitor, &gfx_monitor->PHYSICAL_WIDTH, &gfx_monitor->PHYSICAL_HEIGHT);

			CONNECTED_Monitors.push_back(gfx_monitor);
		}
	}

	//Destroy Operations

	void Vulkan_Core::Destroy_GFX_Resources() {
		GPU* Vulkan_GPU = (GPU*)GPU_TO_RENDER;
		
		Destroy_SwapchainDependentData();
		WINDOW* VK_WINDOW = (WINDOW*)Main_Window;
		vkDestroySurfaceKHR(VK_States.Vulkan_Instance, VK_WINDOW->Window_Surface, nullptr);

		vkDestroyCommandPool(Vulkan_GPU->Logical_Device, FirstTriangle_CommandPool, nullptr);

		//Shader Module Deleting
		for (size_t i = 0; i < 2; i++) {
			VkShaderModule& ShaderModule = FirstShaderProgram[i];
			vkDestroyShaderModule(Vulkan_GPU->Logical_Device, ShaderModule, nullptr);
		}

		//GPU deleting
		for (unsigned int i = 0; i < DEVICE_GPUs.size(); i++) {
			GPU* a_Vulkan_GPU = (GPU*)DEVICE_GPUs[i];
			vkDestroyDevice(a_Vulkan_GPU->Logical_Device, nullptr);
		}
		VK_States.vkDestroyDebugUtilsMessengerEXT()(VK_States.Vulkan_Instance, VK_States.Debug_Messenger, nullptr);
		vkDestroyInstance(VK_States.Vulkan_Instance, nullptr);

		glfwTerminate();

		LOG_STATUS_TAPI("Vulkan Resources are destroyed!");
	}
	void Vulkan_Core::Destroy_SwapchainDependentData() {
		GPU* Vulkan_GPU = (GPU*)GPU_TO_RENDER;
		WINDOW* VK_WINDOW = (WINDOW*)Main_Window;
		
		vkDestroyCommandPool(Vulkan_GPU->Logical_Device, FirstTriangle_CommandPool, nullptr);
		FirstTriangle_CommandBuffers.clear();

		vkDestroySemaphore(Vulkan_GPU->Logical_Device, Wait_GettingFramebuffer, nullptr);
		vkDestroySemaphore(Vulkan_GPU->Logical_Device, Wait_RenderingFirstTriangle, nullptr);
		for (unsigned int i = 0; i < 3; i++) {
			vkDestroyFence(Vulkan_GPU->Logical_Device, SwapchainFences[i], nullptr);
		}

		vkDestroySwapchainKHR(Vulkan_GPU->Logical_Device, VK_WINDOW->Window_SwapChain, nullptr);
	}

	//Input (Keyboard-Controller) Operations
	void Vulkan_Core::Take_Inputs() {
		LOG_STATUS_TAPI("Take inputs!");
		glfwPollEvents();
	}

	//CODE ALL OF THE BELOW FUNCTIONS!!!!
	void Vulkan_Core::Change_Window_Resolution(unsigned int width, unsigned int height) {
		LOG_NOTCODED_TAPI("VulkanCore: Change_Window_Resolution isn't coded!", true);
	}
	void Vulkan_Core::Window_ResizeCallback(GLFWwindow* window, int WIDTH, int HEIGHT) {
		Vulkan_Core* THIS = (Vulkan_Core*)GFX;
		WINDOW* VK_WINDOW = (WINDOW*)THIS->Main_Window;
		if (((Renderer*)GFXRENDERER)->Is_ConstructingRenderGraph()) {
			LOG_ERROR_TAPI("You can not change window's size while recording rendergraph!");
			return;
		}
		VK_WINDOW->Change_Width_Height(WIDTH, HEIGHT);
	}

	void Vulkan_Core::Swapbuffers_ofMainWindow() {
		glfwSwapBuffers(((WINDOW*)Main_Window)->GLFW_WINDOW);
	}
	void Vulkan_Core::Show_RenderTarget_onWindow(unsigned int RenderTarget_GFXID) {
		LOG_NOTCODED_TAPI("VulkanCore: Show_RenderTarget_onWindow isn't coded!", true);
	}
	void Vulkan_Core::Check_Errors() {
		LOG_NOTCODED_TAPI("VulkanCore: Check_Errors isn't coded!", true);
	}
}