#include "synchronization_sys.h"


extern void Create_SyncSystems() {
	fencesys = new fencesys_vk;
	semaphoresys = new semaphoresys_vk;
}