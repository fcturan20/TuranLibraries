#include <iostream>
#include <string>

tapi_threadingsystem ThreadSys = nullptr;
void Job() {
	std::cout << "Job is called!" << std::endl;
}

void Callback(result_tgfx result, const char* Text) {
	std::cout << ((result == result_tgfx_NOTCODED || result == result_tgfx_FAIL) ? ("A notcoded or failed GFX function is called: ") : (""))<< Text << std::endl;
#ifdef _DEBUG
	if (result == result_tgfx_NOTCODED || result == result_tgfx_FAIL) {
		std::cout << "Error breakpoint!\n";
	}
#endif
}

void WindowCallback(tgfx_window WindowHandle, void* UserPTR, unsigned int WIDTH, unsigned int HEIGHT, tgfx_texture* SwapchainTextures) {
	std::cout << "Window is resizing to x: " << WIDTH << " y: " << HEIGHT << "\n";
}

void ThreadedMain() {
	std::cout << "Naber from main!" << std::endl;
	tgfx_CORE* core = tgfx_Create_GFXAPI(tgfx_BACKENDs::VULKAN);
	core->DebugCallback = Callback;
	tgfx_gpu_list DeviceGPUs;
	core->GetGPUList(&DeviceGPUs);
	TGFXLISTCOUNT(core, DeviceGPUs, GPUCOUNT);
	std::cout << "GPU count: " << std::to_string(GPUCOUNT) << std::endl;
	const char* NAME = nullptr; unsigned int APIVER = 0, DRIVERVER = 0;
	tgfx_gpu_types GPUTYPE; tgfx_memorytype_list MemoryAllocList;
	unsigned char isGraphicsSupported = 0, isComputeSupported = 0, isTransferSupported = 0;
	core->Helpers.GetGPUInfo_General(DeviceGPUs[0], &NAME, &APIVER, &DRIVERVER, &GPUTYPE, &MemoryAllocList, &isGraphicsSupported, &isComputeSupported, &isTransferSupported);
	TGFXLISTCOUNT(core, MemoryAllocList, MEMORYALLOCCOUNT);
	std::cout << "Memory allocation count: " << MEMORYALLOCCOUNT << std::endl;
	for (unsigned int i = 0; i < MEMORYALLOCCOUNT; i++) {
		tgfx_memoryallocationtype ALLOCTYPE; unsigned long long MAXSIZE = UINT32_MAX;
		core->Helpers.GetGPUInfo_Memory(MemoryAllocList[i], &ALLOCTYPE, &MAXSIZE);
		core->Helpers.SetMemoryTypeInfo(MemoryAllocList[i], 10 * 1024 * 1024, nullptr);
		std::cout << std::endl;
	}
	result_tgfx extensionresult;
	std::cout << "Desc indexing support: " << unsigned int(core->Helpers.DoesGPUsupportsVKDESCINDEXING(DeviceGPUs[0])) << std::endl;
	tgfx_initializationsecondstageinfo secondinitialize = core->Helpers.Create_GFXInitializationSecondStageInfo(DeviceGPUs[0], 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 1, nullptr);
	core->Initialize_SecondStage(secondinitialize);
	tgfx_monitor_list Monitors;
	core->GetMonitorList(&Monitors);
	tgfx_texture SwapchainTextures[2];
	tgfx_window MainWindow;
	core->CreateWindow(1280, 720, Monitors[0], tgfx_windowmode::WINDOWED, "Main Window", core->Helpers.CreateTextureUsageFlag(1, 1, 1, 1, 1), WindowCallback, nullptr, SwapchainTextures, &MainWindow);
	core->Renderer.Start_RenderGraphConstruction();
	Execute_withoutWait(ThreadSys, Job);
}


int main() {
	JobSystem_Start(&ThreadSys, ThreadedMain);
	std::cout << "Returned to main()!\n";
	return 1;
}