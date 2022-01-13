#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>



struct VK_Extension {
	enum class EXTTYPE : unsigned int {
		UNDEFINED = 0
	};
	EXTTYPE TYPE = EXTTYPE::UNDEFINED;
	void* DATA = nullptr;
};


//Extensions
static bool isActive_SurfaceKHR = false, isSupported_PhysicalDeviceProperties2 = false;


