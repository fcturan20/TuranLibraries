#pragma once
#include "API_includes.h"
#include "Logger_Core.h"
#include "Profiler_Core.h"
#include <functional>
#include <mutex>
#include <atomic>

/*TuranAPI's Multi-Threading and Job System Library
* If you created threads before, you can pass them to the Threader. Threader won't create any other thread.
* But you shouldn't destroy any of their objects before destroying Threader, otherwise there will be a dangling pointer issue
* This ThreadPool system is also capable of using main thread as one the threads
*/

namespace TuranAPI {
	namespace Threading {
		struct JobWaitInfo {
			std::atomic_char RunInfo;
			JobWaitInfo();
		};
		
		
		class TURANAPI JobSystem {
			std::map<std::thread::id, unsigned char> ThreadIDs;
			std::atomic_bool ShouldClose;
			void* Jobs;

			void JobSearch();
		public:
			unsigned char GetThisThreadIndex();
			unsigned char GetThreadCount();
			JobSystem(const std::function<void()>& Main, JobSystem*& PointertoSet);
			//If you want to wait for this job, you can pass a JobWaitInfo
			//But if you won't wait for it, don't use it because there may be crashes because of dangling wait reference
			//Critical Note : You shouldn't use same JobWaitInfo object across Execute_withWait()s without ClearJobWait()!
			void Execute_withWait(const std::function<void()>& func, JobWaitInfo& wait);
			void Execute_withoutWait(const std::function<void()>& func);
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
			void WaitJob_busy(const JobWaitInfo& wait);
			void WaitJob_empty(const JobWaitInfo& wait);
			void ClearWaitInfo(JobWaitInfo& wait);
			void WaitForAllOtherJobs();
			//If you want to close job system, you should call this instead of Desctructor!
			//This function may run jobs if there is any when it is called
			//So that means, destruction of the job system is the responsibility of the user to handle their jobs
			//I mean, you probably should synchronize all your jobs at one point and check if you should close the job system
			//If you call this while running some other jobs in other threads and they don't care if destructor is called
			//Application may run forever 
			void CloseJobSystem();
			~JobSystem();
		};

		//This class includes xxx_strong() functions which means they are doing jobs instead of waiting
		template<class T, unsigned int size>
		class TURANAPI TSJobifiedRingBuffer {
			std::mutex WaitData;
			T data[size];
			unsigned int head = 0, tail = 0;
			JobSystem& JobSys;
		public:
			TSJobifiedRingBuffer(JobSystem& System) : JobSys(System){
				
			}
			template<class T>
			bool push_back_weak(const T& item) {
				std::unique_lock<std::mutex> WaitLock(WaitData);
				if ((head + 1) % size != tail) {
					head = (head + 1) % size;
					data[head] = item;
					return true;
				}

				return false;
			}
			template<class T>
			void push_back_strong(const T& item) {
				while (!push_back_weak(item)) {
					JobSys.JobSearch();
				}
			}
			template<class T>
			bool pop_front_weak(T& item) {
				std::unique_lock<std::mutex> WaitLock(WaitData);
			}
			template<class T>
			void pop_front_strong(T& item) {
				if (!pop_front_weak(item)) {
					JobSys.JobSearch();
				}
			}
		};


		class TURANAPI AtomicUINT {
			std::atomic<uint64_t> data;
		public:
			AtomicUINT(uint64_t Ref);
			AtomicUINT();

			//Returns the old value
			uint64_t DirectAdd(const uint64_t& add);
			//Returns the old value
			uint64_t DirectSubtract(const uint64_t& sub);
			void DirectStore(const uint64_t& Store);
			uint64_t DirectLoad() const;

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
			bool LimitedAdd_weak(const uint64_t& add, const uint64_t& maxlimit);
			//You should use this function only for this case, because this is not performant;
			//The value is very close to the limit so you are gonna reduce the value some nanoseconds later from other threads
			//And when the value is reduced, you want addition to happen immediately
			//"Immediately" here means after this_thread::yield()
			//If you pay attention, this function doesn't return bool. Because this will block the thread until addition is possible
			//So 
			void LimitedAdd_strong(const uint64_t& add, const uint64_t& maxlimit);
			//Similar but the reverse of the LimitedAdd_weak()
			bool LimitedSubtract_weak(const uint64_t& subtract, const uint64_t& minlimit);
			//Similar but the reverse of the LimitedAdd_strong()
			void LimitedSubtract_strong(const uint64_t& subtract, const uint64_t& minlimit);
		};

		//Thread Local std::vector implementation
		//Functions with "unsigned char ThreadID" doesn't use any sync primitive
		//Functions with "unsigned char& ThreadID" uses mutex as sync primitive
		//Don't use PauseAllOperations with functions that uses mutex as sync primitive (Because it causes deadlock)
		template<class T>
		class TURANAPI TLVector {
			std::vector<T>* PerThreadVectors;
			unsigned char ThreadCount = 0;
			std::mutex Sync;
		public:
			TLVector(JobSystem& JobSys) {
				PerThreadVectors = new std::vector<T>[JobSys.GetThreadCount()];
				ThreadCount = JobSys.GetThreadCount();
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
			~TLVector() {
				delete[] PerThreadVectors;
			}
		};
	}
}