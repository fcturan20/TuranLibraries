#pragma once
#include <atomic>
#include <string>
#include <vector>

#include "tgfx_structs.h"
#include "vk_predefinitions.h"
#include "vulkan/vulkan.h"

struct core_public {
  VK_STATICVECTOR<WINDOW_VKOBJ, window_tgfxhnd, VKCONST_MAXWINDOWCOUNT>& GETWINDOWs();
  VK_STATICVECTOR<GPU_VKOBJ, gpu_tgfxhnd, VKCONST_MAXGPUCOUNT>&          getGPUs();
};

static uint32_t gpuCounter = 0;
struct GPU_VKOBJ {
  bool            isALIVE    = 1;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::GPU;
  static uint16_t GET_EXTRAFLAGS(GPU_VKOBJ* obj) { return obj->m_gpuIndx; }

  tgfx_gpu_description desc;

  VkPhysicalDevice                  vk_physical          = {};
  VkDevice                          vk_logical           = {};
  VkPhysicalDeviceProperties2       vk_propsDev          = {};
  VkPhysicalDeviceFeatures2         vk_featuresDev       = {};
  VkPhysicalDeviceMemoryProperties2 vk_propsMemory       = {};
  memoryDescription_tgfx            m_memoryDescTGFX[32] = {};
  texture_tgfxhnd                   m_invalidStorageTexture = {}, m_invalidShaderReadTexture = {};
  sampler_tgfxhnd                   m_invalidSampler = {};
  buffer_tgfxhnd                    m_invalidBuffer  = {};

  VkQueueFamilyProperties2 vk_propsQueue[VKCONST_MAXQUEUEFAMCOUNT_PERGPU] = {};

 private:
  extManager_vkDevice* m_extensions;
  manager_vk*          m_manager = nullptr;
  uint8_t              m_gpuIndx = 255;

 public:
  const extManager_vkDevice* ext() const { return m_extensions; }
  extManager_vkDevice*&      ext() { return m_extensions; }
  const manager_vk*          manager() const { return m_manager; }
  manager_vk*&               manager() { return m_manager; }
  uint8_t                    gpuIndx() const { return m_gpuIndx; }
  void                       setGPUINDX(uint8_t v) { m_gpuIndx = v; }
};

struct MONITOR_VKOBJ {
  bool            isALIVE    = false;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::MONITOR;
  static uint16_t GET_EXTRAFLAGS(MONITOR_VKOBJ* obj) { return 0; }

  MONITOR_VKOBJ() = default;
  MONITOR_VKOBJ(const MONITOR_VKOBJ& copyFrom) { *this = copyFrom; }
  unsigned int width = 0, height = 0, color_bites = 0, refresh_rate = 0, physical_width = 0,
               physical_height = 0;
  const char*  name            = NULL;
  GLFWmonitor* monitorobj      = NULL;
};

struct WINDOW_VKOBJ {
  bool            isALIVE    = false;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::WINDOW;
  static uint16_t GET_EXTRAFLAGS(WINDOW_VKOBJ* obj) { return 0; }

  WINDOW_VKOBJ() = default;
  unsigned int              m_lastWidth, m_lastHeight, m_newWidth, m_newHeight;
  windowmode_tgfx           m_displayMode = windowmode_tgfx_WINDOWED;
  MONITOR_VKOBJ*            m_monitor     = nullptr;
  GPU_VKOBJ*                m_gpu         = nullptr;
  const char*               m_name        = nullptr;
  tgfx_windowResizeCallback m_resizeFnc   = nullptr;
  tgfx_windowKeyCallback    m_keyFnc      = nullptr;
  void*                     m_userData    = nullptr;
  texture_tgfxhnd           m_swapchainTextures[VKCONST_MAXSWPCHNTXTURECOUNT_PERWINDOW] = {};
  unsigned char             m_swapchainTextureCount = 0, m_swapchainCurrentTextureIndx = 0;
  bool                      m_isResized = false, m_isSwapped = false;
  // Presentation Fences should only be used for CPU to wait
  fence_tgfxlsthnd m_presentationFences;
  VkSemaphore      vk_acquireSemaphore = {};

  VkSurfaceKHR    vk_surface    = {};
  VkSwapchainKHR  vk_swapchain  = {};
  GLFWwindow*     vk_glfwWindow = {};
  VkCommandBuffer vk_generalToPresent[VKCONST_MAXQUEUEFAMCOUNT_PERGPU]
                                     [VKCONST_MAXSWPCHNTXTURECOUNT_PERWINDOW] = {},
    vk_presentToGeneral[VKCONST_MAXQUEUEFAMCOUNT_PERGPU][VKCONST_MAXSWPCHNTXTURECOUNT_PERWINDOW] =
      {};
  struct PresentationModes {
    unsigned char immediate : 1;
    unsigned char mailbox : 1;
    unsigned char fifo : 1;
    unsigned char fifo_relaxed : 1;
    PresentationModes() : immediate(0), mailbox(0), fifo(0), fifo_relaxed(0) {}
  };
  PresentationModes m_presentModes;
  VkImageUsageFlags vk_swapchainTextureUsage;
};