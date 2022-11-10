/*
  This file is responsible for managing TGFX Command Bundle.
  TGFX Command Bundle = Secondary Command Buffers.
*/
#include "vk_renderer.h"

#include <algorithm>
#include <numeric>
#include <utility>

#include "tgfx_renderer.h"
#include "vk_contentmanager.h"
#include "vk_core.h"
#include "vk_predefinitions.h"
#include "vk_queue.h"
#include "vk_resource.h"
#include "vkext_timelineSemaphore.h"

vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_RENDERER = nullptr;

struct vk_cmd;
struct CMDBUNDLE_VKOBJ {
  bool            isALIVE    = false;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::CMDBUNDLE;
  static uint16_t GET_EXTRAFLAGS(CMDBUNDLE_VKOBJ* obj) { return 0; }

  VkCommandBuffer vk_cb = nullptr;
  // Render States
  VkPipeline vk_activePipeline = {};
  // To check pipeline compatibility
  VkPipelineLayout      vk_activePipelineLayout                       = {};
  VkDescriptorSetLayout vk_activeDescSets[VKCONST_MAXDESCSET_PERLIST] = {};
  uint8_t               vk_callBuffer[128] = {}; // CallBuffer = Push Constants = 128 byte

  QUEUEFAM_VK* m_queueFam = nullptr;
  GPU_VKOBJ*   m_gpu      = nullptr;
  uint32_t     m_poolIndx = UINT32_MAX;
  vk_cmd*      m_cmds     = nullptr;
  uint64_t     m_cmdCount = 0;

  void createCmdBuffer(uint64_t cmdCount);
};

#define vkEnumType_cmdType() uint32_t
enum class vk_cmdType : vkEnumType_cmdType(){
  error = VK_PRIM_MIN(vkEnumType_cmdType()), // cmdType neither should be 0 nor 255
  bindBindingTables,
  bindVertexBuffer,
  bindIndexBuffer,
  bindViewport,
  drawNonIndexedDirect,
  drawIndexedDirect,
  drawNonIndexedIndirect,
  drawIndexedIndirect,
  barrierTexture,
  barrierBuffer,
  bindPipeline,
  dispatch,
  error_2 = VK_PRIM_MAX(vkEnumType_cmdType())};
// Template struct for new cmd structs
// Then specify the struct in vkCmdStructsLists
struct vkCmdStruct_example {
  // Necessary template variables & functions should have "cmd_" prefix
  static constexpr vk_cmdType type = vk_cmdType::error;
  vkCmdStruct_example()            = default;
  void cmd_execute(CMDBUNDLE_VKOBJ* cmdBundle) {}
};

struct vkCmdStruct_barrierTexture {
  static constexpr vk_cmdType cmd_type = vk_cmdType::barrierTexture;

  void cmd_execute(CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdPipelineBarrier(cmdBundle->vk_cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0,
                         nullptr, 0, nullptr, 1, &m_imBar);
  }

  // Command specific variables should have "m_" prefix
  VkImageMemoryBarrier m_imBar = {};
};

struct vkCmdStruct_bindBindingTables {
  static constexpr vk_cmdType cmd_type = vk_cmdType::bindBindingTables;

  void cmd_execute(CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdBindDescriptorSets(cmdBundle->vk_cb, vk_bindPoint, cmdBundle->vk_activePipelineLayout,
                            m_firstSetIndx, m_setCount, vk_sets, 0, nullptr);
    for (uint32_t i = m_firstSetIndx; i < VKCONST_MAXDESCSET_PERLIST; i++) {
      cmdBundle->vk_activeDescSets[i] = vk_setLayouts[i - m_firstSetIndx];
    }
  }
  VkPipelineBindPoint   vk_bindPoint                              = VK_PIPELINE_BIND_POINT_MAX_ENUM;
  VkDescriptorSet       vk_sets[VKCONST_MAXDESCSET_PERLIST]       = {};
  VkDescriptorSetLayout vk_setLayouts[VKCONST_MAXDESCSET_PERLIST] = {};
  uint32_t              m_setCount = 0, m_firstSetIndx = 0;
};

struct vkCmdStruct_bindPipeline {
  static constexpr vk_cmdType cmd_type = vk_cmdType::bindPipeline;

  void cmd_execute(CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdBindPipeline(cmdBundle->vk_cb, vk_bindPoint, vk_pipeline);
    cmdBundle->vk_activePipeline       = vk_pipeline;
    cmdBundle->vk_activePipelineLayout = vk_pipelineLayout;
  }
  VkPipelineBindPoint vk_bindPoint      = VK_PIPELINE_BIND_POINT_MAX_ENUM;
  VkPipeline          vk_pipeline       = {};
  VkPipelineLayout    vk_pipelineLayout = {};
};

struct vkCmdStruct_dispatch {
  static constexpr vk_cmdType cmd_type = vk_cmdType::dispatch;

  void cmd_execute(CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdDispatch(cmdBundle->vk_cb, m_dispatchSize.x, m_dispatchSize.y, m_dispatchSize.z);
  };
  uvec3_tgfx m_dispatchSize;
};

struct vk_cmd {
  vk_cmdType cmd_type = vk_cmdType::error_2;

  // From https://stackoverflow.com/a/46408751
  template <typename... T>
  static constexpr size_t max_sizeof() {
    return std::max({sizeof(T)...});
  }

#define vkCmdStructsLists                                                         \
  vkCmdStruct_example, vkCmdStruct_barrierTexture, vkCmdStruct_bindBindingTables, \
    vkCmdStruct_bindPipeline, vkCmdStruct_dispatch
  static constexpr uint32_t maxCmdStructSize = max_sizeof<vkCmdStructsLists>();
  uint8_t                   cmd_data[maxCmdStructSize];
  vk_cmd() : cmd_type(vk_cmdType::error) {}
};
static constexpr uint32_t sdf = sizeof(vk_cmd);

template <typename T>
T* vk_createCmdStruct(vk_cmd* cmd) {
  static_assert(T::cmd_type != vk_cmdType::error,
                "You forgot to specify command type as \"cmd_type\" variable in command struct");
  cmd->cmd_type = T::cmd_type;
  static_assert(vk_cmd::maxCmdStructSize >= sizeof(T),
                "You forgot to specify the struct in vk_cmd::maxCmdStructSize!");
  *( T* )cmd->cmd_data = T();
  return ( T* )cmd->cmd_data;
}

void vk_executeCmd(CMDBUNDLE_VKOBJ* bundle, const vk_cmd& cmd) {
  switch (cmd.cmd_type) {
    case vk_cmdType::barrierTexture:
      (( vkCmdStruct_barrierTexture* )cmd.cmd_data)->cmd_execute(bundle);
      break;
    case vk_cmdType::bindBindingTables:
      (( vkCmdStruct_bindBindingTables* )cmd.cmd_data)->cmd_execute(bundle);
      break;
    case vk_cmdType::bindPipeline:
      (( vkCmdStruct_bindPipeline* )cmd.cmd_data)->cmd_execute(bundle);
      break;
    case vk_cmdType::dispatch: (( vkCmdStruct_dispatch* )cmd.cmd_data)->cmd_execute(bundle); break;
    case vk_cmdType::error:
    case vk_cmdType::error_2:
    default: printer(result_tgfx_WARNING, "One of the cmds is not used!");
  }
}

void CMDBUNDLE_VKOBJ::createCmdBuffer(uint64_t cmdCount) {
  vk_virmem::dynamicmem* cmdVirMem = vk_virmem::allocate_dynamicmem(sizeof(vk_cmd) * cmdCount);
  // I don't want to bother with commit operations for now, commit all cmds
  m_cmds     = new (cmdVirMem) vk_cmd[cmdCount];
  m_cmdCount = cmdCount;
}

struct vk_renderer_private {
  VK_STATICVECTOR<CMDBUNDLE_VKOBJ, commandBundle_tgfxhnd, UINT16_MAX> m_cmdBundles;
};
vk_renderer_private* hiddenRenderer = nullptr;

// Synchronization Functions

void vk_createFences(gpu_tgfxhnd gpu, unsigned int fenceCount, uint32_t isSignaled,
                     fence_tgfxlsthnd fenceList) {
  GPU_VKOBJ* GPU = core_vk->getGPUs().getOBJfromHANDLE(gpu);
  for (uint32_t i = 0; i < fenceCount; i++) {
    fenceList[i] = vk_createTGFXFence(GPU, isSignaled);
  }
}

// Command Bundle Functions
////////////////////////////

commandBundle_tgfxhnd vk_beginCommandBundle(gpuQueue_tgfxhnd      i_queue,
                                            renderSubPass_tgfxhnd subpassHandle,
                                            unsigned long long    maxCmdCount,
                                            extension_tgfxlsthnd  exts) {
  getGPUfromQueueHnd(i_queue);

  VkCommandBuffer vk_cmdBuffer = VK_NULL_HANDLE;
  vk_allocateCmdBuffer(fam, VK_COMMAND_BUFFER_LEVEL_SECONDARY, &vk_cmdBuffer, 1);
  CMDBUNDLE_VKOBJ* cmdBundle = hiddenRenderer->m_cmdBundles.add();

  cmdBundle->vk_cb = vk_cmdBuffer;

  cmdBundle->m_poolIndx = threadingsys->this_thread_index();
  cmdBundle->m_queueFam = fam;
  cmdBundle->m_gpu      = gpu;
  cmdBundle->createCmdBuffer(maxCmdCount);

  return hiddenRenderer->m_cmdBundles.returnHANDLEfromOBJ(cmdBundle);
}
void vk_finishCommandBundle(commandBundle_tgfxhnd i_bundle, extension_tgfxlsthnd exts) {
  CMDBUNDLE_VKOBJ*         bundle        = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(i_bundle);
  VkCommandBufferBeginInfo bi            = {};
  bi.flags                               = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  bi.pNext                               = nullptr;
  bi.sType                               = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  VkCommandBufferInheritanceInfo secInfo = {};
  secInfo.sType                          = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
  bi.pInheritanceInfo                    = &secInfo;
  assert(!ThrowIfFailed(vkBeginCommandBuffer(bundle->vk_cb, &bi),
                        "vkBeginCommandBuffer() shouldn't fail!"));
  for (uint64_t cmdIndx = 0; cmdIndx < bundle->m_cmdCount; cmdIndx++) {
    vk_executeCmd(bundle, bundle->m_cmds[cmdIndx]);
  }
  assert(!ThrowIfFailed(vkEndCommandBuffer(bundle->vk_cb), "vkEndCommandBuffer() shouldn't fail!"));
}
void vk_destroyCommandBundle(commandBundle_tgfxhnd hnd) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(hnd);
  vkFreeCommandBuffers(bundle->m_gpu->vk_logical,
                       vk_getSecondaryCmdPool(bundle->m_queueFam, bundle->m_poolIndx), 1,
                       &bundle->vk_cb);
  hiddenRenderer->m_cmdBundles.erase(hiddenRenderer->m_cmdBundles.getINDEX_byOBJ(bundle));
}

void vk_cmdBindBindingTables(commandBundle_tgfxhnd i_bundle, unsigned long long sortKey,
                             bindingTable_tgfxlsthnd bindingTables, unsigned int firstSetIndx,
                             pipelineType_tgfx pipelineType) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(i_bundle);
  auto* cmd = vk_createCmdStruct<vkCmdStruct_bindBindingTables>(&bundle->m_cmds[sortKey]);

  {
    TGFXLISTCOUNT(core_tgfx_main, bindingTables, bindingTableListSize);
    for (uint32_t i = 0; i < bindingTableListSize; i++) {
      BINDINGTABLEINST_VKOBJ* bindingTable =
        contentmanager->GETBINDINGTABLE_ARRAY().getOBJfromHANDLE(bindingTables[i]);
      if (bindingTable) {
        cmd->vk_sets[cmd->m_setCount] = bindingTable->vk_set;
        BINDINGTABLETYPE_VKOBJ* type =
          contentmanager->GETBINDINGTABLETYPE_ARRAY().getOBJfromHANDLE(bindingTable->m_type);
        cmd->vk_setLayouts[cmd->m_setCount++] = type->vk_layout;
      }
    }
  }

  cmd->vk_bindPoint   = vk_findPipelineBindPoint(pipelineType);
  cmd->m_firstSetIndx = firstSetIndx;
}
void vk_cmdBindPipeline(commandBundle_tgfxhnd i_bundle, unsigned long long sortKey,
                        pipeline_tgfxhnd pipeline) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(i_bundle);
  auto*            cmd    = vk_createCmdStruct<vkCmdStruct_bindPipeline>(&bundle->m_cmds[sortKey]);

  PIPELINE_VKOBJ* pipe   = contentmanager->GETPIPELINE_ARRAY().getOBJfromHANDLE(pipeline);
  cmd->vk_bindPoint      = pipe->vk_type;
  cmd->vk_pipeline       = pipe->vk_object;
  cmd->vk_pipelineLayout = pipe->vk_layout;
}
void vk_cmdBindVertexBuffer(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                            buffer_tgfxhnd buffer, unsigned long long offset,
                            unsigned long long dataSize) {
  // BUFFER_VKOBJ* vb = contentmanager->GETBUFFER_ARRAY().getOBJfromHANDLE(buffer);
  //  vkCmdBindVertexBuffers();
}
void vk_cmdBindIndexBuffers(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                            buffer_tgfxhnd buffer, unsigned long long offset,
                            unsigned char IndexTypeSize) {}
void vk_cmdDrawNonIndexedDirect(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                                unsigned int vertexCount, unsigned int instanceCount,
                                unsigned int firstVertex, unsigned int firstInstance) {}
void vk_cmdDrawIndexedDirect(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                             unsigned int indexCount, unsigned int instanceCount,
                             unsigned int firstIndex, int vertexOffset,
                             unsigned int firstInstance) {}
void vk_cmdDrawNonIndexedIndirect(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                                  buffer_tgfxhnd       drawDataBuffer,
                                  unsigned long long   drawDataBufferOffset,
                                  buffer_tgfxhnd       drawCountBuffer,
                                  unsigned long long   drawCountBufferOffset,
                                  extension_tgfxlsthnd exts) {}
void vk_cmdDrawIndexedIndirect(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                               buffer_tgfxhnd       drawCountBuffer,
                               unsigned long long   drawCountBufferOffset,
                               extension_tgfxlsthnd exts) {}
void vk_cmdBarrierTexture(commandBundle_tgfxhnd bndl, unsigned long long key,
                          texture_tgfxhnd i_texture, image_access_tgfx lastAccess,
                          image_access_tgfx nextAccess, textureUsageFlag_tgfxhnd lastUsage,
                          textureUsageFlag_tgfxhnd nextUsage, extension_tgfxlsthnd exts) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(bndl);
  auto*            cmdBar = vk_createCmdStruct<vkCmdStruct_barrierTexture>(&bundle->m_cmds[key]);
  cmdBar->m_imBar.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  cmdBar->m_imBar.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  cmdBar->m_imBar.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  cmdBar->m_imBar.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  cmdBar->m_imBar.subresourceRange.baseArrayLayer = 0;
  cmdBar->m_imBar.subresourceRange.baseMipLevel   = 0;
  cmdBar->m_imBar.subresourceRange.layerCount     = 1;
  cmdBar->m_imBar.subresourceRange.levelCount     = 1;
  TEXTURE_VKOBJ* texture = contentmanager->GETTEXTURES_ARRAY().getOBJfromHANDLE(i_texture);
  cmdBar->m_imBar.image  = texture->vk_image;
  Find_AccessPattern_byIMAGEACCESS(lastAccess, cmdBar->m_imBar.srcAccessMask,
                                   cmdBar->m_imBar.oldLayout);
  Find_AccessPattern_byIMAGEACCESS(nextAccess, cmdBar->m_imBar.dstAccessMask,
                                   cmdBar->m_imBar.newLayout);
  cmdBar->m_imBar.pNext = nullptr;
}
void vk_cmdDispatch(commandBundle_tgfxhnd bndl, unsigned long long key, uvec3_tgfx dispatchSize) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(bndl);
  auto*            cmd = vk_createCmdStruct<vkCmdStruct_dispatch>(&bundle->m_cmds[key]);

  cmd->m_dispatchSize = dispatchSize;
}

void set_VkRenderer_funcPtrs() {
  core_tgfx_main->renderer->cmdBindBindingTables      = vk_cmdBindBindingTables;
  core_tgfx_main->renderer->cmdBindIndexBuffers       = vk_cmdBindIndexBuffers;
  core_tgfx_main->renderer->cmdBindVertexBuffer       = vk_cmdBindVertexBuffer;
  core_tgfx_main->renderer->cmdDrawIndexedDirect      = vk_cmdDrawIndexedDirect;
  core_tgfx_main->renderer->cmdDrawIndexedIndirect    = vk_cmdDrawIndexedIndirect;
  core_tgfx_main->renderer->cmdDrawNonIndexedDirect   = vk_cmdDrawNonIndexedDirect;
  core_tgfx_main->renderer->cmdDrawNonIndexedIndirect = vk_cmdDrawNonIndexedIndirect;
  core_tgfx_main->renderer->cmdBarrierTexture         = vk_cmdBarrierTexture;
  core_tgfx_main->renderer->cmdBindPipeline           = vk_cmdBindPipeline;
  core_tgfx_main->renderer->cmdDispatch               = vk_cmdDispatch;

  core_tgfx_main->renderer->beginCommandBundle   = vk_beginCommandBundle;
  core_tgfx_main->renderer->finishCommandBundle  = vk_finishCommandBundle;
  core_tgfx_main->renderer->createFences         = vk_createFences;
  core_tgfx_main->renderer->destroyCommandBundle = vk_destroyCommandBundle;
  core_tgfx_main->renderer->getFenceValue        = vk_getFenceValue;
  core_tgfx_main->renderer->setFence             = vk_setFenceValue;
}

void vk_initRenderer() {
  set_VkRenderer_funcPtrs();
  VKGLOBAL_VIRMEM_RENDERER = vk_virmem::allocate_dynamicmem(sizeof(vk_renderer_private));
  hiddenRenderer           = new (VKGLOBAL_VIRMEM_RENDERER) vk_renderer_private;
}

// Helper functions

void VK_getQueueAndSharingInfos(gpuQueue_tgfxlsthnd i_queueList, extension_tgfxlsthnd i_exts,
                                uint32_t* o_famList, uint32_t* o_famListSize,
                                VkSharingMode* o_sharingMode) {
  TGFXLISTCOUNT(core_tgfx_main, i_queueList, i_listSize);
  uint32_t   validQueueFamCount = 0;
  GPU_VKOBJ* theGPU             = nullptr;
  for (uint32_t listIndx = 0; listIndx < i_listSize; listIndx++) {
    getGPUfromQueueHnd(i_queueList[listIndx]);
    if (!theGPU) {
      theGPU = gpu;
    }
    assert(theGPU == gpu && "Queues from different devices!");
    bool isPreviouslyAdded = false;
    for (uint32_t validQueueIndx = 0; validQueueIndx < validQueueFamCount; validQueueIndx++) {
      if (o_famList[validQueueIndx] == queue->vk_queueFamIndex) {
        isPreviouslyAdded = true;
        break;
      }
    }
    if (!isPreviouslyAdded) {
      o_famList[validQueueFamCount++] = queue->vk_queueFamIndex;
    }
  }
  if (validQueueFamCount > 1) {
    *o_sharingMode = VK_SHARING_MODE_CONCURRENT;
  } else {
    *o_sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }
  *o_famListSize = validQueueFamCount;
  for (uint32_t i = *o_famListSize; i < VKCONST_MAXQUEUEFAMCOUNT_PERGPU; i++) {
    o_famList[i] = UINT32_MAX;
  }
}

void vk_getSecondaryCmdBuffers(commandBundle_tgfxlsthnd commandBundleList,
                               VkCommandBuffer* secondaryCmdBuffers, uint32_t* cmdBufferCount) {
  uint32_t bundleCount = 0;
  TGFXLISTCOUNT(core_tgfx_main, commandBundleList, cmdListCount);
  for (uint32_t bundleListIndx = 0; bundleListIndx < cmdListCount; bundleListIndx++) {
    if (!commandBundleList[bundleListIndx]) {
      continue;
    }
    CMDBUNDLE_VKOBJ* bundle =
      hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(commandBundleList[bundleListIndx]);
    if (bundle && bundle->isALIVE && bundle->vk_cb && bundleCount < VKCONST_MAXCMDBUNDLE_PERCALL) {
      secondaryCmdBuffers[bundleCount++] = bundle->vk_cb;
    }
  }
  *cmdBufferCount = bundleCount;
}