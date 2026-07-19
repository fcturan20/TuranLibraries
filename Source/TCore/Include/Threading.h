#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(TCThreading, "tcThreading", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

TCORE_DEFINE_HANDLE(TCThread);
TCORE_DEFINE_HANDLE(TCSemaphore);

// Job and Task System with Threading
// Job is atomic and mostly parallelizable operations using Dispatch Index
// Task is a complex and mostly sequential/hardly parallelizable operations
// For example; To synchronize undo/redo operations of a system, Task system should be used
// For example; To cull render objects, Job system should be used

typedef TCResult (*TCThreadFunc)(TCBuffer);
// This system is designed to parallelize some operations using DispatchIndex
// A Job is a function and a user pointer
// A Job Instance is a function running on a thread
// Job Instance can be in 4 states; PENDING, RUNNING, WAITING, EXECUTED
// PENDING; No operation yet, waiting for a thread pick it up
// RUNNING; Picked up by a thread and actively executing
// WAITING; Picked up by a thread but waiting for a semaphore to be signaled
// EXECUTED; Completed the Job Instance. Will be destroyed shortly after
// A Job Instance owned by a thread can not change its thread.
// This means, if the thread starts running a new Job Instance while the previous Job Instance that it runs is waiting,
typedef struct ITCJob
{
	void (*ConsumeJobsFromCurrentThread)();
	// Checks hardware capabilities and creates a thread in task consume loop for each core
	void (*ConsumeFromAllCores)(TBool except_this_thread);
	void (*Dispatch)(TCThreadFunc task, TU4 dispatch_size);
	// Waits till the semaphore is signaled
	// @param consume_tasks: Executes PENDING state tasks till signaled. Checks everytime a task is executed.
	// If false, yields the thread till the semaphore signaled.
	void (*WaitSemaphore)(TCSemaphore semaphore, TBool consume_tasks);
	// Waits till there is no more tasks to dispatch and all threads in the system are idle
	void (*WaitIdle)();
	// No job will be picked up by a thread until either ConsumeJobsFromCurrentThread or UseAllCores is called
	void (*ReleaseThreads)();
} ITCJob;

typedef struct ITCTask
{
	TCThread (*CreateTaskedThread)(const char* thread_name);
	void (*EnqueueTask)(TCThread thread, TCThreadFunc task, TCBuffer data);
	// Thread will be destroyed after this
	void (*StopTaskedThread)(TCThread thread, TBool wait_all_tasks_to_end);
} ITCTask;

// Core functionalities to make debugging easier

typedef struct ITCThreading
{
	TCThread (*Create)(TCThreadFunc main, TCBuffer thread_data, const char* thread_name);
	// @return {TC_RESULTSTATE_SUCCESS, 0}; Thread handle becomes invalid
	TCResult (*Join)(TCThread thread);
	void (*Sleep)(TU8 milliseconds);
	TCThread (*GetCurrentThread)();

	// Synchronization

	TCSemaphore (*CreateSemaphore)();
	TCResult (*SignalSemaphore)(TCSemaphore semaphore);
	TBool (*IsSemaphoreSignaled)(TCSemaphore semaphore);
	// It's undefined behavior to use the semaphore handle after calling this.
	void (*DestroySemaphore)(TCSemaphore semaphore);

	// Helper Systems
	ITCTask* TaskSystem;
	ITCJob* JobSystem;
} ITCThreading;

TCORE_END_C_LINKAGE

// C++ wrapper
#define TCORE_USE_CPP_WRAPPER
#if defined(TCORE_CPP_20) & defined(TCORE_USE_CPP_WRAPPER)
#include <functional>
namespace TCore
{
class Semaphore
{
public:
	Semaphore() { Handle = TCThreading->CreateSemaphore(); }
	TCResult Signal() { return TCThreading->SignalSemaphore(Handle); }
	TBool IsSignaled() { return TCThreading->IsSemaphoreSignaled(Handle); }
	~Semaphore() { TCThreading->DestroySemaphore(Handle); }
	TCSemaphore Handle;
	operator TCSemaphore() const { return Handle; }
};

class Task
{
	virtual void Execute() = 0;
};

class TaskedThread
{
private:
protected:
	void Start(const char* thread_name)
	{
		auto semaphore = TCThreading->CreateSemaphore();
		auto threadLoop = [](TCBuffer buffer) {
			TaskedThread* thread = (TaskedThread*)buffer.Data;
			thread->Semaphore.Signal();
		};
		Thread = TCThreading->TaskSystem->CreateTaskedThread(thread_name);
	}
	void Enqueue(std::function<TCResult(Task&&)> task) {}
	void Stop()
	{
		TCThreading->TaskSystem->StopTaskedThread(Thread, TTRUE);
		TCThreading->DestroySemaphore(Semaphore);
	}
	TCThread Thread;
	Semaphore Semaphore;
};
} // namespace TCore
#endif