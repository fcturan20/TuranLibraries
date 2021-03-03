#include "ThreadedJobCore.h"
#include <functional>

namespace TuranAPI {
	namespace Threading {
		JobWaitInfo::JobWaitInfo() {
			RunInfo.store(0);
		}


		template<class T, unsigned int size>
		class TURANAPI ThreadSafeRingBuffer {
			std::mutex WaitData;
			std::condition_variable strongWait;
			unsigned int head = 0, tail = 0;
			T data[size];
		public:
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
					std::this_thread::yield();
				}
			}
			template<class T>
			bool pop_front_weak(T& item) {
				std::unique_lock<std::mutex> WaitLock(WaitData);
			}
			template<class T>
			void pop_front_strong(T& item) {
				std::unique_lock<std::mutex> WaitLock(WaitData);

			}
		};
#define JobDataConverter(JobDataHandle)     static_cast<TSJobifiedRingBuffer<std::function<void()>, 256>*>(JobDataHandle)
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


		void JobSystem::JobSearch() {
			//ShouldClose will be true if JobSystem destructor is called
			//Destructor will wait for all other jobs to finish
			while (!ShouldClose.load()) {
				std::function<void()> Job;
				JobDataConverter(Jobs)->pop_front_strong(Job);
				Job();
			}
			std::cout << "Thread " << to_string(unsigned int(GetThisThreadIndex())) << " is exited from Job System!\n";
		}
		JobSystem::JobSystem(const std::function<void()>& Main, JobSystem*& PointertoSet) {
			PointertoSet = this;
			ShouldClose.store(false);
			Jobs = new TSJobifiedRingBuffer<std::function<void()>, 256>;
			JobDataConverter(Jobs)->push_back_weak(Main);

			ThreadIDs.insert(std::pair<std::thread::id, unsigned char>(std::this_thread::get_id(), 0));
			unsigned int ThreadCount = std::thread::hardware_concurrency();
			if (ThreadCount == 1) {
				std::cout << "Job system didn't create any std::thread objects because your CPU only has 1 core!\n";
			}
			else {
				ThreadCount--;
			}
			for (unsigned int ThreadIndex = 0; ThreadIndex < ThreadCount; ThreadIndex++) {
				std::thread newthread([this, ThreadIndex]() {ThreadIDs.insert(std::pair<std::thread::id, unsigned char>(std::this_thread::get_id(), ThreadIndex + 1)); this->JobSearch(); });
				newthread.detach();
			}

			JobSearch();
		}
		
		void JobSystem::Execute_withWait(const std::function<void()>& func, JobWaitInfo& wait) {
			//That means job list is full, wake up a thread (if there is anyone sleeping) and yield current thread!
			JobDataConverter(Jobs)->push_back_strong([&wait, func]() {
				wait.RunInfo.store(1);
				func();
				wait.RunInfo.store(2);
				}
			);
		}
		void JobSystem::Execute_withoutWait(const std::function<void()>& func) {
			JobDataConverter(Jobs)->push_back_strong(func);
		}
		void JobSystem::WaitJob_busy(const JobWaitInfo& wait) {
			if (wait.RunInfo.load() != 2) {
				std::this_thread::yield();
			}
			while (wait.RunInfo.load() != 2) {
				std::function<void()> Job;
				JobDataConverter(Jobs)->pop_front_strong(Job);
				Job();
			}
		}
		void JobSystem::WaitJob_empty(const JobWaitInfo& wait) {
			while (wait.RunInfo.load() != 2) {
				std::this_thread::yield();
			}
		}
		void JobSystem::ClearWaitInfo(JobWaitInfo& wait) {
			wait.RunInfo.store(0);
		}
		void JobSystem::WaitForAllOtherJobs() {
			while (!JobDataConverter(Jobs)->isEmpty()) {
				std::function<void()> Job;
				JobDataConverter(Jobs)->pop_front_strong(Job);
				Job();
			}
		}

		unsigned char JobSystem::GetThisThreadIndex() {
			return ThreadIDs[std::this_thread::get_id()];
		}
		unsigned char JobSystem::GetThreadCount() {
			return std::thread::hardware_concurrency();
		}
		JobSystem::~JobSystem() {
			std::cout << "Job system is closed!\n";
		}
		void JobSystem::CloseJobSystem() {
			ShouldClose.store(true);
			WaitForAllOtherJobs();
		}



		AtomicUINT::AtomicUINT(uint64_t Ref) : data(Ref) {
		}
		AtomicUINT::AtomicUINT() : data(0){

		}
		uint64_t AtomicUINT::DirectAdd(const uint64_t& add) {
			return data.fetch_add(add);
		}
		uint64_t AtomicUINT::DirectSubtract(const uint64_t& sub) {
			return data.fetch_sub(sub);
		}
		void AtomicUINT::DirectStore(const uint64_t& Store) {
			data.store(Store);
		}
		uint64_t AtomicUINT::DirectLoad() const {
			return data.load();
		}
		bool AtomicUINT::LimitedAdd_weak(const uint64_t& add, const uint64_t& maxlimit) {
			uint64_t x = data.load();
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
		void AtomicUINT::LimitedAdd_strong(const uint64_t& add, const uint64_t& maxlimit) {
			while (true) {
				if (LimitedAdd_weak(add, maxlimit)) {
					return;
				}
				std::this_thread::yield();
			}
		}
		bool AtomicUINT::LimitedSubtract_weak(const uint64_t& subtract, const uint64_t& minlimit) {
			uint64_t x = data.load();
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
		void AtomicUINT::LimitedSubtract_strong(const uint64_t& subtract, const uint64_t& minlimit) {
			while (true) {
				if (LimitedSubtract_weak(subtract, minlimit)) {
					return;
				}
				std::this_thread::yield();
			}
		}
	}
}