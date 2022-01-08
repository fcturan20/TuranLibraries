#pragma once
#define THREADEDJOBSYS_TAPI_PLUGIN_NAME "tapi_threadedjobsys"
#define THREADEDJOBSYS_TAPI_LOAD_TYPE threadingsys_tapi_type*

/*TuranAPI's C Multi-Threading and Job System Library
* If you created threads before, you can pass them to the Threader. Threader won't create any other thread.
* But you shouldn't destroy any of their objects before destroying Threader, otherwise there will be a dangling pointer issue
* This ThreadPool system is also capable of using main thread as one the threads
*/

//Wait handle is just a pointer
typedef struct tapi_wait_handle* wait_handle_tapi;


typedef struct threadingsys_tapi{
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
	wait_handle_tapi (*create_wait)();
	void (*waitjob_busy)(wait_handle_tapi wait);
	void (*waitjob_empty)(wait_handle_tapi wait);
	void (*clear_waitinfo)(wait_handle_tapi wait);
	void (*wait_all_otherjobs)();

	
	//If you want to wait for this job, you can pass a JobWaitInfo
	//But if you won't wait for it, don't use it because there may be crashes because of dangling wait reference
	//Critical Note : You shouldn't use same JobWaitInfo object across Execute_withWait()s without ClearJobWait()!
	void (*execute_withwait)(void (*func)(), wait_handle_tapi wait);
	void (*execute_withoutwait)(void (*func)());
	//Add this thread to the threading system in a job search state
	//This function doesn't return; either thread'll die or threading system'll close.
	void (*execute_otherjobs)();

	unsigned int (*this_thread_index)();
	unsigned int (*thread_count)();
}	threadingsys_tapi;

typedef struct threadingsys_tapi_d threadingsys_tapi_d;
typedef struct threadingsys_tapi_type{
	threadingsys_tapi_d* data;
	threadingsys_tapi* funcs;
} threadingsys_tapi_type;
