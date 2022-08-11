#include "vk_predefinitions.h"
#include "vk_renderer.h"
#include "tgfx_renderer.h"

struct vk_commandBuffer {
  //Pointer to allocated space for storing render calls below
  unsigned int DATAPTR = 0;
  tgfx_rendererKeySortFunc sortFunc;
  VkCommandBuffer CB;
};

struct vk_renderer_private {

};

commandbuffer_tgfx_handle vk_getCommandBuffer(gpu_tgfxhnd gpu, extension_tgfxlsthnd exts) {
  return nullptr;
}
void vk_executeBundles(commandbuffer_tgfx_handle commandBuffer, commandBundle_tgfxhnd* bundles, const unsigned long long* bundleSortKeys, tgfx_rendererKeySortFunc sortFunc, extension_tgfxlsthnd exts) {

}
void vk_start_renderpass(commandbuffer_tgfx_handle commandBuffer, renderPass_tgfxhnd renderPass) {

}
void vk_next_rendersubpass(commandbuffer_tgfx_handle commandBuffer, renderSubPass_tgfxhnd renderSubPass) {

}
void vk_end_renderpass(commandbuffer_tgfx_handle commandBuffer) {

}
void vk_submitQueue(gpuQueue_tgfxhnd queue, commandbuffer_tgfxlsthnd commandBuffersList, fence_tgfxlsthnd fenceList, semaphore_tgfxlsthnd waitSemaphoreList, semaphore_tgfxlsthnd signalSemaphoreList, extension_tgfxlsthnd exts) {

}

// Synchronization Functions

void vk_createFences(gpu_tgfxhnd gpu, unsigned int fenceCount, const unsigned char* isSignaled, fence_tgfxlsthnd fenceList) {

}
void vk_createSemaphores(gpu_tgfxhnd gpu, unsigned int semaphoreCount, semaphore_tgfxlsthnd semaphoreList) {

}

// Command Bundle Functions
////////////////////////////

commandBundle_tgfxhnd vk_createCommandBundle(renderSubPass_tgfxhnd subpassHandle, tgfx_rendererKeySortFunc sortFunc) {
  return nullptr;
}
void vk_cmdBindBindingTable(commandBundle_tgfxhnd bundle, unsigned long long sortKey, bindingTable_tgfxhnd bindingtable) {

}
void vk_cmdBindVertexBuffer(commandBundle_tgfxhnd bundle, unsigned long long sortKey, buffer_tgfxhnd buffer, unsigned long long offset, unsigned long long boundSize) {

}
void vk_cmdBindIndexBuffers(commandBundle_tgfxhnd bundle, unsigned long long sortKey, buffer_tgfxhnd buffer, unsigned long long offset, unsigned char indexDataTypeSize) {

}
void vk_cmdDrawNonIndexedDirect(commandBundle_tgfxhnd bundle, unsigned long long sortKey, unsigned int vertexCount, unsigned int instanceCount, unsigned int firstVertex, unsigned int firstInstance) {

}
void vk_cmdDrawIndexedDirect(commandBundle_tgfxhnd bundle, unsigned long long sortKey, unsigned int indexCount, unsigned int instanceCount, unsigned int firstIndex, int vertexOffset, unsigned int firstInstance) {

}
void vk_cmdDrawNonIndexedIndirect(commandBundle_tgfxhnd, unsigned long long sortKey,
  buffer_tgfxhnd drawDataBuffer, unsigned long long drawDataBufferOffset, buffer_tgfxhnd drawCountBuffer, unsigned long long drawCountBufferOffset, extension_tgfxlsthnd exts) {

}
void vk_cmdDrawIndexedIndirect(commandBundle_tgfxhnd, unsigned long long sortKey, buffer_tgfxhnd drawCountBuffer, unsigned long long drawCountBufferOffset, extension_tgfxlsthnd exts) {

}
void vk_destroyCommandBundle(commandBundle_tgfxhnd hnd) {

}
void renderer_public::RendererResource_Finalizations() {
  /*
  // Handle RTSlotSet changes by recreating framebuffers of draw passes
  //  by "RTSlotSet changes": DrawPass' slotset is changed to different one or slotset's slots is changed.
  RenderGraph::DP_VK* DP = (RenderGraph::DP_VK*)VKGLOBAL_RG->GetNextPass(nullptr, VK_PASSTYPE::DP);
  while (DP) {
    unsigned char ChangeInfo = DP->SlotSetChanged.load();
    RTSLOTSET_VKOBJ* slotset = contentmanager->GETRTSLOTSET_ARRAY().getOBJbyINDEX(DP->BASESLOTSET_ID);
    if (ChangeInfo == VKGLOBAL_FRAMEINDEX + 1 || ChangeInfo == 3 || slotset->PERFRAME_SLOTSETs[VKGLOBAL_FRAMEINDEX].IsChanged.load()) {
      if (DP->FBs[VKGLOBAL_FRAMEINDEX]) {
        //This is safe because this FB is used 2 frames ago and CPU already waits for the frame's GPU process to end
        vkDestroyFramebuffer(rendergpu->LOGICALDEVICE(), DP->FBs[VKGLOBAL_FRAMEINDEX], nullptr);
      }

      VkFramebufferCreateInfo fb_ci = slotset->FB_ci[VKGLOBAL_FRAMEINDEX];
      fb_ci.renderPass = DP->RenderPassObject;
      if (vkCreateFramebuffer(rendergpu->LOGICALDEVICE(), &fb_ci, nullptr, &DP->FBs[VKGLOBAL_FRAMEINDEX]) != VK_SUCCESS) {
        printer(result_tgfx_FAIL, "vkCreateFramebuffer() has failed while changing one of the drawpasses' current frame slot's texture! Please report this!");
        return;
      }

      slotset->PERFRAME_SLOTSETs[VKGLOBAL_FRAMEINDEX].IsChanged.store(false);
      for (unsigned int SlotIndex = 0; SlotIndex < slotset->PERFRAME_SLOTSETs[VKGLOBAL_FRAMEINDEX].COLORSLOTs_COUNT; SlotIndex++) {
        slotset->PERFRAME_SLOTSETs[VKGLOBAL_FRAMEINDEX].COLOR_SLOTs[SlotIndex].IsChanged.store(false);
      }

      if (ChangeInfo) {
        DP->SlotSetChanged.store(ChangeInfo - VKGLOBAL_FRAMEINDEX - 1);
      }
    }
  }*/
}
RGReconstructionStatus renderer_public::RGSTATUS() {
  return RGReconstructionStatus::Valid;
}

void set_VkRenderer_funcPtrs() {
  core_tgfx_main->renderer->cmdBindBindingTable = vk_cmdBindBindingTable;
  core_tgfx_main->renderer->cmdBindIndexBuffers = vk_cmdBindIndexBuffers;
  core_tgfx_main->renderer->cmdBindVertexBuffer = vk_cmdBindVertexBuffer;
  core_tgfx_main->renderer->cmdDrawIndexedDirect = vk_cmdDrawIndexedDirect;
  core_tgfx_main->renderer->cmdDrawIndexedIndirect = vk_cmdDrawIndexedIndirect;
  core_tgfx_main->renderer->cmdDrawNonIndexedDirect = vk_cmdDrawNonIndexedDirect;
  core_tgfx_main->renderer->cmdDrawNonIndexedIndirect = vk_cmdDrawNonIndexedIndirect;
  core_tgfx_main->renderer->createCommandBundle = vk_createCommandBundle;
  core_tgfx_main->renderer->createFences = vk_createFences;
  core_tgfx_main->renderer->createSemaphores = vk_createSemaphores;
  core_tgfx_main->renderer->destroyCommandBundle = vk_destroyCommandBundle;
  core_tgfx_main->renderer->end_renderpass = vk_end_renderpass;
  core_tgfx_main->renderer->executeBundles = vk_executeBundles;
  core_tgfx_main->renderer->getCommandBuffer = vk_getCommandBuffer;
  core_tgfx_main->renderer->next_rendersubpass = vk_next_rendersubpass;
  core_tgfx_main->renderer->start_renderpass = vk_start_renderpass;
  core_tgfx_main->renderer->submitQueue = vk_submitQueue;
}

void Create_VkRenderer() {
  set_VkRenderer_funcPtrs();
}