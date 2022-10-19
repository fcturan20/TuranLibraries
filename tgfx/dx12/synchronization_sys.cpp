#include "synchronization_sys.h"

struct fence_dx {
  ComPtr<ID3D12Fence> g_Fence;
  uint64_t            g_FrameFenceValues[2] = {};
  HANDLE              g_FenceEvent;

  enum fence_status : unsigned char { invalid = 0, unused = 1, used = 2 };
  fence_status current_status = fence_status::invalid;
  // Each wait semaphore in a queue submission is unsignaled and never signaled in the same
  // submission
  std::vector<semaphore_idtype_dx> wait_semaphores;
#ifdef VULKAN_DEBUGGING
  fence_idtype_dx ID;
#endif
  inline fence_idtype_dx getID() {
#ifdef VULKAN_DEBUGGING
    return ID;
#else
    return this;
#endif
  }
  friend struct fencesys_vk;
};