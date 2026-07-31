#include <vector>
#include <stdio.h>

#define TCORE_USE_CPP_WRAPPER
#include "TCore.h"
#include "TGfxCore.h"

#include "vk_contentmanager.h"
#include "vk_core.h"
#include "vk_predefinitions.h"
#include "vk_queue.h"
#include "vk_renderer.h"

TCORE_PLUGIN_DEFINE(TGfxVulkan, "TGFX Vulkan backend", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0));
TCORE_PLUGIN_INIT(TGfxVulkan)
TCORE_PLUGIN_INIT(TGfx)
TCORE_PLUGIN_INIT(TCAllocator)
TCORE_PLUGIN_INIT(TCLog)
TCORE_PLUGIN_MEMORY_BLOCK_INIT()

TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TGfxVulkan)
TCORE_PLUGIN_RESERVE_ADDRESS_SPACE(1ull << 30);
TCORE_PLUGIN_ENTRY_POINT_END()

TGfxBackendFunctions GBackendFunctions{};

namespace TGFX
{
namespace Vulkan
{
inline bool CheckInstanceExtensionSupported(const char* extName)
{
	bool Is_Found = false;
	for (uint32_t supportedExtIdx = 0; supportedExtIdx < GContext->VKGLOBAL_MAX_INSTANCE_EXT_COUNT; supportedExtIdx++)
	{
		VkExtensionProperties& ext = GContext->SupportedInstanceExtensions[supportedExtIdx];
		if (!ext.extensionName)
		{
			break;
		}
		if (strcmp(extName, ext.extensionName))
		{
			return true;
		}
	}
	char wExtName[1024] = {};
	strcpy_s(wExtName, 1023, extName);
	vkPrint(8, wExtName);
	return false;
}
bool CheckInstanceExtensions(const char** activeInstanceExts)
{
	TU8 instanceExtCount = 0;

	if (CheckInstanceExtensionSupported(VK_KHR_SURFACE_EXTENSION_NAME))
		activeInstanceExts[instanceExtCount++] = VK_KHR_SURFACE_EXTENSION_NAME;
	else
	{
		vkPrint(10);
		return false;
	}

	// Check PhysicalDeviceProperties2KHR
	if (GVkAppInfo.apiVersion == VK_API_VERSION_1_0)
	{
		if (!CheckInstanceExtensionSupported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			vkPrint(8, "Physical Device Properties 2, so Vulkan device creation has failed!");
			return false;
		}
		else
		{
			activeInstanceExts[instanceExtCount++] = (VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		}
	}

#ifdef VK_VALIDATION_LAYER
	if (CheckInstanceExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
	{
		activeInstanceExts[instanceExtCount++] = (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
#endif
	return true;
}
void CreateInstance()
{
	if (auto r = volkInitialize(); r != VK_SUCCESS)
		return;

	// APPLICATION INFO
	GVkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	GVkAppInfo.pApplicationName = "TGFX Vulkan backend";
	GVkAppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	GVkAppInfo.pEngineName = "GFX API";
	GVkAppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	GVkAppInfo.apiVersion = VK_API_VERSION_1_3;

	// CHECK SUPPORTED EXTENSIONs
	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, GContext->SupportedInstanceExtensions);
	if (extCount > GContext->VKGLOBAL_MAX_INSTANCE_EXT_COUNT)
	{
		vkPrint(2, "Vulkan Instance support more extension than backend has imagined, report this please!");
		return;
	}
	const char* activeInstanceExts[GContext->VKGLOBAL_MAX_INSTANCE_EXT_COUNT] = {};
	if (!CheckInstanceExtensions(activeInstanceExts))
		return;
	TU8 activeInstanceExtCount = 0;
	while (activeInstanceExts[activeInstanceExtCount] != nullptr)
		activeInstanceExtCount++;

	// CHECK SUPPORTED LAYERS
	VkConstU4 maxVkLayerCount = 256;
	unsigned int Supported_LayerNumber = 0;
	vkEnumerateInstanceLayerProperties(&Supported_LayerNumber, nullptr);
	if (Supported_LayerNumber > maxVkLayerCount)
	{
		vkPrint(2, "Vulkan Instance support more layer than backend has imagined, report this please!");
		return;
	}
	VkLayerProperties Supported_LayerList[maxVkLayerCount];
	vkEnumerateInstanceLayerProperties(&Supported_LayerNumber, Supported_LayerList);

	// INSTANCE CREATION INFO
	VkInstanceCreateInfo InstCreation_Info = {};
	InstCreation_Info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	InstCreation_Info.pApplicationInfo = &GVkAppInfo;
	// Extensions
	InstCreation_Info.enabledExtensionCount = activeInstanceExtCount;
	InstCreation_Info.ppEnabledExtensionNames = activeInstanceExts;

// Validation Layers
#ifdef VK_VALIDATION_LAYER
	const char* Validation_Layers[] = {"VK_LAYER_KHRONOS_validation"};
	InstCreation_Info.enabledLayerCount = 1;
	InstCreation_Info.ppEnabledLayerNames = Validation_Layers;
#endif

	if (vkCreateInstance(&InstCreation_Info, nullptr, &GVkInstance) != VK_SUCCESS)
	{
		vkPrint(16, "Failed to create a Vulkan Instance!");
	}

	volkLoadInstance(GVkInstance);
}
void SetupDebugging()
{
	VkDebugUtilsMessengerCreateInfoEXT dbgMssngrCi = {};
	dbgMssngrCi.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	dbgMssngrCi.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	dbgMssngrCi.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	dbgMssngrCi.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT Message_Severity,
									 VkDebugUtilsMessageTypeFlagsEXT messageType,
									 const VkDebugUtilsMessengerCallbackDataEXT* pCallback_Data,
									 void* pUserData) -> VkBool32 {
		// All of these titles should at same char count = 87
		constexpr const char* messageTypeTitles[] = {
			"VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: Unrelated to specification or performance\n",
			"VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: Specification violated\n",
			"VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: Non-optimal use of Vulkan\n",
			"Unsupported VK_DEBUG_UTILS_MESSAGE_TYPE: Please report this log\n"};
		constexpr uint32_t messageTypeTitleLen = 87, maxDebugMessageLen = 1ull << 12;

		char debugMessage[maxDebugMessageLen] = {};
		strcpy(debugMessage, messageTypeTitles[messageType]);
		strcat(debugMessage, pCallback_Data->pMessage);
		vkPrint(16, debugMessage);
		return false;
	};
	dbgMssngrCi.pNext = nullptr;
	dbgMssngrCi.pUserData = nullptr;

	if (vkCreateDebugUtilsMessengerEXT(GVkInstance, &dbgMssngrCi, nullptr, &GContext->DebugMessenger) != VK_SUCCESS)
		vkPrint(16, "Vulkan Debug Callback system initialization failed");
}
inline void CreateGpuDevices()
{
	// CHECK GPUs
	uint32_t gpuCount = 0;
	VkPhysicalDevice tempDevices[kMaxGpuCount];
	vkEnumeratePhysicalDevices(GVkInstance, &gpuCount, nullptr);
	if (gpuCount > kMaxGpuCount)
	{
		vkPrint(16, "System has more GPUs than supported, increase VKCONST_MAXGPUCOUNT!");
		return;
	}
	vkEnumeratePhysicalDevices(GVkInstance, &gpuCount, tempDevices);

	if (gpuCount == 0)
	{
		vkPrint(17);
		return;
	}

	// Query GPU info and create VkDevice
	for (TU4 i = 0; i < gpuCount; i++)
	{
		// GPU initializer handles everything else
		GPU* gpu = GContext->GPUs[i];
		gpu->vk_physical.Set(tempDevices[i]);

		// Analize GPU memory & extensions
		AnalizeGpuMemory(gpu);

		VkDeviceQueueCreateInfo queues[kMaxQueueFamilyCountPerGpu]{};
		uint32_t queueFamiliesCount = 0;
		{
			VkQueueFamilyProperties2 props[kMaxQueueFamilyCountPerGpu]{};
			for (uint32_t i = 0; i < kMaxQueueFamilyCountPerGpu; i++)
				props[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
			vkGetPhysicalDeviceQueueFamilyProperties2(gpu->vk_physical, &queueFamiliesCount, nullptr);
			if (queueFamiliesCount > kMaxQueueFamilyCountPerGpu)
			{
				vkPrint(16, "kMaxQueueFamilyCountPerGpu is exceeded, please report this!");
				continue;
			}
			vkGetPhysicalDeviceQueueFamilyProperties2(gpu->vk_physical, &queueFamiliesCount, props);

			for (uint32_t queueFamIdx = 0; queueFamIdx < queueFamiliesCount; queueFamIdx++)
			{
				float priorities[kMaxQueueCountPerQueueFamily]{};
				if (gpu->vk_propsQueue[queueFamIdx].queueFamilyProperties.queueCount > kMaxQueueCountPerQueueFamily)
				{
					vkPrint(16, "kMaxQueueCountPerQueueFamily is exceeded, please report this!");
					continue;
				}
				auto& queueFam = props[queueFamIdx];
				queues[queueFamIdx].queueCount = queueFam.queueFamilyProperties.queueCount;
				queues[queueFamIdx].queueFamilyIndex = queueFamIdx;
				queues[queueFamIdx].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				for (uint32_t queueIdx = 0; queueIdx < queueFam.queueFamilyProperties.queueCount; queueIdx++)
					priorities[queueIdx] =
						1.0f - float(float(queueIdx) / float(queueFam.queueFamilyProperties.queueCount));
				queues[queueFamIdx].pQueuePriorities = priorities;

				if (queueFam.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
					gpu->desc.OperationSupport_Compute = true;

				if (queueFam.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					gpu->desc.OperationSupport_Raster = true;

				if (queueFam.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
					gpu->desc.OperationSupport_Transfer = true;
			}
		}

		// Create logical device
		{
			VkDevice l;
			VkDeviceCreateInfo ci{};
			ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			ci.flags = 0;
			ci.pQueueCreateInfos = queues;
			ci.queueCreateInfoCount = queueFamiliesCount;
			ci.pNext = &gpu->vk_featuresDev;
			ci.enabledExtensionCount = sizeof(kRequiredDeviceExtensionNames) / sizeof(kRequiredDeviceExtensionNames[0]);
			ci.ppEnabledExtensionNames = kRequiredDeviceExtensionNames;
			ci.pEnabledFeatures = nullptr;
			ci.enabledLayerCount = 0;
			if (vkCreateDevice(gpu->vk_physical, &ci, nullptr, &l) != VK_SUCCESS)
			{
				vkPrint(7, "vkCreateDevice()");
				continue;
			}
			gpu->vk_logical.Set(l);
		}

		// Get VkQueue objects
		gpu->desc.QueueFamilyCount = queueFamiliesCount;
		for (uint32_t queueFamIdx = 0; queueFamIdx < queueFamiliesCount; queueFamIdx++)
		{
			for (uint32_t queueIdx = 0; queueIdx < kMaxQueueCountPerQueueFamily; queueIdx++)
			{
				VkQueue queue;
				vkGetDeviceQueue(gpu->vk_logical, queueFamIdx, queueIdx, &queue);

				// Create call synchronization semaphore (DX12 like sequential ordering)
				VkSemaphore sem;
				{
					VkSemaphoreCreateInfo sem_ci = {};
					sem_ci.flags = 0;
					sem_ci.pNext = nullptr;
					sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
					if (vkCreateSemaphore(gpu->vk_logical, &sem_ci, nullptr, &sem) != VK_SUCCESS)
						vkPrint(16, "Queue Call Synchronization Binary Semaphore creation failed!");
				}

				Queue x(gpu);
				x.queue.Set(queue);
				x.CallSynchronizer.Set(sem);
				x.QueueIdx = queueIdx;
				x.QueueFamIdx = queueFamIdx;
			}
		}
	}
}
void AnalizeGpuMemory(GPU* VKGPU)
{
	VkPhysicalDeviceMemoryBudgetPropertiesEXT budgetProps;
	budgetProps.pNext = nullptr;
	budgetProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
	VKGPU->vk_propsMemory.pNext = &budgetProps;
	VKGPU->vk_propsMemory.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
	vkGetPhysicalDeviceMemoryProperties2(VKGPU->vk_physical, &VKGPU->vk_propsMemory);

	for (uint32_t memTypeIdx = 0; memTypeIdx < VKGPU->vk_propsMemory.memoryProperties.memoryTypeCount; memTypeIdx++)
	{
		VkMemoryType& memType = VKGPU->vk_propsMemory.memoryProperties.memoryTypes[memTypeIdx];
		bool isDeviceLocal = false;
		bool isHostVisible = false;
		bool isHostCoherent = false;
		bool isHostCached = false;

		if ((memType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			isDeviceLocal = true;
		if ((memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
			isHostVisible = true;
		if ((memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			isHostCoherent = true;
		if ((memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
			isHostCached = true;

		if (!isDeviceLocal && !isHostVisible && !isHostCoherent && !isHostCached)
			continue;

		auto createMemDesc = [memTypeIdx, VKGPU, memType](TGfxMemoryAllocationType allocType) {
			TGfxMemoryInfo& memtype_desc = VKGPU->m_memoryDescTGFX[memTypeIdx];
			memtype_desc.AllocationType = allocType;
			memtype_desc.MemoryTypeId = memTypeIdx;
			memtype_desc.MaxAllocationSize = VKGPU->vk_propsMemory.memoryProperties.memoryHeaps[memType.heapIndex].size;
		};
		if (isDeviceLocal)
		{
			if (isHostVisible && isHostCoherent)
			{
				createMemDesc(TGFX_MEMORYALLOCATIONTYPE_FASTHOSTVISIBLE);
			}
			else
			{
				createMemDesc(TGFX_MEMORYALLOCATIONTYPE_DEVICELOCAL);
			}
		}
		else if (isHostVisible && isHostCoherent)
		{
			if (isHostCached)
				createMemDesc(TGFX_MEMORYALLOCATIONTYPE_READBACK);
			else
				createMemDesc(TGFX_MEMORYALLOCATIONTYPE_HOSTVISIBLE);
		}
	}

	VKGPU->desc.MemRegions = VKGPU->m_memoryDescTGFX;
	VKGPU->desc.MemRegionsCount = VKGPU->vk_propsMemory.memoryProperties.memoryTypeCount;
}

TCResult VkContext::Initialize()
{
	CreateInstance();
	if (GVkInstance == VK_NULL_HANDLE)
		return {TC_RESULTSTATE_FAILURE, 0};

#ifdef VK_VALIDATION_LAYER
	SetupDebugging();
	if (DebugMessenger == VK_NULL_HANDLE)
		return {TC_RESULTSTATE_FAILURE, 0};
#endif

	CreateGpuDevices();
	if (GPUs.size() == 0)
		return {TC_RESULTSTATE_FAILURE, 0};

	ContentManagerContext::Initialize();
	RendererContext::Initialize();
}

VkContext::VkContext()
{
	GContext = this;
}

VkContext::~VkContext()
{
	GContext = nullptr;
}

VkSurfaceKHR VkContext::FindOrCreateSurface(void* windowOsHnd)
{
	VkSurfaceKHR surface{};
	if (auto it = WindowToVkSurfaceMap.Find(windowOsHnd))
		surface = *it;
	else
	{
		VkWin32SurfaceCreateInfoKHR ci{};
		ci.hinstance = GetModuleHandleW(nullptr);
		ci.hwnd = (HWND)windowOsHnd;
		ci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		if (vkCreateWin32SurfaceKHR(GVkInstance, &ci, nullptr, &surface) != VK_SUCCESS)
			vkPrint(2, "Failed to create surface");
		else
			WindowToVkSurfaceMap[windowOsHnd] = surface;
	}
	return surface;
}

TCResult VkContext::CreateSwapchain(const TGfxSwapchainDescription* desc, TGfxSwapchain* swapchain)
{
	GPU* GPU = GetVkObject(desc->Gpu);

	if (desc->ImageCount > kMaxSwapchainTextureCountPerSwapchain)
		return vkPrint(2, "Requested swapchain image count is greater than kMaxSwapchainTextureCountPerWindow");

	VkSurfaceKHR surface = GContext->FindOrCreateSurface(desc->WindowHnd);

	// Get supported image usage from GPU's capabilities
	VkImageUsageFlags swapchainUsage{};
	{
		VkPhysicalDeviceSurfaceInfo2KHR info{};
		info.surface = surface;
		info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
		VkSurfaceCapabilities2KHR c{};
		c.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
		if (vkGetPhysicalDeviceSurfaceCapabilities2KHR(GPU->vk_physical, &info, &c) != VK_SUCCESS)
			return {TC_RESULTSTATE_FAILURE, 0};

		swapchainUsage = c.surfaceCapabilities.supportedUsageFlags;
	}

	// Create VkSwapchainKHR object
	VkSwapchainKHRHnd swpchn(GPU->ReferenceManager);
	{
		VkSwapchainCreateInfoKHR ci = {};
		ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		ci.flags = 0;
		ci.pNext = nullptr;
		ci.presentMode = GetVkEnum(desc->Presentation);
		ci.surface = surface;
		ci.minImageCount = desc->ImageCount;
		ci.imageFormat = GetVkEnum(desc->Channels);
		ci.imageColorSpace = GetVkEnum(desc->ColorSpace);
		ci.imageExtent = {desc->ImageExtent.x, desc->ImageExtent.y};
		ci.imageArrayLayers = 1;
		ci.imageUsage = swapchainUsage;
		ci.clipped = VK_TRUE;
		ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		ci.oldSwapchain = nullptr;
		ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkSwapchainKHR s;
		if (vkCreateSwapchainKHR(GPU->vk_logical, &ci, nullptr, &s) != VK_SUCCESS)
			return vkPrint(19, "at vkCreateSwapchainKHR()");

		swpchn.Set(s);
	}

	// Get Swapchain Images & Create Views
	VkImage imgs[kMaxSwapchainTextureCountPerSwapchain] = {};
	VkImageView imgViews[kMaxSwapchainTextureCountPerSwapchain] = {};
	{

		uint32_t created_imagecount = 0;
		vkGetSwapchainImagesKHR(GPU->vk_logical, swpchn, &created_imagecount, nullptr);
		if (created_imagecount != desc->ImageCount)
		{
			vkPrint(19, "Number of textures returned from backend isn't enough!");
			return {TC_RESULTSTATE_FAILURE, 0};
		}
		if (vkGetSwapchainImagesKHR(GPU->vk_logical, swpchn, &created_imagecount, imgs) != VK_SUCCESS)
		{
			vkPrint(19, "At vkGetSwapchainImagesKHR()");
			return {TC_RESULTSTATE_FAILURE, 0};
		}

		for (unsigned int i = 0; i < desc->ImageCount; i++)
		{
			VkImageViewCreateInfo ImageView_ci = {};
			ImageView_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ImageView_ci.image = imgs[i];
			ImageView_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ImageView_ci.format = GetVkEnum(desc->Channels);
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

			if (vkCreateImageView(GPU->vk_logical, &ImageView_ci, nullptr, &imgViews[i]) != VK_SUCCESS)
			{
				vkPrint(19, "at vkCreateImageView()");
				return {TC_RESULTSTATE_FAILURE, 0};
			}
		}
	}

	vkDeviceWaitIdle(GPU->vk_logical);

	// Create acquire semaphore
	VkSemaphoreHnd acquireSem(GPU->ReferenceManager);
	{
		VkSemaphore s;
		VkSemaphoreCreateInfo sem_ci = {};
		sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		if (vkCreateSemaphore(GPU->vk_logical, &sem_ci, nullptr, &s) != VK_SUCCESS)
			return vkPrint(16, "at acquire semaphore creation");

		acquireSem.Set(s);
	}

	TGfxGpuSwapchainSupportInfo supportInfo{};
	uint32_t supportedQueueCount = 0;
	for (uint32_t queueFamIdx = 0; queueFamIdx < GPU->desc.QueueFamilyCount; queueFamIdx++)
	{
		VkBool32 isSupported = false;
		if (vkGetPhysicalDeviceSurfaceSupportKHR(GPU->vk_physical, queueFamIdx, surface, &isSupported) != VK_SUCCESS)
			vkPrint(50, "at vkGetPhysicalDeviceSurfaceSupportKHR");
		const uint32_t queueCount = GPU->vk_propsQueue[queueFamIdx].queueFamilyProperties.queueCount;
		if (isSupported)
			for (uint32_t queueIdx = 0;
				 queueIdx < queueCount && supportedQueueCount < TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT;
				 queueIdx++)
				supportInfo.Queues[supportedQueueCount++] = GetOpaqueHandle(getQueue(queueFam, queueIdx));
	}
	if (supportedQueueCount < TGFX_WINDOWGPUSUPPORT_MAXQUEUECOUNT)
		supportInfo.Queues[supportedQueueCount] = nullptr;

	// Create Textures and return handles
	TGfxTexture textures[kMaxSwapchainTextureCountPerSwapchain]{};
	for (uint32_t vkImIdx = 0; vkImIdx < desc->ImageCount; vkImIdx++)
	{
		Texture* swpchnTexture = GContentManagerContext->Textures.CreateObject(GPU);
		swpchnTexture->m_channels = TGFX_TEXTURE_CHANNELS_BGRA8UNORM;
		swpchnTexture->Size = TGfxUVec2{.x = desc->ImageExtent.x, .y = desc->ImageExtent.y};
		swpchnTexture->MipCount = 1;
		swpchnTexture->vk_image.Set(imgs[vkImIdx]);
		swpchnTexture->vk_imageView.Set(imgViews[vkImIdx]);
		swpchnTexture->vk_imageUsage = swpchnTextureUsage;
		swpchnTexture->m_dim = TGFX_TEXTURE_DIMENSIONS_2D;
		// No memory allocation is possible for these textures
		swpchnTexture->m_memBlock = VMemoryBlock::GETINVALID();
		swpchnTexture->m_memReqs = VMemoryRequirements::GETINVALID();

		textures[vkImIdx] = GetOpaqueHandle(swpchnTexture);
	}

	auto finalObj = GContentManagerContext->Swapchains.CreateObject(GPU);
	finalObj->AcquireSemaphore = acquireSem;
	finalObj->Swpchn.Set(swpchn);
	finalObj->Surface.Set(surface);
	finalObj->Info = supportInfo;

	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult VkContext::GetCurrentSwapchainTextureIndex(TGfxSwapchain swapchain, TU4* index)
{
	Swapchain* swpchn = GetVkObject(swapchain);

	uint32_t swpchnIdx = UINT32_MAX;
	if (vkAcquireNextImageKHR(
			swpchn->GetGpu()->vk_logical, swpchn->Swpchn, UINT64_MAX, swpchn->AcquireSemaphore, nullptr, &swpchnIdx) !=
		VK_SUCCESS)
		return vkPrint(16, "vkAcquireNextImageKHR() has failed");

	if (UINT32_MAX == swpchnIdx)
	{
		*index = UINT32_MAX;
		return vkPrint(16, "Acquired texture's index is invalid!");
	}

	// Set current swapchain index
	swpchn->m_swapchainCurrentTextureIdx = swpchnIdx;
	if (index)
		*index = swpchnIdx;

	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult VkContext::ChangeSwapchainResolution(TGfxSwapchain swapchain, TGfxUVec2 newSize) {}

TCResult VkContext::QuerySwapchainSupportOfGpu(TGfxGpu gpu, void* windowOsHnd, TGfxGpuSwapchainSupportInfo* oInfo)
{
	GPU* GPU = GetVkObject(gpu);
	VkSurfaceKHR surface = GContext->FindOrCreateSurface(windowOsHnd);

	// Query
	VkSurfaceCapabilities2KHR capsMain{};
	{
		VkPhysicalDeviceSurfaceInfo2KHR info{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR, .pNext = nullptr, .surface = surface};
		if (vkGetPhysicalDeviceSurfaceCapabilities2KHR(GPU->vk_physical, &info, &capsMain) != VK_SUCCESS)
			return {TC_RESULTSTATE_FAILURE, 0};
	}
	VkSurfaceCapabilitiesKHR& caps = capsMain.surfaceCapabilities;

	uint32_t formatCount = 0;
	VkSurfaceFormatKHR formats[TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT];
	vkGetPhysicalDeviceSurfaceFormatsKHR(GPU->vk_physical, surface, &formatCount, VK_NULL_HANDLE);
	if (formatCount > TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT)
		return vkPrint(16, "Current window has more swapchain formats than supported, please report this!");

	if (vkGetPhysicalDeviceSurfaceFormatsKHR(GPU->vk_physical, surface, &formatCount, formats) != VK_SUCCESS)
		return vkPrint(16, "vkGetPhysicalDeviceSurfaceFormatsKHR() failed");

	for (uint32_t i = 0; i < formatCount; i++)
	{
		oInfo->ColorSpace[i] = GetTGfxEnum(formats[i].colorSpace);
		oInfo->Channels[i] = GetTGfxEnum(formats[i].format);
	}

	if (caps.maxImageCount <= caps.minImageCount)
	{
		return vkPrint(16, "Window Surface Capabilities have issues, maxImageCount <= minImageCount!");
	}
	oInfo->MaxImageCount = caps.maxImageCount;

	oInfo->MaxExtent.x = caps.maxImageExtent.width;
	oInfo->MaxExtent.y = caps.maxImageExtent.height;
	oInfo->MinExtent.x = caps.minImageExtent.width;
	oInfo->MinExtent.y = caps.minImageExtent.height;

	uint32_t presentModesCount = 0;
	VkPresentModeKHR Presentations[TGFX_WINDOWGPUSUPPORT_MAXPRESENTATIONMODE];
	vkGetPhysicalDeviceSurfacePresentModesKHR(GPU->vk_physical, surface, &presentModesCount, VK_NULL_HANDLE);

	if (vkGetPhysicalDeviceSurfacePresentModesKHR(GPU->vk_physical, surface, &presentModesCount, Presentations) !=
		VK_SUCCESS)
		return vkPrint(16, "vkGetPhysicalDeviceSurfacePresentModesKHR() failed");

	if (presentModesCount > TGFX_WINDOWGPUSUPPORT_MAXPRESENTATIONMODE)
		return vkPrint(16, "GPU supports more presentation modes than supported, please report this!");

	for (uint32_t i = 0; i < presentModesCount; i++)
		oInfo->PresentationModes[i] = GetTGfxEnum(Presentations[i]);

	return {TC_RESULTSTATE_SUCCESS, 0};
}

void VkContext::DestroySwapchain(TGfxSwapchain swapchain)
{
	auto s = GetVkObject(swapchain);
	vkDestroySwapchainKHR(s->GetGpu()->vk_logical, s->Swpchn, nullptr);
	for (TU1 i = 0; i < kMaxSwapchainTextureCountPerSwapchain; i++)
	{
		auto texture = GetVkObject(s->m_swapchainTextures[i]);
		texture->vk_image.SetAsDead();
		texture->vk_imageView.SetAsDead();
	}
}

TCResult VkContext::Hook(ITGfx* gfx)
{
	gfx->ChangeSwapchainResolution = ChangeSwapchainResolution;
	gfx->CreateSwapchain = CreateSwapchain;
	gfx->GetCurrentSwapchainTextureIndex = GetCurrentSwapchainTextureIndex;
	gfx->DestroySwapchain = DestroySwapchain;
	gfx->QuerySwapchainSupportOfGpu = QuerySwapchainSupportOfGpu;
	ContentManagerContext::HookContentManager(gfx->ResourceManager);
	RendererContext::HookRenderer(gfx->Renderer);
	return {TC_RESULTSTATE_SUCCESS, 0};
}

} // namespace Vulkan
} // namespace TGFX

TCResult TGfxVulkan_Initialize(void** outPluginAPI)
{
	new TGFX::Vulkan::VkContext;
	if (!TGFX::Vulkan::GContext)
		return {TC_RESULTSTATE_FAILURE, 0};
	if (auto res = TGFX::Vulkan::GContext->Initialize(); res != TC_RESULTSTATE_SUCCESS)
		return res;

	TGfxBackendFunctions* api = new TGfxBackendFunctions;
	api->Hook = TGFX::Vulkan::VkContext::Hook;
	*outPluginAPI = api;
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TGfxVulkan_Shutdown()
{
	if (TGFX::Vulkan::GContext)
		delete TGFX::Vulkan::GContext;
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TGfxVulkan_OnPreShutdown()
{
	return {TC_RESULTSTATE_SUCCESS, 0};
}

void TGfxVulkan_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded) {}