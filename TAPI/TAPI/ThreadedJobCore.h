#pragma once
#include "API_includes.h"

/*TuranAPI's C Multi-Threading and Job System Library
* If you created threads before, you can pass them to the Threader. Threader won't create any other thread.
* But you shouldn't destroy any of their objects before destroying Threader, otherwise there will be a dangling pointer issue
* This ThreadPool system is also capable of using main thread as one the threads
*/

#ifdef TAPI_THREADING

TAPIHANDLE(wait)
TAPIHANDLE(threadingsystem)


#ifdef __cplusplus
extern "C" {
#endif
#ifdef TAPI_THREADING_CPP_HELPER
	//Please don't use this function as a user, this is for C++ templates
	void tapi_JobSearch_DONTUSE(tapi_threadingsystem ThreadingSys);
#endif

	unsigned char tapi_GetThisThreadIndex(tapi_threadingsystem ThreadingSys);
	unsigned char tapi_GetThreadCount(tapi_threadingsystem ThreadingSys);

	/*Wait a job; First the thread yields. If it should wait again, executes any other job while waiting
	* Be careful of nonwait-in-wait situations such as;
	You wait for Job B (Thread2) in Job A (Thread1). Job B takes too long, so Thread1 runs Job C here while Job B is running.
	Job C calls Job D and waits for it, so Job D is executed (it doesn't have to start immediately, but in worst case it starts) (Thread3 or Thread1)
	Job D depends on some data that Job B is working on but you forgot to call wait for Job B in Job D.
	So Job D and Job B works concurrently, which are working on the same data.
	* To handle this case, you should either;
	1) Create-store your JobWaitInfos in a shared context (this requires ClearJobWait() operation and planning all your jobs beforehand)
	2) Use WaitJob_empty in Job A, so thread'll keep yielding till job finishes
	*/
	tapi_wait tapi_Create_Wait(tapi_threadingsystem ThreadingSys);
	void tapi_waitJob_busy(tapi_threadingsystem ThreadingSys, tapi_wait wait);
	void tapi_waitJob_empty(tapi_threadingsystem ThreadingSys, tapi_wait wait);
	void tapi_ClearWaitInfo(tapi_threadingsystem ThreadingSys, tapi_wait wait);
	void tapi_waitForAllOtherJobs(tapi_threadingsystem ThreadingSys);

	//If you want to wait for this job, you can pass a JobWaitInfo
	//But if you won't wait for it, don't use it because there may be crashes because of dangling wait reference
	//Critical Note : You shouldn't use same JobWaitInfo object across Execute_withWait()s without ClearJobWait()!
	void tapi_Execute_withWait(tapi_threadingsystem ThreadingSys, void(*func)(), tapi_wait* wait);
	void tapi_Execute_withoutWait(tapi_threadingsystem ThreadingSys, void(*func()));
	void tapi_JobSystem_Start(tapi_threadingsystem* ThreadingSysPTR, void(*Main()));
	//If you want to close job system, you should call this instead of Desctructor!
	//This function may run jobs if there is any when it is called
	//So that means, destruction of the job system is the responsibility of the user to handle their jobs
	//I mean, you probably should synchronize all your jobs at one point and check if you should close the job system
	//If you call this while running some other jobs in other threads and they don't care if destructor is called
	//Application may run forever 
	void tapi_CloseJobSystem(tapi_threadingsystem threadingsys);

#ifdef __cplusplus
}
#endif



#endif

//C++ Multi-Threading helpers
#ifdef TAPI_THREADING_CPP_HELPER
#ifndef TAPI_THREADING
#error "You should define TAPI_THREADING to use TAPI_THREADING_CPP_HELPER"
#endif
#include <iostream>
#include <vector>
#include <string.h>
#include <functional>
#include <mutex>
#include <atomic>



	//If you want to wait for this job, you can pass a JobWaitInfo
	//But if you won't wait for it, don't use it because there may be crashes because of dangling wait reference
	//Critical Note : You shouldn't use same JobWaitInfo object across Execute_withWait()s without ClearJobWait()!
void tapi_Execute_withWait(const std::function<int(void)>& func, tapi_threadingsystem ThreadingSys, tapi_wait wait);
void tapi_Execute_withoutWait(const std::function<int(void)>& func, tapi_threadingsystem ThreadingSys);
tapi_threadingsystem tapi_JobSystem_Start(const std::function<int()>& func);
//This class includes xxx_strong() functions which means they are doing jobs instead of waiting
template<class T, unsigned int size>
class TURANAPI TSJobifiedRingBuffer {
	std::mutex WaitData;
	T data[size];
	tapi_threadingsystem ThreadingSys;
	unsigned int head = 0, tail = 0;
public:
	TSJobifiedRingBuffer(tapi_threadingsystem Threader) {
		ThreadingSys = Threader;
	}
	template<T>
	bool push_back_weak(const T& item) {
		std::unique_lock<std::mutex> WaitLock(WaitData);
		if ((head + 1) % size != tail) {
			head = (head + 1) % size;
			data[head] = item;
			return true;
		}

		return false;
	}
	template<T>
	void push_back_strong(const T& item) {
		while (!push_back_weak(item)) {
			tapi_JobSearch_DONTUSE(ThreadingSys);
		}
	}
	template<T>
	bool pop_front_weak(T& item) {
		std::unique_lock<std::mutex> WaitLock(WaitData);
	}
	template<T>
	void pop_front_strong(T& item) {
		if (!pop_front_weak(item)) {
			tapi_JobSearch_DONTUSE(ThreadingSys);
		}
	}
};


struct tapi_atomic_uint {
	std::atomic<unsigned int> data;
public:
	tapi_atomic_uint(unsigned int Ref);
	tapi_atomic_uint();

	//Returns the old value
	unsigned int DirectAdd(const unsigned int& add);
	//Returns the old value
	unsigned int DirectSubtract(const unsigned int& sub);
	void DirectStore(const unsigned int& Store);
	unsigned int DirectLoad() const;

	//Deep Sleeping: The thread won't be available soon enough and application will fail at some point (or be buggy)
	//because condition's not gonna be met soon enough. By the way, it keeps yielding at that time.
	//This situation is so dangerous because maybe other threads's keep creating jobs that depends on this job.
	//In such a situation, 2 cases possible;
	//Case 1) Developers were not careful enough to do a "WaitForTheJob()" and the following operations was depending on the job's execution
	//so all the following operations are wrong because job is not finished executing because of Deep Sleeping.
	//Case 2) Developers were careful enough but this means developers' design lacks of a concept that covers not-meeting the condition.
	//For example; atomic::LimitedAdd_strong() waits until addition happens. 
	//But if addition is not possible for the rest of the application, the thread'll keep yielding until termination.
	//Or addition happens late enough that Case 1 occurs, which means following execution is wrong.

	//If you want to try to do addition but want to check if it is possible in a lock-free way, it is this
	//There are 2 cases this function may return false;
	//1) Your addition is already exceeds the maxlimit even there isn't any concurrent addition
	//2) Your addition may not exceed if there wouldn't be any concurrent addition
	//If you think your case is '1', my advice is that you should design your application such that,
	//the function that calls "LimitedAdd_weak()" may fail and user is aware about that
	//You should predict cases like '2' at design level and should change your job scheduling accordingly
	//But if you didn't and need a tricky solution, you can use LimitedAdd_strong(). But be aware of long waits.
	//Also for some possible Deep Sleeping because data won't be small enough to do the addition
	//By design level, I mean that;
	//1) You should know when there won't be any concurrent operations on the data and read it when it's time
	//2) You should predict the max 'add' and min value (and their timings) when concurrent operations will occur
	//3) You should reduce concurrent operations or schedule your concurrent operations such that LimitedAdd_strong()'ll never
	//introduce late awakening (Never awakening) or concurrent operations'll use job waiting instead of this lock-free system
	bool LimitedAdd_weak(const unsigned int& add, const unsigned int& maxlimit);
	//You should use this function only for this case, because this is not performant;
	//The value is very close to the limit so you are gonna reduce the value some nanoseconds later from other threads
	//And when the value is reduced, you want addition to happen immediately
	//"Immediately" here means after this_thread::yield()
	//If you pay attention, this function doesn't return bool. Because this will block the thread until addition is possible
	//So 
	void LimitedAdd_strong(const unsigned int& add, const unsigned int& maxlimit);
	//Similar but the reverse of the LimitedAdd_weak()
	bool LimitedSubtract_weak(const unsigned int& subtract, const unsigned int& minlimit);
	//Similar but the reverse of the LimitedAdd_strong()
	void LimitedSubtract_strong(const unsigned int& subtract, const unsigned int& minlimit);
};

//Thread Local std::vector implementation
//Functions with "unsigned char ThreadID" doesn't use any sync primitive
//Functions with "unsigned char& ThreadID" uses mutex as sync primitive
//Don't use PauseAllOperations with functions that uses mutex as sync primitive (Because it causes deadlock)
template<class T>
class TURANAPI tapi_threadlocal_vector {
	std::vector<T>* PerThreadVectors;
	unsigned char ThreadCount = 0;
	std::mutex Sync;
	tapi_threadingsystem ThreadingSys;
public:
	tapi_threadlocal_vector(tapi_threadingsystem Threader) {
		ThreadingSys = Threader;
		PerThreadVectors = new std::vector<T>[tapi_GetThreadCount(ThreadingSys)];
		ThreadCount = tapi_GetThreadCount(ThreadingSys);
	}
	tapi_threadlocal_vector(const tapi_threadlocal_vector<T>& copyfrom) {
		PerThreadVectors->resize(copyfrom.ThreadCount);
		for (unsigned char ThreadID = 0; ThreadID < copyfrom.ThreadCount; ThreadID++) {
			PerThreadVectors[ThreadID] = copyfrom.PerThreadVectors[ThreadID];
		}
	}
	T& get(unsigned char ThreadID, unsigned int ElementIndex) {
		return PerThreadVectors[ThreadID][ElementIndex];
	}
	unsigned int size(unsigned char ThreadID) {
		return PerThreadVectors[ThreadID].size();
	}
	void push_back(unsigned char ThreadID, const T& item) {
		PerThreadVectors[ThreadID].push_back(item);
	}
	void erase(unsigned char ThreadID, unsigned int ElementIndex) {
		PerThreadVectors[ThreadID].erase(PerThreadVectors[ThreadID].begin() + ElementIndex);
	}
	void clear(unsigned char ThreadID) {
		PerThreadVectors[ThreadID].clear();
	}
	T* data(unsigned char ThreadID) {
		return PerThreadVectors[ThreadID].data();
	}
	bool Search(const T& item, unsigned char ThreadID, unsigned int& ElementIndex) {
		for (unsigned int i = 0; i < PerThreadVectors[ThreadID].size(); i++) {
			if (PerThreadVectors[ThreadID][i] == item) {
				ElementIndex = i;
				return true;
			}
		}
		return false;
	}
	bool SearchAllThreads(const T& item, unsigned char& ThreadID, unsigned int& ElementIndex) {
		std::lock_guard<std::mutex> Locker(Sync);
		for (unsigned int ThreadIndex = 0; ThreadIndex < ThreadCount; ThreadIndex++) {
			for (unsigned int i = 0; i < PerThreadVectors[ThreadIndex].size(); i++) {
				if (PerThreadVectors[ThreadIndex][i] == item) {
					ThreadID = ThreadIndex;
					ElementIndex = i;
					return true;
				}
			}
		}
		return false;
	}
	void PauseAllOperations(std::unique_lock<std::mutex>& Locker) {
		Locker = std::unique_lock<std::mutex>(Sync);
	}
	~tapi_threadlocal_vector() {
		delete[] PerThreadVectors;
	}
};



#endif //TAPI_THREADING_CPP_HELPER