#pragma once
#include <vector>

#include "vk_core.h"
#include "vk_predefinitions.h"
#include "vk_resource.h"

namespace TGFX
{
namespace Vulkan
{
/*
This class manages queues and command buffer allocations per GPU
  This is important in multi-threaded cases because;
1) You can't use the same command pool from different threads at the same time
2) Command buffers should be tracked as if their execution has done, free command buffer
3) Execution tracking is achieved by tracking queues, fences etc. So queues should be managed
	here too
NOTE: User-side command buffers are different because they're not actual calls (sorting
is needed) So after sorting, thread calls poolManager to get command buffer. poolManager find
available command pool, if there isn't then creates. Then attaches command pool to queue, so if
queue finishes executing then command pool is freed. *POSSIBILITY: Maybe no need to bind command
pools, just bind command buffers to queue.
  *  While creating cmdbuffers, search all previous pools and select the one with
	  no actively recorded-executed cmdbuffer. Because pool creation-destruction is costly.
*/
struct manager_vk
{
public:
	// Submit queue operations to GPU
	// Adds the queue's binary semaphore to the first&last submit to synchronize queue submissions.
	// This is because some queue operations are not synchronized by Vulkan (present and sparse).
	// NOTE: Queue's synchronizer binary semaphore is named : "QueueBinSem aka QBS".
	// For ex: Present 1 -> CmdBufferSubmitList (Submits A, B & C) -> Present 2.
	//  Present 1 signals QueueBinSem. SubmitA waits (un-signals) QBS. SubmitC signals QBS. Present 2
	//  both waits then signals QBS.
	void queueSubmit(Queue* family);
};

} // namespace Vulkan
} // namespace TGFX