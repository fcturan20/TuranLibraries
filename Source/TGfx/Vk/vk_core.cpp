#include <vector>
#include <stdio.h>

#define TCORE_USE_CPP_WRAPPER
#include "TCore.h"
#include "TGfxCore.h"

#include "vk_contentmanager.h"
#include "vk_core.h"
#include "vk_extension.h"
#include "vk_helper.h"
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

static class VkContext* GContext = nullptr;
class VkContext
{
public:
	// These are VK_VECTORs, instead of VK_LINEAROBJARRAYs, because they won't change at run-time so
	// frequently
	VK_ARRAY<GPU_VKOBJ, TGfxGpu> GPUs;
	TCore::UnorderedMap<void*, VkSurfaceKHR> WindowToVkSurfaceMap;

	VkConstU4 VKGLOBAL_MAX_INSTANCE_EXT_COUNT = 256;
	VkExtensionProperties SupportedInstanceExtensions[VKGLOBAL_MAX_INSTANCE_EXT_COUNT] = {};

	inline bool CheckInstanceExtensionSupported(const char* extName)
	{
		bool Is_Found = false;
		for (uint32_t supportedExtIndx = 0; supportedExtIndx < VKGLOBAL_MAX_INSTANCE_EXT_COUNT; supportedExtIndx++)
		{
			VkExtensionProperties& ext = SupportedInstanceExtensions[supportedExtIndx];
			if (!ext.extensionName)
			{
				break;
			}
			if (strcmp(extName, ext.extensionName))
			{
				return true;
			}
		}
		wchar_t wExtName[1024] = {};
		mbstowcs(wExtName, extName, 1023);
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
		vkEnumerateInstanceExtensionProperties(nullptr, &extCount, SupportedInstanceExtensions);
		if (extCount > VKGLOBAL_MAX_INSTANCE_EXT_COUNT)
		{
			vkPrint(2, "Vulkan Instance support more extension than backend has imagined, report this please!");
			return;
		}
		const char* activeInstanceExts[VKGLOBAL_MAX_INSTANCE_EXT_COUNT] = {};
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

	void SetupDebugging();

	inline void DetectSystemSpecs()
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

		// GET GPU INFORMATIONs, QUEUE FAMILIES etc
		for (TU4 i = 0; i < gpuCount; i++)
		{
			// GPU initializer handles everything else
			GPU_VKOBJ* vkgpu = GPUs[i];
			vkgpu->vk_physical = tempDevices[i];
			vkgpu->setGPUINDX(i);

			// Analize GPU memory & extensions
			AnalizeGpuMemory(vkgpu);
			ExtensionManager::createExtManager(vkgpu);

			vkgpu->ext()->inspect();
		}

		vk_createManager();
	}

	void AnalizeGpuMemory(GPU_VKOBJ* VKGPU)
	{
		VkPhysicalDeviceMemoryBudgetPropertiesEXT budgetProps;
		budgetProps.pNext = nullptr;
		budgetProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
		VKGPU->vk_propsMemory.pNext = &budgetProps;
		VKGPU->vk_propsMemory.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
		vkGetPhysicalDeviceMemoryProperties2(VKGPU->vk_physical, &VKGPU->vk_propsMemory);

		for (uint32_t memTypeIndx = 0; memTypeIndx < VKGPU->vk_propsMemory.memoryProperties.memoryTypeCount;
			 memTypeIndx++)
		{
			VkMemoryType& memType = VKGPU->vk_propsMemory.memoryProperties.memoryTypes[memTypeIndx];
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

			auto createMemDesc = [memTypeIndx, VKGPU, memType](TGfxMemoryAllocationType allocType) {
				TGfxMemoryInfo& memtype_desc = VKGPU->m_memoryDescTGFX[memTypeIndx];
				memtype_desc.AllocationType = allocType;
				memtype_desc.MemoryTypeId = memTypeIndx;
				memtype_desc.MaxAllocationSize =
					VKGPU->vk_propsMemory.memoryProperties.memoryHeaps[memType.heapIndex].size;
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

		VKGPU->desc.memRegions = VKGPU->m_memoryDescTGFX;
		VKGPU->desc.memRegionsCount = VKGPU->vk_propsMemory.memoryProperties.memoryTypeCount;
	}

	TCResult Initialize()
	{
		CreateInstance();
		if (GVkInstance == VK_NULL_HANDLE)
			return {TC_RESULTSTATE_FAILURE, 0};

#ifdef VK_VALIDATION_LAYER
		SetupDebugging();
		if (DebugMessenger == VK_NULL_HANDLE)
			return {TC_RESULTSTATE_FAILURE, 0};
#endif

		DetectSystemSpecs();
		if (GPUs.size() == 0)
			return {TC_RESULTSTATE_FAILURE, 0};

		// Init systems
		vk_setQueueFncPtrs();
		vk_createContentManager();
		vk_initRenderer();
		vk_setHelperFuncPtrs();
	}

	VkContext() { GContext = this; }

	~VkContext() { GContext = nullptr; }

	VkSurfaceKHR FindOrCreateSurface(void* windowOsHnd)
	{
		VkSurfaceKHR surface{};
		if (auto it = WindowToVkSurfaceMap.find(windowOsHnd); it != WindowToVkSurfaceMap.end())
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

	static TCResult CreateSwapchain(const TGfxSwapchainDescription* desc, TGfxSwapchain* swapchain)
	{
		GPU_VKOBJ* GPU = getOBJ<GPU_VKOBJ>(desc->Gpu);

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
		VkSwapchainKHR swpchn{};
		{
			VkSwapchainCreateInfoKHR ci = {};
			ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			ci.flags = 0;
			ci.pNext = nullptr;
			ci.presentMode = vk_findPresentModeVk(desc->Presentation);
			ci.surface = surface;
			ci.minImageCount = desc->ImageCount;
			ci.imageFormat = vk_findFormatVk(desc->Channels);
			ci.imageColorSpace = vk_findColorSpaceVk(desc->ColorSpace);
			ci.imageExtent = {desc->ImageExtent.x, desc->ImageExtent.y};
			ci.imageArrayLayers = 1;
			ci.imageUsage = swapchainUsage;
			ci.clipped = VK_TRUE;
			ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			ci.oldSwapchain = nullptr;
			ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateSwapchainKHR(GPU->vk_logical, &ci, nullptr, &swpchn) != VK_SUCCESS)
				return vkPrint(19, "at vkCreateSwapchainKHR()");
		}

		// Get Swapchain Images & Create Views
		VkImage imgs[kMaxSwapchainTextureCountPerSwapchain] = {};
		VkImageView imgViews[kMaxSwapchainTextureCountPerSwapchain] = {};
		{
			uint32_t created_imagecount = 0;
			vkGetSwapchainImagesKHR(GPU->vk_logical, window->vk_swapchain, &created_imagecount, nullptr);
			if (created_imagecount != desc->imageCount)
			{
				vkPrint(19, "number of textures returned from backend isn't enough!");
				return {TC_RESULTSTATE_FAILURE, 0};
			}
			if (vkGetSwapchainImagesKHR(GPU->vk_logical, window->vk_swapchain, &created_imagecount, imgs) != VK_SUCCESS)
			{
				vkPrint(19, "at vkGetSwapchainImagesKHR()");
				return {TC_RESULTSTATE_FAILURE, 0};
			}

			for (unsigned int i = 0; i < desc->imageCount; i++)
			{
				VkImageViewCreateInfo ImageView_ci = {};
				ImageView_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				ImageView_ci.image = imgs[i];
				ImageView_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
				ImageView_ci.format = vk_findFormatVk(desc->channels);
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

		window->m_swapchainTextureCount = desc->imageCount;
		window->m_gpu = GPU;

		vkDeviceWaitIdle(GPU->vk_logical);

		// Create acquire semaphore
		{
			vkCreateSemaphore sem = GPU->CreateBinarySemaphore();
			VkSemaphoreCreateInfo sem_ci = {};
			sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			if (vkCreateSemaphore(GPU->vk_logical, &sem_ci, nullptr, &window->vk_acquireSemaphore) != VK_SUCCESS)
				return vkPrint(16, "at acquire semaphore creation");
		}

		// Validate swapchain image texture layout
		static constexpr bool validateTextureLayout = false;
		if constexpr (validateTextureLayout)
		{
			VkCommandPool transitionCmdPool, initializeCmdPool;
			// Create command pools
			{
				VkCommandPoolCreateInfo cp_ci = {};
				cp_ci.flags = 0;
				cp_ci.queueFamilyIndex = vkQueueFamIndx;
				cp_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				if (vkCreateCommandPool(GPU->vk_logical, &cp_ci, nullptr, &transitionCmdPool) != VK_SUCCESS)
				{
					return vkPrint(16, "at command pool creation for swapchain transition");
				}
				if (vkCreateCommandPool(GPU->vk_logical, &cp_ci, nullptr, &initializeCmdPool) != VK_SUCCESS)
				{
					return vkPrint(16, "at command pool creation for swapchain initialization");
				}
			}
			{
				VkCommandBufferAllocateInfo cb_ai = {};
				cb_ai.commandBufferCount = window->m_swapchainTextureCount;
				cb_ai.commandPool = transitionCmdPool;
				cb_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				cb_ai.pNext = nullptr;
				cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				// General -> Present
				if (vkAllocateCommandBuffers(GPU->vk_logical, &cb_ai, window->vk_generalToPresent[vkQueueFamIndx]) !=
					VK_SUCCESS)
				{
					return vkPrint(16, "at general->present command buffer creation for swapchain transition");
				}
				if (vkAllocateCommandBuffers(GPU->vk_logical, &cb_ai, window->vk_presentToGeneral[vkQueueFamIndx]) !=
					VK_SUCCESS)
				{
					return vkPrint(16, "at present->general command buffer creation for swapchain transition");
				}
			}
			for (uint32_t textureIndx = 0; textureIndx < window->m_swapchainTextureCount; textureIndx++)
			{
				VkImageMemoryBarrier imBar = {};
				// General -> Present CB Recording
				VkCommandBufferBeginInfo cb_bi = {};
				cb_bi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
				cb_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				{
					VkCommandBuffer cb = window->vk_generalToPresent[vkQueueFamIndx][textureIndx];
					if (vkBeginCommandBuffer(cb, &cb_bi) != VK_SUCCESS)
					{
						return vkPrint(16, "at general->present command buffer recording begin");
					}
					imBar.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
					imBar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					imBar.image = imgs[textureIndx];
					imBar.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
					imBar.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
					imBar.pNext = nullptr;
					imBar.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
					imBar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					imBar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imBar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imBar.subresourceRange.baseArrayLayer = 0;
					imBar.subresourceRange.baseMipLevel = 0;
					imBar.subresourceRange.layerCount = 1;
					imBar.subresourceRange.levelCount = 1;

					vkCmdPipelineBarrier(cb,
										 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
										 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
										 VK_DEPENDENCY_DEVICE_GROUP_BIT,
										 0,
										 nullptr,
										 0,
										 nullptr,
										 1,
										 &imBar);
					if (vkEndCommandBuffer(cb) != VK_SUCCESS)
					{
						return vkPrint(16, "at general->present command buffer recording end");
					}
				}
				// Present -> General CB Recording
				{
					VkCommandBuffer cb = window->vk_presentToGeneral[vkQueueFamIndx][textureIndx];
					if (vkBeginCommandBuffer(cb, &cb_bi) != VK_SUCCESS)
					{
						vkPrint(16, "at present->general command buffer recording begin");
					}

					imBar.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
					imBar.newLayout = VK_IMAGE_LAYOUT_GENERAL;
					imBar.dstAccessMask = imBar.srcAccessMask;
					imBar.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
					vkCmdPipelineBarrier(cb,
										 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
										 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
										 VK_DEPENDENCY_DEVICE_GROUP_BIT,
										 0,
										 nullptr,
										 0,
										 nullptr,
										 1,
										 &imBar);
					if (vkEndCommandBuffer(cb) != VK_SUCCESS)
					{
						vkPrint(16, "at present->general command buffer recording begin");
					}
				}

				// Present Texture only once
				if (queueFamListIterIndx == 0)
				{
					VkCommandBuffer initializeCmdBuffer = {};
					// Allocate CB
					{
						VkCommandBufferAllocateInfo cb_ai = {};
						cb_ai.commandBufferCount = 1;
						cb_ai.commandPool = transitionCmdPool;
						cb_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
						cb_ai.pNext = nullptr;
						cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
						if (vkAllocateCommandBuffers(GPU->vk_logical, &cb_ai, &initializeCmdBuffer) != VK_SUCCESS)
						{
							vkPrint(16, "at presentation command buffer allocation");
						}
					}
					// Record first CB
					{
						VkCommandBufferBeginInfo cb_bi = {};
						cb_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
						cb_bi.pNext = nullptr;
						cb_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
						vkBeginCommandBuffer(initializeCmdBuffer, &cb_bi);

						imBar.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						imBar.newLayout = VK_IMAGE_LAYOUT_GENERAL;
						vkCmdPipelineBarrier(initializeCmdBuffer,
											 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
											 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
											 VK_DEPENDENCY_DEVICE_GROUP_BIT,
											 0,
											 nullptr,
											 0,
											 nullptr,
											 1,
											 &imBar);

						vkEndCommandBuffer(initializeCmdBuffer);
					}

					// Submit to transition
					{
						VkSubmitInfo si = {};
						si.commandBufferCount = 1;
						si.pCommandBuffers = &initializeCmdBuffer;
						si.pNext = nullptr;
						si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
						si.waitSemaphoreCount = 0;
						si.signalSemaphoreCount = 0;
						if (vkQueueSubmit(
								getQueueVkObj(getQueue(getQueueFam(GPU, vkQueueFamIndx), 0)), 1, &si, VK_NULL_HANDLE) !=
							VK_SUCCESS)
						{
							return vkPrint(16, "at queue submission for layout transition");
						}
					}
				}
			}
			vkDestroyCommandPool(GPU->vk_logical, initializeCmdPool, nullptr);
		}

		// Create TEXTURE_VKOBJs and return handles
		window->vk_swapchainTextureUsage = swapchainUsage;
		for (uint32_t vkImIndx = 0; vkImIndx < desc->ImageCount; vkImIndx++)
		{
			TEXTURE_VKOBJ* SWAPCHAINTEXTURE = contentManager->GETTEXTURES_ARRAY().create_OBJ();
			SWAPCHAINTEXTURE->m_channels = texture_channels_tgfx_BGRA8UNORM;
			SWAPCHAINTEXTURE->m_width = window->m_newWidth;
			SWAPCHAINTEXTURE->m_height = window->m_newHeight;
			SWAPCHAINTEXTURE->m_mips = 1;
			SWAPCHAINTEXTURE->vk_image = imgs[vkImIndx];
			SWAPCHAINTEXTURE->vk_imageView = imgViews[vkImIndx];
			SWAPCHAINTEXTURE->vk_imageUsage = window->vk_swapchainTextureUsage;
			SWAPCHAINTEXTURE->m_dim = texture_dimensions_tgfx_2D;
			SWAPCHAINTEXTURE->m_GPU = GPU->gpuIndx();
			// No memory allocation is possible for these textures
			SWAPCHAINTEXTURE->m_memBlock = VMemoryBlock::GETINVALID();
			SWAPCHAINTEXTURE->m_memReqs = VMemoryRequirements::GETINVALID();

			textures[vkImIndx] = getHANDLE<TGfxTexture>(SWAPCHAINTEXTURE);
			window->m_swapchainTextures[vkImIndx] = textures[vkImIndx];
		}
		window->m_lastHeight = window->m_newHeight;
		window->m_lastWidth = window->m_newWidth;
		return {TC_RESULTSTATE_SUCCESS, 0};
	}

	static TCResult GetCurrentSwapchainTextureIndex(TGfxSwapchain swapchain, TU4* index)
	{
		VkSwapchainObj* swpchn = getOBJ<VkSwapchainObj>(swapchain);

		uint32_t swpchnIndx = UINT32_MAX;
		if (vkAcquireNextImageKHR(swpchn->m_gpu->vk_logical,
								  swpchn->Swapchain,
								  UINT64_MAX,
								  swpchn->AcquireSemaphore,
								  nullptr,
								  &swpchnIndx) != VK_SUCCESS)
			return vkPrint(16, "vkAcquireNextImageKHR() has failed");

		if (UINT32_MAX == swpchnIndx)
		{
			*index = UINT32_MAX;
			return vkPrint(16, "Acquired texture's index is invalid!");
		}

		// Set current swapchain index
		swpchn->m_swapchainCurrentTextureIndx = swpchnIndx;
		if (index)
			*index = swpchnIndx;

		return {TC_RESULTSTATE_SUCCESS, 0};
	}

	static TCResult ChangeSwapchainResolution(TGfxSwapchain swapchain, TGfxUVec2 newSize) {}

	static TCResult QuerySwapchainSupportOfGpu(TGfxGpu gpu, void* windowOsHnd, TGfxGpuSwapchainSupportInfo* oInfo)
	{
		GPU_VKOBJ* GPU = getOBJ<GPU_VKOBJ>(gpu);
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
			oInfo->ColorSpace[i] = vk_findColorSpaceTgfx(formats[i].colorSpace);
			oInfo->Channels[i] = vk_findTextureChannelsTgfx(formats[i].format);
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
			oInfo->PresentationModes[i] = vk_findPresentModeTgfx(Presentations[i]);

		return {TC_RESULTSTATE_SUCCESS, 0};
	}

	static void DestroySwapchain(TGfxSwapchain swapchain) {}

	static TCResult Hook(ITGfx* gfx)
	{
		TCORE_ACTIVE_PLUGIN_NAME;
		gfx->ChangeSwapchainResolution = ChangeSwapchainResolution;
		gfx->CreateSwapchain = CreateSwapchain;
		gfx->GetCurrentSwapchainTextureIndex = GetCurrentSwapchainTextureIndex;
		gfx->DestroySwapchain = DestroySwapchain;
		gfx->QuerySwapchainSupportOfGpu = QuerySwapchainSupportOfGpu;
		return {TC_RESULTSTATE_SUCCESS, 0};
	}

	VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;
};

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

TCResult vk_initGPU(TGfxGpu gpu)
{
	// Create Logical Device
	GPU_VKOBJ* GPU = getOBJ<GPU_VKOBJ>(gpu);
	auto& queueFams = manager->get_queue_cis(GPU);

	VkDeviceCreateInfo logicdevic_ci{};
	logicdevic_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logicdevic_ci.flags = 0;
	logicdevic_ci.pQueueCreateInfos = queueFams.list;
	logicdevic_ci.queueCreateInfoCount = GPU->desc.queueFamilyCount;
	logicdevic_ci.pNext = &GPU->vk_featuresDev;
	logicdevic_ci.ppEnabledExtensionNames = GPU->ext()->getEnabledExtensionNames(&logicdevic_ci.enabledExtensionCount);
	logicdevic_ci.pEnabledFeatures = nullptr;
	logicdevic_ci.enabledLayerCount = 0;
	if (vkCreateDevice(GPU->vk_physical, &logicdevic_ci, nullptr, &GPU->vk_logical) != VK_SUCCESS)
	{
		return vkPrint(7, "vkCreateDevice()");
	}
	manager->get_queue_objects(GPU);
	return {TC_RESULTSTATE_SUCCESS, 0};
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//				Vulkan Debugging Layer
// It's placed at the end because it won't be changed frequently
////////////////////////////////////////////////
////////////////////////////////////////////////

void TGFX::Vulkan::VkContext::SetupDebugging()
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
		static constexpr const char* messageTypeTitles[] = {
			"VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: Unrelated to specification or performance\n",
			"VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: Specification violated\n",
			"VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: Non-optimal use of Vulkan\n",
			"Unsupported VK_DEBUG_UTILS_MESSAGE_TYPE: Please report this log\n"};
		static constexpr uint32_t messageTypeTitleLen = 87, maxDebugMessageLen = 1ull << 12;

		char debugMessage[maxDebugMessageLen] = {};
		strcpy(debugMessage, messageTypeTitles[messageType]);
		strcat(debugMessage, pCallback_Data->pMessage);
		vkPrint(16, debugMessage);
		return false;
	};
	dbgMssngrCi.pNext = nullptr;
	dbgMssngrCi.pUserData = nullptr;

	if (vkCreateDebugUtilsMessengerEXT(GVkInstance, &dbgMssngrCi, nullptr, &DebugMessenger) != VK_SUCCESS)
		vkPrint(16, "Vulkan Debug Callback system initialization failed");
}