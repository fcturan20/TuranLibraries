#include <assert.h>

// Make sure that all tgfx headers are in C
extern "C" {

#define T_INCLUDE_PLATFORM_LIBS
#include <ecs_tapi.h>
#include <logger_tapi.h>
#include <predefinitions_tapi.h>
#include <string_tapi.h>

#include "tgfx_core.h"
#include "tgfx_gpucontentmanager.h"
#include "tgfx_helper.h"
#include "tgfx_imgui.h"
#include "tgfx_renderer.h"
#include "tgfx_structs.h"
}

#include <string>

static core_tgfx_type* core_typePtr;
static ecs_tapi*       core_regSys;
static logger_tapi*    core_logSys;

void defaultPrintCallback(unsigned int logCode, const wchar_t* extraInfo) {
  const wchar_t* logMessage = nullptr;
  result_tgfx    result     = core_typePtr->api->getLogMessage(logCode, &logMessage);
  core_logSys->log(( tapi_log_type )result, false, string_type_tapi_UTF16, L"TGFX -> %v %v",
                   logMessage, extraInfo);
}

static constexpr char* backendFileNames[] = {"TGFXVulkan.dll", "TGFXD3D12.dll"};
constexpr std::size_t  constexpr_slen(const char* s) { return std::char_traits<char>::length(s); }
static constexpr uint32_t maxBackendFileName =
  max(constexpr_slen(backendFileNames[0]), constexpr_slen(backendFileNames[1]));

result_tgfx load_backend(core_tgfx* parent, backends_tgfx backend, tgfx_logCallback printcallback) {
  const char* path = backendFileNames[( int )backend];

  if (!printcallback) {
    printcallback = defaultPrintCallback;
  }

  auto backend_dll = DLIB_LOAD_TAPI(path);
  if (!backend_dll) {
    printcallback(22, nullptr);
    return result_tgfx_FAIL;
  }
  backend_load_func backendloader =
    ( backend_load_func )DLIB_FUNC_LOAD_TAPI(backend_dll, "BACKEND_LOAD");
  if (!backendloader) {
    printcallback(23, nullptr);
    return result_tgfx_FAIL;
  }
  return backendloader(core_regSys, core_typePtr, printcallback);
}

result_tgfx tgfxGetLogMessage(unsigned int logCode, const wchar_t** logMessage);
ECSPLUGIN_ENTRY(ecsSys, reloadFlag) {
  core_tgfx_type* core = ( core_tgfx_type* )malloc(sizeof(core_tgfx_type) + sizeof(core_tgfx));
  if (core == NULL) {
    printf("TGFX core creation failed because malloc failed");
    exit(-1);
  }
  core->api                 = ( core_tgfx* )(core + 1);
  core_typePtr              = core;
  core_regSys               = ecsSys;
  core->api->contentmanager = new gpudatamanager_tgfx;
  core->api->helpers        = new helper_tgfx;
  core->api->imgui          = new dearimgui_tgfx;
  core->api->renderer       = new renderer_tgfx;

  core->api->load_backend  = &load_backend;
  core->api->getLogMessage = &tgfxGetLogMessage;

  ecsSys->addSystem(TGFX_PLUGIN_NAME, TGFX_PLUGIN_VERSION, core);
}
ECSPLUGIN_EXIT(ecsSys, reloadFlag) {}

typedef struct tgfxLogStruct {
  result_tgfx    result;
  const wchar_t* output;
  tgfxLogStruct(result_tgfx r, const wchar_t* o) : result(r), output(o) {}
} logStruct_tgfx;
static tgfxLogStruct tgfxLogMessages[]{
  {result_tgfx_NOTCODED, L"NO_OUTPUT_CODE!"},
  {result_tgfx_NOTCODED, L"Backend needs virmemsys_tapi, init has failed"},
  {result_tgfx_FAIL, L"Backend specific error"},
  {result_tgfx_FAIL, L"Backend page allocation fail!"},
  {result_tgfx_FAIL, L"Backend memory allocator is at max, please report this"},
  {result_tgfx_FAIL, L"Backend tried to free a suballocation wrong, please report this"},
  {result_tgfx_FAIL, L"Backend's suballoc isn't enough for the alloc, please report this"},
  {result_tgfx_FAIL, L"Backend failed to create logical device"},
  {result_tgfx_FAIL, L"Extension isn't supported by the GPU!"},
  {result_tgfx_FAIL, L"Windowing system isn't supported by your system with this backend"},
  {result_tgfx_WARNING, L"System doesn't support display"},
  {result_tgfx_INVALIDARGUMENT, L"Invalid object handle"},
  {result_tgfx_FAIL, L"There are more descsets than supported!"},
  {result_tgfx_FAIL, L"No descset is found!"},
  {result_tgfx_FAIL, L"Subpass handle isn't valid!"},
  {result_tgfx_FAIL, L"Object handle's type didn't match!"},
  {result_tgfx_FAIL, L"Backend specific error"},
  {result_tgfx_FAIL, L"System doesn't support the backend"},
  {result_tgfx_FAIL, L"Windowing system failed to create the window"},
  {result_tgfx_FAIL, L"Swapchain creation failed"},
  {result_tgfx_WARNING, L"System doesn't support raw mouse input mode!"},
  {result_tgfx_WARNING, L"One of the monitors have invalid physical sizes, be careful"},
  {result_tgfx_FAIL, L"Failed to signal fence on CPU!"},
  {result_tgfx_FAIL, L"Backend DLL file isn't found!"},
  {result_tgfx_FAIL,
   L"Backend doesn't have any backend_load() function, which it should. You are "
   L"probably using a newer version that uses different load scheme!"},
  {result_tgfx_FAIL, L"Texture creation has failed at backend object creation"},
  {result_tgfx_FAIL,
   L"Created object requires a dedicated allocation but you didn't use dedicated "
   L"allocation extension. Provide a dedicated allocation info as extension!"},
  {result_tgfx_FAIL, L"Texture creation has failed because mip count of the texture is wrong!"},
  {result_tgfx_FAIL, L"Buffer creation has failed because at vkCreateBuffer()"},
  {result_tgfx_FAIL, L"Binding table creation failed at vkCreateDescriptorPool()"},
  {result_tgfx_INVALIDARGUMENT, L"ElementCount shouldn't be zero"},
  {result_tgfx_INVALIDARGUMENT, L"Static sampler should only be used in sampler binding table!"},
  {result_tgfx_FAIL, L"Binding table creation failed at vkAllocateDescriptors()"},
  {result_tgfx_FAIL, L"Fence creation failed"},
  {result_tgfx_FAIL, L"Fence value reading has failed"},
  {result_tgfx_FAIL, L"Invalid shader source"},
  {result_tgfx_FAIL, L"Shader source compilation has failed"},
  {result_tgfx_FAIL, L"Shaders are from different GPUs"},
  {result_tgfx_FAIL, L"2 shader sources with the same type isn't supported"},
  {result_tgfx_NOTCODED, L"Backend doesn't support this type of shader source"},
  {result_tgfx_FAIL, L"Exceeded max supported attribute or binding count"},
  {result_tgfx_FAIL, L"Attribute or binding index is wrong"},
  {result_tgfx_FAIL, L"Attribute offset or stride is larger than device supports"},
  {result_tgfx_FAIL, L"Pipeline creation failed"},
  {result_tgfx_FAIL, L"Heap creation failed"},
  {result_tgfx_FAIL, L"TGFX already bound the resource to its own dedicated heap"},
  {result_tgfx_FAIL, L"Bind offset should be multiple of the resource's memory alignment"},
  {result_tgfx_FAIL, L"Binding resource to heap has failed"},
  {result_tgfx_FAIL, L"Heap mapping has failed"}};
  static constexpr uint32_t tgfxLogCount = sizeof(tgfxLogMessages) / sizeof(tgfxLogStruct);
result_tgfx               tgfxGetLogMessage(unsigned int logCode, const wchar_t** logMessage) {
  if (logCode >= tgfxLogCount) {
    *logMessage = L"There is no such log!";
    return result_tgfx_FAIL;
  }
  if (logMessage) {
    *logMessage = tgfxLogMessages[logCode].output;
  }
  return tgfxLogMessages[logCode].result;
}