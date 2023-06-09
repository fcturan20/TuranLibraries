#pragma once
#include <atomic>
#include <string>
#include <vector>

#include "tgfx_structs.h"
#include "vk_predefinitions.h"
#include "vulkan/vulkan.h"

struct core_public {
  GPU_VKOBJ* getGPU(uint32_t i);
  uint32_t   gpuCount();
};

struct GPU_VKOBJ {
  bool            isALIVE    = 1;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::GPU;
  static uint16_t GET_EXTRAFLAGS(GPU_VKOBJ* obj) { return obj->m_gpuIndx; }

  tgfx_gpuDescription desc;

  VkPhysicalDevice                  vk_physical             = {};
  VkDevice                          vk_logical              = {};
  VkPhysicalDeviceProperties2       vk_propsDev             = {};
  VkPhysicalDeviceFeatures2         vk_featuresDev          = {};
  VkPhysicalDeviceMemoryProperties2 vk_propsMemory          = {};
  tgfx_memoryDescription            m_memoryDescTGFX[32]    = {};
  struct tgfx_texture*                   m_invalidStorageTexture = {}, *m_invalidShaderReadTexture = {};
  struct tgfx_sampler*                   m_invalidSampler = {};
  struct tgfx_buffer*                    m_invalidBuffer  = {};

  VkQueueFamilyProperties2 vk_propsQueue[VKCONST_MAXQUEUEFAMCOUNT_PERGPU] = {};
  uint32_t                 m_queueFamPtrs[VKCONST_MAXQUEUEFAMCOUNT_PERGPU] = {};
  struct tgfx_gpuQueue*         m_internalQueue                                = {};

 private:
  extManager_vkDevice* m_extensions;
  uint8_t              m_gpuIndx = 255;

 public:
  const extManager_vkDevice* ext() const { return m_extensions; }
  extManager_vkDevice*&      ext() { return m_extensions; }
  uint8_t                    gpuIndx() const { return m_gpuIndx; }
  void                       setGPUINDX(uint8_t v) { m_gpuIndx = v; }
};

struct MONITOR_VKOBJ {
  bool            isALIVE    = false;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::MONITOR;
  static uint16_t GET_EXTRAFLAGS(MONITOR_VKOBJ* obj) { return 0; }

  MONITOR_VKOBJ() = default;
  MONITOR_VKOBJ(const MONITOR_VKOBJ& copyFrom) { *this = copyFrom; }
  tgfx_uvec2   res, physicalSize;
  unsigned int color_bites = 0, refresh_rate = 00;
  const wchar_t*  name            = NULL;
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
  const wchar_t*               m_name        = nullptr;
  tgfx_windowResizeCallback m_resizeFnc   = nullptr;
  tgfx_windowKeyCallback    m_keyFnc      = nullptr;
  void*                     m_userData    = nullptr;
  struct tgfx_texture*           m_swapchainTextures[VKCONST_MAXSWPCHNTXTURECOUNT_PERWINDOW] = {};
  unsigned char             m_swapchainTextureCount = 0, m_swapchainCurrentTextureIndx = 0;
  bool                      m_isResized = false, m_isSwapped = false;
  // Presentation Fences should only be used for CPU to wait
  struct tgfx_fence* m_presentationFences[VKCONST_MAXSWPCHNTXTURECOUNT_PERWINDOW];
  VkSemaphore   vk_acquireSemaphore = {};
  bool          m_isMouseButtonPressed[3] = {};

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