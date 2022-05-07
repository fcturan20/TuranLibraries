#include "predefinitions_dx.h"
#include "core.h"

//Load TuranLibraries
#include <registrysys_tapi.h>
#include <virtualmemorysys_tapi.h>
#include <threadingsys_tapi.h>
#include <tgfx_core.h>
#include <tgfx_helper.h>
#include <tgfx_renderer.h>
#include <tgfx_gpucontentmanager.h>
#include <tgfx_imgui.h>
#include <vector>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

struct core_private {
public:
	std::vector<GPU_VKOBJ*> DEVICE_GPUs;
	//Window Operations
	std::vector<monitor_dx*> MONITORs;
	std::vector<window_dx*> WINDOWs;


	bool isAnyWindowResized = false;	//Instead of checking each window each frame, just check this
};
static core_private* hidden = nullptr;

void GLFWwindowresizecallback(GLFWwindow* glfwwindow, int width, int height) {
	window_dx* vkwindow = (window_dx*)glfwGetWindowUserPointer(glfwwindow);
	vkwindow->resize_cb((window_tgfx_handle)vkwindow, vkwindow->UserPTR, width, height, (texture_tgfx_handle*)vkwindow->Swapchain_Textures);
}
bool Create_WindowSwapchain(window_dx* new_window, unsigned int WIDTH, unsigned int HEIGHT, ComPtr<IDXGISwapChain4>* SwapchainOBJ, texture_dx** SwapchainTextureHandles) {
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = WIDTH;
	swapChainDesc.Height = HEIGHT;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = 0;

	ComPtr<IDXGISwapChain1> swapChain1; 
	ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
		commandQueue.Get(),
		new_window->Window_Handle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1));
	ThrowIfFailed(dxgiFactory->MakeWindowAssociation(new_window->Window_Handle, DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(swapChain1.As(&new_window->Swapchain_Handle));
	

	return true;
}

struct core_functions_dx {
public:
	static void initialize_secondstage(initializationsecondstageinfo_tgfx_handle info);
	//SwapchainTextureHandles should point to an array of 2 elements! Triple Buffering is not supported for now.
	static void createwindow(unsigned int WIDTH, unsigned int HEIGHT, monitor_tgfx_handle monitor,
		windowmode_tgfx Mode, const char* NAME, textureusageflag_tgfx_handle SwapchainUsage, tgfx_windowResizeCallback ResizeCB,
		void* UserPointer, texture_tgfx_handle* SwapchainTextureHandles, window_tgfx_handle* window);
	static void change_window_resolution(window_tgfx_handle WindowHandle, unsigned int width, unsigned int height);
	static void getmonitorlist(monitor_tgfx_listhandle* MonitorList);
	static void getGPUlist(gpu_tgfx_listhandle* GpuList);

	//Debug callbacks are user defined callbacks, you should assign the function pointer if you want to use them
	//As default, all backends set them as empty no-work functions
	static void debugcallback(result_tgfx result, const char* Text);
	//You can set this if TGFX is started with threaded call.
	static void debugcallback_threaded(unsigned char ThreadIndex, result_tgfx Result, const char* Text);


	static void destroy_tgfx_resources();
	static void take_inputs();

	static void Save_Monitors();
	static void Create_Instance();
	static void Setup_Debugging();
	static void Check_Computer_Specs();

};

inline void ThrowIfFailed(HRESULT hr){
    if (FAILED(hr))
    {
        throw std::exception();
    }
}
void printf_log_tgfx(result_tgfx result, const char* text) {
	printf("TGFX %u: %s\n", (unsigned int)result, text);
}
void GFX_Error_Callback(int error_code, const char* description) {
	printer(result_tgfx_FAIL, (std::string("GLFW error: ") + description).c_str());
}
extern void set_helper_functions();
extern "C" FUNC_DLIB_EXPORT result_tgfx backend_load(registrysys_tapi * regsys, core_tgfx_type * core, tgfx_PrintLogCallback printcallback) {
	if (!regsys->get(TGFX_PLUGIN_NAME, TGFX_PLUGIN_VERSION, 0))
		return result_tgfx_FAIL;

	core->api->INVALIDHANDLE = regsys->get(VIRTUALMEMORY_TAPI_PLUGIN_NAME, VIRTUALMEMORY_TAPI_PLUGIN_VERSION, 0);
	//Check if threading system is loaded
	THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE threadsysloaded = (THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE)regsys->get(THREADINGSYS_TAPI_PLUGIN_NAME, THREADINGSYS_TAPI_PLUGIN_VERSION, 0);
	if (threadsysloaded) {
		threadingsys = threadsysloaded->funcs;
	}

	core_dx = new core_public;
	core->data = (core_tgfx_d*)core_dx;
	core_tgfx_main = core->api;
	hidden = new core_private;

	if (printcallback) { printer = printcallback; }
	else { printer = printf_log_tgfx; }


	hInstance = GetModuleHandle(NULL);

	set_helper_functions();

	//Set error callback to handle all glfw errors (including initialization error)!
	glfwSetErrorCallback(GFX_Error_Callback);

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	core_functions_dx::Save_Monitors();
#ifdef 	TD3D12_DEBUGGING
	core_functions_dx::Setup_Debugging();
#endif
	core_functions_dx::Check_Computer_Specs();

	window_tgfx_handle firstwindow;
	core_functions_dx::createwindow(1280, 720, nullptr, windowmode_tgfx_WINDOWED, "First Window", nullptr, nullptr, nullptr, nullptr, &firstwindow);
}




void core_functions_dx::createwindow(unsigned int WIDTH, unsigned int HEIGHT, monitor_tgfx_handle monitor,
	windowmode_tgfx Mode, const char* NAME, textureusageflag_tgfx_handle SwapchainUsage, tgfx_windowResizeCallback ResizeCB,
	void* UserPointer, texture_tgfx_handle* SwapchainTextureHandles, window_tgfx_handle* window) {
	if (ResizeCB) { glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); }
	else { glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); }


	//Create window as it will share resources with Renderer Context to get display texture!
	monitor_dx* MONITOR = (monitor_dx*)monitor;
	GLFWwindow* glfw_window = glfwCreateWindow(WIDTH, HEIGHT, NAME, (Mode == windowmode_tgfx_FULLSCREEN) ? (MONITOR->monitorobj) : (NULL), nullptr);
	window_dx* new_window = new window_dx;
	new_window->LASTWIDTH = WIDTH;
	new_window->LASTHEIGHT = HEIGHT;
	new_window->DISPLAYMODE = Mode;
	new_window->MONITOR = (monitor_dx*)monitor;
	new_window->NAME = NAME;
	new_window->resize_cb = ResizeCB;
	new_window->UserPTR = UserPointer;

	//Check and Report if GLFW fails
	if (glfw_window == NULL) {
		printer(result_tgfx_FAIL, "VulkanCore: We failed to create the window because of GLFW!");
		delete new_window;
		return;
	}

	new_window->Window_Handle = glfwGetWin32Window(new_window->GLFWHANDLE);
	if (!new_window->Window_Handle) {
		printer(result_tgfx_FAIL, "GLFW failed to get the HWDN handle of the window!");
		delete new_window;
		return;
	}

	if (ResizeCB) {
		glfwSetWindowUserPointer(new_window->GLFWHANDLE, new_window);
		glfwSetWindowSizeCallback(new_window->GLFWHANDLE, GLFWwindowresizecallback);
	}

	//Create Swapchain
	texture_dx* SWPCHNTEXTUREHANDLESVK[2];
	if (!Create_WindowSwapchain(new_window, new_window->LASTWIDTH, new_window->LASTHEIGHT, &new_window->Swapchain_Handle, SWPCHNTEXTUREHANDLESVK)) {
		printer(result_tgfx_FAIL, "Window's swapchain creation has failed, so window's creation too!");
		glfwDestroyWindow(new_window->GLFWHANDLE);
		delete new_window;
	}
	SWPCHNTEXTUREHANDLESVK[0]->USAGE = new_window->SWAPCHAINUSAGE;
	SWPCHNTEXTUREHANDLESVK[1]->USAGE = new_window->SWAPCHAINUSAGE;
	new_window->Swapchain_Textures[0] = SWPCHNTEXTUREHANDLESVK[0];
	new_window->Swapchain_Textures[1] = SWPCHNTEXTUREHANDLESVK[1];
	SwapchainTextureHandles[0] = (texture_tgfx_handle)new_window->Swapchain_Textures[0];
	SwapchainTextureHandles[1] = (texture_tgfx_handle)new_window->Swapchain_Textures[1];


	*window = (window_tgfx_handle)new_window;
}

void core_functions_dx::Check_Computer_Specs() {
	UINT createFactoryFlags = 0;
#if defined(TD3D12_DEBUGGING)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	printer(result_tgfx_WARNING, "D3D12 backend only uses the GPU with highest Dedicated Video Memory size right now!");

	ComPtr<IDXGIAdapter4> finalAdapter;

	//Loop over all adapters
	ComPtr<IDXGIAdapter1> currentAdapter;
	SIZE_T maxDedicatedVideoMemory = 0;
	for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &currentAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
		currentAdapter->GetDesc1(&dxgiAdapterDesc1);

		if (dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { continue; }
		if (!SUCCEEDED(D3D12CreateDevice(currentAdapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr))) {
			printer(result_tgfx_FAIL, "Current GPU doesn't support D3D_FEATURE_LEVEL_12_0 so skipped it.");
			continue;
		}
		// Check to see if the adapter can create a D3D12 device without actually 
		// creating it. The adapter with the largest dedicated video memory
		// is favored.
		if (dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
			ThrowIfFailed(currentAdapter.As(&finalAdapter));
		}
	}

	ComPtr<ID3D12Device2> finalLogicalDevice;
	ThrowIfFailed(D3D12CreateDevice(finalAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&finalLogicalDevice)));

#ifdef TD3D12_DEBUGGING
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(finalLogicalDevice.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif


	ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;
	ThrowIfFailed(finalLogicalDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

}

void core_functions_dx::Setup_Debugging() {
	// Always enable the debug layer before doing anything DX12 related
	// so all possible errors generated while creating DX12 objects
	// are caught by the debug layer.
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
} 