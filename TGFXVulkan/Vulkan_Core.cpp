#define GFX_BACKENDBUILD
#include "Vulkan_Core.h"
#define LOG VKCORE->TGFXCORE.DebugCallback

//CODE THESE FUNCTIONs PROPERLY AFTER REFACTORING ENDs

void Vulkan_Core::TGFXDebugCallback(tgfx_result Result, const char* Text) {

}
void Vulkan_Core::TGFXDebugCallback_Threaded(unsigned char ThreadIndex, tgfx_result Result, const char* Text) {

}

Vulkan_Core* VKCORE = nullptr;
Renderer* VKRENDERER = nullptr;
vk_gpudatamanager* VKContentManager = nullptr;
IMGUI_VK* VK_IMGUI = nullptr;
GPU* RENDERGPU = nullptr;
tapi_threadingsystem JobSys = nullptr;
VK_SemaphoreSystem* SEMAPHORESYS = nullptr;

void* StartBackend_returnTGFXCore() {
	new Vulkan_Core(nullptr);
	return &VKCORE->TGFXCORE;
}


#if _WIN32 || _WIN64
#define VK_DLLEXPORTER __declspec(dllexport)

#elif defined(__linux__)
#define VK_DLLEXPORTER __attribute__((visibility("default")))
#endif

//Start Vulkan backend
#ifdef TAPI_THREADING
VK_DLLEXPORTER tgfx_core* Load_TGFXBackend_Threaded(tapi_threadingsystem Threader) {
	VKCORE = new Vulkan_Core(nullptr);
	return &VKCORE->TGFXCORE;
}
#endif

void GFX_Error_Callback(int error_code, const char* description) {
	LOG(tgfx_result_FAIL, description);
}
inline void Save_Monitors();
inline void Create_Instance();
inline void Setup_Debugging();
inline void Check_Computer_Specs();
Vulkan_Core::Vulkan_Core(tapi_threadingsystem JobSystem) : TGFXCORE({}) {
	void* INVALID = new char;
	memcpy(&TGFXCORE.INVALIDHANDLE, &INVALID, sizeof(void*));
	std::cout << "INVALID: " << INVALID << std::endl;
	VKCORE = this;
	//Set function pointers of the user API
	VKCORE->TGFXCORE.Change_Window_Resolution = Vulkan_Core::ChangeWindowResolution;
	VKCORE->TGFXCORE.CreateWindow = Vulkan_Core::CreateWindow;
	VKCORE->TGFXCORE.DebugCallback = Vulkan_Core::TGFXDebugCallback;
	VKCORE->TGFXCORE.DebugCallback_Threaded = Vulkan_Core::TGFXDebugCallback_Threaded;
	VKCORE->TGFXCORE.Destroy_GFX_Resources = Vulkan_Core::Destroy_GFX_Resources;
	VKCORE->TGFXCORE.Initialize_SecondStage = Vulkan_Core::Initialize_SecondStage;
	VKCORE->TGFXCORE.Take_Inputs = Vulkan_Core::Take_Inputs;
	VKCORE->TGFXCORE.GetMonitorList = Vulkan_Core::GetMonitorList;
	VKCORE->TGFXCORE.GetGPUList = Vulkan_Core::GetGPUList;
	SetGFXHelperFunctions();


	//Set error callback to handle all glfw errors (including initialization error)!
	glfwSetErrorCallback(GFX_Error_Callback);

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	Save_Monitors();

	Create_Instance();
#ifdef VULKAN_DEBUGGING
	Setup_Debugging();
#endif
	Check_Computer_Specs();
}


void Vulkan_Core::Initialize_SecondStage(tgfx_initializationsecondstageinfo info) {
	InitializationSecondStageInfo* vkinfo = (InitializationSecondStageInfo*)info;
	RENDERGPU = vkinfo->RENDERERGPU;

	if (vkinfo->shouldActivate_DearIMGUI) {
		VK_IMGUI = new IMGUI_VK;
	}


	VKRENDERER = new Renderer;
	LOG(tgfx_result_SUCCESS, "Before creating GPU ContentManager!");
	VKContentManager = new vk_gpudatamanager(*vkinfo);

	delete vkinfo;


	LOG(tgfx_result_SUCCESS, "VulkanCore: Vulkan systems are started!");
}
Vulkan_Core::~Vulkan_Core() {
	Destroy_GFX_Resources();
}

void Save_Monitors() {
	VKCORE->MONITORs.clear();
	int monitor_count;
	GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
	LOG(tgfx_result_SUCCESS, ("VulkanCore: " + std::to_string(monitor_count) + " number of monitor(s) detected!").c_str());
	for (unsigned int i = 0; i < monitor_count; i++) {
		GLFWmonitor* monitor = monitors[i];

		//Get monitor name provided by OS! It is a driver based name, so it maybe incorrect!
		const char* monitor_name = glfwGetMonitorName(monitor);
		VKCORE->MONITORs.push_back(new vkMONITOR());
		vkMONITOR* Monitor = VKCORE->MONITORs[VKCORE->MONITORs.size() - 1];
		Monitor->NAME = monitor_name;

		//Get videomode to detect at which resolution the OS is using the monitor
		const GLFWvidmode* monitor_vid_mode = glfwGetVideoMode(monitor);
		Monitor->WIDTH = monitor_vid_mode->width;
		Monitor->HEIGHT = monitor_vid_mode->height;
		Monitor->COLOR_BITES = monitor_vid_mode->blueBits;
		Monitor->REFRESH_RATE = monitor_vid_mode->refreshRate;

		//Get monitor's physical size, developer may want to use it!
		int PHYSICALWIDTH, PHYSICALHEIGHT;
		glfwGetMonitorPhysicalSize(monitor, &PHYSICALWIDTH, &PHYSICALHEIGHT);
		if (PHYSICALWIDTH == 0 || PHYSICALHEIGHT == 0) {
			LOG(tgfx_result_WARNING, "One of the monitors have invalid physical sizes, please be careful");
		}
	}
}




inline bool IsExtensionSupported(const char* ExtensionName, VkExtensionProperties* SupportedExtensions, unsigned int SupportedExtensionsCount);
inline void Check_InstanceExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	for (unsigned int i = 0; i < glfwExtensionCount; i++) {
		if (!IsExtensionSupported(glfwExtensions[i], Supported_InstanceExtensionList.data(), Supported_InstanceExtensionList.size())) {
			LOG(tgfx_result_INVALIDARGUMENT, "Your vulkan instance doesn't support extensions that're required by GLFW. This situation is not tested, so report your device to the author!");
			return;
		}
		Active_InstanceExtensionNames.push_back(glfwExtensions[i]);
	}


	if (IsExtensionSupported(VK_KHR_SURFACE_EXTENSION_NAME, Supported_InstanceExtensionList.data(), Supported_InstanceExtensionList.size())) {
		isActive_SurfaceKHR = true;
		Active_InstanceExtensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	}
	else {
		LOG(tgfx_result_WARNING, "Your Vulkan instance doesn't support to display a window, so you shouldn't use any window related functionality such as: GFXRENDERER->Create_WindowPass, GFX->Create_Window, GFXRENDERER->Swap_Buffers ...");
	}


#ifdef VULKAN_DEBUGGING
	if (IsExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, Supported_InstanceExtensionList.data(), Supported_InstanceExtensionList.size())) {
		Active_InstanceExtensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
#endif


	//Check isActive_GetPhysicalDeviceProperties2KHR
	if ((Application_Info.apiVersion == VK_API_VERSION_1_0 &&
		IsExtensionSupported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, Supported_InstanceExtensionList.data(), Supported_InstanceExtensionList.size())
		) || Application_Info.apiVersion != VK_API_VERSION_1_0) {
		isSupported_PhysicalDeviceProperties2 = true;
		if (Application_Info.apiVersion == VK_API_VERSION_1_0) {
			Active_InstanceExtensionNames.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		}
	}
}

void Create_Instance() {
	//APPLICATION INFO
	VkApplicationInfo App_Info = {};
	App_Info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	App_Info.pApplicationName = "Vulkan DLL";
	App_Info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	App_Info.pEngineName = "GFX API";
	App_Info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	App_Info.apiVersion = VK_API_VERSION_1_2;

	//CHECK SUPPORTED EXTENSIONs
	uint32_t extension_count;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> SupportedExtensions;
	//Doesn't construct VkExtensionProperties object, so we have to use resize!
	SupportedExtensions.resize(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, SupportedExtensions.data());
	for (unsigned int i = 0; i < extension_count; i++) {
		Supported_InstanceExtensionList.push_back(SupportedExtensions[i]);
	}
	Check_InstanceExtensions();

	//CHECK SUPPORTED LAYERS
	unsigned int Supported_LayerNumber = 0;
	vkEnumerateInstanceLayerProperties(&Supported_LayerNumber, nullptr);
	VkLayerProperties* Supported_LayerList = new VkLayerProperties[Supported_LayerNumber];
	vkEnumerateInstanceLayerProperties(&Supported_LayerNumber, Supported_LayerList);

	//INSTANCE CREATION INFO
	VkInstanceCreateInfo InstCreation_Info = {};
	InstCreation_Info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	InstCreation_Info.pApplicationInfo = &App_Info;

	//Extensions
	InstCreation_Info.enabledExtensionCount = Active_InstanceExtensionNames.size();
	InstCreation_Info.ppEnabledExtensionNames = Active_InstanceExtensionNames.data();

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

	if (vkCreateInstance(&InstCreation_Info, nullptr, &Vulkan_Instance) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "Failed to create a Vulkan Instance!");
	}
	LOG(tgfx_result_SUCCESS, "Vulkan Instance is created successfully!");

}



VKAPI_ATTR VkBool32 VKAPI_CALL VK_DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Message_Severity, VkDebugUtilsMessageTypeFlagsEXT Message_Type, const VkDebugUtilsMessengerCallbackDataEXT* pCallback_Data, void* pUserData);
PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT();
PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT();
VkDebugUtilsMessengerEXT Debug_Messenger = VK_NULL_HANDLE;
void Setup_Debugging() {
	VkDebugUtilsMessengerCreateInfoEXT DebugMessenger_CreationInfo = {};
	DebugMessenger_CreationInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	DebugMessenger_CreationInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	DebugMessenger_CreationInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	DebugMessenger_CreationInfo.pfnUserCallback = VK_DebugCallback;
	DebugMessenger_CreationInfo.pNext = nullptr;
	DebugMessenger_CreationInfo.pUserData = nullptr;

	auto func = vkCreateDebugUtilsMessengerEXT();
	if (func(Vulkan_Instance, &DebugMessenger_CreationInfo, nullptr, &Debug_Messenger) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "Vulkan's Debug Callback system failed to start!");
	}
	LOG(tgfx_result_SUCCESS, "Vulkan Debug Callback system is started!");
}



inline void Check_Computer_Specs() {
	LOG(tgfx_result_SUCCESS, "Started to check Computer Specifications!");

	//CHECK GPUs
	uint32_t GPU_NUMBER = 0;
	vkEnumeratePhysicalDevices(Vulkan_Instance, &GPU_NUMBER, nullptr);
	std::vector<VkPhysicalDevice> Physical_GPU_LIST;
	for (unsigned int i = 0; i < GPU_NUMBER; i++) {
		Physical_GPU_LIST.push_back(VkPhysicalDevice());
	}
	vkEnumeratePhysicalDevices(Vulkan_Instance, &GPU_NUMBER, Physical_GPU_LIST.data());

	if (GPU_NUMBER == 0) {
		LOG(tgfx_result_FAIL, "There is no GPU that has Vulkan support! Updating your drivers or Upgrading the OS may help");
	}

	//GET GPU INFORMATIONs, QUEUE FAMILIES etc
	for (unsigned int i = 0; i < GPU_NUMBER; i++) {
		//GPU initializer handles everything else
		GPU* VKGPU = new GPU(Physical_GPU_LIST[i]);
	}

	LOG(tgfx_result_SUCCESS, "Finished checking Computer Specifications!");
}




bool Vulkan_Core::Create_WindowSwapchain(WINDOW* Vulkan_Window, unsigned int WIDTH, unsigned int HEIGHT, VkSwapchainKHR* SwapchainOBJ, VK_Texture** SwapchainTextureHandles) {
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
		LOG(tgfx_result_NOTCODED, "VulkanCore: Window Surface Capabilities have issues, maxImageCount <= minImageCount!");
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
	//Swapchain texture can be used as framebuffer, but we should set its bit!
	if (Vulkan_Window->SWAPCHAINUSAGE & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
		Vulkan_Window->SWAPCHAINUSAGE &= ~(1UL << 5);
	}
	swpchn_ci.imageUsage = Vulkan_Window->SWAPCHAINUSAGE;
	
	swpchn_ci.clipped = VK_TRUE;
	swpchn_ci.preTransform = Vulkan_Window->SurfaceCapabilities.currentTransform;
	swpchn_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if (Vulkan_Window->Window_SwapChain) {
		swpchn_ci.oldSwapchain = Vulkan_Window->Window_SwapChain;
	}
	else {
		swpchn_ci.oldSwapchain = nullptr;
	}

	if (RENDERGPU->QUEUEFAMSCOUNT() > 1) {
		swpchn_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	}
	else {
		swpchn_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	swpchn_ci.pQueueFamilyIndices = RENDERGPU->ALLQUEUEFAMILIES();
	swpchn_ci.queueFamilyIndexCount = RENDERGPU->QUEUEFAMSCOUNT();


	if (vkCreateSwapchainKHR(RENDERGPU->LOGICALDEVICE(), &swpchn_ci, nullptr, SwapchainOBJ) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "VulkanCore: Failed to create a SwapChain for a Window");
		return false;
	}

	//Get Swapchain images
	uint32_t created_imagecount = 0;
	vkGetSwapchainImagesKHR(RENDERGPU->LOGICALDEVICE(), *SwapchainOBJ, &created_imagecount, nullptr);
	VkImage* SWPCHN_IMGs = new VkImage[created_imagecount];
	vkGetSwapchainImagesKHR(RENDERGPU->LOGICALDEVICE(), *SwapchainOBJ, &created_imagecount, SWPCHN_IMGs);
	if (created_imagecount < 2) {
		LOG(tgfx_result_FAIL, "TGFX API asked for 2 swapchain textures but Vulkan driver gave less number of textures!");
		return false;
	}
	else if (created_imagecount > 2) { 
		LOG(tgfx_result_SUCCESS, "TGFX API asked for 2 swapchain textures but Vulkan driver gave more than that, so GFX API only used 2 of them!");
	}
	for (unsigned int vkim_index = 0; vkim_index < 2; vkim_index++) {
		VK_Texture* SWAPCHAINTEXTURE = new VK_Texture;
		SWAPCHAINTEXTURE->CHANNELs = tgfx_texture_channels_BGRA8UNORM;
		SWAPCHAINTEXTURE->WIDTH = WIDTH;
		SWAPCHAINTEXTURE->HEIGHT = HEIGHT;
		SWAPCHAINTEXTURE->DATA_SIZE = SWAPCHAINTEXTURE->WIDTH * SWAPCHAINTEXTURE->HEIGHT * 4;
		SWAPCHAINTEXTURE->Image = SWPCHN_IMGs[vkim_index];
		SWAPCHAINTEXTURE->MIPCOUNT = 1;

		SwapchainTextureHandles[vkim_index] = SWAPCHAINTEXTURE;
	}
	delete SWPCHN_IMGs;

	if (Vulkan_Window->SWAPCHAINUSAGE) {
		for (unsigned int i = 0; i < 2; i++) {
			VkImageViewCreateInfo ImageView_ci = {};
			ImageView_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			VK_Texture* SwapchainTexture = SwapchainTextureHandles[i];
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

			if (vkCreateImageView(RENDERGPU->LOGICALDEVICE(), &ImageView_ci, nullptr, &SwapchainTexture->ImageView) != VK_SUCCESS) {
				LOG(tgfx_result_FAIL, "VulkanCore: Image View creation has failed!");
				return false;
			}
		}
	}
	return true;
}
void GLFWwindowresizecallback(GLFWwindow* glfwwindow, int width, int height) {
	WINDOW* vkwindow = (WINDOW*)glfwGetWindowUserPointer(glfwwindow);
	vkwindow->resize_cb((tgfx_window)vkwindow, vkwindow->UserPTR, width, height, (tgfx_texture*)vkwindow->Swapchain_Textures);
}
void Vulkan_Core::CreateWindow(unsigned int WIDTH, unsigned int HEIGHT, tgfx_monitor monitor,
	tgfx_windowmode Mode, const char* NAME, tgfx_textureusageflag SwapchainUsage, tgfx_windowResizeCallback ResizeCB, void* UserPointer,
	tgfx_texture* SwapchainTextureHandles, tgfx_window* window) {
	LOG(tgfx_result_SUCCESS, "Window creation has started!");

	if (ResizeCB) {
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	}
	//Create window as it will share resources with Renderer Context to get display texture!
	GLFWwindow* glfw_window = glfwCreateWindow(WIDTH, HEIGHT, NAME, NULL, nullptr);
	WINDOW* Vulkan_Window = new WINDOW;
	Vulkan_Window->LASTWIDTH = WIDTH;
	Vulkan_Window->LASTHEIGHT = HEIGHT;
	Vulkan_Window->DISPLAYMODE = Mode;
	Vulkan_Window->MONITOR = (vkMONITOR*)monitor;
	Vulkan_Window->NAME = NAME;
	Vulkan_Window->GLFW_WINDOW = glfw_window;
	Vulkan_Window->SWAPCHAINUSAGE = *(VkImageUsageFlags*)SwapchainUsage;
	delete SwapchainUsage;
	Vulkan_Window->resize_cb = ResizeCB;
	Vulkan_Window->UserPTR = UserPointer;
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	//Check and Report if GLFW fails
	if (glfw_window == NULL) {
		LOG(tgfx_result_FAIL, "VulkanCore: We failed to create the window because of GLFW!");
		delete Vulkan_Window;
		return;
	}

	//Window VulkanSurface Creation
	VkSurfaceKHR Window_Surface = {};
	if (glfwCreateWindowSurface(Vulkan_Instance, Vulkan_Window->GLFW_WINDOW, nullptr, &Window_Surface) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "GLFW failed to create a window surface");
		return;
	}
	else {
		LOG(tgfx_result_SUCCESS, "GLFW created a window surface!");
	}
	Vulkan_Window->Window_Surface = Window_Surface;

	if (ResizeCB) {
		glfwSetWindowUserPointer(Vulkan_Window->GLFW_WINDOW, Vulkan_Window);
		glfwSetWindowSizeCallback(Vulkan_Window->GLFW_WINDOW, GLFWwindowresizecallback);
	}


	//Finding GPU_TO_RENDER's Surface Capabilities

	if (!RENDERGPU->isWindowSupported(Vulkan_Window->Window_Surface)) {
		LOG(tgfx_result_FAIL, "Vulkan backend supports windows that your GPU supports but your GPU doesn't support current window. So window creation has failed!");
		return;
	}
	
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(RENDERGPU->PHYSICALDEVICE(), Vulkan_Window->Window_Surface, &Vulkan_Window->SurfaceCapabilities);
	uint32_t FormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(RENDERGPU->PHYSICALDEVICE(), Vulkan_Window->Window_Surface, &FormatCount, nullptr);
	Vulkan_Window->SurfaceFormats.resize(FormatCount);
	if (FormatCount != 0) {
		vkGetPhysicalDeviceSurfaceFormatsKHR(RENDERGPU->PHYSICALDEVICE(), Vulkan_Window->Window_Surface, &FormatCount, Vulkan_Window->SurfaceFormats.data());
	}
	else {
		LOG(tgfx_result_FAIL, "This GPU doesn't support this type of windows, please try again with a different window configuration!");
		delete Vulkan_Window;
		return;
	}

	uint32_t PresentationModesCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(RENDERGPU->PHYSICALDEVICE(), Vulkan_Window->Window_Surface, &PresentationModesCount, nullptr);
	Vulkan_Window->PresentationModes.resize(PresentationModesCount);
	if (PresentationModesCount != 0) {
		vkGetPhysicalDeviceSurfacePresentModesKHR(RENDERGPU->PHYSICALDEVICE(), Vulkan_Window->Window_Surface, &PresentationModesCount, Vulkan_Window->PresentationModes.data());
	}

	VK_Texture* SWPCHNTEXTUREHANDLESVK[2];
	if (!VKCORE->Create_WindowSwapchain(Vulkan_Window, Vulkan_Window->LASTWIDTH, Vulkan_Window->LASTHEIGHT, &Vulkan_Window->Window_SwapChain, SWPCHNTEXTUREHANDLESVK)) {
		LOG(tgfx_result_FAIL, "Window's swapchain creation has failed, so window's creation!");
		vkDestroySurfaceKHR(Vulkan_Instance, Vulkan_Window->Window_Surface, nullptr);
		glfwDestroyWindow(Vulkan_Window->GLFW_WINDOW);
		delete Vulkan_Window;
	}
	Vulkan_Window->Swapchain_Textures[0] = SWPCHNTEXTUREHANDLESVK[0];
	Vulkan_Window->Swapchain_Textures[1] = SWPCHNTEXTUREHANDLESVK[1];
	SwapchainTextureHandles[0] = (tgfx_texture)Vulkan_Window->Swapchain_Textures[0];
	SwapchainTextureHandles[1] = (tgfx_texture)Vulkan_Window->Swapchain_Textures[1];

	//Create presentation wait semaphores
	//We are creating 2 semaphores because if 2 frames combined is faster than vertical blank, there is tearing!
	//Previously, this was 3 but I moved command buffer waiting so there is no command buffer collision with display
	for (unsigned char SemaphoreIndex = 0; SemaphoreIndex < 2; SemaphoreIndex++) {
		Vulkan_Window->PresentationSemaphores[SemaphoreIndex] = SEMAPHORESYS->Create_Semaphore().Get_ID();
	}


	LOG(tgfx_result_SUCCESS, "Window creation is successful!");
	VKCORE->WINDOWs.push_back(Vulkan_Window);
}

//Destroy Operations

void Vulkan_Core::Destroy_GFX_Resources() {
	vkDeviceWaitIdle(RENDERGPU->LOGICALDEVICE());
	//Destroy dear IMGUI
	VK_IMGUI->Destroy_IMGUIResources();
	VkDescriptorPool IMGUIPOOL = VKRENDERER->IMGUIPOOL;
	delete VKRENDERER;	VKRENDERER = nullptr;
	delete VKContentManager; VKContentManager = nullptr;
	delete VK_IMGUI; VK_IMGUI = nullptr;
	vkDestroyDescriptorPool(RENDERGPU->LOGICALDEVICE(), IMGUIPOOL, nullptr);

	std::vector<VkSurfaceKHR> Destroy_SurfaceOBJs;
	//Close windows and delete related datas
	for (unsigned int WindowIndex = 0; WindowIndex < VKCORE->WINDOWs.size(); WindowIndex++) {
		WINDOW* window = VKCORE->WINDOWs[WindowIndex];
		VK_Texture* Texture0 = window->Swapchain_Textures[0];
		VK_Texture* Texture1 = window->Swapchain_Textures[1];
		vkDestroyImageView(RENDERGPU->LOGICALDEVICE(), Texture0->ImageView, nullptr);
		vkDestroyImageView(RENDERGPU->LOGICALDEVICE(), Texture1->ImageView, nullptr);

		vkDestroySwapchainKHR(RENDERGPU->LOGICALDEVICE(), window->Window_SwapChain, nullptr);
		Destroy_SurfaceOBJs.push_back(window->Window_Surface);
		delete window;
	}

	//Free the allocated memories
	for (unsigned int AllocIndex = 0; AllocIndex < RENDERGPU->ALLOCS().size(); AllocIndex++) {
		const VK_MemoryAllocation& Alloc = RENDERGPU->ALLOCS()[AllocIndex];
		if (Alloc.ALLOCATIONSIZE) {
			vkDestroyBuffer(RENDERGPU->LOGICALDEVICE(), Alloc.Buffer, nullptr);
			vkFreeMemory(RENDERGPU->LOGICALDEVICE(), Alloc.Allocated_Memory, nullptr);
		}
	}

	//GPU deleting
	for (unsigned int i = 0; i < VKCORE->DEVICE_GPUs.size(); i++) {
		GPU* a_RENDERGPU = VKCORE->DEVICE_GPUs[i];
		delete[] a_RENDERGPU->ALLQUEUEFAMILIES();
		delete[] a_RENDERGPU->QUEUEFAMS();

		vkDestroyDevice(a_RENDERGPU->LOGICALDEVICE(), nullptr);
	}
	if (Debug_Messenger != VK_NULL_HANDLE) {
		vkDestroyDebugUtilsMessengerEXT()(Vulkan_Instance, Debug_Messenger, nullptr);
	}

	for (unsigned int SurfaceOBJIndex = 0; SurfaceOBJIndex < Destroy_SurfaceOBJs.size(); SurfaceOBJIndex++) {
		vkDestroySurfaceKHR(Vulkan_Instance, Destroy_SurfaceOBJs[SurfaceOBJIndex], nullptr);
	}

	vkDestroyInstance(Vulkan_Instance, nullptr);

	glfwTerminate();


	LOG(tgfx_result_SUCCESS, "Vulkan Resources are destroyed!");
}

//Input (Keyboard-Controller) Operations
void Vulkan_Core::Take_Inputs() {
	glfwPollEvents();
	if (VKCORE->isAnyWindowResized) {
		vkDeviceWaitIdle(RENDERGPU->LOGICALDEVICE());
		for (unsigned int WindowIndex = 0; WindowIndex < VKCORE->WINDOWs.size(); WindowIndex++) {
			WINDOW* VKWINDOW = VKCORE->WINDOWs[WindowIndex];
			if (!VKWINDOW->isResized) {
				continue;
			}
			if (VKWINDOW->NEWWIDTH == VKWINDOW->LASTWIDTH && VKWINDOW->NEWHEIGHT == VKWINDOW->LASTHEIGHT) {
				VKWINDOW->isResized = false;
				continue;
			}

			VkSwapchainKHR swpchn;
			VK_Texture* swpchntextures[2]{ new VK_Texture(), new VK_Texture() };
			//If new window size isn't able to create a swapchain, return to last window size
			if (!VKCORE->Create_WindowSwapchain(VKWINDOW, VKWINDOW->NEWWIDTH, VKWINDOW->NEWHEIGHT, &swpchn, swpchntextures)) {
				LOG(tgfx_result_FAIL, "New size for the window is not possible, returns to the last successful size!");
				glfwSetWindowSize(VKWINDOW->GLFW_WINDOW, VKWINDOW->LASTWIDTH, VKWINDOW->LASTHEIGHT);
				VKWINDOW->isResized = false;
				delete swpchntextures[0]; delete swpchntextures[1];
				continue;
			}

			VK_Texture* oldswpchn0 = VKWINDOW->Swapchain_Textures[0];
			VK_Texture* oldswpchn1 = VKWINDOW->Swapchain_Textures[1];

			vkDestroyImageView(RENDERGPU->LOGICALDEVICE(), oldswpchn0->ImageView, nullptr);
			vkDestroyImageView(RENDERGPU->LOGICALDEVICE(), oldswpchn1->ImageView, nullptr);
			vkDestroySwapchainKHR(RENDERGPU->LOGICALDEVICE(), VKWINDOW->Window_SwapChain, nullptr);
			oldswpchn0->Image = VK_NULL_HANDLE;
			oldswpchn0->ImageView = VK_NULL_HANDLE;
			oldswpchn1->Image = VK_NULL_HANDLE;
			oldswpchn1->ImageView = VK_NULL_HANDLE;
			VKCORE->TGFXCORE.ContentManager.Delete_Texture((tgfx_texture)oldswpchn0, false);
			VKCORE->TGFXCORE.ContentManager.Delete_Texture((tgfx_texture)oldswpchn1, false);

			VKWINDOW->LASTHEIGHT = VKWINDOW->NEWHEIGHT;
			VKWINDOW->LASTWIDTH = VKWINDOW->NEWWIDTH;
			//When you resize window at Frame1, user'd have to track swapchain texture state if I don't do this here
			//So please don't touch!
			VKWINDOW->Swapchain_Textures[0] = VKRENDERER->GetCurrentFrameIndex() ? swpchntextures[1] : swpchntextures[0];
			VKWINDOW->Swapchain_Textures[1] = VKRENDERER->GetCurrentFrameIndex() ? swpchntextures[0] : swpchntextures[1];
			VKWINDOW->Window_SwapChain = swpchn;
			VKWINDOW->CurrentFrameSWPCHNIndex = 0;
			VKWINDOW->resize_cb((tgfx_window)VKWINDOW, VKWINDOW->UserPTR, VKWINDOW->NEWWIDTH, VKWINDOW->NEWHEIGHT, (tgfx_texture*)VKWINDOW->Swapchain_Textures);
			delete swpchntextures;
		}
	}
}


//NOTCODED!
void Vulkan_Core::GetMonitorList(tgfx_monitor_list* List) {
	(*List) = (tgfx_monitor*)new tgfx_monitor*[VKCORE->MONITORs.size() + 1]{ NULL };
	for (unsigned int i = 0; i < VKCORE->MONITORs.size(); i++) {
		(*List)[i] = (tgfx_monitor)VKCORE->MONITORs[i];
	}
}
void Vulkan_Core::ChangeWindowResolution(tgfx_window WindowHandle, unsigned int width, unsigned int height) {
	LOG(tgfx_result_NOTCODED, "VulkanCore: Change_Window_Resolution isn't coded!");
}
void Vulkan_Core::GetGPUList(tgfx_gpu_list* List) {
	*List = new tgfx_gpu[VKCORE->DEVICE_GPUs.size() + 1];
	for (unsigned int i = 0; i < VKCORE->DEVICE_GPUs.size(); i++) {
		(*List)[i] = (tgfx_gpu)VKCORE->DEVICE_GPUs[i];
	}
	(*List)[VKCORE->DEVICE_GPUs.size()] = (tgfx_gpu)VKCORE->TGFXCORE.INVALIDHANDLE;
}



VKAPI_ATTR VkBool32 VKAPI_CALL VK_DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Message_Severity, VkDebugUtilsMessageTypeFlagsEXT Message_Type, const VkDebugUtilsMessengerCallbackDataEXT* pCallback_Data, void* pUserData) {
	std::string Callback_Type = "";
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
		LOG(tgfx_result_FAIL, "Vulkan Callback has returned a unsupported Message_Type");
		return true;
		break;
	}

	switch (Message_Severity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		LOG(tgfx_result_SUCCESS, pCallback_Data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		LOG(tgfx_result_WARNING, pCallback_Data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		LOG(tgfx_result_FAIL, pCallback_Data->pMessage);
		break;
	default:
		LOG(tgfx_result_FAIL, "Vulkan Callback has returned a unsupported debug message type!");
		return true;
	}
	return false;
}
PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT() {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Vulkan_Instance, "vkCreateDebugUtilsMessengerEXT");
	if (func == nullptr) {
		LOG(tgfx_result_FAIL, "(Vulkan failed to load vkCreateDebugUtilsMessengerEXT function!");
		return nullptr;
	}
	return func;
}
PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT() {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Vulkan_Instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func == nullptr) {
		LOG(tgfx_result_FAIL, "(Vulkan failed to load vkDestroyDebugUtilsMessengerEXT function!");
		return nullptr;
	}
	return func;
}