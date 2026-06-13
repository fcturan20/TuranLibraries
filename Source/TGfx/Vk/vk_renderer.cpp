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
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::CMDBUNDLE;
  static uint16_t GET_EXTRAFLAGS(CMDBUNDLE_VKOBJ* obj) { return 0; }

  // Command Buffer States
  VkPipeline vk_activePipeline = {};
  // To check pipeline compatibility
  VkPipelineLayout      vk_activePipelineLayout                       = {};
  VkDescriptorSetLayout vk_activeDescSets[VKCONST_MAXDESCSET_PERLIST] = {};
  uint8_t               vk_callBuffer[128] = {}; // CallBuffer = Push Constants = 128 byte

  GPU_VKOBJ*            m_gpu                                          = {};
  vk_cmd*               m_cmds                                         = {};
  uint64_t              m_cmdCount                                     = 0;
  struct tgfx_pipeline* m_defaultPipeline                              = {};
  VkPipelineBindPoint   vk_bindPoint                                   = {};
  VkCommandBuffer       vk_cmdBuffers[VKCONST_MAXQUEUEFAMCOUNT_PERGPU] = {};
  cmdPool_vk*           vk_cmdPools[VKCONST_MAXQUEUEFAMCOUNT_PERGPU]   = {};

  void createCmdBuffer(uint64_t cmdCount);
};

#define vkEnumType_cmdType() uint32_t
enum class vk_cmdType : vkEnumType_cmdType(){
  error = VK_PRIM_MIN(vkEnumType_cmdType()), // cmdType neither should be 0 nor 255
  bindBindingTables,
  bindVertexBuffers,
  bindIndexBuffer,
  setDepthBounds,
  setViewport,
  setScissor,
  drawNonIndexedDirect,
  drawIndexedDirect,
  executeIndirect,
  barrierTexture,
  barrierBuffer,
  bindPipeline,
  dispatch,
  copyBufferToTexture,
  copyBufferToBuffer,
  pushConstant,
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

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &m_imBar);
  }

  // Command specific variables should have "m_" prefix
  VkImageMemoryBarrier m_imBar = {};
};

struct vkCmdStruct_bindBindingTables {
  static constexpr vk_cmdType cmd_type = vk_cmdType::bindBindingTables;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    VkDescriptorSet sets[VKCONST_MAXDESCSET_PERLIST] = {};
    for (uint32_t i = 0; i < m_setCount; i++) {
      BINDINGTABLEINST_VKOBJ* table   = getOBJ<BINDINGTABLEINST_VKOBJ>(tables[i]);
      sets[i]                         = table->vk_set;
      cmdBundle->vk_activeDescSets[i] = table->vk_layout;
    }
    vkCmdBindDescriptorSets(cb, vk_bindPoint, cmdBundle->vk_activePipelineLayout, m_firstSetIndx,
                            m_setCount, sets, 0, nullptr);
  }
  VkPipelineBindPoint       vk_bindPoint                       = VK_PIPELINE_BIND_POINT_MAX_ENUM;
  struct tgfx_bindingTable* tables[VKCONST_MAXDESCSET_PERLIST] = {};
  uint32_t                  m_setCount = 0, m_firstSetIndx = 0;
};

struct vkCmdStruct_bindPipeline {
  static constexpr vk_cmdType cmd_type = vk_cmdType::bindPipeline;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdBindPipeline(cb, vk_bindPoint, vk_pipeline);
    cmdBundle->vk_activePipeline       = vk_pipeline;
    cmdBundle->vk_activePipelineLayout = vk_pipelineLayout;
  }
  VkPipelineBindPoint vk_bindPoint      = VK_PIPELINE_BIND_POINT_MAX_ENUM;
  VkPipeline          vk_pipeline       = {};
  VkPipelineLayout    vk_pipelineLayout = {};
};

struct vkCmdStruct_dispatch {
  static constexpr vk_cmdType cmd_type = vk_cmdType::dispatch;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdDispatch(cb, m_dispatchSize.x, m_dispatchSize.y, m_dispatchSize.z);
  };
  tgfx_uvec3 m_dispatchSize;
};

struct vkCmdStruct_setViewport {
  static constexpr vk_cmdType cmd_type = vk_cmdType::setViewport;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdSetViewport(cb, 0, 1, &vk_viewport);
  };

  VkViewport vk_viewport = {};
};

struct vkCmdStruct_setScissor {
  static constexpr vk_cmdType cmd_type = vk_cmdType::setScissor;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdSetScissor(cb, 0, 1, &vk_rect);
  };
  VkRect2D vk_rect = {};
};

struct vkCmdStruct_drawNonIndexedDirect {
  static constexpr vk_cmdType cmd_type = vk_cmdType::drawNonIndexedDirect;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdDraw(cb, vertexCount, instanceCount, firstVertex, firstInstance);
  };
  uint32_t vertexCount = {}, instanceCount = {}, firstVertex = {}, firstInstance = {};
};

struct vkCmdStruct_drawIndexedDirect {
  static constexpr vk_cmdType cmd_type = vk_cmdType::drawIndexedDirect;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdDrawIndexed(cb, indxCount, instanceCount, firstIndx, vertexOffset, firstInstance);
  };
  uint32_t indxCount = {}, instanceCount = {}, firstIndx = {}, firstInstance = {};
  int32_t  vertexOffset = {};
};

struct vkCmdStruct_bindIndexBuffer {
  static constexpr vk_cmdType cmd_type = vk_cmdType::bindIndexBuffer;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdBindIndexBuffer(cb, vk_buffer, vk_offset, vk_indexType);
  };
  VkBuffer     vk_buffer;
  VkDeviceSize vk_offset;
  VkIndexType  vk_indexType;
};

struct vkCmdStruct_setDepthBounds {
  static constexpr vk_cmdType cmd_type = vk_cmdType::setDepthBounds;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdSetDepthBounds(cb, min, max);
  };
  float min = 0.0f, max = 1.0f;
};

struct vkCmdStruct_copyBufferToTexture {
  static constexpr vk_cmdType cmd_type = vk_cmdType::copyBufferToTexture;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdCopyBufferToImage(cb, vk_src, vk_dst, vk_dstImageLayout, 1, &vk_copy);
  };
  VkBuffer          vk_src;
  VkImage           vk_dst;
  VkImageLayout     vk_dstImageLayout;
  VkBufferImageCopy vk_copy;
};

struct vkCmdStruct_bindVertexBuffers {
  static constexpr vk_cmdType cmd_type = vk_cmdType::bindVertexBuffers;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdBindVertexBuffers(cb, firstBinding, bindingCount, vk_buffers, vk_bufferOffsets);
  };
  VkBuffer     vk_buffers[VKCONST_MAXVERTEXBINDINGCOUNT];
  VkDeviceSize vk_bufferOffsets[VKCONST_MAXVERTEXBINDINGCOUNT];
  uint32_t     firstBinding, bindingCount;
};

VkDeviceSize vk_findIndirectOperationDataSize(indirectOperationType_tgfx opType) {
  switch (opType) {
    case indirectOperationType_tgfx_DRAWNONINDEXED: return sizeof(VkDrawIndirectCommand);
    case indirectOperationType_tgfx_DRAWINDEXED: return sizeof(VkDrawIndexedIndirectCommand);
    case indirectOperationType_tgfx_DISPATCH: return sizeof(VkDispatchIndirectCommand);
  }
  return UINT64_MAX;
}
struct vkCmdStruct_executeIndirect {
  static constexpr vk_cmdType cmd_type = vk_cmdType::executeIndirect;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    VkDeviceSize activeOffset = vk_bufferOffset;
    for (uint32_t stateIndx = 0; stateIndx < opStateCount; stateIndx++) {
      uint64_t                   loopCount = 1, drawCount = opStates[stateIndx].opCount;
      indirectOperationType_tgfx opType = opStates[stateIndx].opType;
      uint64_t                   indirectArgumentDataSize =
        vk_findIndirectOperationDataSize(opStates[stateIndx].opType);
      // If GPU doesn't support multiDrawIndirect or execute type is compute, call same VkCmd*
      // multiple times with incrementing offsets
      if (!cmdBundle->m_gpu->vk_featuresDev.features.multiDrawIndirect ||
          opType == indirectOperationType_tgfx_DISPATCH) {
        loopCount = opStates[stateIndx].opCount;
        drawCount = 1;
      }
      for (uint32_t loopIndx = 0; loopIndx < loopCount; loopIndx++) {
        switch (opType) {
          case indirectOperationType_tgfx_DRAWNONINDEXED:
            vkCmdDrawIndirect(cb, vk_buffer, activeOffset, drawCount, indirectArgumentDataSize);
            break;
          case indirectOperationType_tgfx_DRAWINDEXED:
            vkCmdDrawIndexedIndirect(cb, vk_buffer, activeOffset, drawCount,
                                     indirectArgumentDataSize);
            break;
          case indirectOperationType_tgfx_DISPATCH:
            vkCmdDispatchIndirect(cb, vk_buffer, activeOffset);
            break;
          default: vkPrint(59); return;
        }
        activeOffset += indirectArgumentDataSize * drawCount;
      }
    }
  };
  void cmd_destroy() { vk_virmem::free_page(VK_POINTER_TO_MEMOFFSET(opStates)); }
  struct vk_indirectOperationState {
    uint32_t                   opCount;
    indirectOperationType_tgfx opType;
  };
  vk_indirectOperationState* opStates     = {};
  uint32_t                   opStateCount = 0;
  VkBuffer                   vk_buffer;
  VkDeviceSize               vk_bufferOffset;
};

struct vkCmdStruct_copyBufferToBuffer {
  static constexpr vk_cmdType cmd_type = vk_cmdType::copyBufferToBuffer;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdCopyBuffer(cb, vk_srcBuffer, vk_dstBuffer, 1, &vk_bufCopy);
  };

  VkBuffer     vk_srcBuffer = {}, vk_dstBuffer = {};
  VkBufferCopy vk_bufCopy = {};
};

struct vkCmdStruct_pushConstant {
  static constexpr vk_cmdType cmd_type = vk_cmdType::pushConstant;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdPushConstants(cb, cmdBundle->vk_activePipelineLayout, VK_SHADER_STAGE_ALL, offset, size,
                       data);
  };

  unsigned char offset, size, data[128];
};

struct vk_cmd {
  vk_cmdType cmd_type = vk_cmdType::error_2;

  // From https://stackoverflow.com/a/46408751
  template <typename... T>
  static constexpr size_t max_sizeof() {
    return std::max({sizeof(T)...});
  }

#define vkCmdStructsLists                                                          \
  vkCmdStruct_example, vkCmdStruct_barrierTexture, vkCmdStruct_bindBindingTables,  \
    vkCmdStruct_bindPipeline, vkCmdStruct_dispatch, vkCmdStruct_bindVertexBuffers, \
    vkCmdStruct_executeIndirect, vkCmdStruct_copyBufferToTexture, vkCmdStruct_pushConstant
  static constexpr uint32_t maxCmdStructSize           = max_sizeof<vkCmdStructsLists>();
  uint8_t                   cmd_data[maxCmdStructSize] = {};
  vk_cmd() : cmd_type(vk_cmdType::error) {}
};

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

void vk_executeCmd(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* bundle, const vk_cmd& cmd) {
  switch (cmd.cmd_type) {
    case vk_cmdType::barrierTexture:
      (( vkCmdStruct_barrierTexture* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::bindBindingTables:
      (( vkCmdStruct_bindBindingTables* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::bindPipeline:
      (( vkCmdStruct_bindPipeline* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::dispatch:
      (( vkCmdStruct_dispatch* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::setScissor:
      (( vkCmdStruct_setScissor* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::setViewport:
      (( vkCmdStruct_setViewport* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::drawNonIndexedDirect:
      (( vkCmdStruct_drawNonIndexedDirect* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::setDepthBounds:
      (( vkCmdStruct_setDepthBounds* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::copyBufferToTexture:
      (( vkCmdStruct_copyBufferToTexture* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::bindVertexBuffers:
      (( vkCmdStruct_bindVertexBuffers* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::bindIndexBuffer:
      (( vkCmdStruct_bindIndexBuffer* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::drawIndexedDirect:
      (( vkCmdStruct_drawIndexedDirect* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::executeIndirect:
      (( vkCmdStruct_executeIndirect* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::copyBufferToBuffer:
      (( vkCmdStruct_copyBufferToBuffer* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::pushConstant:
      (( vkCmdStruct_pushConstant* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::error:
    case vk_cmdType::error_2: vkPrint(60, L"one of the commands in the buffer is not used"); break;
    default: vkPrint(16, L"invalid command type in vk_executeCmd()");
  }
}
void vk_destroyCmd(vk_cmd& cmd) {
  switch (cmd.cmd_type) {
    case vk_cmdType::executeIndirect:
      (( vkCmdStruct_executeIndirect* )cmd.cmd_data)->cmd_destroy();
      break;
  }
}

void CMDBUNDLE_VKOBJ::createCmdBuffer(uint64_t cmdCount) {
  uint32_t allocSize = sizeof(vk_cmd) * cmdCount;
  m_cmds             = ( vk_cmd* )VK_MEMOFFSET_TO_POINTER(vk_virmem::allocatePage(allocSize));
  vm->commit(m_cmds, allocSize);
  m_cmdCount = cmdCount;
  for (uint32_t i = 0; i < cmdCount; i++) {
    m_cmds[i] = {};
  }
}

struct vk_renderer_private {
  VK_LINEAR_OBJARRAY<CMDBUNDLE_VKOBJ, struct tgfx_commandBundle*, 1 << 20> m_cmdBundles;
};
vk_renderer_private* hiddenRenderer = nullptr;

// Synchronization Functions

void vk_createFences(struct tgfx_gpu* gpu, unsigned int count, unsigned int initValue,
                     struct tgfx_fence** fenceList) {
  GPU_VKOBJ* GPU = getOBJ<GPU_VKOBJ>(gpu);
  for (uint32_t i = 0; i < count; i++) {
    fenceList[i] = vk_createTGFXFence(GPU, initValue);
  }
}
void vk_destroyFence(struct tgfx_fence* fence) {
  FENCE_VKOBJ* vkFence = getOBJ<FENCE_VKOBJ>(fence);
  GPU_VKOBJ*   gpu     = core_vk->getGPU(vkFence->m_gpuIndx);
  vkDestroySemaphore(gpu->vk_logical, vkFence->vk_timelineSemaphore, nullptr);
  vkext_timelineSemaphore* ext =
    ( vkext_timelineSemaphore* )gpu->ext()->m_exts[vkext_interface::timelineSemaphores_vkExtEnum];
  ext->fences.destroyObj(ext->fences.getINDEXbyOBJ(vkFence));
}

// Command Bundle Functions
////////////////////////////

struct tgfx_commandBundle* vk_beginCommandBundle(struct tgfx_gpu*              gpu,
                                                 unsigned long long            maxCmdCount,
                                                 struct tgfx_pipeline*         defaultPipeline,
                                                 unsigned int                  extCount,
                                                 struct tgfx_extension* const* exts) {
  VkCommandBuffer vk_cmdBuffer = VK_NULL_HANDLE;
  cmdPool_vk*     cmdPool;
  GPU_VKOBJ*      GPU = getOBJ<GPU_VKOBJ>(gpu);
  if (!GPU) {
    return nullptr;
  }
  CMDBUNDLE_VKOBJ* cmdBundle = hiddenRenderer->m_cmdBundles.create_OBJ();
  for (uint32_t i = 0; i < VKCONST_MAXQUEUEFAMCOUNT_PERGPU; i++) {
    cmdBundle->vk_cmdBuffers[i] = {};
  }
  cmdBundle->m_gpu = GPU;
  cmdBundle->createCmdBuffer(maxCmdCount);
  cmdBundle->m_defaultPipeline = defaultPipeline;
  if (defaultPipeline) {
    PIPELINE_VKOBJ* pipe    = getOBJ<PIPELINE_VKOBJ>(defaultPipeline);
    cmdBundle->vk_bindPoint = pipe->vk_type;
  } else {
    cmdBundle->vk_bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
  }

  return getHANDLE<struct tgfx_commandBundle*>(cmdBundle);
}
void vk_finishCommandBundle(struct tgfx_commandBundle* bndl, unsigned int extCount,
                            struct tgfx_extension* const* exts) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);

  // MOVE THIS RECORDING STAGE BECAUSE FRAMEBUFFER WILL RECORD ALREADY!
}
void vk_destroyCommandBundle(struct tgfx_commandBundle* hnd) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(hnd);

  for (uint32_t i = 0; i < bundle->m_cmdCount; i++) {
    vk_destroyCmd(bundle->m_cmds[i]);
  }
  vk_virmem::free_page(VK_POINTER_TO_MEMOFFSET(bundle->m_cmds));
  const uint32_t cmdBufferCount = sizeof(bundle->vk_cmdBuffers) / sizeof(bundle->vk_cmdBuffers[0]);
  for (uint32_t i = 0; i < cmdBufferCount; i++) {
    VkCommandBuffer cmdBuffer = bundle->vk_cmdBuffers[i];
    if (cmdBuffer != VK_NULL_HANDLE) {
      vk_freeCmdBuffer(bundle->vk_cmdPools[i], cmdBuffer);
    }
  }
}

void vk_cmdBindBindingTables(struct tgfx_commandBundle* bndl, unsigned long long sortKey,
                             unsigned int firstSetIndx, unsigned int bindingTableCount,
                             struct tgfx_bindingTable* const* bindingTables,
                             pipelineType_tgfx                pipelineType) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto* cmd = vk_createCmdStruct<vkCmdStruct_bindBindingTables>(&bundle->m_cmds[sortKey]);

  {
    uint32_t descSetLimit =
      glm::min(VKCONST_MAXDESCSET_PERLIST,
               bundle->m_gpu->vk_propsDev.properties.limits.maxBoundDescriptorSets);
    if (bindingTableCount > descSetLimit) {
      vkPrint(22, L"Max binding table count is exceeded!");
      return;
    }
    for (uint32_t i = 0; i < bindingTableCount; i++) {
      BINDINGTABLEINST_VKOBJ* bindingTable = getOBJ<BINDINGTABLEINST_VKOBJ>(bindingTables[i]);
      assert(bindingTable && "Binding table isn't found!");
      cmd->tables[cmd->m_setCount++] = bindingTables[i];
    }
  }

  cmd->vk_bindPoint   = vk_findPipelineBindPoint(pipelineType);
  cmd->m_firstSetIndx = firstSetIndx;
}
void vk_cmdBindPipeline(struct tgfx_commandBundle* bndl, unsigned long long sortKey,
                        struct tgfx_pipeline* pipeline) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto*            cmd    = vk_createCmdStruct<vkCmdStruct_bindPipeline>(&bundle->m_cmds[sortKey]);
  PIPELINE_VKOBJ*  pipe   = getOBJ<PIPELINE_VKOBJ>(pipeline);

  if (pipe->vk_type != bundle->vk_bindPoint) {
    vkPrint(61);
  }
  cmd->vk_bindPoint      = pipe->vk_type;
  cmd->vk_pipeline       = pipe->vk_object;
  cmd->vk_pipelineLayout = pipe->vk_layout;
}
void vk_cmdSetViewport(struct tgfx_commandBundle* bndl, unsigned long long sortKey,
                       const tgfx_viewportInfo* viewport) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto*            cmd    = vk_createCmdStruct<vkCmdStruct_setViewport>(&bundle->m_cmds[sortKey]);

  cmd->vk_viewport.x        = viewport->topLeftCorner.x;
  cmd->vk_viewport.y        = viewport->topLeftCorner.y;
  cmd->vk_viewport.width    = viewport->size.x;
  cmd->vk_viewport.height   = viewport->size.y;
  cmd->vk_viewport.minDepth = viewport->depthMinMax.x;
  cmd->vk_viewport.maxDepth = viewport->depthMinMax.y;
}
void vk_cmdSetScissor(struct tgfx_commandBundle* bndl, unsigned long long sortKey,
                      const tgfx_ivec2* offset, const tgfx_uvec2* size) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto*            cmd    = vk_createCmdStruct<vkCmdStruct_setScissor>(&bundle->m_cmds[sortKey]);

  cmd->vk_rect.offset.x      = offset->x;
  cmd->vk_rect.offset.y      = offset->x;
  cmd->vk_rect.extent.width  = size->x;
  cmd->vk_rect.extent.height = size->y;
}
void vk_cmdSetDepthBounds(struct tgfx_commandBundle* bndl, unsigned long long sortKey, float min,
                          float max) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto*            cmd = vk_createCmdStruct<vkCmdStruct_setDepthBounds>(&bundle->m_cmds[sortKey]);

  cmd->min = min;
  cmd->max = max;
};
void vk_cmdBindVertexBuffers(struct tgfx_commandBundle* bndl, unsigned long long sortKey,
                             unsigned int firstBinding, unsigned int bindingCount,
                             struct tgfx_buffer* const* buffers,
                             const unsigned long long*  offsets) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto* cmd = vk_createCmdStruct<vkCmdStruct_bindVertexBuffers>(&bundle->m_cmds[sortKey]);

  cmd->firstBinding = firstBinding;
  cmd->bindingCount = bindingCount;
  for (uint32_t i = 0; i < bindingCount; i++) {
    cmd->vk_bufferOffsets[i] = offsets[i];
    cmd->vk_buffers[i]       = getOBJ<BUFFER_VKOBJ>(buffers[i])->vk_buffer;
  }
}
void vk_cmdBindIndexBuffer(struct tgfx_commandBundle* bndl, unsigned long long sortKey,
                           struct tgfx_buffer* buffer, unsigned long long offset,
                           unsigned char IndexTypeSize) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto*            cmd = vk_createCmdStruct<vkCmdStruct_bindIndexBuffer>(&bundle->m_cmds[sortKey]);

  cmd->vk_buffer = getOBJ<BUFFER_VKOBJ>(buffer)->vk_buffer;
  switch (IndexTypeSize) {
    case 1: cmd->vk_indexType = VK_INDEX_TYPE_UINT8_EXT;
    case 2: cmd->vk_indexType = VK_INDEX_TYPE_UINT16; break;
    case 4: cmd->vk_indexType = VK_INDEX_TYPE_UINT32; break;
  }
  cmd->vk_offset = offset;
}
void vk_cmdDrawNonIndexedDirect(struct tgfx_commandBundle* bndl, unsigned long long sortKey,
                                unsigned int vertexCount, unsigned int instanceCount,
                                unsigned int firstVertex, unsigned int firstInstance) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto* cmd = vk_createCmdStruct<vkCmdStruct_drawNonIndexedDirect>(&bundle->m_cmds[sortKey]);

  cmd->firstInstance = firstInstance;
  cmd->firstVertex   = firstVertex;
  cmd->vertexCount   = vertexCount;
  cmd->instanceCount = instanceCount;
}
void vk_cmdDrawIndexedDirect(struct tgfx_commandBundle* bndl, unsigned long long sortKey,
                             unsigned int indexCount, unsigned int instanceCount,
                             unsigned int firstIndex, int vertexOffset,
                             unsigned int firstInstance) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto* cmd = vk_createCmdStruct<vkCmdStruct_drawIndexedDirect>(&bundle->m_cmds[sortKey]);

  cmd->firstIndx     = firstIndex;
  cmd->firstInstance = firstInstance;
  cmd->indxCount     = indexCount;
  cmd->instanceCount = instanceCount;
  cmd->vertexOffset  = vertexOffset;
}
void vk_cmdExecuteIndirect(struct tgfx_commandBundle* bndl, unsigned long long sortKey,
                           unsigned int                      operationCount,
                           const indirectOperationType_tgfx* operationTypes,
                           struct tgfx_buffer* dataBffr, unsigned long long indirectBufferOffset,
                           unsigned int extCount, struct tgfx_extension* const* exts) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto*            cmd = vk_createCmdStruct<vkCmdStruct_executeIndirect>(&bundle->m_cmds[sortKey]);

  // Find operation state count
  for (uint32_t i = 0; i < operationCount;) {
    indirectOperationType_tgfx opType = operationTypes[i];
    for (; i < operationCount && opType == operationTypes[i]; i++) {
    }
    cmd->opStateCount++;
  }
  uint32_t allocSize =
    sizeof(vkCmdStruct_executeIndirect::vk_indirectOperationState) * cmd->opStateCount;
  cmd->opStates =
    ( vkCmdStruct_executeIndirect::vk_indirectOperationState* )VK_MEMOFFSET_TO_POINTER(
      vk_virmem::allocatePage(allocSize, &allocSize));
  vm->commit(cmd->opStates, allocSize);
  for (uint32_t i = 0, stateIndx = 0; i < operationCount;) {
    indirectOperationType_tgfx opType = operationTypes[i];
    cmd->opStates[stateIndx].opType   = opType;
    cmd->opStates[stateIndx].opCount  = 0;
    for (; i < operationCount && opType == operationTypes[i];
         i++, cmd->opStates[stateIndx].opCount++) {
    }
    stateIndx++;
  }
  cmd->vk_buffer       = getOBJ<BUFFER_VKOBJ>(dataBffr)->vk_buffer;
  cmd->vk_bufferOffset = indirectBufferOffset;
}
void vk_cmdBarrierTexture(struct tgfx_commandBundle* bndl, unsigned long long key,
                          struct tgfx_texture* i_texture, image_access_tgfx lastAccess,
                          image_access_tgfx nextAccess, textureUsageMask_tgfxflag lastUsage,
                          textureUsageMask_tgfxflag nextUsage, unsigned int extCount,
                          struct tgfx_extension* const* exts) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto*            cmdBar = vk_createCmdStruct<vkCmdStruct_barrierTexture>(&bundle->m_cmds[key]);
  cmdBar->m_imBar.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  cmdBar->m_imBar.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  cmdBar->m_imBar.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  cmdBar->m_imBar.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  cmdBar->m_imBar.subresourceRange.baseArrayLayer = 0;
  cmdBar->m_imBar.subresourceRange.baseMipLevel   = 0;
  cmdBar->m_imBar.subresourceRange.layerCount     = 1;
  cmdBar->m_imBar.subresourceRange.levelCount     = 1;
  TEXTURE_VKOBJ* texture                          = getOBJ<TEXTURE_VKOBJ>(i_texture);
  cmdBar->m_imBar.image                           = texture->vk_image;
  vk_findImageAccessPattern(lastAccess, cmdBar->m_imBar.srcAccessMask, cmdBar->m_imBar.oldLayout);
  vk_findImageAccessPattern(nextAccess, cmdBar->m_imBar.dstAccessMask, cmdBar->m_imBar.newLayout);
  cmdBar->m_imBar.pNext = nullptr;
}
void vk_cmdDispatch(struct tgfx_commandBundle* bndl, unsigned long long key,
                    const tgfx_uvec3* dispatchSize) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto*            cmd    = vk_createCmdStruct<vkCmdStruct_dispatch>(&bundle->m_cmds[key]);

  cmd->m_dispatchSize = *dispatchSize;
}
void vk_cmdCopyBufferToTexture(struct tgfx_commandBundle* bndl, unsigned long long key,
                               struct tgfx_buffer* srcBuffer, unsigned long long bufferOffset,
                               struct tgfx_texture* dstTexture, image_access_tgfx lastAccess,
                               unsigned int extCount, struct tgfx_extension* const* exts) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto*            cmd = vk_createCmdStruct<vkCmdStruct_copyBufferToTexture>(&bundle->m_cmds[key]);

  BUFFER_VKOBJ*  buffer  = getOBJ<BUFFER_VKOBJ>(srcBuffer);
  TEXTURE_VKOBJ* texture = getOBJ<TEXTURE_VKOBJ>(dstTexture);

  cmd->vk_src = buffer->vk_buffer;
  cmd->vk_dst = texture->vk_image;
  VkAccessFlags flag;
  vk_findImageAccessPattern(lastAccess, flag, cmd->vk_dstImageLayout);
  cmd->vk_copy.imageOffset        = {};
  cmd->vk_copy.imageExtent.width  = texture->m_width;
  cmd->vk_copy.imageExtent.height = texture->m_height;
  cmd->vk_copy.imageExtent.depth  = 1;
  cmd->vk_copy.bufferImageHeight  = 0;
  cmd->vk_copy.bufferOffset       = bufferOffset;
  cmd->vk_copy.bufferRowLength    = 0;
  if (texture->m_channels == texture_channels_tgfx_D32) {
    cmd->vk_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  } else if (texture->m_channels == texture_channels_tgfx_D24S8) {
    cmd->vk_copy.imageSubresource.aspectMask =
      VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  } else {
    cmd->vk_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }
  cmd->vk_copy.imageSubresource.baseArrayLayer = 0;
  cmd->vk_copy.imageSubresource.layerCount     = 1;
  cmd->vk_copy.imageSubresource.mipLevel       = 0;
}

void vk_cmdCopyBufferToBuffer(struct tgfx_commandBundle* bndl, unsigned long long key,
                              unsigned long long size, struct tgfx_buffer* srcBuffer,
                              unsigned long long srcOffset, struct tgfx_buffer* dstBuffer,
                              unsigned long long dstOffset) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto*            cmd = vk_createCmdStruct<vkCmdStruct_copyBufferToBuffer>(&bundle->m_cmds[key]);

  cmd->vk_bufCopy.dstOffset = dstOffset;
  cmd->vk_bufCopy.size      = size;
  cmd->vk_bufCopy.srcOffset = srcOffset;
  cmd->vk_dstBuffer         = getOBJ<BUFFER_VKOBJ>(dstBuffer)->vk_buffer;
  cmd->vk_srcBuffer         = getOBJ<BUFFER_VKOBJ>(srcBuffer)->vk_buffer;
}
void vk_cmdPushConstant(struct tgfx_commandBundle* bndl, unsigned long long key,
                        unsigned char offset, unsigned char size, const void* d) {
  CMDBUNDLE_VKOBJ* bundle = getOBJ<CMDBUNDLE_VKOBJ>(bndl);
  auto*            cmd    = vk_createCmdStruct<vkCmdStruct_pushConstant>(&bundle->m_cmds[key]);
  size                    = std::min(128u, uint32_t(size));
  cmd->size               = size;
  cmd->offset             = offset;
  memcpy(cmd->data, d, size);
}

void vk_getSecondaryCmdBuffers(unsigned int                      cmdBundleCount,
                               struct tgfx_commandBundle* const* cmdBundles, uint32_t queueFamIndx,
                               VkCommandBuffer* secondaryCmdBuffers) {
  uint32_t bundleCount = 0;
  for (uint32_t bundleListIndx = 0; bundleListIndx < cmdBundleCount; bundleListIndx++) {
    if (!cmdBundles[bundleListIndx]) {
      continue;
    }
    const struct tgfx_commandBundle* bundleHnd = cmdBundles[bundleListIndx];
    CMDBUNDLE_VKOBJ*                 bundle    = getOBJ<CMDBUNDLE_VKOBJ>(bundleHnd);

    if (!bundle || bundleCount >= VKCONST_MAXCMDBUNDLE_PERCALL) {
      continue;
    }

    VkCommandBuffer& vkCmdBuffer = bundle->vk_cmdBuffers[queueFamIndx];

    // Command bundle isn't used in this queue fam, so use it
    if (vkCmdBuffer == VK_NULL_HANDLE) {
      vk_allocateCmdBuffer(getQueueFam(bundle->m_gpu, queueFamIndx),
                           VK_COMMAND_BUFFER_LEVEL_SECONDARY, bundle->vk_cmdPools[queueFamIndx],
                           &vkCmdBuffer, 1);

      VkCommandBufferInheritanceRenderingInfo rInfo = {};
      VkCommandBufferBeginInfo                bi    = {};
      bi.flags                                      = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
      VkCommandBufferInheritanceInfo secInfo        = {};
      secInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
      if (bundle->vk_bindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS) {
        rInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
        rInfo.viewMask                = 0;
        PIPELINE_VKOBJ* defaultPipe   = getOBJ<PIPELINE_VKOBJ>(bundle->m_defaultPipeline);
        rInfo.pColorAttachmentFormats = defaultPipe->vk_colorAttachmentFormats;
        while (rInfo.colorAttachmentCount < TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT &&
               rInfo.pColorAttachmentFormats[rInfo.colorAttachmentCount] != VK_FORMAT_UNDEFINED) {
          rInfo.colorAttachmentCount++;
        }
        rInfo.depthAttachmentFormat = defaultPipe->vk_depthAttachmentFormat;
        rInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
        secInfo.pNext               = &rInfo;
        bi.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
      }

      bi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      bi.pInheritanceInfo = &secInfo;
      if (vkBeginCommandBuffer(vkCmdBuffer, &bi) != VK_SUCCESS) {
        vkPrint(16, L"at vkBeginCommandBuffer()");
        return;
      }
      if (bundle->m_defaultPipeline) {
        PIPELINE_VKOBJ* pipe = getOBJ<PIPELINE_VKOBJ>(bundle->m_defaultPipeline);
        vkCmdBindPipeline(vkCmdBuffer, pipe->vk_type, pipe->vk_object);
        bundle->vk_activePipeline       = pipe->vk_object;
        bundle->vk_activePipelineLayout = pipe->vk_layout;
      }
      for (uint64_t cmdIndx = 0; cmdIndx < bundle->m_cmdCount; cmdIndx++) {
        vk_executeCmd(vkCmdBuffer, bundle, bundle->m_cmds[cmdIndx]);
      }
      if (vkEndCommandBuffer(vkCmdBuffer) != VK_SUCCESS) {
        vkPrint(16, L"at vkEndCommandBuffer()");
      }
    }

    secondaryCmdBuffers[bundleCount++] = vkCmdBuffer;
  }
}

void set_VkRenderer_funcPtrs() {
  core_tgfx_main->renderer->beginCommandBundle   = vk_beginCommandBundle;
  core_tgfx_main->renderer->finishCommandBundle  = vk_finishCommandBundle;
  core_tgfx_main->renderer->createFences         = vk_createFences;
  core_tgfx_main->renderer->destroyCommandBundle = vk_destroyCommandBundle;
  core_tgfx_main->renderer->getFenceValue        = vk_getFenceValue;
  core_tgfx_main->renderer->setFence             = vk_setFenceValue;
  core_tgfx_main->renderer->destroyFence         = vk_destroyFence;

  core_tgfx_main->renderer->cmdBindBindingTables    = vk_cmdBindBindingTables;
  core_tgfx_main->renderer->cmdBindIndexBuffer      = vk_cmdBindIndexBuffer;
  core_tgfx_main->renderer->cmdBindVertexBuffers    = vk_cmdBindVertexBuffers;
  core_tgfx_main->renderer->cmdDrawIndexedDirect    = vk_cmdDrawIndexedDirect;
  core_tgfx_main->renderer->cmdExecuteIndirect      = vk_cmdExecuteIndirect;
  core_tgfx_main->renderer->cmdDrawNonIndexedDirect = vk_cmdDrawNonIndexedDirect;
  core_tgfx_main->renderer->cmdBarrierTexture       = vk_cmdBarrierTexture;
  core_tgfx_main->renderer->cmdBindPipeline         = vk_cmdBindPipeline;
  core_tgfx_main->renderer->cmdDispatch             = vk_cmdDispatch;
  core_tgfx_main->renderer->cmdSetViewport          = vk_cmdSetViewport;
  core_tgfx_main->renderer->cmdSetScissor           = vk_cmdSetScissor;
  core_tgfx_main->renderer->cmdSetDepthBounds       = vk_cmdSetDepthBounds;
  core_tgfx_main->renderer->cmdCopyBufferToTexture  = vk_cmdCopyBufferToTexture;
  core_tgfx_main->renderer->cmdCopyBufferToBuffer   = vk_cmdCopyBufferToBuffer;
  core_tgfx_main->renderer->cmdPushConstant         = vk_cmdPushConstant;
}

void vk_initRenderer() {
  set_VkRenderer_funcPtrs();
  VKGLOBAL_VIRMEM_RENDERER = vk_virmem::allocate_dynamicmem(sizeof(vk_renderer_private));
  hiddenRenderer           = new (VKGLOBAL_VIRMEM_RENDERER) vk_renderer_private;
}
