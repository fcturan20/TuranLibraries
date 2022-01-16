#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <atomic>
#include <string>
#include "predefinitions_vk.h"
#include "tgfx_structs.h"

struct core_public {

};


struct gpu_private;
struct gpu_public{
private:
	friend struct core_functions; 
	friend struct allocatorsys_privatefuncs;
	friend struct queuesys_vk;
	gpu_private* hidden = nullptr;
	gpudescription_tgfx desc;
	//Initializes as everything is false (same as CreateInvalidNullFlag)
	struct VK_QUEUEFLAG {
		bool is_GRAPHICSsupported : 1;
		bool is_PRESENTATIONsupported : 1;
		bool is_COMPUTEsupported : 1;
		bool is_TRANSFERsupported : 1;
		bool doesntNeedAnything : 1;	//This is a special flag to be used as "Don't care other parameters, this is a special operation"
		VK_QUEUEFLAG();
		inline static VK_QUEUEFLAG CreateInvalidNullFlag();	//Returned flag's every bit is false. You should set at least one of them as true.
		inline bool isFlagValid() const;
		//bool is_VTMEMsupported : 1;	Not supported for now!
	};
	struct VK_QUEUE {
		void Initialize(VkQueue QUEUE, float PRIORITY);
		~VK_QUEUE();
	private:
		fence_idtype_vk RenderGraphFences[2];
		float Priority = 0.0f;
		VkQueue Queue = VK_NULL_HANDLE;
	};
	struct VK_QUEUEFAM {
	private:
		VK_QUEUEFLAG SupportFlag;
		VK_QUEUE* QUEUEs;
		unsigned int QueueCount;
		uint32_t QueueFamilyIndex;
		unsigned char QueueFeatureScore = 0;
	public:
		inline void Initialize(const VkQueueFamilyProperties& FamProps, unsigned int QueueFamIndex, const std::vector<VkQueue>& Queues);
		inline unsigned char QUEUEFEATURESCORE() const { return QueueFeatureScore; }
		inline VK_QUEUEFLAG QUEUEFLAG() const { return SupportFlag; }
		inline unsigned int QUEUECOUNT() const { return QueueCount; }
		inline uint32_t QUEUEFAMINDEX() const { return QueueFamilyIndex; }
		inline void SUPPORT_PRESENTATION() { if (!SupportFlag.is_PRESENTATIONsupported) { SupportFlag.is_PRESENTATIONsupported = true; QueueFeatureScore++; } }
		~VK_QUEUEFAM();
	};


	std::string NAME;
	unsigned int APIVER = UINT32_MAX, DRIVERVER = UINT32_MAX;
	gpu_type_tgfx GPUTYPE;

	VkPhysicalDevice Physical_Device = {};
	VkPhysicalDeviceProperties Device_Properties = {};
	VkPhysicalDeviceMemoryProperties MemoryProperties = {};
	VkQueueFamilyProperties* QueueFamilyProperties; unsigned int QueueFamiliesCount = 0;
	//Use SortedQUEUEFAMsLIST to access queue families in increasing feature score order
	VK_QUEUEFAM* QUEUEFAMs; unsigned int* SortedQUEUEFAMsLIST;
	//If GRAPHICS_QUEUEIndex is UINT32_MAX, Graphics isn't supported by the device
	unsigned int TRANSFERs_supportedqueuecount = 0, COMPUTE_supportedqueuecount = 0, GRAPHICS_QUEUEFamIndex = UINT32_MAX;
	VkDevice Logical_Device = {};
	VkExtensionProperties* Supported_DeviceExtensions; unsigned int Supported_DeviceExtensionsCount = 0;
	std::vector<const char*> Active_DeviceExtensions;
	VkPhysicalDeviceFeatures Supported_Features = {}, Active_Features = {};

	uint32_t* AllQueueFamilies;

	memorytype_vk* memorytypes_vk;
public:
	inline const std::string DEVICENAME() { return NAME; }
	inline const unsigned int APIVERSION() { return APIVER; }
	inline const unsigned int DRIVERSION() { return DRIVERVER; }
	inline const gpu_type_tgfx DEVICETYPE() { return GPUTYPE; }
	inline const bool GRAPHICSSUPPORTED() { return (GRAPHICS_QUEUEFamIndex == UINT32_MAX) ? (false) : (true); }
	inline const bool COMPUTESUPPORTED() { return COMPUTE_supportedqueuecount; }
	inline const bool TRANSFERSUPPORTED() { return TRANSFERs_supportedqueuecount; }
	inline VkPhysicalDevice PHYSICALDEVICE() { return Physical_Device; }
	inline const VkPhysicalDeviceProperties& DEVICEPROPERTIES() const { return Device_Properties; }
	inline const VkPhysicalDeviceMemoryProperties& MEMORYPROPERTIES() const { return MemoryProperties; }
	inline VkQueueFamilyProperties* QUEUEFAMILYPROPERTIES() const { return QueueFamilyProperties; }
	inline VK_QUEUEFAM* QUEUEFAMS() const { return QUEUEFAMs; }
	inline const uint32_t* ALLQUEUEFAMILIES() const { return AllQueueFamilies; }
	inline unsigned int QUEUEFAMSCOUNT() const { return QueueFamiliesCount; }
	inline unsigned int TRANSFERSUPPORTEDQUEUECOUNT() { return TRANSFERs_supportedqueuecount; }
	inline VkDevice LOGICALDEVICE() { return Logical_Device; }
	inline VkExtensionProperties* EXTPROPERTIES() { return Supported_DeviceExtensions; }
	inline VkPhysicalDeviceFeatures DEVICEFEATURES_SUPPORTED() { return Supported_Features; }
	inline VkPhysicalDeviceFeatures DEVICEFEATURES_ACTIVE() { return Active_Features; }
	inline unsigned int GRAPHICSQUEUEINDEX() { return GRAPHICS_QUEUEFamIndex; }

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
	bool isWindowSupported(VkSurfaceKHR WindowSurface) {
		bool supported = false;
		for (unsigned int QueueIndex = 0; QueueIndex < rendergpu->QUEUEFAMSCOUNT(); QueueIndex++) {
			VkBool32 Does_Support = 0;
			vkGetPhysicalDeviceSurfaceSupportKHR(rendergpu->PHYSICALDEVICE(), rendergpu->QUEUEFAMS()[QueueIndex].QUEUEFAMINDEX(), WindowSurface, &Does_Support);
			if (Does_Support) {
				QUEUEFAMs[QueueIndex].SUPPORT_PRESENTATION();
				supported = true;
			}
		}
		return supported;
	}
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