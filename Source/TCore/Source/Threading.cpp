#include "threadingsys_tapi.h"

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

#include "ecs_tapi.h"

inline std::atomic_bool& WaitHandleConverter(struct tlSemaphore* handle) {
  return *( std::atomic_bool* )(handle);
}

struct tlJobPriv* JOBSYS = nullptr;
struct tlJobPriv {
  tlJob*            sysPtr = nullptr;

 private:
  // This class has to wait as std::conditional_variable() if there is no job when
  // pop_front_strong() is called Because it contains jobs and shouldn't look for new jobs while
  // already waiting for a job
  class TSJobifiedRingBuffer {
    std::mutex              WaitData;
    std::condition_variable NewJobWaitCond;
    std::function<void()>   data[256];
    unsigned int            head = 0, tail = 0;

   public:
    std::atomic_uint64_t IsThreadBusy = 0;
    bool                 isEmpty() {
      WaitData.lock();
      bool result = head == tail;
      WaitData.unlock();
      return result;
    }
    bool push_back_weak(const std::function<void()>& item) {
      WaitData.lock();
      if ((head + 1) % 256 != tail) {
        data[head] = [this, item]() {
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
  std::atomic<unsigned char>               ShouldClose;
  TSJobifiedRingBuffer                     Jobs;

  static void tlExecute(void (*func)(), unsigned int semaphoreCount,
                        struct tlSemaphore** semaphores) {
    // That means job list is full, wake up a thread (if there is anyone sleeping) and yield current
    // thread!
    JOBSYS->Jobs.push_back_strong([semaphoreCount, &semaphores, func]() {
      for (uint32_t i = 0; i < semaphoreCount; i++) {
        WaitHandleConverter(semaphores[i]).store(1);
      }
      func();
      for (uint32_t i = 0; i < semaphoreCount; i++) {
        WaitHandleConverter(semaphores[i]).store(2);
      }
    });
  }

  static struct tlSemaphore* tlCreateSemaphore() {
    return ( struct tlSemaphore* )new std::atomic_bool;
  }

  static void tlWaitSemaphoreBusy(struct tlSemaphore* wait) {
    if (WaitHandleConverter(wait).load() != 2) {
      std::this_thread::yield();
    }
    while (WaitHandleConverter(wait).load() != 2) {
      std::function<void()> Job;
      JOBSYS->Jobs.pop_front_strong(Job);
      Job();
    }
  }
  static void tlWaitSemaphoreFree(struct tlSemaphore* wait) {
    while (WaitHandleConverter(wait).load() != 2) {
      std::this_thread::yield();
    }
  }
  static void tlUnsignalSemaphore(struct tlSemaphore* wait) { WaitHandleConverter(wait).store(0); }
  static void tlWaitAllJobs() {
    while (!JOBSYS->Jobs.isEmpty()) {
      std::function<void()> Job;
      JOBSYS->Jobs.pop_front_strong(Job);
      Job();
    }
    while (JOBSYS->Jobs.IsThreadBusy.load()) {
      std::function<void()> Job;
      if (JOBSYS->Jobs.pop_front_weak(Job)) {
        Job();
      } else {
        std::this_thread::yield();
      }
    }
  }
  static unsigned int tlThisThreadIndx() { return JOBSYS->ThreadIDs[std::this_thread::get_id()]; }
  static unsigned int tlThreadCount() { return std::thread::hardware_concurrency(); }
  static void         tlJoinThread() {
    // ShouldClose will be true if JobSystem destructor is called
    // Destructor will wait for all other jobs to finish
    while (!JOBSYS->ShouldClose.load()) {
      std::function<void()> Job;
      JOBSYS->Jobs.pop_front_strong(Job);
      Job();
    }
  }
};

void tlJobInit() {
  JOBSYS->ShouldClose.store(false);

  JOBSYS->ThreadIDs.insert(
    std::pair<std::thread::id, unsigned char>(std::this_thread::get_id(), 0));
  unsigned int ThreadCount = std::thread::hardware_concurrency();
  if (ThreadCount == 1) {
    printf("Job system didn't create any std::thread objects because your CPU only has 1 core!\n");
  } else {
    ThreadCount--;
  }
  for (uint32_t ThreadIndex = 0; ThreadIndex < ThreadCount; ThreadIndex++) {
    std::thread newthread([ThreadIndex]() {
      JOBSYS->ThreadIDs.insert(
        std::pair<std::thread::id, unsigned char>(std::this_thread::get_id(), ThreadIndex + 1));
      tlJobPriv::tlJoinThread();
    });
    newthread.detach();
  }
}

ECSPLUGIN_ENTRY(ecssys, reloadFlag) {

  tlJob*     type = ( tlJob* )malloc(sizeof(tlJob));
  JOBSYS = new tlJobPriv;
  type->d         = JOBSYS;
  JOBSYS->sysPtr    = type;

  type->unsignalSemaphore = &tlJobPriv::tlUnsignalSemaphore;
  type->createSemaphore   = &tlJobPriv::tlCreateSemaphore;
  type->joinThread        = &tlJobPriv::tlJoinThread;
  type->execute           = &tlJobPriv::tlExecute;
  type->thisThreadIndx    = &tlJobPriv::tlThisThreadIndx;
  type->threadCount       = &tlJobPriv::tlThreadCount;
  type->waitAllJobs       = &tlJobPriv::tlWaitAllJobs;
  type->waitSemaphoreBusy = &tlJobPriv::tlWaitSemaphoreBusy;
  type->waitSemaphoreFree = &tlJobPriv::tlWaitSemaphoreFree;

  ecssys->addSystem(THREADINGSYS_TAPI_PLUGIN_NAME, THREADINGSYS_TAPI_PLUGIN_VERSION, type);

  tlJobInit();
}
ECSPLUGIN_EXIT(ecssys, reloadFlag) { printf("Job system exit isn't coded"); }