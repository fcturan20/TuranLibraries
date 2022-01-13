#include "predefinitions_vk.h"
#include "includes.h"
#include "core.h"
extern "C" {
	#include <registrysys_tapi.h>
	#include <virtualmemorysys_tapi.h>
	#include <threadingsys_tapi.h>
	#include <tgfx_core.h>
}
#include <stdio.h>
#include <vector>
#include <string>

class core_private{
public:
	std::vector<gpu_vk*> DEVICE_GPUs;
	//Window Operations
	std::vector<monitor_vk*> MONITORs;
	std::vector<window_vk*> WINDOWs;

	//Instead of checking each window each frame, just check this
	bool isAnyWindowResized = false;
};
static core_private* local_core = nullptr;

inline void initialize_secondstage(initializationsecondstageinfo_tgfx_handle info);
//SwapchainTextureHandles should point to an array of 2 elements! Triple Buffering is not supported for now.
inline void createwindow_vk(unsigned int WIDTH, unsigned int HEIGHT, monitor_tgfx_handle monitor, 
    windowmode_tgfx Mode, const char* NAME, textureusageflag_tgfx_handle SwapchainUsage, tgfx_windowResizeCallback ResizeCB, 
    void* UserPointer, texture_tgfx_handle* SwapchainTextureHandles, window_tgfx_handle* window);
inline void change_window_resolution(window_tgfx_handle WindowHandle, unsigned int width, unsigned int height);
inline void getmonitorlist(monitor_tgfx_listhandle* MonitorList);
inline void getGPUlist(gpu_tgfx_listhandle* GpuList);

//Debug callbacks are user defined callbacks, you should assign the function pointer if you want to use them
//As default, all backends set them as empty no-work functions
inline void debugcallback(result_tgfx result, const char* Text);
//You can set this if TGFX is started with threaded call.
inline void debugcallback_threaded(unsigned char ThreadIndex, result_tgfx Result, const char* Text);

void change_window_resolution(window_tgfx_handle WindowHandle, unsigned int width, unsigned int height);
void destroy_tgfx_resources();
void take_inputs();

inline void set_helper_functions();
inline void Save_Monitors();
inline void Create_Instance();
inline void Setup_Debugging();
inline void Check_Computer_Specs();
void Create_IMGUI();
void Create_Renderer();
void Create_GPUContentManager();

void GFX_Error_Callback(int error_code, const char* description) {
	printf("TGFX_FAIL: %s\n", description);
}


result_tgfx load(registrysys_tapi* regsys, core_tgfx_type* core) {
	if (!regsys->get(TGFX_PLUGIN_NAME, TGFX_PLUGIN_VERSION, 0)) return result_tgfx_FAIL;

	load(regsys, core);

	core->api->INVALIDHANDLE = regsys->get(VIRTUALMEMORY_TAPI_PLUGIN_NAME, VIRTUALMEMORY_TAPI_PLUGIN_VERSION, 0);
	//Threading is supported
	if (regsys->get(THREADINGSYS_TAPI_PLUGIN_NAME, THREADINGSYS_TAPI_PLUGIN_VERSION, 0)) {

	}
	else {

	}
	
	core_private* newdatas = new core_private;
	core->data = (core_tgfx_d*)newdatas;

	//Set function pointers of the user API
	core->api->change_window_resolution = change_window_resolution;
	core->api->create_window = &createwindow_vk;
	core->api->debugcallback = debugcallback;
	core->api->debugcallback_threaded = debugcallback_threaded;
	core->api->destroy_tgfx_resources = destroy_tgfx_resources;
	core->api->initialize_secondstage = initialize_secondstage;
	core->api->take_inputs = take_inputs;
	core->api->getmonitorlist = getmonitorlist;
	core->api->getGPUlist = getGPUlist;
	set_helper_functions();


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

	return result_tgfx_SUCCESS;
}
extern "C" FUNC_DLIB_EXPORT result_tgfx backend_load(registrysys_tapi* regsys, core_tgfx_type* core){
	return load(regsys, core);
}
void initialize_secondstage(initializationsecondstageinfo_tgfx_handle info) {
	initialization_secondstageinfo* vkinfo = (initialization_secondstageinfo*)info;
	rendergpu = vkinfo->renderergpu;

	if (vkinfo->shouldActivate_DearIMGUI) {
		Create_IMGUI();
	}


	Create_Renderer();
	Create_GPUContentManager();
	delete vkinfo;
}

void Save_Monitors() {
	local_core->MONITORs.clear();
	int monitor_count;
	GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
	printer(result_tgfx_SUCCESS, ("VulkanCore: " + std::to_string(monitor_count) + " number of monitor(s) detected!").c_str());
	for (unsigned int i = 0; i < monitor_count; i++) {
		GLFWmonitor* monitor = monitors[i];

		//Get monitor name provided by OS! It is a driver based name, so it maybe incorrect!
		const char* monitor_name = glfwGetMonitorName(monitor);
		local_core->MONITORs.push_back(new monitor_vk());
		monitor_vk* Monitor = local_core->MONITORs[local_core->MONITORs.size() - 1];
		Monitor->name = monitor_name;

		//Get videomode to detect at which resolution the OS is using the monitor
		const GLFWvidmode* monitor_vid_mode = glfwGetVideoMode(monitor);
		Monitor->width = monitor_vid_mode->width;
		Monitor->height = monitor_vid_mode->height;
		Monitor->color_bites = monitor_vid_mode->blueBits;
		Monitor->refresh_rate = monitor_vid_mode->refreshRate;

		//Get monitor's physical size, developer may want to use it!
		int physical_width, physical_height;
		glfwGetMonitorPhysicalSize(monitor, &physical_width, &physical_height);
		if (PHYSICALWIDTH == 0 || PHYSICALHEIGHT == 0) {
			printer(result_tgfx_WARNING, "One of the monitors have invalid physical sizes, please be careful");
		}
		Monitor->physical_width = physical_width;
		Monitor->physical_height = physical_height;
	}
}

inline bool IsExtensionSupported(const char* ExtensionName, VkExtensionProperties* SupportedExtensions, unsigned int SupportedExtensionsCount);
std::vector<const char*> Active_InstanceExtensionNames;
std::vector<VkExtensionProperties> Supported_InstanceExtensionList;
inline void Check_InstanceExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	for (unsigned int i = 0; i < glfwExtensionCount; i++) {
		if (!IsExtensionSupported(glfwExtensions[i], Supported_InstanceExtensionList.data(), Supported_InstanceExtensionList.size())) {
			printer(result_tgfx_INVALIDARGUMENT, "Your vulkan instance doesn't support extensions that're required by GLFW. This situation is not tested, so report your device to the author!");
			return;
		}
		Active_InstanceExtensionNames.push_back(glfwExtensions[i]);
	}


	if (IsExtensionSupported(VK_KHR_SURFACE_EXTENSION_NAME, Supported_InstanceExtensionList.data(), Supported_InstanceExtensionList.size())) {
		isActive_SurfaceKHR = true;
		Active_InstanceExtensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	}
	else {
		printer(result_tgfx_WARNING, "Your Vulkan instance doesn't support to display a window, so you shouldn't use any window related functionality such as: GFXRENDERER->Create_WindowPass, GFX->Create_Window, GFXRENDERER->Swap_Buffers ...");
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
	//Doesn't construct VkExtensionProperties object, so we have to use resize!
	Supported_InstanceExtensionList.resize(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, Supported_InstanceExtensionList.data());
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
		printer(result_tgfx_FAIL, "Failed to create a Vulkan Instance!");
	}
	printer(result_tgfx_SUCCESS, "Vulkan Instance is created successfully!");

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
		printer(result_tgfx_FAIL, "Vulkan's Debug Callback system failed to start!");
	}
	printer(result_tgfx_SUCCESS, "Vulkan Debug Callback system is started!");
}



inline void Check_Computer_Specs() {
	printer(result_tgfx_SUCCESS, "Started to check Computer Specifications!");

	//CHECK GPUs
	uint32_t GPU_NUMBER = 0;
	vkEnumeratePhysicalDevices(Vulkan_Instance, &GPU_NUMBER, nullptr);
	std::vector<VkPhysicalDevice> Physical_GPU_LIST;
	for (unsigned int i = 0; i < GPU_NUMBER; i++) {
		Physical_GPU_LIST.push_back(VkPhysicalDevice());
	}
	vkEnumeratePhysicalDevices(Vulkan_Instance, &GPU_NUMBER, Physical_GPU_LIST.data());

	if (GPU_NUMBER == 0) {
		printer(result_tgfx_FAIL, "There is no GPU that has Vulkan support! Updating your drivers or Upgrading the OS may help");
	}

	//GET GPU INFORMATIONs, QUEUE FAMILIES etc
	for (unsigned int i = 0; i < GPU_NUMBER; i++) {
		//GPU initializer handles everything else
		gpu_vk* vkgpu = new gpu_vk(Physical_GPU_LIST[i]);
	}

	printer(result_tgfx_SUCCESS, "Finished checking Computer Specifications!");
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
		printer(result_tgfx_NOTCODED, "VulkanCore: Window Surface Capabilities have issues, maxImageCount <= minImageCount!");
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
		printer(result_tgfx_FAIL, "VulkanCore: Failed to create a SwapChain for a Window");
		return false;
	}

	//Get Swapchain images
	uint32_t created_imagecount = 0;
	vkGetSwapchainImagesKHR(RENDERGPU->LOGICALDEVICE(), *SwapchainOBJ, &created_imagecount, nullptr);
	VkImage* SWPCHN_IMGs = new VkImage[created_imagecount];
	vkGetSwapchainImagesKHR(RENDERGPU->LOGICALDEVICE(), *SwapchainOBJ, &created_imagecount, SWPCHN_IMGs);
	if (created_imagecount < 2) {
		printer(result_tgfx_FAIL, "TGFX API asked for 2 swapchain textures but Vulkan driver gave less number of textures!");
		return false;
	}
	else if (created_imagecount > 2) {
		printer(result_tgfx_SUCCESS, "TGFX API asked for 2 swapchain textures but Vulkan driver gave more than that, so GFX API only used 2 of them!");
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
				printer(result_tgfx_FAIL, "VulkanCore: Image View creation has failed!");
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
	printer(result_tgfx_SUCCESS, "Window creation has started!");

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
		printer(result_tgfx_FAIL, "VulkanCore: We failed to create the window because of GLFW!");
		delete Vulkan_Window;
		return;
	}

	//Window VulkanSurface Creation
	VkSurfaceKHR Window_Surface = {};
	if (glfwCreateWindowSurface(Vulkan_Instance, Vulkan_Window->GLFW_WINDOW, nullptr, &Window_Surface) != VK_SUCCESS) {
		printer(result_tgfx_FAIL, "GLFW failed to create a window surface");
		return;
	}
	else {
		printer(result_tgfx_SUCCESS, "GLFW created a window surface!");
	}
	Vulkan_Window->Window_Surface = Window_Surface;

	if (ResizeCB) {
		glfwSetWindowUserPointer(Vulkan_Window->GLFW_WINDOW, Vulkan_Window);
		glfwSetWindowSizeCallback(Vulkan_Window->GLFW_WINDOW, GLFWwindowresizecallback);
	}


	//Finding GPU_TO_RENDER's Surface Capabilities

	if (!RENDERGPU->isWindowSupported(Vulkan_Window->Window_Surface)) {
		printer(result_tgfx_FAIL, "Vulkan backend supports windows that your GPU supports but your GPU doesn't support current window. So window creation has failed!");
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
		printer(result_tgfx_FAIL, "This GPU doesn't support this type of windows, please try again with a different window configuration!");
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
		printer(result_tgfx_FAIL, "Window's swapchain creation has failed, so window's creation!");
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


	printer(result_tgfx_SUCCESS, "Window creation is successful!");
	VKCORE->WINDOWs.push_back(Vulkan_Window);
}
