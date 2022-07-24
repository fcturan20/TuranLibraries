#include "vk_synchronization.h"


extern void Create_SyncSystems() {
	fencesys = new fencesys_vk;
	semaphoresys = new semaphoresys_vk;
}