#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <atomic>
#include <string>
#include "vk_predefinitions.h"
#include "tgfx_structs.h"
#include "vk_extension.h"

struct core_public {
	VK_STATICVECTOR<WINDOW_VKOBJ, 50>& GETWINDOWs();
};


struct gpu_private;
struct GPU_VKOBJ{
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::GPU;
	static uint16_t GET_EXTRAFLAGS(GPU_VKOBJ* obj) { return 0; }
	void operator = (const GPU_VKOBJ& copyFrom) {
		isALIVE.store(true);
		Active_DeviceExtensions = copyFrom.Active_DeviceExtensions; Active_Features = copyFrom.Active_Features; AllQueueFamilies = copyFrom.AllQueueFamilies; 
		desc = copyFrom.desc; Device_Properties = copyFrom.Device_Properties; extensions = copyFrom.extensions; graphicsqueue = copyFrom.graphicsqueue;
		GRAPHICS_supportedqueuecount = copyFrom.GRAPHICS_supportedqueuecount; hidden = copyFrom.hidden; Logical_Device = copyFrom.Logical_Device;
		MemoryProperties = copyFrom.MemoryProperties; memtype_ids = copyFrom.memtype_ids; Physical_Device = copyFrom.Physical_Device;
		queuefams = copyFrom.queuefams; TRANSFERs_supportedqueuecount = copyFrom.TRANSFERs_supportedqueuecount; Supported_Features = copyFrom.Supported_Features;
		COMPUTE_supportedqueuecount = copyFrom.COMPUTE_supportedqueuecount; 
	}

private:
	friend struct core_functions; 
	friend struct gpuallocatorsys_vk;
	friend struct queuesys_vk;
	gpu_private* hidden = nullptr;
	tgfx_gpu_description desc;

	VkPhysicalDevice Physical_Device = {};
	VkPhysicalDeviceMemoryProperties MemoryProperties = {};
	VkPhysicalDeviceProperties2 Device_Properties = {};
	unsigned int QueueFamiliesCount = 0, TRANSFERs_supportedqueuecount = 0, COMPUTE_supportedqueuecount = 0, GRAPHICS_supportedqueuecount = 0;
	VkDevice Logical_Device = {};
	std::vector<const char*> Active_DeviceExtensions;
	extension_manager extensions;
	VkPhysicalDeviceFeatures Supported_Features = {}, Active_Features = {};
	uint32_t* AllQueueFamilies;


	std::vector<unsigned int> memtype_ids;
	//Use SortedQUEUEFAMsLIST to access queue families in increasing feature score order
	queuefam_vk** queuefams;
	queuefam_vk* graphicsqueue;
public:
	inline const char* DEVICENAME() { return desc.NAME; }
	inline const unsigned int APIVERSION() { return desc.API_VERSION; }
	inline const unsigned int DRIVERSION() { return desc.DRIVER_VERSION; }
	inline const gpu_type_tgfx DEVICETYPE() { return desc.GPU_TYPE; }
	inline const bool GRAPHICSSUPPORTED() { return GRAPHICS_supportedqueuecount; }
	inline const bool COMPUTESUPPORTED() { return COMPUTE_supportedqueuecount; }
	inline const bool TRANSFERSUPPORTED() { return TRANSFERs_supportedqueuecount; }
	inline VkPhysicalDevice PHYSICALDEVICE() { return Physical_Device; }
	inline const VkPhysicalDeviceProperties& DEVICEPROPERTIES() const { return Device_Properties.properties; }
	inline const VkPhysicalDeviceMemoryProperties& MEMORYPROPERTIES() const { return MemoryProperties; }
	inline queuefam_vk** QUEUEFAMS() const { return queuefams; }
	inline queuefam_vk* GRAPHICSQUEUEFAM() { return graphicsqueue; }
	inline const uint32_t* ALLQUEUEFAMILIES() const { return AllQueueFamilies; }
	inline unsigned int QUEUEFAMSCOUNT() const { return QueueFamiliesCount; }
	inline unsigned int TRANSFERSUPPORTEDQUEUECOUNT() { return TRANSFERs_supportedqueuecount; }
	inline VkDevice LOGICALDEVICE() { return Logical_Device; }
	inline VkPhysicalDeviceFeatures DEVICEFEATURES_SUPPORTED() { return Supported_Features; }
	inline VkPhysicalDeviceFeatures DEVICEFEATURES_ACTIVE() { return Active_Features; }
	inline const tgfx_gpu_description& DESC() { return desc; }
	inline extension_manager* EXTMANAGER() { return &extensions; }
	inline const unsigned int* MEMORYTYPE_IDS() { return memtype_ids.data(); }
	inline unsigned int MEMORYTYPE_IDSCOUNT() { return (uint32_t)memtype_ids.size(); }
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

struct MONITOR_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::MONITOR;
	static uint16_t GET_EXTRAFLAGS(MONITOR_VKOBJ* obj) { return 0; }
	void operator = (const MONITOR_VKOBJ& copyFrom) {
		isALIVE.store(true);
		width = copyFrom.width; height = copyFrom.height;
		color_bites = copyFrom.color_bites; refresh_rate = copyFrom.refresh_rate;
		physical_width = copyFrom.physical_width;
		physical_height = copyFrom.physical_height; name = copyFrom.name;
		monitorobj = copyFrom.monitorobj;
	}
	MONITOR_VKOBJ() = default;
	MONITOR_VKOBJ(const MONITOR_VKOBJ& copyFrom) { *this = copyFrom; }
	unsigned int width = 0, height = 0, color_bites = 0, refresh_rate = 0,
		physical_width = 0, physical_height = 0;
	const char* name = NULL;
	GLFWmonitor* monitorobj = NULL;
};

struct WINDOW_VKOBJ {
	std::atomic_bool isALIVE = false;
	static constexpr VKHANDLETYPEs HANDLETYPE = VKHANDLETYPEs::WINDOW;
	static uint16_t GET_EXTRAFLAGS(WINDOW_VKOBJ* obj) { return 0; }
	void operator = (const WINDOW_VKOBJ& copyFrom) {
		isALIVE.store(true);
		LASTWIDTH = copyFrom.LASTWIDTH; LASTHEIGHT = copyFrom.LASTHEIGHT;
		NEWWIDTH = copyFrom.NEWWIDTH; NEWHEIGHT = copyFrom.NEWHEIGHT;
		DISPLAYMODE = copyFrom.DISPLAYMODE; MONITOR = copyFrom.MONITOR;
		NAME = copyFrom.NAME; resize_cb = copyFrom.resize_cb;
		UserPTR = copyFrom.UserPTR;
		CurrentFrameSWPCHNIndex = copyFrom.CurrentFrameSWPCHNIndex;
		isResized.store(copyFrom.isResized.load());
		isSwapped.store(isSwapped.load());
		presentationqueue = copyFrom.presentationqueue;
		Window_Surface = copyFrom.Window_Surface;
		Window_SwapChain = copyFrom.Window_SwapChain;
		GLFW_WINDOW = copyFrom.GLFW_WINDOW;
		SurfaceCapabilities = copyFrom.SurfaceCapabilities;
		presentationmodes = copyFrom.presentationmodes;
		SWAPCHAINUSAGE = copyFrom.SWAPCHAINUSAGE;
		for (unsigned int i = 0; i < VKCONST_SwapchainTextureCountPerWindow; i++) {
			Swapchain_Textures[i] = copyFrom.Swapchain_Textures[i];
			PresentationSemaphores[i] = copyFrom.PresentationSemaphores[i];
			PresentationFences[i] = copyFrom.PresentationFences[i];
		}
		for (unsigned int i = 0; i < VKCONST_MAXSURFACEFORMATCOUNT; i++) {
			SurfaceFormats[i] = copyFrom.SurfaceFormats[i];
		}
	}
	WINDOW_VKOBJ() = default;
	WINDOW_VKOBJ(const WINDOW_VKOBJ& copyFrom) { *this = copyFrom; }
	unsigned int LASTWIDTH, LASTHEIGHT, NEWWIDTH, NEWHEIGHT;
	windowmode_tgfx DISPLAYMODE;
	MONITOR_VKOBJ* MONITOR;
	std::string NAME;
	tgfx_windowResizeCallback resize_cb = nullptr;
	void* UserPTR;
	uint32_t Swapchain_Textures[VKCONST_SwapchainTextureCountPerWindow];
	unsigned char CurrentFrameSWPCHNIndex = 0;
	//To avoid calling resize or swapbuffer twice in a frame!
	std::atomic<bool> isResized = false, isSwapped = false;
	//Don't make these SwapchainTextureCount + 1 (STC + 1) again! There should (STC + 1) textures to be able to use (STC + 1) semaphores, 
	// otherwise (STC) different semaphores for the same texture doesn't work as you expect!
	//Presentation Semaphores should only be used for rendergraph submission as wait
	semaphore_idtype_vk PresentationSemaphores[VKCONST_SwapchainTextureCountPerWindow];
	queuefam_vk* presentationqueue = nullptr;
	//Presentation Fences should only be used for CPU to wait
	fence_idtype_vk PresentationFences[VKCONST_SwapchainTextureCountPerWindow];

	VkSurfaceKHR Window_Surface = {};
	VkSwapchainKHR Window_SwapChain = {};
	GLFWwindow* GLFW_WINDOW = {};
	VkSurfaceCapabilitiesKHR SurfaceCapabilities = {};
	VkSurfaceFormatKHR SurfaceFormats[100] = {};
	struct PresentationModes {
		unsigned char immediate : 1;
		unsigned char mailbox : 1;
		unsigned char fifo : 1;
		unsigned char fifo_relaxed : 1;
		PresentationModes() : immediate(0), mailbox(0), fifo(0), fifo_relaxed(0){}
	};
	PresentationModes presentationmodes;
	VkImageUsageFlags SWAPCHAINUSAGE;
};

struct initialization_secondstageinfo{
	GPU_VKOBJ* renderergpu;

	unsigned int MAXDESCCOUNTs[VKCONST_DESCPOOLCOUNT_PERDESCTYPE], MAXDESCSETCOUNT;
	bool shouldActivate_DearIMGUI : 1;
};