#define TAPI_THREADING_CPP_HELPER
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <map>
#include <mutex>
#include <condition_variable>
#include <thread>




template<typename T, unsigned int size>
class TURANAPI ThreadSafeRingBuffer {
	std::mutex WaitData;
	std::condition_variable strongWait;
	unsigned int head = 0, tail = 0;
	T data[size];
public:
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
			std::this_thread::yield();
		}
	}
	template<T>
	bool pop_front_weak(T& item) {
		std::unique_lock<std::mutex> WaitLock(WaitData);
	}
	template<T>
	void pop_front_strong(T& item) {
		std::unique_lock<std::mutex> WaitLock(WaitData);

	}
};
//This class has to wait as std::conditional_variable() if there is no job when pop_front_strong() is called
//Because it contains jobs and shouldn't look for new jobs while already waiting for a job
template<>
class TSJobifiedRingBuffer<std::function<void()>, 256> {
	std::mutex WaitData;
	std::condition_variable NewJobWaitCond;
	std::function<void()> data[256];
	unsigned int head = 0, tail = 0;
public:
	bool isEmpty() {
		WaitData.lock();
		bool result = head == tail;
		WaitData.unlock();
		return result;
	}
	bool push_back_weak(const std::function<void()>& item) {
		WaitData.lock();
		if ((head + 1) % 256 != tail) {
			data[head] = item;
			head = (head + 1) % 256;
			NewJobWaitCond.notify_one();
			WaitData.unlock();
			return true;
		}

		WaitData.unlock();
		return false;
	}
	void push_back_strong(const std::function<void()>& item) {
		while (!push_back_weak(item)) {
			std::function<void()> Job;
			pop_front_strong(Job);
			Job();
		}
	}
	bool pop_front_weak(std::function<void()>& item) {
		WaitData.lock();
		if (tail != head) {
			item = data[tail];
			tail = (tail + 1) % 256;
			WaitData.unlock();
			return true;
		}
		WaitData.unlock();
		return false;
	}
	void pop_front_strong(std::function<void()>& item) {
		while (!pop_front_weak(item)) {
			std::unique_lock<std::mutex> Locker(WaitData);
			NewJobWaitCond.wait(Locker);
		}
	}
};


inline std::atomic_bool& WaitHandleConverter(tapi_wait handle) { return *(std::atomic_bool*)(handle); }

class JobSystem {
public:
	std::map<std::thread::id, unsigned char> ThreadIDs;
	std::atomic_bool ShouldClose;
	TSJobifiedRingBuffer<std::function<void()>, 256> Jobs;
};


void tapi_JobSearch_DONTUSE(tapi_threadingsystem ThreadingSys){
	JobSystem* JOBSYS = (JobSystem*)ThreadingSys;
	//ShouldClose will be true if JobSystem destructor is called
	//Destructor will wait for all other jobs to finish
	while (!JOBSYS->ShouldClose.load()) {
		std::function<void()> Job;
		JOBSYS->Jobs.pop_front_strong(Job);
		Job();
	}
#ifdef TURAN_LOGGER
	LOG_STATUS("Thread " + to_string(unsigned int(tapi_GetThisThreadIndex())) + " is exited from Job System!\n");
#endif
}

tapi_threadingsystem tapi_JobSystem_Start(const std::function<void()>& Main) {
	tapi_threadingsystem Threader = (tapi_threadingsystem)new JobSystem;
	JobSystem* JOBSYS = (JobSystem*)Threader;
	JOBSYS->ShouldClose.store(false);
	JOBSYS->Jobs.push_back_weak(Main);

	JOBSYS->ThreadIDs.insert(std::pair<std::thread::id, unsigned char>(std::this_thread::get_id(), 0));
	unsigned int ThreadCount = std::thread::hardware_concurrency();
	if (ThreadCount == 1) {
		std::cout << "Job system didn't create any std::thread objects because your CPU only has 1 core!\n";
	}
	else {
		ThreadCount--;
	}
	for (unsigned int ThreadIndex = 0; ThreadIndex < ThreadCount; ThreadIndex++) {
		std::thread newthread([JOBSYS, ThreadIndex]() {JOBSYS->ThreadIDs.insert(std::pair<std::thread::id, unsigned char>(std::this_thread::get_id(), ThreadIndex + 1)); tapi_JobSearch_DONTUSE((tapi_threadingsystem)JOBSYS); });
		newthread.detach();
	}

	tapi_JobSearch_DONTUSE(Threader);
	return Threader;
}
void tapi_JobSystem_Start(tapi_threadingsystem* ThreadingSysPTR, void(*Main())) {
	*ThreadingSysPTR = (tapi_threadingsystem)new JobSystem;
	JobSystem* JOBSYS = (JobSystem*)*ThreadingSysPTR;
	JOBSYS->ShouldClose.store(false);
	JOBSYS->Jobs.push_back_weak(Main);

	JOBSYS->ThreadIDs.insert(std::pair<std::thread::id, unsigned char>(std::this_thread::get_id(), 0));
	unsigned int ThreadCount = std::thread::hardware_concurrency();
	if (ThreadCount == 1) {
		printf("Job system didn't create any std::thread objects because your CPU only has 1 core!\n");
	}
	else {
		ThreadCount--;
	}
	for (unsigned int ThreadIndex = 0; ThreadIndex < ThreadCount; ThreadIndex++) {
		std::thread newthread([JOBSYS, ThreadIndex]() {JOBSYS->ThreadIDs.insert(std::pair<std::thread::id, unsigned char>(std::this_thread::get_id(), ThreadIndex + 1)); tapi_JobSearch_DONTUSE((tapi_threadingsystem)JOBSYS); });
		newthread.detach();
	}

	tapi_JobSearch_DONTUSE(*ThreadingSysPTR);
}

void tapi_Execute_withWait(tapi_threadingsystem ThreadingSys, const std::function<void()>& func, tapi_wait* wait) {
	JobSystem* JOBSYS = (JobSystem*)ThreadingSys;
	//That means job list is full, wake up a thread (if there is anyone sleeping) and yield current thread!
	JOBSYS->Jobs.push_back_strong([&wait, func]() {
		WaitHandleConverter(*wait).store(1);
		func();
		WaitHandleConverter(*wait).store(2);
		}
	);
}
void tapi_Execute_withoutWait(tapi_threadingsystem ThreadingSys, const std::function<void()>& func) {
	JobSystem* JOBSYS = (JobSystem*)ThreadingSys;
	JOBSYS->Jobs.push_back_strong(func);
}
void tapi_Execute_withWait(tapi_threadingsystem ThreadingSys, void(*func)(), tapi_wait* wait) {
	JobSystem* JOBSYS = (JobSystem*)ThreadingSys;
	//That means job list is full, wake up a thread (if there is anyone sleeping) and yield current thread!
	JOBSYS->Jobs.push_back_strong([&wait, func]() {
		WaitHandleConverter(*wait).store(1);
		func();
		WaitHandleConverter(*wait).store(2);
		}
	);
}
void tapi_Execute_withoutWait(tapi_threadingsystem ThreadingSys, void(*func())) {
	JobSystem* JOBSYS = (JobSystem*)ThreadingSys;
	JOBSYS->Jobs.push_back_strong(func);
}
tapi_wait tapi_Create_Wait(tapi_threadingsystem ThreadingSys) {
	JobSystem* JOBSYS = (JobSystem*)ThreadingSys;
	return (tapi_wait)new std::atomic_bool;
}
void tapi_waitJob_busy(tapi_threadingsystem ThreadingSys, tapi_wait wait) {
	JobSystem* JOBSYS = (JobSystem*)ThreadingSys;
	if (WaitHandleConverter(wait).load() != 2) {
		std::this_thread::yield();
	}
	while (WaitHandleConverter(wait).load() != 2) {
		std::function<void()> Job;
		JOBSYS->Jobs.pop_front_strong(Job);
		Job();
	}
}
void tapi_waitJob_empty(tapi_threadingsystem ThreadingSys, tapi_wait wait) {
	JobSystem* JOBSYS = (JobSystem*)ThreadingSys;
	while (WaitHandleConverter(wait).load() != 2) {
		std::this_thread::yield();
	}
}
void tapi_ClearWaitInfo(tapi_threadingsystem ThreadingSys, tapi_wait wait) {
	JobSystem* JOBSYS = (JobSystem*)ThreadingSys;
	WaitHandleConverter(wait).store(0);
}
void tapi_waitForAllOtherJobs(tapi_threadingsystem ThreadingSys) {
	JobSystem* JOBSYS = (JobSystem*)ThreadingSys;
	while (!JOBSYS->Jobs.isEmpty()) {
		std::function<void()> Job;
		JOBSYS->Jobs.pop_front_strong(Job);
		Job();
	}
}

unsigned char tapi_GetThisThreadIndex(tapi_threadingsystem ThreadingSys) {
	JobSystem* JOBSYS = (JobSystem*)ThreadingSys;
	return JOBSYS->ThreadIDs[std::this_thread::get_id()];
}
unsigned char tapi_GetThreadCount(tapi_threadingsystem ThreadingSys) {
	JobSystem* JOBSYS = (JobSystem*)ThreadingSys;
	return std::thread::hardware_concurrency();
}
void tapi_CloseJobSystem(tapi_threadingsystem ThreadingSys) {
	JobSystem* JOBSYS = (JobSystem*)ThreadingSys;
	JOBSYS->ShouldClose.store(true);
	tapi_waitForAllOtherJobs(ThreadingSys);
	delete JOBSYS;
	printf("Job system is closed!\n");
}


tapi_atomic_uint::tapi_atomic_uint(unsigned int Ref) : data(Ref) {
}
tapi_atomic_uint::tapi_atomic_uint() : data(0) {

}
unsigned int tapi_atomic_uint::DirectAdd(const unsigned int& add) {
	return data.fetch_add(add);
}
unsigned int tapi_atomic_uint::DirectSubtract(const unsigned int& sub) {
	return data.fetch_sub(sub);
}
void tapi_atomic_uint::DirectStore(const unsigned int& Store) {
	data.store(Store);
}
unsigned int tapi_atomic_uint::DirectLoad() const {
	return data.load();
}
bool tapi_atomic_uint::LimitedAdd_weak(const unsigned int& add, const unsigned int& maxlimit) {
	unsigned int x = data.load();
	if (x + add > maxlimit ||	//Addition is bigger
		x + add < x				//UINT overflow
		) {
		return false;
	}
	if (!data.compare_exchange_strong(x, x + add)) {
		return LimitedSubtract_weak(add, maxlimit);
	}
	return true;
}
void tapi_atomic_uint::LimitedAdd_strong(const unsigned int& add, const unsigned int& maxlimit) {
	while (true) {
		if (LimitedAdd_weak(add, maxlimit)) {
			return;
		}
		std::this_thread::yield();
	}
}
bool tapi_atomic_uint::LimitedSubtract_weak(const unsigned int& subtract, const unsigned int& minlimit) {
	unsigned int x = data.load();
	if (x - subtract < minlimit ||	//Subtraction is bigger
		x - subtract > x				//UINT overflow
		) {
		return false;
	}
	if (!data.compare_exchange_strong(x, x - subtract)) {
		return LimitedSubtract_weak(subtract, minlimit);
	}
	return true;
}
void tapi_atomic_uint::LimitedSubtract_strong(const unsigned int& subtract, const unsigned int& minlimit) {
	while (true) {
		if (LimitedSubtract_weak(subtract, minlimit)) {
			return;
		}
		std::this_thread::yield();
	}
}

