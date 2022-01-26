#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include "predefinitions_vk.h"
#include <threadingsys_tapi.h>
#include <iostream>
#include <algorithm>
#include <mutex>

//Some algorithms and data structures to help in C++ (like threadlocalvector)

template<class T>
class threadlocal_vector {
	T** lists;
	std::mutex Sync;
	//Element order: thread0-size, thread0-capacity, thread1-size, thread1-capacity...
	unsigned long* sizes_and_capacities;
	inline void expand_if_necessary(const unsigned int thread_i) {
		if (sizes_and_capacities[(thread_i * 2)] == sizes_and_capacities[(thread_i * 2) + 1]) {
			T* newlist = new T[std::max((unsigned long)1, sizes_and_capacities[thread_i * 2])];
			memcpy(newlist, lists[thread_i], sizeof(T) * sizes_and_capacities[thread_i * 2]);
			delete[] lists[thread_i];
			lists[thread_i] = newlist;
			sizes_and_capacities[(thread_i * 2) + 1] = std::max((unsigned long)1, sizes_and_capacities[(thread_i * 2) + 1]);
			sizes_and_capacities[(thread_i * 2) + 1] *= 2;
		}
	}
public:
	threadlocal_vector(const threadlocal_vector& copy) {
		unsigned int threadcount = (threadingsys) ? (threadingsys->thread_count()) : 1;
		lists = new T * [threadcount];
		sizes_and_capacities = new unsigned long[(threadingsys) ? (threadingsys->thread_count() * 2) : 2];
		for (unsigned int thread_i = 0; thread_i < ((threadingsys) ? (threadingsys->thread_count()) : 1); thread_i++) {
			lists[thread_i] = new T[copy.sizes_and_capacities[(thread_i * 2) + 1]];
			for (unsigned int element_i = 0; element_i < copy.sizes_and_capacities[thread_i * 2]; element_i++) {
				lists[thread_i][element_i] = copy.lists[thread_i][element_i];
			}
			sizes_and_capacities[thread_i * 2] = copy.sizes_and_capacities[thread_i * 2];
			sizes_and_capacities[(thread_i * 2) + 1] = copy.sizes_and_capacities[(thread_i * 2) + 1];
		}
	}
	//This constructor allocates initial_sizes * 2 memory but doesn't add any element to it, so you should use push_back()
	threadlocal_vector(unsigned long initial_sizes) {
		unsigned int threadcount = (threadingsys) ? (threadingsys->thread_count()) : 1;
		lists = new T * [threadcount];
		sizes_and_capacities = new unsigned long[(threadingsys) ? (threadingsys->thread_count() * 2) : 2];
		if (initial_sizes) {
			for (unsigned int thread_i = 0; thread_i < ((threadingsys) ? (threadingsys->thread_count()) : 1); thread_i++) {
				lists[thread_i] = new T[initial_sizes * 2];
				sizes_and_capacities[thread_i * 2] = 0;
				sizes_and_capacities[(thread_i * 2) + 1] = initial_sizes * 2;
			}
		}
	}
	//This constructor allocates initial_sizes * 2 memory and fills it with initial_sizes number of ref objects
	threadlocal_vector(unsigned long initial_sizes, const T& ref) {
		unsigned int threadcount = (threadingsys) ? (threadingsys->thread_count()) : 1;
		lists = new T * [threadcount];
		sizes_and_capacities = new unsigned long[(threadingsys) ? (threadingsys->thread_count() * 2) : 2];
		if (initial_sizes) {
			for (unsigned int thread_i = 0; thread_i < ((threadingsys) ? (threadingsys->thread_count()) : 1); thread_i++) {
				lists[thread_i] = new T[initial_sizes * 2];
				for (unsigned long element_i = 0; element_i < initial_sizes; element_i++) {
					lists[thread_i][element_i] = ref;
				}
				sizes_and_capacities[thread_i * 2] = initial_sizes;
				sizes_and_capacities[(thread_i * 2) + 1] = initial_sizes * 2;
			}
		}
	}
	unsigned long size(unsigned int ThreadIndex = UINT32_MAX) {
		return sizes_and_capacities[(threadingsys) ? (threadingsys->this_thread_index() * 2) : 0];
	}
	void push_back(const T& ref, unsigned int ThreadIndex = UINT32_MAX) {
		const unsigned int thread_i = (ThreadIndex == UINT32_MAX) ? (ThreadIndex) : ((threadingsys) ? (threadingsys->this_thread_index()) : 0);
		expand_if_necessary(thread_i);
		lists[thread_i][sizes_and_capacities[thread_i * 2]] = ref;
		sizes_and_capacities[thread_i * 2] += 1;
	}
	T* data(unsigned int ThreadIndex = UINT32_MAX) {
		return lists[(ThreadIndex != UINT32_MAX) ? (ThreadIndex) : ((threadingsys) ? (threadingsys->this_thread_index()) : 0)];
	}
	void clear(unsigned int ThreadIndex = UINT32_MAX) {
		sizes_and_capacities[(ThreadIndex != UINT32_MAX) ? (ThreadIndex * 2) : ((threadingsys) ? (threadingsys->this_thread_index() * 2) : 0)] = 0;
	}
	T& get(unsigned int ThreadIndex, unsigned int ElementIndex) {
		return lists[ThreadIndex][ElementIndex];
	}
	T& operator[](unsigned int ElementIndex) {
		return lists[((threadingsys) ? (threadingsys->this_thread_index()) : 0)][ElementIndex];
	}
	void PauseAllOperations(std::unique_lock<std::mutex>& Locker) {
		Locker = std::unique_lock<std::mutex>(Sync);
	}
};

template<typename T>
class vk_atomic {
	std::atomic<T> data;
public:
	//Returns the old value
	uint64_t DirectAdd(const uint64_t& add) {
		return data.fetch_add(add);
	}
	//Returns the old value
	uint64_t DirectSubtract(const uint64_t& sub) {
		return data.fetch_sub(sub);
	}
	void DirectStore(const uint64_t& Store) {
		data.store(Store);
	}
	uint64_t DirectLoad() const {
		return data.load();
	}

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
	bool LimitedAdd_weak(const uint64_t& add, const uint64_t& maxlimit) {
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
	//You should use this function only for this case, because this is not performant;
	//The value is very close to the limit so you are gonna reduce the value some nanoseconds later from other threads
	//And when the value is reduced, you want addition to happen immediately
	//"Immediately" here means after this_thread::yield()
	//If you pay attention, this function doesn't return bool. Because this will block the thread until addition is possible
	//So 
	void LimitedAdd_strong(const uint64_t& add, const uint64_t& maxlimit) {
		while (true) {
			if (LimitedAdd_weak(add, maxlimit)) {
				return;
			}
			std::this_thread::yield();
		}
	}
	//Similar but the reverse of the LimitedAdd_weak()
	bool LimitedSubtract_weak(const uint64_t& subtract, const uint64_t& minlimit) {
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
	//Similar but the reverse of the LimitedAdd_strong()
	void LimitedSubtract_strong(const uint64_t& subtract, const uint64_t& minlimit) {
		while (true) {
			if (LimitedSubtract_weak(subtract, minlimit)) {
				return;
			}
			std::this_thread::yield();
		}
	}
};