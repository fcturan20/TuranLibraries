#define TCORE_USE_CPP_WRAPPER
#include <TGfxCore.h>

#include <assert.h>
#include <algorithm>
#include <string>

#include <TGfxRenderer.h>
#include <TGfxGpuContentManager.h>

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TGfx)
TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TGfx)
TCORE_PLUGIN_ENTRY_POINT_END()

namespace TGFX
{

static class TGfxContext* GContext = nullptr;
class TGfxContext
{
public:
	const char* BackendPluginName = nullptr;
	static TCResult RegisterBackend(const char* backendName)
	{
		TGfxBackendFunctions* backendFuncs = nullptr;
		if (auto res = TC->GetPlugin(backendName, 0, nullptr, (const void**)&backendFuncs);
			res != TC_RESULTSTATE_SUCCESS)
			return res;
		if (auto res = backendFuncs->Hook((ITGfx*)TGfx); res != TC_RESULTSTATE_SUCCESS)
			return res;
		if (auto res = ValidateInterfaces(); res != TC_RESULTSTATE_SUCCESS)
			return res;
		GContext->BackendPluginName = backendName;
		return {TC_RESULTSTATE_SUCCESS, 0};
	}
	static TCResult ValidateInterfaces()
	{
		if (!TGfx || !TGfx->Renderer || !TGfx->ResourceManager)
			return {TC_RESULTSTATE_FAILURE, 0};
		if (TGfx->RegisterBackend != RegisterBackend)
			return {TC_RESULTSTATE_FAILURE, 0};
		if (!TGfx->ChangeSwapchainResolution || !TGfx->CreateSwapchain || !TGfx->GetCurrentSwapchainTextureIndex)
			return {TC_RESULTSTATE_FAILURE, 0};
		return {TC_RESULTSTATE_SUCCESS, 0};
	}
};

struct LogEntry
{
	TCResultState State;
	const char* Message;
	LogEntry(TCResultState r, const char* m) : State(r), Message(m) {}
};

static LogEntry GTGfxLogs[]{
	{TC_RESULTSTATE_UNIMPLEMENTED, "NO_OUTPUT_CODE!"},
	{TC_RESULTSTATE_UNIMPLEMENTED, "Backend needs virmemsys_tapi, init has failed"},
	{TC_RESULTSTATE_FAILURE, "Backend specific error"},
	{TC_RESULTSTATE_FAILURE, "Backend page allocation fail!"},
	{TC_RESULTSTATE_FAILURE, "Backend memory allocator is at max, please report this"},
	{TC_RESULTSTATE_FAILURE, "Backend tried to free a suballocation wrong, please report this"},
	{TC_RESULTSTATE_FAILURE, "Backend's suballoc isn't enough for the alloc, please report this"},
	{TC_RESULTSTATE_FAILURE, "Backend failed to create logical device"},
	{TC_RESULTSTATE_FAILURE, "Extension isn't supported by the GPU!"},
	{TC_RESULTSTATE_FAILURE, "Windowing system isn't supported by your system with this backend"},
	{TC_RESULTSTATE_SUCCESS, "System doesn't support display"},
	{TC_RESULTSTATE_INVALID_ARGUMENT, "Invalid object handle"},
	{TC_RESULTSTATE_FAILURE, "There are more binding tables than supported!"},
	{TC_RESULTSTATE_FAILURE, "No descset is found!"},
	{TC_RESULTSTATE_FAILURE, "Subpass handle isn't valid!"},
	{TC_RESULTSTATE_FAILURE, "Object handle's type didn't match!"},
	{TC_RESULTSTATE_FAILURE, "Backend specific error"},
	{TC_RESULTSTATE_FAILURE, "System doesn't support the backend"},
	{TC_RESULTSTATE_FAILURE, "Windowing system failed to create the window"},
	{TC_RESULTSTATE_FAILURE, "Swapchain creation failed"},
	{TC_RESULTSTATE_SUCCESS, "System doesn't support raw mouse input mode!"},
	{TC_RESULTSTATE_SUCCESS, "One of the monitors have invalid physical sizes, be carefu"},
	{TC_RESULTSTATE_FAILURE, "Failed to signal fence on CPU!"},
	{TC_RESULTSTATE_FAILURE, "Texture creation has failed at backend object creation"},
	{TC_RESULTSTATE_FAILURE, "Created object requires a dedicated allocation but you didn't"},
	{TC_RESULTSTATE_FAILURE, "Texture creation has failed because mip count of the texture is wrong!"},
	{TC_RESULTSTATE_FAILURE, "Buffer creation has failed because at vkCreateBuffer()"},
	{TC_RESULTSTATE_FAILURE, "Binding table creation failed at vkCreateDescriptorPool()"},
	{TC_RESULTSTATE_INVALID_ARGUMENT, "ElementCount shouldn't be zero"},
	{TC_RESULTSTATE_INVALID_ARGUMENT, "Static sampler should only be used in sampler binding table!"},
	{TC_RESULTSTATE_FAILURE, "Binding table creation failed at vkAllocateDescriptors()"},
	{TC_RESULTSTATE_FAILURE, "Fence creation failed"},
	{TC_RESULTSTATE_FAILURE, "Fence value reading has failed"},
	{TC_RESULTSTATE_FAILURE, "Invalid shader source"},
	{TC_RESULTSTATE_FAILURE, "Shader source compilation has failed"},
	{TC_RESULTSTATE_INVALID_ARGUMENT, "Objects belong to different GPUs"},
	{TC_RESULTSTATE_FAILURE, "2 shader sources with the same type isn't supported"},
	{TC_RESULTSTATE_UNIMPLEMENTED, "Backend doesn't support this type of shader source"},
	{TC_RESULTSTATE_FAILURE, "Exceeded max supported attribute or binding count"},
	{TC_RESULTSTATE_FAILURE, "Attribute or binding index is wrong"},
	{TC_RESULTSTATE_FAILURE, "Attribute offset or stride is larger than device supports"},
	{TC_RESULTSTATE_FAILURE, "Pipeline creation failed"},
	{TC_RESULTSTATE_FAILURE, "Heap creation failed"},
	{TC_RESULTSTATE_FAILURE, "TGFX already bound the resource to its own dedicated heap"},
	{TC_RESULTSTATE_FAILURE, "Bind offset should be multiple of the resource's memory alignment"},
	{TC_RESULTSTATE_FAILURE, "Binding resource to heap has failed"},
	{TC_RESULTSTATE_FAILURE, "Heap mapping has failed"},
	{TC_RESULTSTATE_FAILURE, "Querying queue support for window has failed"},
	{TC_RESULTSTATE_FAILURE, "GPU doesn't support Compute, Graphics or Transfer; GPU isn't usable"},
	{TC_RESULTSTATE_FAILURE, "Queue submission failed"},
	{TC_RESULTSTATE_FAILURE, "Command buffer recording failed"},
	{TC_RESULTSTATE_FAILURE, "Active queue operation type isn't matching"},
	{TC_RESULTSTATE_FAILURE, "Querying texture type limits failed"},
	{TC_RESULTSTATE_SUCCESS, "Seperate depth-stencil layouts aren't supported by the GPU"},
	{TC_RESULTSTATE_SUCCESS, "Depth bounds testing isn't supported by the GPU"},
	{TC_RESULTSTATE_FAILURE, "Invalid depth attachment info"},
	{TC_RESULTSTATE_FAILURE, "Invalid indirect operation type"},
	{TC_RESULTSTATE_SUCCESS, "Backend specific warning"},
	{TC_RESULTSTATE_FAILURE, "This command can't be called in this bundle"},
	{TC_RESULTSTATE_FAILURE, "GPU doesn't support dynamic rendering, use subpass extension"}};
static constexpr TU8 GTGfxLogCount = sizeof(GTGfxLogs) / sizeof(LogEntry);

TCResultState GetResultStateByReturnCode(TU4 returnCode, const char** message)
{
	if (returnCode >= GTGfxLogCount)
	{
		*message = "There is no such log!";
		return TC_RESULTSTATE_UNIMPLEMENTED;
	}
	auto& log = GTGfxLogs[returnCode];
	if (message)
		*message = log.Message;
	return log.State;
}

} // namespace TGFX

TCResult TGfx_Initialize(const void** outPluginAPI)
{
	auto interfacesSize = sizeof(ITGfx) + sizeof(ITGfxRenderer) + sizeof(ITGfxResourceManager);
	auto interfaces = malloc(interfacesSize);
	auto ITGFX = (ITGfx*)interfaces;
	if (!ITGFX)
		return {TC_RESULTSTATE_OUT_OF_MEMORY, 0};
	ITGFX->Renderer = (ITGfxRenderer*)((char*)interfaces + sizeof(ITGfx));
	ITGFX->ResourceManager = (ITGfxResourceManager*)((char*)interfaces + sizeof(ITGfx) + sizeof(ITGfxRenderer));
	ITGFX->RegisterBackend = TGFX::TGfxContext::RegisterBackend;
	ITGFX->GetResultStateByReturnCode = TGFX::GetResultStateByReturnCode;
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TGfx_Shutdown()
{
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TGfx_OnPreShutdown()
{
	return {TC_RESULTSTATE_SUCCESS, 0};
}

void TGfx_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded)
{
	if (!isLoaded && TGFX::GContext->BackendPluginName &&
		strcmp(pluginInfo->Name, TGFX::GContext->BackendPluginName) == 0)
		TGFX::GContext->BackendPluginName = nullptr;
}