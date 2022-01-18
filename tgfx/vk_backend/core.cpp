#include "predefinitions_vk.h"
#include "includes.h"
#include "core.h"
#include <registrysys_tapi.h>
#include <virtualmemorysys_tapi.h>
#include <threadingsys_tapi.h>
#include <tgfx_core.h>
#include <stdio.h>
#include <vector>
#include <string>
#include "resource.h"
#include <tgfx_helper.h>
#include <tgfx_renderer.h>
#include <tgfx_gpucontentmanager.h>
#include <tgfx_imgui.h>
#include <vector>
#include "memory.h"
#include "queue.h"
#include "extension.h"

struct core_private{
public:
	std::vector<gpu_public*> DEVICE_GPUs;
	//Window Operations
	std::vector<monitor_vk*> MONITORs;
	std::vector<window_vk*> WINDOWs;

	//Instead of checking each window each frame, just check this
	bool isAnyWindowResized = false, isActive_SurfaceKHR = false, isSupported_PhysicalDeviceProperties2 = false;
};
static core_private* hidden = nullptr;


void GFX_Error_Callback(int error_code, const char* description) {
	printf("TGFX_FAIL: %s\n", description);
}

struct device_features_chainedstructs;
struct core_functions {
public:
	static inline void initialize_secondstage(initializationsecondstageinfo_tgfx_handle info);
	//SwapchainTextureHandles should point to an array of 2 elements! Triple Buffering is not supported for now.
	static inline void createwindow_vk(unsigned int WIDTH, unsigned int HEIGHT, monitor_tgfx_handle monitor,
		windowmode_tgfx Mode, const char* NAME, textureusageflag_tgfx_handle SwapchainUsage, tgfx_windowResizeCallback ResizeCB,
		void* UserPointer, texture_tgfx_handle* SwapchainTextureHandles, window_tgfx_handle* window);
	static inline void change_window_resolution(window_tgfx_handle WindowHandle, unsigned int width, unsigned int height);
	static inline void getmonitorlist(monitor_tgfx_listhandle* MonitorList);
	static inline void getGPUlist(gpu_tgfx_listhandle* GpuList);

	//Debug callbacks are user defined callbacks, you should assign the function pointer if you want to use them
	//As default, all backends set them as empty no-work functions
	static inline void debugcallback(result_tgfx result, const char* Text);
	//You can set this if TGFX is started with threaded call.
	static inline void debugcallback_threaded(unsigned char ThreadIndex, result_tgfx Result, const char* Text);


	static void destroy_tgfx_resources();
	static void take_inputs();

	static inline void set_helper_functions();
	static inline void Save_Monitors();
	static inline void Create_Instance();
	static inline void Setup_Debugging();
	static inline void Check_Computer_Specs();
	static inline void createwindow_vk(unsigned int WIDTH, unsigned int HEIGHT, monitor_tgfx_handle monitor,
		windowmode_tgfx Mode, const char* NAME, textureusageflag_tgfx_handle SwapchainUsage, tgfx_windowResizeCallback ResizeCB, void* UserPointer,
		texture_vk* SwapchainTextureHandles, window_tgfx_handle* window);

	//GPU ANALYZATION FUNCS

	static inline void Gather_PhysicalDeviceInformations(gpu_public* VKGPU);
	static inline void Describe_SupportedExtensions(gpu_public* VKGPU);
	static inline void ActivateDeviceFeatures(gpu_public* VKGPU, VkDeviceCreateInfo& device_ci, device_features_chainedstructs& chainer);
	static inline void CheckDeviceLimits(gpu_public* VKGPU);
};


extern void Create_IMGUI();
extern void Create_Renderer();
extern void Create_GPUContentManager();
extern void set_helper_functions();
extern void Create_AllocatorSys();
extern void Create_QueueSys();


void printf_log_tgfx(result_tgfx result, const char* text) {
	printf("TGFX %u: %s\n", (unsigned int)result, text);
}
result_tgfx load(registrysys_tapi* regsys, core_tgfx_type* core, tgfx_PrintLogCallback printcallback) {
	if (!regsys->get(TGFX_PLUGIN_NAME, TGFX_PLUGIN_VERSION, 0))
		return result_tgfx_FAIL;

	core->api->INVALIDHANDLE = regsys->get(VIRTUALMEMORY_TAPI_PLUGIN_NAME, VIRTUALMEMORY_TAPI_PLUGIN_VERSION, 0);
	//Threading is supported
	if (regsys->get(THREADINGSYS_TAPI_PLUGIN_NAME, THREADINGSYS_TAPI_PLUGIN_VERSION, 0)) {

	}
	else {

	}

	core_vk = new core_public;
	core->data = (core_tgfx_d*)core_vk;
	core_tgfx_main = core->api;
	hidden = new core_private;

	if (printcallback) { printer = printcallback; }
	else { printer = printf_log_tgfx; }

	core->api->initialize_secondstage = &core_functions::initialize_secondstage;
	
	//Set function pointers of the user API
	//core->api->change_window_resolution = &core_functions::change_window_resolution;
	core->api->create_window = &core_functions::createwindow_vk;
	//core->api->debugcallback = &core_functions::debugcallback;
	//core->api->debugcallback_threaded = &core_functions::debugcallback_threaded;
	//core->api->destroy_tgfx_resources = &core_functions::destroy_tgfx_resources;
	//core->api->take_inputs = &core_functions::take_inputs;
	core->api->getmonitorlist = &core_functions::getmonitorlist;
	core->api->getGPUlist = &core_functions::getGPUlist;
	set_helper_functions();
	Create_AllocatorSys();
	Create_QueueSys();


	//Set error callback to handle all glfw errors (including initialization error)!
	glfwSetErrorCallback(GFX_Error_Callback);

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	core_functions::Save_Monitors();

	core_functions::Create_Instance();
#ifdef VULKAN_DEBUGGING
	core_functions::Setup_Debugging();
#endif
	core_functions::Check_Computer_Specs();

	return result_tgfx_SUCCESS;
}
extern "C" FUNC_DLIB_EXPORT result_tgfx backend_load(registrysys_tapi* regsys, core_tgfx_type* core, tgfx_PrintLogCallback printcallback){
	return load(regsys, core, printcallback);
 }
//While enabling features, some struct should be chained. This struct is to keep data object lifetimes optimal
struct device_features_chainedstructs {
	VkPhysicalDeviceDescriptorIndexingFeatures DescIndexingFeatures = {};
};
//You have to enable some features to use some device extensions
inline void core_functions::ActivateDeviceFeatures(gpu_public* VKGPU, VkDeviceCreateInfo& device_ci, device_features_chainedstructs& chainer) {
	const void*& dev_ci_Next = device_ci.pNext;
	void** extending_Next = nullptr;


	//Check for seperate RTSlot blending
	if (VKGPU->Supported_Features.independentBlend) {
		VKGPU->Active_Features.independentBlend = VK_TRUE;
		//GPUDesc.isSupported_SeperateRTSlotBlending = true;
	}

	//Activate Descriptor Indexing Features
	if (VKGPU->extensions->ISSUPPORTED_DESCINDEXING()) {
		chainer.DescIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
		chainer.DescIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		chainer.DescIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
		chainer.DescIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
		chainer.DescIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
		chainer.DescIndexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
		chainer.DescIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
		chainer.DescIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
		chainer.DescIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		chainer.DescIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
		chainer.DescIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
		chainer.DescIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
		chainer.DescIndexingFeatures.pNext = nullptr;
		chainer.DescIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

		if (extending_Next) {
			*extending_Next = &chainer.DescIndexingFeatures;
			extending_Next = &chainer.DescIndexingFeatures.pNext;
		}
		else {
			dev_ci_Next = &chainer.DescIndexingFeatures;
			extending_Next = &chainer.DescIndexingFeatures.pNext;
		}
	}
}
inline void core_functions::CheckDeviceLimits(gpu_public* VKGPU) {
	//Check Descriptor Limits
	{
		if (VKGPU->extensions->ISSUPPORTED_DESCINDEXING()) {
			VkPhysicalDeviceDescriptorIndexingProperties* descindexinglimits = new VkPhysicalDeviceDescriptorIndexingProperties{};
			descindexinglimits->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;
			VkPhysicalDeviceProperties2 devprops2;
			devprops2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			devprops2.pNext = descindexinglimits;
			vkGetPhysicalDeviceProperties2(VKGPU->Physical_Device, &devprops2);

			VKGPU->extensions->GETMAXDESC(desctype_vk::SAMPLER) = descindexinglimits->maxDescriptorSetUpdateAfterBindSampledImages;
			VKGPU->extensions->GETMAXDESC(desctype_vk::IMAGE) = descindexinglimits->maxDescriptorSetUpdateAfterBindStorageImages;
			VKGPU->extensions->GETMAXDESC(desctype_vk::SBUFFER) = descindexinglimits->maxDescriptorSetUpdateAfterBindStorageBuffers;
			VKGPU->extensions->GETMAXDESC(desctype_vk::UBUFFER) = descindexinglimits->maxDescriptorSetUpdateAfterBindUniformBuffers;
			VKGPU->extensions->GETMAXDESCALL() = descindexinglimits->maxUpdateAfterBindDescriptorsInAllPools;
			VKGPU->extensions->GETMAXDESCPERSTAGE() = descindexinglimits->maxPerStageUpdateAfterBindResources;
			delete descindexinglimits;
		}
		else {
			VKGPU->extensions->GETMAXDESC(desctype_vk::SAMPLER) = VKGPU->Device_Properties.limits.maxPerStageDescriptorSampledImages;
			VKGPU->extensions->GETMAXDESC(desctype_vk::IMAGE) = VKGPU->Device_Properties.limits.maxPerStageDescriptorStorageImages;
			VKGPU->extensions->GETMAXDESC(desctype_vk::SBUFFER) = VKGPU->Device_Properties.limits.maxPerStageDescriptorStorageBuffers;
			VKGPU->extensions->GETMAXDESC(desctype_vk::UBUFFER) = VKGPU->Device_Properties.limits.maxPerStageDescriptorUniformBuffers;
			VKGPU->extensions->GETMAXDESCALL() = VKGPU->Device_Properties.limits.maxPerStageResources;
			VKGPU->extensions->GETMAXDESCPERSTAGE() = UINT32_MAX;
		}
	}
}
void core_functions::initialize_secondstage(initializationsecondstageinfo_tgfx_handle info) {
	initialization_secondstageinfo* vkinfo = (initialization_secondstageinfo*)info;
	rendergpu = vkinfo->renderergpu;

	if (vkinfo->shouldActivate_DearIMGUI) {
		Create_IMGUI();
	}

	//Create Logical Device
	{
		std::vector<VkDeviceQueueCreateInfo> queues_ci = queuesys->get_queue_cis(rendergpu);

		VkDeviceCreateInfo Logical_Device_CreationInfo{};
		Logical_Device_CreationInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		Logical_Device_CreationInfo.flags = 0;
		Logical_Device_CreationInfo.pQueueCreateInfos = queues_ci.data();
		Logical_Device_CreationInfo.queueCreateInfoCount = static_cast<uint32_t>(queues_ci.size());
		//This is to destroy datas of extending features
		device_features_chainedstructs chainer;
		ActivateDeviceFeatures(rendergpu, Logical_Device_CreationInfo, chainer);

		Logical_Device_CreationInfo.enabledExtensionCount = rendergpu->Active_DeviceExtensions.size();
		Logical_Device_CreationInfo.ppEnabledExtensionNames = rendergpu->Active_DeviceExtensions.data();
		Logical_Device_CreationInfo.pEnabledFeatures = &rendergpu->Active_Features;

		Logical_Device_CreationInfo.enabledLayerCount = 0;
		if (vkCreateDevice(rendergpu->Physical_Device, &Logical_Device_CreationInfo, nullptr, &rendergpu->Logical_Device) != VK_SUCCESS) {
			printer(result_tgfx_SUCCESS, "Vulkan failed to create a Logical Device!");
			return;
		}
		printer(result_tgfx_SUCCESS, "After vkCreateDevice()");
		
		queuesys->get_queue_objects(rendergpu);

		CheckDeviceLimits(rendergpu);
		printer(result_tgfx_SUCCESS, "After Check_DeviceLimits()");
	}


	Create_Renderer();
	Create_GPUContentManager();
	delete vkinfo;
}

inline bool IsExtensionSupported(const char* ExtensionName, VkExtensionProperties* SupportedExtensions, unsigned int SupportedExtensionsCount) {
	bool Is_Found = false;
	for (unsigned int supported_extension_index = 0; supported_extension_index < SupportedExtensionsCount; supported_extension_index++) {
		if (strcmp(ExtensionName, SupportedExtensions[supported_extension_index].extensionName)) {
			return true;
		}
	}
	printer(result_tgfx_WARNING, ("Extension: " + std::string(ExtensionName) + " is not supported by the GPU!").c_str());
	return false;
}
inline void core_functions::Describe_SupportedExtensions(gpu_public* VKGPU) {
	//GET SUPPORTED DEVICE EXTENSIONS
	vkEnumerateDeviceExtensionProperties(VKGPU->Physical_Device, nullptr, &VKGPU->Supported_DeviceExtensionsCount, nullptr);
	VKGPU->Supported_DeviceExtensions = new VkExtensionProperties[VKGPU->Supported_DeviceExtensionsCount];
	vkEnumerateDeviceExtensionProperties(VKGPU->Physical_Device, nullptr, &VKGPU->Supported_DeviceExtensionsCount, VKGPU->Supported_DeviceExtensions);

	//Check Seperate_DepthStencil
	if (Application_Info.apiVersion == VK_API_VERSION_1_0 || Application_Info.apiVersion == VK_API_VERSION_1_1) {
		if (IsExtensionSupported(VK_KHR_SEPARATE_DEPTH_STENCIL_LAYOUTS_EXTENSION_NAME, VKGPU->Supported_DeviceExtensions, VKGPU->Supported_DeviceExtensionsCount)) {
			VKGPU->extensions->SUPPORT_SEPERATEDDEPTHSTENCILLAYOUTS();
			VKGPU->Active_DeviceExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
			if (Application_Info.apiVersion == VK_API_VERSION_1_0) {
				VKGPU->Active_DeviceExtensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);
				VKGPU->Active_DeviceExtensions.push_back(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
			}
		}
	}
	else {
		VKGPU->extensions->SUPPORT_SEPERATEDDEPTHSTENCILLAYOUTS();
	}

	if (IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, VKGPU->Supported_DeviceExtensions, VKGPU->Supported_DeviceExtensionsCount)) {
		VKGPU->extensions->SUPPORT_SWAWPCHAINDISPLAY();
		VKGPU->Active_DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}
	else {
		printer(result_tgfx_WARNING, "Current GPU doesn't support to display a swapchain, so you shouldn't use any window related functionality such as: GFXRENDERER->Create_WindowPass, GFX->Create_Window, GFXRENDERER->Swap_Buffers ...");
	}


	//Check Descriptor Indexing
	//First check Maintenance3 and PhysicalDeviceProperties2 for Vulkan 1.0
	if (Application_Info.apiVersion == VK_API_VERSION_1_0) {
		if (IsExtensionSupported(VK_KHR_MAINTENANCE3_EXTENSION_NAME, VKGPU->Supported_DeviceExtensions, VKGPU->Supported_DeviceExtensionsCount) && hidden->isSupported_PhysicalDeviceProperties2) {
			VKGPU->Active_DeviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
			if (IsExtensionSupported(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, VKGPU->Supported_DeviceExtensions, VKGPU->Supported_DeviceExtensionsCount)) {
				VKGPU->Active_DeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
				VKGPU->extensions->SUPPORT_DESCINDEXING();
			}
		}
	}
	//Maintenance3 and PhysicalDeviceProperties2 is core in 1.1, so check only Descriptor Indexing
	else {
		//If Vulkan is 1.1, check extension
		if (Application_Info.apiVersion == VK_API_VERSION_1_1) {
			if (IsExtensionSupported(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, VKGPU->Supported_DeviceExtensions, VKGPU->Supported_DeviceExtensionsCount)) {
				VKGPU->Active_DeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
				VKGPU->extensions->SUPPORT_DESCINDEXING();
			}
		}
		//1.2+ Vulkan supports DescriptorIndexing by default.
		else {
			VKGPU->extensions->SUPPORT_DESCINDEXING();
		}
	}
}
void core_functions::Save_Monitors() {
	hidden->MONITORs.clear();
	int monitor_count;
	GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
	printer(result_tgfx_SUCCESS, ("VulkanCore: " + std::to_string(monitor_count) + " number of monitor(s) detected!").c_str());
	for (unsigned int i = 0; i < monitor_count; i++) {
		GLFWmonitor* monitor = monitors[i];

		//Get monitor name provided by OS! It is a driver based name, so it maybe incorrect!
		const char* monitor_name = glfwGetMonitorName(monitor);
		hidden->MONITORs.push_back(new monitor_vk());
		monitor_vk* Monitor = hidden->MONITORs[hidden->MONITORs.size() - 1];
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
		hidden->isActive_SurfaceKHR = true;
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
		hidden->isSupported_PhysicalDeviceProperties2 = true;
		if (Application_Info.apiVersion == VK_API_VERSION_1_0) {
			Active_InstanceExtensionNames.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		}
	}
}

void core_functions::Create_Instance() {
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
void core_functions::Setup_Debugging() {
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


void core_functions::Gather_PhysicalDeviceInformations(gpu_public* VKGPU) {
	vkGetPhysicalDeviceProperties(VKGPU->PHYSICALDEVICE(), &VKGPU->Device_Properties);
	vkGetPhysicalDeviceFeatures(VKGPU->Physical_Device, &VKGPU->Supported_Features);

	//GET QUEUE FAMILIES, SAVE THEM TO GPU OBJECT, CHECK AND SAVE GRAPHICS,COMPUTE,TRANSFER QUEUEFAMILIES INDEX
	vkGetPhysicalDeviceQueueFamilyProperties(VKGPU->Physical_Device, &VKGPU->QueueFamiliesCount, nullptr);
	VKGPU->QueueFamilyProperties = new VkQueueFamilyProperties[VKGPU->QueueFamiliesCount];
	vkGetPhysicalDeviceQueueFamilyProperties(VKGPU->Physical_Device, &VKGPU->QueueFamiliesCount, VKGPU->QueueFamilyProperties);

	vkGetPhysicalDeviceMemoryProperties(VKGPU->Physical_Device, &VKGPU->MemoryProperties);
}


inline const char* Convert_VendorID_toaString(uint32_t VendorID) {
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
		printer(result_tgfx_FAIL, "(Vulkan_Core::Check_Computer_Specs failed to find GPU's Vendor, Vendor ID is: " + VendorID);
		return "NULL";
	}
}
inline void core_functions::Check_Computer_Specs() {
	printer(result_tgfx_SUCCESS, "Started to check Computer Specifications!");

	//CHECK GPUs
	uint32_t GPU_NUMBER = 0;
	vkEnumeratePhysicalDevices(Vulkan_Instance, &GPU_NUMBER, nullptr);
	std::vector<VkPhysicalDevice> Physical_GPU_LIST(GPU_NUMBER, VK_NULL_HANDLE);
	vkEnumeratePhysicalDevices(Vulkan_Instance, &GPU_NUMBER, Physical_GPU_LIST.data());

	if (GPU_NUMBER == 0) {
		printer(result_tgfx_FAIL, "There is no GPU that has Vulkan support! Updating your drivers or Upgrading the OS may help");
	}

	std::vector<VkPhysicalDevice> DiscreteGPUs;
	//GET GPU INFORMATIONs, QUEUE FAMILIES etc
	for (unsigned int i = 0; i < GPU_NUMBER; i++) {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(Physical_GPU_LIST[i], &props);
		if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			DiscreteGPUs.push_back(Physical_GPU_LIST[i]);
		}
		else{
			printer(result_tgfx_WARNING, "Non-Discrete GPUs are not available in vulkan backend!");
		}
	}
	for(unsigned int i = 0; i < DiscreteGPUs.size(); i++){
		//GPU initializer handles everything else
		gpu_public* vkgpu = new gpu_public;
		vkgpu->hidden = nullptr;	//there is no private gpu data for now
		vkgpu->Physical_Device = DiscreteGPUs[i];
		hidden->DEVICE_GPUs.push_back(vkgpu);


		Gather_PhysicalDeviceInformations(vkgpu);
		gpudescription_tgfx gpudesc;
		{
			std::string fullname = Convert_VendorID_toaString(vkgpu->DEVICEPROPERTIES().vendorID) + std::string(vkgpu->DEVICEPROPERTIES().deviceName);
			vkgpu->desc.MODEL = new char[fullname.length() + 1];
			memcpy(vkgpu->desc.MODEL, fullname.c_str(), fullname.length() + 1);
		}

		//SAVE BASIC INFOs TO THE GPU DESC
		gpudesc.DRIVER_VERSION = vkgpu->Device_Properties.driverVersion;
		gpudesc.API_VERSION = vkgpu->Device_Properties.apiVersion;
		gpudesc.DRIVER_VERSION = vkgpu->Device_Properties.driverVersion;

		allocatorsys->analize_gpumemory(vkgpu);
		queuesys->analize_queues(vkgpu);

		printer(result_tgfx_SUCCESS, "Finished checking Computer Specifications!");
		/*


		VkDeviceCreateInfo Logical_Device_CreationInfo{};
		Logical_Device_CreationInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		Logical_Device_CreationInfo.flags = 0;
		Logical_Device_CreationInfo.pQueueCreateInfos = QueueCreationInfos.data();
		Logical_Device_CreationInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreationInfos.size());
		VK_States.Activate_DeviceExtensions(VKGPU);
		LOG_STATUS_TAPI("Activated Device Extensions");
		//This is to destroy datas of extending features
		DeviceExtendedFeatures extendedfeatures;
		VK_States.Activate_DeviceFeatures(VKGPU, GPUdesc, Logical_Device_CreationInfo, extendedfeatures);

		Logical_Device_CreationInfo.enabledExtensionCount = VKGPU->Active_DeviceExtensions.size();
		Logical_Device_CreationInfo.ppEnabledExtensionNames = VKGPU->Active_DeviceExtensions.data();
		Logical_Device_CreationInfo.pEnabledFeatures = &VKGPU->Active_Features;

		Logical_Device_CreationInfo.enabledLayerCount = 0;

		if (VKGPU->Device_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			if (vkCreateDevice(VKGPU->Physical_Device, &Logical_Device_CreationInfo, nullptr, &VKGPU->Logical_Device) != VK_SUCCESS) {
				LOG_STATUS_TAPI("Vulkan failed to create a Logical Device!");
				return;
			}
			LOG_STATUS_TAPI("After vkCreateDevice()");

			VKGPU->AllQueueFamilies = new uint32_t[VKGPU->QUEUEs.size()];
			for (unsigned int QueueIndex = 0; QueueIndex < VKGPU->QUEUEs.size(); QueueIndex++) {
				LOG_STATUS_TAPI("Queue Feature Score: " + to_string(VKGPU->QUEUEs[QueueIndex].QueueFeatureScore));
				vkGetDeviceQueue(VKGPU->Logical_Device, VKGPU->QUEUEs[QueueIndex].QueueFamilyIndex, 0, &VKGPU->QUEUEs[QueueIndex].Queue);
				LOG_STATUS_TAPI("After vkGetDeviceQueue() " + to_string(QueueIndex));
				VKGPU->AllQueueFamilies[QueueIndex] = VKGPU->QUEUEs[QueueIndex].QueueFamilyIndex;
			}
			LOG_STATUS_TAPI("After vkGetDeviceQueue()");
			LOG_STATUS_TAPI("Vulkan created a Logical Device!");

			VK_States.Check_DeviceLimits(VKGPU, GPUdesc);
			LOG_STATUS_TAPI("After Check_DeviceLimits()");
		}
		else {
		}
		/*
		Analize_Queues(VKGPU, GPUdesc);
		LOG_STATUS_TAPI("Analized Queues");

		vector<VkDeviceQueueCreateInfo> QueueCreationInfos(0);
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
		VK_States.Activate_DeviceExtensions(VKGPU);
		LOG_STATUS_TAPI("Activated Device Extensions");
		//This is to destroy datas of extending features
		DeviceExtendedFeatures extendedfeatures;
		VK_States.Activate_DeviceFeatures(VKGPU, GPUdesc, Logical_Device_CreationInfo, extendedfeatures);

		Logical_Device_CreationInfo.enabledExtensionCount = VKGPU->Active_DeviceExtensions.size();
		Logical_Device_CreationInfo.ppEnabledExtensionNames = VKGPU->Active_DeviceExtensions.data();
		Logical_Device_CreationInfo.pEnabledFeatures = &VKGPU->Active_Features;

		Logical_Device_CreationInfo.enabledLayerCount = 0;

		if (VKGPU->Device_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			if (vkCreateDevice(VKGPU->Physical_Device, &Logical_Device_CreationInfo, nullptr, &VKGPU->Logical_Device) != VK_SUCCESS) {
				LOG_STATUS_TAPI("Vulkan failed to create a Logical Device!");
				return;
			}
			LOG_STATUS_TAPI("After vkCreateDevice()");

			VKGPU->AllQueueFamilies = new uint32_t[VKGPU->QUEUEs.size()];
			for (unsigned int QueueIndex = 0; QueueIndex < VKGPU->QUEUEs.size(); QueueIndex++) {
				LOG_STATUS_TAPI("Queue Feature Score: " + to_string(VKGPU->QUEUEs[QueueIndex].QueueFeatureScore));
				vkGetDeviceQueue(VKGPU->Logical_Device, VKGPU->QUEUEs[QueueIndex].QueueFamilyIndex, 0, &VKGPU->QUEUEs[QueueIndex].Queue);
				LOG_STATUS_TAPI("After vkGetDeviceQueue() " + to_string(QueueIndex));
				VKGPU->AllQueueFamilies[QueueIndex] = VKGPU->QUEUEs[QueueIndex].QueueFamilyIndex;
			}
			LOG_STATUS_TAPI("After vkGetDeviceQueue()");
			LOG_STATUS_TAPI("Vulkan created a Logical Device!");

			VK_States.Check_DeviceLimits(VKGPU, GPUdesc);
			LOG_STATUS_TAPI("After Check_DeviceLimits()");

			GPUdescs.push_back(GPUdesc);
			DEVICE_GPUs.push_back(VKGPU);
		}
		else {
			LOG_WARNING_TAPI("RenderDoc doesn't support to create multiple vkDevices, so Device object isn't created for non-Discrete GPUs!");
		}
		*/
	}

}




bool Create_WindowSwapchain(window_vk* Vulkan_Window, unsigned int WIDTH, unsigned int HEIGHT, VkSwapchainKHR* SwapchainOBJ, texture_vk** SwapchainTextureHandles) {
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

	if (rendergpu->QUEUEFAMSCOUNT() > 1) {
		swpchn_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	}
	else {
		swpchn_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	swpchn_ci.pQueueFamilyIndices = rendergpu->ALLQUEUEFAMILIES();
	swpchn_ci.queueFamilyIndexCount = rendergpu->QUEUEFAMSCOUNT();


	if (vkCreateSwapchainKHR(rendergpu->LOGICALDEVICE(), &swpchn_ci, nullptr, SwapchainOBJ) != VK_SUCCESS) {
		printer(result_tgfx_FAIL, "VulkanCore: Failed to create a SwapChain for a Window");
		return false;
	}

	//Get Swapchain images
	uint32_t created_imagecount = 0;
	vkGetSwapchainImagesKHR(rendergpu->LOGICALDEVICE(), *SwapchainOBJ, &created_imagecount, nullptr);
	VkImage* SWPCHN_IMGs = new VkImage[created_imagecount];
	vkGetSwapchainImagesKHR(rendergpu->LOGICALDEVICE(), *SwapchainOBJ, &created_imagecount, SWPCHN_IMGs);
	if (created_imagecount < 2) {
		printer(result_tgfx_FAIL, "TGFX API asked for 2 swapchain textures but Vulkan driver gave less number of textures!");
		return false;
	}
	else if (created_imagecount > 2) {
		printer(result_tgfx_SUCCESS, "TGFX API asked for 2 swapchain textures but Vulkan driver gave more than that, so GFX API only used 2 of them!");
	}
	for (unsigned int vkim_index = 0; vkim_index < 2; vkim_index++) {
		texture_vk* SWAPCHAINTEXTURE = new texture_vk;
		SWAPCHAINTEXTURE->CHANNELs = texture_channels_tgfx_BGRA8UNORM;
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
			texture_vk* SwapchainTexture = SwapchainTextureHandles[i];
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

			if (vkCreateImageView(rendergpu->LOGICALDEVICE(), &ImageView_ci, nullptr, &SwapchainTexture->ImageView) != VK_SUCCESS) {
				printer(result_tgfx_FAIL, "VulkanCore: Image View creation has failed!");
				return false;
			}
		}
	}
	return true;
}

#include "synchronization_sys.h"
void GLFWwindowresizecallback(GLFWwindow* glfwwindow, int width, int height) {
	window_vk* vkwindow = (window_vk*)glfwGetWindowUserPointer(glfwwindow);
	vkwindow->resize_cb((window_tgfx_handle)vkwindow, vkwindow->UserPTR, width, height, (texture_tgfx_handle*)vkwindow->Swapchain_Textures);
}
void core_functions::createwindow_vk(unsigned int WIDTH, unsigned int HEIGHT, monitor_tgfx_handle monitor,
	windowmode_tgfx Mode, const char* NAME, textureusageflag_tgfx_handle SwapchainUsage, tgfx_windowResizeCallback ResizeCB, void* UserPointer,
	texture_tgfx_handle* SwapchainTextureHandles, window_tgfx_handle* window) {
	printer(result_tgfx_SUCCESS, "Window creation has started!");

	if (ResizeCB) {
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	}
	//Create window as it will share resources with Renderer Context to get display texture!
	GLFWwindow* glfw_window = glfwCreateWindow(WIDTH, HEIGHT, NAME, NULL, nullptr);
	window_vk* Vulkan_Window = new window_vk;
	Vulkan_Window->LASTWIDTH = WIDTH;
	Vulkan_Window->LASTHEIGHT = HEIGHT;
	Vulkan_Window->DISPLAYMODE = Mode;
	Vulkan_Window->MONITOR = (monitor_vk*)monitor;
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

	if (!queuesys->check_windowsupport(rendergpu, Vulkan_Window->Window_Surface)) {
		printer(result_tgfx_FAIL, "Vulkan backend supports windows that your GPU supports but your GPU doesn't support current window. So window creation has failed!");
		return;
	}

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(rendergpu->PHYSICALDEVICE(), Vulkan_Window->Window_Surface, &Vulkan_Window->SurfaceCapabilities);
	uint32_t FormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(rendergpu->PHYSICALDEVICE(), Vulkan_Window->Window_Surface, &FormatCount, nullptr);
	Vulkan_Window->SurfaceFormats.resize(FormatCount);
	if (FormatCount != 0) {
		vkGetPhysicalDeviceSurfaceFormatsKHR(rendergpu->PHYSICALDEVICE(), Vulkan_Window->Window_Surface, &FormatCount, Vulkan_Window->SurfaceFormats.data());
	}
	else {
		printer(result_tgfx_FAIL, "This GPU doesn't support this type of windows, please try again with a different window configuration!");
		delete Vulkan_Window;
		return;
	}

	uint32_t PresentationModesCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(rendergpu->PHYSICALDEVICE(), Vulkan_Window->Window_Surface, &PresentationModesCount, nullptr);
	Vulkan_Window->PresentationModes.resize(PresentationModesCount);
	if (PresentationModesCount != 0) {
		vkGetPhysicalDeviceSurfacePresentModesKHR(rendergpu->PHYSICALDEVICE(), Vulkan_Window->Window_Surface, &PresentationModesCount, Vulkan_Window->PresentationModes.data());
	}

	texture_vk* SWPCHNTEXTUREHANDLESVK[2];
	if (!Create_WindowSwapchain(Vulkan_Window, Vulkan_Window->LASTWIDTH, Vulkan_Window->LASTHEIGHT, &Vulkan_Window->Window_SwapChain, SWPCHNTEXTUREHANDLESVK)) {
		printer(result_tgfx_FAIL, "Window's swapchain creation has failed, so window's creation!");
		vkDestroySurfaceKHR(Vulkan_Instance, Vulkan_Window->Window_Surface, nullptr);
		glfwDestroyWindow(Vulkan_Window->GLFW_WINDOW);
		delete Vulkan_Window;
	}
	Vulkan_Window->Swapchain_Textures[0] = SWPCHNTEXTUREHANDLESVK[0];
	Vulkan_Window->Swapchain_Textures[1] = SWPCHNTEXTUREHANDLESVK[1];
	SwapchainTextureHandles[0] = (texture_tgfx_handle)Vulkan_Window->Swapchain_Textures[0];
	SwapchainTextureHandles[1] = (texture_tgfx_handle)Vulkan_Window->Swapchain_Textures[1];

	//Create presentation wait semaphores
	//We are creating 2 semaphores because if 2 frames combined is faster than vertical blank, there is tearing!
	//Previously, this was 3 but I moved command buffer waiting so there is no command buffer collision with display
	for (unsigned char SemaphoreIndex = 0; SemaphoreIndex < 2; SemaphoreIndex++) {
		Vulkan_Window->PresentationSemaphores[SemaphoreIndex] = semaphoresys->Create_Semaphore().get_id();
	}


	printer(result_tgfx_SUCCESS, "Window creation is successful!");
	hidden->WINDOWs.push_back(Vulkan_Window);
}


inline void core_functions::getmonitorlist(monitor_tgfx_listhandle* MonitorList) {
	(*MonitorList) = new monitor_tgfx_handle[hidden->MONITORs.size() + 1]{ NULL };
	for (unsigned int i = 0; i < hidden->MONITORs.size(); i++) {
		(*MonitorList)[i] = (monitor_tgfx_handle)hidden->MONITORs[i];
	}
}
inline void core_functions::getGPUlist(gpu_tgfx_listhandle* GPULIST) {
	*GPULIST = new gpu_tgfx_handle[hidden->DEVICE_GPUs.size() + 1];
	for (unsigned int i = 0; i < hidden->DEVICE_GPUs.size(); i++) {
		(*GPULIST)[i] = (gpu_tgfx_handle)hidden->DEVICE_GPUs[i];
	}
	(*GPULIST)[hidden->DEVICE_GPUs.size()] = (gpu_tgfx_handle)core_tgfx_main->INVALIDHANDLE;
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
		printer(result_tgfx_FAIL, "Vulkan Callback has returned a unsupported Message_Type");
		return true;
		break;
	}

	switch (Message_Severity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		printer(result_tgfx_SUCCESS, pCallback_Data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		printer(result_tgfx_WARNING, pCallback_Data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		printer(result_tgfx_FAIL, pCallback_Data->pMessage);
		break;
	default:
		printer(result_tgfx_FAIL, "Vulkan Callback has returned a unsupported debug message type!");
		return true;
	}
	return false;
}
PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT() {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Vulkan_Instance, "vkCreateDebugUtilsMessengerEXT");
	if (func == nullptr) {
		printer(result_tgfx_FAIL, "(Vulkan failed to load vkCreateDebugUtilsMessengerEXT function!");
		return nullptr;
	}
	return func;
}
PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT() {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Vulkan_Instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func == nullptr) {
		printer(result_tgfx_FAIL, "(Vulkan failed to load vkDestroyDebugUtilsMessengerEXT function!");
		return nullptr;
	}
	return func;
}