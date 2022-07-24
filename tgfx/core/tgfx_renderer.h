#pragma once
#include "tgfx_forwarddeclarations.h"
#include "tgfx_structs.h"

typedef void (*tgfx_rendererKeySortFunc)(unsigned long long* keyList);
typedef struct renderer_tgfx {
  // Command Buffer Functions
  ////////////////////////////
  
  // Command buffers are one-time only buffers
  //  so when you submit them for execution, they'll be freed after their execution
  commandbuffer_tgfx_handle (*getCommandBuffer)(gpu_tgfx_handle gpu, extension_tgfx_listhandle exts);
  // If you call this in a rendersubpass, bundle should be created with the rendersubpass' handle
  // If you call this outside of renderpasses, bundle shouldn't be created with rendersubpass handle (NULL should be passed)
  // Supported Extensions: No_Secondary_CommandBuffer 
  void (*executeBundles)(commandbuffer_tgfx_handle commandBuffer, commandbundle_tgfx_handle* bundles, const unsigned long long* bundleSortKeys, tgfx_rendererKeySortFunc sortFunc, extension_tgfx_listhandle exts);
  void (*start_renderpass)(commandbuffer_tgfx_handle commandBuffer, renderpass_tgfx_handle renderPass);
  void (*next_rendersubpass)(commandbuffer_tgfx_handle commandBuffer, rendersubpass_tgfx_handle renderSubPass);
  void (*end_renderpass)(commandbuffer_tgfx_handle commandBuffer);
  // @param commandBufferList can be NULL. This should be only used if you want to copy a semaphore's state to other ones.
  void (*submitQueue)(gpuqueue_tgfx_handle queue, commandbuffer_tgfx_listhandle commandBuffersList, fence_tgfx_listhandle fenceList, semaphore_tgfx_listhandle waitSemaphoreList, semaphore_tgfx_listhandle signalSemaphoreList, extension_tgfx_listhandle exts);
  
  // Synchronization Functions

  // @param fenceCount: Fence count that will be created
  // @param isSignaled: Pointer to list of signal states of each created fence. NULL will create all fences as unsignaled
  // @param fenceList: User should create the array of fence_tgfx_handles and pass it. So array isn't created by backend
  void (*createFences)(gpu_tgfx_handle gpu, unsigned int fenceCount, const unsigned char* isSignaled, fence_tgfx_listhandle fenceList);
  // @param semaphoreList: User should create the array of semaphore_tgfx_handles and pass it. So array isn't created by backend
  void (*createSemaphores)(gpu_tgfx_handle gpu, unsigned int semaphoreCount, semaphore_tgfx_listhandle semaphoreList);

  // Command Bundle Functions
  ////////////////////////////

  // @param subpassHandle: Used to check if calls are called correctly + does bundle matches with the active rendersubpass
  // @param sortFunc: When executed, calls are sorted according to this function. NULL: calls sorted in incremental order
  commandbundle_tgfx_handle (*createCommandBundle)(rendersubpass_tgfx_handle subpassHandle, tgfx_rendererKeySortFunc sortFunc);
  void (*cmdBindBindingTable)(commandbundle_tgfx_handle bundle, unsigned long long sortKey, bindingtable_tgfx_handle bindingtable);
  // For devices that doesn't allow storage buffers to store vertex buffers, this function is needed
  void (*cmdBindVertexBuffer)(commandbundle_tgfx_handle bundle, unsigned long long sortKey, buffer_tgfx_handle buffer, unsigned long long offset, unsigned long long boundSize);
  // @param indexDataTypeSize: Specify the byte size of index data type (most devices support only 2 and 4)
  void (*cmdBindIndexBuffers)(commandbundle_tgfx_handle bundle, unsigned long long sortKey, buffer_tgfx_handle buffer, unsigned long long offset, unsigned char indexDataTypeSize);
  void (*cmdDrawNonIndexedDirect)(commandbundle_tgfx_handle bundle, unsigned long long sortKey, unsigned int vertexCount, unsigned int instanceCount, unsigned int firstVertex, unsigned int firstInstance);
  void (*cmdDrawIndexedDirect)(commandbundle_tgfx_handle bundle, unsigned long long sortKey, unsigned int indexCount, unsigned int instanceCount, unsigned int firstIndex, int vertexOffset, unsigned int firstInstance);
  // @param Note: If your GPU doesn't support draw_count_indirect; drawCountBuffer should be NULL & bufferOffset should be drawCount
  void (*cmdDrawNonIndexedIndirect)(commandbundle_tgfx_handle, unsigned long long sortKey, 
    buffer_tgfx_handle drawDataBuffer, unsigned long long drawDataBufferOffset, buffer_tgfx_handle drawCountBuffer, unsigned long long drawCountBufferOffset, extension_tgfx_listhandle exts);
  // @param Note: If your GPU doesn't support draw_count_indirect; drawCountBuffer should be NULL & bufferOffset should be drawCount
  void (*cmdDrawIndexedIndirect)(commandbundle_tgfx_handle, unsigned long long sortKey, buffer_tgfx_handle drawCountBuffer, unsigned long long drawCountBufferOffset, extension_tgfx_listhandle exts);
  // If you won't execute same bundle later, destroy to allow backend implementation to optimize memory usage
  void (*destroyCommandBundle)(commandbundle_tgfx_handle hnd);
} renderer_tgfx;
