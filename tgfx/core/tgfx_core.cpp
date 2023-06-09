#include <assert.h>

#define TGFX_BACKEND
#define T_INCLUDE_PLATFORM_LIBS
#include <ecs_tapi.h>
#include <logger_tapi.h>
#include <predefinitions_tapi.h>
#include <string_tapi.h>

#include <string>

#include "tgfx_core.h"
#include "tgfx_forwarddeclarations.h"
#include "tgfx_gpucontentmanager.h"
#include "tgfx_helper.h"
#include "tgfx_imgui.h"
#include "tgfx_renderer.h"
#include "tgfx_structs.h"

static tgfx_core_type*    core_typePtr;
static const tapi_ecs*    core_regSys;
static const tapi_logger* core_logSys;

void defaultPrintCallback(unsigned int logCode, const wchar_t* extraInfo) {
  const wchar_t* logMessage = nullptr;
  result_tgfx    result     = core_typePtr->api->getLogMessage(logCode, &logMessage);
  tapi_log_type  logType    = log_type_tapi_STATUS;
  switch (result) {
    case result_tgfx_SUCCESS: logType = log_type_tapi_STATUS; break;
    case result_tgfx_FAIL:
    case result_tgfx_INVALIDARGUMENT:
    case result_tgfx_WRONGTIMING: logType = log_type_tapi_ERROR; break;
    case result_tgfx_NOTCODED: logType = log_type_tapi_NOTCODED; break;
    case result_tgfx_WARNING: logType = log_type_tapi_WARNING; break;
  }
  if (extraInfo) {
    core_logSys->log(logType, false, L"[TGFX] %v; %v", logMessage, extraInfo);
  } else {
    core_logSys->log(logType, false, L"[TGFX] %v", logMessage);
  }
}

static constexpr char* backendFileNames[] = {"", "TGFXVulkan.dll", "TGFXD3D12.dll"};
constexpr std::size_t  constexpr_slen(const char* s) { return std::char_traits<char>::length(s); }
static constexpr uint32_t maxBackendFileName =
  max(constexpr_slen(backendFileNames[0]), constexpr_slen(backendFileNames[1]));

typedef result_tgfx (*load_backend_fnc)(const struct tapi_ecs* ecsSys, struct tgfx_core_type* core,
                                    void (*printFnc)(unsigned int   logCode,
                                                     const wchar_t* extraInfo));
result_tgfx load_backend(tgfx_core* parent, backends_tgfx backend,
                         void (*printFnc)(unsigned int logCode, const wchar_t* extraInfo)) {
  const char* path = backendFileNames[( int )backend];

  if (!printFnc) {
    printFnc = defaultPrintCallback;
  }

  auto backend_dll = DLIB_LOAD_TAPI(path);
  if (!backend_dll) {
    printFnc(22, nullptr);
    return result_tgfx_FAIL;
  }
  load_backend_fnc backendloader =
    ( load_backend_fnc )DLIB_FUNC_LOAD_TAPI(backend_dll, "BACKEND_LOAD");
  if (!backendloader) {
    printFnc(23, nullptr);
    return result_tgfx_FAIL;
  }
  return backendloader(core_regSys, core_typePtr, printFnc);
}

result_tgfx tgfxGetLogMessage(unsigned int logCode, const wchar_t** logMessage);
ECSPLUGIN_ENTRY(ecsSys, reloadFlag) {
  tgfx_core_type* core = ( tgfx_core_type* )malloc(sizeof(tgfx_core_type) + sizeof(tgfx_core));
  if (core == NULL) {
    printf("TGFX core creation failed because malloc failed");
    exit(-1);
  }
  core->api                 = ( tgfx_core* )(core + 1);
  core_typePtr              = core;
  core_regSys               = ecsSys;
  core->api->contentmanager = new tgfx_gpuDataManager;
  core->api->helpers        = new tgfx_helper;
  core->api->imgui          = new tgfx_dearImgui;
  core->api->renderer       = new tgfx_renderer;
  core_logSys =
    (( LOGGER_TAPI_PLUGIN_LOAD_TYPE )core_regSys->getSystem(LOGGER_TAPI_PLUGIN_NAME))->funcs;

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
  {result_tgfx_FAIL, L"There are more binding tables than supported!"},
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
  {result_tgfx_FAIL, L"Backend's loading scheme isn't supported"},
  {result_tgfx_FAIL, L"Texture creation has failed at backend object creation"},
  {result_tgfx_FAIL, L"Created object requires a dedicated allocation but you didn't"},
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
  {result_tgfx_INVALIDARGUMENT, L"Objects are from different GPUs"},
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
  {result_tgfx_FAIL, L"Heap mapping has failed"},
  {result_tgfx_INVALIDARGUMENT, L"Fix your function arguments or report this!"},
  {result_tgfx_FAIL, L"Querying queue support for window has failed"},
  {result_tgfx_FAIL, L"GPU doesn't support Compute, Graphics or Transfer; GPU isn't usable"},
  {result_tgfx_FAIL, L"Queue submission failed"},
  {result_tgfx_FAIL, L"Command buffer recording failed"},
  {result_tgfx_FAIL, L"Active queue operation type isn't matching"},
  {result_tgfx_FAIL, L"Querying texture type limits failed"},
  {result_tgfx_WARNING, L"Seperate depth-stencil layouts aren't supported by the GPU"},
  {result_tgfx_WARNING, L"Depth bounds testing isn't supported by the GPU"},
  {result_tgfx_FAIL, L"Invalid depth attachment info"},
  {result_tgfx_FAIL, L"Invalid indirect operation type"},
  {result_tgfx_WARNING, L"Backend specific warning"},
  {result_tgfx_FAIL, L"This command can't be called in this bundle"},
  {result_tgfx_FAIL, L"GPU doesn't support dynamic rendering, use subpass extension"}};
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