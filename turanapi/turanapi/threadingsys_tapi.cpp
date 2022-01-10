#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <map>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

#include "threadingsys_tapi.h"
#include "registrysys_tapi.h"



class JobSystem {
	//This class has to wait as std::conditional_variable() if there is no job when pop_front_strong() is called
	//Because it contains jobs and shouldn't look for new jobs while already waiting for a job
	class TSJobifiedRingBuffer {
		std::mutex WaitData;
		std::condition_variable NewJobWaitCond;
		std::function<void()> data[256];
		unsigned int head = 0, tail = 0;
	public:
		std::atomic_uint64_t IsThreadBusy = 0;
		bool isEmpty() {
			WaitData.lock();
			bool result = head == tail;
			WaitData.unlock();
			return result;
		}
		bool push_back_weak(const std::function<void()>& item) {
			WaitData.lock();
			if ((head + 1) % 256 != tail) {
				data[head] = [this, item](){
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
	public:
	std::map<std::thread::id, unsigned char> ThreadIDs;
	std::atomic<unsigned char> ShouldClose;
	TSJobifiedRingBuffer Jobs;
};
static JobSystem* JOBSYS = nullptr;

void JobSearch(){
	//ShouldClose will be true if JobSystem destructor is called
	//Destructor will wait for all other jobs to finish
	while (!JOBSYS->ShouldClose.load()) {
		std::function<void()> Job;
		JOBSYS->Jobs.pop_front_strong(Job);
		Job();
	}
}
inline std::atomic_bool& WaitHandleConverter(wait_handle_tapi handle) { return *(std::atomic_bool*)(handle); } 


void Execute_withWait(void(*func)(), wait_handle_tapi wait) {
	//That means job list is full, wake up a thread (if there is anyone sleeping) and yield current thread!
	JOBSYS->Jobs.push_back_strong([&wait, func]() {
		WaitHandleConverter(wait).store(1);
		func();
		WaitHandleConverter(wait).store(2);
		}
	);
} 
void Execute_withoutWait(void(*func)()) {
	JOBSYS->Jobs.push_back_strong(func);
}

wait_handle_tapi Create_Wait() {
	return (wait_handle_tapi)new std::atomic_bool;
}
void waitJob_busy(wait_handle_tapi wait) {
	if (WaitHandleConverter(wait).load() != 2) {
		std::this_thread::yield();
	}
	while (WaitHandleConverter(wait).load() != 2) {
		std::function<void()> Job;
		JOBSYS->Jobs.pop_front_strong(Job);
		Job();
	}
}
void waitJob_empty(wait_handle_tapi wait) {
	while (WaitHandleConverter(wait).load() != 2) {
		std::this_thread::yield();
	}
}
void ClearWaitInfo(wait_handle_tapi wait) {
	WaitHandleConverter(wait).store(0);
}
void waitForAllOtherJobs() {
	while (!JOBSYS->Jobs.isEmpty()) {
		std::function<void()> Job;
		JOBSYS->Jobs.pop_front_strong(Job);
		Job();
	}
	while(JOBSYS->Jobs.IsThreadBusy.load()){
		std::function<void()> Job;
		if(JOBSYS->Jobs.pop_front_weak(Job)){
			Job();
		}
		else{
			std::this_thread::yield();
		}
	}
}
unsigned int GetThisThreadIndex() {
	return JOBSYS->ThreadIDs[std::this_thread::get_id()];
}
unsigned int GetThreadCount() {
	return std::thread::hardware_concurrency();
}









void load_in_cpp(){
	JOBSYS = new JobSystem;
	JOBSYS->ShouldClose.store(false);

	JOBSYS->ThreadIDs.insert(std::pair<std::thread::id, unsigned char>(std::this_thread::get_id(), 0));
	unsigned int ThreadCount = std::thread::hardware_concurrency();
	if (ThreadCount == 1) {
		printf("Job system didn't create any std::thread objects because your CPU only has 1 core!\n");
	}
	else {
		ThreadCount--;
	}
	for (unsigned int ThreadIndex = 0; ThreadIndex < ThreadCount; ThreadIndex++) {
		std::thread newthread([ThreadIndex]() {JOBSYS->ThreadIDs.insert(std::pair<std::thread::id, unsigned char>(std::this_thread::get_id(), ThreadIndex + 1)); JobSearch(); });
		newthread.detach();
	}
}

typedef struct threadingsys_tapi_d{
	JobSystem* JOBSYS = nullptr;
	threadingsys_tapi_type* plugin_type_ptr = nullptr;
} threadingsys_tapi_d;
extern "C" FUNC_DLIB_EXPORT void* load_plugin(registrysys_tapi* reg_sys, unsigned char reload){
	load_in_cpp();

	threadingsys_tapi_type* type = (threadingsys_tapi_type*)malloc(sizeof(threadingsys_tapi_type));
	type->data = (threadingsys_tapi_d*)malloc(sizeof(threadingsys_tapi_d));
	type->funcs = (threadingsys_tapi*)malloc(sizeof(threadingsys_tapi));
	type->data->JOBSYS = JOBSYS;
	type->data->plugin_type_ptr = type;

	type->funcs->clear_waitinfo = &ClearWaitInfo;
	type->funcs->create_wait = &Create_Wait;
	type->funcs->execute_otherjobs = &JobSearch;
	type->funcs->execute_withoutwait = &Execute_withoutWait;
	type->funcs->execute_withwait = &Execute_withWait;
	type->funcs->this_thread_index = &GetThisThreadIndex;
	type->funcs->thread_count = &GetThreadCount;
	type->funcs->wait_all_otherjobs = &waitForAllOtherJobs;
	type->funcs->waitjob_busy = &waitJob_busy;
	type->funcs->waitjob_empty = &waitJob_empty;
	
	reg_sys->add(THREADINGSYS_TAPI_PLUGIN_NAME, THREADINGSYS_TAPI_PLUGIN_VERSION, type);

	return type;
}
extern "C" FUNC_DLIB_EXPORT void unload_plugin(registrysys_tapi* reg_sys, unsigned char reload){
	printf("qwe");
}
/*







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
*/