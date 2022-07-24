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

commandbuffer_tgfx_handle vk_getCommandBuffer(gpu_tgfx_handle gpu, extension_tgfx_listhandle exts) {

}
void vk_executeBundles(commandbuffer_tgfx_handle commandBuffer, commandbundle_tgfx_handle* bundles, const unsigned long long* bundleSortKeys, tgfx_rendererKeySortFunc sortFunc, extension_tgfx_listhandle exts) {

}
void vk_start_renderpass(commandbuffer_tgfx_handle commandBuffer, renderpass_tgfx_handle renderPass) {

}
void vk_next_rendersubpass(commandbuffer_tgfx_handle commandBuffer, rendersubpass_tgfx_handle renderSubPass) {

}
void vk_end_renderpass(commandbuffer_tgfx_handle commandBuffer) {

}
void vk_submitQueue(gpuqueue_tgfx_handle queue, commandbuffer_tgfx_listhandle commandBuffersList, fence_tgfx_listhandle fenceList, semaphore_tgfx_listhandle waitSemaphoreList, semaphore_tgfx_listhandle signalSemaphoreList, extension_tgfx_listhandle exts) {

}

// Synchronization Functions

void vk_createFences(gpu_tgfx_handle gpu, unsigned int fenceCount, const unsigned char* isSignaled, fence_tgfx_listhandle fenceList) {

}
void vk_createSemaphores(gpu_tgfx_handle gpu, unsigned int semaphoreCount, semaphore_tgfx_listhandle semaphoreList) {

}

// Command Bundle Functions
////////////////////////////

commandbundle_tgfx_handle vk_createCommandBundle(rendersubpass_tgfx_handle subpassHandle, tgfx_rendererKeySortFunc sortFunc) {

}
void vk_cmdBindBindingTable(commandbundle_tgfx_handle bundle, unsigned long long sortKey, bindingtable_tgfx_handle bindingtable) {

}
void vk_cmdBindVertexBuffer(commandbundle_tgfx_handle bundle, unsigned long long sortKey, buffer_tgfx_handle buffer, unsigned long long offset, unsigned long long boundSize) {

}
void vk_cmdBindIndexBuffers(commandbundle_tgfx_handle bundle, unsigned long long sortKey, buffer_tgfx_handle buffer, unsigned long long offset, unsigned char indexDataTypeSize) {

}
void vk_cmdDrawNonIndexedDirect(commandbundle_tgfx_handle bundle, unsigned long long sortKey, unsigned int vertexCount, unsigned int instanceCount, unsigned int firstVertex, unsigned int firstInstance) {

}
void vk_cmdDrawIndexedDirect(commandbundle_tgfx_handle bundle, unsigned long long sortKey, unsigned int indexCount, unsigned int instanceCount, unsigned int firstIndex, int vertexOffset, unsigned int firstInstance) {

}
void vk_cmdDrawNonIndexedIndirect(commandbundle_tgfx_handle, unsigned long long sortKey,
  buffer_tgfx_handle drawDataBuffer, unsigned long long drawDataBufferOffset, buffer_tgfx_handle drawCountBuffer, unsigned long long drawCountBufferOffset, extension_tgfx_listhandle exts) {

}
void vk_cmdDrawIndexedIndirect(commandbundle_tgfx_handle, unsigned long long sortKey, buffer_tgfx_handle drawCountBuffer, unsigned long long drawCountBufferOffset, extension_tgfx_listhandle exts) {

}
void vk_destroyCommandBundle(commandbundle_tgfx_handle hnd) {

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