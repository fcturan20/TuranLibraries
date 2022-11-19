#include "vk_core.h"

#include <ecs_tapi.h>
#include <stdio.h>
#include <tgfx_core.h>
#include <tgfx_gpucontentmanager.h>
#include <tgfx_helper.h>
#include <tgfx_imgui.h>
#include <tgfx_renderer.h>
#include <threadingsys_tapi.h>
#include <virtualmemorysys_tapi.h>

#include <string>
#include <vector>

#include "vk_extension.h"
#include "vk_helper.h"
#include "vk_predefinitions.h"
#include "vk_queue.h"
#include "vk_renderer.h"
#include "vk_contentmanager.h"

struct device_features_chainedstructs;

struct core_private {
 public:
  // These are VK_VECTORs, instead of VK_LINEAROBJARRAYs, because they won't change at run-time so
  // frequently
  VK_STATICVECTOR<GPU_VKOBJ, gpu_tgfxhnd, VKCONST_MAXGPUCOUNT> DEVICE_GPUs;
  // Window Operations
  VK_STATICVECTOR<MONITOR_VKOBJ, monitor_tgfxhnd, 16> MONITORs;
  VK_STATICVECTOR<WINDOW_VKOBJ, window_tgfxhnd, VKCONST_MAXWINDOWCOUNT> WINDOWs;

  bool isAnyWindowResized = false, // Instead of checking each window each frame, just check this
    isActive_SurfaceKHR = false, isSupported_PhysicalDeviceProperties2 = true;
};
static core_private* hidden = nullptr;

VK_STATICVECTOR<WINDOW_VKOBJ, window_tgfxhnd, VKCONST_MAXWINDOWCOUNT>& core_public::GETWINDOWs() {
  return hidden->WINDOWs;
}
VK_STATICVECTOR<GPU_VKOBJ, gpu_tgfxhnd, VKCONST_MAXGPUCOUNT>& core_public::getGPUs() {
  return hidden->DEVICE_GPUs;
}

// While enabling features, some struct should be chained. This struct is to keep data object
// lifetimes optimal
struct device_features_chainedstructs {
  VkPhysicalDeviceDescriptorIndexingFeatures          DescIndxingFtrs     = {};
  VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures DepthStncl_Seperate = {};
  VkPhysicalDeviceBufferDeviceAddressFeatures         BufferDeviceAddress = {};
  VkPhysicalDeviceTimelineSemaphoreFeatures           timelineSemaphore   = {};
};

result_tgfx vk_initGPU(gpu_tgfxhnd gpu) {
  // Create Logical Device
  GPU_VKOBJ* GPU       = hidden->DEVICE_GPUs.getOBJfromHANDLE(gpu);
  auto&      queueFams = GPU->manager()->get_queue_cis();

  VkDeviceCreateInfo logicdevic_ci{};
  logicdevic_ci.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  logicdevic_ci.flags                = 0;
  logicdevic_ci.pQueueCreateInfos    = queueFams.list;
  logicdevic_ci.queueCreateInfoCount = GPU->manager()->m_queueFams.size();
  logicdevic_ci.pNext                = &GPU->vk_featuresDev;

  logicdevic_ci.enabledExtensionCount   = GPU->ext()->m_activeDevExtNames.size();
  logicdevic_ci.ppEnabledExtensionNames = GPU->ext()->m_activeDevExtNames.data();
  logicdevic_ci.pEnabledFeatures        = nullptr;
  logicdevic_ci.enabledLayerCount       = 0;
  if (ThrowIfFailed(vkCreateDevice(GPU->vk_physical, &logicdevic_ci, nullptr, &GPU->vk_logical),
                    "Vulkan failed to create a Logical Device!")) {
    printer(result_tgfx_FAIL, "Vulkan failed to create a Logical Device!");
    return result_tgfx_FAIL;
  }
  GPU->manager()->get_queue_objects();
  return result_tgfx_SUCCESS;
}

vk_uint32c VKGLOBAL_MAX_INSTANCE_EXT_COUNT = 256;
VK_STATICVECTOR<const char*, const char*, VKGLOBAL_MAX_INSTANCE_EXT_COUNT> activeInstanceExts;
VkExtensionProperties supportedInstanceExts[VKGLOBAL_MAX_INSTANCE_EXT_COUNT] = {};

inline bool vk_checkInstExtSupported(const char* extName) {
  bool Is_Found = false;
  for (uint32_t supported_extIndx = 0; supported_extIndx < VKGLOBAL_MAX_INSTANCE_EXT_COUNT;
       supported_extIndx++) {
    VkExtensionProperties& ext = supportedInstanceExts[supported_extIndx];
    if (!ext.extensionName) {
      break;
    }
    if (strcmp(extName, ext.extensionName)) {
      return true;
    }
  }
  printer(result_tgfx_WARNING,
          ("Extension: " + std::string(extName) + " is not supported by the GPU!").c_str());
  return false;
}
bool vk_checkInstanceExts() {
  uint32_t     glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  for (int i = 0; i < glfwExtensionCount; i++) {
    if (!vk_checkInstExtSupported(glfwExtensions[i])) {
      printer(result_tgfx_INVALIDARGUMENT,
              "Your vulkan instance doesn't support extensions that're required by GLFW. This "
              "situation is not tested, so report your device to the author!");
      return false;
    }
    activeInstanceExts.push_back(glfwExtensions[i]);
  }

  if (vk_checkInstExtSupported(VK_KHR_SURFACE_EXTENSION_NAME)) {
    hidden->isActive_SurfaceKHR = true;
    activeInstanceExts.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  } else {
    printer(result_tgfx_WARNING,
            "Your Vulkan instance doesn't support to display a window, so you shouldn't use any "
            "window related functionality such as: GFXRENDERER->Create_WindowPass, "
            "GFX->Create_Window, GFXRENDERER->Swap_Buffers ...");
  }

  // Check PhysicalDeviceProperties2KHR
  if (VKGLOBAL_APPINFO.apiVersion == VK_API_VERSION_1_0) {
    if (!vk_checkInstExtSupported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
      hidden->isSupported_PhysicalDeviceProperties2 = false;
      printer(result_tgfx_FAIL,
              "Your OS doesn't support Physical Device Properties 2 extension which is required, "
              "so Vulkan device creation has failed!");
      return false;
    } else {
      activeInstanceExts.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }
  }

#ifdef VULKAN_DEBUGGING
  if (vk_checkInstExtSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
    activeInstanceExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
#endif
  return true;
}
void vk_createInstance() {
  // APPLICATION INFO
  VKGLOBAL_APPINFO.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  VKGLOBAL_APPINFO.pApplicationName   = "TGFX Vulkan backend";
  VKGLOBAL_APPINFO.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  VKGLOBAL_APPINFO.pEngineName        = "GFX API";
  VKGLOBAL_APPINFO.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
  VKGLOBAL_APPINFO.apiVersion         = VK_API_VERSION_1_3;

  // CHECK SUPPORTED EXTENSIONs
  uint32_t extCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extCount, supportedInstanceExts);
  if (!vk_checkInstanceExts()) {
    return;
  }

  // CHECK SUPPORTED LAYERS
  vk_uint32c   maxVkLayerCount       = 256;
  unsigned int Supported_LayerNumber = 0;
  vkEnumerateInstanceLayerProperties(&Supported_LayerNumber, nullptr);
  if (Supported_LayerNumber > maxVkLayerCount) {
    printer(result_tgfx_FAIL,
            "Vulkan Instance support more layer than backend has imagined, report this please!");
    return;
  }
  VkLayerProperties Supported_LayerList[maxVkLayerCount];
  vkEnumerateInstanceLayerProperties(&Supported_LayerNumber, Supported_LayerList);

  // INSTANCE CREATION INFO
  VkInstanceCreateInfo InstCreation_Info = {};
  InstCreation_Info.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  InstCreation_Info.pApplicationInfo     = &VKGLOBAL_APPINFO;
  // Extensions
  InstCreation_Info.enabledExtensionCount   = ( uint32_t )activeInstanceExts.size();
  InstCreation_Info.ppEnabledExtensionNames = activeInstanceExts.data();

  // Validation Layers
#ifdef VULKAN_DEBUGGING
  const char* Validation_Layers[1]      = {"VK_LAYER_KHRONOS_validation"};
  InstCreation_Info.enabledLayerCount   = 1;
  InstCreation_Info.ppEnabledLayerNames = Validation_Layers;
#else
  InstCreation_Info.enabledLayerCount   = 0;
  InstCreation_Info.ppEnabledLayerNames = nullptr;
#endif

  ThrowIfFailed(vkCreateInstance(&InstCreation_Info, nullptr, &VKGLOBAL_INSTANCE),
                "Failed to create a Vulkan Instance!");
}

void vk_analizeGPUmemory(GPU_VKOBJ* VKGPU) {
  VkPhysicalDeviceMemoryBudgetPropertiesEXT budgetProps;
  budgetProps.pNext = nullptr;
  budgetProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
  VkPhysicalDeviceMemoryProperties2 props;
  props.pNext = &budgetProps;
  props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
  vkGetPhysicalDeviceMemoryProperties2(VKGPU->vk_physical, &props);

  for (uint32_t memTypeIndx = 0;
       memTypeIndx < VKGPU->vk_propsMemory.memoryProperties.memoryTypeCount; memTypeIndx++) {
    VkMemoryType& memType     = VKGPU->vk_propsMemory.memoryProperties.memoryTypes[memTypeIndx];
    bool          isDeviceLocal  = false;
    bool          isHostVisible  = false;
    bool          isHostCoherent = false;
    bool          isHostCached   = false;

    if ((memType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ==
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
      isDeviceLocal = true;
    }
    if ((memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
      isHostVisible = true;
    }
    if ((memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ==
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
      isHostCoherent = true;
    }
    if ((memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) ==
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
      isHostCached = true;
    }

    if (!isDeviceLocal && !isHostVisible && !isHostCoherent && !isHostCached) {
      continue;
    }

    auto createMemDesc = [memTypeIndx, VKGPU, memType](memoryallocationtype_tgfx allocType) {
      tgfx_memory_description& memtype_desc = VKGPU->m_memoryDescTGFX[memTypeIndx];
      memtype_desc.allocationtype           = allocType;
      memtype_desc.memorytype_id            = memTypeIndx;
      memtype_desc.max_allocationsize =
        VKGPU->vk_propsMemory.memoryProperties.memoryHeaps[memType.heapIndex].size;
    };
    if (isDeviceLocal) {
      if (isHostVisible && isHostCoherent) {
        createMemDesc(memoryallocationtype_FASTHOSTVISIBLE);
      } else {
        createMemDesc(memoryallocationtype_DEVICELOCAL);
      }
    } else if (isHostVisible && isHostCoherent) {
      if (isHostCached) {
        createMemDesc(memoryallocationtype_READBACK);
      } else {
        createMemDesc(memoryallocationtype_HOSTVISIBLE);
      }
    }
  }

  VKGPU->desc.memRegions      = VKGPU->m_memoryDescTGFX;
  VKGPU->desc.memRegionsCount = props.memoryProperties.memoryTypeCount;
}

inline void vk_checkComputerSpecs() {
  // CHECK GPUs
  uint32_t GPU_NUMBER = 0;
  vkEnumeratePhysicalDevices(VKGLOBAL_INSTANCE, &GPU_NUMBER, nullptr);
  if (GPU_NUMBER > VKCONST_MAXGPUCOUNT) {
    printer(result_tgfx_FAIL, "Your device has more GPUs than supported!");
    return;
  }
  VkPhysicalDevice PhysicalGPUs[VKCONST_MAXGPUCOUNT] = {};
  vkEnumeratePhysicalDevices(VKGLOBAL_INSTANCE, &GPU_NUMBER, PhysicalGPUs);

  if (GPU_NUMBER == 0) {
    printer(result_tgfx_FAIL,
            "There is no GPU that has Vulkan support! Updating your drivers or Upgrading the OS "
            "may help");
  }

  // GET GPU INFORMATIONs, QUEUE FAMILIES etc
  for (unsigned int i = 0; i < GPU_NUMBER; i++) {
    // GPU initializer handles everything else
    GPU_VKOBJ* vkgpu   = hidden->DEVICE_GPUs.add();
    vkgpu->vk_physical = PhysicalGPUs[i];
    vkgpu->setGPUINDX(i);

    // Analize GPU memory & extensions
    vk_analizeGPUmemory(vkgpu);
    if (!manager_vk::createManager(vkgpu)) {
      hidden->DEVICE_GPUs.erase(hidden->DEVICE_GPUs.size() - 1);
      continue;
    }
    extManager_vkDevice::createExtManager(vkgpu);

    // SAVE BASIC INFOs TO THE GPU DESC
    vkgpu->desc.name           = vkgpu->vk_propsDev.properties.deviceName;
    vkgpu->desc.driverVersion = vkgpu->vk_propsDev.properties.driverVersion;
    vkgpu->desc.gfxApiVersion    = vkgpu->vk_propsDev.properties.apiVersion;
    vkgpu->desc.driverVersion = vkgpu->vk_propsDev.properties.driverVersion;
    vkgpu->desc.type       = vk_findGPUTypeTgfx(vkgpu->vk_propsDev.properties.deviceType);

    vkgpu->ext()->inspect();
  }
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
//		  Window Operations (GLFW, Swapchain & OS)
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

void GLFWwindowresizecallback(GLFWwindow* glfwwindow, int width, int height) {
  WINDOW_VKOBJ* vkwindow = ( WINDOW_VKOBJ* )glfwGetWindowUserPointer(glfwwindow);
  vkwindow->m_resizeFnc(( window_tgfxhnd )vkwindow, vkwindow->m_userData, width, height,
                        ( texture_tgfxhnd* )vkwindow->m_swapchainTextures);
}
void vk_createWindow(const windowDescription_tgfx* desc, void* user_ptr, window_tgfxhnd* window) {
  if (desc->ResizeCB) {
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  } else {
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  }
  GLFWmonitor* monitor = nullptr;
  if (desc->monitor && desc->Mode == windowmode_tgfx_FULLSCREEN) {
    monitor = hidden->MONITORs.getOBJfromHANDLE(desc->monitor)->monitorobj;
  }
  GLFWwindow* glfw_window =
    glfwCreateWindow(desc->size.x, desc->size.y, desc->NAME, monitor, nullptr);

  // Check and Report if GLFW fails
  if (glfw_window == NULL) {
    printer(result_tgfx_FAIL, "VulkanCore: We failed to create the window because of GLFW!");
    return;
  }

  // Window VulkanSurface Creation
  VkSurfaceKHR Window_Surface = {};
  if (ThrowIfFailed(
        glfwCreateWindowSurface(VKGLOBAL_INSTANCE, glfw_window, nullptr, &Window_Surface),
        "GLFW failed to create a window surface")) {
    return;
  }

  WINDOW_VKOBJ* vkWindow             = hidden->WINDOWs.add();
  vkWindow->m_lastWidth              = desc->size.x;
  vkWindow->m_newWidth               = desc->size.x;
  vkWindow->m_lastHeight             = desc->size.y;
  vkWindow->m_newHeight              = desc->size.y;
  vkWindow->m_displayMode            = desc->Mode;
  vkWindow->m_monitor                = nullptr;
  vkWindow->m_name                   = desc->NAME;
  vkWindow->vk_glfwWindow            = glfw_window;
  vkWindow->vk_swapchainTextureUsage = 0; // This will be set while creating swapchain
  vkWindow->m_resizeFnc              = desc->ResizeCB;
  vkWindow->m_userData               = user_ptr;
  vkWindow->vk_surface               = Window_Surface;

  if (desc->ResizeCB) {
    glfwSetWindowUserPointer(vkWindow->vk_glfwWindow, vkWindow->m_userData);
    glfwSetWindowSizeCallback(vkWindow->vk_glfwWindow, GLFWwindowresizecallback);
  }

  *window = hidden->WINDOWs.returnHANDLEfromOBJ(vkWindow);
}
result_tgfx vk_createSwapchain(gpu_tgfxhnd gpu, const tgfx_swapchain_description* desc,
                               texture_tgfxhnd* textures) {
  WINDOW_VKOBJ* window = hidden->WINDOWs.getOBJfromHANDLE(desc->window);
  GPU_VKOBJ*    GPU    = hidden->DEVICE_GPUs.getOBJfromHANDLE(gpu);

  // Create VkSwapchainKHR object
  VkSwapchainCreateInfoKHR swpchn_ci = {};
  {
    swpchn_ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swpchn_ci.flags            = 0;
    swpchn_ci.pNext            = nullptr;
    swpchn_ci.presentMode      = vk_findPresentModeVk(desc->presentationMode);
    swpchn_ci.surface          = window->vk_surface;
    swpchn_ci.minImageCount    = desc->imageCount;
    swpchn_ci.imageFormat      = vk_findFormatVk(desc->channels);
    swpchn_ci.imageColorSpace  = vk_findColorSpaceVk(desc->colorSpace);
    swpchn_ci.imageExtent      = {window->m_newWidth, window->m_newHeight};
    swpchn_ci.imageArrayLayers = 1;
    // swpchn_ci.imageUsage       = Get_VkTextureUsageFlag_byTGFX(desc->SwapchainUsage);
    //  Swapchain texture can be used as framebuffer, but we should set its bit!
    if (swpchn_ci.imageUsage &
        (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
      swpchn_ci.imageUsage &= ~(1UL << 5);
    }
    swpchn_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    swpchn_ci.clipped        = VK_TRUE;
    swpchn_ci.preTransform   = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swpchn_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (window->vk_swapchain) {
      swpchn_ci.oldSwapchain = window->vk_swapchain;
    } else {
      swpchn_ci.oldSwapchain = nullptr;
    }

    uint32_t queueFamIndxLst[VKCONST_MAXQUEUEFAMCOUNT_PERGPU] = {UINT32_MAX};
    VK_getQueueAndSharingInfos(desc->permittedQueues, nullptr, queueFamIndxLst,
                               &swpchn_ci.queueFamilyIndexCount, &swpchn_ci.imageSharingMode);
    swpchn_ci.pQueueFamilyIndices = queueFamIndxLst;

    THROW_RETURN_IF_FAIL(
      vkCreateSwapchainKHR(GPU->vk_logical, &swpchn_ci, nullptr, &window->vk_swapchain),
      "Failed to create a Swapchain in vkCreateSwapchainKHR", result_tgfx_FAIL);
  }

  // Get Swapchain Images & Create Views
  VkImage     SWPCHN_IMGs[VKCONST_MAXSWPCHNTXTURECOUNT_PERWINDOW]     = {};
  VkImageView SWPCHN_IMGVIEWs[VKCONST_MAXSWPCHNTXTURECOUNT_PERWINDOW] = {};
  {
    {
      uint32_t created_imagecount = 0;
      vkGetSwapchainImagesKHR(GPU->vk_logical, window->vk_swapchain, &created_imagecount, nullptr);
      if (created_imagecount != desc->imageCount) {
        printer(result_tgfx_FAIL,
                "VK backend asked for swapchain textures but Vulkan driver gave less number of "
                "textures than intended!");
        return result_tgfx_FAIL;
      }
      vkGetSwapchainImagesKHR(GPU->vk_logical, window->vk_swapchain, &created_imagecount,
                              SWPCHN_IMGs);

      for (unsigned int i = 0; i < desc->imageCount; i++) {
        VkImageViewCreateInfo ImageView_ci       = {};
        ImageView_ci.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ImageView_ci.image                       = SWPCHN_IMGs[i];
        ImageView_ci.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
        ImageView_ci.format                      = vk_findFormatVk(desc->channels);
        ImageView_ci.flags                       = 0;
        ImageView_ci.pNext                       = nullptr;
        ImageView_ci.components.r                = VK_COMPONENT_SWIZZLE_IDENTITY;
        ImageView_ci.components.g                = VK_COMPONENT_SWIZZLE_IDENTITY;
        ImageView_ci.components.b                = VK_COMPONENT_SWIZZLE_IDENTITY;
        ImageView_ci.components.a                = VK_COMPONENT_SWIZZLE_IDENTITY;
        ImageView_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageView_ci.subresourceRange.baseArrayLayer = 0;
        ImageView_ci.subresourceRange.baseMipLevel   = 0;
        ImageView_ci.subresourceRange.layerCount     = 1;
        ImageView_ci.subresourceRange.levelCount     = 1;

        if (vkCreateImageView(GPU->vk_logical, &ImageView_ci, nullptr, &SWPCHN_IMGVIEWs[i]) !=
            VK_SUCCESS) {
          printer(result_tgfx_FAIL, "VulkanCore: Image View creation has failed!");
          return result_tgfx_FAIL;
        }
      }
    }
  }

  window->m_swapchainTextureCount       = desc->imageCount;
  window->m_swapchainCurrentTextureIndx = 0;
  window->m_gpu                         = GPU;

  // Create acquire semaphore
  {
    VkSemaphoreCreateInfo sem_ci = {};
    sem_ci.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    THROW_RETURN_IF_FAIL(
      vkCreateSemaphore(GPU->vk_logical, &sem_ci, nullptr, &window->vk_acquireSemaphore),
      "Acquire semaphore creation has failed!", result_tgfx_FAIL);
  }

  uint32_t queueFamListIterIndx = 0;
  while (swpchn_ci.pQueueFamilyIndices[queueFamListIterIndx] != UINT32_MAX) {
    QUEUEFAM_VK* queueFam =
      GPU->manager()->m_queueFams[swpchn_ci.pQueueFamilyIndices[queueFamListIterIndx]];
    VkCommandPool transitionCmdPool, initializeCmdPool;
    // Create command pools
    {
      VkCommandPoolCreateInfo cp_ci = {};
      cp_ci.flags                   = 0;
      cp_ci.queueFamilyIndex        = queueFam->vk_queueFamIndex;
      cp_ci.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      THROW_RETURN_IF_FAIL(
        vkCreateCommandPool(GPU->vk_logical, &cp_ci, nullptr, &transitionCmdPool),
        "Command Pool creation for swapchain transition has failed!", result_tgfx_FAIL);
      THROW_RETURN_IF_FAIL(
        vkCreateCommandPool(GPU->vk_logical, &cp_ci, nullptr, &initializeCmdPool),
        "Command Pool creation for swapchain transition has failed!", result_tgfx_FAIL);
    }
    {
      VkCommandBufferAllocateInfo cb_ai = {};
      cb_ai.commandBufferCount          = window->m_swapchainTextureCount;
      cb_ai.commandPool                 = transitionCmdPool;
      cb_ai.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      cb_ai.pNext                       = nullptr;
      cb_ai.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      // General -> Present
      THROW_RETURN_IF_FAIL(
        vkAllocateCommandBuffers(GPU->vk_logical, &cb_ai,
                                 window->vk_generalToPresent[queueFam->vk_queueFamIndex]),
        "General->Present Command Buffer creation for swapchain transition has failed!",
        result_tgfx_FAIL);
      // Present -> General
      THROW_RETURN_IF_FAIL(
        vkAllocateCommandBuffers(GPU->vk_logical, &cb_ai,
                                 window->vk_presentToGeneral[queueFam->vk_queueFamIndex]),
        "Present->General Command Buffer creation for swapchain transition has failed!",
        result_tgfx_FAIL);
    }
    for (uint32_t textureIndx = 0; textureIndx < window->m_swapchainTextureCount; textureIndx++) {
      VkImageMemoryBarrier imBar = {};
      // General -> Present CB Recording
      VkCommandBufferBeginInfo cb_bi = {};
      cb_bi.flags                    = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
      cb_bi.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      {
        VkCommandBuffer cb = window->vk_generalToPresent[queueFam->vk_queueFamIndex][textureIndx];
        THROW_RETURN_IF_FAIL(vkBeginCommandBuffer(cb, &cb_bi),
                             "General -> Present CB recording begin failed!", result_tgfx_FAIL);
        imBar.dstAccessMask               = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        imBar.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        imBar.image                       = SWPCHN_IMGs[textureIndx];
        imBar.newLayout                   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imBar.oldLayout                   = VK_IMAGE_LAYOUT_GENERAL;
        imBar.pNext                       = nullptr;
        imBar.srcAccessMask               = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        imBar.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        imBar.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imBar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imBar.subresourceRange.baseArrayLayer = 0;
        imBar.subresourceRange.baseMipLevel   = 0;
        imBar.subresourceRange.layerCount     = 1;
        imBar.subresourceRange.levelCount     = 1;

        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_DEVICE_GROUP_BIT, 0,
                             nullptr, 0, nullptr, 1, &imBar);
        THROW_RETURN_IF_FAIL(vkEndCommandBuffer(cb),
                             "General -> Present CB recording begin failed!", result_tgfx_FAIL);
      }
      // Present -> General CB Recording
      {
        VkCommandBuffer cb = window->vk_presentToGeneral[queueFam->vk_queueFamIndex][textureIndx];
        THROW_RETURN_IF_FAIL(vkBeginCommandBuffer(cb, &cb_bi),
                             "Present -> General CB recording begin failed!", result_tgfx_FAIL);

        imBar.oldLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imBar.newLayout     = VK_IMAGE_LAYOUT_GENERAL;
        imBar.dstAccessMask = imBar.srcAccessMask;
        imBar.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_DEVICE_GROUP_BIT, 0,
                             nullptr, 0, nullptr, 1, &imBar);
        THROW_RETURN_IF_FAIL(vkEndCommandBuffer(cb),
                             "Present -> General CB recording begin failed!", result_tgfx_FAIL);
      }

      // Present Texture only once
      if (queueFamListIterIndx == 0) {
        VkCommandBuffer initializeCmdBuffer = {};
        // Allocate CB
        {
          VkCommandBufferAllocateInfo cb_ai = {};
          cb_ai.commandBufferCount          = 1;
          cb_ai.commandPool                 = transitionCmdPool;
          cb_ai.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
          cb_ai.pNext                       = nullptr;
          cb_ai.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
          THROW_RETURN_IF_FAIL(
            vkAllocateCommandBuffers(GPU->vk_logical, &cb_ai, &initializeCmdBuffer),
            "Presentation CB allocation failed!", result_tgfx_FAIL);
        }
        // Record first CB
        {
          VkCommandBufferBeginInfo cb_bi = {};
          cb_bi.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
          cb_bi.pNext                    = nullptr;
          cb_bi.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
          vkBeginCommandBuffer(initializeCmdBuffer, &cb_bi);

          imBar.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
          imBar.newLayout = VK_IMAGE_LAYOUT_GENERAL;
          vkCmdPipelineBarrier(initializeCmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                               VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_DEVICE_GROUP_BIT,
                               0, nullptr, 0, nullptr, 1, &imBar);

          vkEndCommandBuffer(initializeCmdBuffer);
        }

        // Submit to transition
        {
          VkSubmitInfo si         = {};
          si.commandBufferCount   = 1;
          si.pCommandBuffers      = &initializeCmdBuffer;
          si.pNext                = nullptr;
          si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
          si.waitSemaphoreCount   = 0;
          si.signalSemaphoreCount = 0;
          ThrowIfFailed(vkQueueSubmit(queueFam->m_queues[0]->vk_queue, 1, &si, VK_NULL_HANDLE),
                        "Queue submission for layout transition");
        }
      }
    }
    vkDestroyCommandPool(GPU->vk_logical, initializeCmdPool, nullptr);

    queueFamListIterIndx++;
  }
  
  // Create TEXTURE_VKOBJs and return handles
  window->vk_swapchainTextureUsage = swpchn_ci.imageUsage;
  for (uint32_t vkim_index = 0; vkim_index < desc->imageCount; vkim_index++) {
    TEXTURE_VKOBJ* SWAPCHAINTEXTURE = contentmanager->GETTEXTURES_ARRAY().create_OBJ();
    SWAPCHAINTEXTURE->m_channels    = texture_channels_tgfx_BGRA8UNORM;
    SWAPCHAINTEXTURE->m_width       = window->m_newWidth;
    SWAPCHAINTEXTURE->m_height      = window->m_newHeight;
    SWAPCHAINTEXTURE->m_mips        = 1;
    SWAPCHAINTEXTURE->vk_image      = SWPCHN_IMGs[vkim_index];
    SWAPCHAINTEXTURE->vk_imageView  = SWPCHN_IMGVIEWs[vkim_index];
    SWAPCHAINTEXTURE->vk_imageUsage = window->vk_swapchainTextureUsage;
    SWAPCHAINTEXTURE->m_dim         = texture_dimensions_tgfx_2D;
    SWAPCHAINTEXTURE->m_GPU         = GPU->gpuIndx();
    // No memory allocation is possible for these textures
    SWAPCHAINTEXTURE->m_memBlock = memoryBlock_vk::GETINVALID();
    SWAPCHAINTEXTURE->m_memReqs  = memoryRequirements_vk::GETINVALID();

    textures[vkim_index] =
      contentmanager->GETTEXTURES_ARRAY().returnHANDLEfromOBJ(SWAPCHAINTEXTURE);
    window->m_swapchainTextures[vkim_index] = textures[vkim_index];
  }
  return result_tgfx_SUCCESS;
}
result_tgfx vk_getCurrentSwapchainTextureIndex(window_tgfxhnd i_window, uint32_t* index) {
  WINDOW_VKOBJ* window = hidden->WINDOWs.getOBJfromHANDLE(i_window);

  uint32_t swpchnIndx = UINT32_MAX;
  THROW_RETURN_IF_FAIL(
    vkAcquireNextImageKHR(window->m_gpu->vk_logical, window->vk_swapchain, UINT64_MAX,
                          window->vk_acquireSemaphore, nullptr, &swpchnIndx),
    "Acquiring swapchain texture has failed!", result_tgfx_FAIL);
  if (UINT32_MAX == swpchnIndx) {
    printer(result_tgfx_FAIL, "Acquire failed because acquiring gave UINT32_MAX!");
    *index = UINT32_MAX;
    return result_tgfx_FAIL;
  }

  // Set current swapchain index
  window->m_swapchainCurrentTextureIndx = swpchnIndx;
  if (index) {
    *index = swpchnIndx;
  }
  return result_tgfx_SUCCESS;
}
void vk_takeInputs() {
  glfwPollEvents();
  /*
  if (hidden->isAnyWindowResized) {
          vkDeviceWaitIdle(rendergpu->devLogical);
          for (unsigned int WindowIndex = 0; WindowIndex < hidden->WINDOWs.size(); WindowIndex++) {
                  WINDOW_VKOBJ* VKWINDOW = hidden->WINDOWs[WindowIndex];
                  if (!VKWINDOW->isALIVE.load()) { continue; }
                  if (!VKWINDOW->isResized) {
                          continue;
                  }
                  if (VKWINDOW->NEWWIDTH == VKWINDOW->LASTWIDTH && VKWINDOW->NEWHEIGHT ==
  VKWINDOW->LASTHEIGHT) { VKWINDOW->isResized = false; continue;
                  }

                  printer(result_tgfx_NOTCODED, "Resize functionality in take_inputs() was designed
  before swapchain creation exposed, so please re-design it");


                  /*
                  VkSwapchainKHR swpchn;
                  TEXTURE_VKOBJ* swpchntextures[VKCONST_SwapchainTextureCountPerWindow] = {};
                  //If new window size isn't able to create a swapchain, return to last window size
                  if (!Create_WindowSwapchain(VKWINDOW, VKWINDOW->NEWWIDTH, VKWINDOW->NEWHEIGHT,
  &swpchn, swpchntextures)) { printer(result_tgfx_FAIL, "New size for the window is not possible,
  returns to the last successful size!"); glfwSetWindowSize(VKWINDOW->GLFW_WINDOW,
  VKWINDOW->LASTWIDTH, VKWINDOW->LASTHEIGHT); VKWINDOW->isResized = false; continue;
                  }

                  TEXTURE_VKOBJ* oldswpchn[2] = {
  contentmanager->GETTEXTURES_ARRAY().getOBJbyINDEX(VKWINDOW->Swapchain_Textures[0]),
                          contentmanager->GETTEXTURES_ARRAY().getOBJbyINDEX(VKWINDOW->Swapchain_Textures[1])
  }; for (unsigned int texture_i = 0; texture_i < VKCONST_SwapchainTextureCountPerWindow;
  texture_i++) { vkDestroyImageView(rendergpu->devLogical, oldswpchn[texture_i]->ImageView,
  nullptr); oldswpchn[texture_i]->Image = VK_NULL_HANDLE; oldswpchn[texture_i]->ImageView =
  VK_NULL_HANDLE;
                          core_tgfx_main->contentmanager->Delete_Texture((texture_tgfx_handle)oldswpchn[texture_i]);
                  }
                  vkDestroySwapchainKHR(rendergpu->devLogical, VKWINDOW->Window_SwapChain,
  nullptr);

                  VKWINDOW->LASTHEIGHT = VKWINDOW->NEWHEIGHT;
                  VKWINDOW->LASTWIDTH = VKWINDOW->NEWWIDTH;
                  //When you resize window at Frame1, user'd have to track swapchain texture state
  if I don't do this here
                  //So please don't touch!
                  VKWINDOW->Swapchain_Textures[0] = core_tgfx_main->renderer->GetCurrentFrameIndex()
  ? contentmanager->GETTEXTURES_ARRAY().getINDEXbyOBJ(swpchntextures[1]) :
  contentmanager->GETTEXTURES_ARRAY().getINDEXbyOBJ(swpchntextures[0]);
                  VKWINDOW->Swapchain_Textures[1] = core_tgfx_main->renderer->GetCurrentFrameIndex()
  ? contentmanager->GETTEXTURES_ARRAY().getINDEXbyOBJ(swpchntextures[0]) :
  contentmanager->GETTEXTURES_ARRAY().getINDEXbyOBJ(swpchntextures[1]); VKWINDOW->Window_SwapChain =
  swpchn; VKWINDOW->CurrentFrameSWPCHNIndex = 0; VKWINDOW->resize_cb((window_tgfx_handle)VKWINDOW,
  VKWINDOW->UserPTR, VKWINDOW->NEWWIDTH, VKWINDOW->NEWHEIGHT,
  (texture_tgfx_handle*)VKWINDOW->Swapchain_Textures);

          }
  }*/
}
void vk_saveMonitors() {
  int           monitor_count;
  GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
  for (int i = 0; i < monitor_count; i++) {
    GLFWmonitor* monitor = monitors[i];

    // Get monitor name provided by OS! It is a driver based name, so it maybe incorrect!
    const char*    monitor_name = glfwGetMonitorName(monitor);
    MONITOR_VKOBJ* Monitor      = hidden->MONITORs.add();
    Monitor->name               = monitor_name;

    // Get videomode to detect at which resolution the OS is using the monitor
    const GLFWvidmode* monitor_vid_mode = glfwGetVideoMode(monitor);
    Monitor->width                      = monitor_vid_mode->width;
    Monitor->height                     = monitor_vid_mode->height;
    Monitor->color_bites                = monitor_vid_mode->blueBits;
    Monitor->refresh_rate               = monitor_vid_mode->refreshRate;

    // Get monitor's physical size, developer may want to use it!
    int physical_width, physical_height;
    glfwGetMonitorPhysicalSize(monitor, &physical_width, &physical_height);
    if (physical_width == 0 || physical_height == 0) {
      printer(result_tgfx_WARNING,
              "One of the monitors have invalid physical sizes, please be careful");
    }
    Monitor->physical_width  = physical_width;
    Monitor->physical_height = physical_height;
    Monitor->monitorobj      = monitor;
  }
}

inline void vk_getMonitorList(monitor_tgfxlsthnd* MonitorList) {
  *MonitorList = hidden->MONITORs.returnHANDLELIST();
}
inline void vk_getGPUList(gpu_tgfxlsthnd* GPULIST) {
  *GPULIST = hidden->DEVICE_GPUs.returnHANDLELIST();
}
inline void vk_getGPUInfoGeneral(gpu_tgfxhnd h, tgfx_gpu_description* dsc) {
  GPU_VKOBJ* VKGPU = core_vk->getGPUs().getOBJfromHANDLE(h);
  *dsc             = VKGPU->desc;
}
inline void vk_getGPUInfoQueues(gpu_tgfxhnd GPUhnd, unsigned int queueFamIndx,
                                gpuQueue_tgfxlsthnd* queueList) {
  GPU_VKOBJ*   VKGPU = core_vk->getGPUs().getOBJfromHANDLE(GPUhnd);
  QUEUEFAM_VK* fam   = VKGPU->manager()->m_queueFams[queueFamIndx];
  *queueList         = fam->m_queues.returnHANDLELIST();
}
result_tgfx vk_getWindow_GPUSupport(window_tgfxhnd i_window, gpu_tgfxhnd gpu,
                                    windowGPUsupport_tgfx* info) {
  GPU_VKOBJ*    GPU    = core_vk->getGPUs().getOBJfromHANDLE(gpu);
  WINDOW_VKOBJ* window = core_vk->GETWINDOWs().getOBJfromHANDLE(i_window);

  VkSurfaceCapabilitiesKHR caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GPU->vk_physical, window->vk_surface, &caps);
  info->usageFlag = Get_TGFXTextureUsageFlag_byVk(caps.supportedUsageFlags);

  uint32_t           formatCount = 0;
  VkSurfaceFormatKHR formats[TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT];
  vkGetPhysicalDeviceSurfaceFormatsKHR(GPU->vk_physical, window->vk_surface, &formatCount,
                                       VK_NULL_HANDLE);
  THROW_RETURN_IF_FAIL(vkGetPhysicalDeviceSurfaceFormatsKHR(GPU->vk_physical, window->vk_surface,
                                                            &formatCount, formats),
                       "vkGetPhysicalDeviceSurfaceFormatsKHR failed!", result_tgfx_FAIL);
  if (formatCount > TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT) {
    printer(result_tgfx_WARNING,
            "Current window has TGFX_WINDOWGPUSUPPORT_MAXFORMATCOUNT+ supported swapchain formats, "
            "which backend supports only VKCONST_MAXSURFACEFORMATCOUNT. Report this issue!");
  }
  for (uint32_t i = 0; i < formatCount; i++) {
    info->colorSpace[i] = vk_findColorSpaceTgfx(formats[i].colorSpace);
    info->channels[i]   = vk_findTextureChannelsTgfx(formats[i].format);
  }

  if (caps.maxImageCount <= caps.minImageCount) {
    printer(result_tgfx_FAIL,
            "VulkanCore: Window Surface Capabilities have issues, maxImageCount <= minImageCount!");
    return result_tgfx_FAIL;
  }
  if (caps.maxImageCount == 0) {
    info->maxImageCount = UINT32_MAX;
  } else {
    info->maxImageCount = caps.maxImageCount;
  }

  info->maxExtent.x = caps.maxImageExtent.width;
  info->maxExtent.y = caps.maxImageExtent.height;
  info->minExtent.x = caps.minImageExtent.width;
  info->minExtent.y = caps.minImageExtent.height;

  uint32_t         PresentationModesCount = 0;
  VkPresentModeKHR Presentations[TGFX_WINDOWGPUSUPPORT_MAXPRESENTATIONMODE];
  vkGetPhysicalDeviceSurfacePresentModesKHR(GPU->vk_physical, window->vk_surface,
                                            &PresentationModesCount, VK_NULL_HANDLE);
  THROW_RETURN_IF_FAIL(
    vkGetPhysicalDeviceSurfacePresentModesKHR(GPU->vk_physical, window->vk_surface,
                                              &PresentationModesCount, Presentations),
    "vkGetPhysicalDeviceSurfacePresentModesKHR failed!", result_tgfx_FAIL);
  if (PresentationModesCount > TGFX_WINDOWGPUSUPPORT_MAXPRESENTATIONMODE) {
    printer(result_tgfx_WARNING,
            "More presentation modes than predefined, possible memory corruption");
  }
  for (uint32_t i = 0; i < PresentationModesCount; i++) {
    info->presentationModes[i] = vk_findPresentModeTgfx(Presentations[i]);
  }

  uint32_t supportedQueueCount = 0;
  for (uint32_t queueFamIndx = 0; queueFamIndx < GPU->manager()->m_queueFams.size();
       queueFamIndx++) {
    VkBool32 isSupported = false;
    ThrowIfFailed(vkGetPhysicalDeviceSurfaceSupportKHR(GPU->vk_physical, queueFamIndx,
                                                       window->vk_surface, &isSupported),
                  "Querying queue family's swapchain support has failed!");
    QUEUEFAM_VK* queueFam = GPU->manager()->m_queueFams[queueFamIndx];
    if (isSupported) {
      for (uint32_t queueIndx = 0; queueIndx < queueFam->m_queues.size(); queueIndx++) {
        info->queues[supportedQueueCount++] =
          queueFam->m_queues.returnHANDLEfromOBJ(queueFam->m_queues[queueIndx]);
      }
    }
  }
  info->queues[supportedQueueCount] = ( gpuQueue_tgfxhnd )core_tgfx_main->INVALIDHANDLE;

  return result_tgfx_SUCCESS;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//				Backend Initialization
////////////////////////////////////////////////
////////////////////////////////////////////////

void vk_printfLog(result_tgfx result, const char* text) {
  printf("TGFX %u: %s\n", ( unsigned int )result, text);
}

#ifndef NO_IMGUI
// extern void Create_IMGUI();
#endif // !NO_IMGUI

extern void vk_createBackendAllocator();
extern void vk_createContentManager();
// extern void Create_SyncSystems();
void        vk_setupDebugging();
void        vk_initRenderer();
void        vk_setHelperFuncPtrs();
extern void vk_setQueueFuncPtrs();
void        vk_errorCallback(int error_code, const char* description) {
         printer(result_tgfx_FAIL, (std::string("GLFW error: ") + description).c_str());
}
result_tgfx vk_load(ecs_tapi* regsys, core_tgfx_type* core, tgfx_PrintLogCallback printcallback) {
  if (!regsys->getSystem(TGFX_PLUGIN_NAME)) return result_tgfx_FAIL;

  printer_cb = printcallback;

  VIRTUALMEMORY_TAPI_PLUGIN_TYPE virmemsystype =
    ( VIRTUALMEMORY_TAPI_PLUGIN_TYPE )regsys->getSystem(VIRTUALMEMORY_TAPI_PLUGIN_NAME);
  if (!virmemsystype) {
    printer(result_tgfx_FAIL,
            "Vulkan backend needs virtual memory system, so initialization has failed!");
    return result_tgfx_FAIL;
  } else {
    virmemsys = virmemsystype->funcs;
  }

  // Check if threading system is loaded
  THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE threadsysloaded =
    ( THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE )regsys->getSystem(THREADINGSYS_TAPI_PLUGIN_NAME);
  if (threadsysloaded) {
    threadingsys = threadsysloaded->funcs;
    threadcount  = threadingsys->thread_count();
  }

  core_tgfx_main = core->api;
  vk_createBackendAllocator();
  uint32_t malloc_size = sizeof(core_public) + sizeof(core_private);
  uint32_t allocptr    = vk_virmem::allocatePage(malloc_size);
  core_vk              = ( core_public* )VK_MEMOFFSET_TO_POINTER(allocptr);
  core->data           = ( core_tgfx_d* )core_vk;
  hidden               = ( core_private* )VK_MEMOFFSET_TO_POINTER(allocptr + sizeof(core_public));
  virmemsys->virtual_commit(VK_MEMOFFSET_TO_POINTER(allocptr), malloc_size);

  // Set function pointers
  {
    core->api->initGPU                         = &vk_initGPU;
    core->api->createWindow                    = &vk_createWindow;
    core->api->getmonitorlist                  = &vk_getMonitorList;
    core->api->getGPUlist                      = &vk_getGPUList;
    core->api->createSwapchain                 = &vk_createSwapchain;
    core->api->helpers->getGPUInfo_General     = &vk_getGPUInfoGeneral;
    core->api->helpers->getGPUInfo_Queues      = &vk_getGPUInfoQueues;
    core->api->helpers->getWindow_GPUSupport   = &vk_getWindow_GPUSupport;
    core->api->getCurrentSwapchainTextureIndex = &vk_getCurrentSwapchainTextureIndex;
    // core->api->change_window_resolution = &change_window_resolution;
    // core->api->debugcallback = &debugcallback;
    // core->api->debugcallback_threaded = &debugcallback_threaded;
    // core->api->destroy_tgfx_resources = &destroy_tgfx_resources;
  }

  // Set error callback to handle all glfw errors (including initialization error)!
  glfwSetErrorCallback(vk_errorCallback);

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  vk_saveMonitors();

  vk_createInstance();
#ifdef VULKAN_DEBUGGING
  vk_setupDebugging();
#endif
  vk_checkComputerSpecs();

  // Init systems
  vk_setQueueFuncPtrs();
  vk_createContentManager();
  vk_initRenderer();
  vk_setHelperFuncPtrs();
  return result_tgfx_SUCCESS;
}

TGFX_BACKEND_ENTRY() { return vk_load(ecsSys, core, printCallback); }

////////////////////////////////////////////////
////////////////////////////////////////////////
//				Vulkan Debugging Layer
// It's placed at the end because it won't be changed
////////////////////////////////////////////////
////////////////////////////////////////////////

VKAPI_ATTR VkBool32 VKAPI_CALL
vk_debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      Message_Severity,
                 VkDebugUtilsMessageTypeFlagsEXT             Message_Type,
                 const VkDebugUtilsMessengerCallbackDataEXT* pCallback_Data, void* pUserData) {
  const char* Callback_Type = "";
  switch (Message_Type) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
      Callback_Type =
        "VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT : Some event has happened that "
        "is unrelated to the specification or performance\n";
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
      Callback_Type =
        "VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: Something has happened that "
        "violates the specification or indicates a possible mistake\n";
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
      Callback_Type =
        "VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: Potential non-optimal use of Vulkan\n";
      break;
    default:
      printer(result_tgfx_FAIL, "Vulkan Callback has returned a unsupported Message_Type\n");
      return true;
      break;
  }

  switch (Message_Severity) {
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
loadVkExtFunc(vkCreateDebugUtilsMessengerEXT);
loadVkExtFunc(vkDestroyDebugUtilsMessengerEXT);

VkDebugUtilsMessengerEXT dbg_mssngr = VK_NULL_HANDLE;

void vk_setupDebugging() {
  vkCreateDebugUtilsMessengerEXT_loaded  = vkCreateDebugUtilsMessengerEXT_loadVkFunc();
  vkDestroyDebugUtilsMessengerEXT_loaded = vkDestroyDebugUtilsMessengerEXT_loadVkFunc();

  VkDebugUtilsMessengerCreateInfoEXT dbg_mssngr_ci = {};
  dbg_mssngr_ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  dbg_mssngr_ci.messageSeverity =
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  dbg_mssngr_ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  dbg_mssngr_ci.pfnUserCallback = vk_debugCallback;
  dbg_mssngr_ci.pNext           = nullptr;
  dbg_mssngr_ci.pUserData       = nullptr;

  ThrowIfFailed(
    vkCreateDebugUtilsMessengerEXT_loaded(VKGLOBAL_INSTANCE, &dbg_mssngr_ci, nullptr, &dbg_mssngr),
    "Vulkan's Debug Callback system failed to start!");
}