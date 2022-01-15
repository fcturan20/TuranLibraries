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
#include <logger_tapi.h>
#include "resource.h"
#include <tgfx_helper.h>
#include <tgfx_renderer.h>
#include <tgfx_gpucontentmanager.h>
#include <tgfx_imgui.h>

struct core_private{
public:
	std::vector<gpu_public*> DEVICE_GPUs;
	//Window Operations
	std::vector<monitor_vk*> MONITORs;
	std::vector<window_vk*> WINDOWs;

	//Instead of checking each window each frame, just check this
	bool isAnyWindowResized = false;
};
static core_private* hidden = nullptr;
static logger_tapi* logsys = nullptr;

struct gpu_private {

	struct suballocation_vk {
		VkDeviceSize Size = 0, Offset = 0;
		std::atomic<bool> isEmpty;
		suballocation_vk();
		suballocation_vk(const suballocation_vk& copyblock);
	};
	struct memoryallocation_vk {
		std::atomic<uint32_t> UnusedSize = 0;
		uint32_t MemoryTypeIndex;
		unsigned long long MaxSize = 0, ALLOCATIONSIZE = 0;
		void* MappedMemory;
		memoryallocationtype_tgfx TYPE;

		VkDeviceMemory Allocated_Memory;
		VkBuffer Buffer;
		threadlocal_vector<suballocation_vk> Allocated_Blocks;
		memoryallocation_vk();
		memoryallocation_vk(const memoryallocation_vk& copyALLOC);
		//Use this function in Suballocate_Buffer and Suballocate_Image after you're sure that you have enough memory in the allocation
		VkDeviceSize FindAvailableOffset(VkDeviceSize RequiredSize, VkDeviceSize AlignmentOffset, VkDeviceSize RequiredAlignment);
		//Free a block
		void FreeBlock(VkDeviceSize Offset);
	};
	std::vector<memoryallocation_vk> ALLOCs;
public:
	inline std::vector<memoryallocation_vk>& ALLOCS() { return ALLOCs; }
};


void GFX_Error_Callback(int error_code, const char* description) {
	printf("TGFX_FAIL: %s\n", description);
}

class core_functions {
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

	static inline void core_functions::Gather_PhysicalDeviceInformations(gpu_public* VKGPU);
};


static void Create_IMGUI();
static void Create_Renderer();
static void Create_GPUContentManager();
static void set_helper_functions();


void printf_log(result_tgfx result, const char* text) {
	printf("TGFX %u: %s", (unsigned int)result, text);
}
void loggersys_log(result_tgfx result, const char* text) {
	logsys->log_status(text);
}
result_tgfx load(registrysys_tapi* regsys, core_tgfx_type* core) {
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

	LOGGER_TAPI_PLUGIN_LOAD_TYPE logsys_type = (LOGGER_TAPI_PLUGIN_LOAD_TYPE)regsys->get(LOGGER_TAPI_PLUGIN_NAME, LOGGER_TAPI_PLUGIN_VERSION, 0);
	if (logsys_type) { logsys = logsys_type->funcs; printer = loggersys_log; }
	else {printer = printf_log;}

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
	//set_helper_functions();


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
extern "C" FUNC_DLIB_EXPORT result_tgfx backend_load(registrysys_tapi* regsys, core_tgfx_type* core){
	return load(regsys, core);
 }
void core_functions::initialize_secondstage(initializationsecondstageinfo_tgfx_handle info) {
	initialization_secondstageinfo* vkinfo = (initialization_secondstageinfo*)info;
	rendergpu = vkinfo->renderergpu;

	if (vkinfo->shouldActivate_DearIMGUI) {
		//Create_IMGUI();
	}


	//Create_Renderer();
	//Create_GPUContentManager();
	delete vkinfo;
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

/*
void core_functions::Gather_PhysicalDeviceInformations(gpu_public* VKGPU) {
	vkGetPhysicalDeviceProperties(VKGPU->PHYSICALDEVICE(), &VKGPU->Device_Properties);
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
				GFX_API::MemoryType MEMTYPE;
				MEMTYPE.HEAPTYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::FASTHOSTVISIBLE;
				MEMTYPE.MemoryTypeIndex = GPUdesc.MEMTYPEs.size();
				GPUdesc.MEMTYPEs.push_back(MEMTYPE);
				VK_MemoryAllocation alloc;
				alloc.MemoryTypeIndex = MemoryTypeIndex;
				alloc.TYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::FASTHOSTVISIBLE;
				VKGPU->ALLOCs.push_back(alloc);
				GPUdesc.FASTHOSTVISIBLE_MaxMemorySize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				LOG_STATUS_TAPI("Found FAST HOST VISIBLE BIT! Size: " + to_string(GPUdesc.FASTHOSTVISIBLE_MaxMemorySize));
			}
			else {
				GFX_API::MemoryType MEMTYPE;
				MEMTYPE.HEAPTYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL;
				MEMTYPE.MemoryTypeIndex = GPUdesc.MEMTYPEs.size();
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
				GFX_API::MemoryType MEMTYPE;
				MEMTYPE.HEAPTYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::READBACK;
				MEMTYPE.MemoryTypeIndex = GPUdesc.MEMTYPEs.size();
				GPUdesc.MEMTYPEs.push_back(MEMTYPE);
				VK_MemoryAllocation alloc;
				alloc.MemoryTypeIndex = MemoryTypeIndex;
				alloc.TYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::READBACK;
				VKGPU->ALLOCs.push_back(alloc);
				GPUdesc.READBACK_MaxMemorySize = VKGPU->MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				LOG_STATUS_TAPI("Found READBACK BIT! Size: " + to_string(GPUdesc.READBACK_MaxMemorySize));
			}
			else {
				GFX_API::MemoryType MEMTYPE;
				MEMTYPE.HEAPTYPE = GFX_API::SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE;
				MEMTYPE.MemoryTypeIndex = GPUdesc.MEMTYPEs.size();
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
		VKQUEUE.QueueFamilyIndex = static_cast<uint32_t>(queuefamily_index);
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
}*/

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

	//GET GPU INFORMATIONs, QUEUE FAMILIES etc
	for (unsigned int i = 0; i < GPU_NUMBER; i++) {
		//GPU initializer handles everything else
		gpu_public* vkgpu = new gpu_public;
		vkgpu->hidden = new gpu_private;
		vkgpu->physicaldevice = Physical_GPU_LIST[i];
		hidden->DEVICE_GPUs.push_back(vkgpu);




		/*
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

	printer(result_tgfx_SUCCESS, "Finished checking Computer Specifications!");
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

	if (!rendergpu->isWindowSupported(Vulkan_Window->Window_Surface)) {
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