#include "vk_predefinitions.h"
#include "vk_includes.h"
#include "vk_core.h"
#include <registrysys_tapi.h>
#include <virtualmemorysys_tapi.h>
#include <threadingsys_tapi.h>
#include <tgfx_core.h>
#include <stdio.h>
#include <vector>
#include <string>
#include "vk_resource.h"
#include <tgfx_helper.h>
#include <tgfx_renderer.h>
#include <tgfx_gpucontentmanager.h>
#include <tgfx_imgui.h>
#include <vector>
#include "memory.h"
#include "vk_queue.h"
#include "vk_extension.h"

#include "vk_contentmanager.h"

struct device_features_chainedstructs;
struct core_functions {
public:
	static void initialize_secondstage(initializationsecondstageinfo_tgfx_handle info);
	//SwapchainTextureHandles should point to an array of 2 elements! Triple Buffering is not supported for now.
	static void createwindow_vk(unsigned int WIDTH, unsigned int HEIGHT, monitor_tgfx_handle monitor,
		windowmode_tgfx Mode, const char* NAME, tgfx_windowResizeCallback ResizeCB, void* UserPointer, window_tgfx_handle* window);
	static result_tgfx create_swapchain(window_tgfx_handle window, windowpresentation_tgfx presentationMode, windowcomposition_tgfx composition,
		colorspace_tgfx colorSpace, texture_channels_tgfx channels, textureusageflag_tgfx_handle SwapchainUsage, texture_tgfx_handle* textures);
	static void change_window_resolution(window_tgfx_handle WindowHandle, unsigned int width, unsigned int height);
	static void getmonitorlist(monitor_tgfx_listhandle* MonitorList);
	static void getGPUlist(gpu_tgfx_listhandle* GpuList);

	//Debug callbacks are user defined callbacks, you should assign the function pointer if you want to use them
	//As default, all backends set them as empty no-work functions
	static void debugcallback(result_tgfx result, const char* Text);
	//You can set this if TGFX is started with threaded call.
	static void debugcallback_threaded(unsigned char ThreadIndex, result_tgfx Result, const char* Text);


	static void destroy_tgfx_resources();

	static void Save_Monitors();
	static void Create_Instance();
	static void Setup_Debugging();
	static void Check_Computer_Specs();

	//GPU ANALYZATION FUNCS

	static void Gather_PhysicalDeviceInformations(GPU_VKOBJ* VKGPU);
	static void Describe_SupportedExtensions(GPU_VKOBJ* VKGPU);
	static void Activate_DeviceExtension_Features(GPU_VKOBJ* VKGPU, VkDeviceCreateInfo& device_ci, device_features_chainedstructs& chainer);
};
void printf_log_tgfx(result_tgfx result, const char* text) {
	printf("TGFX %u: %s\n", (unsigned int)result, text);
}

#ifndef NO_IMGUI
extern void Create_IMGUI();
#endif // !NO_IMGUI

extern void Create_BackendAllocator();
extern void Create_VkRenderer();
extern void Create_GPUContentManager(initialization_secondstageinfo* info);
extern void set_helper_functions();
extern void Create_AllocatorSys();
extern void Create_QueueSys();
extern void Create_SyncSystems();
struct core_private {
public:
	//These are VK_VECTORs, instead of VK_LINEAROBJARRAYs, because they won't change at run-time so frequently
	VK_STATICVECTOR<GPU_VKOBJ, 10> DEVICE_GPUs;
	//Window Operations
	VK_STATICVECTOR<MONITOR_VKOBJ, 20> MONITORs;
	VK_STATICVECTOR<WINDOW_VKOBJ, 50> WINDOWs;


	bool isAnyWindowResized = false; //Instead of checking each window each frame, just check this
	bool isActive_SurfaceKHR = false, isSupported_PhysicalDeviceProperties2 = true;
};
static core_private* hidden = nullptr;
void GFX_Error_Callback(int error_code, const char* description) {
	printer(result_tgfx_FAIL, (std::string("GLFW error: ") + description).c_str());
}
result_tgfx load(registrysys_tapi* regsys, core_tgfx_type* core, tgfx_PrintLogCallback printcallback) {
	if (!regsys->get(TGFX_PLUGIN_NAME, TGFX_PLUGIN_VERSION, 0))
		return result_tgfx_FAIL;

	if (printcallback) { printer_cb = printcallback; }
	else { printer_cb = printf_log_tgfx; }

	VIRTUALMEMORY_TAPI_PLUGIN_TYPE virmemsystype = (VIRTUALMEMORY_TAPI_PLUGIN_TYPE)regsys->get(VIRTUALMEMORY_TAPI_PLUGIN_NAME, VIRTUALMEMORY_TAPI_PLUGIN_VERSION, 0);
	if (!virmemsystype) {
		printer(result_tgfx_FAIL, "Vulkan backend needs virtual memory system, so initialization has failed!");
		return result_tgfx_FAIL;
	}
	else { virmemsys = virmemsystype->funcs; }

	//Check if threading system is loaded
	THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE threadsysloaded = (THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE)regsys->get(THREADINGSYS_TAPI_PLUGIN_NAME, THREADINGSYS_TAPI_PLUGIN_VERSION, 0);
	if (threadsysloaded) {
		threadingsys = threadsysloaded->funcs;
	}

	core_tgfx_main = core->api;
	Create_BackendAllocator();
	uint32_t malloc_size = sizeof(core_public) + sizeof(core_private);
	uint32_t allocptr = vk_virmem::allocate_page((malloc_size / VKCONST_VIRMEMPAGESIZE) + 1);
	core_vk = (core_public*)VK_MEMOFFSET_TO_POINTER(allocptr);
	core->data = (core_tgfx_d*)core_vk;
	hidden = (core_private*)VK_MEMOFFSET_TO_POINTER(allocptr + sizeof(core_public));
	virmemsys->virtual_commit(VK_MEMOFFSET_TO_POINTER(allocptr), malloc_size);


	core->api->initialize_secondstage = &core_functions::initialize_secondstage;

	//Set function pointers of the user API
	//core->api->change_window_resolution = &core_functions::change_window_resolution;
	core->api->create_window = &core_functions::createwindow_vk;
	//core->api->debugcallback = &core_functions::debugcallback;
	//core->api->debugcallback_threaded = &core_functions::debugcallback_threaded;
	//core->api->destroy_tgfx_resources = &core_functions::destroy_tgfx_resources;
	core->api->getmonitorlist = &core_functions::getmonitorlist;
	core->api->getGPUlist = &core_functions::getGPUlist;
	core->api->create_swapchain = &core_functions::create_swapchain;
	set_helper_functions();
	Create_AllocatorSys();
	Create_QueueSys();
	Create_SyncSystems();


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
extern "C" FUNC_DLIB_EXPORT result_tgfx backend_load(registrysys_tapi * regsys, core_tgfx_type * core, tgfx_PrintLogCallback printcallback) {
	return load(regsys, core, printcallback);
}


VK_STATICVECTOR<WINDOW_VKOBJ, 50>& core_public::GETWINDOWs(){
	return hidden->WINDOWs;
}



//While enabling features, some struct should be chained. This struct is to keep data object lifetimes optimal
struct device_features_chainedstructs {
	VkPhysicalDeviceDescriptorIndexingFeatures DescIndexingFeatures = {};
	VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures seperatedepthstencillayouts = {};
	VkPhysicalDeviceBufferDeviceAddressFeatures bufferdeviceaddress = {};
};
//You have to enable some features to use some device extensions
inline void core_functions::Activate_DeviceExtension_Features(GPU_VKOBJ* VKGPU, VkDeviceCreateInfo& device_ci, device_features_chainedstructs& chainer) {
	const void*& dev_ci_Next = device_ci.pNext;
	void** extending_Next = nullptr;


	//Check for seperate RTSlot blending
	if (VKGPU->Supported_Features.independentBlend) {
		VKGPU->Active_Features.independentBlend = VK_TRUE;
		//GPUDesc.isSupported_SeperateRTSlotBlending = true;
	}

	//Activate Descriptor Indexing Features
	if (VKGPU->extensions.ISSUPPORTED_DESCINDEXING()) {
		chainer.DescIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
		chainer.DescIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		chainer.DescIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
		chainer.DescIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
		chainer.DescIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
		chainer.DescIndexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
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

	if (VKGPU->extensions.ISSUPPORTED_SEPERATEDDEPTHSTENCILLAYOUTS()) {
		chainer.seperatedepthstencillayouts.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES;
		chainer.seperatedepthstencillayouts.separateDepthStencilLayouts = VK_TRUE;

		if (extending_Next) {
			*extending_Next = &chainer.seperatedepthstencillayouts;
			extending_Next = &chainer.seperatedepthstencillayouts.pNext;
		}
		else {
			dev_ci_Next = &chainer.seperatedepthstencillayouts;
			extending_Next = &chainer.seperatedepthstencillayouts.pNext;
		}
	}

	if (VKGPU->extensions.ISSUPPORTED_BUFFERDEVICEADRESS()) {
		chainer.bufferdeviceaddress.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_ADDRESS_FEATURES_EXT;
		chainer.bufferdeviceaddress.pNext = nullptr;
		chainer.bufferdeviceaddress.bufferDeviceAddress = VK_TRUE;
		chainer.bufferdeviceaddress.bufferDeviceAddressCaptureReplay = VK_FALSE;
		chainer.bufferdeviceaddress.bufferDeviceAddressMultiDevice = VK_FALSE;
		

		if (extending_Next) {
			*extending_Next = &chainer.bufferdeviceaddress;
			extending_Next = &chainer.bufferdeviceaddress.pNext;
		}
		else {
			dev_ci_Next = &chainer.bufferdeviceaddress;
			extending_Next = &chainer.bufferdeviceaddress.pNext;
		}
	}
}
void core_functions::initialize_secondstage(initializationsecondstageinfo_tgfx_handle info) {
	initialization_secondstageinfo* vkinfo = (initialization_secondstageinfo*)info;
	rendergpu = vkinfo->renderergpu;

#ifndef NO_IMGUI
	if (vkinfo->shouldActivate_DearIMGUI) {
		Create_IMGUI();
	}
#endif

	//Create Logical Device
	{
		std::vector<VkDeviceQueueCreateInfo> queues_ci = queuesys->get_queue_cis(rendergpu);

		VkDeviceCreateInfo Logical_Device_CreationInfo{};
		Logical_Device_CreationInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		Logical_Device_CreationInfo.flags = 0;
		Logical_Device_CreationInfo.pQueueCreateInfos = queues_ci.data();
		Logical_Device_CreationInfo.queueCreateInfoCount = static_cast<uint32_t>(queues_ci.size());
		//This is to destroy datas of extending features
		device_features_chainedstructs chainer_activate;
		Activate_DeviceExtension_Features(rendergpu, Logical_Device_CreationInfo, chainer_activate);

		Logical_Device_CreationInfo.enabledExtensionCount = static_cast<uint32_t>(rendergpu->Active_DeviceExtensions.size());
		Logical_Device_CreationInfo.ppEnabledExtensionNames = rendergpu->Active_DeviceExtensions.data();
		Logical_Device_CreationInfo.pEnabledFeatures = &rendergpu->Active_Features;
		Logical_Device_CreationInfo.enabledLayerCount = 0;
		if (vkCreateDevice(rendergpu->Physical_Device, &Logical_Device_CreationInfo, nullptr, &rendergpu->Logical_Device) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "Vulkan failed to create a Logical Device!");
			return;
		}
		
		queuesys->get_queue_objects(rendergpu);
		rendergpu->EXTMANAGER()->CheckDeviceExtensionProperties(rendergpu);
		gpu_allocator->do_allocations(rendergpu);
	}


	Create_VkRenderer();
	
	Create_GPUContentManager(vkinfo);
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
inline void core_functions::Describe_SupportedExtensions(GPU_VKOBJ* VKGPU) {
	//GET SUPPORTED DEVICE EXTENSIONS
	VkExtensionProperties* Exts = nullptr;	unsigned int ExtsCount = 0;
	vkEnumerateDeviceExtensionProperties(VKGPU->Physical_Device, nullptr, &ExtsCount, nullptr);
	uint32_t extsPTR = vk_virmem::allocate_from_dynamicmem(VKGLOBAL_VIRMEM_CURRENTFRAME, sizeof(VkExtensionProperties) * ExtsCount);
	Exts = (VkExtensionProperties*)VK_MEMOFFSET_TO_POINTER(extsPTR);
	vkEnumerateDeviceExtensionProperties(VKGPU->Physical_Device, nullptr, &ExtsCount, Exts);

	//GET SUPPORTED FEATURES OF THE DEVICE EXTENSIONS
	VkPhysicalDeviceFeatures2 features2;
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.pNext = nullptr;
	device_features_chainedstructs chainer_check;
	{	//Fill pNext to check all necessary features you'll need from the extensions below
		chainer_check.DescIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		chainer_check.seperatedepthstencillayouts.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES;
		chainer_check.bufferdeviceaddress.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_ADDRESS_FEATURES_EXT;

		features2.pNext = &chainer_check.DescIndexingFeatures;
		chainer_check.DescIndexingFeatures.pNext = &chainer_check.seperatedepthstencillayouts;
		chainer_check.seperatedepthstencillayouts.pNext = &chainer_check.bufferdeviceaddress;
	}
	vkGetPhysicalDeviceFeatures2(VKGPU->Physical_Device, &features2);
	

	//Check Seperate_DepthStencil
	if (chainer_check.seperatedepthstencillayouts.separateDepthStencilLayouts) {
		if (Application_Info.apiVersion == VK_API_VERSION_1_0 || Application_Info.apiVersion == VK_API_VERSION_1_1) {
			if (IsExtensionSupported(VK_KHR_SEPARATE_DEPTH_STENCIL_LAYOUTS_EXTENSION_NAME, Exts, ExtsCount)) {
				VKGPU->extensions.SUPPORT_SEPERATEDDEPTHSTENCILLAYOUTS();
				VKGPU->Active_DeviceExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
				if (Application_Info.apiVersion == VK_API_VERSION_1_0) {
					VKGPU->Active_DeviceExtensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);
					VKGPU->Active_DeviceExtensions.push_back(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
				}
			}
		}
		else { VKGPU->extensions.SUPPORT_SEPERATEDDEPTHSTENCILLAYOUTS(); }
	}

	if (IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, Exts, ExtsCount)) {
		VKGPU->extensions.SUPPORT_SWAPCHAINDISPLAY();
		VKGPU->Active_DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}
	else {
		printer(result_tgfx_WARNING, "Current GPU doesn't support to display a swapchain, so you shouldn't use any window related functionality such as: GFXRENDERER->Create_WindowPass, GFX->Create_Window, GFXRENDERER->Swap_Buffers ...");
	}

	if (chainer_check.bufferdeviceaddress.bufferDeviceAddress) {
		if (Application_Info.apiVersion == VK_API_VERSION_1_0 || Application_Info.apiVersion == VK_API_VERSION_1_1) {
			VkPhysicalDeviceBufferDeviceAddressFeatures x;
		}
		else {}
	}
	//Check Descriptor Indexing
	if (chainer_check.DescIndexingFeatures.descriptorBindingVariableDescriptorCount){
		//First check Maintenance3 and PhysicalDeviceProperties2 for Vulkan 1.0
		if (Application_Info.apiVersion == VK_API_VERSION_1_0) {
			if (IsExtensionSupported(VK_KHR_MAINTENANCE3_EXTENSION_NAME, Exts, ExtsCount) && hidden->isSupported_PhysicalDeviceProperties2) {
				VKGPU->Active_DeviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
				if (IsExtensionSupported(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, Exts, ExtsCount)) {
					VKGPU->Active_DeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
					VKGPU->extensions.SUPPORT_VARIABLEDESCCOUNT();
				}
			}
		}
		//Maintenance3 and PhysicalDeviceProperties2 is core in 1.1, so check only Descriptor Indexing
		else {
			//If Vulkan is 1.1, check extension
			if (Application_Info.apiVersion == VK_API_VERSION_1_1) {
				if (IsExtensionSupported(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, Exts, ExtsCount)) {
					VKGPU->Active_DeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
					VKGPU->extensions.SUPPORT_VARIABLEDESCCOUNT();
				}
			}
			//1.2+ Vulkan supports DescriptorIndexing by default.
			else {
				VKGPU->extensions.SUPPORT_VARIABLEDESCCOUNT();
			}
		}
	}
}
void core_functions::Save_Monitors() {
	int monitor_count;
	GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
	for (unsigned int i = 0; i < monitor_count; i++) {
		GLFWmonitor* monitor = monitors[i];

		//Get monitor name provided by OS! It is a driver based name, so it maybe incorrect!
		const char* monitor_name = glfwGetMonitorName(monitor);
		hidden->MONITORs.push_back(MONITOR_VKOBJ());
		MONITOR_VKOBJ* Monitor = hidden->MONITORs[hidden->MONITORs.size() - 1];
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
		Monitor->monitorobj = monitor;
	}
}

std::vector<const char*> Active_InstanceExtensionNames;
std::vector<VkExtensionProperties> Supported_InstanceExtensionList;
bool Check_InstanceExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	for (unsigned int i = 0; i < glfwExtensionCount; i++) {
		if (!IsExtensionSupported(glfwExtensions[i], Supported_InstanceExtensionList.data(), Supported_InstanceExtensionList.size())) {
			printer(result_tgfx_INVALIDARGUMENT, "Your vulkan instance doesn't support extensions that're required by GLFW. This situation is not tested, so report your device to the author!");
			return false;
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
	
	//Check PhysicalDeviceProperties2KHR
	if (Application_Info.apiVersion == VK_API_VERSION_1_0){
		if (!IsExtensionSupported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, Supported_InstanceExtensionList.data(), Supported_InstanceExtensionList.size())) {
			hidden->isSupported_PhysicalDeviceProperties2 = false;
			printer(result_tgfx_FAIL, "Your OS doesn't support Physical Device Properties 2 extension which should be supported, so Vulkan device creation has failed!");
			return false;
		}
		else{ Active_InstanceExtensionNames.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME); }
	}

#ifdef VULKAN_DEBUGGING
	if (IsExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, Supported_InstanceExtensionList.data(), Supported_InstanceExtensionList.size())) {
		Active_InstanceExtensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
#endif
}

void core_functions::Create_Instance() {
	//APPLICATION INFO
	Application_Info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	Application_Info.pApplicationName = "Vulkan DLL";
	Application_Info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	Application_Info.pEngineName = "GFX API";
	Application_Info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	Application_Info.apiVersion = VK_API_VERSION_1_2;

	//CHECK SUPPORTED EXTENSIONs
	uint32_t extension_count;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
	//Doesn't construct VkExtensionProperties object, so we have to use resize!
	Supported_InstanceExtensionList.resize(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, Supported_InstanceExtensionList.data());
	if (!Check_InstanceExtensions()) {
		return;
	}

	//CHECK SUPPORTED LAYERS
	unsigned int Supported_LayerNumber = 0;
	vkEnumerateInstanceLayerProperties(&Supported_LayerNumber, nullptr);
	VkLayerProperties* Supported_LayerList = (VkLayerProperties*)VK_ALLOCATE_AND_GETPTR(VKGLOBAL_VIRMEM_CURRENTFRAME, sizeof(VkLayerProperties) * Supported_LayerNumber);
	vkEnumerateInstanceLayerProperties(&Supported_LayerNumber, Supported_LayerList);

	//INSTANCE CREATION INFO
	VkInstanceCreateInfo InstCreation_Info = {};
	InstCreation_Info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	InstCreation_Info.pApplicationInfo = &Application_Info;
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
}


void core_functions::Gather_PhysicalDeviceInformations(GPU_VKOBJ* VKGPU) {
	vkGetPhysicalDeviceFeatures(VKGPU->Physical_Device, &VKGPU->Supported_Features);

	VKGPU->Device_Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	VkPhysicalDeviceMaintenance3Properties props; props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES; props.pNext = nullptr;
	VKGPU->Device_Properties.pNext = &props;
	vkGetPhysicalDeviceProperties2(VKGPU->PHYSICALDEVICE(), &VKGPU->Device_Properties);

	//GET QUEUE FAMILIES, SAVE THEM TO GPU OBJECT, CHECK AND SAVE GRAPHICS,COMPUTE,TRANSFER QUEUEFAMILIES INDEX
	vkGetPhysicalDeviceQueueFamilyProperties(VKGPU->Physical_Device, &VKGPU->QueueFamiliesCount, nullptr);

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
	//CHECK GPUs
	uint32_t GPU_NUMBER = 0;
	vkEnumeratePhysicalDevices(Vulkan_Instance, &GPU_NUMBER, nullptr);
	VkPhysicalDevice* Physical_GPU_LIST = (VkPhysicalDevice*)VK_ALLOCATE_AND_GETPTR(VKGLOBAL_VIRMEM_CURRENTFRAME, sizeof(VkPhysicalDevice) * GPU_NUMBER);
	vkEnumeratePhysicalDevices(Vulkan_Instance, &GPU_NUMBER, Physical_GPU_LIST);

	if (GPU_NUMBER == 0) {
		printer(result_tgfx_FAIL, "There is no GPU that has Vulkan support! Updating your drivers or Upgrading the OS may help");
	}


	//GET GPU INFORMATIONs, QUEUE FAMILIES etc
	for (unsigned int i = 0; i < GPU_NUMBER; i++) {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(Physical_GPU_LIST[i], &props);
		if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			//GPU initializer handles everything else
			hidden->DEVICE_GPUs.push_back(GPU_VKOBJ());
			GPU_VKOBJ* vkgpu = hidden->DEVICE_GPUs[hidden->DEVICE_GPUs.size() - 1];
			vkgpu->hidden = nullptr;	//there is no private gpu data for now
			vkgpu->Physical_Device = Physical_GPU_LIST[i];

			Gather_PhysicalDeviceInformations(vkgpu);
			//SAVE BASIC INFOs TO THE GPU DESC
			vkgpu->desc.NAME = vkgpu->DEVICEPROPERTIES().deviceName;
			vkgpu->desc.DRIVER_VERSION = vkgpu->DEVICEPROPERTIES().driverVersion;
			vkgpu->desc.API_VERSION = vkgpu->DEVICEPROPERTIES().apiVersion;
			vkgpu->desc.DRIVER_VERSION = vkgpu->DEVICEPROPERTIES().driverVersion;

			gpu_allocator->analize_gpumemory(vkgpu);
			queuesys->analize_queues(vkgpu);
			Describe_SupportedExtensions(vkgpu);
			vkgpu->EXTMANAGER()->CheckDeviceExtensionProperties(vkgpu);
		}
		else{
			printer(result_tgfx_WARNING, "Non-Discrete GPUs are not available in vulkan backend!");
		}
	}
}




bool Create_WindowSwapchain(WINDOW_VKOBJ* Vulkan_Window, unsigned int WIDTH, unsigned int HEIGHT, VkSwapchainKHR* SwapchainOBJ, TEXTURE_VKOBJ** SwapchainTextureHandles) {
	//Choose Surface Format
	VkSurfaceFormatKHR Window_SurfaceFormat = {};
	for (unsigned int i = 0; i < 100; i++) {
		VkSurfaceFormatKHR& SurfaceFormat = Vulkan_Window->SurfaceFormats[i];
		if (SurfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && SurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			Window_SurfaceFormat = SurfaceFormat;
		}
	}

	//Choose Surface Presentation Mode
	VkPresentModeKHR Window_PresentationMode = VK_PRESENT_MODE_FIFO_KHR;
	if (!Vulkan_Window->presentationmodes.fifo) { printer(result_tgfx_FAIL, "Swapchain doesn't support FIFO presentation mode, report this to authors to get support for other presentation modes!"); }


	VkExtent2D Window_ImageExtent = { WIDTH, HEIGHT };
	if (!Vulkan_Window->SurfaceCapabilities.maxImageCount > Vulkan_Window->SurfaceCapabilities.minImageCount) {
		printer(result_tgfx_NOTCODED, "VulkanCore: Window Surface Capabilities have issues, maxImageCount <= minImageCount!");
		return false;
	}
	VkSwapchainCreateInfoKHR swpchn_ci = {};
	swpchn_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swpchn_ci.flags = 0;
	swpchn_ci.pNext = nullptr;
	swpchn_ci.presentMode = Window_PresentationMode;
	swpchn_ci.surface = Vulkan_Window->Window_Surface;
	swpchn_ci.minImageCount = VKCONST_SwapchainTextureCountPerWindow;
	swpchn_ci.imageFormat = Window_SurfaceFormat.format;
	swpchn_ci.imageColorSpace = Window_SurfaceFormat.colorSpace;
	swpchn_ci.imageExtent = Window_ImageExtent;
	swpchn_ci.imageArrayLayers = 1;
	//Swapchain texture can be used as framebuffer, but we should set its bit!
	if (Vulkan_Window->SWAPCHAINUSAGE & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) { 
		Vulkan_Window->SWAPCHAINUSAGE &= ~(1UL << 5); }
	swpchn_ci.imageUsage = Vulkan_Window->SWAPCHAINUSAGE;

	swpchn_ci.clipped = VK_TRUE;
	swpchn_ci.preTransform = Vulkan_Window->SurfaceCapabilities.currentTransform;
	swpchn_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if (Vulkan_Window->Window_SwapChain) { swpchn_ci.oldSwapchain = Vulkan_Window->Window_SwapChain; }
	else { swpchn_ci.oldSwapchain = nullptr; }

	if (rendergpu->QUEUEFAMSCOUNT() > 1) { swpchn_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT; }
	else { swpchn_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; }
	swpchn_ci.pQueueFamilyIndices = rendergpu->ALLQUEUEFAMILIES();
	swpchn_ci.queueFamilyIndexCount = rendergpu->QUEUEFAMSCOUNT();

	if (vkCreateSwapchainKHR(rendergpu->LOGICALDEVICE(), &swpchn_ci, nullptr, SwapchainOBJ) != VK_SUCCESS) {
		printer(result_tgfx_FAIL, "VulkanCore: Failed to create a SwapChain for a Window");
		return false;
	}

	//Get Swapchain images
	uint32_t created_imagecount = 0;
	vkGetSwapchainImagesKHR(rendergpu->LOGICALDEVICE(), *SwapchainOBJ, &created_imagecount, nullptr);
	if (created_imagecount != VKCONST_SwapchainTextureCountPerWindow) {
		printer(result_tgfx_FAIL, "TGFX API asked for swapchain textures but Vulkan driver gave less number of textures than intended!");
		return false;
	}
	VkImage SWPCHN_IMGs[VKCONST_SwapchainTextureCountPerWindow];
	vkGetSwapchainImagesKHR(rendergpu->LOGICALDEVICE(), *SwapchainOBJ, &created_imagecount, SWPCHN_IMGs);
	TEXTURE_VKOBJ textures_temp[VKCONST_SwapchainTextureCountPerWindow] = {};
	for (unsigned int vkim_index = 0; vkim_index < VKCONST_SwapchainTextureCountPerWindow; vkim_index++) {
		TEXTURE_VKOBJ* SWAPCHAINTEXTURE = &textures_temp[vkim_index];
		SWAPCHAINTEXTURE->CHANNELs = texture_channels_tgfx_BGRA8UNORM;
		SWAPCHAINTEXTURE->WIDTH = WIDTH;
		SWAPCHAINTEXTURE->HEIGHT = HEIGHT;
		SWAPCHAINTEXTURE->DATA_SIZE = SWAPCHAINTEXTURE->WIDTH * SWAPCHAINTEXTURE->HEIGHT * 4;
		SWAPCHAINTEXTURE->Image = SWPCHN_IMGs[vkim_index];
		SWAPCHAINTEXTURE->MIPCOUNT = 1;
	}

	for (unsigned int i = 0; i < VKCONST_SwapchainTextureCountPerWindow; i++) {
		VkImageViewCreateInfo ImageView_ci = {};
		ImageView_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		TEXTURE_VKOBJ* SwapchainTexture = &textures_temp[i];
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
	for (unsigned int i = 0; i < VKCONST_SwapchainTextureCountPerWindow; i++) {
		SwapchainTextureHandles[i] = contentmanager->GETTEXTURES_ARRAY().create_OBJ();
		*SwapchainTextureHandles[i] = textures_temp[i];
	}
	return true;
}

#include "synchronization_sys.h"
void GLFWwindowresizecallback(GLFWwindow* glfwwindow, int width, int height) {
	WINDOW_VKOBJ* vkwindow = (WINDOW_VKOBJ*)glfwGetWindowUserPointer(glfwwindow);
	vkwindow->resize_cb((window_tgfx_handle)vkwindow, vkwindow->UserPTR, width, height, (texture_tgfx_handle*)vkwindow->Swapchain_Textures);
}
void core_functions::createwindow_vk(unsigned int WIDTH, unsigned int HEIGHT, monitor_tgfx_handle monitor,
	windowmode_tgfx Mode, const char* NAME, tgfx_windowResizeCallback ResizeCB, void* UserPointer, window_tgfx_handle* window) {
	if (ResizeCB) { glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); }
	else { glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); }
	GLFWwindow* glfw_window = glfwCreateWindow(WIDTH, HEIGHT, NAME, nullptr, nullptr);
	WINDOW_VKOBJ Vulkan_Window;
	Vulkan_Window.LASTWIDTH = WIDTH;
	Vulkan_Window.NEWWIDTH = WIDTH;
	Vulkan_Window.LASTHEIGHT = HEIGHT;
	Vulkan_Window.NEWHEIGHT = HEIGHT;
	Vulkan_Window.DISPLAYMODE = Mode;
	Vulkan_Window.MONITOR = nullptr;
	Vulkan_Window.NAME = NAME;
	Vulkan_Window.GLFW_WINDOW = glfw_window;
	Vulkan_Window.SWAPCHAINUSAGE = 0;	//This will be set while creating swapchain
	Vulkan_Window.resize_cb = ResizeCB;
	Vulkan_Window.UserPTR = UserPointer;

	//Check and Report if GLFW fails
	if (glfw_window == NULL) {
		printer(result_tgfx_FAIL, "VulkanCore: We failed to create the window because of GLFW!");
		return;
	}

	//Window VulkanSurface Creation
	VkSurfaceKHR Window_Surface = {};
	if (glfwCreateWindowSurface(Vulkan_Instance, Vulkan_Window.GLFW_WINDOW, nullptr, &Window_Surface) != VK_SUCCESS) {
		printer(result_tgfx_FAIL, "GLFW failed to create a window surface");
		return;
	}
	Vulkan_Window.Window_Surface = Window_Surface;

	if (ResizeCB) {
		glfwSetWindowUserPointer(Vulkan_Window.GLFW_WINDOW, Vulkan_Window.UserPTR);
		glfwSetWindowSizeCallback(Vulkan_Window.GLFW_WINDOW, GLFWwindowresizecallback);
	}


	//Finding GPU_TO_RENDER's Surface Capabilities
	Vulkan_Window.presentationqueue = queuesys->check_windowsupport(rendergpu, Vulkan_Window.Window_Surface);
	if (!Vulkan_Window.presentationqueue) {
		printer(result_tgfx_FAIL, "Vulkan backend supports windows that your GPU supports but your GPU doesn't support current window. So window creation has failed!");
		return;
	}

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(rendergpu->PHYSICALDEVICE(), Vulkan_Window.Window_Surface, &Vulkan_Window.SurfaceCapabilities);
	uint32_t FormatCount = 100;
	ThrowIfFailed(vkGetPhysicalDeviceSurfaceFormatsKHR(rendergpu->PHYSICALDEVICE(), Vulkan_Window.Window_Surface, &FormatCount, Vulkan_Window.SurfaceFormats), "vkGetPhysicalDeviceSurfaceFormatsKHR failed!");
	if (FormatCount > VKCONST_MAXSURFACEFORMATCOUNT) {
		printer(result_tgfx_WARNING, "Current window has VKCONST_MAXSURFACEFORMATCOUNT+ supported swapchain formats, which backend supports only VKCONST_MAXSURFACEFORMATCOUNT. Report this issue!");
	}
	if(!FormatCount) {
		printer(result_tgfx_FAIL, "This GPU doesn't support this type of windows, please try again with a different window configuration!");
		return;
	}
	if (!Vulkan_Window.SurfaceCapabilities.maxImageCount > Vulkan_Window.SurfaceCapabilities.minImageCount) {
		printer(result_tgfx_FAIL, "VulkanCore: Window Surface Capabilities have issues, maxImageCount <= minImageCount!");
		return;
	}


	uint32_t PresentationModesCount = 10;
	VkPresentModeKHR Presentations[10];		//10 is enough, considering future modes
	vkGetPhysicalDeviceSurfacePresentModesKHR(rendergpu->PHYSICALDEVICE(), Vulkan_Window.Window_Surface, &PresentationModesCount, Presentations);
	for (unsigned int i = 0; i < PresentationModesCount; i++) {
		switch (Presentations[i]) {
		case VK_PRESENT_MODE_FIFO_KHR: Vulkan_Window.presentationmodes.fifo = true; break;
		case VK_PRESENT_MODE_FIFO_RELAXED_KHR: Vulkan_Window.presentationmodes.fifo_relaxed = true; break;
		case VK_PRESENT_MODE_IMMEDIATE_KHR: Vulkan_Window.presentationmodes.immediate = true; break;
		case VK_PRESENT_MODE_MAILBOX_KHR: Vulkan_Window.presentationmodes.mailbox = true; break;
		}
	}


	hidden->WINDOWs.push_back(Vulkan_Window);
	VKOBJHANDLE handle = hidden->WINDOWs.returnHANDLEfromOBJ(hidden->WINDOWs[hidden->WINDOWs.size() - 1]);
	*window = *(window_tgfx_handle*)&handle;
}
result_tgfx core_functions::create_swapchain(window_tgfx_handle window, windowpresentation_tgfx presentationMode, windowcomposition_tgfx composition,
	colorspace_tgfx colorSpace, texture_channels_tgfx channels, textureusageflag_tgfx_handle SwapchainUsage, texture_tgfx_handle* SwapchainTextureHandles) {
	WINDOW_VKOBJ* Vulkan_Window = hidden->WINDOWs.getOBJfromHANDLE(*(VKOBJHANDLE*)&window);
	if (VKCONST_isPointerContainVKFLAG) { Vulkan_Window->SWAPCHAINUSAGE = *(VkImageUsageFlags*)&SwapchainUsage; }
	else { Vulkan_Window->SWAPCHAINUSAGE = *(VkImageUsageFlags*)SwapchainUsage; }

	TEXTURE_VKOBJ* SWPCHNTEXTUREHANDLESVK[2];

	//Choose Surface Format
	VkSurfaceFormatKHR Window_SurfaceFormat = {};
	for (unsigned int i = 0; i < 100; i++) {
		VkSurfaceFormatKHR& SurfaceFormat = Vulkan_Window->SurfaceFormats[i];
		if (SurfaceFormat.format == Find_VkFormat_byTEXTURECHANNELs(channels) && SurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			Window_SurfaceFormat = SurfaceFormat;
		}
	}

	//Create VkSwapchainKHR Object
	{

		VkSwapchainCreateInfoKHR swpchn_ci = {};
		swpchn_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swpchn_ci.flags = 0;
		swpchn_ci.pNext = nullptr;
		swpchn_ci.presentMode = Find_VkPresentMode_byTGFXPresent(presentationMode);
		swpchn_ci.surface = Vulkan_Window->Window_Surface;
		swpchn_ci.minImageCount = VKCONST_SwapchainTextureCountPerWindow;
		swpchn_ci.imageFormat = Window_SurfaceFormat.format;
		swpchn_ci.imageColorSpace = Window_SurfaceFormat.colorSpace;
		swpchn_ci.imageExtent = { Vulkan_Window->NEWWIDTH, Vulkan_Window->NEWHEIGHT };
		swpchn_ci.imageArrayLayers = 1;
		//Swapchain texture can be used as framebuffer, but we should set its bit!
		if (Vulkan_Window->SWAPCHAINUSAGE & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
			Vulkan_Window->SWAPCHAINUSAGE &= ~(1UL << 5);
		}
		swpchn_ci.imageUsage = Vulkan_Window->SWAPCHAINUSAGE;

		swpchn_ci.clipped = VK_TRUE;
		swpchn_ci.preTransform = Vulkan_Window->SurfaceCapabilities.currentTransform;
		swpchn_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		if (Vulkan_Window->Window_SwapChain) { swpchn_ci.oldSwapchain = Vulkan_Window->Window_SwapChain; }
		else { swpchn_ci.oldSwapchain = nullptr; }

		if (rendergpu->QUEUEFAMSCOUNT() > 1) { swpchn_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT; }
		else { swpchn_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; }
		swpchn_ci.pQueueFamilyIndices = rendergpu->ALLQUEUEFAMILIES();
		swpchn_ci.queueFamilyIndexCount = rendergpu->QUEUEFAMSCOUNT();

		if (vkCreateSwapchainKHR(rendergpu->LOGICALDEVICE(), &swpchn_ci, nullptr, &Vulkan_Window->Window_SwapChain) != VK_SUCCESS) {
			printer(result_tgfx_FAIL, "VulkanCore: Failed to create a SwapChain for a Window");
			return result_tgfx_FAIL;
		}
	}

	//Get Swapchain images
	{
		uint32_t created_imagecount = 0;
		vkGetSwapchainImagesKHR(rendergpu->LOGICALDEVICE(), Vulkan_Window->Window_SwapChain, &created_imagecount, nullptr);
		if (created_imagecount != VKCONST_SwapchainTextureCountPerWindow) {
			printer(result_tgfx_FAIL, "VK backend asked for swapchain textures but Vulkan driver gave less number of textures than intended!");
			return result_tgfx_FAIL;
		}
		VkImage SWPCHN_IMGs[VKCONST_SwapchainTextureCountPerWindow];
		vkGetSwapchainImagesKHR(rendergpu->LOGICALDEVICE(), Vulkan_Window->Window_SwapChain, &created_imagecount, SWPCHN_IMGs);
		TEXTURE_VKOBJ textures_temp[VKCONST_SwapchainTextureCountPerWindow] = {};
		for (unsigned int vkim_index = 0; vkim_index < VKCONST_SwapchainTextureCountPerWindow; vkim_index++) {
			TEXTURE_VKOBJ* SWAPCHAINTEXTURE = &textures_temp[vkim_index];
			SWAPCHAINTEXTURE->CHANNELs = texture_channels_tgfx_BGRA8UNORM;
			SWAPCHAINTEXTURE->WIDTH = Vulkan_Window->NEWWIDTH;
			SWAPCHAINTEXTURE->HEIGHT = Vulkan_Window->NEWHEIGHT;
			SWAPCHAINTEXTURE->DATA_SIZE = SWAPCHAINTEXTURE->WIDTH * SWAPCHAINTEXTURE->HEIGHT * 4;
			SWAPCHAINTEXTURE->Image = SWPCHN_IMGs[vkim_index];
			SWAPCHAINTEXTURE->MIPCOUNT = 1;
		}

		for (unsigned int i = 0; i < VKCONST_SwapchainTextureCountPerWindow; i++) {
			VkImageViewCreateInfo ImageView_ci = {};
			ImageView_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			TEXTURE_VKOBJ* SwapchainTexture = &textures_temp[i];
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
				return result_tgfx_FAIL;
			}
		}
		for (unsigned int i = 0; i < VKCONST_SwapchainTextureCountPerWindow; i++) {
			SWPCHNTEXTUREHANDLESVK[i] = contentmanager->GETTEXTURES_ARRAY().create_OBJ();
			*SWPCHNTEXTUREHANDLESVK[i] = textures_temp[i];
		}
	}


	SWPCHNTEXTUREHANDLESVK[0]->USAGE = Vulkan_Window->SWAPCHAINUSAGE;
	SWPCHNTEXTUREHANDLESVK[1]->USAGE = Vulkan_Window->SWAPCHAINUSAGE;
	Vulkan_Window->Swapchain_Textures[0] = contentmanager->GETTEXTURES_ARRAY().getINDEXbyOBJ(SWPCHNTEXTUREHANDLESVK[0]);
	Vulkan_Window->Swapchain_Textures[1] = contentmanager->GETTEXTURES_ARRAY().getINDEXbyOBJ(SWPCHNTEXTUREHANDLESVK[1]);
	VKOBJHANDLE handles[2] = { contentmanager->GETTEXTURES_ARRAY().returnHANDLEfromOBJ(SWPCHNTEXTUREHANDLESVK[0]), contentmanager->GETTEXTURES_ARRAY().returnHANDLEfromOBJ(SWPCHNTEXTUREHANDLESVK[1]) };
	SwapchainTextureHandles[0] = *(texture_tgfx_handle*)&handles[0];
	SwapchainTextureHandles[1] = *(texture_tgfx_handle*)&handles[1];
	return result_tgfx_SUCCESS;
}
void take_inputs() {
	glfwPollEvents();
	if (hidden->isAnyWindowResized) {
		vkDeviceWaitIdle(rendergpu->LOGICALDEVICE());
		for (unsigned int WindowIndex = 0; WindowIndex < hidden->WINDOWs.size(); WindowIndex++) {
			WINDOW_VKOBJ* VKWINDOW = hidden->WINDOWs[WindowIndex];
			if (!VKWINDOW->isALIVE.load()) { continue; }
			if (!VKWINDOW->isResized) {
				continue;
			}
			if (VKWINDOW->NEWWIDTH == VKWINDOW->LASTWIDTH && VKWINDOW->NEWHEIGHT == VKWINDOW->LASTHEIGHT) {
				VKWINDOW->isResized = false;
				continue;
			}

			printer(result_tgfx_NOTCODED, "Resize functionality in take_inputs() was designed before swapchain creation exposed, so please re-design it");


			/*
			VkSwapchainKHR swpchn;
			TEXTURE_VKOBJ* swpchntextures[VKCONST_SwapchainTextureCountPerWindow] = {};
			//If new window size isn't able to create a swapchain, return to last window size
			if (!Create_WindowSwapchain(VKWINDOW, VKWINDOW->NEWWIDTH, VKWINDOW->NEWHEIGHT, &swpchn, swpchntextures)) {
				printer(result_tgfx_FAIL, "New size for the window is not possible, returns to the last successful size!");
				glfwSetWindowSize(VKWINDOW->GLFW_WINDOW, VKWINDOW->LASTWIDTH, VKWINDOW->LASTHEIGHT);
				VKWINDOW->isResized = false;
				continue;
			}

			TEXTURE_VKOBJ* oldswpchn[2] = { contentmanager->GETTEXTURES_ARRAY().getOBJbyINDEX(VKWINDOW->Swapchain_Textures[0]),
				contentmanager->GETTEXTURES_ARRAY().getOBJbyINDEX(VKWINDOW->Swapchain_Textures[1]) };
			for (unsigned int texture_i = 0; texture_i < VKCONST_SwapchainTextureCountPerWindow; texture_i++) {
				vkDestroyImageView(rendergpu->LOGICALDEVICE(), oldswpchn[texture_i]->ImageView, nullptr);
				oldswpchn[texture_i]->Image = VK_NULL_HANDLE;
				oldswpchn[texture_i]->ImageView = VK_NULL_HANDLE;
				core_tgfx_main->contentmanager->Delete_Texture((texture_tgfx_handle)oldswpchn[texture_i]);
			}
			vkDestroySwapchainKHR(rendergpu->LOGICALDEVICE(), VKWINDOW->Window_SwapChain, nullptr);

			VKWINDOW->LASTHEIGHT = VKWINDOW->NEWHEIGHT;
			VKWINDOW->LASTWIDTH = VKWINDOW->NEWWIDTH;
			//When you resize window at Frame1, user'd have to track swapchain texture state if I don't do this here
			//So please don't touch!
			VKWINDOW->Swapchain_Textures[0] = core_tgfx_main->renderer->GetCurrentFrameIndex() ? contentmanager->GETTEXTURES_ARRAY().getINDEXbyOBJ(swpchntextures[1]) : contentmanager->GETTEXTURES_ARRAY().getINDEXbyOBJ(swpchntextures[0]);
			VKWINDOW->Swapchain_Textures[1] = core_tgfx_main->renderer->GetCurrentFrameIndex() ? contentmanager->GETTEXTURES_ARRAY().getINDEXbyOBJ(swpchntextures[0]) : contentmanager->GETTEXTURES_ARRAY().getINDEXbyOBJ(swpchntextures[1]);
			VKWINDOW->Window_SwapChain = swpchn;
			VKWINDOW->CurrentFrameSWPCHNIndex = 0;
			VKWINDOW->resize_cb((window_tgfx_handle)VKWINDOW, VKWINDOW->UserPTR, VKWINDOW->NEWWIDTH, VKWINDOW->NEWHEIGHT, (texture_tgfx_handle*)VKWINDOW->Swapchain_Textures);
			*/
		}
	}
}


inline void core_functions::getmonitorlist(monitor_tgfx_listhandle* MonitorList) {
	*MonitorList = (monitor_tgfx_handle*)(uintptr_t(VKCONST_VIRMEMSPACE_BEGIN) + vk_virmem::allocate_from_dynamicmem(VKGLOBAL_VIRMEM_CURRENTFRAME, sizeof(VKDATAHANDLE) * (hidden->MONITORs.size() + 1)));
	for (unsigned int i = 0; i < hidden->MONITORs.size(); i++) {
		(*MonitorList)[i] = (monitor_tgfx_handle)hidden->MONITORs[i];
	}
	(*MonitorList)[hidden->MONITORs.size()] = (monitor_tgfx_handle)core_tgfx_main->INVALIDHANDLE;
}
inline void core_functions::getGPUlist(gpu_tgfx_listhandle* GPULIST) {
	*GPULIST = (gpu_tgfx_listhandle)(uintptr_t(VKCONST_VIRMEMSPACE_BEGIN) + vk_virmem::allocate_from_dynamicmem(VKGLOBAL_VIRMEM_CURRENTFRAME, sizeof(VKDATAHANDLE) * (hidden->DEVICE_GPUs.size() + 1)));
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