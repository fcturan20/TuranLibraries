#pragma once
#include "tgfx_forwarddeclarations.h"
#include "tgfx_structs.h"

typedef void (*tgfx_rendererKeySortFunc)(unsigned long long* keyList);
typedef tgfx_rendererKeySortFunc rendererKeySortFunc_tgfx;
typedef struct tgfx_renderer{
  // Command Buffer Functions
  ////////////////////////////
  
  // Command buffers are one-time only buffers
  //  so when you submit for execution, they'll be freed after their execution
  commandbuffer_tgfx_handle (*getCommandBuffer)
    (gpu_tgfxhnd gpu, extension_tgfxlsthnd exts);
  // In a Rendersubpass: Bundle should be created with the rendersubpass' handle
  // Outside: Bundle should be created with rendersubpass as NULL.
  // Supported Extensions: No_Secondary_CommandBuffer 
  void (*executeBundles)
   (
     commandbuffer_tgfx_handle commandBuffer, commandBundle_tgfxhnd* bundles,
     const unsigned long long* bundleSortKeys, tgfx_rendererKeySortFunc sortFnc,
     extension_tgfxlsthnd exts
   );
  void (*start_renderpass)
    (commandbuffer_tgfx_handle commandBuffer, renderPass_tgfxhnd renderPass);
  void (*next_rendersubpass)
    (commandbuffer_tgfx_handle cmdBuffer, renderSubPass_tgfxhnd renderSubPass);
  void (*end_renderpass)(commandbuffer_tgfx_handle commandBuffer);
  // @param commandBufferList: Only use if you want to copy a semaphore's state.
  void (*submitQueue)
  (
    gpuQueue_tgfxhnd queue, commandbuffer_tgfxlsthnd commandBuffersList,
    fence_tgfxlsthnd fenceList, semaphore_tgfxlsthnd waitSemaphoreList,
    semaphore_tgfxlsthnd signalSemaphoreList, extension_tgfxlsthnd exts
  );
  
  // Synchronization Functions

  // @param fenceCount: Fence count that will be created
  // @param isSignaled: Pointer to list of signal states of each created fence.
  //    NULL will create all fences as unsignaled
  // @param fenceList: User should create the array of fence_tgfx_handles. 
  //    So array isn't created by backend
  void (*createFences)
  (
    gpu_tgfxhnd gpu, unsigned int count, unsigned char signaled,
    fence_tgfxlsthnd fenceList
  );
  // @param semaphoreList: User should create array of semaphore_tgfx_handles.
  //    So array isn't created by backend
  void (*createSemaphores)
    (gpu_tgfxhnd gpu, unsigned int count, semaphore_tgfxlsthnd semaphoreList);

  // Command Bundle Functions
  ////////////////////////////

  // @param subpassHandle: Used to check if calls are called correctly + does
  //    bundle matches with the active rendersubpass
  // @param sortFunc: When executed, calls are sorted according to this function
  //    NULL: calls sorted in incremental order
  commandBundle_tgfxhnd (*createCommandBundle)
    (renderSubPass_tgfxhnd subpassHandle, rendererKeySortFunc_tgfx sortFunc);
  void (*cmdBindBindingTable)
  (
    commandBundle_tgfxhnd bundle, unsigned long long sortKey,
    bindingTable_tgfxhnd bindingtable
  );
  // For devices that doesn't allow storage buffers to store vertex buffers,
  //   this function is needed
  void (*cmdBindVertexBuffer)
  (
    commandBundle_tgfxhnd bundle, unsigned long long key, buffer_tgfxhnd buffer,
    unsigned long long offset, unsigned long long boundSize
  );
  // @param indexDataTypeSize: Specify the byte size of index data type
  //   (most devices support only 2 and 4)
  void (*cmdBindIndexBuffers)
  (
    commandBundle_tgfxhnd bundle, unsigned long long key, buffer_tgfxhnd buffer,
    unsigned long long offset, unsigned char indexDataTypeSize
  );
  void (*cmdDrawNonIndexedDirect)
  (
    commandBundle_tgfxhnd bndl, unsigned long long key, unsigned int vertCount,
    unsigned int instanceCount, unsigned int frstVert, unsigned int frstInstance
  );
  void (*cmdDrawIndexedDirect)
  (
    commandBundle_tgfxhnd bndl, unsigned long long key, unsigned int indxCount,
    unsigned int instanceCount, unsigned int firstIndex, int vertexOffset,
    unsigned int firstInstance
  );
  // @param Note: If your GPU doesn't support draw_count_indirect;
  //   drawCountBuffer should be NULL & bufferOffset should be drawCount
  void (*cmdDrawNonIndexedIndirect)
  (
    commandBundle_tgfxhnd bndl, unsigned long long key, buffer_tgfxhnd dataBffr,
    unsigned long long drawDataBufferOffset, buffer_tgfxhnd drawCountBuffer,
    unsigned long long drawCountBufferOffset, extension_tgfxlsthnd exts
  );
  // @param Note: If your GPU doesn't support draw_count_indirect;
  //   drawCountBuffer should be NULL & bufferOffset should be drawCount
  void (*cmdDrawIndexedIndirect)
  (
    commandBundle_tgfxhnd bndl, unsigned long long key, buffer_tgfxhnd dataBffr,
    unsigned long long drawCountBufferOffset, extension_tgfxlsthnd exts
  );
  // If you won't execute same bundle later, destroy to allow backend
  //   implementation to optimize memory usage
  void (*destroyCommandBundle)(commandBundle_tgfxhnd hnd);
} renderer_tgfx;
