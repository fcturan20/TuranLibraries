#include "Vulkan_Includes.h"
#define VK_NO_PROTOTYPES 1
#include "Vulkan_Resource.h"
#include <numeric>
#include "Vulkan_Core.h"
#define LOG VKCORE->TGFXCORE.DebugCallback

//RenderNode Dependency Helpers

//WaitInfos is a pointer because function expects a list of waits (Color attachment output and also VertexShader-UniformReads etc)
tgfx_passwaitdescription CreatePassWait_DrawPass(tgfx_drawpass* PassHandle, unsigned char SubpassIndex,
	tgfx_waitsignaldescription* WaitInfos, unsigned char isLastFrame) {return tgfx_passwaitdescription();}
//WaitInfo is single, because function expects only one wait and it should be created with CreateWaitSignal_ComputeShader()
tgfx_passwaitdescription CreatePassWait_ComputePass(tgfx_computepass* PassHandle, unsigned char SubpassIndex,
	tgfx_waitsignaldescription WaitInfo, unsigned char isLastFrame) {return tgfx_passwaitdescription();}
//WaitInfo is single, because function expects only one wait and it should be created with CreateWaitSignal_Transfer()
tgfx_passwaitdescription CreatePassWait_TransferPass(tgfx_transferpass* PassHandle,
	tgfx_transferpass_type Type, tgfx_waitsignaldescription WaitInfo, unsigned char isLastFrame) {return tgfx_passwaitdescription();}
//There is no option because you can only wait for a penultimate window pass
//I'd like to support last frame wait too but it confuses the users and it doesn't have much use
tgfx_passwaitdescription CreatePassWait_WindowPass(tgfx_windowpass* PassHandle) { return tgfx_passwaitdescription(); }
void GetMonitor_Resolution_ColorBites_RefreshRate(tgfx_monitor MonitorHandle, unsigned int* WIDTH, unsigned int* HEIGHT, unsigned int* ColorBites, unsigned int* RefreshRate){}
tgfx_subdrawpassdescription CreateSubDrawPassDescription() { return tgfx_subdrawpassdescription(); }

//Barrier Dependency Helpers

tgfx_waitsignaldescription CreateWaitSignal_DrawIndirectConsume() { return tgfx_waitsignaldescription(); }
tgfx_waitsignaldescription CreateWaitSignal_VertexInput(unsigned char IndexBuffer, unsigned char VertexAttrib) { return tgfx_waitsignaldescription(); }
tgfx_waitsignaldescription CreateWaitSignal_VertexShader(unsigned char UniformRead, unsigned char StorageRead, unsigned char StorageWrite) { return tgfx_waitsignaldescription(); }
tgfx_waitsignaldescription CreateWaitSignal_FragmentShader(unsigned char UniformRead, unsigned char StorageRead, unsigned char StorageWrite) { return tgfx_waitsignaldescription(); }
tgfx_waitsignaldescription CreateWaitSignal_ComputeShader(unsigned char UniformRead, unsigned char StorageRead, unsigned char StorageWrite) { return tgfx_waitsignaldescription(); }
tgfx_waitsignaldescription CreateWaitSignal_FragmentTests(unsigned char isEarly, unsigned char isRead, unsigned char isWrite) { return tgfx_waitsignaldescription(); }

					//HARDWARE CAPABILITY HELPERs

tgfx_textureusageflag CreateTextureUsageFlag(unsigned char isCopiableFrom, unsigned char isCopiableTo,
	unsigned char isRenderableTo, unsigned char isSampledReadOnly, unsigned char isRandomlyWrittenTo) {
	VkImageUsageFlags* UsageFlag = new VkImageUsageFlags;
	*UsageFlag = ((isCopiableFrom) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0) | 
		((isCopiableTo) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0) |
		((isRenderableTo) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0) |
		((isSampledReadOnly) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0) |
		((isRandomlyWrittenTo) ? VK_IMAGE_USAGE_STORAGE_BIT : 0);
	return (tgfx_textureusageflag)UsageFlag;
}
void GetSupportedAllocations_ofTexture(unsigned int GPUIndex, unsigned int* SupportedMemoryTypesBitset){}
void GetGPUInfo_General(tgfx_gpu GPUHandle, const char** NAME, unsigned int* API_VERSION, unsigned int* DRIVER_VERSION, tgfx_gpu_types* GPUTYPE,
	tgfx_memorytype_list* MemTypes, unsigned char* isGraphicsOperationsSupported, unsigned char* isComputeOperationsSupported, unsigned char* isTransferOperationsSupported);
void GetGPUInfo_Memory(tgfx_memorytype MemoryType, tgfx_memoryallocationtype* AllocType, unsigned long long* MaxAllocSize);
tgfx_result SetMemoryTypeInfo(tgfx_memorytype GPUMemoryType, unsigned long long AllocationSize, tgfx_extension_list Extensions);
void GetMonitor_Resolution_ColorBites_RefreshRate(tgfx_monitor MonitorHandle, unsigned int* WIDTH, unsigned int* HEIGHT, unsigned int* ColorBites, unsigned int* RefreshRate);
unsigned char DoesGPUsupportVKDescIndexing(tgfx_gpu GPUHandle) { GPU* VKGPU = (GPU*)GPUHandle; return VKGPU->ExtensionRelatedDatas.ISSUPPORTED_DESCINDEXING(); }

//END



unsigned char GetTextureTypeLimits(tgfx_texture_dimensions dims, tgfx_texture_order dataorder, tgfx_texture_channels channeltype,
	tgfx_textureusageflag usageflag, tgfx_gpu GPUHandle, unsigned int* MAXWIDTH, unsigned int* MAXHEIGHT, unsigned int* MAXDEPTH, unsigned int* MAXMIPLEVEL);

tgfx_initializationsecondstageinfo Create_GFXInitializationSecondStageInfo(tgfx_gpu RendererGPU, unsigned int MaterialCount, unsigned int MaxSumMaterial_SampledTexture, unsigned int MaxSumMaterial_ImageTexture, unsigned int MaxSumMaterial_UniformBuffer, unsigned int MaxSumMaterial_StorageBuffer, unsigned int GlobalShaderInput_SampledTextureCount, unsigned int GlobalShaderInput_ImageTextureCount, unsigned int GlobalShaderInput_UniformBufferCount, unsigned int GlobalShaderInput_StorageBufferCount, unsigned char isGlobalUniformBuffer_Index1, unsigned char isGlobalSampledTexture_Index1, unsigned char ShouldActive_dearIMGUI, tgfx_extension_list ExtensionList);



void Destroy_ExtensionData(tgfx_extension EXT) {
#ifdef VULKAN_DEBUGGING
	if (EXT == nullptr) {
		LOG(tgfx_result_FAIL, "You're trying to destroy a null pointer as extension, this should be avoided if you use release build!");
		return;
	}
	if (EXT == VKCORE->TGFXCORE.INVALIDHANDLE) {
		LOG(tgfx_result_FAIL, "You're trying to destroy a invalidterminator as extension, this should be avoided if you use release build!");
		return;
	}
#endif
	VK_Extension* VKEXT = (VK_Extension*)EXT;
#ifdef VULKAN_DEBUGGING
	if (VKEXT->DATA == nullptr || VKEXT->TYPE == VK_Extension::EXTTYPE::UNDEFINED) {
		LOG(tgfx_result_FAIL, "You're trying to destroy an extensions that's already destroyed, this should be avoided if you use release build!");
		return;
	}
	if (VKEXT->DATA == VKCORE->TGFXCORE.INVALIDHANDLE) {
		LOG(tgfx_result_FAIL, "You probably tried to change the data extension handle's pointing at, this may cause undefined behaviour and should be avoided if you use release build!");
		return;
	}
#endif
	switch (VKEXT->TYPE) {
	}
	VKEXT->DATA = nullptr;
	VKEXT->TYPE = VK_Extension::EXTTYPE::UNDEFINED;
}

void SetGFXHelperFunctions() {
	VKCORE->TGFXCORE.Helpers.CreatePassWait_ComputePass = CreatePassWait_ComputePass;
	VKCORE->TGFXCORE.Helpers.CreatePassWait_DrawPass = CreatePassWait_DrawPass;
	VKCORE->TGFXCORE.Helpers.CreatePassWait_TransferPass = CreatePassWait_TransferPass;
	VKCORE->TGFXCORE.Helpers.CreatePassWait_WindowPass = CreatePassWait_WindowPass;
	VKCORE->TGFXCORE.Helpers.CreateTextureUsageFlag = CreateTextureUsageFlag;
	VKCORE->TGFXCORE.Helpers.CreateWaitSignal_ComputeShader = CreateWaitSignal_ComputeShader;
	VKCORE->TGFXCORE.Helpers.CreateWaitSignal_DrawIndirectConsume = CreateWaitSignal_DrawIndirectConsume;
	VKCORE->TGFXCORE.Helpers.CreateWaitSignal_FragmentShader = CreateWaitSignal_FragmentShader;
	VKCORE->TGFXCORE.Helpers.CreateWaitSignal_FragmentTests = CreateWaitSignal_FragmentTests;
	VKCORE->TGFXCORE.Helpers.CreateWaitSignal_VertexInput = CreateWaitSignal_VertexInput;
	VKCORE->TGFXCORE.Helpers.CreateWaitSignal_VertexShader = CreateWaitSignal_VertexShader;
	VKCORE->TGFXCORE.Helpers.Create_GFXInitializationSecondStageInfo = Create_GFXInitializationSecondStageInfo;
	VKCORE->TGFXCORE.Helpers.GetGPUInfo_General = GetGPUInfo_General;
	VKCORE->TGFXCORE.Helpers.GetGPUInfo_Memory = GetGPUInfo_Memory;
	VKCORE->TGFXCORE.Helpers.SetMemoryTypeInfo = SetMemoryTypeInfo;
	VKCORE->TGFXCORE.Helpers.GetSupportedAllocations_ofTexture = GetSupportedAllocations_ofTexture;
	VKCORE->TGFXCORE.Helpers.GetTextureTypeLimits = GetTextureTypeLimits;
	VKCORE->TGFXCORE.Helpers.GetMonitor_Resolution_ColorBites_RefreshRate = GetMonitor_Resolution_ColorBites_RefreshRate;
	VKCORE->TGFXCORE.Helpers.DoesGPUsupportsVKDESCINDEXING = DoesGPUsupportVKDescIndexing;
	VKCORE->TGFXCORE.Helpers.Destroy_ExtensionData = Destroy_ExtensionData;

	SEMAPHORESYS = new VK_SemaphoreSystem;
}


void* GetExtension_FromList(VK_Extension** List, VK_Extension::EXTTYPE Type) {
	if (List == nullptr || List == VKCORE->TGFXCORE.INVALIDHANDLE) {
		return nullptr;
	}
	unsigned int i = 0;
	while (List[i] != VKCORE->TGFXCORE.INVALIDHANDLE) {
		if (List[i]->DATA != nullptr && List[i]->TYPE == Type) {
			return List[i]->DATA;
		}
		i++;
	}
	return nullptr;
}

tgfx_initializationsecondstageinfo Create_GFXInitializationSecondStageInfo(tgfx_gpu RendererGPU, unsigned int MaterialCount, 
	unsigned int MaxSumMaterial_SampledTexture, unsigned int MaxSumMaterial_ImageTexture, unsigned int MaxSumMaterial_UniformBuffer, unsigned int MaxSumMaterial_StorageBuffer, 
	unsigned int GlobalShaderInput_SampledTextureCount, unsigned int GlobalShaderInput_ImageTextureCount, unsigned int GlobalShaderInput_UniformBufferCount, unsigned int GlobalShaderInput_StorageBufferCount,
	unsigned char isGlobalUniformBuffer_Index1, unsigned char isGlobalSampledTexture_Index1, 
	unsigned char ShouldActive_dearIMGUI, tgfx_extension_list ExtensionList) {
	GPU* VKGPU = (GPU*)RendererGPU;

	unsigned int MAXDESC_SAMPLEDTEXTURE = MaxSumMaterial_SampledTexture + GlobalShaderInput_SampledTextureCount;
	unsigned int MAXDESC_IMAGETEXTURE = MaxSumMaterial_ImageTexture + GlobalShaderInput_ImageTextureCount;
	unsigned int MAXDESC_UNIFORMBUFFER = MaxSumMaterial_UniformBuffer + GlobalShaderInput_UniformBufferCount;
	unsigned int MAXDESC_STORAGEBUFFER = MaxSumMaterial_StorageBuffer + GlobalShaderInput_StorageBufferCount;
	if (MAXDESC_SAMPLEDTEXTURE > VKGPU->ExtensionRelatedDatas.GETMAXDESC(DescType::SAMPLER) || MAXDESC_IMAGETEXTURE > VKGPU->ExtensionRelatedDatas.GETMAXDESC(DescType::IMAGE) ||
		MAXDESC_UNIFORMBUFFER > VKGPU->ExtensionRelatedDatas.GETMAXDESC(DescType::UBUFFER) || MAXDESC_STORAGEBUFFER > VKGPU->ExtensionRelatedDatas.GETMAXDESC(DescType::SBUFFER)) {
		LOG(tgfx_result_FAIL, "One of the shader input types exceeds the GPU's limits. Don't forget that Global + General + Per Instance shouldn't exceed GPU's related shader input type limits!");
		return nullptr;
	}
	
	InitializationSecondStageInfo* info = new InitializationSecondStageInfo;

	info->MaxSumMaterial_ImageTexture = MaxSumMaterial_ImageTexture;
	info->MaxSumMaterial_SampledTexture = MaxSumMaterial_SampledTexture;
	info->MaxSumMaterial_StorageBuffer = MaxSumMaterial_StorageBuffer;
	info->MaxSumMaterial_UniformBuffer = MaxSumMaterial_UniformBuffer;
	info->GlobalShaderInput_ImageTextureCount = GlobalShaderInput_ImageTextureCount;
	info->GlobalShaderInput_SampledTextureCount = GlobalShaderInput_SampledTextureCount;
	info->GlobalShaderInput_StorageBufferCount = GlobalShaderInput_StorageBufferCount;
	info->GlobalShaderInput_UniformBufferCount = GlobalShaderInput_UniformBufferCount;
	info->MaxMaterialCount = MaterialCount;
	info->RENDERERGPU = VKGPU;
	info->isSampledTexture_Index1 = isGlobalSampledTexture_Index1;
	info->isUniformBuffer_Index1 = isGlobalUniformBuffer_Index1;
	info->shouldActivate_DearIMGUI = ShouldActive_dearIMGUI;
	return (tgfx_initializationsecondstageinfo)info;
}


unsigned char GetTextureTypeLimits(tgfx_texture_dimensions dims, tgfx_texture_order dataorder, tgfx_texture_channels channeltype,
	tgfx_textureusageflag usageflag, tgfx_gpu GPUHandle, unsigned int* MAXWIDTH, unsigned int* MAXHEIGHT, unsigned int* MAXDEPTH, unsigned int* MAXMIPLEVEL) {
	LOG(tgfx_result_NOTCODED, "Vulkan backend's GetTextureTypeLimits() isn't coded yet!");
	LOG(tgfx_result_SUCCESS, "Before vkGetPhysicalDeviceImageFormatProperties()!");
	VkImageFormatProperties props = {};
	/*
	if (vkGetPhysicalDeviceImageFormatProperties(RENDERGPU->Physical_Device, Find_VkFormat_byTEXTURECHANNELs(channeltype),
		Find_VkImageType(dims), Find_VkTiling(dataorder), Find_VKImageUsage_forGFXTextureDesc(*(VkImageUsageFlags*)usageflag, channeltype),
		0, &props) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "GFX->GetTextureTypeLimits() has failed!");
		return false;
	}*/
	LOG(tgfx_result_SUCCESS, "After vkGetPhysicalDeviceImageFormatProperties()!");

	*MAXWIDTH = props.maxExtent.width;
	*MAXHEIGHT = props.maxExtent.height;
	*MAXDEPTH = props.maxExtent.depth;
	*MAXMIPLEVEL = props.maxMipLevels;
	return true;
}
bool GetTextureTypeLimits(tgfx_texture_dimensions dims, tgfx_texture_order dataorder, tgfx_texture_channels channeltype,
	tgfx_textureusageflag usageflag, unsigned int GPUIndex, unsigned int& MAXWIDTH, unsigned int& MAXHEIGHT, unsigned int& MAXDEPTH, unsigned int& MAXMIPLEVEL) {
	LOG(tgfx_result_SUCCESS, "Before vkGetPhysicalDeviceImageFormatProperties()!");
	VkImageFormatProperties props;
	if (vkGetPhysicalDeviceImageFormatProperties(RENDERGPU->PHYSICALDEVICE(), Find_VkFormat_byTEXTURECHANNELs(channeltype),
		Find_VkImageType(dims), Find_VkTiling(dataorder), *(VkImageUsageFlags*)usageflag,
		0, &props) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "(GFX->GetTextureTypeLimits() has failed!");
			return false;
	}
	LOG(tgfx_result_SUCCESS, "After vkGetPhysicalDeviceImageFormatProperties()!");

	MAXWIDTH = props.maxExtent.width;
	MAXHEIGHT = props.maxExtent.height;
	MAXDEPTH = props.maxExtent.depth;
	MAXMIPLEVEL = props.maxMipLevels;
	return true;
}
/*
void GetSupportedAllocations_ofTexture(unsigned int GPUIndex, unsigned int* SupportedMemoryTypesBitset) {
	VkImageCreateInfo im_ci = {};
	im_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	im_ci.arrayLayers = 1;
	im_ci.extent.width = TEXTURE.WIDTH;
	im_ci.extent.height = TEXTURE.HEIGHT;
	im_ci.extent.depth = 1;
	im_ci.flags = 0;
	im_ci.format = Find_VkFormat_byTEXTURECHANNELs(TEXTURE.CHANNEL_TYPE);
	im_ci.imageType = Find_VkImageType(TEXTURE.DIMENSION);
	im_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	im_ci.mipLevels = 1;
	im_ci.pNext = nullptr;


	if (GPU_TO_RENDER->QUEUEs.size() > 1) {
		im_ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
	}
	else {
		im_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	im_ci.pQueueFamilyIndices = GPU_TO_RENDER->AllQueueFamilies;
	im_ci.queueFamilyIndexCount = GPU_TO_RENDER->QUEUEs.size();
	im_ci.tiling = Find_VkTiling(TEXTURE.DATAORDER);
	im_ci.usage = Find_VKImageUsage_forGFXTextureDesc(TEXTURE.USAGE, TEXTURE.CHANNEL_TYPE);
	im_ci.samples = VK_SAMPLE_COUNT_1_BIT;


	LOG(tgfx_result_SUCCESS, "Before vkCreateImage()!");
	VkImage Imageobj;
	if (vkCreateImage(GPU_TO_RENDER->Logical_Device, &im_ci, nullptr, &Imageobj) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "GFX->IsTextureSupported() has failed in vkCreateImage()!");
			return;
	}
	LOG(tgfx_result_SUCCESS, "After vkCreateImage()!");

	VkMemoryRequirements req;
	vkGetImageMemoryRequirements(GPU_TO_RENDER->Logical_Device, Imageobj, &req);
	LOG(tgfx_result_SUCCESS, "After vkGetImageMemoryRequirements()!");
	vkDestroyImage(GPU_TO_RENDER->Logical_Device, Imageobj, nullptr);
	LOG(tgfx_result_SUCCESS, "After vkDestroyImage()!");
	bool isFound = false;
	for (unsigned int GFXMemoryTypeIndex = 0; GFXMemoryTypeIndex < GPU_TO_RENDER->ALLOCs.size(); GFXMemoryTypeIndex++) {
		VK_MemoryAllocation& ALLOC = GPU_TO_RENDER->ALLOCs[GFXMemoryTypeIndex];
		if (req.memoryTypeBits & (1u << ALLOC.MemoryTypeIndex)) {
			SupportedMemoryTypesBitset |= (1u << GFXMemoryTypeIndex);
			isFound = true;
		}
	}
	if (!isFound) {
		LOG(tgfx_result_FAIL, "(Your image is supported by a memory allocation that GFX API doesn't support to allocate, please report this issue!");
	}
}
void GetSupportedAllocations_ofTexture(const tgfx_texture_Description& TEXTURE, unsigned int GPUIndex, unsigned int& SupportedMemoryTypesBitset) {
	VkImageCreateInfo im_ci = {};
	im_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	im_ci.arrayLayers = 1;
	im_ci.extent.width = TEXTURE.WIDTH;
	im_ci.extent.height = TEXTURE.HEIGHT;
	im_ci.extent.depth = 1;
	im_ci.flags = 0;
	im_ci.format = Find_VkFormat_byTEXTURECHANNELs(TEXTURE.CHANNEL_TYPE);
	im_ci.imageType = Find_VkImageType(TEXTURE.DIMENSION);
	im_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	im_ci.mipLevels = 1;
	im_ci.pNext = nullptr;


	if (GPU_TO_RENDER->QUEUEs.size() > 1) {
		im_ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
	}
	else {
		im_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	im_ci.pQueueFamilyIndices = GPU_TO_RENDER->AllQueueFamilies;
	im_ci.queueFamilyIndexCount = GPU_TO_RENDER->QUEUEs.size();
	im_ci.tiling = Find_VkTiling(TEXTURE.DATAORDER);
	im_ci.usage = Find_VKImageUsage_forGFXTextureDesc(TEXTURE.USAGE, TEXTURE.CHANNEL_TYPE);
	im_ci.samples = VK_SAMPLE_COUNT_1_BIT;


	LOG(tgfx_result_SUCCESS, "Before vkCreateImage()!");
	VkImage Imageobj;
	if (vkCreateImage(GPU_TO_RENDER->Logical_Device, &im_ci, nullptr, &Imageobj) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "(GFX->IsTextureSupported() has failed in vkCreateImage()!");
			return;
	}
	LOG(tgfx_result_SUCCESS, "After vkCreateImage()!");

	VkMemoryRequirements req;
	vkGetImageMemoryRequirements(GPU_TO_RENDER->Logical_Device, Imageobj, &req);
	LOG(tgfx_result_SUCCESS, "After vkGetImageMemoryRequirements()!");
	vkDestroyImage(GPU_TO_RENDER->Logical_Device, Imageobj, nullptr);
	LOG(tgfx_result_SUCCESS, "After vkDestroyImage()!");
	bool isFound = false;
	for (unsigned int GFXMemoryTypeIndex = 0; GFXMemoryTypeIndex < GPU_TO_RENDER->ALLOCs.size(); GFXMemoryTypeIndex++) {
		VK_MemoryAllocation& ALLOC = GPU_TO_RENDER->ALLOCs[GFXMemoryTypeIndex];
		if (req.memoryTypeBits & (1u << ALLOC.MemoryTypeIndex)) {
			SupportedMemoryTypesBitset |= (1u << GFXMemoryTypeIndex);
			isFound = true;
		}
	}
	if (!isFound) {
		LOG(tgfx_result_FAIL, "(Your image is supported by a memory allocation that GFX API doesn't support to allocate, please report this issue!");
	}
}*/


					//HARDWARE CAPABILITY HELPERs

void GetGPUInfo_General(tgfx_gpu GPUHandle, const char** NAME, unsigned int* API_VERSION, unsigned int* DRIVER_VERSION, tgfx_gpu_types* GPUTYPE, tgfx_memorytype_list* MemTypes,
	unsigned char* isGraphicsOperationsSupported, unsigned char* isComputeOperationsSupported, unsigned char* isTransferOperationsSupported) {
	GPU* VKGPU = (GPU*)GPUHandle;
	if (NAME) {*NAME = VKGPU->DEVICENAME().c_str();}
	if (API_VERSION) { *API_VERSION = VKGPU->APIVERSION(); }
	if (DRIVER_VERSION) { *DRIVER_VERSION = VKGPU->DRIVERSION(); }
	if (GPUTYPE) { *GPUTYPE = VKGPU->DEVICETYPE(); }
	
	if (MemTypes) { 
		*MemTypes = new tgfx_memorytype[VKGPU->ALLOCS().size() + 1]; 
		for (unsigned int i = 0; i < VKGPU->ALLOCS().size(); i++) { (*MemTypes)[i] = (tgfx_memorytype)&VKGPU->ALLOCS()[i]; }
		(*MemTypes)[VKGPU->ALLOCS().size()] = (tgfx_memorytype)VKCORE->TGFXCORE.INVALIDHANDLE;
	}
	if (isGraphicsOperationsSupported) { *isGraphicsOperationsSupported = VKGPU->GRAPHICSSUPPORTED(); }
	if (isComputeOperationsSupported) { *isComputeOperationsSupported = VKGPU->COMPUTESUPPORTED(); }
	if (isTransferOperationsSupported) { *isTransferOperationsSupported = VKGPU->TRANSFERSUPPORTED(); }
}
void GetGPUInfo_Memory(tgfx_memorytype MemoryType, tgfx_memoryallocationtype* AllocType, unsigned long long* MaxAllocSize) {
#ifdef VULKAN_DEBUGGING
	if (MemoryType == nullptr) {
		LOG(tgfx_result_FAIL, "GetGPUInfo_Memory() has failed because MemoryType handle is nullptr! You should avoid this in release build!");
		return;
	}
	if (MemoryType == VKCORE->TGFXCORE.INVALIDHANDLE) {
		LOG(tgfx_result_FAIL, "GetGPUInfo_Memory() has failed because MemoryType handle is invalid handle (terminator)! You should avoid this in release build!");
		return;
	}
#endif
	VK_MemoryAllocation* MEMTYPE = (VK_MemoryAllocation*)MemoryType;
	if (MaxAllocSize) { *MaxAllocSize = MEMTYPE->MaxSize; }
	if (AllocType) { *AllocType = MEMTYPE->TYPE; }
}
tgfx_result SetMemoryTypeInfo(tgfx_memorytype GPUMemoryType, unsigned long long AllocationSize, tgfx_extension_list Extensions) {
	VK_MemoryAllocation* ALLOC = (VK_MemoryAllocation*)GPUMemoryType;
	if (AllocationSize > ALLOC->MaxSize) {
		LOG(tgfx_result_INVALIDARGUMENT, "SetMemoryTypeInfo() has failed because allocation size can't be larger than maximum size!");
		return tgfx_result_INVALIDARGUMENT;
	}
	ALLOC->ALLOCATIONSIZE = AllocationSize;
	return tgfx_result_SUCCESS;
}






//													VULKAN BACKEND-ONLY STRUCT AND FUNCTION DEFINITIONS








void VK_SemaphoreSystem::DestroySemaphore(VK_Semaphore::SemaphoreIDType SemaphoreID) {
#ifdef VULKAN_DEBUGGING
	VK_Semaphore& Semaphore = GetSemaphore(SemaphoreID);
	if (Semaphore.ID == VK_Semaphore::INVALID_ID) {
		LOG(tgfx_result_FAIL, "Vulkan backend couldn't find the semaphore in DestroySemaphore() function!");
		return;
	}
	Semaphore.isUsed = false;
#else
	VK_Semaphore* Semaphore = (VK_Semaphore*)SemaphoreID;
	Semaphore->isUsed = false;
#endif
}
VK_SemaphoreSystem::VK_SemaphoreSystem() {
	VK_Semaphore* INVALIDSEM = new VK_Semaphore;
	INVALIDSEM->isUsed = false;
	INVALIDSEM->SPHandle = VK_NULL_HANDLE;
#ifdef VULKAN_DEBUGGING
	INVALIDSEM->ID = VK_Semaphore::INVALID_ID;
#endif
	Semaphores.push_back(INVALIDSEM);
}
VK_Semaphore& VK_SemaphoreSystem::Create_Semaphore() {
	for (unsigned int i = 0; i < SEMAPHORESYS->Semaphores.size(); i++) {
		if (!SEMAPHORESYS->Semaphores[i]->isUsed) {
			SEMAPHORESYS->Semaphores[i]->isUsed = true;
			return *SEMAPHORESYS->Semaphores[i];
		}
	}
	VkSemaphoreCreateInfo Semaphore_ci = {};
	Semaphore_ci.flags = 0;
	Semaphore_ci.pNext = nullptr;
	Semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore vksemp = VK_NULL_HANDLE;
	if (vkCreateSemaphore(RENDERGPU->LOGICALDEVICE(), &Semaphore_ci, nullptr, &vksemp) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "Window creation has failed while creating semaphores for each swapchain texture!");
		return *SEMAPHORESYS->Semaphores[0];
	}
#ifdef VULKAN_DEBUGGING
	if (vksemp == VK_NULL_HANDLE) {
		LOG(tgfx_result_FAIL, "vkCreateSemaphore() has failed!");
		return *SEMAPHORESYS->Semaphores[0];
	}
#endif

	VK_Semaphore* NewSemaphore = new VK_Semaphore;
	NewSemaphore->isUsed = true;
	NewSemaphore->SPHandle = vksemp;
#ifdef VULKAN_DEBUGGING
	NewSemaphore->ID = SEMAPHORESYS->Semaphores.size();
#endif
	SEMAPHORESYS->Semaphores.push_back(NewSemaphore);
	return *NewSemaphore;
}
void VK_SemaphoreSystem::ClearList() { LOG(tgfx_result_NOTCODED, "SemaphoreSys->ClearList() isn't coded!"); }


#ifdef VULKAN_DEBUGGING
VK_Semaphore& VK_SemaphoreSystem::GetSemaphore(VK_Semaphore::SemaphoreIDType ID) {
	for (VK_Semaphore::SemaphoreIDType i = 0; i < SEMAPHORESYS->Semaphores.size(); i++) {
		if (SEMAPHORESYS->Semaphores[i]->Get_ID() == ID) {
			return *SEMAPHORESYS->Semaphores[i];
		}
	}
	LOG(tgfx_result_FAIL, "There is no semaphore that has this ID! Invalid Semaphore has been returned!");
	return *SEMAPHORESYS->Semaphores[0];
}
#endif



VK_QUEUEFLAG::VK_QUEUEFLAG() {
	is_GRAPHICSsupported = false;
	is_COMPUTEsupported = false;
	is_PRESENTATIONsupported = false;
	is_TRANSFERsupported = false;
	doesntNeedAnything = false;
}
VK_QUEUEFLAG VK_QUEUEFLAG::CreateInvalidNullFlag() {
	return VK_QUEUEFLAG();
}
bool VK_QUEUEFLAG::isFlagValid() const {
	if (doesntNeedAnything && (is_GRAPHICSsupported || is_COMPUTEsupported || is_PRESENTATIONsupported || is_TRANSFERsupported)) {
		LOG(tgfx_result_FAIL, "(This flag doesn't need anything but it also needs something, this shouldn't happen!");
			return false;
	}
	if (!doesntNeedAnything && !is_GRAPHICSsupported && !is_COMPUTEsupported && !is_PRESENTATIONsupported && !is_TRANSFERsupported) {
		LOG(tgfx_result_FAIL, "(This flag needs something but it doesn't support anything");
			return false;
	}
	return true;
}
void VK_QUEUE::Initialize(VkQueue QUEUE, float PRIORITY) {
	Queue = QUEUE; Priority = PRIORITY;
	RenderGraphFences[0] = FENCESYS->CreateFence().ID;
	RenderGraphFences[1] = FENCESYS->CreateFence().ID;
}
void VK_QUEUEFAM::Initialize(const VkQueueFamilyProperties& FamProps, unsigned int QueueFamIndex, const std::vector<VkQueue>& Queues) {
	QueueCount = Queues.size();
	QueueFamilyIndex = static_cast<uint32_t>(QueueFamIndex);
	if (FamProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
		SupportFlag.is_GRAPHICSsupported = true;
		QueueFeatureScore++;
	}
	if (FamProps.queueFlags & VK_QUEUE_COMPUTE_BIT) {
		SupportFlag.is_COMPUTEsupported = true;
		QueueFeatureScore++;
	}
	if (FamProps.queueFlags & VK_QUEUE_TRANSFER_BIT) {
		SupportFlag.is_TRANSFERsupported = true;
		QueueFeatureScore++;
	}

	QUEUEs = new VK_QUEUE[Queues.size()];
	for (unsigned int QueueIndex = 0; QueueIndex < Queues.size(); QueueIndex++) {
		QUEUEs[QueueIndex].Initialize(Queues[QueueIndex], 1.0f - (QueueIndex * (1.0f / Queues.size())));
	}
}




void GPU::Gather_PhysicalDeviceInformations() {
	vkGetPhysicalDeviceProperties(Physical_Device, &Device_Properties);
	vkGetPhysicalDeviceFeatures(Physical_Device, &Supported_Features);

	//GET QUEUE FAMILIES, SAVE THEM TO GPU OBJECT, CHECK AND SAVE GRAPHICS,COMPUTE,TRANSFER QUEUEFAMILIES INDEX
	vkGetPhysicalDeviceQueueFamilyProperties(Physical_Device, &QueueFamiliesCount, nullptr);
	QueueFamilyProperties = new VkQueueFamilyProperties[QueueFamiliesCount];
	vkGetPhysicalDeviceQueueFamilyProperties(Physical_Device, &QueueFamiliesCount, QueueFamilyProperties);

	vkGetPhysicalDeviceMemoryProperties(Physical_Device, &MemoryProperties);
}
void GPU::Analize_PhysicalDeviceMemoryProperties() {
	for (uint32_t MemoryTypeIndex = 0; MemoryTypeIndex < MemoryProperties.memoryTypeCount; MemoryTypeIndex++) {
		VkMemoryType& MemoryType = MemoryProperties.memoryTypes[MemoryTypeIndex];
		bool isDeviceLocal = false;
		bool isHostVisible = false;
		bool isHostCoherent = false;
		bool isHostCached = false;

		if ((MemoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
			isDeviceLocal = true;
		}
		if ((MemoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
			isHostVisible = true;
		}
		if ((MemoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
			isHostCoherent = true;
		}
		if ((MemoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
			isHostCached = true;
		}

		if (Device_Properties.deviceType != VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			continue;
		}
		if (!isDeviceLocal && !isHostVisible && !isHostCoherent && !isHostCached) {
			continue;
		}
		if (isDeviceLocal) {
			if (isHostVisible && isHostCoherent) {
				VK_MemoryAllocation alloc;
				alloc.MemoryTypeIndex = MemoryTypeIndex;
				alloc.TYPE = tgfx_memoryallocationtype::FASTHOSTVISIBLE;
				alloc.MaxSize = MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				ALLOCs.push_back(alloc);

				LOG(tgfx_result_SUCCESS, ("Found FAST HOST VISIBLE BIT! Size: " + std::to_string(MemoryProperties.memoryHeaps[MemoryType.heapIndex].size)).c_str());
			}
			else {
				VK_MemoryAllocation alloc;
				alloc.TYPE = tgfx_memoryallocationtype::DEVICELOCAL;
				alloc.MaxSize = MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				alloc.MemoryTypeIndex = MemoryTypeIndex;
				ALLOCs.push_back(alloc);
				LOG(tgfx_result_SUCCESS, ("Found DEVICE LOCAL BIT! Size: " + std::to_string(MemoryProperties.memoryHeaps[MemoryType.heapIndex].size)).c_str());
			}
		}
		else if (isHostVisible && isHostCoherent) {
			if (isHostCached) {
				VK_MemoryAllocation alloc;
				alloc.MemoryTypeIndex = MemoryTypeIndex;
				alloc.MaxSize = MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				alloc.TYPE = tgfx_memoryallocationtype::READBACK;
				ALLOCs.push_back(alloc);
				LOG(tgfx_result_SUCCESS, ("Found READBACK BIT! Size: " + std::to_string(MemoryProperties.memoryHeaps[MemoryType.heapIndex].size)).c_str());
			}
			else {
				VK_MemoryAllocation alloc;
				alloc.MemoryTypeIndex = MemoryTypeIndex;
				alloc.MaxSize = MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
				alloc.TYPE = tgfx_memoryallocationtype::HOSTVISIBLE;
				ALLOCs.push_back(alloc);
				LOG(tgfx_result_SUCCESS, ("Found HOST VISIBLE BIT! Size: " + std::to_string(MemoryProperties.memoryHeaps[MemoryType.heapIndex].size)).c_str());
			}
		}
	}
}
void GPU::Analize_Queues() {

	if (GRAPHICS_QUEUEFamIndex == UINT32_MAX || !TRANSFERs_supportedqueuecount || !COMPUTE_supportedqueuecount) {
		LOG(tgfx_result_FAIL, "The GPU doesn't support one of the following operations, so we can't let you use this GPU: Compute, Transfer, Graphics");
		return;
	}
}
const char* GPU::Convert_VendorID_toaString(uint32_t VendorID){
	switch (VendorID) {
	case 0x1002:
		return "AMD";
	case 0x10DE:
		return "Nvidia";
	case 0x8086:
		return "Intel";
	case 0x13B5:
		return "ARM";
	default:
		LOG(tgfx_result_FAIL, "(Vulkan_Core::Check_Computer_Specs failed to find GPU's Vendor, Vendor ID is: " + VendorID);
		return "NULL";
	}
}
inline const char* const* Get_Supported_LayerNames(const VkLayerProperties* list, uint32_t length) {
	const char** NameList = new const char* [length];
	for (unsigned int i = 0; i < length; i++) {
		NameList[i] = list[i].layerName;
	}
	return NameList;
}
inline bool IsExtensionSupported(const char* ExtensionName, VkExtensionProperties* SupportedExtensions, unsigned int SupportedExtensionsCount) {
	bool Is_Found = false;
	for (unsigned int supported_extension_index = 0; supported_extension_index < SupportedExtensionsCount; supported_extension_index++) {
		if (strcmp(ExtensionName, SupportedExtensions[supported_extension_index].extensionName)) {
			return true;
		}
	}
	LOG(tgfx_result_WARNING, ("Extension: " + std::string(ExtensionName) + " is not supported by the GPU!").c_str());
	return false;
}
//Some features needs device extensions, so check if they're supported here
//After finding supported device features, activate them in Activate_DeviceFeatures
void GPU::Activate_DeviceExtensions() {
	//GET SUPPORTED DEVICE EXTENSIONS
	vkEnumerateDeviceExtensionProperties(Physical_Device, nullptr, &Supported_DeviceExtensionsCount, nullptr);
	Supported_DeviceExtensions = new VkExtensionProperties[Supported_DeviceExtensionsCount];
	vkEnumerateDeviceExtensionProperties(Physical_Device, nullptr, &Supported_DeviceExtensionsCount, Supported_DeviceExtensions);

	//Check Seperate_DepthStencil
	if (Application_Info.apiVersion == VK_API_VERSION_1_0 || Application_Info.apiVersion == VK_API_VERSION_1_1) {
		if (IsExtensionSupported(VK_KHR_SEPARATE_DEPTH_STENCIL_LAYOUTS_EXTENSION_NAME, Supported_DeviceExtensions, Supported_DeviceExtensionsCount)) {
			ExtensionRelatedDatas.SUPPORT_SEPERATEDDEPTHSTENCILLAYOUTS();
			Active_DeviceExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
			if (Application_Info.apiVersion == VK_API_VERSION_1_0) {
				Active_DeviceExtensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);
				Active_DeviceExtensions.push_back(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
			}
		}
	}
	else {
		ExtensionRelatedDatas.SUPPORT_SEPERATEDDEPTHSTENCILLAYOUTS();
	}

	if (IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, Supported_DeviceExtensions, Supported_DeviceExtensionsCount)) {
		ExtensionRelatedDatas.SUPPORT_SWAWPCHAINDISPLAY();
		Active_DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}
	else {
		LOG(tgfx_result_WARNING, "Current GPU doesn't support to display a swapchain, so you shouldn't use any window related functionality such as: GFXRENDERER->Create_WindowPass, GFX->Create_Window, GFXRENDERER->Swap_Buffers ...");
	}


	//Check Descriptor Indexing
	//First check Maintenance3 and PhysicalDeviceProperties2 for Vulkan 1.0
	if (Application_Info.apiVersion == VK_API_VERSION_1_0) {
		if (IsExtensionSupported(VK_KHR_MAINTENANCE3_EXTENSION_NAME, Supported_DeviceExtensions, Supported_DeviceExtensionsCount) && isSupported_PhysicalDeviceProperties2) {
			Active_DeviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
			if (IsExtensionSupported(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, Supported_DeviceExtensions, Supported_DeviceExtensionsCount)) {
				Active_DeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
				ExtensionRelatedDatas.SUPPORT_DESCINDEXING();
			}
		}
	}
	//Maintenance3 and PhysicalDeviceProperties2 is core in 1.1, so check only Descriptor Indexing
	else {
		//If Vulkan is 1.1, check extension
		if (Application_Info.apiVersion == VK_API_VERSION_1_1) {
			if (IsExtensionSupported(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, Supported_DeviceExtensions, Supported_DeviceExtensionsCount)) {
				Active_DeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
				ExtensionRelatedDatas.SUPPORT_DESCINDEXING();
			}
		}
		//1.2+ Vulkan supports DescriptorIndexing by default.
		else {
			ExtensionRelatedDatas.SUPPORT_DESCINDEXING();
		}
	}
}
void GPU::Activate_DeviceFeatures(VkDeviceCreateInfo& dev_ci, DeviceExtendedFeatures& dev_ftrs) {
	const void*& dev_ci_Next = dev_ci.pNext;
	void** extending_Next = nullptr;


	//Check for seperate RTSlot blending
	if (Supported_Features.independentBlend) {
		Active_Features.independentBlend = VK_TRUE;
		//GPUDesc.isSupported_SeperateRTSlotBlending = true;
	}

	//Activate Descriptor Indexing Features
	if (ExtensionRelatedDatas.ISSUPPORTED_DESCINDEXING()) {
		dev_ftrs.DescIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
		dev_ftrs.DescIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		dev_ftrs.DescIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
		dev_ftrs.DescIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
		dev_ftrs.DescIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
		dev_ftrs.DescIndexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
		dev_ftrs.DescIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
		dev_ftrs.DescIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
		dev_ftrs.DescIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		dev_ftrs.DescIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
		dev_ftrs.DescIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
		dev_ftrs.DescIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
		dev_ftrs.DescIndexingFeatures.pNext = nullptr;
		dev_ftrs.DescIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

		if (extending_Next) {
			*extending_Next = &dev_ftrs.DescIndexingFeatures;
			extending_Next = &dev_ftrs.DescIndexingFeatures.pNext;
		}
		else {
			dev_ci_Next = &dev_ftrs.DescIndexingFeatures;
			extending_Next = &dev_ftrs.DescIndexingFeatures.pNext;
		}
	}
}
//It is safe to use logical device here, because it is already created
void GPU::Check_DeviceLimits() {
	//Check Descriptor Limits
	{
		if (ExtensionRelatedDatas.ISSUPPORTED_DESCINDEXING()) {
			VkPhysicalDeviceDescriptorIndexingProperties* descindexinglimits = new VkPhysicalDeviceDescriptorIndexingProperties{};
			descindexinglimits->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;
			VkPhysicalDeviceProperties2 devprops2;
			devprops2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			devprops2.pNext = descindexinglimits;
			vkGetPhysicalDeviceProperties2(Physical_Device, &devprops2);

			ExtensionRelatedDatas.GETMAXDESC(DescType::SAMPLER) = descindexinglimits->maxDescriptorSetUpdateAfterBindSampledImages;
			ExtensionRelatedDatas.GETMAXDESC(DescType::IMAGE) = descindexinglimits->maxDescriptorSetUpdateAfterBindStorageImages;
			ExtensionRelatedDatas.GETMAXDESC(DescType::SBUFFER) = descindexinglimits->maxDescriptorSetUpdateAfterBindStorageBuffers;
			ExtensionRelatedDatas.GETMAXDESC(DescType::UBUFFER) = descindexinglimits->maxDescriptorSetUpdateAfterBindUniformBuffers;
			ExtensionRelatedDatas.GETMAXDESCALL() = descindexinglimits->maxUpdateAfterBindDescriptorsInAllPools;
			ExtensionRelatedDatas.GETMAXDESCPERSTAGE() = descindexinglimits->maxPerStageUpdateAfterBindResources;
			delete descindexinglimits;
		}
		else {
			ExtensionRelatedDatas.GETMAXDESC(DescType::SAMPLER) = Device_Properties.limits.maxPerStageDescriptorSampledImages;
			ExtensionRelatedDatas.GETMAXDESC(DescType::IMAGE) = Device_Properties.limits.maxPerStageDescriptorStorageImages;
			ExtensionRelatedDatas.GETMAXDESC(DescType::SBUFFER) = Device_Properties.limits.maxPerStageDescriptorStorageBuffers;
			ExtensionRelatedDatas.GETMAXDESC(DescType::UBUFFER) = Device_Properties.limits.maxPerStageDescriptorUniformBuffers;
			ExtensionRelatedDatas.GETMAXDESCALL() = Device_Properties.limits.maxPerStageResources;
			ExtensionRelatedDatas.GETMAXDESCPERSTAGE() = UINT32_MAX;
		}
	}
}
GPU::GPU(VkPhysicalDevice PhyDevice) : Physical_Device(PhyDevice) {
	Gather_PhysicalDeviceInformations();
	const char* VendorName = Convert_VendorID_toaString(Device_Properties.vendorID);

	//SAVE BASIC INFOs TO THE GPU DESC
	NAME = Device_Properties.deviceName;
	APIVER = Device_Properties.apiVersion;
	DRIVERVER = Device_Properties.driverVersion;

	//CHECK IF GPU IS DISCRETE OR INTEGRATED
	switch (Device_Properties.deviceType) {
	case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		GPUTYPE = tgfx_DISCRETE_GPU;
		break;
	case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		GPUTYPE = tgfx_INTEGRATED_GPU;
		break;
	default:
		LOG(tgfx_result_FAIL, ("Vulkan_Core::Check_Computer_Specs failed to find GPU's Type (Only Discrete and Integrated GPUs supported!), Type is:" +
			std::to_string(Device_Properties.deviceType)).c_str());
		break;
	}

	Analize_PhysicalDeviceMemoryProperties();

	std::vector<VkDeviceQueueCreateInfo> QueueCreationInfos(0);
	std::vector<std::vector<float>> QueuePriorityList(QueueFamiliesCount);
	//Queue Creation Processes
	for (unsigned int QueueFamIndex = 0; QueueFamIndex < QueueFamiliesCount; QueueFamIndex++) {
		VkQueueFamilyProperties& FamProp = QueueFamilyProperties[QueueFamIndex];
		QueuePriorityList[QueueFamIndex].resize(FamProp.queueCount);
		for (unsigned int QueueIndex = 0; QueueIndex < FamProp.queueCount; QueueIndex++) {
			QueuePriorityList[QueueFamIndex][QueueIndex] = 1.0f - (QueueIndex * (1.0f / FamProp.queueCount));
		}
		VkDeviceQueueCreateInfo QueueInfo = {};
		QueueInfo.flags = 0;
		QueueInfo.pNext = nullptr;
		QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueInfo.queueFamilyIndex = QueueFamIndex;
		QueueInfo.pQueuePriorities = QueuePriorityList[QueueFamIndex].data();
		QueueInfo.queueCount = FamProp.queueCount;
		QueueCreationInfos.push_back(QueueInfo);
	}

	VkDeviceCreateInfo Logical_Device_CreationInfo{};
	Logical_Device_CreationInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	Logical_Device_CreationInfo.flags = 0;
	Logical_Device_CreationInfo.pQueueCreateInfos = QueueCreationInfos.data();
	Logical_Device_CreationInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreationInfos.size());
	Activate_DeviceExtensions();
	LOG(tgfx_result_SUCCESS, "Activated Device Extensions");
	//This is to destroy datas of extending features
	DeviceExtendedFeatures extendedfeatures;
	Activate_DeviceFeatures(Logical_Device_CreationInfo, extendedfeatures);

	Logical_Device_CreationInfo.enabledExtensionCount = Active_DeviceExtensions.size();
	Logical_Device_CreationInfo.ppEnabledExtensionNames = Active_DeviceExtensions.data();
	Logical_Device_CreationInfo.pEnabledFeatures = &Active_Features;

	Logical_Device_CreationInfo.enabledLayerCount = 0;

	if (Device_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		if (vkCreateDevice(Physical_Device, &Logical_Device_CreationInfo, nullptr, &Logical_Device) != VK_SUCCESS) {
			LOG(tgfx_result_SUCCESS, "Vulkan failed to create a Logical Device!");
			return;
		}
		LOG(tgfx_result_SUCCESS, "After vkCreateDevice()");

		AllQueueFamilies = new uint32_t[QueueFamiliesCount];
		SortedQUEUEFAMsLIST = new unsigned int[QueueFamiliesCount];
		for (unsigned int QueueFamIndex = 0; QueueFamIndex < QueueFamiliesCount; QueueFamIndex++) {
			LOG(tgfx_result_SUCCESS, ("Queue Family Index: " + std::to_string(QueueFamIndex)).c_str());
			std::vector<VkQueue> QueueList(QueueFamilyProperties[QueueFamIndex].queueCount);
			vkGetDeviceQueue(Logical_Device, QueueFamIndex, 0, QueueList.data());
			LOG(tgfx_result_SUCCESS, ("After vkGetDeviceQueue() " + std::to_string(QueueFamIndex)).c_str());
			QUEUEFAMs[QueueFamIndex].Initialize(QueueFamilyProperties[QueueFamIndex], QueueFamIndex, QueueList);
			AllQueueFamilies[QueueFamIndex] = QueueFamIndex;
		}

		for (unsigned int QueueFamIndex = 0; QueueFamIndex < QueueFamiliesCount; QueueFamIndex++) {
			if (QUEUEFAMs[QueueFamIndex].QUEUEFLAG().is_GRAPHICSsupported) {
				GRAPHICS_QUEUEFamIndex = QueueFamIndex;
			}
			if (QUEUEFAMs[QueueFamIndex].QUEUEFLAG().is_COMPUTEsupported) {
				COMPUTE_supportedqueuecount++;
			}
			if (QUEUEFAMs[QueueFamIndex].QUEUEFLAG().is_TRANSFERsupported) {
				TRANSFERs_supportedqueuecount++;
			}
		}

		//Sort the queues by their feature count (Example: Element 0 is Transfer Only, Element 1 is Transfer-Compute, Element 2 is Graphics-Transfer-Compute etc)
		unsigned char LatestScore = 0;
		for (unsigned int i = 0; i < QueueFamiliesCount; i++) {
			bool isLowerScoreFound = false;
			for (unsigned int j = i; j < QueueFamiliesCount && !isLowerScoreFound; j++) {
				if (QUEUEFAMs[j].QUEUEFEATURESCORE() < QUEUEFAMs[i].QUEUEFEATURESCORE() && QUEUEFAMs[j].QUEUEFEATURESCORE() > LatestScore) {
					isLowerScoreFound = true;
					LatestScore = QUEUEFAMs[j].QUEUEFEATURESCORE();
				}
			}
			if (!isLowerScoreFound) {
				LatestScore = QUEUEFAMs[i].QUEUEFEATURESCORE();
			}
			SortedQUEUEFAMsLIST[i] = LatestScore;
		}


		LOG(tgfx_result_SUCCESS, "After vkGetDeviceQueue()");
		LOG(tgfx_result_SUCCESS, "Vulkan created a Logical Device!");

		Check_DeviceLimits();
		LOG(tgfx_result_SUCCESS, "After Check_DeviceLimits()");

		VKCORE->DEVICE_GPUs.push_back(this);
	}
	else {
		LOG(tgfx_result_WARNING, "RenderDoc doesn't support to create multiple vkDevices, so Device object isn't created for non-Discrete GPUs!");
	}

}
VK_QUEUEFAM* GPU::Find_BestQueue(const VK_QUEUEFLAG& Flag) {
	if (!Flag.isFlagValid()) {
		LOG(tgfx_result_FAIL, "(Find_BestQueue() has failed because flag is invalid!");
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
bool GPU::DoesQueue_Support(const VK_QUEUEFAM* QUEUEFAM, const VK_QUEUEFLAG& Flag) const  {
	const VK_QUEUEFLAG& supportflag = QUEUEFAM->QUEUEFLAG();
	if (Flag.doesntNeedAnything) {
		LOG(tgfx_result_FAIL, "(You should handle this type of flag in a special way, don't call this function for this type of flag!");
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
bool GPU::isWindowSupported(VkSurfaceKHR WindowSurface) {
	bool supported = false;
	for (unsigned int QueueIndex = 0; QueueIndex < RENDERGPU->QUEUEFAMSCOUNT(); QueueIndex++) {
		VkBool32 Does_Support = 0;
		vkGetPhysicalDeviceSurfaceSupportKHR(RENDERGPU->PHYSICALDEVICE(), RENDERGPU->QUEUEFAMS()[QueueIndex].QUEUEFAMINDEX(), WindowSurface, &Does_Support);
		if (Does_Support) {
			QUEUEFAMs[QueueIndex].SUPPORT_PRESENTATION();
			supported = true;
		}
	}
	return supported;
}

VK_CommandPool::VK_CommandPool() {

}
VK_CommandPool::VK_CommandPool(const VK_CommandPool& RefCP) {
	CPHandle = RefCP.CPHandle;
	CBs = RefCP.CBs;
}
void VK_CommandPool::operator=(const VK_CommandPool& RefCP) {
	CPHandle = RefCP.CPHandle;
	CBs = RefCP.CBs;
}
VK_QUEUE::~VK_QUEUE() {
	if (RenderGraphFences[0] != VK_Fence::INVALID_ID) {
		FENCESYS->DestroyFence(RenderGraphFences[0]);
	}
	if (RenderGraphFences[1] != VK_Fence::INVALID_ID) {
		FENCESYS->DestroyFence(RenderGraphFences[1]);
	}
}
VK_QUEUEFAM::~VK_QUEUEFAM() {
	delete[] QUEUEs;
}
VK_SubAllocation::VK_SubAllocation() : isEmpty(true) {

}
VK_SubAllocation::VK_SubAllocation(const VK_SubAllocation& copyblock) {
	isEmpty.store(copyblock.isEmpty.load());
	Size = copyblock.Size;
	Offset = copyblock.Offset;
}

void Fill_DepthAttachmentReference_SeperatedDSLayouts(VkAttachmentReference& Ref, unsigned int index, tgfx_texture_channels channels, tgfx_operationtype DEPTHOPTYPE, tgfx_operationtype STENCILOPTYPE) {
	Ref.attachment = index;
	if (channels == tgfx_texture_channels_D32) {
		switch (DEPTHOPTYPE) {
		case tgfx_operationtype_READ_ONLY:
			Ref.layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
		case tgfx_operationtype_READ_AND_WRITE:
		case tgfx_operationtype_WRITE_ONLY:
			Ref.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			break;
		case tgfx_operationtype_UNUSED:
			Ref.attachment = VK_ATTACHMENT_UNUSED;
			Ref.layout = VK_IMAGE_LAYOUT_UNDEFINED;
			break;
		default:
			LOG(tgfx_result_INVALIDARGUMENT, "VK::Fill_SubpassStructs() doesn't support this type of Operation Type for DepthBuffer!");
		}
	}
	else if (channels == tgfx_texture_channels_D24S8) {
		switch (STENCILOPTYPE) {
		case tgfx_operationtype_READ_ONLY:
			if (DEPTHOPTYPE == tgfx_operationtype_UNUSED || DEPTHOPTYPE == tgfx_operationtype_READ_ONLY) {
				Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			else if (DEPTHOPTYPE == tgfx_operationtype_READ_AND_WRITE || DEPTHOPTYPE == tgfx_operationtype_WRITE_ONLY) {
				Ref.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
			}
			break;
		case tgfx_operationtype_READ_AND_WRITE:
		case tgfx_operationtype_WRITE_ONLY:
			if (DEPTHOPTYPE == tgfx_operationtype_UNUSED || DEPTHOPTYPE == tgfx_operationtype_READ_ONLY) {
				Ref.layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else if (DEPTHOPTYPE == tgfx_operationtype_READ_AND_WRITE || DEPTHOPTYPE == tgfx_operationtype_WRITE_ONLY) {
				Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			break;
		case tgfx_operationtype_UNUSED:
			if (DEPTHOPTYPE == tgfx_operationtype_UNUSED) {
				Ref.attachment = VK_ATTACHMENT_UNUSED;
				Ref.layout = VK_IMAGE_LAYOUT_UNDEFINED;
			}
			else if (DEPTHOPTYPE == tgfx_operationtype_READ_AND_WRITE || DEPTHOPTYPE == tgfx_operationtype_WRITE_ONLY) {
				Ref.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
			}
			else if (DEPTHOPTYPE == tgfx_operationtype_READ_ONLY) {
				Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			break;
		default:
			LOG(tgfx_result_INVALIDARGUMENT, "VK::Fill_SubpassStructs() doesn't support this type of Operation Type for DepthSTENCILBuffer!");
		}
	}
}
void Fill_DepthAttachmentReference_NOSeperated(VkAttachmentReference& Ref, unsigned int index, tgfx_texture_channels channels, tgfx_operationtype DEPTHOPTYPE, tgfx_operationtype STENCILOPTYPE) {
	Ref.attachment = index;
	if (DEPTHOPTYPE == tgfx_operationtype_UNUSED) {
		Ref.attachment = VK_ATTACHMENT_UNUSED;
		Ref.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	}
	else {
		Ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}
}
void Fill_DepthAttachmentDescription_SeperatedDSLayouts(VkAttachmentDescription& Desc, VK_DEPTHSTENCILSLOT* DepthSlot) {
	Desc = {};
	Desc.format = Find_VkFormat_byTEXTURECHANNELs(DepthSlot->RT->CHANNELs);
	Desc.samples = VK_SAMPLE_COUNT_1_BIT;
	Desc.flags = 0;
	Desc.loadOp = Find_LoadOp_byGFXLoadOp(DepthSlot->DEPTH_LOAD);
	Desc.stencilLoadOp = Find_LoadOp_byGFXLoadOp(DepthSlot->STENCIL_LOAD);
	if (DepthSlot->IS_USED_LATER) {
		Desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	}
	else {
		Desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
	if (DepthSlot->RT->CHANNELs == tgfx_texture_channels_D32) {
		if (DepthSlot->DEPTH_OPTYPE == tgfx_operationtype_READ_ONLY) {
			Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
			Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
		}
		else {
			Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		}
	}
	else if (DepthSlot->RT->CHANNELs == tgfx_texture_channels_D24S8) {
		if (DepthSlot->DEPTH_OPTYPE == tgfx_operationtype_READ_ONLY) {
			if (DepthSlot->STENCIL_OPTYPE != tgfx_operationtype_READ_ONLY &&
				DepthSlot->STENCIL_OPTYPE != tgfx_operationtype_UNUSED) {
				Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
				Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else {
				Desc.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				Desc.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
		}
		else {
			if (DepthSlot->STENCIL_OPTYPE != tgfx_operationtype_READ_ONLY &&
				DepthSlot->STENCIL_OPTYPE != tgfx_operationtype_UNUSED) {
				Desc.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				Desc.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else {
				Desc.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
				Desc.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
			}
		}
	}
}
void Fill_DepthAttachmentDescription_NOSeperated(VkAttachmentDescription& Desc, VK_DEPTHSTENCILSLOT* DepthSlot) {
	Desc = {};
	Desc.format = Find_VkFormat_byTEXTURECHANNELs(DepthSlot->RT->CHANNELs);
	Desc.samples = VK_SAMPLE_COUNT_1_BIT;
	Desc.flags = 0;
	Desc.loadOp = Find_LoadOp_byGFXLoadOp(DepthSlot->DEPTH_LOAD);
	Desc.stencilLoadOp = Find_LoadOp_byGFXLoadOp(DepthSlot->STENCIL_LOAD);
	if (DepthSlot->IS_USED_LATER) {
		Desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	}
	else {
		Desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		Desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
	Desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	Desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

GPU::DeviceExtensions::DeviceExtensions() {
	Fill_DepthAttachmentReference = Fill_DepthAttachmentReference_NOSeperated;
	Fill_DepthAttachmentDescription = Fill_DepthAttachmentDescription_NOSeperated;
}
void GPU::DeviceExtensions::SUPPORT_DESCINDEXING() { isDescriptorIndexingSupported = true; }
void GPU::DeviceExtensions::SUPPORT_SWAWPCHAINDISPLAY() { SwapchainDisplay = true; }
void GPU::DeviceExtensions::SUPPORT_SEPERATEDDEPTHSTENCILLAYOUTS() {
	SeperatedDepthStencilLayouts = true;
	Fill_DepthAttachmentDescription = Fill_DepthAttachmentDescription_SeperatedDSLayouts;
	Fill_DepthAttachmentReference = Fill_DepthAttachmentReference_SeperatedDSLayouts;
}

VK_MemoryAllocation::VK_MemoryAllocation() : Allocated_Blocks(JobSys), MemoryTypeIndex(0){

}
VK_MemoryAllocation::VK_MemoryAllocation(const VK_MemoryAllocation& copyALLOC) : UnusedSize(copyALLOC.UnusedSize.DirectLoad()),
Allocated_Blocks(JobSys), Buffer(copyALLOC.Buffer), Allocated_Memory(copyALLOC.Allocated_Memory), TYPE(copyALLOC.TYPE),
MappedMemory(copyALLOC.MappedMemory), MemoryTypeIndex(copyALLOC.MemoryTypeIndex), MaxSize(copyALLOC.MaxSize) {

}





VkFormat Find_VkFormat_byDataType(tgfx_datatype datatype) {
	switch (datatype) {
	case tgfx_datatype_VAR_VEC2:
		return VK_FORMAT_R32G32_SFLOAT;
	case tgfx_datatype_VAR_VEC3:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case tgfx_datatype_VAR_VEC4:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	default:
		LOG(tgfx_result_FAIL, "(Find_VkFormat_byDataType() doesn't support this data type! UNDEFINED");
			return VK_FORMAT_UNDEFINED;
	}
}
VkFormat Find_VkFormat_byTEXTURECHANNELs(tgfx_texture_channels channels) {
	switch (channels) {
	case tgfx_texture_channels_BGRA8UNORM:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case tgfx_texture_channels_BGRA8UB:
		return VK_FORMAT_B8G8R8A8_UINT;
	case tgfx_texture_channels_RGBA8UB:
		return VK_FORMAT_R8G8B8A8_UINT;
	case tgfx_texture_channels_RGBA8B:
		return VK_FORMAT_R8G8B8A8_SINT;
	case tgfx_texture_channels_RGBA32F:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case tgfx_texture_channels_RGB8UB:
		return VK_FORMAT_R8G8B8_UINT;
	case tgfx_texture_channels_D32:
		return VK_FORMAT_D32_SFLOAT;
	case tgfx_texture_channels_D24S8:
		return VK_FORMAT_D24_UNORM_S8_UINT;
	default:
		LOG(tgfx_result_FAIL, "(Find_VkFormat_byTEXTURECHANNELs doesn't support this type of channel!");
			return VK_FORMAT_UNDEFINED;
	}
}
VkDescriptorType Find_VkDescType_byMATDATATYPE(tgfx_shaderinput_type TYPE) {
	switch (TYPE) {
	case tgfx_shaderinput_type_UBUFFER_G:
	case tgfx_shaderinput_type_UBUFFER_PI:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case tgfx_shaderinput_type_SBUFFER_G:
	case tgfx_shaderinput_type_SBUFFER_PI:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case tgfx_shaderinput_type_SAMPLER_G:
	case tgfx_shaderinput_type_SAMPLER_PI:
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case tgfx_shaderinput_type_IMAGE_G:
	case tgfx_shaderinput_type_IMAGE_PI:
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case tgfx_shaderinput_type_UNDEFINED:
		LOG(tgfx_result_FAIL, "(Find_VkDescType_byMATDATATYPE() has failed because SHADERINPUT_TYPE = UNDEFINED!");
	default:
		LOG(tgfx_result_FAIL, "(Find_VkDescType_byMATDATATYPE() doesn't support this type of SHADERINPUT_TYPE!");
	}
	return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}
VkDescriptorType Find_VkDescType_byDescTypeCategoryless(DescType type) {
	switch (type)
	{
	case DescType::IMAGE:
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case DescType::SAMPLER:
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case DescType::UBUFFER:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case DescType::SBUFFER:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	default:
		LOG(tgfx_result_FAIL, "(Find_VkDescType_byDescTypeCategoryless() doesn't support this type!");
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

VkSamplerAddressMode Find_AddressMode_byWRAPPING(tgfx_texture_wrapping Wrapping) {
	switch (Wrapping) {
	case tgfx_texture_wrapping_REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case tgfx_texture_wrapping_MIRRORED_REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case tgfx_texture_wrapping_CLAMP_TO_EDGE:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	default:
		LOG(tgfx_result_INVALIDARGUMENT, "Find_AddressMode_byWRAPPING() doesn't support this wrapping type!");
		return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
	}
}

VkFilter Find_VkFilter_byGFXFilter(tgfx_texture_mipmapfilter filter) {
	switch (filter) {
	case tgfx_texture_mipmapfilter::LINEAR_FROM_1MIP:
	case tgfx_texture_mipmapfilter::LINEAR_FROM_2MIP:
		return VK_FILTER_LINEAR;
	case tgfx_texture_mipmapfilter::NEAREST_FROM_1MIP:
	case tgfx_texture_mipmapfilter::NEAREST_FROM_2MIP:
		return VK_FILTER_NEAREST;
	default:
		LOG(tgfx_result_INVALIDARGUMENT, "Find_VkFilter_byGFXFilter() doesn't support this filter type!");
		return VK_FILTER_MAX_ENUM;
	}
}
VkSamplerMipmapMode Find_MipmapMode_byGFXFilter(tgfx_texture_mipmapfilter filter) {
	switch (filter) {
	case tgfx_texture_mipmapfilter::LINEAR_FROM_2MIP:
	case tgfx_texture_mipmapfilter::NEAREST_FROM_2MIP:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	case tgfx_texture_mipmapfilter::LINEAR_FROM_1MIP:
	case tgfx_texture_mipmapfilter::NEAREST_FROM_1MIP:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}
}

VkCullModeFlags Find_CullMode_byGFXCullMode(tgfx_cullmode mode) {
	switch (mode)
	{
	case tgfx_cullmode::CULL_OFF:
		return VK_CULL_MODE_NONE;
		break;
	case tgfx_cullmode::CULL_BACK:
		return VK_CULL_MODE_BACK_BIT;
		break;
	case tgfx_cullmode::CULL_FRONT:
		return VK_CULL_MODE_FRONT_BIT;
		break;
	default:
		LOG(tgfx_result_INVALIDARGUMENT, "This culling type isn't supported by Find_CullMode_byGFXCullMode()!");
		return VK_CULL_MODE_NONE;
		break;
	}
}
VkPolygonMode Find_PolygonMode_byGFXPolygonMode(tgfx_polygonmode mode) {
	switch (mode)
	{
	case tgfx_polygonmode_FILL:
		return VK_POLYGON_MODE_FILL;
		break;
	case tgfx_polygonmode_LINE:
		return VK_POLYGON_MODE_LINE;
		break;
	case tgfx_polygonmode_POINT:
		return VK_POLYGON_MODE_POINT;
		break;
	default:
		LOG(tgfx_result_INVALIDARGUMENT, "This polygon mode isn't support by Find_PolygonMode_byGFXPolygonMode()");
		break;
	}
}
VkPrimitiveTopology Find_PrimitiveTopology_byGFXVertexListType(tgfx_vertexlisttypes vertexlist) {
	switch (vertexlist)
	{
	case tgfx_vertexlisttypes::TRIANGLELIST:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	default:
		LOG(tgfx_result_INVALIDARGUMENT, "This type of vertex list is not supported by Find_PrimitiveTopology_byGFXVertexListType()");
		return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
		break;
	}
}
VkIndexType Find_IndexType_byGFXDATATYPE(tgfx_datatype datatype) {
	switch (datatype)
	{
	case tgfx_datatype_VAR_UINT32:
		return VK_INDEX_TYPE_UINT32;
	case tgfx_datatype_VAR_UINT16:
		return VK_INDEX_TYPE_UINT16;
	default:
		LOG(tgfx_result_INVALIDARGUMENT, "This type of data isn't supported by Find_IndexType_byGFXDATATYPE()");
		return VK_INDEX_TYPE_MAX_ENUM;
	}
}
VkCompareOp Find_CompareOp_byGFXDepthTest(tgfx_depthtests test) {
	switch (test) {
	case tgfx_DEPTH_TEST_NEVER:
		return VK_COMPARE_OP_NEVER;
	case tgfx_DEPTH_TEST_ALWAYS:
		return VK_COMPARE_OP_ALWAYS;
	case tgfx_DEPTH_TEST_GEQUAL:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case tgfx_DEPTH_TEST_GREATER:
		return VK_COMPARE_OP_GREATER;
	case tgfx_DEPTH_TEST_LEQUAL:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case tgfx_DEPTH_TEST_LESS:
		return VK_COMPARE_OP_LESS;
	default:
		LOG(tgfx_result_INVALIDARGUMENT, "Find_CompareOp_byGFXDepthTest() doesn't support this type of test!");
		return VK_COMPARE_OP_MAX_ENUM;
	}
}

void Find_DepthMode_byGFXDepthMode(tgfx_depthmodes mode, VkBool32& ShouldTest, VkBool32& ShouldWrite) {
	switch (mode)
	{
	case tgfx_depthmode_READ_WRITE:
		ShouldTest = VK_TRUE;
		ShouldWrite = VK_TRUE;
		break;
	case tgfx_depthmode_READ_ONLY:
		ShouldTest = VK_TRUE;
		ShouldWrite = VK_FALSE;
		break;
	case tgfx_depthmode_OFF:
		ShouldTest = VK_FALSE;
		ShouldWrite = VK_FALSE;
		break;
	default:
		LOG(tgfx_result_INVALIDARGUMENT, "Find_DepthMode_byGFXDepthMode() doesn't support this type of depth mode!");
		break;
	}
}

VkAttachmentLoadOp Find_LoadOp_byGFXLoadOp(tgfx_drawpassload load) {
	switch (load) {
	case tgfx_drawpassload_CLEAR:
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case tgfx_drawpassload_FULL_OVERWRITE:
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	case tgfx_drawpassload_LOAD:
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	default:
		LOG(tgfx_result_INVALIDARGUMENT, "Find_LoadOp_byGFXLoadOp() doesn't support this type of load!");
		return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
	}
}

VkCompareOp Find_CompareOp_byGFXStencilCompare(tgfx_stencilcompare op) {
	switch (op)
	{
	case tgfx_stencilcompare::NEVER_PASS:
		return VK_COMPARE_OP_NEVER;
	case tgfx_stencilcompare::LESS_PASS:
		return VK_COMPARE_OP_LESS;
	case tgfx_stencilcompare::LESS_OR_EQUAL_PASS:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case tgfx_stencilcompare::EQUAL_PASS:
		return VK_COMPARE_OP_EQUAL;
	case tgfx_stencilcompare::NOTEQUAL_PASS:
		return VK_COMPARE_OP_NOT_EQUAL;
	case tgfx_stencilcompare::GREATER_OR_EQUAL_PASS:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case tgfx_stencilcompare::GREATER_PASS:
		return VK_COMPARE_OP_GREATER;
	case tgfx_stencilcompare::OFF:
	case tgfx_stencilcompare::ALWAYS_PASS:
		return VK_COMPARE_OP_ALWAYS;
	default:
		LOG(tgfx_result_INVALIDARGUMENT, "Find_CompareOp_byGFXStencilCompare() doesn't support this type of stencil compare!");
		return VK_COMPARE_OP_ALWAYS;
	}
}

VkStencilOp Find_StencilOp_byGFXStencilOp(tgfx_stencilop op) {
	switch (op)
	{
	case tgfx_stencilop_DONT_CHANGE:
		return VK_STENCIL_OP_KEEP;
	case tgfx_stencilop_SET_ZERO:
		return VK_STENCIL_OP_ZERO;
	case tgfx_stencilop_CHANGE:
		return VK_STENCIL_OP_REPLACE;
	case tgfx_stencilop_CLAMPED_INCREMENT:
		return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
	case tgfx_stencilop_WRAPPED_INCREMENT:
		return VK_STENCIL_OP_INCREMENT_AND_WRAP;
	case tgfx_stencilop_CLAMPED_DECREMENT:
		return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
	case tgfx_stencilop_WRAPPED_DECREMENT:
		return VK_STENCIL_OP_DECREMENT_AND_WRAP;
	case tgfx_stencilop_BITWISE_INVERT:
		return VK_STENCIL_OP_INVERT;
	default:
		LOG(tgfx_result_INVALIDARGUMENT, "Find_StencilOp_byGFXStencilOp() doesn't support this type of stencil operation!");
		return VK_STENCIL_OP_KEEP;
	}
}
VkBlendOp Find_BlendOp_byGFXBlendMode(tgfx_blendmode mode) {
	switch (mode)
	{
	case tgfx_blendmode_ADDITIVE:
		return VK_BLEND_OP_ADD;
	case tgfx_blendmode_SUBTRACTIVE:
		return VK_BLEND_OP_SUBTRACT;
	case tgfx_blendmode_SUBTRACTIVE_SWAPPED:
		return VK_BLEND_OP_REVERSE_SUBTRACT;
	case tgfx_blendmode_MIN:
		return VK_BLEND_OP_MIN;
	case tgfx_blendmode_MAX:
		return VK_BLEND_OP_MAX;
	default:
		LOG(tgfx_result_INVALIDARGUMENT, "Find_BlendOp_byGFXBlendMode() doesn't support this type of blend mode!");
		return VK_BLEND_OP_MAX_ENUM;
	}
}
VkBlendFactor Find_BlendFactor_byGFXBlendFactor(tgfx_blendfactor factor) {
	switch (factor)
	{
	case tgfx_blendfactor_ONE:
		return VK_BLEND_FACTOR_ONE;
	case tgfx_blendfactor_ZERO:
		return VK_BLEND_FACTOR_ZERO;
	case tgfx_blendfactor_SRC_COLOR:
		return VkBlendFactor::VK_BLEND_FACTOR_SRC_COLOR;
	case tgfx_blendfactor_SRC_1MINUSCOLOR:
		return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case tgfx_blendfactor_SRC_ALPHA:
		return VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;
	case tgfx_blendfactor_SRC_1MINUSALPHA:
		return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case tgfx_blendfactor_DST_COLOR:
		return VkBlendFactor::VK_BLEND_FACTOR_DST_COLOR;
	case tgfx_blendfactor_DST_1MINUSCOLOR:
		return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case tgfx_blendfactor_DST_ALPHA:
		return VkBlendFactor::VK_BLEND_FACTOR_DST_ALPHA;
	case tgfx_blendfactor_DST_1MINUSALPHA:
		return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	case tgfx_blendfactor_CONST_COLOR:
		return VkBlendFactor::VK_BLEND_FACTOR_CONSTANT_COLOR;
	case tgfx_blendfactor_CONST_1MINUSCOLOR:
		return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
	case tgfx_blendfactor_CONST_ALPHA:
		return VkBlendFactor::VK_BLEND_FACTOR_CONSTANT_ALPHA;
	case tgfx_blendfactor_CONST_1MINUSALPHA:
		return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
	default:
		LOG(tgfx_result_INVALIDARGUMENT, "Find_BlendFactor_byGFXBlendFactor() doesn't support this type of blend factor!");
		return VK_BLEND_FACTOR_MAX_ENUM;
	}
}
void Fill_ComponentMapping_byCHANNELs(tgfx_texture_channels channels, VkComponentMapping& mapping) {
	switch (channels)
	{
	case tgfx_texture_channels_D32:
	case tgfx_texture_channels_D24S8:
	case tgfx_texture_channels_BGRA8UB:
	case tgfx_texture_channels_BGRA8UNORM:
	case tgfx_texture_channels_RGBA32F:
	case tgfx_texture_channels_RGBA32UI:
	case tgfx_texture_channels_RGBA32I:
	case tgfx_texture_channels_RGBA8UB:
	case tgfx_texture_channels_RGBA8B:
		mapping.r = VK_COMPONENT_SWIZZLE_R;
		mapping.g = VK_COMPONENT_SWIZZLE_G;
		mapping.b = VK_COMPONENT_SWIZZLE_B;
		mapping.a = VK_COMPONENT_SWIZZLE_A;
		return;
	case tgfx_texture_channels_RGB32F:
	case tgfx_texture_channels_RGB32UI:
	case tgfx_texture_channels_RGB32I:
	case tgfx_texture_channels_RGB8UB:
	case tgfx_texture_channels_RGB8B:
		mapping.r = VK_COMPONENT_SWIZZLE_R;
		mapping.g = VK_COMPONENT_SWIZZLE_G;
		mapping.b = VK_COMPONENT_SWIZZLE_B;
		mapping.a = VK_COMPONENT_SWIZZLE_ZERO;
		return;
	case tgfx_texture_channels_RA32F:
	case tgfx_texture_channels_RA32UI:
	case tgfx_texture_channels_RA32I:
	case tgfx_texture_channels_RA8UB:
	case tgfx_texture_channels_RA8B:
		mapping.r = VK_COMPONENT_SWIZZLE_R;
		mapping.g = VK_COMPONENT_SWIZZLE_ZERO;
		mapping.b = VK_COMPONENT_SWIZZLE_ZERO;
		mapping.a = VK_COMPONENT_SWIZZLE_A;
		return;
	case tgfx_texture_channels_R32F:
	case tgfx_texture_channels_R32UI:
	case tgfx_texture_channels_R32I:
	case tgfx_texture_channels_R8UB:
	case tgfx_texture_channels_R8B:
		mapping.r = VK_COMPONENT_SWIZZLE_R;
		mapping.g = VK_COMPONENT_SWIZZLE_ZERO;
		mapping.b = VK_COMPONENT_SWIZZLE_ZERO;
		mapping.a = VK_COMPONENT_SWIZZLE_ZERO;
		return;
	default:
		break;
	}
}
void Find_SubpassAccessPattern(tgfx_subdrawpass_access access, bool isSource, VkPipelineStageFlags& stageflag, VkAccessFlags& accessflag) {
	switch (access)
	{
	case tgfx_subdrawpass_access_ALLCOMMANDS:
		if (isSource) {
			stageflag |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}
		else {
			stageflag |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
		break;
	case tgfx_subdrawpass_access_INDEX_READ:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		accessflag |= VK_ACCESS_INDEX_READ_BIT;
		break;
	case tgfx_subdrawpass_access_VERTEXATTRIB_READ:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		accessflag |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		break;
	case tgfx_subdrawpass_access_VERTEXUBUFFER_READONLY:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		accessflag |= VK_ACCESS_UNIFORM_READ_BIT;
		break;
	case tgfx_subdrawpass_access_VERTEXSBUFFER_READONLY:
	case tgfx_subdrawpass_access_VERTEXSAMPLED_READONLY:
	case tgfx_subdrawpass_access_VERTEXIMAGE_READONLY:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT;
		break;
	case tgfx_subdrawpass_access_VERTEXSBUFFER_READWRITE:
	case tgfx_subdrawpass_access_VERTEXIMAGE_READWRITE:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		break;
	case tgfx_subdrawpass_access_VERTEXIMAGE_WRITEONLY:
	case tgfx_subdrawpass_access_VERTEXSBUFFER_WRITEONLY:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
		break;
	case tgfx_subdrawpass_access_VERTEXINPUTS_READONLY:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
		break;
	case tgfx_subdrawpass_access_VERTEXINPUTS_READWRITE:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		break;
	case tgfx_subdrawpass_access_VERTEXINPUTS_WRITEONLY:
		stageflag |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
		break;

	case tgfx_subdrawpass_access_EARLY_Z_READ:
		stageflag |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		break;
	case tgfx_subdrawpass_access_EARLY_Z_READWRITE:
		stageflag |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case tgfx_subdrawpass_access_EARLY_Z_WRITEONLY:
		stageflag |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case tgfx_subdrawpass_access_FRAGMENTUBUFFER_READONLY:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_UNIFORM_READ_BIT;
		break;
	case tgfx_subdrawpass_access_FRAGMENTSBUFFER_READONLY:
	case tgfx_subdrawpass_access_FRAGMENTSAMPLED_READONLY:
	case tgfx_subdrawpass_access_FRAGMENTIMAGE_READONLY:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT;
		break;
	case tgfx_subdrawpass_access_FRAGMENTSBUFFER_READWRITE:
	case tgfx_subdrawpass_access_FRAGMENTIMAGE_READWRITE:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		break;
	case tgfx_subdrawpass_access_FRAGMENTIMAGE_WRITEONLY:
	case tgfx_subdrawpass_access_FRAGMENTSBUFFER_WRITEONLY:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
		break;
	case tgfx_subdrawpass_access_FRAGMENTINPUTS_READONLY:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
		break;
	case tgfx_subdrawpass_access_FRAGMENTINPUTS_READWRITE:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		break;
	case tgfx_subdrawpass_access_FRAGMENTINPUTS_WRITEONLY:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_SHADER_WRITE_BIT;
		break;
	case tgfx_subdrawpass_access_FRAGMENTRT_READONLY:
		stageflag |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessflag |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		break;
	case tgfx_subdrawpass_access_LATE_Z_READ:
		stageflag |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		break;
	case tgfx_subdrawpass_access_LATE_Z_READWRITE:
		stageflag |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case tgfx_subdrawpass_access_LATE_Z_WRITEONLY:
		stageflag |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		accessflag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case tgfx_subdrawpass_access_FRAGMENTRT_WRITEONLY:
		stageflag |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		accessflag |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	default:
		break;
	}
}
DescType Find_DescType_byGFXShaderInputType(tgfx_shaderinput_type dtype) {
	switch (dtype)
	{
	case tgfx_shaderinput_type_SAMPLER_PI:
	case tgfx_shaderinput_type_SAMPLER_G:
		return DescType::SAMPLER;
	case tgfx_shaderinput_type_IMAGE_PI:
	case tgfx_shaderinput_type_IMAGE_G:
		return DescType::IMAGE;
	case tgfx_shaderinput_type_UBUFFER_PI:
	case tgfx_shaderinput_type_UBUFFER_G:
		return DescType::UBUFFER;
	case tgfx_shaderinput_type_SBUFFER_PI:
	case tgfx_shaderinput_type_SBUFFER_G:
		return DescType::SBUFFER;
	default:
		LOG(tgfx_result_FAIL, "(Find_DescType_byGFXShaderInputType() doesn't support this type!");
			return DescType::SAMPLER;
	}
}

//Don't use this functions outside of the FindAvailableOffset
VkDeviceSize CalculateOffset(VkDeviceSize baseoffset, VkDeviceSize AlignmentOffset, VkDeviceSize ReqAlignment) {
	VkDeviceSize FinalOffset = 0;
	LOG(tgfx_result_NOTCODED, "VulkanBackend was using C++17's LCM before, but because LCM is a basic math please fix the backend to use a custom math code. C++17 for this feature is unnecessary!");
	//VkDeviceSize LCM = std::lcm(AlignmentOffset, ReqAlignment);
	//FinalOffset = (baseoffset % LCM) ? (((baseoffset / LCM) + 1) * LCM) : baseoffset;
	return FinalOffset;
}
VkDeviceSize VK_MemoryAllocation::FindAvailableOffset(VkDeviceSize RequiredSize, VkDeviceSize AlignmentOffset, VkDeviceSize RequiredAlignment) {
	VkDeviceSize FurthestOffset = 0;
	if (AlignmentOffset && !RequiredAlignment) {
		RequiredAlignment = AlignmentOffset;
	}
	else if (!AlignmentOffset && RequiredAlignment) {
		AlignmentOffset = RequiredAlignment;
	}
	else if (!AlignmentOffset && !RequiredAlignment) {
		AlignmentOffset = 1;
		RequiredAlignment = 1;
	}
	for (unsigned int ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
		for (unsigned int i = 0; i < Allocated_Blocks.size(ThreadID); i++) {
			VK_SubAllocation& Block = Allocated_Blocks.get(ThreadID, i);
			if (FurthestOffset <= Block.Offset) {
				FurthestOffset = Block.Offset + Block.Size;
			}
			if (!Block.isEmpty.load()) {
				continue;
			}

			VkDeviceSize Offset = CalculateOffset(Block.Offset, AlignmentOffset, RequiredAlignment);

			if (Offset + RequiredSize - Block.Offset > Block.Size ||
				Offset + RequiredSize - Block.Offset < (Block.Size / 5) * 3) {
				continue;
			}
			bool x = true, y = false;
			//Try to get the block first (Concurrent usages are prevented that way)
			if (!Block.isEmpty.compare_exchange_strong(x, y)) {
				continue;
			}
			//Don't change the block's own offset, because that'd probably cause shifting offset the memory block after free-reuse-free-reuse sequences
			return Offset;
		}
	}

	//None of the current blocks is suitable, so create a new block in this thread's local memoryblocks list
	VK_SubAllocation newblock;
	newblock.isEmpty.store(false);
	newblock.Offset = CalculateOffset(FurthestOffset, AlignmentOffset, RequiredAlignment);
	newblock.Size = RequiredSize + newblock.Offset - FurthestOffset;
	Allocated_Blocks.push_back(tapi_GetThisThreadIndex(JobSys), newblock);
	return newblock.Offset;
}
void VK_MemoryAllocation::FreeBlock(VkDeviceSize Offset) {
	std::unique_lock<std::mutex> Locker;
	Allocated_Blocks.PauseAllOperations(Locker);
	for (unsigned char ThreadID = 0; ThreadID < tapi_GetThreadCount(JobSys); ThreadID++) {
		for (unsigned int i = 0; i < Allocated_Blocks.size(ThreadID); i++) {
			VK_SubAllocation& Block = Allocated_Blocks.get(ThreadID, i);
			if (Block.isEmpty.load()) {
				continue;
			}
			if (Block.Offset <= Offset && Block.Offset + Block.Size > Offset) {
				Block.isEmpty.store(true);
				return;
			}
		}
	}
}