#include "synchronization_sys.h"

struct fence_dx
{
	ComPtr<ID3D12Fence> Fence;
	uint64_t FrameFenceValues[2] = {};
	HANDLE FenceEvent;

	enum FenceStatus : unsigned char
	{
		FENCE_STATUS_INVALID = 0,
		FENCE_STATUS_UNUSED = 1,
		FENCE_STATUS_USED = 2
	};
	FenceStatus CurrentStatus = FenceStatus::FENCE_STATUS_INVALID;
	// Each wait semaphore in a queue submission is unsignaled and never signaled in the same
	// submission
	std::vector<semaphore_idtype_dx> WaitSemaphores;
#ifdef VULKAN_DEBUGGING
	fence_idtype_dx Id;
#endif
	inline fence_idtype_dx GetId()
	{
#ifdef VULKAN_DEBUGGING
		return Id;
#else
		return this;
#endif
	}
	friend struct fencesys_vk;
};