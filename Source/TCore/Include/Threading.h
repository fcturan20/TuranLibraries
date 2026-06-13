#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define THREADINGSYS_TAPI_PLUGIN_NAME "tlJob"
#define THREADINGSYS_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)
#define THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE const struct tlJob*

/* Multi-Threaded Job System
 * If you created threads before, you can pass them to the Threader. Threader won't create any other
 * thread. But you shouldn't destroy any of their objects before destroying Threader, otherwise
 * there will be a dangling pointer issue This ThreadPool system is also capable of using main
 * thread as one the threads
 */

struct tlJob {
  const struct tlJobPriv* d;
  /*Wait a job; First the thread yields. If it should wait again, executes any other job while
  waiting
  * Be careful of nonwait-in-wait situations such as;
  You wait for Job B (Thread2) in Job A (Thread1). Job B takes too long, so Thread1 runs Job C here
  while Job B is running. Job C calls Job D and waits for it, so Job D is executed (it doesn't have
  to start immediately, but in worst case it starts) (Thread3 or Thread1) Job D depends on some data
  that Job B is working on but you forgot to call wait for Job B in Job D. So Job D and Job B works
  concurrently, which are working on the same data.
  * To handle this case, you should either;
  1) Create-store your JobWaitInfos in a shared context (this requires ClearJobWait() operation and
  planning all your jobs beforehand) 2) Use WaitJob_empty in Job A, so thread'll keep yielding till
  job finishes
  */
  struct tlSemaphore* (*createSemaphore)();

  void (*execute)(void (*func)(), unsigned int signalSemaphoreCount,
                  struct tlSemaphore** semaphores);
  // Thread joins into job system
  // This function will return only when job system destroyed
  void (*joinThread)();

  void (*unsignalSemaphore)(struct tlSemaphore* semaphore);
  // Won't execute any available job while waiting for the signal
  void (*waitSemaphoreFree)(struct tlSemaphore* semaphore);
  // Will execute available jobs while waiting for the signal
  void (*waitSemaphoreBusy)(struct tlSemaphore* semaphore);
  void (*waitAllJobs)();

  unsigned int (*thisThreadIndx)();
  unsigned int (*threadCount)();
};

#ifdef __cplusplus
}
#endif