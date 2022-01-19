#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <atomic>
#include <string>
#include "predefinitions_vk.h"
#include "tgfx_structs.h"

struct core_public {

};

struct extension_private;
struct extension_public {
	extension_private* hidden = nullptr;
};

struct gpu_private;
struct gpu_public{
private:
	friend struct core_functions; 
	friend struct allocatorsys_vk;
	friend struct queuesys_vk;
	gpu_private* hidden = nullptr;
	gpudescription_tgfx desc;

	VkPhysicalDevice Physical_Device = {};
	VkPhysicalDeviceProperties Device_Properties = {};
	VkPhysicalDeviceMemoryProperties MemoryProperties = {};
	VkQueueFamilyProperties* QueueFamilyProperties; unsigned int QueueFamiliesCount = 0;
	//If GRAPHICS_QUEUEIndex is UINT32_MAX, Graphics isn't supported by the device
	unsigned int TRANSFERs_supportedqueuecount = 0, COMPUTE_supportedqueuecount = 0, GRAPHICS_supportedqueuecount = 0;
	VkDevice Logical_Device = {};
	VkExtensionProperties* Supported_DeviceExtensions; unsigned int Supported_DeviceExtensionsCount = 0;
	std::vector<const char*> Active_DeviceExtensions;
	extension_manager* extensions;
	VkPhysicalDeviceFeatures Supported_Features = {}, Active_Features = {};
	uint32_t* AllQueueFamilies;


	std::vector<memorytype_vk*> ALLOCs;
	//Use SortedQUEUEFAMsLIST to access queue families in increasing feature score order
	queuefam_vk** queuefams;
public:
	inline const char* DEVICENAME() { return desc.NAME.c_str(); }
	inline const unsigned int APIVERSION() { return desc.API_VERSION; }
	inline const unsigned int DRIVERSION() { return desc.DRIVER_VERSION; }
	inline const gpu_type_tgfx DEVICETYPE() { return desc.GPU_TYPE; }
	inline const bool GRAPHICSSUPPORTED() { return GRAPHICS_supportedqueuecount; }
	inline const bool COMPUTESUPPORTED() { return COMPUTE_supportedqueuecount; }
	inline const bool TRANSFERSUPPORTED() { return TRANSFERs_supportedqueuecount; }
	inline VkPhysicalDevice PHYSICALDEVICE() { return Physical_Device; }
	inline const VkPhysicalDeviceProperties& DEVICEPROPERTIES() const { return Device_Properties; }
	inline const VkPhysicalDeviceMemoryProperties& MEMORYPROPERTIES() const { return MemoryProperties; }
	inline VkQueueFamilyProperties* QUEUEFAMILYPROPERTIES() const { return QueueFamilyProperties; }
	inline queuefam_vk** QUEUEFAMS() const { return queuefams; }
	inline const uint32_t* ALLQUEUEFAMILIES() const { return AllQueueFamilies; }
	inline unsigned int QUEUEFAMSCOUNT() const { return QueueFamiliesCount; }
	inline unsigned int TRANSFERSUPPORTEDQUEUECOUNT() { return TRANSFERs_supportedqueuecount; }
	inline VkDevice LOGICALDEVICE() { return Logical_Device; }
	inline VkExtensionProperties* EXTPROPERTIES() { return Supported_DeviceExtensions; }
	inline VkPhysicalDeviceFeatures DEVICEFEATURES_SUPPORTED() { return Supported_Features; }
	inline VkPhysicalDeviceFeatures DEVICEFEATURES_ACTIVE() { return Active_Features; }
	inline const gpudescription_tgfx& DESC() { return desc; }
	inline extension_manager* EXTMANAGER() { return extensions; }

	/*
	//This function searches the best queue that has least specs but needed specs
	//For example: Queue 1->Graphics,Transfer,Compute - Queue 2->Transfer, Compute - Queue 3->Transfer
	//If flag only supports Transfer, then it is Queue 3
	//If flag only supports Compute, then it is Queue 2
	//This is because user probably will use Direct Queues (Graphics,Transfer,Compute supported queue and every GPU has at least one)
	//Users probably gonna leave the Direct Queue when they need async operations (Async compute or Async Transfer)
	//Also if flag doesn't need anything (probably Barrier TP only), returns nullptr
	VK_QUEUEFAM* Find_BestQueue(const VK_QUEUEFLAG& Branch) {
		if (!Flag.isFlagValid()) {
			printer(result_tgfx_FAIL, "(Find_BestQueue() has failed because flag is invalid!");
			return nullptr;
		}
		if (Flag.doesntNeedAnything) {
			return nullptr;
		}
		unsigned char BestScore = 0;
		VK_QUEUEFAM* BestQueue = nullptr;
		for (unsigned char QueueIndex = 0; QueueIndex < QUEUEs.size(); QueueIndex++) {
			VK_QUEUEFAM* Queue = &QUEUEs[QueueIndex];
			if (DoesQueue_Support(Queue, Flag)) {
				if (!BestScore || BestScore > Queue->QueueFeatureScore) {
					BestScore = Queue->QueueFeatureScore;
					BestQueue = Queue;
				}
			}
		}
		return BestQueue;
	}
	bool DoesQueue_Support(const VK_QUEUEFAM* QUEUEFAM, const VK_QUEUEFLAG& FLAG) const {
		const VK_QUEUEFLAG& supportflag = QUEUEFAM->QUEUEFLAG();
		if (Flag.doesntNeedAnything) {
			printer(result_tgfx_FAIL, "(You should handle this type of flag in a special way, don't call this function for this type of flag!");
			return false;
		}
		if (Flag.is_COMPUTEsupported && !supportflag.is_COMPUTEsupported) {
			return false;
		}
		if (Flag.is_GRAPHICSsupported && !supportflag.is_GRAPHICSsupported) {
			return false;
		}
		if (Flag.is_PRESENTATIONsupported && !supportflag.is_PRESENTATIONsupported) {
			return false;
		}
		return true;
	}
	*/
};

struct monitor_vk {
	unsigned int width, height, color_bites, refresh_rate, physical_width, physical_height;
	const char* name;
	//I will fill this structure when I investigate monitor configurations deeper!
};

struct window_vk {
	unsigned int LASTWIDTH, LASTHEIGHT, NEWWIDTH, NEWHEIGHT;
	windowmode_tgfx DISPLAYMODE;
	monitor_vk* MONITOR;
	std::string NAME;
	VkImageUsageFlags SWAPCHAINUSAGE;
	tgfx_windowResizeCallback resize_cb = nullptr;
	void* UserPTR;

	VkSurfaceKHR Window_Surface = {};
	VkSwapchainKHR Window_SwapChain = {};
	GLFWwindow* GLFW_WINDOW = {};
	unsigned char CurrentFrameSWPCHNIndex = 0;
	texture_vk* Swapchain_Textures[2];
	VkSurfaceCapabilitiesKHR SurfaceCapabilities = {};
	std::vector<VkSurfaceFormatKHR> SurfaceFormats;
	std::vector<VkPresentModeKHR> PresentationModes;
	bool isResized = false;
	semaphore_idtype_vk PresentationSemaphores[2];
};

struct initialization_secondstageinfo{
	gpu_public* renderergpu;
	//Global Descriptors Info
	unsigned int MaxMaterialCount, MaxSumMaterial_SampledTexture, MaxSumMaterial_ImageTexture, MaxSumMaterial_UniformBuffer, MaxSumMaterial_StorageBuffer,
		GlobalShaderInput_UniformBufferCount, GlobalShaderInput_StorageBufferCount, GlobalShaderInput_SampledTextureCount, GlobalShaderInput_ImageTextureCount;
	bool shouldActivate_DearIMGUI : 1, isUniformBuffer_Index1 : 1, isSampledTexture_Index1 : 1;
};
struct VK_ShaderStageFlag {
	VkShaderStageFlags flag;
};