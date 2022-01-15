#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <stdio.h>


struct VK_Extension {
	enum class EXTTYPE : unsigned int {
		UNDEFINED = 0
	};
	EXTTYPE TYPE = EXTTYPE::UNDEFINED;
	void* DATA = nullptr;
};


//Extensions
static bool isActive_SurfaceKHR = false, isSupported_PhysicalDeviceProperties2 = false;

template<typename T>
class threadlocal_vector {
	T** const lists;
	//Element order: thread0-size, thread0-capacity, thread1-size, thread1-capacity...
	unsigned long* const sizes_and_capacities;
	inline void expand_if_necessary(const unsigned int thread_i) {
		if (sizes_and_capacities[(thread_i * 2)] == sizes_and_capacities[(thread_i * 2) + 1]) {
			T* newlist = new T[sizes_and_capacities[thread_i * 2]];
			memcpy(newlist, lists[thread_i], sizeof(T) * sizes_and_capacities[thread_i * 2]);
			delete[] lists[thread_i];
			lists[thread_i] = newlist;
			sizes_and_capacities[(thread_i * 2) + 1] *= 2;
		}
	}
public:
	template <typename T> threadlocal_vector(unsigned long initial_sizes = 0) : lists(new T* [(threadingsys) ? (threadingsys->thread_count()) : 1]), sizes(new unsigned long[(threadingsys) ? (threadingsys->thread_count()) : 1]) {
		if (initial_sizes) {
			for (unsigned long thread_i = 0; thread_i < (threadingsys) ? (threadingsys->thread_count()) : 1; thread_i++) {
				lists[thread_i] = new T[initial_sizes * 2];
				sizes_and_capacities[thread_i * 2] = initial_sizes;
				sizes_and_capacities[(thread_i * 2) + 1] = initial_sizes * 2;
			}
		}
	}
	template <typename T> threadlocal_vector(unsigned long initial_sizes, const T& ref) : lists(new T* [(threadingsys) ? (threadingsys->thread_count()) : 1]), sizes(new unsigned long[(threadingsys) ? (threadingsys->thread_count()) : 1]) {
		if (initial_sizes) {
			for (unsigned int thread_i = 0; thread_i < (threadingsys) ? (threadingsys->thread_count()) : 1; thread_i++) {
				lists[thread_i] = new T[initial_sizes * 2];
				for (unsigned long element_i = 0; element_i < initial_sizes; element_i) {
					lists[thread_i][element_i] = ref;
				}
				sizes_and_capacities[thread_i * 2] = initial_sizes;
				sizes_and_capacities[(thread_i * 2) + 1] = initial_sizes * 2;
			}
		}
	}
	unsigned long size() {
		return sizes_and_capacities[(threadingsys) ? (threadingsys->this_thread_index()) : 0]
	}
	template <typename T> void push_back(const T& ref) {
		const unsigned int thread_i = (threadingsys) ? (threadingsys->this_thread_index()) : 0;
		expand_if_necessary(thread_i);
		lists[thread_i][sizes_and_capacities[thread_i * 2]] = ref;
		sizes_and_capacities[thread_i * 2] += 1;
	}
	template <typename T> T* data() {
		return lists[(threadingsys) ? (threadingsys->this_thread_index()) : 0]
	}
};