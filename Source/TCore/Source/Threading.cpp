#include "Threading.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <locale>
#include <iostream>
#include <codecvt>

#ifdef T_ENVWINDOWS
#include <windows.h>
#endif

#define TCORE_USE_CPP_WRAPPER
#include "Allocator.h"

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCThreading)
TCORE_PLUGIN_INIT(TCAllocator)
TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCThreading)
TCORE_PLUGIN_HARD_DEPENDENCY(TCAllocator, TCAllocator_PLUGIN_VERSION)
TCORE_PLUGIN_ENTRY_POINT_END()

namespace TCore
{
namespace Threading
{

// This class has to wait as std::conditional_variable() if there is no job when
// pop_front_strong() is called Because it contains jobs and shouldn't look for new jobs while
// already waiting for a job
class TSJobifiedRingBuffer
{
	std::mutex WaitData;
	std::condition_variable NewJobWaitCond;
	std::function<TCThreadFunc> Data[256];
	std::atomic_bool Locked = false;
	unsigned int head = 0, tail = 0;

public:
	std::atomic_uint64_t IsThreadBusy = 0;
	bool IsEmpty()
	{
		WaitData.lock();
		bool result = head == tail;
		WaitData.unlock();
		return result;
	}
	bool push_back_weak(const std::function<TCThreadFunc>& item)
	{
		WaitData.lock();
		if ((head + 1) % 256 != tail)
		{
			Data[head] = [this, item]() {
				this->IsThreadBusy.fetch_add(1);
				item();
				this->IsThreadBusy.fetch_sub(1);
			};
			head = (head + 1) % 256;
			NewJobWaitCond.notify_one();
			WaitData.unlock();
			return true;
		}

		WaitData.unlock();
		return false;
	}
	void push_back_strong(const std::function<TCThreadFunc>& item)
	{
		while (!push_back_weak(item))
		{
			std::function<TCThreadFunc> Job;
			pop_front_strong(Job);
			Job();
		}
	}
	TCResult pop_front_weak(std::function<TCThreadFunc>& item)
	{
		std::unique_lock lock(WaitData);
		if (tail != head)
		{
			item = Data[tail];
			tail = (tail + 1) % 256;
			return {TC_RESULTSTATE_SUCCESS, 0};
		}
		return {TC_RESULTSTATE_FAILURE, 0};
	}
	void pop_front_strong(std::function<TCThreadFunc>& item)
	{
		while (!pop_front_weak(item))
		{
			std::unique_lock lock(WaitData);
			NewJobWaitCond.wait(lock);
		}
	}
};

struct TCThread
{
	std::thread::id Id;
	std::thread::native_handle_type NativeHnd;
	TBool IsRunning = TTRUE;
	TBool IsJoined = TFALSE;

	TCThread GetHnd() { return (TCThread)this; }
	TBool IsJoinable() { return !IsRunning && !IsJoined; }
	void Join()
	{
		while (!IsJoinable())
		{
		}
		IsJoined = TTRUE;
	}
};

struct TCThreadingContext* GContext = nullptr;
struct TCThreadingContext
{
	std::map<std::thread::id, TCThread> ThreadHandles;
	std::atomic<TBool> ShouldClose;

	TCThread* GetThread(TCThread hnd) { return (TCThread*)hnd; }

	static void Initialize()
	{
		GContext = new TCThreadingContext;
		GContext->ShouldClose.store(false);
	}

	static TCThread Create(TCThreadFunc func, TCBuffer thread_data, const char* thread_name)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring wideString = converter.from_bytes(thread_name);

		auto thread = new TCThread;
		std::thread newThread([&thread_data, func, thread_name, thread]() {
			thread->IsRunning = true;
			thread->IsJoined = false;
			func(thread_data);
			thread->IsRunning = false;
		});
		newThread.detach();

		SetThreadDescription(thread->NativeHnd, wideString.c_str());
		GContext->ThreadHandles.insert(std::make_pair(thread->Id, thread->GetHnd()));
		return thread->GetHnd();
	}

	static TCResult Join(TCThread thread)
	{
		auto t = GContext->GetThread(thread);
		t->Join();
		return {TC_RESULTSTATE_SUCCESS, 0};
	}

	static void Sleep(TU8 milliseconds) { ::Sleep(milliseconds); }

	static TCThread GetCurrentThread() { return GContext->ThreadHandles[std::this_thread::get_id()]; }
};

struct TCJobContext* JobContext = nullptr;
struct TCJobContext
{
public:
	TSJobifiedRingBuffer Jobs;
	// If you want only a portion of threads to be in job system, use this
	static void ConsumeJobsFromCurrentThread() {}
	// Checks hardware capabilities and creates a thread in job consume loop for each core
	static void ConsumeFromAllCores(TBool except_this_thread) {}
	static void DispatchTask(TCThreadFunc job, TU4 dispatch_size) {}
	// Waits till there is no more jobs to dispatch and all threads in the system are idle
	static void WaitIdle()
	{
		while (!JobContext->Jobs.IsEmpty())
		{
		}
		JobContext->Jobs.Lock();
	}
};

struct TCTaskContext* TaskContext = nullptr;
struct TCTaskContext
{
public:
	struct TCTaskedThread
	{
		TCThread Thread;
		TSJobifiedRingBuffer Jobs;
		TBool ShouldClose;
		TBool ForceClose;
	};
	Vector<TCTaskedThread> Threads;

	// Add checks here
	TCTaskedThread* GetThreadFromHnd(TCThread hnd) { return (TCTaskedThread*)hnd; }

	static TCThread CreateTaskedThread(const char* thread_name)
	{
		auto thread = new TCTaskedThread;
		auto threadLoop = [](TCBuffer buffer) -> TCResult {
			TCTaskedThread* thread = (TCTaskedThread*)buffer.Data;
			thread->Thread = TCThreading->GetCurrentThread();
			while (!thread->ShouldClose)
			{
				while (!thread->Jobs.IsEmpty() && !thread->ForceClose)
				{
					std::function<TCThreadFunc> func{};
					thread->Jobs.pop_front_strong(func);
					func();
				}
			}
		};
		auto hnd = TCThreadingContext::Create(threadLoop, {.Data = thread, .Size = sizeof(thread)}, thread_name);
	}

	static void EnqueueTask(TCThread thread, TCThreadFunc task, TCBuffer data)
	{
		auto t = TaskContext->GetThreadFromHnd(thread);
		t->Jobs.push_back_strong()
	}

	static void StopTaskedThread(TCThread thread, TBool wait_all_tasks_to_end)
	{
		auto t = TaskContext->GetThreadFromHnd(thread);
		if (!wait_all_tasks_to_end)
			t->ForceClose = true;
		t->ShouldClose = true;
		TCThreadingContext::Join(thread);
	}
};

} // namespace Threading
} // namespace TCore

TCResult TCThreading_Initialize(const void** outPluginAPI)
{
	TCore::Threading::TCThreadingContext::Initialize();

	auto services = new ITCThreading;
	services->Create = TCore::Threading::TCThreadingContext::Create;
	services->Join = TCore::Threading::TCThreadingContext::Join;
	services->Sleep = TCore::Threading::TCThreadingContext::Sleep;
	services->GetCurrentThread = TCore::Threading::TCThreadingContext::GetCurrentThread;

	services->JobSystem = new ITCJob;
	services->JobSystem->Dispatch = TCore::Threading::TCJobContext::DispatchTask;
	services->JobSystem->ConsumeFromAllCores = TCore::Threading::TCJobContext::ConsumeFromAllCores;
	services->JobSystem->ConsumeJobsFromCurrentThread = TCore::Threading::TCJobContext::ConsumeJobsFromCurrentThread;
	services->JobSystem->WaitIdle = TCore::Threading::TCJobContext::WaitIdle;

	services->TaskSystem = new ITCTask;
	services->TaskSystem->CreateTaskedThread = TCore::Threading::TCTaskContext::CreateTaskedThread;
	services->TaskSystem->EnqueueTask = TCore::Threading::TCTaskContext::EnqueueTask;
	services->TaskSystem->StopTaskedThread = TCore::Threading::TCTaskContext::StopTaskedThread;

	TCThreading = services;
	*outPluginAPI = TCThreading;
	return {TC_RESULTSTATE_SUCCESS, 0};
}

void TCThreading_OnPluginLoadStateChange(const TCPluginInfo* info, TBool is_loaded) {}

TCResult TCThreading_OnPreShutdown() {}

TCResult TCThreading_Shutdown()
{
	return {TC_RESULTSTATE_SUCCESS, 0};
}

void EndUsageOfTheApi() {}